#include <stdlib.h>
#include <stdio.h>
#include "mainHeader.h"
#include <string.h>
#include <strings.h>
#include <time.h>

/**
 * Starting with initialising the main data structures for the program ; 
 * the size for it is fixed and they are initialised to be empty at the start. 
 * the data structure are : 
 * Hashmap for the storing recent users - and a priority queue for the eviction logic 
 * another Hashmap for storing all the recently viewed movies - and a priority queue for the eviction logic 
 */

void init_User(RecentUser * map) {
	map->arr = (UserEntry**)malloc(sizeof(UserEntry*) * USER);
	for(int i = 0; i < USER; i++){
		map->arr[i] = NULL;
	}
	map->elements = 0;
}

void init_userEvict(evictUser *que) {
	que->arr = (UserEntry**)malloc(sizeof(UserEntry*) * EVICT_USER);
	for(int i = 0; i < EVICT_USER; i++) {que->arr[i] = NULL;}
	que->top = -1;
}

void init_Movie(RecentMovie *map) {
	map->arr = (MovieEntry**) malloc(sizeof(MovieEntry*) * MOVIE);
	map->elements = 0;
}

void init_movieEvict(evictMovie *que) {

	que->arr = (MovieEntry**)malloc(sizeof(MovieEntry*) * EVICT_MOVIE);
	que->top = -1;
}

/**
 *  Unique hash function to generate unique index for hashmaps;
 */

int hash_User(char *id) {
	int idx = 0;

	for(int i = 0; i < strlen(id); i++) {
		idx +=  i*17 + i*i + (int)id[i] * 11;	
	}

	return idx % USER;
}

int hash_Movie(char *title) {
	int idx = 0;

	for(int i = 0; i < strlen(title); i++) {
		idx +=  i*19 + + i*i + (int)title[i] * 17;
	}
	return idx % MOVIE;
}

UserEntry* addUserEntry(RecentUser *map, char *id, char *preference, int likes, int rated) {
	
	UserEntry *entry = (UserEntry*)malloc(sizeof(UserEntry));
	entry->uuid = (char*)malloc(sizeof(char)*37);
	entry->pref = (char*)malloc(sizeof(char) * 20);
	entry->liked_movies = likes;
	entry->rated_movies = rated;
	bool done = false;

	if(!entry) {return NULL;}

	strcpy(entry->uuid, id);
	strcpy(entry->pref, preference);
	entry->access = 0;
	entry->modified = false;

	pthread_mutex_lock(&userLock);

	int idx = hash_User(id);
	for(int i = 0; i < USER; i++) {
		int t = (idx + i + i*i) % USER;
		if(map->arr[t] == NULL) {
			map->arr[t] = entry;
			done = true;
			break;
		}
	}

	pthread_mutex_unlock(&userLock);
	
	if(!done) {
		return NULL;
	}

	return entry;
}

bool addMovieEntry(RecentMovie *map, char *id, char *title, float rate, int total, int likes, int dislikes, char *gen, bool new) {
	
	MovieEntry *entry = (MovieEntry*)malloc(sizeof(MovieEntry));
	if(!entry) {return false;}

	entry->id = (char*)malloc(sizeof(char) * 21);
	entry->title = (char*)malloc(sizeof(char) * 30);
	entry->genre = (char*)malloc(sizeof(char) * 20);
	bool done = false;

	strcpy(entry->id, id);
	strcpy(entry->title, title);
	entry->rating = rate;
	entry->total_rating = total;
	entry->likes = likes;
	entry->dislikes = dislikes;
	strcpy(entry->genre, gen);
	entry->modified = false;
	entry->isNew = new;
	entry->access = 0;

	pthread_mutex_lock(&movieLock);

	int idx = hash_Movie(title);
	for(int i = 0; i < MOVIE; i++) {
		int t = (idx + i + i*i) % MOVIE;
		if(map->arr[t] == NULL) {
			entry->idx = t;
			map->arr[t] = entry;
			done = true;
			break;
		}
	}
	pthread_mutex_unlock(&movieLock);
	return done;
}

UserEntry* getUser(RecentUser *map, char *id) {
	int idx = hash_User(id);
	UserEntry *entry = NULL;

	pthread_mutex_lock(&userLock);

	for(int i = 0; i < USER; i++) {
		int t = (idx + i + i*i) % USER;
		if(map->arr[t] != NULL && strcmp(map->arr[t]->uuid, id) == 0) {
			entry = map->arr[t];
			entry->access += 1;
			break;
		}
	}

	pthread_mutex_unlock(&userLock);

	return entry;
}

