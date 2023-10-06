#include <stdlib.h>
#include <stdio.h>

int main() {
  char str[200];
  fgets(str, 200, stdin);
  printf("str: %s\n", str);
  return 0;
}