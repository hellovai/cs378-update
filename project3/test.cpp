#include <climits>
#include <iostream>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include <queue>
#include <stack>
#include <vector>
#include <string>

#include <thread>
#include <mutex>

#include "csr.h"
#include "parallelsssp.h"

typedef struct { int id; } Node;

typedef struct {
  int size;
  int* csr;
  int* edges;
  int* indirect;
  Node* data;
} Graph;

typedef struct {
  int index;
  int weight;
} QueueType;

class Comparator {
  bool reverse;

 public:
  Comparator(const bool& revparam = false) { reverse = revparam; }
  bool operator()(const QueueType& lhs, const QueueType& rhs) const {
    if (reverse)
      return (lhs.weight > rhs.weight);
    else
      return (lhs.weight < rhs.weight);
  }
};

template <typename T>
class MyQueue : public std::queue<T> {
 public:
  T top() { return this->front(); }
};

std::mutex mutex;

template <typename T>
void dijkstra(Graph* g, int* dist, T q) {
  int start, end, u, v, alt, i;
  while (!q.empty()) {
    u = q.top().index;
    start = (u == 0) ? 0 : g->indirect[u - 1];
    end = g->indirect[u];
    q.pop();
    for (i = start; i < end; ++i) {
      v = g->csr[i];
      alt = g->edges[i] + dist[u];
      mutex.lock();
      if (alt < dist[v]) {
        dist[v] = alt;
        QueueType temp = {v, alt};
        q.push(temp);
      }
      mutex.unlock();
    }
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

Graph init(int size) {
  try {
    Node* data = new Node[size];
    int* csr = new int[size * size];
    int* edges = new int[size * size];
    int* indirect = new int[size];

    createCSR(csr, edges, indirect, 2, size);
    return {size, csr, edges, indirect, data};
  } catch (std::bad_alloc& e) {
    std::cout << "Memory Allocation error" << std::endl;
    exit(-1);
  }
}

double deltaTime(struct timeval t1, struct timeval t2) {
  struct timeval ret;
  timersub(&t2, &t1, &ret);
  return (double)ret.tv_sec + (double)ret.tv_usec / 1000000.0;
}

template <typename T>
void runDijkstra(Graph* g, int num_threads, std::string name, int source = 0) {
  struct timeval t1, t2;
  std::cout << "----" << name << "----" << std::endl;

  int* dist = new int[g->size];
  for (int i = 0; i < g->size; ++i) {
    dist[i] = INT_MAX;
  }

  // init
  T q;
  dist[source] = 0;
  QueueType temp = {source, 0};
  q.push(temp);

  std::vector<std::thread> v;
  for (int i = 0; i < num_threads; i++) {
    v.push_back(std::thread(dijkstra<T>, g, dist, q));
  }

  gettimeofday(&t1, 0);
  for (int i = 0; i < num_threads; i++) {
    v[i].join();
  }
  gettimeofday(&t2, 0);
  printf("Time:\t%f\n", deltaTime(t1, t2));

  delete dist;
}

int main(int argc, char const* argv[]) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <size> <num_threads>" << std::endl;
    exit(-1);
  }
  int size = atoi(argv[1]);
  int num_threads = atoi(argv[2]);

  Graph g = init(size);

  runDijkstra<MyQueue<QueueType> >(&g, num_threads, "Queue");
  runDijkstra<std::stack<QueueType> >(&g, num_threads, "Stack");
  runDijkstra<
      std::priority_queue<QueueType, std::vector<QueueType>, Comparator> >(
      &g, num_threads, "Priority Queue");

  // free memory
  delete g.data;
  delete g.csr;
  delete g.edges;
  delete g.indirect;

  return 0;
}

// compile
// g++ parallelsssp.cpp csr.cpp -std=c++0x -o project3 -pthread
