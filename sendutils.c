#include "sendutils.h"
#include "utils.h"

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