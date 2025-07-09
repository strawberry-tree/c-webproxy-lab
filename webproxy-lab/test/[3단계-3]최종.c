#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36\r\n";

// 캐시를 연결 리스트 형태로 구현
struct _node {
  char *prefix;                 // 호스트네임 (캐시 경로 매칭 용도)
  char *portNo;                 // 포트번호 (캐시 경로 매칭 용도)
  char *suffix;                 // URI (캐시 경로 매칭 용도)
  char *data;                   // 캐시에 저장할 web object
  struct _node *prev;           // 이전 노드
  struct _node *next;           // 다음 노드
  ssize_t object_size;          // web object는 MAX_OBJECT_SIZE를 초과할 수 없음
};

typedef struct _node Node;

typedef struct {
  Node *head;          // 머리 노드
  Node *tail;          // 꼬리 노드
  ssize_t cache_size;   // MAX_CACHE_SIZE를 초과할 수 없음
} Cache;

// semaphore 사용 위함
volatile int read_count = 0;    // 현재 캐시를 읽고 있는 쓰레드 수
sem_t mutex, w;                 // 각각 Reader, Writer 세마포어

// 캐시 (연결리스트) 전역변수 선언
Cache cache;
  
void insert_node(Node *);
void remove_node(Node *);
void *doit(void *vargp);
void read_requesthdrs(rio_t *rp, char *write_buf, char *prefix);
void parse_url(char *url, char *prefix, char *portNo, char *suffix);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
Node *find_cache(char *prefix, char *portNo, char *suffix);

// 캐시 연결리스트 맨 앞에 노드를 추가 (Writer)
void insert_node(Node *_node){
  P(&w);
  if (_node == NULL){
    V(&w);
    return;
  } 
  
  cache.cache_size += _node -> object_size;   // 전체용량 추가
  _node -> prev = NULL;
  _node -> next = cache.head;

  // 빈 리스트인 경우
  if (cache.head == NULL){
    cache.tail = _node;
  } else {
    cache.head -> prev = _node;
  }
  cache.head = _node;
  V(&w);
}

// 캐시 연결리스트 맨 뒤의 노드를 제거 (Writer)
void remove_node(Node *_node){
  P(&w);
  if (cache.head == NULL || cache.tail == NULL || _node == NULL) {
    V(&w);
    return; 
  }
  
  cache.cache_size -= _node -> object_size;


  // 이전 노드와 다음 노드를 연결 (next 포인터 설정)
  if (_node -> prev == NULL){
    cache.head = _node -> next;
  } else {
    _node -> prev -> next = _node -> next;
  }

  // 이전 노드와 다음 노드를 연결 (prev 포인터 설정)
  if (_node -> next == NULL){
    cache.tail = _node -> prev;
  } else {
    _node -> next -> prev = _node -> prev;
  }

  // 노드 구조체 요소 및 노드 구조체를 free
  free(_node -> prefix);
  free(_node -> portNo);
  free(_node -> suffix);
  free(_node -> data);
  free(_node);
  V(&w);
}

// 노드를 맨 앞으로 이동
void front_node(Node *_node){
  P(&w);
  // 예외 처리
  if (cache.head == NULL || cache.tail == NULL || _node == NULL) {
    V(&w);
    return; 
  }

  // 이미 노드가 맨 앞에 있는 경우
  if(_node == cache.head){
    V(&w);
    return;
  }

  
  // 이전 노드와 다음 노드를 연결 (next 포인터 설정)
  _node -> prev -> next = _node -> next;

  // 이전 노드와 다음 노드를 연결 (prev 포인터 설정)
  if (_node -> next == NULL){
    cache.tail = _node -> prev;
  } else {
    _node -> next -> prev = _node -> prev;
  }

  // 노드를 맨 앞에 보냄
  _node -> next = cache.head;
  cache.head -> prev = _node;
  _node -> prev = NULL;
  cache.head = _node;

  V(&w);
}

