#include "csapp.h"

int main(int argc, char **argv){
    int clientfd;           // 클라이언트 소켓 명시자
    char *host, *port;      // 호스트명, 포트명
    char buf[MAXLINE];      // 사용자 버퍼 (송수신 데이터 저장)
    rio_t rio;              // RIO 구조체

    // 잘못 입력했을 때
    if (argc != 3){
        fprintf(stderr, "usage: %s <host> <port>", argv[0]);
    }
    
    host = argv[1];
    port = argv[2];

    // 해당 호스트명, 포트명의 서버와 연결 후, 클라이언트 소켓 명시자 반환
    clientfd = Open_clientfd(host, port);

    // 버퍼 초기화
    Rio_readinitb(&rio, clientfd);

    // 표준 입력 -> 키보드로 입력한 데이터를 사용자 버퍼 buf에 저장
    // 아무것도 입력되지 않으면 NULL을 반환. 입력 바이트 수를 반환하기 때문
    while (Fgets(buf, MAXLINE, stdin) != NULL){
        // buf에 저장된 데이터를 서버로 송신. 송신 종료시까지 블로킹.
        Rio_writen(clientfd, buf, strlen(buf));
        // 서버에서 수신받은 데이터를 buf에 저장. 수신될 때까지 블로킹.
        Rio_readlineb(&rio, buf, MAXLINE);
        // buf에 저장된 데이터를 출력.
        Fputs(buf, stdout);
    }

    Close(clientfd);    // 연결 종료
    exit(0);
}