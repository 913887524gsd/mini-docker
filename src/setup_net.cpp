#include <config.h>
#include <typedef.h>
#include <setup.h>
#include <IPC.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <net/if.h>
#include <sys/stat.h>
#include <linux/rtnetlink.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/msg.h>
#include <netlink/handlers.h>
#include <netlink/cache.h>
#include <netlink/route/route.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/route/link/veth.h>

static void delete_ip(int, void *filepath)
{
    errexit(unlink((const char *)filepath));
    free(filepath);
}

static int check_ip_in_use(const char *ip_addr)
{
    char *filepath = NULL;
    errexit(asprintf(&filepath, "%s/%s", NET_DIR, ip_addr));
    int fd = open(filepath, O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        return -1;
    } else {
        close(fd);
        errexit(on_exit(delete_ip, filepath));
        return 0;
    }
}

static const char *random_string(const char *prefix, int length)
{
    int prefix_length = strlen(prefix);
    char *str = (char *)malloc(prefix_length + length + 1);
    strcpy(str, prefix);
    for (int i = 0 ; i < length ; i++) {
        static const char *dict = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        str[prefix_length + i] = dict[rand() % 52];
    }
    str[prefix_length + length] = '\0';
    return str;
}

static int create_veth(struct nl_sock *sock, pid_t pid, const char **veth)
{
    struct rtnl_link *link, *peer;
    int err;

    link = rtnl_link_veth_alloc();
    if (!link) {
        fprintf(stderr, "rtnl_link_veth_alloc failed\n");
        return -1;
    }
    peer = rtnl_link_veth_get_peer(link);
    rtnl_link_set_ns_pid(peer, pid);
    
    int count = 0;
    do {
        const char *name[2];
        name[0] = random_string("veth", 6);
        name[1] = random_string("veth", 6);
        rtnl_link_set_name(link, name[0]);
        rtnl_link_set_name(peer, name[1]);
        fprintf(stderr, "veth %s<->%s\n", name[0], name[1]);
        if ((err = rtnl_link_add(sock, link, NLM_F_CREATE | NLM_F_EXCL)) < 0) {
            fprintf(stderr, "failed to create veth pair: %s\n", nl_geterror(err));
        }
        if (err != 0 && ++count >= 10) {
            fprintf(stderr, "failed to create veth pair\n");
            rtnl_link_put(link);
            rtnl_link_put(peer);
            return -1;
        }
        free((void *)name[0]);
        free((void *)name[1]);
    } while (err != 0);

    veth[0] = strdup(rtnl_link_get_name(link));
    veth[1] = strdup(rtnl_link_get_name(peer));
    rtnl_link_put(link);
    rtnl_link_put(peer);
    return 0;
}

static int netdev_bind_address(
    struct nl_sock *sock, 
    const char *if_name, 
    const char *ip_addr, 
    int prefix_len
)
{
    struct rtnl_addr *addr;
    struct nl_addr *local_addr;
    int err, if_index;

    if ((if_index = if_nametoindex(if_name)) == 0) {
        fprintf(stderr, "if_nametoindex failed: %s\n", strerror(errno));
        return -1;
    }

    if ((addr = rtnl_addr_alloc()) == NULL) {
        fprintf(stderr, "rtnl_addr_alloc failed\n");
        return -1;
    }
    rtnl_addr_set_ifindex(addr, if_index);

    if ((err = nl_addr_parse(ip_addr, AF_INET, &local_addr)) < 0) {
        fprintf(stderr, "nl_addr_parse failed: %s, %s\n", ip_addr, nl_geterror(err));
        rtnl_addr_put(addr);
        return -1;
    }
    rtnl_addr_set_local(addr, local_addr);
    nl_addr_put(local_addr);
    rtnl_addr_set_prefixlen(addr, prefix_len);
    
    if ((err = rtnl_addr_add(sock, addr, NLM_F_EXCL)) < 0) {
        fprintf(stderr, "rtnl_addr_add failed: %s\n", nl_geterror(err));
        rtnl_addr_put(addr);
        return -1;
    }
    rtnl_addr_put(addr);

    return 0;
}

