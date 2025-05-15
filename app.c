#include<stdio.h>
#include<string.h>
#include "mainHeader.h"

pthread_mutex_t userLock;
pthread_mutex_t movieLock;

int main (int argc, char **argv) {
	
	RecentUser userMap;
	RecentMovie movieMap;
	evictUser evictUser;
	evictMovie evictMovie;

	pthread_mutex_init(&userLock, NULL);
	pthread_mutex_init(&movieLock, NULL);

	init_User(&userMap);
	init_Movie(&movieMap);
	init_userEvict(&evictUser);
	init_movieEvict(&evictMovie);

	char input[256];

	while(true) {
		if (fgets(input, sizeof(input), stdin) != NULL) {
			// Allocate and copy input for the thread
			char* input_copy = strdup(input); 

			pthread_t tid;
			pthread_create(&tid, NULL, handle_request, input_copy);
			pthread_detach(tid); 
        }
	}

	return 0;
}