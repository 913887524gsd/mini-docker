#include <config.h>
#include <typedef.h>
#include <setup.h>
#include <IPC.h>
#include <util.h>
#include <tclap/CmdLine.h>

#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>

char *imagedir;

std::vector<std::pair<std::string, std::string>> volumn;
double cpu_limit;
size_t memory_limit;
size_t blkio_limit;
std::string image;
std::vector<std::string> command;

static void init_runtime_enviroment(void)
{
    errexit(mkdir_recursive(IMAGE_DIR, 0755));
    errexit(mkdir_recursive(RUNTIME_DIR, 0755));
    errexit(mkdir_recursive(NET_DIR, 0755));
    if (mkdir(CGROUP_RUNTIME_DIR(CPU), 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "mkdir failed: %s\n", strerror(errno));
    }
    if (mkdir(CGROUP_RUNTIME_DIR(MEM), 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "mkdir failed: %s\n", strerror(errno));
    }
    if (mkdir(CGROUP_RUNTIME_DIR(BLK), 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "mkdir failed: %s\n", strerror(errno));
    }
    // ignore SIGTTOU for tcsetpgrp use
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = SIG_IGN;
    errexit(sigaction(SIGTTOU, &act, NULL));
}

static bool is_valid_scale_arg(const std::string &arg, size_t *ret)
{
    try {
        if (arg == "") {
            *ret = 0;
            return true;
        }
        switch (*arg.rbegin()) {
            case 'k': case 'K':
            case 'm': case 'M':
            case 'g': case 'G':
                break;
            default:
                throw std::invalid_argument("error");
                break;
        }
        size_t pos;
        char scale = *arg.rbegin();
        std::string __arg = arg;
        __arg.pop_back();
        *ret = std::stoul(__arg, &pos);
        if (scale == 'k' || scale == 'K')
            *ret *= 1024;
        else if (scale == 'm' || scale == 'M')
            *ret *= 1024 * 1024;
        else if (scale == 'g' || scale == 'G')
            *ret *= 1024 * 1024 * 1024;
        else
            assert(0);
        return pos == __arg.size();
    } catch (const std::invalid_argument& e) {
        return false;
    } catch (const std::out_of_range& e) {
        return false;
    }
}

static void arg_parse(int argc, char *argv[])
{
    try {
        TCLAP::CmdLine cmd("a mini docker for study use");

        TCLAP::MultiArg<std::string> __volumn(
            "v", "volumn", "bind mount (usage: hostpath:containerpath)",
            false, "string"
        );
        cmd.add(__volumn);
        TCLAP::ValueArg<double> __cpu(
            "c", "cpu", "cpu limit(for example 0.5 means container will use at most 0.5 cpu cores' performance)",
            false, -1, "float"
        );
        cmd.add(__cpu);
        TCLAP::ValueArg<std::string> __memory(
            "m", "memory", "memory limit(a integer with a scale identifier:k/m/g)",
            false, "", "string"
        );
        cmd.add(__memory);
        TCLAP::ValueArg<std::string> __blkio(
            "b", "blkio", "blkio limit, both read/write(a integer with a scale identifier:k/m/g)",
            false, "", "string"
        );
        cmd.add(__blkio);
        TCLAP::UnlabeledValueArg<std::string> __image(
            "Image", "image name for new container", true, "", "string"
        );
        cmd.add(__image);
        TCLAP::UnlabeledMultiArg<std::string> __command(
            "Command", "command to run in container", true, "string"
        );
        cmd.add(__command);
        cmd.parse(argc, argv);
        
        for (auto &str : __volumn.getValue()) {
            size_t pos = str.find_first_of(':');
            if (pos == std::string::npos) {
                throw TCLAP::ArgException("invalid volumn");
            } else {
                std::string str1 = str.substr(0, pos);
                std::string str2 = str.substr(pos + 1);
                volumn.push_back(std::make_pair(str1, str2));
            }
        }
        cpu_limit = __cpu.getValue();
        if (cpu_limit < 0 && cpu_limit != -1)
            throw TCLAP::ArgException("unknown cpu value");
        if (!is_valid_scale_arg(__memory.getValue(), &memory_limit))
            throw TCLAP::ArgException("unknown scale value");
        if (!is_valid_scale_arg(__blkio.getValue(), &blkio_limit))
            throw TCLAP::ArgException("unknown scale value");
        image = __image.getValue();
        command = __command.getValue();
    } catch (TCLAP::ArgException &e) {
        fprintf(stderr, "ERROR: %s %s\n", e.error().c_str(), e.argId().c_str());
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    if (geteuid() != 0) {
        fprintf(stderr, "You must be root to run this script");
        exit(EXIT_FAILURE);
    }
    init_runtime_enviroment();
    arg_parse(argc, argv);
    volumn.push_back(std::make_pair(DNS_FILE, "/etc/resolv.conf"));
    setup_image();
    pid_t pid = setup_child();
    host_setup_net(pid);
    setup_cgroup(pid);
    IPC_send_sync();
    IPC_free();
    errexit(waitpid(pid, NULL, 0));
    // get back terminal control
    // because child process's sid is not same with parent process
    // it's need to ignore SIGTTOU preventing being stopped
    errexit(tcsetpgrp(STDIN_FILENO, getpgid(0)));
}