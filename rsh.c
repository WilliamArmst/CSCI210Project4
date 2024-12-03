#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#define N 13

extern char **environ;
char uName[20];

char *allowed[N] = {"cp","touch","mkdir","ls","pwd","cat","grep","chmod","diff","cd","exit","help","sendmsg"};

struct message {
	char source[50];
	char target[50]; 
	char msg[200];
};

void terminate(int sig) {
	printf("Exiting....\n");
	fflush(stdout);
	exit(0);
}

void sendmsg (char *user, char *target, char *msg) {
    char sentMsg[300];
    strcpy(sentMsg, user);
    strcat(sentMsg, ";");
    strcat(sentMsg, target);
    strcat(sentMsg, ";");
    strcat(sentMsg, msg);
    strcat(sentMsg, "\0");

    // printf("sentMsg: %s\n", sentMsg);

    sleep(1); // wait for server to process last message
    int fd = open("serverFIFO", O_WRONLY);
	write(fd, &sentMsg, strlen(sentMsg));
	close(fd);
}

void* messageListener(void *arg) {
	int fd = open(uName, O_RDONLY);

	while (1) {
	    char buf[512];
        int n = read(fd, buf, 511);
        buf[n] = '\0';

		if (n) {
            char source[50];
            char msg[450];

            strcpy(source, strtok(buf, ";"));
            strcpy(msg, strtok(NULL, ";"));
            printf("Incoming message from %s: %s\n", source, msg);
        }
	}

    close(fd);
	pthread_exit((void*)0);
}

int isAllowed(const char*cmd) {
	int i;
	for (i=0;i<N;i++) {
		if (strcmp(cmd,allowed[i])==0) {
			return 1;
		}
	}
	return 0;
	}

int main(int argc, char **argv) {
	pid_t pid;
	char **cargv; 
	char *path;
	char line[256];
	int status;
	posix_spawnattr_t attr;

	if (argc!=2) {
		printf("Usage: ./rsh <username>\n");
		exit(1);
	}
	signal(SIGINT,terminate);

	strcpy(uName,argv[1]);

	// TODO:
	// create the message listener thread
    pthread_t tid;
    int *arr;
    arr = (int*)malloc(sizeof(int)*500);
    pthread_create(&tid, NULL, messageListener, (void *)arr); 
	// printf("created thread: %ld\n", tid);

	while (1) {
		fprintf(stderr,"rsh>");

		// get line
		if (fgets(line,256,stdin)==NULL) continue;
		if (strcmp(line,"\n")==0) continue;
		line[strlen(line)-1]='\0';

		char cmd[256];
		char line2[256];
		strcpy(line2,line);
		strcpy(cmd,strtok(line," "));

		if (!isAllowed(cmd)) {
			printf("NOT ALLOWED!\n");
			continue;
		}

		if (strcmp(cmd,"sendmsg") == 0) {
			// TODO: Create the target user and
			// the message string and call the sendmsg function
			char target[256];
			char message[256];

			char *token = strtok(NULL, " ");
			if (token == NULL) {
				printf("sendmsg: you have to specify target user\n");
				continue;
			}
			strcpy(target, token);

			token = strtok(NULL, "\0");
			if (token == NULL) {
				printf("sendMsg: you have to enter a message\n");
				continue;
			}
			strcpy(message, token);

			// printf("target: %s\n", target);
			// printf("message: %s\n", message);

			sendmsg(uName, target, message);
			continue;
		}

		if (strcmp(cmd,"exit")==0) break;

		if (strcmp(cmd,"cd")==0) {
			char *targetDir=strtok(NULL," ");
			if (strtok(NULL," ")!=NULL) {
				printf("-rsh: cd: too many arguments\n");
			}
			else {
				chdir(targetDir);
			}
			continue;
		}

		if (strcmp(cmd,"help")==0) {
			printf("The allowed commands are:\n");
			for (int i=0;i<N;i++) {
				printf("%d: %s\n",i+1,allowed[i]);
			}
			continue;
		}

		cargv = (char**)malloc(sizeof(char*));
		cargv[0] = (char *)malloc(strlen(cmd)+1);

		path = (char *)malloc(9+strlen(cmd)+1);
		strcpy(path,cmd);
		strcpy(cargv[0],cmd);

		char *attrToken = strtok(line2," "); /* skip cargv[0] which is completed already */
		attrToken = strtok(NULL, " ");
		int n = 1;
		while (attrToken!=NULL) {
			n++;
			cargv = (char**)realloc(cargv,sizeof(char*)*n);
			cargv[n-1] = (char *)malloc(strlen(attrToken)+1);
			strcpy(cargv[n-1],attrToken);
			attrToken = strtok(NULL, " ");
		}
		cargv = (char**)realloc(cargv,sizeof(char*)*(n+1));
		cargv[n] = NULL;

		// Initialize spawn attributes
		posix_spawnattr_init(&attr);

		// Spawn a new process
		if (posix_spawnp(&pid, path, NULL, &attr, cargv, environ) != 0) {
			perror("spawn failed");
			exit(EXIT_FAILURE);
		}

		// Wait for the spawned process to terminate
		if (waitpid(pid, &status, 0) == -1) {
			perror("waitpid failed");
			exit(EXIT_FAILURE);
		}

		// Destroy spawn attributes
		posix_spawnattr_destroy(&attr);

	}
	return 0;
}
