#include "csapp.h"
#include "sbuf.h"
#define NTHREADS 4
#define SBUFSIZE 16

void echo_count(int connfd);
void *thread(void *vargp);  // 쓰레드에서 실행할 루틴

sbuf_t sbuf;                // shared buffer: 연결 명시자의 버퍼

int main(int argc, char **argv){
    int listenfd, connfd;                   // 듣기 식별자 / 연결 식별자 포인터
    socklen_t clientlen;                    // 클라이언트 소켓주소 구조체의 크기.
    struct sockaddr_storage clientaddr;     // sockaddr_in 대신 이걸 사용. 어느 주소든 충분히 저장 가능.
    pthread_t tid;                          // 쓰레드 ID 저장
    char client_hostname[MAXLINE], client_port[MAXLINE];    // 클라이언트 호스트네임, 포트번호 저장

    // 잘못된 호출
    if (argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // 입력받은 포트 번호로, 소켓을 열고 듣기 식별자 반환
    listenfd = Open_listenfd(argv[1]);
    
    sbuf_init(&sbuf, SBUFSIZE);     // 버퍼를 초기화하기
    
    // 4개의 쓰레드를 미리 만들어줌
    for (int i = 0; i < NTHREADS; i++){
        Pthread_create(&tid, NULL, thread, NULL);
    }

    // Accept될 때마다 sbuf에 식별자 넣기
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd);

    }
}

// 쓰레드 함수
// 연결 식별자를 제거할 수 있을 때까지 기다림
// 이후 쓰레드 중 하나를 sbuf에서 꺼냄
void *thread(void *vargp){
    Pthread_detach(pthread_self()); // 쓰레드 분리하기
    while (1) {
        int connfd = sbuf_remove(&sbuf);
        echo_count(connfd);
        Close(connfd);
    }
}