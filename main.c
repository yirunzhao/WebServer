/*
    author : WHU.CS.Ryan
    course : Linux network programming
    date   : 2020-5
*/
#include "getline.h"
#include "epollfunc.h"
#include "utils.h"
#include "sendutils.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("please use format : ./httpserver port path\n");
    }
    // port
    int port = atoi(argv[1]);

    // change dir
    int ret = chdir(argv[2]);
    if (ret != 0)
    {
        perror("change dir error");
        exit(1);
    }
    // run epoll listening
    epoll_run(port);
    return 0;
}