#include "utils.h"

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