MovieEntry* getMovie(RecentMovie *map, char *name) {
	int idx = hash_Movie(name);
	MovieEntry *entry = NULL;

	pthread_mutex_lock(&movieLock);

	for(int i = 0; i < MOVIE; i++) {
		int t = (idx + i + i*i) % MOVIE;
		if(map->arr[t] != NULL && strcmp(map->arr[t]->title, name) == 0) {
			entry = map->arr[t];
			entry->access += 1;
			break;
		}
	}

	pthread_mutex_unlock(&movieLock);

	return entry;
}

bool addUserToQue(evictUser *que, UserEntry *entry) {	
	if(que->top == EVICT_USER - 1) {return false;}

	pthread_mutex_lock(&userLock);
	que->top += 1;
	que->arr[que->top] = entry;

	int i = que->top;

	while(i > 0) {
		int p = (i - 1)/2;
		if(que->arr[p]->access < que->arr[i]->access) {break;}
		
		UserEntry *temp = que->arr[p];
		que->arr[p] = que->arr[i];
		que->arr[i] = temp;

		que->arr[p]->idx = p;
		que->arr[i]->idx = i;
		i = p;
	}
	pthread_mutex_unlock(&userLock);

	return true;
}

bool addMovieToQue(evictMovie *que, MovieEntry *entry) {

	if(que->top == EVICT_MOVIE - 1) {return false;}

	pthread_mutex_lock(&movieLock);

	que->top += 1;
	que->arr[que->top] = entry;

	int i = que->top;

	while(i > 0) {
		int p = (i - 1)/2;
		if(que->arr[p]->access < que->arr[i]->access) {break;}
		
		MovieEntry *temp = que->arr[p];
		que->arr[p] = que->arr[i];
		que->arr[i] = temp;

		i = p;
	}
	pthread_mutex_unlock(&movieLock);

	return true;
}

void changeAccess(evictUser *que, UserEntry *entry) {
	
	pthread_mutex_lock(&userLock);

	int i = entry->idx;
	entry->access += 1;
	UserEntry **array = que->arr;

	while(2 * i + 1 <= que->top) {
		int lc = 2*i + 1;
		int rc = 2*i + 2;
		
		if(array[lc]->access >= array[i]->access && array[rc]->access >= array[i]->access) {
			break;
		}

		else if(array[lc]->access <= array[i]->access) {
			UserEntry *temp = array[lc];
			array[lc] = array[i];
			array[i] = temp;
			
			array[lc]->idx = lc;
			array[i]->idx = i;

			i = lc;
		}

		else if(array[rc]->access <= array[i]->access) {
			UserEntry *temp = array[rc];
			array[rc] = array[i];
			array[i] = temp;

			array[rc]->idx = rc;
			array[i]->idx = i;

			i = rc;
		}

		else {
			int mn = (array[lc]->access < array[rc]->access) ? lc : rc;
			UserEntry *temp = array[mn];
			array[mn] = array[i];
			array[i] = temp;

			array[mn]->idx = mn;
			array[i]->idx = i;

			i = mn;
		}
	}

	pthread_mutex_unlock(&userLock);
}

void changeAccessMovie(evictMovie *que, MovieEntry *entry) {
	
	pthread_mutex_lock(&movieLock);
	
	int i = entry->idx;
	entry->access += 1;
	MovieEntry **array = que->arr;

	while(2 * i + 1 <= que->top) {
		int lc = 2*i + 1;
		int rc = 2*i + 2;
		
		if(array[lc]->access >= array[i]->access && array[rc]->access >= array[i]->access) {
			break;
		}

		else if(array[lc]->access <= array[i]->access) {
			MovieEntry *temp = array[lc];
			array[lc] = array[i];
			array[i] = temp;
			
			array[lc]->idx = lc;
			array[i]->idx = i;

			i = lc;
		}

		else if(array[rc]->access <= array[i]->access) {
			MovieEntry *temp = array[rc];
			array[rc] = array[i];
			array[i] = temp;

			array[rc]->idx = rc;
			array[i]->idx = i;

			i = rc;
		}

		else {
			int mn = (array[lc]->access < array[rc]->access) ? lc : rc;
			MovieEntry *temp = array[mn];
			array[mn] = array[i];
			array[i] = temp;

			array[mn]->idx = mn;
			array[i]->idx = i;

			i = mn;
		}
	}

	pthread_mutex_unlock(&movieLock);
}

