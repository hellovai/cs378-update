#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <x86intrin.h>
#include <omp.h>
#include <sys/time.h>
#include <time.h>

#define INDEX(N, C, R) (R * N) + C
#define MAX_VAL 1.0
#define TOL 1.0E-24
#define ALIGN 16

double deltaTime(struct timeval t1, struct timeval t2) {
  struct timeval ret;
  timersub(&t2, &t1, &ret);
  return (double)ret.tv_sec + (double)ret.tv_usec / 1000000.0;
}

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
  for (int i = 0; i < size; i++) sum += (x[i] - y[i]) * (x[i] - y[i]);
  return sum;
}

void print1D(float* a, int size) {
  std::cout << "\t";
  for (int i = 0; i < size; ++i) std::cout << a[i] << "\t";
  std::cout << std::endl;
}

void print2D(float* a, int size) {
  for (int i = 0; i < size; ++i) {
    std::cout << "\t";
    for (int j = 0; j < size; ++j) std::cout << a[INDEX(size, i, j)] << "\t";
    std::cout << std::endl;
  }
}

float scalarProduct(float* A, float* y, const int i, const float b,
                    const int size) {
  float val = 0.0;
  for (int j = 0; j < size; ++j)
    if (j != i) val += A[INDEX(size, i, j)] * y[j];
  return (-val + b) / A[INDEX(size, i, i)];
}

float vectorizedProduct(float* A, float* y, const int i, const float b,
                        const int size) {
  __m128 rA, rB, rC;

  int j = 0;
  float val = 0.0;
  float acc[4];

  for (j = 0; j < size; j += 4) {
    // rB = _mm_load_ps(&A[INDEX(size, i, j)]);
    rB = _mm_set_ps(A[INDEX(size, i, j)], A[INDEX(size, i, j + 1)],
                    A[INDEX(size, i, j + 2)], A[INDEX(size, i, j + 3)]);
    rC = _mm_load_ps(&y[j]);
    rA = _mm_add_ps(rA, _mm_mul_ps(rB, rC));
  }

  _mm_store_ps(acc, rA);
  val += (acc[0] + acc[1] + acc[2] + acc[3]);

  val -= (A[INDEX(size, i, i)] * y[i]);
  return (-val + b) / A[INDEX(size, i, i)];
}

void driver(float* A, float* b, int size, int max_iterations,
            float (*f)(float*, float*, const int, const float, const int)) {
  char* x_x = new char[sizeof(float) * size + ALIGN];
  char* y_x = new char[sizeof(float) * size + ALIGN];
  float* x = (float*)(((uintptr_t)x_x + ALIGN) & (~(uintptr_t)0x0F));
  float* y = (float*)(((uintptr_t)y_x + ALIGN) & (~(uintptr_t)0x0F));

  for (int i = 0; i < size; ++i) {
    x[i] = y[i] = 0.0;
  }

  struct timeval t1, t2;
  int k = 0, icol = 0;
  gettimeofday(&t1, 0);
  do {
    for (int i = 0; i < size; ++i) {
      y[i] = x[i];
    }

#pragma omp parallel for private(icol) shared(x, y, b)
    for (icol = 0; icol < size; ++icol) {
      x[icol] = (*f)(A, y, icol, b[icol], size);
    }

    for (int i = 0; i < size; i += 4) {
      _mm_store_ps(&y[i], _mm_load_ps(&x[i]));
    }

  } while (++k < max_iterations); // && distance(y, x, size) >= TOL);
  gettimeofday(&t2, 0);

  float err = 0.0;
  multiply(A, y, x, size);
  int loop;
#pragma omp parallel for reduction(+ : err) private(loop) shared(b, x)
  for (loop = 0; loop < size; ++loop) {
    err += (x[loop] - b[loop]) * (x[loop] - b[loop]);
  }
  err = sqrt(err) / size;
  std::cout << err << "\t" << deltaTime(t1, t2) << "\t";

  delete x_x;
  delete y_x;
}

void init(float* A, float* b, int size, float density) {
  for (int i = 0; i < size; ++i) {
    b[i] = frand(1.0, true);
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

  char* A_x = new char[sizeof(float) * size * size + ALIGN];
  char* b_x = new char[sizeof(float) * size + ALIGN];

  float* A = (float*)(((uintptr_t)A_x + ALIGN) & (~(uintptr_t)0x0F));
  float* b = (float*)(((uintptr_t)b_x + ALIGN) & (~(uintptr_t)0x0F));

  init(A, b, size, density);
  std::cout << size << "\t" << max_iterations << "\t" << density << "\t"
            << num_threads << "\t";
  driver(A, b, size, max_iterations, scalarProduct);
  driver(A, b, size, max_iterations, vectorizedProduct);
  std::cout << std::endl;

  delete b_x;
  delete A_x;

  return 0;
}