static int netdev_bind_bridge(struct nl_sock *sock, const char *if_name)
{
    struct rtnl_link *link, *change;
    int err, if_index, master_if_index;

    if ((if_index = if_nametoindex(if_name)) == 0) {
        fprintf(stderr, "if_nametoindex failed: %s\n", strerror(errno));
        return -1;
    }
    if ((master_if_index = if_nametoindex(NETBRIDGE_DEV)) == 0) {
        fprintf(stderr, "if_nametoindex failed: %s\n", strerror(errno));
        return -1;
    }
    
    if ((err = rtnl_link_get_kernel(sock, if_index, if_name, &link)) < 0) {
        fprintf(stderr, "rtnl_link_get_kernel failed: %s\n", nl_geterror(err));
        return -1;
    }
    if ((change = rtnl_link_alloc()) == NULL) {
        fprintf(stderr, "rtnl_link_alloc failed");
        rtnl_link_put(link);
        return -1;
    }
    rtnl_link_set_master(change, master_if_index);

    if ((err = rtnl_link_change(sock, link, change, 0)) < 0) {
        fprintf(stderr, "rtnl_link_change failed: %s\n", nl_geterror(err));
        rtnl_link_put(link);
        rtnl_link_put(change);
        return -1;
    }
    
    rtnl_link_put(link);
    rtnl_link_put(change);
    return 0;
}

static int netdev_set_up(struct nl_sock *sock, const char *if_name)
{
    struct rtnl_link *link, *change;
    int if_index, err;

    if ((if_index = if_nametoindex(if_name)) == 0) {
        fprintf(stderr, "if_nametoindex failed: %s\n", strerror(errno));
        return -1;
    }

    if ((err = rtnl_link_get_kernel(sock, if_index, if_name, &link)) < 0) {
        fprintf(stderr, "rtnl_link_get_kernel failed: %s\n", nl_geterror(err));
        return -1;
    }
    if ((change = rtnl_link_alloc()) == NULL) {
        fprintf(stderr, "rtnl_link_alloc failed");
        rtnl_link_put(link);
        return -1;
    }
    rtnl_link_set_flags(change, IFF_UP);

    if ((err = rtnl_link_change(sock, link, change, 0)) < 0) {
        fprintf(stderr, "rtnl_link_change failed: %s\n", nl_geterror(err));
        rtnl_link_put(link);
        rtnl_link_put(change);
        return -1;
    }
    
    rtnl_link_put(link);
    rtnl_link_put(change);
    return 0;
}

static int setup_default_route(struct nl_sock *sock, const char *if_name)
{
    struct rtnl_route *route;
    struct rtnl_nexthop *nh;
    struct nl_addr *dst, *gw;
    int err, if_index;

    if ((if_index = if_nametoindex(if_name)) == 0) {
        fprintf(stderr, "if_nametoindex failed: %s\n", strerror(errno));
        return -1;
    }

    if ((route = rtnl_route_alloc()) == NULL) {
        fprintf(stderr, "rtnl_route_alloc failed\n");
        return -1;
    }

    if ((nh = rtnl_route_nh_alloc()) == NULL) {
        fprintf(stderr, "rtnl_route_nh_alloc failed\n");
        rtnl_route_put(route);
        return -1;
    }

    nl_addr_parse("0.0.0.0/0", AF_INET, &dst);
    rtnl_route_set_dst(route, dst);

    nl_addr_parse(NETBRIDGE_ADDR, AF_INET, &gw);
    rtnl_route_nh_set_gateway(nh, gw);
    
    rtnl_route_nh_set_ifindex(nh, if_index);
    rtnl_route_add_nexthop(route, nh);

    if ((err = rtnl_route_add(sock, route, 0)) < 0) {
        fprintf(stderr, "rtnl_route_add failed: %s\n", nl_geterror(err));
        nl_addr_put(gw);
        nl_addr_put(dst);
        rtnl_route_put(route);
        return -1;
    }

    nl_addr_put(gw);
    nl_addr_put(dst);
    rtnl_route_put(route);
    return 0;
}

