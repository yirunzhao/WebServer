#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include "getline.h"
#include <stdlib.h>

void send_error_page(int cfd, int status, char *title, char *text);
void send_response(int cfd, int no, char *desc, const char *type, int len);
void send_file(int cfd, const char *file);
void send_dir(int cfd, const char *dirname);