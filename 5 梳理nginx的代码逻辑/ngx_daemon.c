#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
//守护进程的代码
//子进程继承了父进程的进程组ID，但具有一个新的进程ID
//这保证了子进程不是一个进程组的组长
//这是调用setsid的必要条件
int ngx_daemon(){
    int fd;
    switch(fork()){
        case -1: 
            return -1;
        case 0: 
            break;
        default: 
            exit(0); //父进程退出        
    }
    //setsid创建一个新的会话，使得调用进程
    //1.成为新会话的首进程
    //2.成为一个新进程组 的组长进程
    //3.没有控制终端

    setsid();
    umask(0);
    fd = open("/dev/null",O_RDWR);
    dup2(fd,STDIN_FILENO);
    dup2(fd,STDOUT_FILENO);
    dup2(fd,STDERR_FILENO);
    close(fd);
    return 0;
}