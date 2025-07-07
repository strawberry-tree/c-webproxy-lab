#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36\r\n";

void doit(int fd);
void read_requesthdrs(rio_t *rp, char *write_buf, char *prefix);
void parse_url(char *url, char *prefix, char *portNo, char *suffix);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);


int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd)
{
  int final_fd;     // 최종 서버로의 소켓식별자
  
  // 사용자 버퍼
  char read_buf[MAXLINE], write_buf[MAXLINE];
  
  // 받은 request
  char method[MAXLINE], url[MAXLINE], version[MAXLINE];

  // url 파싱
  char prefix[MAXLINE], portNo[MAXLINE], suffix[MAXLINE];
  rio_t rio, final_rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (!Rio_readlineb(&rio, read_buf, MAXLINE))
    return;
  printf("%s", read_buf);
  sscanf(read_buf, "%s %s %s", method, url, version);
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
    return;
  }

  

  /* Parse URL from GET request */
  parse_url(url, prefix, portNo, suffix);
  
  // 무엇을 보내야 할까?
  char *p = write_buf;

  // [method] [suffix] HTTP/1.0 꼴로 보냄
  snprintf(write_buf, sizeof(write_buf), "%s %s HTTP/1.0\r\n", method, suffix);

  // request header도 보내긴 해야 할 텐데
  read_requesthdrs(&rio, write_buf, prefix);
  strcat(write_buf, "\r\n");

  // 보낼 서버와 연결된 소켓식별자를 반환
  // 별도의 rio 객체도 필요하다는 점, 기억하자!
  final_fd = Open_clientfd(prefix, portNo);
  Rio_readinitb(&final_rio, final_fd);
  
  // 이제 우리가 요청을 보내야지
  Rio_writen(final_fd, write_buf, strlen(write_buf));
  printf("(%d) -> proxy -> (%d) 서버에 요청을 보냇어요.\n", fd, final_fd);
  printf(write_buf);

  ssize_t n;
  // 그리고 서버에서 요청의 응답을 받으면, 클라이언트로 보낸다
  // 중간에 짤리니까 이거 해결해야 함.
  while ((n = Rio_readnb(&final_rio, read_buf, MAXLINE)) != 0){
    Rio_writen(fd, read_buf, n);
  };

  Close(final_fd);

}

/*
 * read_requesthdrs - read HTTP request headers
 * 그리고 바꾸지 않고 forward할 수 있도록, 쓰기 버퍼 write_buf에 넣기
 */
void read_requesthdrs(rio_t *rp, char *write_buf, char *prefix)
{
  char read_buf[MAXLINE];
  
  strcat(write_buf, "Host: ");
  strcat(write_buf, prefix);
  strcat(write_buf, "\r\n");
  strcat(write_buf, user_agent_hdr);
  strcat(write_buf, "Connection: close\r\n");
  strcat(write_buf, "Proxy-Connection: close\r\n");

  do{
    Rio_readlineb(rp, read_buf, MAXLINE);

    // 위에서 명시한 4개의 헤더는 무시한다.
    if (strncmp(read_buf, "Host:", strlen("Host:")) ||
        strncmp(read_buf, "User-Agent:", strlen("User-Agent:")) ||
        strncmp(read_buf, "Connection:", strlen("Connection:")) ||
        strncmp(read_buf, "Proxy-Connection:", strlen("Proxy-Connection"))){
          continue;
    }

    strcat(write_buf, read_buf);
    printf("%s", read_buf);
  } while (strcmp(read_buf, "\r\n"));
  return;
}

/*
 * parse_url - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
// 브라우저는 `GET http://www.cmu.edu/hub/index.html HTTP/1.1`과 같이 요청을 보냄
// prefix에 www.cmu.edu를
// portNo에 포트 번호를 (명시를 한 경우)
// suffix에 ./hub/index.html을 저장
void parse_url(char *url, char *prefix, char *portNo, char *suffix)
{
  char *start_ptr, *suffix_ptr, *port_ptr;

  // http://, https:// 존재하는 요청이면, :// 다음으로 포인터를 이동.
  if ((start_ptr = strstr(url, "://"))){
    start_ptr += 3;
  } else {
    start_ptr = url;
  }
  

  // suffix를 파싱.
  if ((suffix_ptr = index(start_ptr, '/'))){
    strcpy(suffix, suffix_ptr);
    *suffix_ptr = '\0';
  } else {
    strcpy(suffix, "/");
  }


  // 포트 번호가 존재하는 요청이면 포트 번호를 파싱.
  if ((port_ptr = index(start_ptr, ':'))){
    strcpy(portNo, port_ptr + 1);
    *port_ptr = '\0';
  } else {
    strcpy(portNo, "80");
  }

  // prefix를 파싱.
  strcpy(prefix, start_ptr);
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

