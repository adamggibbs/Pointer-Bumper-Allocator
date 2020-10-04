#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv){

  char* x = malloc(24);
  char* y = malloc(19);
  char* z = malloc(32);

  // due the follow to test realloc():
  // 1)  reallocate block x to a smaller block, address should not change
  // 2a) reallocate block y to a larger block, address should change
  // 2b) black char 'a' at y and char 'b' at y+1
  //     print these before and after realloc() call, they should copy over
  // 3)  reallocate block z to a block of the same size, addree should not change

  printf("%s\n", "Test 1 - address should not change");
  printf("x = %p\n", x);
  x = realloc(x, 20);
  printf("x = %p\n", x);
  printf("\n");

  printf("%s\n", "Test 2 - address should change and contents should copy over");
  *y = 'a';
  char* y2  = y + 1;
  *y2 = 'b';
  printf("y = %p\n", y);
  printf("%d\n", *y);
  printf("%d\n", *y2);
  y = realloc(y, 33);
  printf("y = %p\n", y);
  printf("%d\n", *y);
  printf("%d\n", *y2);
  printf("\n");
  
  printf("%s\n", "Test 3 - address should not change");
  printf("z = %p\n", z);
  z = realloc(z, 32);
  printf("z = %p\n", z);

}
