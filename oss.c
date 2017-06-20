#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "commons.h"

void parseArgs(int argc, char **argv);
void init();
void initSharedMemory();
void initFiles();
int initSem(int semval, int ftokIndex, int numsems);
void sigHandler(int signo); 
void sendSignal(int signo);
pid_t r_wait(int *stat_loc);
void doProcess();
void processRequest();
int getAvailSlot();
int getProcCount();
void canFork();
void forkProcess(int index);
void monitorRequestList();
void cleanup();
void addRequest(int index,ull wait);
void removeRequestFromQueue();
struct Node* getFront();
void monitorReqQueue();
int getTotFreeFrames();
void checkForDead();
void freeChildMemory(int index); 
void updateTimer(ull nsec);
void printResults();
int shmid,n,nop;
SystemShare *ssd;
struct Node *front=NULL,*rear=NULL;
ull toNextFork;
float throughput;
int done=0,minframes;
int main(int argc,char *argv[])
{	
	setlocale(LC_NUMERIC, "");
	alarm(ALARMTIME);	
	parseArgs(argc, argv);	
	init();
	doProcess();
	cleanup();
	//return 0;
}
void parseArgs(int argc, char **argv) 
{
	n = DEFAULTPROC;
	int x=0;
	if (argc == 2) 
	{
		if (isInt(argv[1])) 
		{
			x = atoi(argv[1]);
		}
	}	
	if(x>0)
		n = x;
}
void init() 
{
	srand(time(NULL));
	shmid = setSharedMemory();
	printf("SHMID=%d\n",shmid);	
	initLog();
	printf("Log initialized\n");
	initSharedMemory();
	printf("Shared Memory initialized\n");
	initFiles();
	printf("System Directory initialized\n");
	setSigHandler(*sigHandler);

}


void initSharedMemory() 
{
	ssd->timer.sec = ssd->timer.nsec = 0;
	ssd->pSem = initSem(0, 13, n);
	int i;
	ssd->numReqs = 0;		
	for (i=0; i<MAXPROC; i++) 
	{
		Request *req = &ssd->reqlist[i];		
		req->type = req->diridx = req->fileidx=req->flag = 0;		
		ssd->procStatFlag[i] = 0;
	}
}

int initSem(int semval, int ftokIndex, int numsems) 
{
	int semid;
	key_t key = ftok("./oss.c", ftokIndex);
	errno = 0;
	printf("Key=%d\nnumsem=%d\n",key,numsems);
	if ((semid = semget(key, numsems, 0666 | IPC_CREAT)) == -1) 
	{
		perror("Failed to access sempahore");
		exit(0);
	}
	int i;
	for (i=0; i<=numsems; i++)
		semctl(semid, i, SETVAL, semval);
	return semid;

}

void initFiles()
{
	SysDir *sysdir=&ssd->sysdir;
	int i=0;
	for(i=0;i<MAXDIRS;i++)
	{
		int j=0;
		DirEntry *curdir=&sysdir->dirs[i];
		curdir->diridx=i;
		sprintf(curdir->name,"%s%d","dir",i);
		if(!isDirExists(curdir->name))
		{	
			mkdir(curdir->name,S_IRWXU|S_IRWXG|S_IRWXO);
		}
		for(j=0;j<MAXFILES;j++)
		{
			FileEntry *curfile=&curdir->files[j];
			curfile->fileidx=j;						
			sprintf(curfile->name,"%s%s%d%s",curdir->name,"/File-",j,".txt");			
			if(createFile(curfile->name))
			{
				int bytes=0;				
				bytes=writeToFile(curfile->name,"Test Data",USER_WRITE_FLAGS,FILE_PERMS);						
			}
			
		}
	}
}

