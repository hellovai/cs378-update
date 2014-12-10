#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <thread>
#include <atomic>
#include <iomanip>
#include "xmmintrin.h"

#define INDEX(N, C, R) (R * N) + C
#define MAX_VAL 1.0
#define TOL 0.000000001

float frand(float m = MAX_VAL) {
  return ((float)rand()) / ((float)(RAND_MAX / m));
}

void multiply(float* A, float* b, float* x, int size) {
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++) x[i] += b[i] * A[INDEX(size, j, i)];
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

float scalarSolution(const float* A, const float* y, const int i, const float b, const int size) {
  float val = 0.0;
  for (int j = 0; j < size; ++j)
    if (j != i) val += A[INDEX(size, i, j)] * y[j];
  return (-val + b) / A[INDEX(size, i, i)];
}

float vectorizedSolution(float* A, float* y, const int i, const float b, const int size) {
  __m128 rA, rB, rC;

  int j = 0;
  float val = 0.0;
  float acc[4];

  for(j = 0; j < size; j += 4) {
    rB = _mm_set_ps(A[INDEX(size, i, j)], A[INDEX(size, i, j+1)], A[INDEX(size, i, j+2)], A[INDEX(size, i, j+3)]);
    rC = _mm_set_ps(y[j], y[j+1], y[j+2], y[j+3]);
    _mm_add_ps(rA, _mm_mul_ps(rB, rC));
  }

  _mm_store_ps(acc, rA);
  val += (acc[0] + acc[1] + acc[2] + acc[3]);

  val -= (A[INDEX(size, i, i)] * y[i]);
  return (- val + b) / A[INDEX(size, i, i)];
}

std::atomic<int> counter(0);

void runThread(float* x, float* A, float* y, const float* b, const int size) {
  while (counter < size) {
    int row = counter++;
    if (row >= size) break;
    x[row] = vectorizedSolution(A, y, row, b[row], size);
  }
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
  float density = (float) atof(argv[3]);
  int num_threads = atoi(argv[4]);

  float* A = new float[size * size];
  float* x = new float[size];
  float* y = new float[size];
  float* b = new float[size];

  for (int i = 0; i < size; ++i) {
    b[i] = frand();
    x[i] = y[i] = 0.0;
  }

  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j)
      A[INDEX(size, i, j)] = (frand() < density) ? frand(10.0) : 0.0;

  float sum = 0;

  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j) 
      sum += A[INDEX(size, i, j)];

  for (int i = 0; i < size; ++i) 
    A[INDEX(size, i, i)] = frand() + sum;

  for (int k = 0; k < max_iterations; ++k) {
    counter = 0;

    std::vector<std::thread> v;
    for (int i = 0; i < num_threads; ++i) {
      v.push_back(std::thread(runThread, x, A, y, b, size));
    }

    for (int i = 0; i < num_threads; ++i) {
      v[i].join();
    }

    for (int i = 0; i < size; ++i) {
      y[i] = x[i];
    }

    // for (int i = 0; i < size; i+=4) {
    //   __m128 _x = _mm_load_ps(&x[i]);
    //   _mm_store_ps(&y[i], _x);
    // }
  }

  float err = 0.0;
  multiply(A, y, x, size);
  for (int i = 0; i < size; ++i) {
    err += (x[i] - b[i]) * (x[i] - b[i]);
  }
  err = sqrt(err) / size;
  std::cout << "Error: " << err << std::endl;

  delete x;
  delete y;
  delete b;
  delete A;

  return 0;
}
