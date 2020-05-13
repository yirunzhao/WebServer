#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "getline.h"
#include "sendutils.h"
#include "utils.h"
#define MAXSIZE 1024

void epoll_run(int port);
void do_read(int cfd, int epfd);
void disconnect(int cfd, int epfd);
void do_accept(int lfd, int epfd);
int init_listen_fd(int port, int epfd);
void http_request(int cfd, const char *file);