void sigHandler(int signo) 
{
	if (signo == SIGINT) 
	{
		writeLog("OSS received SIGINT - Exiting.\n");
		sendSignal(SIGUSR1);
		cleanup();		
	}
	else if (signo == SIGALRM) 
	{
		sendSignal(signo);
		writeLog("OSS - Out of time. Cleaning up and terminating.\n");
	}
	else if (signo == SIGUSR1)
		return;
	cleanup();
}
void sendSignal(int signo) 
{
	int i;
	for (i=0; i<MAXPROC; i++) 
	{
		if (ssd->procStatFlag[i] == 1) 
		{
			kill(ssd->child[i], signo);
			r_wait(NULL);
		}
	}		
}
pid_t r_wait(int *stat_loc) 
{
	int retval;
	while (((retval = wait(stat_loc)) == -1) && (errno == EINTR));
	return retval;
}
void doProcess()
{
	int availslot=getAvailSlot();
	forkProcess(availslot);
	toNextFork = mstons(rand() % PINTERVAL + 1);
	printf("To Next Fork=%d\n",toNextFork);
	int cnt=0;
	while(1)
	{
		canFork();
		monitorRequestList();
		updateTimer(150);
		monitorReqQueue();
		checkForDead();			
		//usleep(1);
	}
}
void forkProcess(int index) 
{
	int pid;
	ssd->procStatFlag[index] = 1;
	pid = fork();
	if ( pid == 0 ) 
	{
		char *args = malloc(getDigitCount(index));
		int x = sprintf(args, "%d", index);		
		execl("./UserProc", "UserProc", args, NULL); 
		perror("Failed to exec a user process.");
	}
	else if (pid == -1 ) 
	{
		perror("Failed to fork.");
		sleep(1);
	}
}
int getAvailSlot() 
{
	int i;
	for (i=0; i<MAXPROC; i++) 
	{
		if (ssd->procStatFlag[i] == 0) 
			return i;
	}
	return -1;
}
void canFork() 
{
	if (toNextFork == 0) 
	{	
		int proccnt=getProcCount();
		if ( proccnt< MAXPROC && nop<n) 
		{			
			toNextFork = mstons(rand() % PINTERVAL + 1);
			printf("To Next Fork=%d\t",toNextFork);
			nop++;
			if(nop==n)
				throughput=1000.0f/((float)stoms(ssd->timer.sec)+nstoms(ssd->timer.nsec))*n;
			printf("No Procs=%d\tTill Time=%f msec %\n",nop,(float)stoms(ssd->timer.sec)+nstoms(ssd->timer.nsec));
			forkProcess(getAvailSlot());						
		}
	}
}
int getProcCount() 
{
	int i;
	int pcnt = 0;
	for (i=0; i<MAXPROC; i++) 
	{
		if (ssd->procStatFlag[i] == 1)
			pcnt++;
	}
	return pcnt;
}
void monitorRequestList() 
{
	int i;
	for (i=0; i<MAXPROC; i++) 
	{
		if (ssd->reqlist[i].flag == 1) 
		{
			//printf("Before Processing Req=%d\n",i);
			processRequest(i);
			//printf("After Processing Req=%d\n",i);
		}
	}
}
void processRequest(int index) 
{
	Request* req = &ssd->reqlist[index];
	SysDir *dirptr=&ssd->sysdir;
	int diridx=req->diridx;
	int fileidx=req->fileidx;
	DirEntry *reqdir=&dirptr->dirs[diridx];
	FileEntry *reqfile=&reqdir->files[fileidx];
	if(reqfile->flag==0)
	{
		req->flag=2;
		if(req->type==READ)
			reqfile->locktype=READ_LOCK;
		else
			reqfile->locktype=WRITE_LOCK;
		updateTimer(mstons(15));		
		semSig(ssd->pSem, index); 
	}
	else
	{
		if(req->type==READ && reqfile->locktype==READ_LOCK)
		{
			req->flag=2;
			updateTimer(mstons(15));		
			semSig(ssd->pSem, index); 
		}
		else 
		{		
			addRequest(index,mstons(15));	
			req->flag = 3;
		}
	}
	
}
void checkForDead() 
{
	int i;
	for (i=0; i<MAXPROC; i++) 
	{
		if (ssd->procStatFlag[i] == 2) 
		{			
			r_wait(NULL); 
			ssd->procStatFlag[i] = 0; 
			freeChildMemory(i);
		}
	}
}
void freeChildMemory(int index) 
{		
	semctl(ssd->pSem, index, SETVAL, 0);
	ssd->reqlist[index].flag = 0;
}
void updateTimer(ull nsec) 
{
	//printf("Before Update Timer\n");
	ssd->timer.nsec+=nsec;
	if (ssd->timer.nsec >= 1000000000) 
	{ 
		ssd->timer.nsec -= 1000000000; 
		ssd->timer.sec++; 
	}	
	int i;
	int ct=0;
	struct Node *ptr=NULL;
	//printf("Before For front=%u\n",front);
	for (ptr=front;ptr!=NULL;ptr=ptr->next) 
	{		
		if (ptr->req>= 0 ) 
		{
			if (ptr->wait>=nsec) 
			{				
				ptr->wait-=nsec;
			}
			else if (ptr->wait!= 0) 
			{
				ptr->wait=0;
			}
		}
		ct++;
	}
	if (toNextFork >= nsec)
		toNextFork-=nsec;
	else
		toNextFork=0;
	//printf("After Update Timer\n");		
}
void cleanupFiles()
{
	SysDir *sysdir=&ssd->sysdir;
	int i=0;
	for(i=0;i<MAXDIRS;i++)
	{
		int j=0;
		DirEntry *curdir=&sysdir->dirs[i];
		for(j=0;j<MAXFILES;j++)
		{
			FileEntry *curfile=&curdir->files[j];
			remove(curfile->name);						
		}
		remove(curdir->name);
	}
}

