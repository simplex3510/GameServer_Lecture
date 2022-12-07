#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
  
#define FILLED 0
#define Ready1 1
#define Ready2 2
#define Ready3 3
#define NotReady -1
  
struct memory {	
     char buff[100];
	 char priBuff[100];
     int status, pid1, pid2, pid3, breaker;
     int offender;
     int offenderSelect;
     char loser[20];
     int gameOver;
     int voted;
     int voteStep;
};
  
struct memory* shmptr;  
  
void handler(int signum)
{ 
    if (signum == SIGUSR1) {
		if(shmptr->status == Ready1)
			printf("user1: ");
		else if(shmptr->status == Ready2)
		{
			printf("user2: ");
		}
        puts(shmptr->buff);
    }
	else if(signum == SIGUSR2)
    {
        int offenderSelect;
        printf("emergency meeting has been called!\n");
        shmptr->voteStep=0;
        shmptr->voted=0;
        sleep(1);

        if(shmptr->breaker==2)
        {
            while(1)
            {
                printf("who is offender?(user1:0, user2:1, user3:2) : ");
                scanf("%d%*c", &shmptr->offender);
                if((shmptr->offender<0)||(2<shmptr->offender))
                {
                    printf("wrong input. check number.\n");
                    continue;
                }
                break;
            }
            shmptr->voteStep++;
        }
        else
        {
            printf("pointing at the target. wait a second...\n");
        }

        while(shmptr->voteStep==0)
            continue;

        printf("selected offender is ");
        if(shmptr->offender==0)
            printf("user1.\n");
        else if(shmptr->offender==1)
            printf("user2.\n");
        else
            printf("user3.\n");
        printf("vote(agree:1, disagree:0): ");
        scanf("%d%*c", &offenderSelect);
        shmptr->offenderSelect+=offenderSelect;
        shmptr->voted++;
        printf("voting is in progress. wait a second...\n");

        while(shmptr->voteStep==1)
        {
            continue;
        }

		if(2<=shmptr->offenderSelect)
		{
			printf("this vote was passed.\n");
			sleep(1);
			if(shmptr->offender==0)
				printf("user1 ");
			else if(shmptr->offender==1)
			printf("user2 ");
			else
				printf("user3 ");
			printf("is a loser and offender.\n");
			shmdt((void*)shmptr);
			exit(0);
		}
		else
		{
			printf("this vote was rejected.\n");
			sleep(1);
			printf("this game continues.\n");
			sleep(1);
		}
	}
	else if(signum == SIGINT)
	{
		printf("%s is loser!\ngame over.\n", shmptr->loser);
		sleep(2);
		shmdt((void*)shmptr);
		exit(0);
	}
	else if(signum == SIGALRM)
	{
		printf("\ntime out.\n");
		strcpy(shmptr->loser, "user3");

		kill(shmptr->pid1, SIGINT);
		kill(shmptr->pid2, SIGINT);
		kill(shmptr->pid3, SIGINT);
	}

}
  
int main()
{
    int pid = getpid();  
    int shmid;
    int key = 12348;
  
    shmid = shmget(key, sizeof(struct memory), IPC_CREAT | 0666);
    
    shmptr = (struct memory*)shmat(shmid, NULL, 0);
  
    shmptr->pid3 = pid;
  
    shmptr->status = Ready1;
  
    signal(SIGUSR1, handler);
	signal(SIGUSR2, handler);
	signal(SIGINT, handler);
	signal(SIGALRM, handler);
	printf("when user1 types the word, game starts. please wait\ntime limit is 7 seconds,\n"); 
    while (1) {
		while (shmptr->status != Ready3)
		{
			continue;
        }
        
		alarm(7);
        printf("User3: ");
		strcpy(shmptr->priBuff, shmptr->buff);
        fgets(shmptr->buff, 100, stdin);
		alarm(0);
		printf("\n");
		if(!strcmp(shmptr->buff, "objection!\n"))
		{
			shmptr->breaker=2;
			kill(shmptr->pid1, SIGUSR2);
			kill(shmptr->pid2, SIGUSR2);
			kill(shmptr->pid3, SIGUSR2);
			strcpy(shmptr->buff, shmptr->priBuff);
			continue;
		}
		
		if(shmptr->priBuff[strlen(shmptr->priBuff)-2]!=shmptr->buff[0])
		{
			strcpy(shmptr->loser, "user3");
			kill(shmptr->pid1, SIGUSR1);
			kill(shmptr->pid2, SIGUSR1);
			sleep(1);
			shmptr->status=Ready1;

			kill(shmptr->pid1, SIGINT);
			kill(shmptr->pid2, SIGINT);
			kill(shmptr->pid3, SIGINT);

		}
  
        kill(shmptr->pid1, SIGUSR1);
		kill(shmptr->pid2, SIGUSR1);
		sleep(1);
		shmptr->status = Ready1;

    }
		shmdt((void*)shmptr);
        return 0;
}
