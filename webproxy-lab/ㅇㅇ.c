/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);  // 한 개의 HTTP 트랜잭션을 처리한다.
void read_requesthdrs(rio_t *rp); // 요청 헤더를 읽고 무시한다.
int parse_uri(char *uri, char *filename, char *cgiargs);  // HTTP URI(접미어)를 분석한다.
void serve_static(int fd, char *filename, int filesize, int getflag);  // 정적 콘텐츠를 클라이언트에 서비스한다.
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, int getflag); // 동적 콘텐츠를 클라이언트에 서비스한다.
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg); // 에러메시지를 클라이언트에게 보낸다.

int main(int argc, char **argv)
{
  int listenfd, connfd;                   // 듣기 식별자, 연결 식별자
  char hostname[MAXLINE], port[MAXLINE];  // 연결된 클라이언트의 호스트네임, 포트번호 저장
  socklen_t clientlen;                    // 클라이언트 주소구조체의 크기
  struct sockaddr_storage clientaddr;     // 클라이언으 주소구조체

  /* 입력이 잘못된 경우  */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 해당 포트 번호로 서버 소켓을 만들어 열고, 듣기식별자 반환
  listenfd = Open_listenfd(argv[1]);

  printf("서버가 준비되었습니다\n");

  // 무한반복
  while (1)
  {
    clientlen = sizeof(clientaddr);
    printf("수락 대기 중...\n");

    // 클라이언트의 연결 요청을 수락. connfd에 연결식별자 저장됨
    // clientaddr에 클라이언트 주소정보, clientlen에 size를 저장.
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept

    // 클라이언트 주소정보를 통해, 호스트네임과 포트번호를 확인
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("%d, Accepted connection from (%s, %s)\n", connfd, hostname, port);

    // 1개의 HTTP 트랜잭션을 처리
    doit(connfd);  // line:netp:tiny:doit

    // 연결 종료 
    // (한 연결이 한 트랜잭션만 처리하므로 HTTP/1.0임을 대충 알 수 있음)
    printf("%d, Closed connection\n", connfd);
    Close(connfd); // line:netp:tiny:close
  }
}

// 하나의 HTTP 트랜잭션을 처리한다.
// fd: 연결 명시자.
void doit(int fd){
    printf("DOIT 실행: %d\n", fd);
    /* (0) 변수 선언 */
    int is_static;    // 동적? 정적?
    struct stat sbuf; // 파일 정보를 저장하는 구조체. stat 함수와 함께 사용.

    // buf에는 HTTP 요청 라인 전체를 저장. 
    // HTTP 요청 라인의 구성요소인 method / URI / version을 저장.
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    
    // 파일명과 cgi함수에 전달할 인자를 저장.
    char filename[MAXLINE], cgiargs[MAXLINE];

    // rio 구조체.
    rio_t rio;

    /* (1) 요청 라인 및 요청 헤더를 읽음 */
    // rio 함수 사용 위한 버퍼 초기화
    Rio_readinitb(&rio, fd);
    // 입력 한 줄 읽고, buf에 저장.
    if (Rio_readlineb(&rio, buf, MAXLINE) <= 0){
      return;
    }
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    // 메서드가 GET이 아닌 경우, 구현되지 않았으니 에러를 클라이언트로 전달. 물론 구현을 해야겠지 나중에.
    int getflag;
    if (!strcasecmp(method, "GET")){
      getflag = 1;
    } else if (!strcasecmp(method, "HEAD")){
      getflag = 0;
    } else {
      clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
      return; // main 루틴으로 돌아감.
    }

    read_requesthdrs(&rio); // 요청 헤더 읽고 무시. 나중에 바꿀 땐 무시하지 않게 바꿔야겠지.

    /* (2) GET 요청에서 URI를 parse하기 */
    // uri를 parse해서 filename, cgiargs에 저장하는 형식임에 유의.
    is_static = parse_uri(uri, filename, cgiargs);  // 정적 요청이면 1 아니면 0

    // stat 함수 실행이 실패한 경우
    if (stat(filename, &sbuf) < 0){
      clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
      return;
    }

    /* (3) 정적 or 동적 콘텐츠 */

    // 정적
    if (is_static){
      // 일반 파일이 아니거나 (디렉토리/소켓 등), 읽기 권한이 없는 경우
      if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
        clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
        return;
      }
      printf("Serve Static\n");
      // sbuf.st_size는 파일의 크기
      serve_static(fd, filename, sbuf.st_size, getflag);
    } else {  // 동적
      // 일반 파일이 아니거나 (딜게토리/소켓 등), 실행 권한이 없는 경우
      if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
        clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
        return;
      }
      printf("Serve Dynamic\n");
      serve_dynamic(fd, filename, cgiargs, getflag);
    }
}

