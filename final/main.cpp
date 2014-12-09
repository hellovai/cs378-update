#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>

#define INDEX(N, R, C) (R * N) + C
#define MAX_VAL 1.0
#define TOL 0.000000001
#define MAX_IT 10

float frand(float m = MAX_VAL) {
  return ((float)rand()) / ((float)(RAND_MAX / m));
}

struct Matrix {
  float* _a;
  const int N;
  Matrix* D;
  Matrix* L;
  Matrix* U;
  Matrix* DI;
  Matrix* LU;

  Matrix(int n)
      : N(n), D(nullptr), L(nullptr), U(nullptr), DI(nullptr), LU(nullptr) {
    _a = new float[n * n];
    for (int i = 0; i < N * N; i++) _a[i] = 0.0;
  }

  ~Matrix() {
    delete _a;
    if (D) delete D;
    if (L) delete L;
    if (U) delete U;
    if (DI) delete DI;
    if (LU) delete LU;
  }

  void fill(const float p) {
    for (int i = 0; i < N; i++)
      for (int j = 0; j < N; j++)
        if (frand(1.0) <= p) _a[INDEX(N, i, j)] = frand();
  }

  std::ostream& print(std::ostream& os) {
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < N; j++) os << _a[INDEX(N, i, j)] << " ";
      os << std::endl;
    }
    os << std::endl;
    return os;
  }

  Matrix* getD() {
    if (!D) {
      D = new Matrix(N);
      for (int i = 0; i < N; i++) D->_a[INDEX(N, i, i)] = _a[INDEX(N, i, i)];
    }
    return D;
  }

  Matrix* getDI() {
    if (!DI) {
      DI = new Matrix(N);
      for (int i = 0; i < N; i++)
        DI->_a[INDEX(N, i, i)] = 1.0 / _a[INDEX(N, i, i)];
    }
    return DI;
  }

  Matrix* getL() {
    if (!L) {
      L = new Matrix(N);
      for (int i = 0; i < N; i++)
        for (int j = 0; j < i; j++) L->_a[INDEX(N, i, j)] = _a[INDEX(N, i, j)];
    }
    return L;
  }

  Matrix* getU() {
    if (!U) {
      U = new Matrix(N);
      for (int i = 0; i < N; i++)
        for (int j = 0; j < i; j++) U->_a[INDEX(N, j, i)] = _a[INDEX(N, j, i)];
    }
    return U;
  }

  Matrix* getLU() {
    if (!LU) {
      LU = new Matrix(N);
      memcpy(LU->_a, _a, sizeof(_a[0]) * N * N);
      for (int i = 0; i < N; i++) LU->_a[INDEX(N, i, i)] = 0.0;
    }
    return LU;
  }

  void multiply(float* b, float* x) {
    for (int i = 0; i < N; i++)
      for (int j = 0; j < N; j++) x[i] += b[i] * _a[INDEX(N, j, i)];
  }
};

void print1D(float* a, int size) {
  for (int i = 0; i < size; ++i) std::cout << a[i] << " ";
  std::cout << std::endl;
}

void print2D(float* a, int size) {
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) std::cout << a[INDEX(size, i, j)] << " ";
    std::cout << std::endl;
  }
}

int main(int argc, char const* argv[]) {
  srand(0);
  std::cout << std::setprecision(4);

  int size = atoi(argv[1]);

  float* a = new float[size * size];
  float* x = new float[size];
  float* xt = new float[size];
  float* b = new float[size];

  for (int i = 0; i < size; ++i) {
    b[i] = frand();
    x[i] = xt[i] = 0.0;
  }

  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j) a[INDEX(size, i, j)] = frand();

  std::cout << "A" << std::endl;
  print2D(a, size);
  std::cout << std::endl;

  std::cout << "B" << std::endl;
  print1D(b, size);
  std::cout << std::endl;

  int counter = 0;
  while (counter++ < MAX_IT) {

    for (int i = 0; i < size; ++i) {
      float val = 0.0;
      for (int j = 0; j < size; ++j)
        if (j != i) val += a[INDEX(size, i, j)] * xt[j];
      x[i] = (-val + b[i]) / a[INDEX(size, i, i)];
    }

    std::cout << "x" << std::endl;
    print1D(x, size);

    float err = 0;
    for (int i = 0; i < size; ++i) {
      err += (x[i] - xt[i]) * (x[i] - xt[i]);
      xt[i] = x[i];
    }
    err = sqrt(err);
    std::cout << counter << " " << err << std::endl << std::endl;

    if (err <= TOL) break;
  }

  std::cout << "x" << std::endl;
  print1D(xt, size);
  std::cout << std::endl;

  delete x;
  delete xt;
  delete b;
  delete a;

  return 0;
}
