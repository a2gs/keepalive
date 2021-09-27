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
#include <pthread.h>

#include "keepalive.h"

#define TOT_THREADS	(5)

/*
char *servers[] = {"server_1", "server_2", "server_3", "server_4", "server_5",
                   "server_6", "server_7", "server_8", "server_9", "server_10",
                   "server_11", "server_12", "server_13", "server_14", "server_15",
                   "server_16", "server_17", "server_18", "server_19", "server_20",
                   "server_21", "server_22", "server_23", "server_24", "server_25"};
						 */
char *servers[] = {"localhost"};

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

int pingServer(char *server, char *msgFromServer)
{
	return(1);
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
