#linux .so, this /lib/x86_64-linux-gnu/ is find by ldd command
locate=${shell pwd}

libtest.so:so0.h so1.c so2.c
	gcc so0.h so1.c so2.c -fPIC -shared -o libtest.so

proc:libtest.so mv2lib
	gcc proc.c -L ${locate} -ltest -o proc

mv2lib:libtest.so
	cp ./libtest.so /lib/x86_64-linux-gnu/libtest.so

clean:
	rm /lib/x86_64-linux-gnu/libtest.so proc libtest.so

