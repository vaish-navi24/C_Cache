#include<stdio.h>
#include<string.h>
#include <pthread.h>
#include<stdbool.h>
#include <stdlib.h>

void* handle_request(void* arg) {
    char* input_str = (char*)arg;

    if (strcmp(input_str, "hello") == 0) {
        printf("output : hello\n");
    } else {
        printf("output : chalo\n");
    }

    fflush(stdout);      // IMPORTANT: flush output so Node can receive it
    free(input_str);     // free strdup'ed string
    return NULL;
}

int main() {
	
	
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


