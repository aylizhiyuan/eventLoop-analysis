# 异步的演变

## 1. BIO 阻塞模型

IO指的就是硬盘文件(open/read/write)的读写、网卡(socket/read/write)等外部设备的读写、键盘、鼠标输入/屏幕输出.....

- CPU会把就绪状态的进程通过调度(分派CPU时间片),进程转为运行状态
- 处于运行状态的进程在时间片用完之后，不得不让出CPU,从而转化为就绪态,剥夺了它对CPU的使用权
- 当进程中有IO操作的时候,由运行态转为阻塞态,进程会以系统调用sysCall的方式请求内核提供服务,用户态切换到内核态,CPU不会对阻塞态的进程做任何操作,它会去执行其他运行态的进程....
- 当IO操作结束后,进程的阻塞状态又变为就绪状态,这时候返回第一步操作,继续去执行该进程

在整个过程中,CPU就不参与IO了,全程可能交给DMA来进行,IO结束后通知CPU继续执行该进程下的代码

代码中的任何读写文件、读写socket操作默认都是阻塞的,一旦速度较慢的话,CPU会去执行其他的进程,等IO操作结束后,再通知CPU继续执行剩余的代码...这里，CPU会在这些运行态的进程之间来回的切换....以提高CPU的利用率




        //接受一个客户端的连接,如果没有连接过来,阻塞在这里(三次握手阶段)
        cfd = accept(lfd,(struct sockaddr *)&clie_addr,&clie_addr_len);
        while(1){
            //read <----- 服务器读内核缓冲区(协议栈解析) <--------- 网卡（解帧格式）<---- 交换机/路由器<------ 客户端数据(数据传输-请求阶段)
            //如果客户端数据迟迟没有来,也会阻塞在这里....
            n = read(cfd,buf,sizeof(buf));
            //程序对数据进行处理....
            for(i=0;i<n;i++){
                buf[i] = toupper(buf[i]);
            }
            //write ----> 服务器写内核缓冲区（协议栈解析） -----> 网卡（封装帧格式） -----> 交换机/路由器 ---> 客户端(数据传输-响应阶段)
            //如果客户端迟迟不读数据,内核缓冲区数据满了之后,也会阻塞在这里....
            write(cfd,buf,n);
        }


TCP协议的主要作用就是控制两个内核之间的数据交互问题


    客户端程序 --------------- > 客户端内核          <==========>         服务器内核  <-------------- 服务器程序
                                    |                                      |  
                            序号 + 确认号 + 窗口                       序号 + 确认号 + 窗口




**多进程**

    while(1){
        //循环接收客户端的请求,当有客户端请求的时候,cfd可读
        cfd = Accept(lfd,(struct sockaddr *)&clie_addr,&clie_addr_len);
        //连接的客户端
        //多进程版的服务器
        //理论上讲的话，一个进程能支持的TCP连接数跟文件打开描述符的数量(一个连接就占用一个文件描述符)和端口数量(一个连接就占用一个端口)有关
        pid = fork();
        if(pid == 0){
            //这是新创建的子进程
            while(1){
                //读取客户端的数据
                n = Read(cfd,buf,sizeof(buf));
                //写回给客户端
                write(cfd,buf,n);
            }
        }
    }

多进程的优点很明显，每个客户端都独立占据一部分的内存空间(堆和栈都是独立的)，处理单个请求不会影响其他的请求，但是，问题也是非常的大，原因是因为多进程的服务器会占用更多的系统资源。

**多线程**

那么可以扩展到多线程的模式，多线程的程序会共享内存空间(堆是共享的，但栈是独立的)，线程和线程之间会进行争抢，会存在处理顺序上的问题，所有的线程都会修改堆的东西.....

        void *do_work(void *arg){
            while (1) {
                n = Read(ts->connfd, buf, MAXLINE);
                Write(ts->connfd, buf, n);
            }
        }
        while (1) {
            connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
            /* 达到线程最大数时，pthread_create出错处理, 增加服务器稳定性 */
            //ts代表的就是服务器的请求，会发送给线程内部
            pthread_create(&tid, NULL, do_work, (void*)&ts[i]);
        }

