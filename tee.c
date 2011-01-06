#include <stdio.h>
#include <string.h>


int print(){
	printf("tee\n");
	return 1;
}

void create_message(char *m, int size){
//input NO check
	memset(m, 0, size);
	snprintf(m, size,
		"HTTP/1.0 200 OK\r\n"
		"Content-Length: 20\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		"TEE\r\n\r\n");

}