void cleanup()
{	
	printResults();
	char cmd[100];
	char sid[15];
	int i;
	for (i=0; i<n; i++) 
	{
		semctl(ssd->pSem, i, IPC_RMID);
	}
	strcpy(cmd,"ipcrm -m ");
	strcat(cmd,Itoa(shmid,sid,10));
	printf("Commnad=%s\n",cmd);
	system(cmd);
	if(CLEAN_FILES)
		cleanupFiles();
	exit(0);						
}
void clear()
{
	//printTable();
	//printResults();
	char cmd[100];
	char sid[15];
	int i;
	for (i=0; i<n; i++) 
	{
		semctl(ssd->pSem, i, IPC_RMID);
	}
	strcpy(cmd,"ipcrm -m ");
	strcat(cmd,Itoa(shmid,sid,10));	
	system(cmd);							
}
//-------------------------------------Req-Q FUNCTIONS---------------------
struct Node * getNode(int index,ull wait)
{
	//printf("Get struct Node Start\n");
	struct Node *newnode=(struct Node*)malloc(sizeof(struct Node));
	newnode->req=index;
	newnode->wait=wait;
	//printf("Get struct Node End=%u\n",newnode);
	return newnode;
}
void addRequest(int index,ull wait)
{
	struct Node *newnode=getNode(index,wait);
	if(rear==NULL)
	{
		front=newnode;
		rear=newnode;
		//printf("First struct Node=%u\n",front);
		//newnode->next=NULL;		
		return;
	}
	rear->next=newnode;
	newnode->next=NULL;
	rear=newnode;		
}
void removeRequestFromQueue()
{
	if(front==NULL)
	{
		debugLog("Request Queue is Empty\n");
		return;
	}
	struct Node *ptr=front;
	front=front->next;
	if(front==NULL)
		rear=NULL;
	free(ptr);
}
struct Node* getFront()
{
	if(rear==NULL||front==NULL)
	{
		//debugLog("Request Queue is Empty\n");
		return NULL;
	}
	return front;
}
void monitorReqQueue()
{
	struct Node *curhead=getFront();
	if(curhead!=NULL)
	{
		int index=curhead->req;
		if(curhead->wait<=0)
		{		
			processRequest(index) ;
			ssd->reqlist[index].flag = 2; //reset the flag
			semSig(ssd->pSem, index); //signal on the semaphore
			removeRequestFromQueue();
		}		
	}
}

void printResults() 
{	
	
	writeLog("=======================================\n");
	writeLog("FINAL REPORT\n");
	writeLog("REQUESTS    		: %d\n", ssd->numReqs);
	writeLog("TOTAL READ REQS 	: %d\n", ssd->nor);
	writeLog("TOTAL WRITE REQS	: %.2f%%\n",ssd->now);
	writeLog("THROUGHPUT  		: %.2f\n", throughput);	
	writeLog("=======================================\n\n");
}



