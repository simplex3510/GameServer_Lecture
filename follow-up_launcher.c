#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

struct memory { //구조체로 여러 변수 공유 메모리에서 공유
    char buff[100]; //플레이어의 입력값 저장 변수
    char priBuff[100]; //이전 플레이어 입력값 저장, 문자 이어지는지 판별용
    int status, pid1, pid2, pid3, breaker; //순서대로 각 플레이어의 상태, 플레어이들의 pid값, 투표 시작한 사람 번호
    int offender; //반칙자가 누구이지 저장
    int offenderSelect; //반칙자 투표의 찬성표 수
    char loser[20]; //진 플레이어 저장
    int gameOver; //안씀
    int voted; //투표한 사람 수
    int voteStep; //투표 단계 진행도
};
 
struct memory* shmptr;

int main()
{
	int child, status; //자식 프로세스 대기용 변수
	int shmid;
	int key=12348;
	int select=0; //사용자 입력값
	shmid=shmget(key, sizeof(struct memory), IPC_CREAT | 0666);
	shmptr=(struct memory*)shmat(shmid,NULL,0);
	
	shmptr->status=1;// 사실상 더미

	printf("choose player\nuser1:1, user2:2, user3:3\n>> ");
	scanf("%d", &select); //플레이 할 user 선택
	if((select<1)||(3<select)) //유효값 아닐 시 종료
	{
		printf("wrong input.");
		return 0;
	}
	if(select==1) //1,2,3 선택에 맞게 프로세스 fork 후 exec로 새 프로세스 실행
	{
		if(fork()==0)
		{
			execlp("./user1", "./user1", NULL);
			fprintf(stderr, "first fork fail\n"); //실패 시 오류 문구
		}
	}
	else if(select==2)
	{
		if(fork()==0)
		{
			execlp("./user2", "./user2", NULL);
			fprintf(stderr, "second fork fail\n");
		}
	}
	else
	{
		if(fork()==0)
		{
			execlp("./user3", "./user3", NULL);
			fprintf(stderr, "third fork fail\n");
		}
	}
	
	child=wait(&status); //자식 프로세스 종료 대기
	printf("user%d  process end\n", select);
	
	shmdt((void*)shmptr); //공유 메모리 해제
    if(select==1) //종료 후 user1이 종료되면 공유 메모리 제거
		shmctl(shmid, IPC_RMID, NULL);

	return 0;
}
