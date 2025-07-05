# 1. `clientlen = sizeof(clientaddr)`로 초기화하는 이유

좋은 질문이에요!

```c
clientlen = sizeof(clientaddr);
connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
```
- 여기서 `clientlen`에 `sizeof(clientaddr)` 값을 저장해두는 이유는, Accept 함수가 클라이언트 주소 정보를 받아올 때 입력/출력 파라미터로 쓰이기 때문입니다.

## 자세한 설명
- `clientlen`은 `Accept` 함수 호출 전에 클라이언트 주소 구조체의 메모리 크기를 알려주기 위해 초기화해야 해요.

- `Accept`는 이 `clientlen` 값을 참조해서 `clientaddr` 버퍼에 클라이언트 주소 정보를 저장할 수 있는 크기를 알고,

- 클라이언트 주소를 받아온 뒤, 실제로 채워진 주소 정보 크기를 `clientlen`에 다시 출력값으로 업데이트합니다.

즉,

- 호출 전: `clientlen` = `clientaddr` 버퍼 크기 (예: 16, 28, 128 등)

- 호출 후: `clientlen` = 실제 클라이언트 주소 정보 크기

이렇게 쓰는 이유는 유연한 주소 크기 처리 때문이에요.

## 왜 꼭 이렇게 해야 할까?
- IPv4, IPv6 등 주소 체계에 따라 주소 길이가 다를 수 있고

- Accept가 실제로 받은 주소 크기를 정확히 알려줘야 이후에 주소 정보를 처리할 때 올바른 길이를 알 수 있습니다.

- 그래서 Accept 호출 전에 clientlen에 버퍼 크기를 줘야 하고, 호출 후에는 실제 채워진 크기로 값이 바뀌는 것이죠.

# 2. `stat` 함수와 `struct stat` 구조체
```c
#include <unistd.h>
#include <sys/stat.h>

int stat(const char *filename, struct stat *buf);
// 성공 0 반환. 실패 -1 반환.
```
- 파일 이름(`filename`)을 입력으로 받아, `stat` 구조체의 멤버를 채움.
- `stat` 구조체의 대표적 멤버
  - `st_size`: 파일 크기를 바이트로 저장.
  - `st_mode`: 파일 권한 비트 + 파일 타입.
- `st_mode` 멤버로 파일 타입을 확인하는 방법: `sys/stat.h`에서 정의된 매크로를 사용함
  - `S_ISREG(sbuf.st_mode)`: 일반 파일인가?
  - `S_ISDIR(sbuf.st_mode)`: 디렉토리인가?
  - `S_ISSOCK(sbuf.st_mode)`: 네트워크 소킷인가?
- `st_mode` 멤버로 파일 권한을 확인하는 방법: `st_mode멤버`와 아래 상수 간 & 연산
  - `sbuf.st_mode & S_IRUSR`: 소유자의 읽기 권한이 존재하는가?
  - `sbuf.st_mode & S_IWUSR`: 소유자의 쓰기 권한이 존재하는가?
  - `sbuf.st_mode & S_IXUSR`: 소유자의 실행 권한이 존재하는가?

# 3. `sscanf`, `sprintf` 함수

- 사용자 버퍼에 입출력을 하는 함수 (`char buf[MAXLINE]`)
  - 사용자 버퍼라는 거창한 말을 쓰는데.. 그냥 문자로 구성된 배열임.
- 사용자 버퍼는, 다른 호스트에서 수신받은 데이터가 임시 저장되는 내부 버퍼와 다른 버퍼임에 유의할 것
  - 사용자 버퍼는 주로 프로그램 내부에서 사용자 데이터 처리, 출력용 데이터 생성 등 용도로 사용
  - 일반적으로 `char buf[MAXLINE]`으로 정의. `MAXLINE`은 한 줄 최대 문자 수로, `8192`로 정의되어있음.

## `sprintf`
- `printf`가 변수의 값을 **화면에 출력(`stdout`)**했다면,
- `sprintf`는 변수의 값을 **사용자 버퍼에 출력**한다.
- 이때 이전에 버퍼에 존재한 값은 덮어씌우기된다는 점에 유의한다.
- cf. `\r\n`은 네트워크 통신에서 줄바꿈을 명확히 하기 위해 사용한다. HTTP에서 이거 쓰라고 규정함.

```c
// 사용자버퍼 body에 아래 문자열을 순차적으로 추가
// 네트워크 통신에서 줄바꿈을 명확히 하기 위해서 \r\n 사용.

char buf[MAXLINE];
sprintf(body, "<html><title>Tiny Error</title>");
sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
```
- 사용자버퍼 `body`에 문자열을 누적하는 방식으로 `sprintf` 사용
- `sprintf`는 매번 `body` 전체를 덮어쓰되, 기존 내용을 첫 번째 인자로 포함시킴

## `sscanf`

- `scanf`가 **키보드 입력(`stdin`)한 값**을, 형식에 맞게 인자로 받은 주소에 저장했다면
- `sscanf`은 **사용자 버퍼의 값**을, 형식에 맞게 인자로 받은 주소에 저장한다.

```c
char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

// buf에 문자열 "GET / HTTP/1.1"이 저장되었다고 가정
sscanf(buf, "%s %s %s", method, uri, version);

// 배열 method엔 "GET"이, uri엔 "/"이, version엔 "HTTP/1.1"이 저장됨
```

