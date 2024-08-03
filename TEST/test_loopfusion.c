#include <stdio.h>

void foo(int a[5], int b[5], int c[5], int d[5]) {
  const int N = 5;
  for (int i = 0; i < N; i++)
    a[i] = b[i] * c[i];

  for (int i = 0; i < N; i++)
    d[i] = a[i] + c[i];

  for (int i = 0; i < N; i++)
    a[i] = d[i] / 2;
}

int main() {
  int a[5] = {10, 20, 30, 40, 50};
  int b[5] = {11, 21, 31, 41, 51};
  int c[5] = {12, 22, 32, 42, 52};
  int d[5] = {13, 23, 33, 43, 53};
  foo(a, b, c, d);
  printf("result: %d,%d,%d,%d,%d", a[0], a[1], a[2], a[3], a[4]);
}