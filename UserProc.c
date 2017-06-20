#include "commons.h"

void doProcess();
void genRequest();
void cleanup();
void sigHandler(int);
void processResponse();
void writeData(char *file);
void readData(char *file);
void printResults();
SystemShare *ssd;
int curIndex;
int numReqs,nor,now;
Timer thisAge;
double totwt;
int main(int argc, char **argv) 
{
	signal(SIGINT, SIG_IGN);
	setlocale(LC_NUMERIC, "");
	setSharedMemory();
	setSigHandler(*sigHandler);
	curIndex = atoi(argv[1]);	
	printf("Generated process=%d\tIndex=%d.\n", getpid(),curIndex);

	srand(time(NULL)+curIndex);
	thisAge.sec = ssd->timer.sec;
	thisAge.nsec = ssd->timer.nsec;

	ssd->procStatFlag[curIndex] = 1;
	ssd->child[curIndex] = getpid();
	//printf("before do process %d.\n", curIndex);
	doProcess();
	
	return 0;
}


void doProcess() 
{
	int numReqs = rand()%100 + 1000;
	int i = 0;
	for (i=0;i<numReqs;i++) 
	{
		clock_t tic = clock();
		genRequest();	
		semWait(ssd->pSem, curIndex);
		processResponse();	
		clock_t toc = clock();
		double curwt=(double)(toc - tic) / CLOCKS_PER_SEC;		
		totwt+=curwt;	
	
		if (i==numReqs-1) 
		{
			if (rand() % 6 != 1) 
			{
				i=0;
				numReqs = rand()%100 + 1000;
			}
		}
		
	}
	cleanup();
}



void genRequest() 
{
	reqType type;
	int rw = rand() % 2;
	if (rw == 1)	
		type = WRITE;
	else
		type=READ;

	int diridx = rand()%MAXDIRS;
	int fileidx=rand()% MAXFILES;
	ssd->reqlist[curIndex].type = type;
	ssd->reqlist[curIndex].diridx = diridx;
	ssd->reqlist[curIndex].fileidx = fileidx;
	ssd->reqlist[curIndex].flag = 1;	
	numReqs++; 
	ssd->numReqs++; 
}
void processResponse()
{
	Request* req = &ssd->reqlist[curIndex];
	if(req->flag==2)
	{
		SysDir *dirptr=&ssd->sysdir;
		int diridx=req->diridx;
		int fileidx=req->fileidx;
		DirEntry *reqdir=&dirptr->dirs[diridx];
		FileEntry *reqfile=&reqdir->files[fileidx];
		if(req->type==READ)
		{			
			readData(reqfile->name);						
		}
		else
		{
			writeData(reqfile->name);
		}
		reqfile->locktype=NOLOCK;
		req->flag=0;
	}
	else if(req->flag==3)
	{
		writeLog("Request=%s Failed for User Process=%d\n",(req->type==READ)?"READ":"WRITE",getpid());
	}		
}
void readData(char *file)
{
	char buf[BLOCKSIZE];
	int bytes=readFromFile(file,buf,READ_FLAGS,FILE_PERMS);
	if(bytes>0)
	{
		writeLog("User Process=%d\nOperation=Read\nFile=%s\nBytes=%d\n",getpid(),file,bytes);
		nor++; 
		ssd->nor++; 
	}
}
void writeData(char *file)
{
	char buf[BLOCKSIZE];
	sprintf(buf,"Random Data %d Placed by User Process=%d\n",rand()%999,getpid());
	int bytes=writeToFile(file,buf,USER_WRITE_FLAGS,FILE_PERMS);
	if(bytes>0)
	{
		writeLog("User Process=%d\nOperation=Write\nFile=%s\nWritten Bytes=%d\n",getpid(),file,bytes);
		now++; 
		ssd->now++; 
	}
}
float myAgeInMS() 
{
	ull nsDiff;
	ui sDiff = ssd->timer.sec - thisAge.sec;
	if (ssd->timer.nsec >= thisAge.nsec) 
	{
		nsDiff = ssd->timer.nsec - thisAge.nsec;
	}
	else {
		nsDiff = sectons(1) - (thisAge.nsec - ssd->timer.nsec);
		sDiff--;
	}
	float ms = stoms(sDiff) + nstoms(nsDiff);
	return ms;

}
void printResults() 
{	
	writeLog("=======================================\nProcess=%d\tIndex=%d Report\nTAT   		: %.2fms\nREQUESTS    		: %d\nNO OF READ REQS 	: %d\nNO OF WRITE REQS 	: %d\n=======================================\n\n",getpid(),curIndex,myAgeInMS(),numReqs,nor,now);
/*	writeLog("Process=%d\tIndex=%d Report\n", getpid(),curIndex);
	writeLog("TAT   		: %.2fms\n", myAgeInMS());	
	writeLog("REQUESTS    		: %d\n", numReqs);
	writeLog("NO OF READ REQS 	: %d\n", nor);
	writeLog("NO OF WRITE REQS 	: %d\n", now);	
	writeLog("=======================================\n\n");*/

}
void cleanup() 
{
	printResults();
	ssd->procStatFlag[curIndex] = 2;	
	exit(0);
}



void sigHandler(int signo) 
{
	if (signo == SIGINT) 
	{
		return;
	}
	else if (signo == SIGALRM) 
	{
		writeLog("User Process %d - Killed.\n", curIndex);
		cleanup();
	}
	else if (signo == SIGUSR1) 
	{
		writeLog("User Process %d - Killed.\n", curIndex);
		cleanup();
	}
}
