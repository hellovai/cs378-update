#include "csr.h"
#include "parallelsssp.h"
#include <climits>
#include <iostream>
#include <queue>
#include <omp.h>
#include <stdlib.h>
#define NUM_THREADS 2

typedef struct { int id; } Node;

typedef struct {
  int size;
  int* csr;
  int* edges;
  int* indirect;
  Node* data;
} Graph;

void dijkstra(Graph* g, int source, int* dist) {
  std::queue<int> q;
  dist[source] = 0;
  q.push(source);
  int start, end, u, v, alt, i;
  try {
  #pragma omp parallel private(start, end, u, v, alt, i) shared(g, q, dist)
  {
    while (!q.empty()) {
      // std::cout << omp_get_thread_num() << std::endl;
      u = q.front();

      start = (u == 0) ? 0 : g->indirect[u - 1];
      end = g->indirect[u];
      q.pop();
      #pragma omp critical
      {
        for (i = start; i < end; ++i) {
          v = g->csr[i];
          alt = g->edges[i] + dist[u];
          if (alt < dist[v]) {
            dist[v] = alt;
            q.push(v);
          }
        }
      }
    }
  }
  } catch (...) {
    std::cout << "Exception!" << std::endl;
  }
}

void print(int* csr, int* indirect, int MAX_NODES) {
  for (int i = 0; i < MAX_NODES; ++i) {
    int s = i == 0 ? 0 : indirect[i - 1];
    int e = indirect[i];
    for (int j = s; j < e; ++j) {
      // printf("%d\t%d\n", i, csr[j]);
    }
  }
}

int main(int argc, char const* argv[]) {
  omp_set_num_threads(NUM_THREADS);
  std::cout << "start..." << std::endl;
  /* code */
  int size = 1000;

  int* dist;
  Graph g;
  try {
    Node* data = new Node[size];
    int* csr = new int[size * size];
    int* edges = new int[size * size];
    int* indirect = new int[size];
    dist = new int[size];

    createCSR(csr, edges, indirect, 2, size);
    g = {size, csr, edges, indirect, data};
  } catch (std::bad_alloc& e) {
    exit(-1);
  }

  for (int i = 0; i < size; ++i) {
    dist[i] = INT_MAX;
  }

  dijkstra(&g, 0, dist);

  std::cout << "done..." << std::endl;
  for (int i = 0; i < size; ++i) {
    std::cout << dist[i] << " ";
  }
  std::cout << std::endl;

  delete g.data;
  delete g.csr;
  delete g.edges;
  delete g.indirect;
  delete dist;

  return 0;
}
