#include "csapp.h"

int main(int argc, char **argv){
    // argc: 전달된 인자의 개수
    // argv: 문자열 포인터 배열 (char *argv[]와 동일)
    // 이때 argv[0]은 프로그램 이름, 1, 2부터 첫째, 둘째 사용자 인자

    struct addrinfo *p, *listp;
    // listp: getaddrinfo()가 반환하는 주소 정보 리스트의 첫 번째 노드를 가리킴
    // p: 주소 리스트를 순회할 때 사용하는 임시 포인터

    struct addrinfo hints;
    // hints: getaddrinfo()에 전달할 입력 조건 설정. 주소 타입(ai_family), 소켓 타입(ai_socktype) 설정.

    char buf[MAXLINE];                  // 한 줄 최대 길이는 8192 바이트
    int rc, flags;
    // rc: getaddrinfo의 결과 저장


    if (argc != 2){
        fprintf(stderr, "usage: %s <domain name>\n", argv[0]);
        exit(0);
    }

    // memset: 시작 주소부터 일정 바이트까지 값을 원하는 값으로 초기화함.
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // 연결 용도로만 사용

    // getaddrinfo로, 1번째 인자(호스트주소)를 소켓 주소 구조체로 변환
    // 소켓 주소 구조체는 연결 리스트로 구성되는데, 노드의 주소를 listp에 저장.
    // 단 실패하면 에러를 띄움
    if ((rc = getaddrinfo(argv[1], NULL, &hints, &listp)) != 0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        exit(1);
    }

    // 리스트의 각 IP주소를 표시
    flags = NI_NUMERICHOST; // 도메인이름 대신 숫자주소 스트링을 리턴
    
    // p가 NULL일 때까지 리스트 탐색
    for (p = listp; p; p = p -> ai_next){
        Getnameinfo(p -> ai_addr, p -> ai_addrlen, buf, MAXLINE, NULL, 0, flags);
        // 소켓 주소 구조체, 해당 구조체의 길이, 도메인네임 OR 숫자주소 스트링을 저장할 버퍼, 버퍼의 길이, 포트번호 OR 서비스명을 저장할 버퍼, 버퍼의 길이, 설정 플래그
        printf("%s\n", buf);
    }

    Freeaddrinfo(listp);
    exit(0);
}