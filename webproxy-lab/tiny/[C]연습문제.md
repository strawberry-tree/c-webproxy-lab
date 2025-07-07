# 11.6c
- Tiny의 출력을 조사해서 브라우저의 HTTP 버전을 결정하라.

```c
/* doit 함수 내부 코드 */
// rio 함수 사용 위한 버퍼 초기화
Rio_readinitb(&rio, fd);
// 입력 한 줄 읽고, buf에 저장.
if (Rio_readlineb(&rio, buf, MAXLINE) <= 0){
    return;
}
printf("Request headers:\n");
printf("%s", buf);
```

```bash
Request headers:
GET /home.html HTTP/1.1
Connection: keep-alive
# 이하 생략
```
- 뭐 바꿔줄 필요도 없음. 요청 헤더에 자동으로 포함됨.

# 11.7
- Tiny를 확장해서 MPG 비디오 파일을 처리하도록 하시오. 실제 브라우저를 사용해서 여러분의 결과를 체크하시오.
  - HTML5는 MPG를 지원 안 해서, 부득이하게 MP4로 처리

```c
else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
```
- `get_filetype`에 `video/mp4`에 대응되는 Content-type 추가 (이걸 꼭 해 줘야 하는진 모르겠음)

```HTML
<html>
<head><title>test</title></head>
<body> 
<img align="middle" src="godzilla.gif">
<video width="640" height="360" controls>
<source src="sample.mp4" type="video/mp4">
Your browser does not support the video tag.
</video>
Dave O'Hallaron
</body>
</html>
```
- 이후 `home.html`에 `video` 태그로 열어주면, 잘 처리됨

![alt text](image-2.png)

# 11.9
- Tiny를 수정해서 정적 콘텐츠를 처리할 때 요청한 파일을 `mmap`, `rio_readn` 대신에 `malloc`, `rio_readn`, `rio_writen`을 사용해서 연결 식별자에게 복사하도록 하시오.

```c
srcp = Malloc(filesize);          // filesize를 저장할 수 있는 공간 확보
Rio_readn(srcfd, srcp, filesize); // srcfd의 데이터를 메모리 scrp에 filesize만큼 복사
Close(srcfd);                     // 매핑 완료됐으니 파일 닫기. 메모리 누수 막기 위함.
Rio_writen(fd, srcp, filesize);   // 클라이언트에 파일 전송
Free(srcp);                       // 메모리 할당해제
```
- `mmap`의 역할은 메모리 공간을 할당하고, 파일을 매핑하는 것 
- 메모리 공간 할당은 `malloc`이 대신 해 줌
- 해당 메모리 주소에 파일의 데이터를 복사하기 위해, `rio_readn`을 사용
- 이후 과정은 `munmap` 대신 `free`가 사용됐다는 점 제외하고 동일함

# 11.10
- CGI adder 함수에 대한 HTML form을 작성하시오. form은 사용자가 함께 더할 두 개의 숫자로 채우는 두 개의 text box를 포함해야 하며, GET 메소드를 사용해서 컨텐츠를 요청해야 한다.
- 실제 브라우저를 이용해 Tiny로부터 form을 요청하고, 채운 form을 Tiny에 보내고, adder가 생성한 동적 콘텐츠를 표시하는 방법으로 작업을 체크하라.

```html
<form method="get" action="cgi-bin/adder">
    <label>Number 1</label><input type="number" name="num1">
    <label>Number 2</label><input type="number" name="num2">
    <button>Calculate</button>
</form>
```
- `home.html`에 위와 같은 form 추가
  - `method`를 `get`으로, `action`을 `adder` 파일의 경로로 설정하는 것 잊지 말 것
  - `input`의 `name`은 뭘로 설정하든 상관없음. 하지만 `name`이 없으면 쿼리스트링에 포함되지 않으니 뭐라도 설정할 것.

![alt text](image.png)
![alt text](image-1.png)

# 11.11
- Tiny를 확장하여 HTTP HEAD 메소드를 지원하도록 하라. Telnet을 웹 클라이언트로 사용해서 작업 결과를 체크하시오.

```c
// doit 함수
int getflag;
if (!strcasecmp(method, "GET")){
    getflag = 1;
} else if (!strcasecmp(method, "HEAD")){
    getflag = 0;
} else {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return; // main 루틴으로 돌아감.
}

// 생략
serve_static(fd, filename, sbuf.st_size, getflag);

// 생략
serve_dynamic(fd, filename, cgiargs, getflag);
```
- `doit` 함수 내 `getflag` 변수를 선언
- 메서드가 `GET`이면 `1`, `HEAD`면 `0`으로 설정
- 이후 `getflag`를 `serve_static`, `serve_dynamic`에 매개변수로 추가 전송

```c
void serve_static(int fd, char *filename, int filesize, int getflag){
// 생략
  // 응답 헤더를 보냄
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  // HEAD 요청인 경우 응답 본체를 보내지 않음
  if (!getflag) return;

  // 응답 본체를 보내는 코드가 이어짐
}
```
- `serve_static`: `getflag`가 `0`이면 응답 본체를 보내기 전에 `return`으로 함수 종료.

```c
void serve_dynamic(int fd, char *filename, char *cgiargs, int getflag){
  char buf[MAXLINE], *arglist[] = {filename, getflag ? "1" : "0", NULL};

  // 생략
  if (Fork() == 0){
    // 생략
    Execve(filename, arglist, environ);
  }
  // 생략
}
```
- `serve_dynamic`: `getflag`의 값을 `arglist`에 저장.

```c
int main(int argc, char** argv)
{
  // 생략
  if (argc > 1){
    getflag = atoi(argv[1]);
  }

  // 생략

  if(getflag){
    printf("%s", content);
  }

  // 생략
}
```
- `cgi-bin/adder.c`: 함수에 `argc`, `argv` 설정
- 이후 `arglist`의 값은 `argv`에 들어감
  - 우리가 입력한 `getflag`는 `argv[1]`에 위치
- 이후 `getflag`이 `1`일 때만 `content`를 출력

## 실행결과
```bash
> telnet localhost 8000
Trying ::1...
Connection failed: Connection refused
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
> HEAD / HTTP/1.0

HTTP/1.0 200 OK
Filename: ./home.html
Server: Tiny Web Server
Connection: close
Content-length: 481
Content-type: text/html
```

```bash
> telnet localhost 8000
Trying ::1...
Connection failed: Connection refused
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
>HEAD /sample.mp4 HTTP/1.0

HTTP/1.0 200 OK
Filename: ./sample.mp4
Server: Tiny Web Server
Connection: close
Content-length: 6217126
Content-type: video/mp4

Connection closed by foreign host.
```

```bash
> telnet localhost 8000
Trying ::1...
Connection failed: Connection refused
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
>  HEAD /cgi-bin/adder?num1=15&num2=30 HTTP/1.0

HTTP/1.0 200 OK
Server: Tiny Web Server
Connection ADDER: close
Content-type: text/html
Content-length: 133

Connection closed by foreign host.
```