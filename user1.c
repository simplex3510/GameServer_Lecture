#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
  
#define FILLED 0 //더미
#define Ready1 1 //플레이어1,2,3 진행 순서 판별용
#define Ready2 2
#define Ready3 3
#define NotReady -1 //더미
  
struct memory { //구조체로 여러 변수 공유 메모리로 공유
    char buff[100]; //입력 단어 버퍼
	char priBuff[100]; //이전 단어 저장
    int status, pid1, pid2, pid3, breaker; //현재 진행 상태, 프로세스 pid, 이의제기한 유저가 누구인지 저장용 변수
	int offender; //반칙자 플레이어 지정
	int offenderSelect; //반칙자 투표 찬성표 수
	char loser[20]; //패자 이름 저장
	int gameOver; //더미
	int voted; //투표한 인원 수
	int voteStep; //투표 진행단계 관리
};
 
struct memory* shmptr; //구조체 메모리 공유용
    
void handler(int signum) //시그널 관리 함수
{  
    if (signum == SIGUSR1) { //타 플레이어 진행 상황 출력 시그널
		if(shmptr->status == Ready2)
			printf("user2: ");
		else if(shmptr->status == Ready3)
			printf("user3: ");
        puts(shmptr->buff);
    }
	else if(signum == SIGUSR2) //이의 제기 시 호출 시그널
	{
		int offenderSelect; //플레이어가 입력한 찬반 투표 값
		printf("emergency meeting has been called!\n");
		shmptr->voteStep=0; //투표 관련 변수 초기화
		shmptr->voted=0;
		shmptr->offenderSelect=0;
		sleep(1);

		if(shmptr->breaker==0) //이의 제기 플레이어일 때 실행, 반칙자 지정
		{ //0:user1, 1:user2, 2:user3
			while(1)
			{
				fflush(stdin); //사실 별 의미 없음, 유닉스와 윈도우 차이로 입력 벼퍼 초기화 안해줌
				printf("who is offender?(user1:0, user2:1, user3:2) : ");
				scanf("%d%*c", &shmptr->offender); //반칙자 지정, scanf와 fgets 혼용을 위해 %*c로 "\n"를 버퍼에서 삭제
				if((shmptr->offender<0)||(2<shmptr->offender)) //무효값이면 재입력
				{
					printf("wrong input. check number.\n");
					continue;
				}
				break;
			}
			shmptr->voteStep++; //반칙자 지정 끝나면 다음 투표 단계로
		}
		else //이의 신청자 아니면 반칙자 지정까지 대기
		{
			printf("pointing at the target. wait a second...\n");
		}

		while(shmptr->voteStep==0) //반칙자 지정까지 루프로 대기
			continue;

		printf("selected offender is "); //지정된 반칙자 공표
		if(shmptr->offender==0)
			printf("user1.\n");
		else if(shmptr->offender==1)
			printf("user2.\n");
		else
			printf("user3.\n");
		printf("vote(agree:1, disagree:0): "); //찬반 투표 시작
		scanf("%d%*c", &offenderSelect); //%*c로 개행문자 삭제
		shmptr->offenderSelect+=offenderSelect; //플레이어의 입력값 고유 메모리에 업로드
		shmptr->voted++; //투표 완료 인원 수 증가
		printf("voting is in progress. wait a second...\n");
		
		while(shmptr->voteStep==1) //전체 투표 완료까지 대기
		{
			if(shmptr->voted==3) //투표완료자 3명되면 다음 단계로, 이 코드는 user1만 존재
				shmptr->voteStep++;
			continue;
		}
		
		if(2<=shmptr->offenderSelect) //찬성표가 2표 이상이면 가결
		{
			printf("this vote was passed.\n");
			sleep(1);
			if(shmptr->offender==0) //패배자 출력
				printf("user1 ");
			else if(shmptr->offender==1)
			printf("user2 ");
			else
				printf("user3 ");
			printf("is a loser and offender.\n");
			shmdt((void*)shmptr); //공유 메모리 해제 
			exit(0); //종료
		}
		else //기각되면 계속 진행
		{
			printf("this vote was rejected.\n");
			sleep(1);
			printf("this game continues.\n");
			sleep(1);
		}
	}
	else if(signum == SIGINT) //게임 종료 시그널
	{
		printf("%s is loser!\ngame over.\n", shmptr->loser); //패배자 출력하고 종료
		sleep(2);
		shmdt((void*)shmptr); //공유 메모리 해제 
		exit(0);
	}
	else if(signum == SIGALRM) //입력 시간 초과 시그널
	{
		printf("\ntime out.\n");
		strcpy(shmptr->loser, "user1"); //패배자 loser에 입력, 시간초과인 플레이어의 프로세스에서 시그널 발동되어 본인의 이름 저장

		kill(shmptr->pid2, SIGINT); //다른 프로세스+본인 프로세스에 종료 시그널 호출
		kill(shmptr->pid3, SIGINT);
		kill(shmptr->pid1, SIGINT);

		
	}
}
  
