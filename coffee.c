#include <stdio.h>
#include <string.h>


int print(){
	printf("coffee\n");
	return 1;
}

void create_message(char *m, int size){
	memset(m, 0, size);
	snprintf(m, size,
		"HTTP/1.0 200 OK\r\n"
		"Content-Length: 20\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		"COFFEE\r\n\r\n");

}
