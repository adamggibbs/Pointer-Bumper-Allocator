#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv){

  char* x = malloc(24);
  char* y = malloc(19);
  char* z = malloc(32);
  
  printf("x = %p\n", x);
  printf("y = %p\n", y);
  printf("z = %p\n", z);

}