int main()
{
	int isFirst=1; //user1만 존재, 첫 시작 시 이전 단어 구분 코드 생략용
    int pid = getpid(); //pid저장
    int shmid; //공유메모리 id 저장용
    int key = 12348; //키값
	 
    shmid = shmget(key, sizeof(struct memory), IPC_CREAT | 0666); //공유 메모리 키값으로 구조체 크기로 생성
    shmptr = (struct memory*)shmat(shmid, NULL, 0); //공유 메모리 주소 저장

    shmptr->pid1 = pid; //본인 pid 업로드
    shmptr->status = Ready1; //게임 상태 user1 순서로 초기화
	shmptr->gameOver=0; //더미
	strcpy(shmptr->buff, ""); //버퍼 모두 초기화
	strcpy(shmptr->priBuff, "");

    signal(SIGUSR1, handler); //사용자화된 시그널 handler로 지정
	signal(SIGUSR2, handler);
	signal(SIGINT, handler);
	signal(SIGALRM, handler);
	printf("when user1 types the words, game starts\ntime limit is 7 seconds\n");
    while (1) { //게임 메인 루프
		if(isFirst==0) //첫 시작 시 알람 시작 안함
			alarm(7);
        printf("User1: ");
		strcpy(shmptr->priBuff, shmptr->buff); //이전 입력 단어 pribuff로 복사, 단어 끝과 시작 비교용
		fgets(shmptr->buff, 100, stdin); //stdin에서 키보트 입력값 읽어오기
		alarm(0); //입력 종료 시 타이머 초기화
		printf("\n");
		if(!strcmp(shmptr->buff, "objection!\n")) //이의신청 시 breaker에 본인 식별 정수 입력 후 이의신청 시그널 모두 호출
		{
			shmptr->breaker=0;
			kill(shmptr->pid2, SIGUSR2);
			kill(shmptr->pid3, SIGUSR2);
			kill(shmptr->pid1, SIGUSR2);
			strcpy(shmptr->buff, shmptr->priBuff); //이 줄 실행되면 투표 기각되었단 뜻, 입력 버퍼에 이전 버퍼값 덮어씌우고 재실행
			continue;
		}
		
		if((shmptr->priBuff[strlen(shmptr->priBuff)-2]!=shmptr->buff[0])&&(isFirst==0)) //이전 입력 단어 끝 문자와 입력 단어 첫 문자 비교
		{
			strcpy(shmptr->loser, "user1"); //다르면 loser에 본인 이름 입력 후 다른 플레이어 화면에 입력값 출력, 종료 시그널 호출
			kill(shmptr->pid2, SIGUSR1);
			kill(shmptr->pid3, SIGUSR1);
			sleep(1);
			shmptr->status=Ready2; //없어도 될듯

			kill(shmptr->pid2, SIGINT);
			kill(shmptr->pid3, SIGINT);
			kill(shmptr->pid1, SIGINT);
		}
		isFirst=0; //user1의 첫 실행이 끝나면 0으로 변경
		  
        kill(shmptr->pid2, SIGUSR1); //다른 플레이어의 창에 입력값 출력 갱신
		kill(shmptr->pid3, SIGUSR1);
		sleep(1);
		shmptr->status = Ready2; //user2차례로 변경

		while(shmptr->status != Ready1) //user1차례까지 대기
			continue;
    }
  
    shmdt((void*)shmptr); //공유 메모리 연결 해제, 근데 여기까지 코드 진행 안옴, 더미
    return 0;
}
