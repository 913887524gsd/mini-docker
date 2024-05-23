#include <config.h>
#include <typedef.h>
#include <setup.h>
#include <util.h>

#include <fcntl.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <map>

static void __atexit_report_error(int, void *str)
{
    fprintf(stderr, "%s", (const char *)str);
    free(str);
}

static void atexit_report_error(const char *fmt)
{
    char *str = NULL;
    asprintf(&str, fmt, strerror(errno));
    on_exit(__atexit_report_error, str);
}

static void setup_iptables(const char *ip_addr, int hp, int cp)
{
    char *comm;
    do {
        comm = NULL;
        errexit(asprintf(&comm, "iptables -t nat -A %s -j DNAT -p tcp ! -i %s --dport %d --to-destination %s:%d",
            IPTABLE_CHAIN, BRIDGE_DEV, hp, ip_addr, cp));
        errexit(system(comm));
        free(comm);
        comm = NULL;
        errexit(asprintf(&comm, "iptables -t nat -D %s -j DNAT -p tcp ! -i %s --dport %d --to-destination %s:%d",
            IPTABLE_CHAIN, BRIDGE_DEV, hp, ip_addr, cp));
        errexit(on_exit(atexit_do_command, comm));
    } while (0);
    do {
        comm = NULL;
        errexit(asprintf(&comm, "iptables -t nat -A POSTROUTING -j MASQUERADE -p tcp -s %s -d %s --dport %d",
            ip_addr, ip_addr, cp));
        errexit(system(comm));
        free(comm);
        comm = NULL;
        errexit(asprintf(&comm, "iptables -t nat -D POSTROUTING -j MASQUERADE -p tcp -s %s -d %s --dport %d",
            ip_addr, ip_addr, cp));
        errexit(on_exit(atexit_do_command, comm));
    } while (0);
}

static int set_non_block(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        fprintf(stderr, "fcntl failed: %s\n", strerror(errno));
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        fprintf(stderr, "fcntl failed: %s\n", strerror(errno));
        return -1;
    } 
    return 0;
}

static int create_signal_fd(void)
{
    int fd;
    sigset_t sig;
    sigfillset(&sig);
    if (sigprocmask(SIG_BLOCK, &sig, NULL) == -1) {
        fprintf(stderr, "sigprocmask failed: %s\n", strerror(errno));
        return -1;
    }
    sigemptyset(&sig);
    sigaddset(&sig, SIGTERM);
    if ((fd = signalfd(-1, &sig, SFD_NONBLOCK)) == -1) {
        fprintf(stderr, "signalfd failed: %s\n", strerror(errno));
        return -1;
    }
    return fd;
}

static int create_listen_fd(int port)
{
    int fd;
    struct sockaddr_in addr;
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        fprintf(stderr, "socket failed: %s\n", strerror(errno));
        return -1;
    }
    if (set_non_block(fd) == -1) {
        fprintf(stderr, "set_non_block: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        fprintf(stderr, "bind failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    if (listen(fd, 5) == -1) {
        fprintf(stderr, "listen failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static int create_reciever_fd(int listen_fd)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr); 
    
    if ((fd = accept4(listen_fd, (struct sockaddr *)&addr, &addr_len, SOCK_NONBLOCK)) == -1) {
        return -1;
    }
    return fd;
}

static int create_sender_fd(const char *ip_addr, int port)
{
    int fd;
    struct sockaddr_in addr;
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        return -1;
    }
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_addr, &addr.sin_addr.s_addr) == -1) {
        close(fd);
        return -1;
    }
    addr.sin_port = htons(port);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(fd);
        return -1;
    }
    if (set_non_block(fd) == -1) {
        close(fd);
        return -1;
    }
    return fd;
}

static int add_to_epoll(int epoll_fd, int fd, int events)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

static int socket_transmit(int send_fd, int recv_fd)
{
    int write_tries;
    ssize_t readc, writec;
    char buf[512];
    while (1) {
        do {
            if ((readc = read(recv_fd, buf, 512)) == -1) {
                if (errno == EINTR) {
                    continue;
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return 0; // finished
                } else {
                    return -1;
                }
            } else {
                break;
            }
        } while (1);
        
        if(readc == 0)
            return 1;
        write_tries = 0;

        do {
            if ((writec = write(send_fd, buf, readc)) == -1) {
                if (errno == EINTR) {
                    continue;
                } else {
                    return -1;
                }
            } else {
                if (writec == 0) {
                    return 1;
                } else if (++write_tries == 10) {
                    return -1;
                } else if (writec < readc) {
                    memmove(buf, buf + writec, readc - writec);
                    readc -= writec;
                } else {
                    break;
                }
            }
        } while (1);
    }
}

