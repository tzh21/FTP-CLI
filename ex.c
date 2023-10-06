#include <stdlib.h>
#include <stdio.h>

int main() {
  char str[100] = "abc\nefg";
  int len = strlen(str);
  printf("%d\n", len);
  return 0;
}