void host_setup_net(pid_t pid)
{
    struct nl_sock *sock;
    const char *veth[2];
    char *ip_addr = NULL;
    int err;
    
    if ((sock = nl_socket_alloc()) == NULL) {
        fprintf(stderr, "nl_socket_alloc failed\n");
        goto err0;
    }
    if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
        fprintf(stderr, "nl_connect failed: %s\n", nl_geterror(err));
        goto err1;
    }
    if (create_veth(sock, pid, veth) < 0) {
        fprintf(stderr, "create_veth failed\n");
        goto err2;
    }
    
    for (int ip = 2 ; ip <= 254 ; ip++) {
        static_assert(SUBNET_PREFIX_LEN == 24);
        errexit(asprintf(&ip_addr, "%s.%d", SUBNET_PREFIX, ip));
        if (check_ip_in_use(ip_addr) == 0) {
            break;
        } else {
            if (errno != EEXIST) {
                fprintf(stderr, "check_ip_in_use failed: %s\n", strerror(errno));
                goto err3;
            }
        }
        free(ip_addr);
        ip_addr = NULL;
    }
    if (ip_addr == NULL) {
        fprintf(stderr, "netdev_bind_address failed\n");
        goto err3;
    }
    if (IPC_send(veth[1], strlen(veth[1]) + 1) < 0) {
        fprintf(stderr, "IPC_send failed: %s\n", strerror(errno));
        goto err4;   
    }
    if (IPC_send(ip_addr, strlen(ip_addr) + 1) < 0) {
        fprintf(stderr, "IPC_send failed: %s\n", strerror(errno));
        goto err4;   
    }
    if (netdev_bind_bridge(sock, veth[0]) == -1) {
        fprintf(stderr, "netdev_bind_bridge failed\n");
        goto err4;
    }
    if (netdev_set_up(sock, veth[0]) < 0) {
        fprintf(stderr, "netdev_set_up failed\n");
        goto err4;
    }
    free(ip_addr);
    free((void *)veth[0]);
    free((void *)veth[1]);
    nl_close(sock);
    nl_socket_free(sock);
    return;
err4:
    free(ip_addr);
err3:
    free((void *)veth[0]);
    free((void *)veth[1]);
err2:
    nl_close(sock);
err1:
    nl_socket_free(sock);
err0:
    exit(EXIT_FAILURE);
}

void container_setup_net(void)
{
    struct nl_sock *sock;
    const char *veth = NULL;
    const char *ip_addr = NULL;
    int err;

    if ((sock = nl_socket_alloc()) == NULL) {
        fprintf(stderr, "nl_socket_alloc failed\n");
        goto err0;
    }
    if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
        fprintf(stderr, "nl_connect failed: %s\n", nl_geterror(err));
        goto err1;
    }
    if (IPC_recv((const void **)&veth) < 0) {
        fprintf(stderr, "IPC_recv failed: %s\n", strerror(errno));
        goto err2;
    }
    if (IPC_recv((const void **)&ip_addr) < 0) {
        fprintf(stderr, "IPC_recv failed: %s\n", strerror(errno));
        goto err3;
    }
    if (netdev_bind_address(sock, veth, ip_addr, SUBNET_PREFIX_LEN) == -1) {
        fprintf(stderr, "netdev_bind_address failed\n");
        goto err4;
    }
    if (netdev_set_up(sock, veth) < 0 || netdev_set_up(sock, "lo") < 0) {
        fprintf(stderr, "netdev_set_up failed\n");
        goto err4;
    }
    if (setup_default_route(sock, veth) < 0) {
        fprintf(stderr, "setup_default_route failed\n");
        goto err4;
    }
    free((void *)ip_addr);
    free((void *)veth);
    nl_close(sock);
    nl_socket_free(sock);
    return;
err4:
    free((void *)ip_addr);
err3:
    free((void *)veth);
err2:
    nl_close(sock);
err1:
    nl_socket_free(sock);
err0:
    exit(EXIT_FAILURE);
}