// 경로를 이용해 캐시에 캐싱되어 있는지 확인 (Reader)
Node *find_cache(char *prefix, char *portNo, char *suffix){
  P(&mutex);
  read_count++;
  if (read_count == 1)
    P(&w);
  V(&mutex);

  Node *curr = NULL;

  if (cache.head != NULL){
    for (curr = cache.head; curr != NULL; curr = curr -> next){
      if (!strcmp(curr -> prefix, prefix) && !strcmp(curr -> portNo, portNo) && !strcmp(curr -> suffix, suffix)){
        // prefix, portNo, suffix가 모두 동일한 경로가 캐시되어 있는 경우, 캐시 히트
        break;
      }
    }
  }
  
  P(&mutex);
  read_count--;
  if (read_count == 0)
    V(&w);
  V(&mutex);
  
  // 캐시 히트인 경우 해당 노드를, 캐시 미스인 경우는 NULL 포인터를 반환.
  return curr;
}

int main(int argc, char **argv)
{
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  // 캐시 초기화
  cache.head = NULL;
  cache.tail = NULL;
  cache.cache_size = 0;

  // 세마포어 초기화
  Sem_init(&mutex, 0, 1);
  Sem_init(&w, 0, 1);

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 포트 번호를 이용해 듣기 식별자 반환
  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    // 듣기 소켓을 클라이언트와 연결
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("(%d): Accepted connection from (%s, %s)\n", *connfdp, hostname, port);

    // 연결되면 해당 연결 소켓의 통신을 담당할 쓰레드를 만듦
    Pthread_create(&tid, NULL, doit, connfdp);
  }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void *doit(void *vargp)
{
  int fd = *((int *)vargp);

  // 루틴 종료 시 쓰레드가 자동으로 메모리 자원을 반환하게끔 함
  Pthread_detach(pthread_self());
  free(vargp);
  int final_fd;     // 최종 서버로의 소켓식별자
  
  // 읽기 / 쓰기 / 캐싱용 사용자 버퍼
  char read_buf[MAXLINE], write_buf[MAXLINE], cache_buf[MAX_OBJECT_SIZE];
  
  // 클라이언트로부터 request
  char method[MAXLINE], url[MAXLINE], version[MAXLINE];

  // url 파싱용 버퍼
  char prefix[MAXLINE], portNo[MAXLINE], suffix[MAXLINE];
  rio_t rio, final_rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (!Rio_readlineb(&rio, read_buf, MAXLINE))
    return NULL;
  printf("%s", read_buf);
  sscanf(read_buf, "%s %s %s", method, url, version);
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not Implemented",
                "Proxy does not implement this method");
    Close(fd);
    return NULL;
  }

  /* Parse URL from GET request */
  // prefix엔 호스트네임이, portNo엔 포트번호 (기본값 80), suffix엔 URI가 옴
  parse_url(url, prefix, portNo, suffix);

  // [method] [suffix] HTTP/1.0로 request line 작성
  snprintf(write_buf, sizeof(write_buf), "%s %s HTTP/1.0\r\n", method, suffix);

  // request header도 작성
  read_requesthdrs(&rio, write_buf, prefix);
  strcat(write_buf, "\r\n");

  // 만약 이미 캐시에 저장되어 있다면, 이후 절차를 건너뛰고 바로 저장 데이터 반환 가능
  Node *cache_node;
  if ((cache_node = (Node *)find_cache(prefix, portNo, suffix)) != NULL){
    // 여기서 클라이언트에 보내고 바로 리턴하면 됨.
    Rio_writen(fd, cache_node -> data, cache_node -> object_size);
    Close(fd);
    // 현재 확인한 캐시를 맨 앞으로 보내줌 -> 이후 탐색 시 제일 먼저 확인됨.
    front_node(cache_node);
    return NULL;
  }

  // 보낼 서버와 연결된 소켓식별자 및 Rio 버퍼를 준비
  final_fd = Open_clientfd(prefix, portNo);
  Rio_readinitb(&final_rio, final_fd);
  
  // 이제 우리가 요청을 보내야지
  Rio_writen(final_fd, write_buf, strlen(write_buf));
  printf("%s", write_buf);

  // 서버의 응답을 클라이언트에 보냄과 동시에
  // 데이터 캐싱에 사용될 cache_buf에 복사함
  ssize_t n;                // Rio_readnb가 반환하는 바이트 수
  ssize_t buffer_size = 0;  // 현재 cache_buf에 담긴 바이트 수
  int buffer_full = 0;      // buffer_size가 캐시 MAX_OBJECT_SIZE 초과 시 1이 됨
  // 그리고 서버에서 요청의 응답을 받으면, 클라이언트로 보낸다
  // 중간에 짤리니까 이거 해결해야 함.
  while ((n = Rio_readnb(&final_rio, read_buf, MAXLINE)) > 0){
    Rio_writen(fd, read_buf, n);
    if (buffer_size + n <= MAX_OBJECT_SIZE){
      memcpy(cache_buf + buffer_size, read_buf, n);
      buffer_size += n;
    } else {
      buffer_full = 1;
    }
  };

  if(!buffer_full){
    // 공간 부족 시, 연결 리스트 끝에 있는 노드 제거 (LRU 정책)
    while (cache.cache_size + buffer_size > MAX_CACHE_SIZE ){
        remove_node(cache.tail);
    }

    // 새로운 노드를 만들고, 내용을 복사한 뒤, 연결 리스트에 삽입
    Node *new_node = Malloc(sizeof(Node));

    new_node -> prefix = strdup(prefix);
    new_node -> portNo = strdup(portNo);
    new_node -> suffix = strdup(suffix);
    new_node -> data = Malloc(buffer_size);
    memcpy(new_node -> data, cache_buf, buffer_size);
    new_node -> object_size = buffer_size;
    insert_node(new_node);  // 연결리스트 삽입
  }

  Close(final_fd);
  Close(fd);
  return NULL;
}

