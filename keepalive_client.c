/* Andre Augusto Giannotti Scota (a2gs)
 * https://sites.google.com/view/a2gs/
 * andre.scota@gmail.com
 *
 * A simple client for keepalive
 *
 * MIT License
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "keepalive.h"

#define TOT_THREADS	(5)
#define KEEPALIVE_ADDRESS_AND_PORT_STRING_LEN	(200)
#define KEEPALIVE_PROC_UPTIME	("/proc/uptime")
#define KEEPALIVE_PROC_LOADAVG	("/proc/loadavg")

/*
char *servers[] = {"server_1:9988", "server_2:9988", "server_3:9988", "server_4:9988", "server_5:9988",
                   "server_6:9988", "server_7:9988", "server_8:9988", "server_9:9988", "server_10:9988",
                   "server_11:9988", "server_12:9988", "server_13:9988", "server_14:9988", "server_15:9988",
                   "server_16:9988", "server_17:9988", "server_18:9988", "server_19:9988", "server_20:9988",
                   "server_21:9988", "server_22:9988", "server_23:9988", "server_24:9988", "server_25:9988"};
						 */
char *servers[KEEPALIVE_ADDRESS_AND_PORT_STRING_LEN + 1] = {"localhost:9988"};

static size_t tot_servers = (sizeof(servers)/sizeof(char *));
static unsigned int serverIndex = 0;

pthread_mutex_t getServerMutex = PTHREAD_MUTEX_INITIALIZER;

int startServerIndex(void)
{
	serverIndex = 0;
	return(KEEPALIVE_OK);
}

int getAndAddServerIndex(unsigned int *x)
{
	pthread_mutex_lock(&getServerMutex);

	*x = serverIndex;

	if(serverIndex < tot_servers)
		serverIndex++;

	pthread_mutex_unlock(&getServerMutex);

	return(KEEPALIVE_OK);
}

#define STRADDR_SZ	(60)
#define KEEPALIVE_PORT_STRING_LEN	(8)

