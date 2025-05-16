#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include "mainHeader.h"

pthread_mutex_t userLock;
pthread_mutex_t movieLock;

RecentUser userMap;
RecentMovie movieMap;
evictUser userQueue;
evictMovie movieQueue;

void* handle_request(void* arg) {
    char* input_str = (char*)arg;

	char* func = strtok(input_str, " ");  

    if (strcmp(func, "tokenExists") == 0) {
		
		char* token = strtok(NULL, " ");
        tokenExists(token, &userMap);

    } else if (strcmp(func, "setUser") == 0) {
		
		char* token = strtok(NULL, " ");
		char* pref = strtok(NULL, " ");
        addUserEntry(&userMap, token, pref);

    } else {
        printf("output : chalo\n");
    }
    fflush(stdout);      // IMPORTANT: flush output so Node can receive it
    free(input_str);     // free strdup'ed string
    return NULL;
}

int main (int argc, char **argv) {

	pthread_mutex_init(&userLock, NULL);
	pthread_mutex_init(&movieLock, NULL);

	init_User(&userMap);
	init_Movie(&movieMap);
	init_userEvict(&userQueue);
	init_movieEvict(&movieQueue);

	char input[256];

	while(true) {
		
		if (fgets(input, sizeof(input), stdin) != NULL) {
			
			input[strcspn(input, "\n")] = '\0';
			char* input_copy = strdup(input); 

			pthread_t tid;
			pthread_create(&tid, NULL, handle_request, input_copy);
			pthread_detach(tid); 
        }
	}

	return 0;
}	