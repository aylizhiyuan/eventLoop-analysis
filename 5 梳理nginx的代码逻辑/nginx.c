#include <string.h>
#include "ngx_config.h"
#include "ngx_cycle.h"
#include "ngx_process.h"
#include "ngx_process_cycle.h"
#include "ngx_os.h"
#include "ngx_conf_file.h"

extern unsigned int ngx_process;
static int ngx_get_options(int argc,const char *argv[]);
//下面的变量在解析命令行选项流程中赋值
static char *ngx_prefix; //存储命令行 -p 参数
static char *ngx_signal;//存储命令行 -s 参数
int main(int argc,const char *argv[]){
    ngx_core_conf_t *ccf;
    ngx_get_options(argc,argv);
    //初始化日志
    ngx_log_init(ngx_prefix);
    if(ngx_signal){
        return ngx_signal_process(ngx_signal);
    }
    //获取配置参数，这是一个伪实现
    ccf = ngx_get_conf();
    //根据配置文件，确定使用何种进程模式，默认是单进程模式
    if(ccf->master && ngx_process == NGX_PROCESS_SINGLE){
        ngx_process = NGX_PROCESS_MASTER;
    }
    ngx_init_signals();
    //deamonized
    if(ccf->daemon){
        ngx_daemon();
    }
    ngx_create_pidfile(NGX_PID_PATH);
    if(ngx_process == NGX_PROCESS_SINGLE){
        ngx_single_process_cycle();
    }else{
        ngx_master_process_cycle();
    }
    return 0;
}
static int ngx_get_options(int argc,const char *argv[]){
    int i;
    char *p;
    //argv[0] = nginx
    //argv[1] = -s
    //argv[2] = stop
    for(i=1;i<argc;i++){
        p = argv[i]; // [-]s
        if(*p != '-'){
            ngx_log_stderr("invalid option:\"%s\"",argv[i]);
            return -1;
        }
        p++; // -[s]
        while(*p){
            switch(*p++){
                case "s": 
                    if(*p){
                        //这个意思是支持sstop
                        ngx_signal = p;
                    }else if(argv[++i]){
                        ngx_signal = argv[i];
                    }else{
                        ngx_log_stderr("option \"-s\" require parameter");
                        return -1;
                    }
                    if(strcmp(ngx_signal,"stop") == 0){
                        goto next;
                    }
                    ngx_log_stderr("invalid option:\"-s %s\"",ngx_signal);
                    return -1;
                default: 
                    ngx_log_stderr("invalid option:\"%c\"",*(p-1));
                    return -1;    
            }
        }
next : continue;       
    }
    return 0;
}
