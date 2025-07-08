#include "csapp.h"

void *thread(void *vargp);

// global shared variable
//  다중 쓰레드 환경에서 count 변수가 쓰레드 외부에서 예기치 않게 바뀔 수 있음을 컴파일러에게 알려줌
// 레지스터 캐싱, 읽기쓰기최적화 불가능
volatile long count = 0;

int main(int argc, char **argv){
    long niters;
    pthread_t tid1, tid2;

    if (argc != 2){
        printf("usage %s <niters>", argv[0]);
        exit(0);
    }
    niters = atoi(argv[1]);

    // 쓰레드를 만들고 끝날 때까지 기다리기
    Pthread_create(&tid1, NULL, thread, &niters);
    Pthread_create(&tid2, NULL, thread, &niters);
    Pthread_join(tid1, NULL);
    Pthread_join(tid2, NULL);

    // 결과 확인하기
    if (count != (2 * niters))
        printf("BOOM! cnt=%ld\n", count);
    else
        printf("OK! cnt=%ld\n", count);
    exit(0);
}

void *thread(void *vargp){
    long i, niters = *((long *)vargp);

    for (i = 0; i < niters; i++)
        count++;

    return NULL;
}