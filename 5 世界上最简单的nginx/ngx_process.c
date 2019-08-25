#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include "ngx_process.h"

ngx_process_t ngx_processes[NGX_MAX_PROCESSES];
int worker_ipcfd;
extern ngx_reconfigure;

void ngx_signal_handler(int signo);
//这个猜是nginx的进程数组
void ngx_init_processes_array(){
    int i;
    for(i=0;i<NGX_MAX_PROCESSES;i++){
        ngx_processes[i].pid = -1;
        ngx_processes[i].ipcfd = -1;
    }
}
int ngx_init_signals(){
    int signo = SIGHUP;
    struct sigaction sa;
    sa.sa_handler = ngx_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    //注册一个信号处理函数,当有人发出SIGHUP信号的时候，执行ngx_signal_handler
    if(sigaction(signo,&sa,NULL) == -1){
        return (int) SIG_ERR;
    }
    return 0;
}
void ngx_signal_handler(int signo){
    switch(signo){
        case SIGHUP: 
            //感觉这个信号就是将ngx_reconfigure这个值设置为1
            ngx_reconfigure = 1;
            break;
    }
}

pid_t ngx_spawn_process(ngx_spawn_proc_pt proc){
    pid_t pid;
    int fd[2],i;
    //找到空闲的进程组中的下标就直接跳出了
    for(i=0;i<NGX_MAX_PROCESSES;i++){
        if(ngx_processes[i].pid == -1){
            break;
        }
    }
    if(socketpair(AF_UNIX,SOCK_STREAM,0,fd) == -1){
        return -1;
    }
    //创建一个子进程
    pid = fork();
    switch(pid){
        case -1: 
            close(fd[0]);
            close(fd[1]);
            return -1;
        case 0: 
            close(fd[0]);
            worker_ipcfd = fd[1];
            proc();//当前进程启动成功后，执行proc函数
            break;
        default: 
            break;        
    }
    //将这个进程存放到进程数组中
    ngx_processes[i].pid = pid;
    ngx_processes[i].ipcfd = fd[0];
    return pid;
}
