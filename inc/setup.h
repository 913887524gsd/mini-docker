#pragma once

#include <unistd.h>
#include <string>
#include <vector>

extern char *runtimedir;
extern char *mergeddir;

extern char *imagedir;

extern char *runtimeID;

extern std::vector<std::pair<std::string, std::string>> volumn;
extern double cpu_limit;
extern size_t memory_limit;
extern size_t blkio_limit;
extern std::string image;
extern std::vector<std::string> command;

void setup_fs(void);
void setup_image(void);
pid_t setup_child(void);
void setup_cgroup(pid_t pid);
void host_setup_net(pid_t pid);
void container_setup_net(void);