// 에러 메시지를 만들고, 클라이언트에게 보낸다.
// fd: 연결 명시자.
// cause: 오류 원인 (GET 이외의 메서드, 없는 파일명 등)
// errnum: 오류 번호 (404 등), shortmsg: 오류 번호에 대응되는 메시지 (Not Found 등)
// longmsg: 조금 더 길게 오류를 설명하는 메시지
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
  /* (0) 변수 설정 */
  
  // buf: HTTP 응답 (응답 라인, 요청 헤더, 헤더 종료 빈 줄)
  // body: 리턴할 HTML 컨텐츠 (응답 본체)
  char buf[MAXLINE], body[MAXBUF];

  /* (1) 응답 본체 만들기 */
  // 사용자버퍼 body에 아래 문자열을 계속 sprintf로 출력
  // 네트워크 통신에서 줄바꿈을 명확히 하기 위해서 \r\n 사용.
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);  // em: 강조용 태그

  /* (2) HTTP 응답 만들기. 이와 동시에 클라이언트에 전송도 함. */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

// 클라이언트가 요청한 헤더를 읽고, 놀랍게도 아무것도 하지 않는다.
// 그야 헤더를 쓸 일이 없으니까 이 서버에선. 나중에 수정하게 될 것 같다.
void read_requesthdrs(rio_t *rp){
  // buf: 클라이언트에서 전송된 요청 헤더를 저장하는 사용자 버퍼.
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);
  
  // buf에 담긴 값이 "\r\n"이 아닌 경우.
  // 요청 헤더를 종료할 때 빈 텍스트 줄을 보내야 함을 기억할 것.
  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);  // 서버 프로세스에서 출력만 함.
  }
  return;
}

// URI를 parse
// 파일의 상대경로를 filename에 저장
// 추가로 동적 콘텐츠인 경우, 전달할 인자 문자열을 cgiargs에 저장
// 정적 콘텐츠는 현재 디렉토리, 실행 파일은 /cgi-bin� 존재한다고 가정
int parse_uri(char *uri, char *filename, char *cgiargs){
  char *ptr;

  
  if (!strstr(uri, "cgi-bin")){
    // 정적 콘텐츠일 때 (uri에 "cgi-bin"이 포함되지 않음)

    // 정적 콘텐츠이므로, cgiargs는 빈칸으로
    strcpy(cgiargs, "");
    
    // filename: URI 앞에 "."을 붙임 ("./index.html" 꼴)
    strcpy(filename, ".");
    strcat(filename, uri);

    // URI의 마지막 문자가 '/'인 경우, 기본 파일인 "home.html"을 추가
    if (uri[strlen(uri) - 1] == '/'){
      strcat(filename, "home.html");
    }
    return 1; // 정적콘텐츠이면 1 반환
  } else {
    // 동적 콘텐츠일 때 (uri에 "cgi-bin"이 포함됨)
    ptr = strchr(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr+1);   // cgi 인자들을 추출해 cgiargs에 저장
      *ptr = '\0';              // `?` 및 `?` 이후 문자를 문자열에서 제거
    } else {
      strcpy(cgiargs, "");
    }
    // ? 앞에 있는 나머지 URI 부분을 파일 상대 경로로 변경
    // 이후 filename에 저장
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0; // 동적콘텐츠이면 0 반환
  }
}

// 정적 컨텐츠를 클라이언트에게 서비스
// fd: 연결 소켓, filename: 파일명, filesize: 파일의 크기
void serve_static(int fd, char *filename, int filesize, int getflag){
  int srcfd;  // 파일 명시자
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  // 응답 헤더 구성
  get_filetype(filename, filetype); // 파일타입을 filetype에 저장
  
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sFilename: %s\r\n", buf, filename);
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);

  // 응답 헤더를 보냄
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  // HEAD 요청인 경우 응답 본체를 보내지 않음
  if (!getflag) return;

  // 응답 본체를 보냄
  // 읽기 전용(O_RDONLY)으로 파일을 열고, 파일 명시자를 반환
  // 0은 파일이 생성된 후 권한을 나타내는 비트인데
  // 차피 여기선 파일을 생성하지, 읽진 않음. 0으로 해도됨
  srcfd = Open(filename, O_RDONLY, 0);
  if (srcfd < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't open the file");
    return;
}

  // // 파일 scrfd의 앞 filesize 바이트를, 주소 scrp에서 시작하는
  // // 사적 / 읽기-허용 가상메모리 영역으로 매핑
  // // start는 NULL -> 그냥 알아서 적절한 데에다 생성해 줌. 시작 포인터 반환.
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // Close(srcfd); // 매핑 완료됐으니 파일 닫기. 메모리 누수 막기 위함
  // Rio_writen(fd, srcp, filesize); // 클라이언트에 파일 전송
  // Munmap(srcp, filesize); // 매핑된 가상메모리 주소 반환

  srcp = Malloc(filesize);          // filesize를 저장할 수 있는 공간 확보
  Rio_readn(srcfd, srcp, filesize); // srcfd의 데이터를 메모리 scrp에 filesize만큼 복사
  Close(srcfd);                     // 매핑 완료됐으니 파일 닫기. 메모리 누수 막기 위함.
  Rio_writen(fd, srcp, filesize);   // 클라이언트에 파일 전송
  Free(srcp);                       // 메모리 할당해제
}

