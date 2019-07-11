#include <stdio.h>
#include <uv.h>
int64_t counter = 0;
void wait_for_a_while(uv_idle_t * handle){
    counter++;
    if(counter >= 10e6){
        uv_idle_stop(handle);
    }
}
int main(){
    uv_idle_t idler;
    //向loop注册了idle handle
    uv_idle_init(uv_default_loop(),&idler);
    //添加对应的回调函数
    uv_idle_start(&idler,wait_for_a_while);
    printf("idling...\n");
    uv_run(uv_default_loop(),UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    return 0;
}