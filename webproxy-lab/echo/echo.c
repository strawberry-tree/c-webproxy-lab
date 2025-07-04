#include "csapp.h"

void echo(int connfd){
    size_t n;
    char buf[MAXLINE];  // 사용자 버퍼 (송수신 데이터 저장)
    rio_t rio;

    // 수신을 받을 버퍼 초기화
    Rio_readinitb(&rio, connfd);

    // 버퍼에서 MAXLINE만큼 읽은 뒤, buf에 저장
    // 반환값 n은 읽은 바이트 수
    // 클라이언트 연결 종료 시 0을 반환
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
        printf("Server received %d bytes\n", (int)n);
        // buf에 저장된 데이터를 클라이언트에게 n바이트만큼 보냄
        Rio_writen(connfd, buf, n);
    }
}