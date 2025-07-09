// 정수 0, ..., N - 1의 배열을 병렬로 더하기

#include "csapp.h"
#define MAXTHREADS 32

void *sum_mutex(void *varpg);   // 쓰레드 루틴

// 전역 변수
long gsum = 0;                  // 공유변수, 배열의 값들의 합
long psum[MAXTHREADS] = {0};
long nelems_per_thread          // 각 쓰레드마다 개산할 원소의 수
sem_t mutex;                    // gsum을 보호하기 위한 뮤텍스

int main(int argc, char **argv){
    long nthreads, myid[MAXTHREADS], nelems, log_nelems;

    pthread_t tid[MAXTHREADS];  // 32개의 쓰레드 id

    if (argc != 3) {
        printf("USAGE: %s <nthreads> <log_nelems>\n", argv[0]);
        exit(0);
    }

    nthreads = atoi(argv[1]);
    long_nelems = atoi(argv[2]);
    nelems = (1L << log_nelems);    // 1L: 1을 long 형으로 만듦
    nelems_per_thread = nelems / nthreads;
    sem_init(&mutex, 0, 1);

    for (int i = 0; i < nthreads; i++){
        // 영역식별을 위한 id를 각 쓰레드에 전달. 이 id는 기존 thread id가 아님!
        myid[i] = i;    // 굳이 이렇게 하는 이유는 race condition 막기 위해서 같음
        Pthread_create(&tid[i], NULL, sum_mutex, &myid[i]);
    }

    for (int i = 0; i < nthreads; i++){
        Pthread_join(tid[i], NULL);
    }

    printf("Result: %ld\n", gsum);
    exit(0);

}

// 매우 느림. 동기화 연산으로 잃는 시간이, 메모리 갱신 병렬화로 얻는 시간보다 많음.
// 원래 병렬처리로 연산을 나누어 빨리 끝내야 하지만
// 여기선 각 스레드가 서로의 연산을 기다려야 하므로 의미가 없음.
void *sum_mutex(void *vargp){
    long myid = *((long *)vargp);
    long start = myid * nelems_per_thread;
    long end = start + nelems_per_thread;

    for (long i = start; i < end; i++){
        P(&mutex);
        gsum += i;
        V(&mutex);
    }

    return NULL;
}

// 각 쓰레드에 갱신할 고유 메모리 위치를 독립적으로 줌
// 갱신을 뮤텍스로 보호할 필요가 없음
void *sum_array(void *vargp){
    long myid = *((long *)vargp);
    long start = myid * nelems_per_thread;
    long end = start + nelems_per_thread;
    
    for (long i = start; i < end; i++){
        psum[myid] += 1;
    }

    return NULL;
}

// 지역변수 사용 -> 불필요한 메모리 참조 X. 빨라짐.
void *sum_array(void *vargp){
    long myid = *((long *)vargp);
    long start = myid * nelems_per_thread;
    long end = start + nelems_per_thread;
    
    for (long i = start; i < end; i++){
        sum += 1;
    }
    psum[myid] = sum;
    return NULL;
}