void get_filetype(char *filename, char *filetype){
  // 파일명에 포함된 확장자에 따라, filetype에 적절한 파일명 저장.
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

// 동적 콘텐츠 제공.
// fd: 연결 식별자, filename: 파일명, cgiargs: cgi 프로그램에 보낼 인자
void serve_dynamic(int fd, char *filename, char *cgiargs, int getflag){
  // buf: HTTP 응답 저장용 버퍼
  // emptylist: 인자를 저장할 리스트????
  char buf[MAXLINE], *arglist[] = {filename, getflag ? "1" : "0", NULL};

  // HTTP 응답 반환
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  

  // 자녀 프로세스 생성. 자녀는 fork호출 직후부터 실행됨.
  if (Fork() == 0){
    // QUERY_STRING (CGI 환경변수) 를 cgiargs로 설정.
    // 1: 기존 환경 변수가 있어도 덮어쓴다는 뜻.
    // 실제 서버는 여기서 다른 환경변수도 설정함.
    setenv("QUERY_STRING", cgiargs, 1);
    

    // 자식의 표준 출력을, 연결 파일 식별자로 재지정
    Dup2(fd, STDOUT_FILENO);

    // 인자리스트 emptylist, 환경변수리스트 environ
    // environ은 전역 변수로, 운영체제가 프로그램 실행 시에 만들어 주는 환경 변수 리스트
    // filename을 실행
    Execve(filename, arglist, environ);
  }

  Wait(NULL); // 부모 프로세스는 대기 -> 자식 프로세스 청소
}
de)) || !(S_IXUSR & sbuf.st_mode)){
        clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
        return;
      }
      printf("Serve Dynamic\n");
      serve_dynamic(fd, filename, cgiargs, getflag);
    }
}

// 에러 메시지를 만들고, 클라이언트에게 보낸다.
// fd: 연결 명시자.
// cause: 오류 원인 (GET 이외의 메서드, 없는 파일명 등)
// errnum: 오류 번호 (404 등), shortmsg: 오류 번호에 대응되는 메시지 (Not Found 등)
// longmsg: 조금 더 길게 오류를 설명하는 메시지
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
  /* (0) 변수 설정 */
  
  // buf: HTTP 응답 (응답 라인, 요청 헤더, 헤더 종료 빈 줄)
  // body: 리턴할 HTML 컨텐츠 (응답 본체)
  char buf[MAXLINE], body[MAXBUF];

  /* (1) 응답 본체 만들기 */
  // 사용자버퍼 body에 아래 문자열을 계속 sprintf로 출력
  // 네트워크 통신에서 줄바꿈을 명확히 하기 위해서 \r\n 사용.
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);  // em: 강조용 태그

  /* (2) HTTP 응답 만들기. 이와 동시에 클라이언트에 전송도 함. */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

// 클라이언트가 요청한 헤더를 읽고, 놀랍게도 아무것도 하지 않는다.
// 그야 헤더를 쓸 일이 없으니까 이 서버에선. 나중에 수정하게 될 것 같다.
void read_requesthdrs(rio_t *rp){
  // buf: 클라이언트에서 전송된 요청 헤더를 저장하는 사용자 버퍼.
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);
  
  // buf에 담긴 값이 "\r\n"이 아닌 경우.
  // 요청 헤더를 종료할 때 빈 텍스트 줄을 보내야 함을 기억할 것.
  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);  // 서버 프로세스에서 출력만 함.
  }
  return;
}

// URI를 parse
// 파일의 상대경로를 filename에 저장
// 추가로 동적 콘텐츠인 경우, 전달할 인자 문자열을 cgiargs에 저장
// 정적 콘텐츠는 현재 디렉토리, 실행 파일은 /cgi-bin�Connection closed by foreign host.