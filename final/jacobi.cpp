#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>

#define INDEX(N, R, C) (R * N) + C
#define MAX_VAL 1.0
#define TOL 0.000000001

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
  std::cout << "\t";
  for (int i = 0; i < size; ++i) std::cout << a[i] << " ";
  std::cout << std::endl;
}

void print2D(float* a, int size) {
  for (int i = 0; i < size; ++i) {
    std::cout << "\t";
    for (int j = 0; j < size; ++j) std::cout << a[INDEX(size, i, j)] << " ";
    std::cout << std::endl;
  }
}

int main(int argc, char const* argv[]) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <matrix_size> <max_iterations>"
              << std::endl;
    exit(-1);
  }

  srand(0);
  std::cout << std::setprecision(4);

  int size = atoi(argv[1]);
  int max_iterations = atoi(argv[2]);

  float* A = new float[size * size];
  float* x = new float[size];
  float* y = new float[size];
  float* b = new float[size];

  for (int i = 0; i < size; ++i) {
    b[i] = frand();
    x[i] = y[i] = 0.0;
  }

  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j) A[INDEX(size, i, j)] = frand();

  std::cout << "A" << std::endl;
  print2D(A, size);
  std::cout << std::endl;

  std::cout << "b" << std::endl;
  print1D(b, size);
  std::cout << std::endl;

  float err = 0;
  int counter = 0;
  for(; counter < max_iterations; ++counter) {

    for (int i = 0; i < size; ++i) {
      float val = 0.0;
      for (int j = 0; j < size; ++j)
        if (j != i) val += A[INDEX(size, i, j)] * y[j];
      x[i] = (-val + b[i]) / A[INDEX(size, i, i)];
    }

    err = 0;
    for (int i = 0; i < size; ++i) {
      err += (x[i] - y[i]) * (x[i] - y[i]);
      y[i] = x[i];
    }
    err = sqrt(err);
    std::cout << "iteration: " << (counter + 1) << " error: " << err << std::endl;
    print1D(x, size);
    std::cout << std::endl;

    if (err <= TOL) break;
  }

  std::cout << "final value:: iteration: " << counter << " error: " << err << std::endl;
  print1D(y, size);
  std::cout << std::endl;

  delete x;
  delete y;
  delete b;
  delete A;

  return 0;
}
