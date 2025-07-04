#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv){
    int listenfd, connfd;                   // 듣기 식별자 / 연결 식별자
    socklen_t clientlen;                    // 클라이언트 소켓주소 구조체의 크기.
    struct sockaddr_storage clientaddr;     // sockaddr_in 대신 이걸 사용. 어느 주소든 충분히 저장 가능.
    char client_hostname[MAXLINE], client_port[MAXLINE];    // 클라이언트 호스트네임, 포트번호 저장

    // 잘못된 호출
    if (argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // 입력받은 포트 번호로, 소켓을 열고 듣기 식별자 반환
    listenfd = Open_listenfd(argv[1]);
    
    // 무한 반복
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);

        // 클라이언트와의 연결을 대기
        // 연결 완료 시 연결 식별자 반환. clientaddr에는 클라이언트 소켓주소, clientlen엔 그 크기 저장.
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        // 소켓주소 구조체 clientaddr을, 대응되는 호스트(client_hostname)와 서비스이름 스트링(client_port)으로 변환
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        
        // 텍스트 줄을 읽고 echo해주는 함수. 클라이언트의 연결이 끝날 때 까지 무한반복됨.
        echo(connfd);

        // 연결이 끝나면 연결 식별자를 닫음. 단 듣기 식별자로 여전히 새로운 연결 가능.
        Close(connfd);
    }
    exit(0);
}