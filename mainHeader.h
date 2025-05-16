#include <stdbool.h>

#ifndef MAIN_HEADER_H
#define MAIN_HEADER_H

#include <pthread.h>
extern pthread_mutex_t userLock;
extern pthread_mutex_t movieLock;
#endif


#define USER 105
#define MOVIE 153
#define SET 10
#define EVICT_USER 80
#define EVICT_MOVIE 100 

typedef struct UserEntry {
	char* uuid;
	char *pref;
	int access;
	bool modified;
	int idx;
}UserEntry;

typedef struct RecentUser {
	UserEntry ** arr;
	int elements;
}RecentUser;

typedef struct evictUser {
	UserEntry **arr;
	int top;
}evictUser;

typedef struct MovieEntry {
	char *id;
	char *title;
	float rating;
	int total_rating;
	int likes;
	int dislikes;
	char *genre;
	bool modified;
	bool isNew;
	int access;
	int idx;
}MovieEntry;

typedef struct RecentMovie {
	MovieEntry **arr;
	int elements;
}RecentMovie;

typedef struct evictMovie {
	MovieEntry **arr;
	int top;
}evictMovie;

void init_User(RecentUser * map);

void init_userEvict(evictUser *que);

void init_Movie(RecentMovie *map);

void init_movieEvict(evictMovie *que);

int hash_User(char *id);

int hash_Movie(char *title);

bool addUserEntry(RecentUser *map, char *id, char *preference);

bool addMovieEntry(RecentMovie *map, char *id, char *title, float rate, int total, int likes, int dislikes, char *gen, bool new);

UserEntry* getUser(RecentUser *map, char *id);

MovieEntry* getMovie(RecentMovie *map, char *name);

bool addUserToQue(evictUser *que, UserEntry *entry);

bool addMovieToQue(evictMovie *que, MovieEntry *entry);

void changeAccess(evictUser *que, UserEntry *entry);

void changeAccessMovie(evictMovie *que, MovieEntry *entry);

UserEntry* evict_User(evictUser *que);

MovieEntry* evict_Movie(evictMovie *que);

void tokenExists(char *token, RecentUser* map);