UserEntry* evict_User(evictUser *que) {

	pthread_mutex_lock(&userLock);

	UserEntry * ret = que->arr[0];

	que->arr[0] = que->arr[que->top];
	que->arr[0]->idx = 0;

	int i = 0;
	UserEntry ** array = que->arr;

	while(2 * i + 1 <= que->top) {
		int lc = 2*i + 1;
		int rc = 2*i + 2;
		
		if(array[lc]->access >= array[i]->access && array[rc]->access >= array[i]->access) {
			break;
		}

		else if(array[lc]->access <= array[i]->access) {
			UserEntry *temp = array[lc];
			array[lc] = array[i];
			array[i] = temp;
			
			array[lc]->idx = lc;
			array[i]->idx = i;

			i = lc;
		}

		else if(array[rc]->access <= array[i]->access) {
			UserEntry *temp = array[rc];
			array[rc] = array[i];
			array[i] = temp;

			array[rc]->idx = rc;
			array[i]->idx = i;

			i = rc;
		}

		else {
			int mn = (array[lc]->access < array[rc]->access) ? lc : rc;
			UserEntry *temp = array[mn];
			array[mn] = array[i];
			array[i] = temp;

			array[mn]->idx = mn;
			array[i]->idx = i;

			i = mn;
		}
	}

	pthread_mutex_unlock(&userLock);

	return ret;
}

MovieEntry* evict_Movie(evictMovie *que) {

	pthread_mutex_lock(&movieLock);

	MovieEntry * ret = que->arr[0];
	que->arr[0] = que->arr[que->top];
	que->arr[0]->idx = 0;

	int i = 0;
	MovieEntry ** array = que->arr;

	while(2 * i + 1 <= que->top) {
		int lc = 2*i + 1;
		int rc = 2*i + 2;
		
		if(array[lc]->access >= array[i]->access && array[rc]->access >= array[i]->access) {
			break;
		}

		else if(array[lc]->access <= array[i]->access) {
			MovieEntry *temp = array[lc];
			array[lc] = array[i];
			array[i] = temp;
			
			array[lc]->idx = lc;
			array[i]->idx = i;

			i = lc;
		}

		else if(array[rc]->access <= array[i]->access) {
			MovieEntry *temp = array[rc];
			array[rc] = array[i];
			array[i] = temp;

			array[rc]->idx = rc;
			array[i]->idx = i;

			i = rc;
		}

		else {
			int mn = (array[lc]->access < array[rc]->access) ? lc : rc;
			MovieEntry *temp = array[mn];
			array[mn] = array[i];
			array[i] = temp;

			array[mn]->idx = mn;
			array[i]->idx = i;

			i = mn;
		}
	}

	pthread_mutex_unlock(&movieLock);

	return ret;
}

void tokenExists(char *token, RecentUser* map, evictUser* que) {

	UserEntry *entry = getUser(map, token);
		
	if(!entry) {
		printf("User found : false \n");
	}
	else {
		changeAccess(que, entry);
		printf("{\"uuid\": \"%s\", \"pref\": \"%s\", \"liked_movies\": %d, \"rated_movies\": %d}\n",entry->uuid, entry->pref, entry->liked_movies, entry->rated_movies);
	}
	fflush(stdout); 
} 

void invalidateUser(RecentUser *map, UserEntry *del) {
	int idx = hash_User(del->uuid);
	
	for(int i = 0; i < USER; i++) {
		int t = (idx + i + i*i) % USER;
		if(map->arr[t] != NULL && strcmp(map->arr[t]->uuid, del->uuid) == 0) {
			map->arr[t] = NULL;
			break;
		}
	}
}

void addUser(RecentUser *map, char *id, char *pref, int like, int rated, evictUser *que) {
	
	UserEntry* done = addUserEntry(map, id, pref, like, rated);
	if(!done) {
		UserEntry *del = evict_User(que);
		invalidateUser(map, del);
		free(del);

		printf("No more space for the entry \n");
		fflush(stdout);
	}
	
	else {

		addUserToQue(que, done);

		printf("User added successfully \n");
		fflush(stdout);
	}
}