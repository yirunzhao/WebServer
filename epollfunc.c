#include "epollfunc.h"

void epoll_run(int port)
{
    int i = 0;
    struct epoll_event all_events[MAXSIZE];

    // create a epoll tree
    int epfd = epoll_create(MAXSIZE);
    if (epfd == -1)
    {
        perror("epoll create error");
        exit(-1);
    }

    // create lfd and add it to the tree
    int lfd = init_listen_fd(port, epfd);

    while (1)
    {
        // listen
        int ret = epoll_wait(epfd, all_events, MAXSIZE, -1);
        if (ret == -1)
        {
            perror("epoll_wait error");
            exit(-1);
        }

        for (i = 0; i < ret; i++)
        {
            // listen readonly event since this is a http server
            struct epoll_event *pev = &all_events[i];

            if (!(pev->events * EPOLLIN))
            {
                continue;
            }
            if (pev->data.fd == lfd)
            {
                // accept
                do_accept(lfd, epfd);
            }
            else
            {
                // read and write operation
                do_read(pev->data.fd, epfd);
            }
        }
    }
}

void do_read(int cfd, int epfd)
{
    // read a line of http protocol and process it

    // 1.use get_line() to get first line in http request
    // 2.split GET, file name, protocol version etc.
    // 3.judge whether the file exists, use stat()
    // 4.judge wheter the file is dir or file
    // 5.if is file, open -> read -> back to browser
    // write http response header
    // write file data

    char line[1024] = {0};
    int len = get_line(cfd, line, sizeof(line)); // read the first line

    if (len == 0)
    {
        printf("Server, detecting client closes!\n");
        disconnect(cfd, epfd);
    }
    else
    {
        char method[16], path[256], protocol[16];
        sscanf(line, "%[^ ]  %[^ ] %[^ ]", method, path, protocol);
        char *file = path + 1;
        // test
        printf("method: %s, path: %s, protocol: %s\n", method, path, protocol);

        while (1)
        {
            char buf[1024] = {0};
            len = get_line(cfd, buf, sizeof(buf));
            if (len == '\n')
            {
                break;
            }
            // printf("--%d--%s--",len,buf);
            else if (len == -1)
            {
                printf("something bad happened\n");
                break;
            }
        }
        if (strcmp(path, "/") == 0)
        {
            file = "./";
        }
        // if is GET method, we should deal with it
        if (strncasecmp(method, "GET", 3) == 0)
        {
            // extract file name that client requests

            printf("file name is %s\n", file);
            http_request(cfd, file);
        }
    }
}

void disconnect(int cfd, int epfd)
{
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
    if (ret != 0)
    {
        perror("delete error");
        exit(-1);
    }
    close(cfd);
}

void do_accept(int lfd, int epfd)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int cfd = accept(lfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (cfd == -1)
    {
        perror("accept error");
        exit(-1);
    }

    // print client's IP and port, it is optional
    char client_ip[64] = {0};
    printf("New client IP: %s, Port: %d, cfd: %d\n", inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)), ntohs(client_addr.sin_port), cfd);

    // set not hop
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    // mount cfd to epoll tree
    struct epoll_event ev;
    ev.data.fd = cfd;

    ev.events = EPOLLIN | EPOLLET;

    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
    if (ret == -1)
    {
        perror("epoll ctl add cfd error");
        exit(-1);
    }
}

int init_listen_fd(int port, int epfd)
{
    // create listen fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket error");
        exit(-1);
    }

    // create server address struct
    struct sockaddr_in server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // reuse port
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // bind lfd
    int ret = bind(lfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1)
    {
        perror("bind error");
        exit(-1);
    }

    // set listen limits
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen error");
        exit(-1);
    }

    // mount lfd to epoll tree
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if (ret == -1)
    {
        perror("epoll ctl add lfd error");
        exit(-1);
    }

    return lfd;
}
// deal with http request
void http_request(int cfd, const char *file)
{
    struct stat sbuf;
    // if file does not exist
    int ret = stat(file, &sbuf);
    if (ret != 0)
    {
        // perror("stat error");
        // should tell the browser the file doesn't exist
        // we can use 404 html page
        send_error_page(cfd, 404, "Not Found", "No such file or dir, please check again!");
        printf("file does not exist\n");
    }

    // is a normal file
    if (S_ISREG(sbuf.st_mode))
    {
        printf("It is a file");
        // return http response
        // send_response(cfd, 200, "OK", "Content-Type: text/plain; charset=utf-8", sbuf.st_size);
        send_response(cfd, 200, "OK", get_file_type(file), sbuf.st_size);
        // return request file data
        send_file(cfd, file);
    }
    // if is a dir, show the files
    else if (S_ISDIR(sbuf.st_mode))
    {
        send_response(cfd, 200, "OK", get_file_type(".html"), -1);
        send_dir(cfd, file);
    }
}