# 4. `strcmp`, `strcasecmp` 함수
- `strcmp`는 두 문자열을 비교
  - 대소문자가 다르면 다른 문자열 취급
- `strcasecmp`는 
  - **대소문자를 구분하지 않음**. `GeT`, `geT`는 동일 문자열 취급.
- 앞 문자열이 사전 순서상 앞에 있으면 `-1` 반환
- 뒷 문자열이 사전 순서상 앞에 있으면 `1` 반환
- 두 문자열이 동일하면 `0`을 반환
```c
if (strcasecmp(method, "GET")){
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
}
```
- `method`에 저장된 문자열이 `get`, `GET`, `gEt`, `GeT`, .... 등이 아니라면, 조건문이 참이라서 아래 에러 함수가 호출됨.

# 5. `strstr` 함수
- 문자열 내부의 부분 문자열을 찾을 때 사용
- 찾으면 처음 발견된 위치를 가리키는 포인터 반환
- 찾지 못하면 `NULL` 반환

```c
void get_filetype(char *filename, char *filetype){
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
```
- `filename`이 가리키는 문자열에 해당 확장자가 포함되면, `filetype`이 가리키는 메모리에 이에 대응되는 문자열 복사

# 6. `index` 함수
- 문자열 내부의 문자를 찾을 때 사용
- 찾으면 처음 발견된 위치를 가리키는 포인터 반환
- 찾지 못하면 `NULL` 반환

```c
// uri는 char*형 포인터로 문자열을 가리킴
ptr = index(uri, '?');
if (ptr){
    strcpy(cgiargs, ptr+1);   // cgi 인자들을 추출해 cgiargs에 저장
    *ptr = '\0';              // '?'가 있었던 바이트를 문자열의 끝을 뜻하는 널 문자로 덮어씌움
} else {
    strcpy(cgiargs, "");
}
```
- `uri`가 가리키는 문자열에 `'?'`가 포함되면 `if`문 다음 중괄호의 코드가, 포함되지 않으면 `else`문 다음 중괄호의 코드가 실행됨
- `ptr`엔 `?`가 처음 나오는 메모리 주소가 저장되는데, `if`문에선 `ptr`이 가리키는 `?` 문자를 `\0`(널 문자)로 덮어씀
  - 널 문자는 문자열의 끝을 의미. 즉 원래 `uri` 중 `?` 및 `?` 이후 문자들을 모두 날렸다고 생각하면 됨.

# 7. `setenv`: 환경 변수 지정 / `getenv`: 환경 변수 읽음
- 환경 변수: 운영체제에서 프로세스의 동작에 영향을 주는 문자열 변수
- CGI 프로그램은 값을 표준 입력(stdin)이 아닌, **환경 변수**로부터 읽음
- 따라서 `setenv`로 환경 변수 설정하고, `exceve`로 프로그램 실행하고, `getenv`로 환경 변수 읽는 방식
  - 이때 `exceve`에 `environ`을 보냄. 전역 변수로, 운영체제가 프로그램 실행 시에 만들어 주는 환경 변수 리스트임. 이걸 보내야 환경 변수가 유지됨에 유의할 것.


```c
// 서버 코드
// QUERY_STRING (CGI 환경변수) 를 cgiargs로 설정.
// 1: 기존 환경 변수가 있어도 덮어쓴다는 뜻.

// 자녀 프로세스 생성. 자녀는 fork호출 직후부터 실행됨.
if (Fork() == 0){
    setenv("QUERY_STRING", cgiargs, 1);

    // 자식의 표준 출력을, 연결 소켓 식별자로 재지정
    Dup2(fd, STDOUT_FILENO);

    // 운영체제가 설정한 전역 변수인, 환경변수리스트 environ을 보냄
    // filename을 실행
    Execve(filename, emptylist, environ);
}
```

```c
// CGI 프로그램 내 이런 코드가 있을 것임...
buf = getenv("QUERY_STRING")
```

# 8. `dup2`: 입출력 재지정
- `dup2`를 통해 표준 입력 및 출력을 디스크 파일과 연결할 수 있음.
- 표준 입력을 파일과 연결하면, 이후의 모든 `stdin` 입력을 키보드 대신 파일에서 읽어들임. (`scanf()`, `fgets(stdin)` 등)
  - `Dup2(파일 명시자, STDIN_FILENO)`
- 표준 출력을 파일과 연결하면, 이후의 모든 `stdout` 출력을 모두 디스크 파일에 저장한다. (`printf()`, `fputs(stdout)` 등)
  - `Dup2(파일 명시자, STDOUT_FILENO)`


```c
// 자녀 프로세스 생성. 자녀는 fork호출 직후부터 실행됨.
if (Fork() == 0){
    setenv("QUERY_STRING", cgiargs, 1);

    // 자식의 표준 출력을, 연결 소켓 식별자로 재지정
    Dup2(fd, STDOUT_FILENO);

    // 운영체제가 설정한 전역 변수인, 환경변수리스트 environ을 보냄
    // filename을 실행
    Execve(filename, emptylist, environ);
}
```
- 웹서버 내에서 동적 콘텐츠 제공을 위해 CGI 프로그램을 실행시킬 때, 멋대로 프로그램을 바꿔서 출력되는 내용을 소켓에 입력할 수는 없는 법임.
- 따라서 자식 프로세스를 만들고, `dup2`를 이용해 프로세스의 출력을 소켓과 연결하고, `execve`로 자식 프로세스를 실행하게끔 함.