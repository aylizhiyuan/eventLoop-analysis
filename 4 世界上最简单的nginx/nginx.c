int main(int argc,char *argv[]){
    //我猜是初始化信号的相关操作的
    ngx_init_signals();
    //启动主进程
    ngx_master_process_cycle();
    return 0;
}