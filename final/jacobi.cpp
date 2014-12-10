#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <thread>
#include <atomic>
#include <iomanip>

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

// vectorize
float getNewSolution(const float* A, const float* y, const int i, const float b,
                     const int size) {
  float val = 0.0;
  for (int j = 0; j < size; ++j)
    if (j != i) val += A[INDEX(size, i, j)] * y[j];
  return (-val + b) / A[INDEX(size, i, i)];
}

std::atomic<int> counter(0);
std::atomic<bool> iterator(true);
std::atomic<int> waiting(0);
std::atomic<bool> poll(true);

void runThread(float* x, const float* A, const float* y, const float* b,
               const int size) {
  while (iterator) {
    while (counter < size) {
      int row = counter++;
      if (row >= size) break;
      x[row] = getNewSolution(A, y, row, b[row], size);
    }
    waiting++;
    while(poll);
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
  float density = (float)atof(argv[3]);
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
      A[INDEX(size, i, j)] = (frand() < density) ? frand() : 0.0;

  float sum = 0;

  for (int i = 0; i < size; ++i)
    for (int j = 0; j < size; ++j) sum += A[INDEX(size, i, j)];

  for (int i = 0; i < size; ++i) A[INDEX(size, i, i)] = frand() + size;

  std::vector<std::thread> v;
  for (int i = 0; i < num_threads; ++i) {
    v.push_back(std::thread(runThread, x, A, y, b, size));
  }

  for (int k = 0; k < max_iterations; ++k) {
    while (waiting < num_threads);
    counter = 0;
    waiting = 0;

    // vectorize
    for (int i = 0; i < size; ++i) {
      y[i] = x[i];
    }

    if (k == max_iterations - 1)
      counter = size;

    poll = false;
  }
  iterator = false;
  poll = false;

  for (int i = 0; i < num_threads; ++i) {
    v[i].join();
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