// trasmit port have to abort
// for transmit threads, it's a big problem to report error
// so it's need to wait for main thread to exit(get back terminal control)
// and then report thread's error
// 0 -> const char *ip_addr
// 1 -> size_t host port
// 2 -> size_t container port
static void *port_transmit(void *arg)
{
    const char *ip_addr;
    int hp, cp;
    int sigfd = -1, listen_fd = -1, epoll_fd = -1;
    int signaled = 0;
    std::map<int, int> peerfd;

    ip_addr = ((const char **)arg)[0];
    hp = ((size_t *)arg)[1];
    cp = ((size_t *)arg)[2];
    free(arg);

    if ((sigfd = create_signal_fd()) == -1) {
        atexit_report_error("create_signal_fd failed: %s\n");
        goto error;
    }
    if ((listen_fd = create_listen_fd(hp)) == -1) {
        atexit_report_error("create_listen_fd failed: %s\n");
        goto error;
    }
    if ((epoll_fd = epoll_create1(0)) == -1) {
        atexit_report_error("epoll_create1 failed: %s\n");
        goto error;
    }
    if (add_to_epoll(epoll_fd, sigfd, EPOLLIN) == -1) {
        atexit_report_error("add_to_epoll failed: %s\n");
        goto error;
    }
    if (add_to_epoll(epoll_fd, listen_fd, EPOLLIN) == -1) {
        atexit_report_error("add_to_epoll failed: %s\n");
        goto error;
    }
    while (!signaled) {
        int nfds;
        struct epoll_event evs[10];
        
        if ((nfds = epoll_wait(epoll_fd, evs, 10, -1)) == -1) {
            if (errno == EINTR)
                continue;
            atexit_report_error("epoll_wait failed: %s\n");
            goto error;
        }
        
        for (int i = 0 ; i < nfds ; i++) {
            struct epoll_event *ev = &evs[i];
            if (ev->data.fd == listen_fd) {
                int recv_fd, send_fd;
                if ((recv_fd = create_reciever_fd(listen_fd)) == -1)
                    continue;
                if ((send_fd = create_sender_fd(ip_addr, cp)) == -1) {
                    close(recv_fd);
                    continue;
                }
                peerfd[recv_fd] = send_fd;
                peerfd[send_fd] = recv_fd;
                if (add_to_epoll(epoll_fd, send_fd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP) == -1) {
                    atexit_report_error("add_to_epoll failed: %s\n");
                    goto error;
                }
                if (add_to_epoll(epoll_fd, recv_fd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP) == -1) {
                    atexit_report_error("add_to_epoll failed: %s\n");
                    goto error;
                }
            } else if (ev->data.fd == sigfd) {
                struct signalfd_siginfo info;
                read(sigfd, &info, sizeof(info));
                if (info.ssi_signo == SIGTERM)
                    signaled = 1;
            } else {
                int closed = 0;
                int recv_fd, send_fd;
                
                recv_fd = ev->data.fd;
                send_fd = peerfd[recv_fd];
                if (ev->events & (EPOLLRDHUP | EPOLLHUP)) {
                    closed = 1;
                } else {
                    int ret = socket_transmit(send_fd, recv_fd);
                    if (ret == -1) {
                        atexit_report_error("socket_transmit failed: %s\n");
                        goto error;
                    } else if (ret == 1) {
                        closed = 1;
                    }
                }
                if (closed) {
                    if (peerfd.find(recv_fd) == peerfd.end())
                        continue; // reciever and sender are all closed
                    peerfd.erase(recv_fd);
                    peerfd.erase(send_fd);
                    close(recv_fd);
                    close(send_fd);
                }
            }
        }
    }
    
    goto free;
error:
    kill(getpid(), SIGTERM);
free:
    for (auto it : peerfd) {
        close(it.first);
    }
    peerfd.clear();
    if (sigfd != -1)
        close(sigfd);
    if (listen_fd != -1)
        close(listen_fd);
    if (epoll_fd != -1)
        close(epoll_fd);
    return NULL;
}

static void kill_port_transmit(int, void *arg)
{
    int err;
    pthread_t tid = (pthread_t)arg;
    pthread_kill(tid, SIGTERM);
    if ((err = pthread_join(tid, NULL)) != 0) {
        fprintf(stderr, "pthread_join failed: %s\n", strerror(err));
    }
}

static void free_ip_addr(int, void *ip_addr)
{
    free(ip_addr);
}

void setup_port(const char *__ip_addr)
{
    if (ports.empty())
        return;
    char *ip_addr = strdup(__ip_addr);
    errexit(on_exit(free_ip_addr, ip_addr));
    for (auto [hp,cp] : ports) {
        pthread_t tid;
        size_t *arg = (size_t *)malloc(3 * sizeof(size_t));
        arg[0] = (size_t)ip_addr;
        arg[1] = hp;
        arg[2] = cp;
        setup_iptables(ip_addr, hp, cp);
        pthread_create(&tid, NULL, port_transmit, arg);
        errexit(on_exit(kill_port_transmit, (void *)tid));
    }
}