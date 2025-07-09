#include "csapp.h"

static int byte_count;  // 모든 클라이언트로부터 echo에서 총 몇 바이트를 읽었는지
static sem_t mutex;     // 공유변수 byte_count를 보호하는 뮤텍스

static void init_echo_count(void){
    Sem_init(&mutex, 0, 1);
    byte_count = 0;
}

void echo_count(int connfd){
    int n;
    char buf[MAXLINE];  // 사용자 버퍼 (송수신 데이터 저장)
    rio_t rio;

    // **POSIX 스레드(pthread)**를 사용할 때 한 번만 실행되어야 하는 초기화 코드를 작성
    static pthread_once_t once = PTHREAD_ONCE_INIT;

    // init_echo_count는 한 번만 실행됨. 한번 실행되면 이후엔 아무것도 발생하지 않음.
    // (대신 매번 pthread_once가 호출되는 단점이 있긴 하다.)
    Pthread_once(&once, init_echo_count);

    // 수신을 받을 버퍼 초기화
    Rio_readinitb(&rio, connfd);

    // 버퍼에서 MAXLINE만큼 읽은 뒤, buf에 저장
    // 반환값 n은 읽은 바이트 수
    // 클라이언트 연결 종료 시 0을 반환
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
        P(&mutex);
        byte_count += n;
        printf("Server received %d (%d total) bytes on fd %d\n", n, byte_count, connfd);
        // buf에 저장된 데이터를 클라이언트에게 n바이트만큼 보냄
        V(&mutex);
        Rio_writen(connfd, buf, n);
    }
}