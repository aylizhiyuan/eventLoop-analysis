
default:	build

clean:
	rm -rf Makefile objs

build:
	$(MAKE) -f objs/Makefile

install:
	$(MAKE) -f objs/Makefile install

modules:
	$(MAKE) -f objs/Makefile modules

upgrade:
	/Users/lizhiyuan/Desktop/nginx-test/sbin/nginx -t

	kill -USR2 `cat /Users/lizhiyuan/Desktop/nginx-test/logs/nginx.pid`
	sleep 1
	test -f /Users/lizhiyuan/Desktop/nginx-test/logs/nginx.pid.oldbin

	kill -QUIT `cat /Users/lizhiyuan/Desktop/nginx-test/logs/nginx.pid.oldbin`
