# eventLoop-analysis


## 异步的演变

网络处理高并发请求的思路一般都是从如何解决多个请求开始的，当服务器new Socket()---> bind() ---> listen() .每个客户端调用connection进行连接，服务器则调用accept接受这个连接

        //接受一个客户端的连接,如果没有连接过来,阻塞在这里
        cfd = accept(lfd,(struct sockaddr *)&clie_addr,&clie_addr_len);
        while(1){
            //read <----- 服务器内核缓冲区 <------ 客户端数据
            //如果客户端数据迟迟没有来,也会阻塞在这里....
            n = read(cfd,buf,sizeof(buf));
            for(i=0;i<n;i++){
                buf[i] = toupper(buf[i]);
            }
            //write ----> 服务器内核缓冲区 -----> 客户端
            //如果客户端迟迟不读数据,内核缓冲区数据满了之后,也会阻塞在这里....
            write(cfd,buf,n);
        }

> 无论是网络socket的数据读取写入，还是文件的读取写入，都是要过内核的，要将数据读入内存后，再重新的写入，应用层控制读写速度或者读写策略的主要原因还是要尽可能的保证内核缓冲区的数据不能太满，也不能太空....        

网络服务器肯定是需要支持多个客户端的请求的.....

    while(1){
        //循环接收客户端的请求,当有客户端请求的时候,cfd可读
        cfd = Accept(lfd,(struct sockaddr *)&clie_addr,&clie_addr_len);
        //连接的客户端
        //多进程版的服务器
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

多进程的优点很明显，每个客户端都独立占据一部分的内存空间，处理单个请求不会影响其他的请求，但是，问题也是非常的大，原因是因为多进程的服务器会占用更多的系统资源。

那么可以扩展到多线程的模式，多线程的程序会共享内存空间，线程和线程之间会进行争抢，会存在处理顺序上的问题

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

epoll是目前大量采用的技术，核心的优点是在于队列在内核，并且epoll有一个等待的就绪队列，直接循环就绪队列，有新的请求来的时候会调用。

epoll在于提供了一个单一的入口来管理这些请求，多路IO复用，简单来看的话，通过一个单一的入口管理所有的网络请求

    //创建我们的epoll模型，传递建议最大的监听数量
	efd = epoll_create(OPEN_MAX);
    //调用ctl函数进行监听的参数设置,将listenfd添加到监听节点中去
	res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);
    while(1){
        //刚开始的时候只监听服务器的读事件
		//eq是我们返回的发生事件的所有fd的集合，刚开始的时候是0
		//下次我们监听的就是服务器和客户端所有的读事件了
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




