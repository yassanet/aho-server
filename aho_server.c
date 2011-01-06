#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dlfcn.h>
#include <string.h>
#include <signal.h>

#define SOCK_MAX	20
#define EMPTY	(-1)

struct htmlmethod {
	void (*create_mes1)(char*, int);
	void (*create_mes2)(char*, int);
};

//syg func
void trap(int signo) {
	char *m = "sig now";
//signal safe function "write"
	write(1, m, strlen(m));

}


int main(int argc, char** args){
	int sock[SOCK_MAX + 1];
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in client;
	int len;
	int yes = 1;

	char buf1[2048];
	char buf2[2048];
	char inbuf[2048];

//signal
//signal handle sigint. this is old format!
	signal(SIGINT, trap);
//	signal(SIGINT, SIG_IGN);

/*
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = trap;
	sa.sa_flags |= SA_RESTART;
	
	if( sigaction( SIGINT, &sa, NULL ) != 0 ) {
		fprintf( stderr, "sigaction(2) error!\n" );
	} 
*/

//polling
	fd_set readfds;
	fd_set monitor_fd;
	int maxfd = 0;
	int maxsocksize = 0;
	int TIMEOUT = 1;
        int UTIMEOUT = 0;
        struct timeval timeout;

	int y = 0;
	for(y = 0; y < SOCK_MAX; y++) {
		sock[y] = EMPTY;
	}

//dynamic lib & function pointer
	struct htmlmethod hm;
	void *dlhandle;
	void (*func)();
	//void (*create_mes)(char*, int);

	dlhandle = dlopen("libtee.so", RTLD_LAZY);
	if(!dlhandle) {
		printf("open error\n");
		return 1;
	}

	func = dlsym(dlhandle, "print");
	func();
	hm.create_mes1 = dlsym(dlhandle, "create_message");

	close(dlhandle);

	dlhandle = dlopen("libcoffee.so", RTLD_LAZY);
	if(!dlhandle) {
		printf("open error\n");
		return 1;
	}

	func = dlsym(dlhandle, "print");
	func();
	hm.create_mes2 = dlsym(dlhandle, "create_message");

	close(dlhandle);
	
//socket programing (server & mluti client)
	sock[0] = socket(AF_INET, SOCK_STREAM, 0);
	if (sock[0] < 0) {
		perror("socket");
		return 1;
	}

	addr.sin_family = AF_INET;
	//static bind
	addr.sin_port = htons(12345);
	//random bind
	//addr.sin_port = htons(0);
	addr.sin_addr.s_addr = INADDR_ANY;

//reuse sock
//	setsockopt(sock[0],
//		SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

	if (bind(sock[0], (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		perror("bind");
		return 1;
	}
	if (getsockname(sock[0], (struct sockaddr *)&addr, &addrlen) < 0) {
		perror("getsockname");
		return 1;
	}
	printf("bind port = %d\n", ntohs(addr.sin_port));

	if (listen(sock[0], 5) != 0) {
		perror("listen");
		return 1;
	}

	//create message
/*
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf),
			"HTTP/1.0 200 OK\r\n"
			"Content-Length: 20\r\n"
			"Content-Type: text/html\r\n"
			"\r\n"
			"HELLO\r\n");
*/
	hm.create_mes1(buf1, 2048);
	printf("mes = %s\n", buf1);
	hm.create_mes2(buf2, 2048);
	printf("mes = %s\n", buf2);

	maxfd = sock[0];
	maxsocksize = 1;	//max size = sock[maxsocksize -1]
	int i = 0;
	int acceptfd = 1;
	int meslen = 0;

//polling
	while (1) {
		//init
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = UTIMEOUT;
		FD_ZERO(&readfds);
		printf("maxsocksize = %d, maxfd = %d\n", maxsocksize, maxfd);
		for(i = 0; i < maxsocksize; i++) {
			if(sock[i] != EMPTY) {
				FD_SET(sock[i], &readfds);
				printf("set sock[%d] = %d\n", i, sock[i]);
			}
		}

		if(select(maxfd+1, &readfds, NULL, NULL, &timeout) < 0) {
		//if(pselect(maxfd+1, &readfds, NULL, NULL, &timeout, SIG_IGN) < 0) {
			perror("select");
			return 1;
		}

		for(acceptfd = 1; acceptfd < maxsocksize; acceptfd++) {
			if(sock[acceptfd] != EMPTY) {
				if(FD_ISSET(sock[acceptfd], &readfds)) {
					memset(inbuf, 0, sizeof(inbuf));
					meslen = recv(sock[acceptfd], inbuf, sizeof(inbuf), 0);
					//
					if(meslen > 0) {
						printf("client[%d]\n %s",acceptfd, inbuf);
						send(sock[acceptfd], buf1, (int)strlen(buf1), 0);
					} else if(meslen == 0) {
						printf("client[%d] close\n ",acceptfd);
						close(sock[acceptfd]);
						sock[acceptfd] = EMPTY;
						if(sock[acceptfd] == maxfd) {
							//get second maxfd
						}
						if(acceptfd == maxsocksize - 1) {
							maxsocksize--;
						}
					}
				}
			}
		}

		if(FD_ISSET(sock[0], &readfds)) {
			printf("accept nau\n");
//reuse list
			int j = 0;
			for(j = 1; j < SOCK_MAX; j++) {
				if(sock[j] == EMPTY) {
					len = sizeof(client);
					sock[j] = accept(sock[0], (struct sockaddr *)&client, &len);
					if (sock < 0) {
						perror("accept");
					} else {
						printf("sock %d\n", sock[j]);
						if(j > maxsocksize - 1) {
							maxsocksize++;
						}
						if(sock[j] > maxfd) {
							maxfd = sock[j];
						}
						//printf("accept maxsocksize = %d, maxfd = %d\n", maxsocksize, maxfd);
						break;
					}
				}
			}
//++
/*
			len = sizeof(client);
			sock[maxsocksize] = accept(sock[0], (struct sockaddr *)&client, &len);
			if (sock < 0) {
				perror("accept");
			} 
			printf("accept sock %d\n", sock[maxsocksize]);
			if(sock[maxsocksize] > maxfd) {
				printf("maxfd = %d\n", sock[maxsocksize]);
				maxfd = sock[maxsocksize];
			}
			if(maxsocksize < SOCK_MAX) {
				maxsocksize++;
			} else {
				printf("over\n");
				close(sock[maxsocksize]);
			}
*/			
		}
		printf("loop END\n\n");
	}

	return 0;
}