int pingServer(char *server, char *msgFromServer)
{
	struct addrinfo hints, *res = NULL, *rp = NULL;
	int errGetAddrInfoCode = 0, errRet = 0;
	int sockfd = 0;
	char msg[KEEPALIVE_MSG_FROM_SERVER_LEN + 1] = {0};
	char serverAddress[KEEPALIVE_ADDRESS_AND_PORT_STRING_LEN + 1] = {0};
	char *endLine = NULL;
	char strAddr[STRADDR_SZ + 1] = {'\0'};
	void *pAddr = NULL;
	char portNum[KEEPALIVE_PORT_STRING_LEN + 1] = {0};
	float uptime = 0.0, load1 = 0.0, load2 = 0.0, load3 = 0.0;
	FILE *fpproc = NULL;

	signal(SIGPIPE, SIG_IGN);

	strncpy(serverAddress, server, KEEPALIVE_ADDRESS_AND_PORT_STRING_LEN);

	endLine = strchr(serverAddress, ':');
	if(endLine == NULL){
		printf("No port set: [%s]\n", serverAddress);
		return(KEEPALIVE_ERRO);
	}
	*endLine = '\0';

	strncpy(portNum, endLine + 1, KEEPALIVE_PORT_STRING_LEN);

	printf("Server: [%s] port [%s]\n", serverAddress, portNum);

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET; /* Forcing IPv4. The best thing to do is specify: AF_UNSPEC (IPv4 and IPv6 servers support) */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME | AI_ADDRCONFIG; /* getaddrinfo() AI_ADDRCONFIG: This flag is useful on, for example, IPv4-only  systems, to ensure that getaddrinfo() does not return IPv6 socket addresses that would always fail in connect(2) or bind(2) */

	errGetAddrInfoCode = getaddrinfo(serverAddress, portNum, &hints, &res);
	if(errGetAddrInfoCode != 0){
		printf("ERRO: getaddrinfo() [%s].\n", gai_strerror(errGetAddrInfoCode));
		return(KEEPALIVE_ERRO);
	}

	for(rp = res; rp != NULL; rp = rp->ai_next){
		/* res->ai_family must be AF_INET (IPv4), but this loop is useful to try connect to anothers address to a given DNS */
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1){
			printf("ERRO: socket() [%s].\n", strerror(errno));
			continue;
		}

		if(rp->ai_family == AF_INET)       pAddr = &((struct sockaddr_in *) rp->ai_addr)->sin_addr;
		else if(rp->ai_family == AF_INET6) pAddr = &((struct sockaddr_in6 *) rp->ai_addr)->sin6_addr;
		else                               pAddr = NULL;

		inet_ntop(rp->ai_family, pAddr, strAddr, STRADDR_SZ);
		printf("Trying connect to [%s/%s:%s].\n", rp->ai_canonname, strAddr, portNum);

		errRet = connect(sockfd, rp->ai_addr, rp->ai_addrlen);
		if(errRet == 0)
			break;

		printf("ERRO: connect() to [%s/%s:%s] [%s].\n", rp->ai_canonname, strAddr, portNum, strerror(errno));

		close(sockfd);
	}

	if(res == NULL || errRet == -1){ /* End of getaddrinfo() list or connect() returned error */
		printf("ERRO: Unable connect to any address.\n");
		return(KEEPALIVE_ERRO);
	}

	freeaddrinfo(res);

	memset(msg, 0, KEEPALIVE_MSG_FROM_SERVER_LEN);

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

	errRet = send(sockfd, msg, 2, 0);
	if(errRet == -1){
		printf("ERRO: send() [%s].\n", strerror(errno));
		return(KEEPALIVE_ERRO);
	}

	errRet = recv(sockfd, msg, KEEPALIVE_MSG_FROM_SERVER_LEN, 0);
	if(errRet == 0){
		printf("End of data\n");
		shutdown(sockfd, 2);
		return(KEEPALIVE_ERRO);
	}

	if(errRet < 0){
		printf("ERRO recv(): [%s].\n", strerror(errno));
		shutdown(sockfd, 2);
		return(KEEPALIVE_ERRO);
	}

	endLine = strrchr(msg, '\r');
	if(endLine != NULL) (*endLine) = '\0';

	shutdown(sockfd, 2);

	printf("Msg client [%s].\n", msg);

	return(KEEPALIVE_OK);
}

void * pingWorker(void *data)
{
	int retPing = 0;
	unsigned int i = 0;
	char msgFromServer[KEEPALIVE_MSG_FROM_SERVER_LEN + 1] = {'\0'};

	for(;;){
		getAndAddServerIndex(&i);

		if(i >= tot_servers)
			break;

		retPing = pingServer(servers[i], msgFromServer);

		if(retPing == KEEPALIVE_OK){
			printf("Ping [%d] from server [%s] Ok! Mensage: [%s]\n", *(unsigned int *) data, servers[i], msgFromServer);
		}else if(retPing == KEEPALIVE_ERRO){
		}else if(retPing == KEEPALIVE_TIMEOUT){
		}else{
		}

	}

	return(NULL);
}

int main(int argc, char *argv[])
{
	unsigned int i = 0;
	int ptRet = 0;
	pthread_t threads[TOT_THREADS];
	pthread_attr_t pt_attr;
	pthread_mutexattr_t mtx_attr;

	pthread_attr_init(&pt_attr);
	pthread_mutexattr_init(&mtx_attr);
	pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_DEFAULT);

	startServerIndex();

	for(i = 0; i < TOT_THREADS; i++){
		ptRet = pthread_create(&threads[i], &pt_attr, pingWorker, (void *)&i);

		if(ptRet != 0){
			printf("ERRO pthread_create() [%d - %s]\n", ptRet, strerror(ptRet));
			return(-1);
		}

	}

	for(i = 0; i < TOT_THREADS; i++){
		ptRet = pthread_join(threads[i], NULL);
	}

	pthread_mutexattr_destroy(&mtx_attr);
	pthread_mutex_destroy(&getServerMutex);

	pthread_attr_destroy(&pt_attr);


	return(0);
}
