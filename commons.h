#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <locale.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include<dirent.h>
#include <fcntl.h>

#define MAXPROC 18
#define DEFAULTPROC 12
#define TOTMEMSIZE 8
#define MAXFILES 1024
#define MAXDIRS 8
#define BLOCKSIZE 1024
#define ALARMTIME 60
#define PINTERVAL 20

#define DEBUG 1
#define CLEAN_FILES 0 // to remove directories on exit 1-clean 0-no clean

#define USER_WRITE_FLAGS (O_WRONLY | O_APPEND | O_EXCL)
#define WRITE_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define FILE_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
#define READ_FLAGS O_RDONLY

typedef unsigned long long ull;
typedef unsigned int ui;

typedef enum {READ, WRITE} reqType;
typedef enum {NOLOCK,READ_LOCK, WRITE_LOCK} lockType;
union semun 
{
	int val;
	struct semid_ds *buf;
	ushort *array;
};
typedef struct
{
	ui sec;
	ui nsec;
}Timer;

typedef struct
{
	int fileidx;
	char name[50];
	int flag;// 0-free,1-alloted
	lockType locktype;//0=> no lock,1=>read lock,2=>write lock;		
}FileEntry;
typedef struct
{
	int diridx;
	char name[20];
	FileEntry files[MAXFILES];
}DirEntry;
typedef struct
{
	DirEntry dirs[MAXDIRS];
}SysDir;
typedef struct 
{
	reqType type; 
	int diridx;
	int fileidx; 
	int flag;//0->reset,1-> post request,2-> positive response,3-> negative resonse
}Request;

struct Node
{	
	ull wait; //how long to wait
	int req;
	struct Node *next;
};

typedef struct 
{
	Timer timer; //the simulated clock.
	int pSem; //semaphores for each process, to be signaled on when their request is fulfilled.
	int numReqs;	
	int nor,now;// number of read and write requests
	SysDir sysdir;
	Request reqlist[MAXPROC];
	int procStatFlag[MAXPROC]; //flags for processes. 1 means they exist, 2 means they need to be waited on.
	pid_t child[MAXPROC];			
}SystemShare;

int setSharedMemory();
void semWait(int, int);
void semSig(int, int);
void setSigHandler( void handler() );

void writeLog (const char * format, ...);
void debugLog (const char * format, ...);

float nstoms(ull);
ull mstons(float ms);
float nstosec(ull);
ull sectons(int s);

int isInt(char *str);
int getDigitCount(int num);
char* Itoa(int value, char* str, int radix);
int canRead(char *file);
int canWrite(char *file);
int isDirExists(char *path);
int createFile(char *path);
int openFile(char *path, int flags, mode_t mode);
int writeToFile(char *path,char *data,int flags,mode_t mode);
int readFromFile(char *path,char *data,int flags,mode_t mode);
ssize_t r_read(int fd, void *buf, size_t size);
ssize_t r_write(int fd, void *buf, size_t size);