多线程中依然会存在各种各样的性能问题，每次请求过来，计算机就要为线程单独开辟一个空间，依然是非常消耗的，来一个开一个，过大的连接会把计算机的资源耗尽

总结:阻塞的这种模型下,如果客户端没有给你传递数据，单一的线程或者进程在遇到一个客户端连接的时候，就要阻塞在那里等待客户端的数据到来,如果客户端的数据迟迟不来的话，那么这时候如果有新的请求过来的话，我们是无法进行处理的....我们的服务器要支持多人连接，就必须为每一个客户端的请求单独开辟一个线程或者进程，这样相互独立，即便是阻塞的情况也可以同时处理多个请求,但是对于系统调度来说,需要频繁的在这些线程和进程之间切换...占用系统的资源

## 2. NIO 非阻塞模型

非阻塞的模型暂时我们目前不去考虑它.....

## 3. 多路IO模型

epoll是目前大量采用的技术，核心的优点是在于队列在内核，并且epoll有一个等待的就绪队列，直接循环就绪队列，有新的请求来的时候会调用。

epoll在于提供了一个单一的入口来管理这些请求，多路IO复用，简单来看的话，通过一个单一的入口管理所有的网络请求

    //创建我们的epoll模型，传递建议最大的监听数量
	efd = epoll_create(OPEN_MAX);
    //调用ctl函数进行监听的参数设置,将listenfd添加到监听节点中去
    //实际是一个红黑树,每个节点都存放着需要监听的节点,可以理解为来一个请求就在红黑树中添加一个节点
	res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);
    while(1){
        //刚开始的时候只监听服务器的读事件
		//eq是我们返回的发生事件的所有fd的集合，刚开始的时候是0
		//下次我们监听的就是服务器和客户端所有的读事件了
        //epoll_wait会监听红黑树中的所有节点，一旦有事件发生，将返回给epoll_wait
        //epoll_wait返回ready_list队列,进行处理
		nready = epoll_wait(efd, ep, OPEN_MAX, -1); /* 阻塞监听 */
        //循环监听的请求
        for (i = 0; i < nready; i++) {
            //服务器的读事件，处理新加入的请求
            if (ep[i].data.fd == listenfd) {
                //接收连接，并且将我们的客户端文件描述符放到client数组里面
				connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                for (j = 0; j < OPEN_MAX; j++) {
					if (client[j] < 0) {
						client[j] = connfd; /* save descriptor */
						break;
					}
				}
                //将客户端的连接加入到我们的监听集合中去
				res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);

            }else{
                //如果是客户端的读事件发生的话
				sockfd = ep[i].data.fd;
				n = Read(sockfd, buf, MAXLINE);
				//客户端关闭了连接，读到了0字节
				if (n == 0) {
					for (j = 0; j <= maxi; j++) {
						if (client[j] == sockfd) {
							client[j] = -1;
							break;
						}
					}
					//将客户端从监听节点中去掉
					res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
					if (res == -1)
						perr_exit("epoll_ctl");

					Close(sockfd);
					printf("client[%d] closed connection\n", j);
				} else {
					//否则读取客户端的数据，写回给客户端
					for (j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);
					Writen(sockfd, buf, n);
				}
            }
        }
    }

epoll维护了一个监听的数组，当数组中的读事件发生的时候，再去调用对应的处理函数处理，注意，这时候epoll_wait类似于accept都是阻塞监听的,实质上依然是同步的把，但是可以设置超时的时间，让它不阻塞

那我们总结一下epoll的好处把，epoll_create的时候创建的监听队列是在内核中的，如果发现有读事件发生的时候，直接在内核态传递给用户态，减少了一次用户态到内核态的复制

并且epoll队列维护了一个就绪的队列nready，只需要轮询所有的就绪队列中的事件即可，不需要将所有的请求都轮询一遍判断当前哪个请求发生了读事件



## 4.异步AIO




