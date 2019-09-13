#include <signal.h>
#include "ngx_process.h"
#define NULL 0
int ngx_reconfigure;
extern ngx_process_t ngx_processes[NGX_MAX_PROCESSES];

static void ngx_worker_process_cycle();
static void ngx_worker_process_init();
static void ngx_signal_worker_processes(int signo);

//毫无疑问，这是启动主进程的代码
void
ngx_master_process_cycle()
{
    sigset_t set; 
    /*
     * signal
     */
    sigemptyset(&set);
    sigaddset(&set, SIGHUP); 

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {

    }
    sigemptyset(&set);


    ngx_init_processes_array();
    //主函数中应该是默认启动了两个子进程吧，猜的
    ngx_start_worker_processes(2);

    /*
     * watch worker process
     */
    for ( ; ; ) {

	sigsuspend(&set);
	/*TODO ngx_signal_handler() had returned */
	ngx_signal_worker_processes(SIGHUP);
    }
}
//这个应该是启动两个子进程的具体代码
int ngx_start_worker_processes(int n){
    int i;
    for (i = 0; i < n; i++) {
        //这里就是启动两个进程的代码，ngx_workd_process_cycle是处理函数，里面会调用它
	    ngx_spawn_process(ngx_worker_process_cycle);
    }
}

static void
ngx_worker_process_cycle()
{
    ngx_worker_process_init();

    for ( ; ; ) {
    //读取子进程中的数据并打印    
	ngx_process_events_and_timers();
    }
}

static void
ngx_worker_process_init()
{
    sigset_t set;

    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {

    }

    /* module init*/
    //ngx_modules[i]->init_process();
}
//这个感觉应该是监视子进程的，可以向子进程写一些数据吧
static void
ngx_signal_worker_processes(int signo)
{
    int i, command;
    char c[] = "abc";
    /**
     * NGX_SHUTDOWN_SIGNAL QUIT
     * NGX_TERMINATE_SIGNAL TERM, 
     * NGX_REOPEN_SIGNAL    USR1
     * above signo, IPC socket
     */
    switch (signo) {
	case SIGQUIT:
	case SIGTERM:
	case SIGUSR1:
	    command = 1;
	    break;

	default:
	    command = 0;
    }

    for (i = 0; i < NGX_MAX_PROCESSES; i++) {
	if (ngx_processes[i].pid == -1)
	    continue;

	/* ipc socket*/
	if (command) {
	    write(ngx_processes[i].ipcfd, c, sizeof(c));
	    continue;
	}

	kill(ngx_processes[i].pid, signo);
    }
}