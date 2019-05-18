## 1. 实现一个最为简单的http服务器

总体思路:服务器httpd采用多线程接收客户端请求，再分析报文，主要是分GET请求和POST请求，GET请求直接请求，如果get后面有?或者post请求，直接使用CGI动态处理界面，说白了比较交单，http是在tcp的基础上添加了http报文的基础解析内容，本质还是业务逻辑的处理

步骤讲解:
1. 进入主函数，主函数是具体的实现逻辑，启动HTTP服务器
2. 创建进程属性结构体 attr ,创建16位的端口号，默认为0
3. 进入serstart函数，传入端口号，返回服务器socket实例
4. 创建sockaddr_in结构体saddr传入IP地址、端口号等信息用于bind，首先使用htons转化为网络字节序，也就是整数的低字节存放在内存的低字节处
5. saddr.sin_addr.s_addr = INADDR_ANY 设置本机的地址是0.0.0.0
6. 使用bind函数绑定socket实例和saddr地址信息
7. 当前的端口号是0,所以在以端口号为0的时候调用bind后，使用getsockname返回内核赋予的本地端口号，然后使用ntohs将端口号转化为本地存储的字节序
8. 调用listen函数，启动监听任务,最后返回socket实例
9. 调用pthread_attr_init初始化线程的属性，并设置线程的分离状态，当线程结束后，马上释放系统的资源
10. 进行一个死循环，在无限循环中，调用accpet函数阻塞等待等待客户端的连接，并使用pthread_create创建线程，并在创建好的线程中传入客户端的socket实例，这样，我们就可以在request_accept函数中操作客户端的socket实例了
11. 创建buf数组，存储请求来的数据，创建path数组，存储请求的路径，创建type数组，存储请求的方式
12. 发送的请求头类似于:

        GET /index.html HTTP/1.0\r\nUser-Agent: Happy is good.\r\nHost: 127.0.0.1:43543\r\nConnection: close\r\n\r\n

13. 调用getfdline读取请求头的一行数据，具体的做法如下:

        char *tp = buf;//buf是一个1024长度的数组,tp指向buf的第一个元素,数组的起始位置
        read(fd,&c,1);//调用read函数读取客户端发来的数据，没接收一个字符，就赋值给c
        if(c == '\r');//判断读取的字符是否等于\r，如果等于的话，就意味着其实已经到了一行的末尾了
        recv(fd,&c,1,MSG_PEEK);//从缓冲区中读下一个字符，判断下是否是\n
        read(fd,&c,1);//如果是\n的话，那么也把\n读出来
        *tp++ = c;//不断的将读取的字符赋值给buf数组存起来
        *tp = '\0';//最后加上结束符
        return tp - buf;//返回读出来的字节数

14. 读出 GET /index.html HTTP/1.0\r\n以后放入buf数组里，随后将GET或者POST放入type中去
15. 判断type是否是POST或者是GET，如果都不是，返回500
16. 读取一个字符，就是请求方式后的空格
17. 读取路径部分，放入Path ./index.html
18. 读取路径部分，提取出query部分，赋值给query ,iscgi = 0
19. 使用stat将path的权限读取到st中来，如果读取失败了，则直接返回404
20. 这时候再判断文件是否有执行、写入、读取的权限，只要有一个权限，iscgi = 0
21. 判断是否iscgi的值为真，如果为真的话，那么直接返回文件，如果为0的话，说明GET方式有参数
22. response_file非常的简单，读取文件，最后使用write写给服务器
23. request_cgi是我们动态处理参数的函数，这里会传入客户端的socket,路径信息，请求类型和GET参数
24. 




