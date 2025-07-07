/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(int argc, char** argv)
{
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  int getflag = 0;

  // atoi: 문자열을 정수로. ascii to integer.
  if (argc > 1){
    getflag = atoi(argv[1]);
  }
  


  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    p = strchr(buf, '&');
    *p = '\0';
    strcpy(arg1, buf);
    strcpy(arg2, p + 1);
    n1 = atoi(strchr(arg1, '=') + 1);
    n2 = atoi(strchr(arg2, '=') + 1);
  }

  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s\r\n<p>", buf);
  sprintf(content + strlen(content), "Welcome to add.com: ");
  sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>");
  sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>",
          n1, n2, n1 + n2);
  sprintf(content + strlen(content), "Thanks for visiting!\r\n");


  /* Generate the HTTP response */
  printf("Connection ADDER: close\r\n");
  printf("Content-type: text/html\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("\r\n");
  if(getflag){
    printf("%s", content);
  }
  fflush(stdout);

  exit(0);
}
/* $end adder */
