#include "csapp.h"
void *thread(void *vargp);

// 메인 쓰레드
int main(){
    pthread_t tid;   // 피어 쓰레드의 ID를 저장할 변수
    Pthread_create(&tid, NULL, thread, NULL); // 피어 쓰레드 생성
    Pthread_join(tid, NULL);    // 피어 쓰레드의 종료까지 기다림
    exit(0);                    // 현재 프로세스의 모든 쓰레드 (메인 쓰레드 포함) 종료
}

// 피어 쓰레드
void *thread(void *vargp){
    // 간단히 문자열출력 후 종료
    printf("할루 월드~\n");
    return NULL;
}