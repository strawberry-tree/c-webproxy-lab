#include <stdio.h>
#include <string.h>

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
    strcpy(suffix, ".");
    strcpy(suffix, suffix_ptr);
    *suffix_ptr = '\0';
  }


  // 포트 번호가 존재하는 요청이면 포트 번호를 파싱.
  if ((port_ptr = index(start_ptr, ':'))){
    strcpy(portNo, port_ptr + 1);
    *port_ptr = '\0';
  }

  // prefix를 파싱.
  strcpy(prefix, start_ptr);
}

int main(void){
    char url[1000] = "http://www.cmu.edu:8000/hub/index.html";
    char prefix[1000], portNo[1000], suffix[1000];

    parse_url(url, prefix, portNo, suffix);
    printf("%s\n", prefix);
    printf("%s\n", portNo);
    printf("%s\n", suffix);
}

