#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CyCLE_H_INCLUDED_
typedef struct {
    int daemon; //是否启动守护进程
    int master; //是否启动Master-worker模式
} ngx_core_conf_t;

void ngx_create_pidfile(const char *);
void ngx_delete_pidfile();
int ngx_signal_process(const char *);
#endif