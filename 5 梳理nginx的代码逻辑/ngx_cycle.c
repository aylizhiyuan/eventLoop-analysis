#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "ngx_config.h"
#include "ngx_log.h"


void ngx_create_pidfile(const char *name){
    int fd;
    size_t len;
    char pid[NGX_INT64_LEN + 2];
    fd = open(name,O_RDWR|O_CREAT,0644);
    len = snprintf(pid,NGX_INT64_LEN+2,"%d",getpid());
    write(fd,pid,len+1);
    close(fd);
}
int ngx_signal_process(const char *sig){
    int fd,pid;
    ssize_t n;
    char buf[NGX_INT64_LEN + 2];
    if((fd = open(NGX_PID_PATH,O_RDONLY)) == -1){
        ngx_log_stderr("open pidfile faild");
        return -1;
    }
    if((n == read(fd,buf,NGX_INT64_LEN+2)) == -1){
        close(fd);
        return -1;
    }
    close(fd);
    ngx_log_error("pid file content %s",buf);
    pid = atoi(buf);
    if(pid == -1){
        ngx_log_stderr("invalid pid");
        return -1;
    }
    if(strcmp(sig,"stop") == 0){
        kill(pid,SIGTERM);
        return 0;
    }
    return -1;
}