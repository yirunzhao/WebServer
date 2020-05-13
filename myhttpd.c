/*
    author : WHU.CS.Ryan
    course : Linux network programming
    date   : 2020-5
*/

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
#include <dirent.h>

#define MAXSIZE 1024

int get_line(int cfd, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(cfd, &c, 1, 0);
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(cfd, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                {
                    recv(cfd, &c, 1, 0);
                }
                else
                {
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        }
        else
        {
            c = '\n';
        }
    }
    buf[i] = '\0';

    if (-1 == n)
        i = n;

    return i;
}

const char *get_file_type(const char *name)
{
    char *dot;
    dot = strrchr(name, '.');
    if (dot == NULL)
        return "Content-Type: text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "Content-Type: text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "Content-Type: image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "Content-Type: image/gif";
    if (strcmp(dot, ".png") == 0)
        return "Content-Type: image/png";
    if (strcmp(dot, ".css") == 0)
        return "Content-Type: text/css";
    if (strcmp(dot, ".au") == 0)
        return "Content-Type: audio/basic";
    if (strcmp(dot, "..wav") == 0)
        return "Content-Type: audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "Content-Type: video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "Content-Type: video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "Content-Type: video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "Content-Type: model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "Content-Type: audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "Content-Type: audio/mpeg";
    if (strcmp(dot, "..ogg") == 0)
        return "Content-Type: application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "Content-Type: application/x-ns-proxy-autoconfig";

    return "Content-Type: text/plain; charset=utf-8";
}

void send_error_page(int cfd, int status, char *title, char *text)
{
    char buf[4096] = {0};
    sprintf(buf, "%s %d %s\r\n", "HTTP/1.1", status, title);
    sprintf(buf + strlen(buf), "Content-Type:%s\r\n", "text/html");
    sprintf(buf + strlen(buf), "Content-Length:%d\r\n", -1);
    sprintf(buf + strlen(buf), "Connection: close\r\n");
    send(cfd, buf, strlen(buf), 0);
    send(cfd, "\r\n", 2, 0);

    memset(buf, 0, sizeof(buf));

    sprintf(buf, "<html><head><titile>%d %s</title></head>\n", status, title);
    sprintf(buf + strlen(buf), "<body><h4 align=\"center\">%d %s</h4>\n", status, title);
    sprintf(buf + strlen(buf), "%s\n", text);
    sprintf(buf + strlen(buf), "<hr>\n</body>\n</html>\n");
    send(cfd, buf, strlen(buf), 0);

    return;
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

// client fd, error number, error description, file type, file length
void send_response(int cfd, int no, char *desc, const char *type, int len)
{
    char buf[1024] = {0};
    sprintf(buf, "HTTP/1.1 %d %s\r\n", no, desc);
    sprintf(buf + strlen(buf), "%s\r\n", type);
    sprintf(buf + strlen(buf), "Content-Length: %d\r\n", len);
    send(cfd, buf, strlen(buf), 0);
    send(cfd, "\r\n", 2, 0);
}

void send_file(int cfd, const char *file)
{
    int n = 0;
    char buf[4096];
    int ret;
    // open local file
    int fd = open(file, O_RDONLY);
    if (fd == -1)
    {
        // open fail
        perror("open file error");
        exit(-1);
    }

    // send to browser
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        ret = send(cfd, buf, n, 0);
        if (ret == -1)
        {
            printf("errno = %d\n", errno);
            if (errno == EAGAIN)
            {
                printf("--EAGAIN--\n");
                continue;
            }
            else if (errno == EINTR)
            {
                printf("--EINTR--\n");
                continue;
            }
            else
            {
                perror("send error");
                exit(-1);
            }
        }
    }

    close(fd);
}
void encode_str(char *to, int tosize, const char *from)
{
    int tolen;

    for(tolen = 0; *from != '\0' && tolen +4 < tosize; from ++){
        if(isalnum(*from) || strchr("/_.-~",*from) != (char*)0){
            *to = *from;
            ++to;
            ++tolen;
        }else{
            sprintf(to,"%%%02x",(int)*from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}
void send_dir(int cfd, const char *dirname)
{
    int i, ret;

    char buf[4096] = {0};

    sprintf(buf, "<html><head><title>dir name: %s</title></head>", dirname);
    sprintf(buf + strlen(buf), "<body><h1>current dir: %s</h1><table>", dirname);

    char enstr[1024] = {0};
    char path[1024] = {0};

    struct dirent **ptr;
    int num = scandir(dirname, &ptr, NULL, alphasort);

    for (i = 0; i < num; i++)
    {
        char *name = ptr[i]->d_name;

        sprintf(path, "%s/%s", dirname, name);
        printf("path = %s ||||\n", path);
        struct stat st;
        stat(path, &st);

        encode_str(enstr,sizeof(enstr),name);

        if (S_ISREG(st.st_mode))
        {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", enstr, name, (long)st.st_size);
        }
        else if (S_ISDIR(st.st_mode))
        {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>", enstr, name, (long)st.st_size);
        }

        ret = send(cfd, buf, strlen(buf), 0);
        if (ret == -1)
        {
            if (errno == EAGAIN)
            {
                perror("send error in send dir:");
                continue;
            }
            else if (errno == EINTR)
            {
                perror("send error in send dir:");
                continue;
            }
            else
            {
                perror("send error");
                exit(-1);
            }
        }
        memset(buf, 0, sizeof(buf));
    }

    sprintf(buf + strlen(buf), "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);
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