/*
 * read_requesthdrs - read HTTP request headers
 * 그리고 내용을 forward할 수 있도록, 쓰기 버퍼 write_buf에 넣기
 */
void read_requesthdrs(rio_t *rp, char *write_buf, char *prefix)
{
  char read_buf[MAXLINE];
  
  // 이렇게 작성하라고 명시된 헤더 4개
  strcat(write_buf, "Host: ");
  strcat(write_buf, prefix);
  strcat(write_buf, "\r\n");
  strcat(write_buf, user_agent_hdr);
  strcat(write_buf, "Connection: close\r\n");
  strcat(write_buf, "Proxy-Connection: close\r\n");

  do{
    Rio_readlineb(rp, read_buf, MAXLINE);

    // 위에서 명시한 4개의 헤더는 무시한다.
    if (!strncmp(read_buf, "Host:", strlen("Host:")) ||
        !strncmp(read_buf, "User-Agent:", strlen("User-Agent:")) ||
        !strncmp(read_buf, "Connection:", strlen("Connection:")) ||
        !strncmp(read_buf, "Proxy-Connection:", strlen("Proxy-Connection"))){
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
  if ((suffix_ptr = strchr(start_ptr, '/'))){
    strcpy(suffix, suffix_ptr);
    *suffix_ptr = '\0';
  } else {
    strcpy(suffix, "/");
  }

  // 포트 번호가 존재하는 요청이면 포트 번호를 파싱.
  if ((port_ptr = strchr(start_ptr, ':'))){
    strcpy(portNo, port_ptr + 1);
    *port_ptr = '\0';
  } else {
    strcpy(portNo, "80");
  }

  // prefix를 파싱.
  strcpy(prefix, start_ptr);
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Proxy Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Proxy Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
