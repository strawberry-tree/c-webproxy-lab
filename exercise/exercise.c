// 클라이언트에서 호출되는 함수이다.
// 호스트 hostname에서 돌아가고, 포트번호 port에 연결 요청을 듣는 서버와 연결을 설정한다.
// 소켓 식별자를 반환한다.
int open_clientfd(char *hostname, char *port){
    int clientfd;   // 소켓 식별자 저장
    struct addrinfo hints, *listp, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; // 연결을 위해
    hints.ai_flags = AI_NUMERICSERV; // service 인자에 포트번호만 들어올 수 있음
    hints.ai_flags |= AI_ADDRCONFIG; // 호스트가 IPv4로 설정되면 IPv4 반환. v6도 마찬가지
    Getaddrinfo(hostname, port, &hints, &listp);

    for (p = listp; p; p = p -> ai_next){
        // 소켓 설정에 실패했으면 continue
        if ((clientfd = socket(p -> ai_family, p -> ai_socktype, p -> ai_protocol)) < 0) continue;

        // 소켓 식별자와 소켓을 connect하려고 시도.
        if (connect(clientfd, p -> ai_addr, p -> ai_addrlen) != -1){
            break;          // 성공한 경우
        }
        Close(clientfd);    // 실패한 경우 닫고 리스트 다음 노드로 넘어가 재시도
    }

    Freeaddrinfo(listp);    // 리스트누수 방지
    if (!p){               
        return -1;          // 모든 연결 실패
    } else {
        return clientfd;
    }

}

// 서버에서 호출되는 함수이다.
// 연결 요청을 받을 준비가 된, 듣기 식별자를 생성한다.
// 포트번호만 매개변수로 받는다.

int open_listfd(char *port){
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1;
    // listenfd는 반환될 듣기 식별자

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; // 연결을 위해
    hints.ai_flags = AI_NUMERICSERV; // service 인자에 포트번호만 들어올 수 있음
    hints.ai_flags |= AI_ADDRCONFIG; // 호스트가 IPv4로 설정되면 IPv4 반환. v6도 마찬가지

    // host == NULL 이면 로컬 머신을 의미함.
    // 자신의 모든 IP 주소에 바인딩할 수 있는 주소 리스트 생성.
    Getaddrinfo(NULL, port, &hints, &listp);

    for (p = listp; p; p = p -> ai_next){
        // 소켓 식별자 만들기
        if ((listenfd = socket(p -> ai_family, p -> ai_socktype, p -> ai_protocol)) < 0) continue;

        // 서버 소켓을 재사용 가능하도록 설정
        // OS가 이전 소켓을 아직 완전히 닫지 않았음. TIME_WAIT 상태인데, 같은 포트에 bind하려고 하면 실패함.
        Setsockopt(listenfd, SOL_SOCKET, SO_RESUREADDR, (const void *)&optval, sizeof(int));

        // 소켓 식별자와 소켓을 bind하려고 시도.
        if (bind(listenfd, p -> ai_addr, p -> ai_addrlen) != -1)
        break;

        Close(listenfd);    // bind가 실패한 경우

    }

    Freeaddrinfo(listp);
    if (!p) return -1;

    if (listen(listenfd, LISTENQ) < 0){
        Close(listenfd);
        return -1;
    }
    else return clientfd;

}