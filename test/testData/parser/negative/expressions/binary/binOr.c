int i1, i2;
float f1, f2;
int *pi1, pi2;
float *pf1, *pf2;

int arr1[20], arr2[20];

struct S1 {};
struct S2 {};

struct S1 s11, s12, *ps11, *ps12;
struct S2 s21, s22, *ps21, *ps22;

void testOr() {
  i1 | i2;
  i1 | f1;
  i1 | f2;
  i1 | pi1;
  i1 | pi2;
  i1 | pf1;
  i1 | pf2;
  i1 | arr1;
  i1 | arr2;
  i1 | s11;
  i1 | s12;
  i1 | ps11;
  i1 | ps12;
  i1 | s21;
  i1 | s22;
  i1 | ps21;
  i1 | ps22;
  i2 | i1;
  i2 | f1;
  i2 | f2;
  i2 | pi1;
  i2 | pi2;
  i2 | pf1;
  i2 | pf2;
  i2 | arr1;
  i2 | arr2;
  i2 | s11;
  i2 | s12;
  i2 | ps11;
  i2 | ps12;
  i2 | s21;
  i2 | s22;
  i2 | ps21;
  i2 | ps22;
  f1 | i1;
  f1 | i2;
  f1 | f2;
  f1 | pi1;
  f1 | pi2;
  f1 | pf1;
  f1 | pf2;
  f1 | arr1;
  f1 | arr2;
  f1 | s11;
  f1 | s12;
  f1 | ps11;
  f1 | ps12;
  f1 | s21;
  f1 | s22;
  f1 | ps21;
  f1 | ps22;
  f2 | i1;
  f2 | i2;
  f2 | f1;
  f2 | pi1;
  f2 | pi2;
  f2 | pf1;
  f2 | pf2;
  f2 | arr1;
  f2 | arr2;
  f2 | s11;
  f2 | s12;
  f2 | ps11;
  f2 | ps12;
  f2 | s21;
  f2 | s22;
  f2 | ps21;
  f2 | ps22;
  pi1 | i1;
  pi1 | i2;
  pi1 | f1;
  pi1 | f2;
  pi1 | pi2;
  pi1 | pf1;
  pi1 | pf2;
  pi1 | arr1;
  pi1 | arr2;
  pi1 | s11;
  pi1 | s12;
  pi1 | ps11;
  pi1 | ps12;
  pi1 | s21;
  pi1 | s22;
  pi1 | ps21;
  pi1 | ps22;
  pi2 | i1;
  pi2 | i2;
  pi2 | f1;
  pi2 | f2;
  pi2 | pi1;
  pi2 | pf1;
  pi2 | pf2;
  pi2 | arr1;
  pi2 | arr2;
  pi2 | s11;
  pi2 | s12;
  pi2 | ps11;
  pi2 | ps12;
  pi2 | s21;
  pi2 | s22;
  pi2 | ps21;
  pi2 | ps22;
  pf1 | i1;
  pf1 | i2;
  pf1 | f1;
  pf1 | f2;
  pf1 | pi1;
  pf1 | pi2;
  pf1 | pf2;
  pf1 | arr1;
  pf1 | arr2;
  pf1 | s11;
  pf1 | s12;
  pf1 | ps11;
  pf1 | ps12;
  pf1 | s21;
  pf1 | s22;
  pf1 | ps21;
  pf1 | ps22;
  pf2 | i1;
  pf2 | i2;
  pf2 | f1;
  pf2 | f2;
  pf2 | pi1;
  pf2 | pi2;
  pf2 | pf1;
  pf2 | arr1;
  pf2 | arr2;
  pf2 | s11;
  pf2 | s12;
  pf2 | ps11;
  pf2 | ps12;
  pf2 | s21;
  pf2 | s22;
  pf2 | ps21;
  pf2 | ps22;
  arr1 | i1;
  arr1 | i2;
  arr1 | f1;
  arr1 | f2;
  arr1 | pi1;
  arr1 | pi2;
  arr1 | pf1;
  arr1 | pf2;
  arr1 | arr2;
  arr1 | s11;
  arr1 | s12;
  arr1 | ps11;
  arr1 | ps12;
  arr1 | s21;
  arr1 | s22;
  arr1 | ps21;
  arr1 | ps22;
  arr2 | i1;
  arr2 | i2;
  arr2 | f1;
  arr2 | f2;
  arr2 | pi1;
  arr2 | pi2;
  arr2 | pf1;
  arr2 | pf2;
  arr2 | arr1;
  arr2 | s11;
  arr2 | s12;
  arr2 | ps11;
  arr2 | ps12;
  arr2 | s21;
  arr2 | s22;
  arr2 | ps21;
  arr2 | ps22;
  s11 | i1;
  s11 | i2;
  s11 | f1;
  s11 | f2;
  s11 | pi1;
  s11 | pi2;
  s11 | pf1;
  s11 | pf2;
  s11 | arr1;
  s11 | arr2;
  s11 | s12;
  s11 | ps11;
  s11 | ps12;
  s11 | s21;
  s11 | s22;
  s11 | ps21;
  s11 | ps22;
  s12 | i1;
  s12 | i2;
  s12 | f1;
  s12 | f2;
  s12 | pi1;
  s12 | pi2;
  s12 | pf1;
  s12 | pf2;
  s12 | arr1;
  s12 | arr2;
  s12 | s11;
  s12 | ps11;
  s12 | ps12;
  s12 | s21;
  s12 | s22;
  s12 | ps21;
  s12 | ps22;
  ps11 | i1;
  ps11 | i2;
  ps11 | f1;
  ps11 | f2;
  ps11 | pi1;
  ps11 | pi2;
  ps11 | pf1;
  ps11 | pf2;
  ps11 | arr1;
  ps11 | arr2;
  ps11 | s11;
  ps11 | s12;
  ps11 | ps12;
  ps11 | s21;
  ps11 | s22;
  ps11 | ps21;
  ps11 | ps22;
  ps12 | i1;
  ps12 | i2;
  ps12 | f1;
  ps12 | f2;
  ps12 | pi1;
  ps12 | pi2;
  ps12 | pf1;
  ps12 | pf2;
  ps12 | arr1;
  ps12 | arr2;
  ps12 | s11;
  ps12 | s12;
  ps12 | ps11;
  ps12 | s21;
  ps12 | s22;
  ps12 | ps21;
  ps12 | ps22;
  s21 | i1;
  s21 | i2;
  s21 | f1;
  s21 | f2;
  s21 | pi1;
  s21 | pi2;
  s21 | pf1;
  s21 | pf2;
  s21 | arr1;
  s21 | arr2;
  s21 | s11;
  s21 | s12;
  s21 | ps11;
  s21 | ps12;
  s21 | s22;
  s21 | ps21;
  s21 | ps22;
  s22 | i1;
  s22 | i2;
  s22 | f1;
  s22 | f2;
  s22 | pi1;
  s22 | pi2;
  s22 | pf1;
  s22 | pf2;
  s22 | arr1;
  s22 | arr2;
  s22 | s11;
  s22 | s12;
  s22 | ps11;
  s22 | ps12;
  s22 | s21;
  s22 | ps21;
  s22 | ps22;
  ps21 | i1;
  ps21 | i2;
  ps21 | f1;
  ps21 | f2;
  ps21 | pi1;
  ps21 | pi2;
  ps21 | pf1;
  ps21 | pf2;
  ps21 | arr1;
  ps21 | arr2;
  ps21 | s11;
  ps21 | s12;
  ps21 | ps11;
  ps21 | ps12;
  ps21 | s21;
  ps21 | s22;
  ps21 | ps22;
  ps22 | i1;
  ps22 | i2;
  ps22 | f1;
  ps22 | f2;
  ps22 | pi1;
  ps22 | pi2;
  ps22 | pf1;
  ps22 | pf2;
  ps22 | arr1;
  ps22 | arr2;
  ps22 | s11;
  ps22 | s12;
  ps22 | ps11;
  ps22 | ps12;
  ps22 | s21;
  ps22 | s22;
  ps22 | ps21;
}

