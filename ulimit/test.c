#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main()
{
  
  int start,end;
  start = sbrk(0);
  char *p = (char *)malloc(32*1024*1024);
  memset(p, 4, sizeof(p));
  printf("%d\n", *p);
  end = sbrk(0);
  printf("hello I used %d vmemory\n",end - start);
  for(int i = 0; i < 10; ++i) {
    char *p = (char *)malloc(32*1024*1024);
    memset(p, i, sizeof(p));
    printf("%d\n", *p);
    sleep(5);
  }
  while(1) {
    memset(p, 9, sizeof(p));
  }
  return 0;
}
