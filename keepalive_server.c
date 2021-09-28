/* Andre Augusto Giannotti Scota (a2gs)
 * https://sites.google.com/view/a2gs/
 * andre.scota@gmail.com
 *
 * A simple server for keepalive
 *
 * MIT License
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "keepalive.h"

#define KEEPALIVE_PROC_UPTIME	("/proc/uptime")
#define KEEPALIVE_PROC_LOADAVG	("/proc/loadavg")

int main(int argc, char *argv[])
{
	int listenfd = 0, connfd = 0, readRet = 0;
	socklen_t len;
	struct sockaddr_in servaddr, cliaddr;
	char addStr[255 + 1] = {0};
	char msg[KEEPALIVE_MSG_FROM_SERVER_LEN + 1] = {0}, *endLine = NULL;
	float uptime = 0.0, load1 = 0.0, load2 = 0.0, load3 = 0.0;
	FILE *fpproc = NULL;

	if(argc != 2){
		printf("Usage:\n\t%s [PORT_NUM]\n", argv[0]);
		return(1);
	}

	listenfd = socket(AF_INET, SOCK_STREAM, 0); /* For IPv6: AF_INET6 (but listen() must receives a sockaddr_in6 struct) */
	if(listenfd == -1){
		printf("ERRO socket(): [%s].\n", strerror(errno));
		return(1);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET; /* The best thing to do is specify: AF_UNSPEC (IPv4 and IPv6 clients support) */
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	if(bind(listenfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) != 0){
		printf("ERRO bind(): [%s].\n", strerror(errno));
		return(1);
	}

	if(listen(listenfd, 0) != 0){
		printf("ERRO listen(): [%s].\n", strerror(errno));
		return(1);
	}

	for(;;){
		len = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len);
		if(connfd == -1){
			printf("ERRO accept(): [%s].\n", strerror(errno));
			return(1);
		}

		printf("Connection from %s, port %d\n", inet_ntop(cliaddr.sin_family, &cliaddr.sin_addr, addStr, sizeof(addStr)), ntohs(cliaddr.sin_port));

		memset(msg, 0, KEEPALIVE_MSG_FROM_SERVER_LEN);

		readRet = recv(connfd, msg, KEEPALIVE_MSG_FROM_SERVER_LEN, 0);
		if(readRet == 0){
			printf("End of data\n");
			shutdown(connfd, 2);
			break;
		}

		if(readRet < 0){
			printf("ERRO recv(): [%s].\n", strerror(errno));
			shutdown(connfd, 2);
			break;
		}

		endLine = strrchr(msg, '\r');
		if(endLine != NULL) (*endLine) = '\0';

		printf("Ping received from client. Msg: [%s]\n", msg);

		/* Message */

		fpproc = fopen(KEEPALIVE_PROC_UPTIME, "r");
		if(fpproc == NULL){
			printf("Error opening (uptime) [%s]: [%s].\n", KEEPALIVE_PROC_UPTIME, strerror(errno));
			return(KEEPALIVE_ERRO);
		}
		fscanf(fpproc, "%f", &uptime);

		fclose(fpproc);

		fpproc = fopen(KEEPALIVE_PROC_LOADAVG, "r");
		if(fpproc == NULL){
			printf("Error opening (load) [%s]: [%s].\n", KEEPALIVE_PROC_LOADAVG, strerror(errno));
			return(KEEPALIVE_ERRO);
		}
		fscanf(fpproc, "%f %f %f", &load1, &load2, &load3);

		fclose(fpproc);

		snprintf(msg, KEEPALIVE_MSG_FROM_SERVER_LEN, "%f\t%f\t%f\t%f", uptime, load1, load2, load3);

		printf("Enviando: [%s]\n", msg);

		readRet = send(connfd, msg, strlen(msg), 0);
		if(readRet < -1){
			printf("ERRO send(): [%s].\n", strerror(errno));
			shutdown(connfd, 2);
			break;
		}

		shutdown(connfd, 2);
	}

	close(listenfd);

	return(0);
}
