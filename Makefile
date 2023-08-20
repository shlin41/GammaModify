gamma: gamma.c atomic.h cond.h futex.h mutex.h spinlock.h
	#gcc -std=gnu11 -Wall -g -fsanitize=thread -o gamma gamma.c -lpthread
	gcc -std=gnu11 -Wall -o gamma gamma.c -lpthread
