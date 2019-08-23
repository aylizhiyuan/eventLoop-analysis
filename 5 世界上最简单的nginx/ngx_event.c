#include <stdio.h>
#include <unistd.h>
extern int worker_ipcfd;

void ngx_process_events_and_timers(){
    char b[1024];
    //我猜是主动唤醒子进程的代码吧！
    if(read(worker_ipcfd,b,1024) > 0){
        printf("woker %d said: %s\n",getpid(),b);
    }
}