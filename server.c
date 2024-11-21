#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

struct message {
	char source[50];
	char target[50]; 
	char msg[200]; // message body
};

void terminate(int sig) {
	printf("Exiting....\n");
	fflush(stdout);
	exit(0);
}

int main() {
	int server;
	// int target;
	int dummyfd;
	struct message req;
	signal(SIGPIPE,SIG_IGN);
	signal(SIGINT,terminate);
	server = open("serverFIFO",O_RDONLY);
	dummyfd = open("serverFIFO",O_WRONLY);

	while (1) {
		// TODO:
		// read requests from serverFIFO
		char buf[512];
		int n = read(server, buf, 511);
		buf[n] = '\0';
		strcpy(req.source, strtok(buf, ":"));
		strcpy(req.target, strtok(NULL, ":"));
		strcpy(req.msg, strtok(NULL, ":"));
		printf("Received a request from %s to send the message %s to %s.\n",req.source,req.msg,req.target);

		char toSend[512] = {};
		strcat(toSend, req.source);
		strcat(toSend, ":");
		strcat(toSend, req.msg);
		
		int targetFIFO = open(req.target, O_WRONLY);
		write(targetFIFO, toSend, n);
		close(targetFIFO);
	}

	close(server);
	close(dummyfd);
	return 0;
}
