#include "csr.h"

void createCSR(int* csr, int* edges, int* indirect, int connectedTo, int size) {
  for (int i = 0; i < size; ++i) {
    indirect[i] = (i + 1) * connectedTo;
    for (int j = 0; j < connectedTo; ++j) {
      csr[i * connectedTo + j] = (j + i + 1) % size;
      edges[i * connectedTo + j] = 1;
    }
  }
}
