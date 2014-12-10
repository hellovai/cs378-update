#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <omp.h>

#define INDEX(N, C, R) (R * N) + C
#define MAX_VAL 1.0
#define TOL 1.0E-24
#define ALIGN 16

float frand(float m = MAX_VAL, bool sign = false) {
  return sign ? (((float)rand()) / ((float)(RAND_MAX / (2 * m))) - m)
              : (((float)rand()) / ((float)(RAND_MAX / (m))));
}

void multiply(float* A, float* b, float* x, int size) {
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++) x[i] += b[i] * A[INDEX(size, j, i)];
}

float distance(float* x, float* y, int size) {
  float sum = 0.0;
  for (int index = 0; index < size; index++)
    sum += (x[index] - y[index]) * (x[index] - y[index]);
  return sum;
}

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

// vectorize
float getNewSolution(const float* A, const float* y, const int i, const float b,
                     const int size) {
  float val = 0.0;
  for (int j = 0; j < size; ++j)
    if (j != i) val += A[INDEX(size, i, j)] * y[j];
  return (-val + b) / A[INDEX(size, i, i)];
}

void init(float* A, float* b, float* x, float* y, int size, float density) {
  for (int i = 0; i < size; ++i) {
    b[i] = frand(1.0, true);
    x[i] = y[i] = 0.0;
  }

  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j)
      A[INDEX(size, i, j)] = (frand() < density) ? frand(1.0, true) : 0.0;

  float sum = 0;

  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j) sum += abs(A[INDEX(size, i, j)]);

  // to make matrix diagonally dominate
  for (int i = 0; i < size; ++i)
    A[INDEX(size, i, i)] = frand(1.0, false) + size;
}

int main(int argc, char const* argv[]) {
  if (argc != 5) {
    std::cout << "Usage: " << argv[0]
              << " <matrix_size / 4> <max_iterations> <density> <num_threads>"
              << std::endl;
    exit(-1);
  }

  srand(0);
  std::cout << std::setprecision(4);

  int size = atoi(argv[1]) * 4;
  int max_iterations = atoi(argv[2]);
  float density = (float)atof(argv[3]);
  int num_threads = atoi(argv[4]);
  omp_set_num_threads(num_threads);

  char * A_x = new char[sizeof(float) * size * size + ALIGN];
  char * x_x = new char[sizeof(float) * size + ALIGN];
  char * y_x = new char[sizeof(float) * size + ALIGN];
  char * b_x = new char[sizeof(float) * size + ALIGN];

  float* A = (float*)(((uintptr_t) A_x + ALIGN) & (~(uintptr_t)0x0F));
  float* x = (float*)(((uintptr_t) x_x + ALIGN) & (~(uintptr_t)0x0F));
  float* y = (float*)(((uintptr_t) y_x + ALIGN) & (~(uintptr_t)0x0F));
  float* b = (float*)(((uintptr_t) b_x + ALIGN) & (~(uintptr_t)0x0F));

  init(A, b, x, y, size, density);

  int k = 0, icol;
  do {
    for (int i = 0; i < size; ++i) {
      y[i] = x[i];
    }

#pragma omp parallel for private(icol) shared(x, y, b)
    for (icol = 0; icol < size; ++icol) {
      x[icol] = getNewSolution(A, y, icol, b[icol], size);
    }

  } while (k < max_iterations && distance(y, x, size) >= TOL);

  float err = 0.0;
  multiply(A, y, x, size);
  for (int i = 0; i < size; ++i) {
    err += (x[i] - b[i]) * (x[i] - b[i]);
  }
  err = sqrt(err) / size;
  std::cout << "Error: " << err << std::endl;

  print2D(A, size);

  print1D(b, size);

  print1D(x, size);

  delete x_x;
  delete y_x;
  delete b_x;
  delete A_x;

  return 0;
}
