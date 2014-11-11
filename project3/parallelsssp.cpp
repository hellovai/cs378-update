#include <climits>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include <queue>
#include <stack>
#include <vector>
#include <map>
#include <deque>
#include <string>

#include <thread>
#include <mutex>

// #include "csr.h"
#include "parallelsssp.h"

typedef struct {
  int size;
  std::vector<int> csr;
  std::vector<int> edges;
  std::vector<int> indirect;
  std::vector<int> data;
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
  T popper(int id) {
    T temp = this->front();
    this->pop();
    return temp;
  }
  void pusher(T item, int id) { this->push(item); }
};

template <typename T>
class MyStack : public std::stack<T> {
 public:
  T popper(int id) {
    T temp = this->top();
    this->pop();
    return temp;
  }
  void pusher(T item, int id) { this->push(item); }
};

template <typename T, typename C>
class MyPriority : public std::priority_queue<T, std::vector<T>, C> {
 public:
  T popper(int id) {
    T temp = this->top();
    this->pop();
    return temp;
  }
  void pusher(T item, int id) { this->push(item); }
};

std::mutex lock;

template <typename T>
class SharedQueue {
  std::vector<std::priority_queue<T, std::vector<T>, Comparator>> _list;
  std::deque<std::mutex *> _block;
  int counter;
 public:
  static int N;
  SharedQueue() : counter(0) {
    for (int i = 0; i < N; ++i) {
      _list.push_back(std::priority_queue<T, std::vector<T>, Comparator>());
      _block.push_back(new std::mutex());
    }
  }

  T popper(int id) {
    T temp;
    _block[id]->lock();
    if (_list[id].size() > 0) {
      temp = _list[id].top();
      _list[id].pop();
      lock.lock();
      counter--;
      lock.unlock();
    } else {
      int i = (id == 0) ? 1 : 0;
      bool searching = true;
      while (searching) {
        while (!_block[i]->try_lock()) {
          i = (++i) % N;
          if (counter == 0) {
            _block[id]->unlock();
            return {-1, 0};
          }
        }
        if (_list[i].size() > 0) {
          temp = _list[i].top();
          _list[i].pop();
          lock.lock();
          counter--;
          lock.unlock();
          searching = false;
        }
        _block[i]->unlock();
      }
    }
    _block[id]->unlock();
    return temp;
  }

  void pusher(T obj, int id) {
    _block[id]->lock();
    _list[id].push(obj);
    lock.lock();
    counter++;
    lock.unlock();
    _block[id]->unlock();
  }
  bool empty() { return counter == 0; }
};

std::mutex m;

template <typename T>
void dijkstra(Graph* g, int* dist, T q, int id) {
  int start, end, u, v, alt, i;
  while (!q.empty()) {
    u = q.popper(id).index;
    if (u == -1) return;
    start = (u == 0) ? 0 : g->indirect[u - 1];
    end = g->indirect[u];
    for (i = start; i < end; ++i) {
      v = g->csr[i];
      alt = g->edges[i] + dist[u];
      m.lock();
      if (alt < dist[v]) {
        dist[v] = alt;
        QueueType temp = {v, alt};
        q.pusher(temp, id);
      }
      m.unlock();
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

Graph init(std::string filename, int size) {
  std::vector<int> csr(size), edges(size), data;
  std::map<int, int> d, d_ctr;
  std::ifstream ifs;
  ifs.open(filename, std::ifstream::in);
  int index = 0;

  while (ifs.good()) {
    int x, y, dist;
    ifs >> x >> y >> dist;
    if (d.count(x) == 0) {
      d[x] = index++;
      d_ctr[x] = 0;
      data.push_back(x);
    }
    if (d.count(y) == 0) {
      d[y] = index++;
      d_ctr[y] = 0;
      data.push_back(y);
    }
    d_ctr[x]++;

    csr.push_back(d[y]);
    edges.push_back(dist);
  }
  int sum = 0;
  std::vector<int> indirect(data.size());
  for (int i = 0; i < data.size(); i++) {
    sum += d_ctr[data[i]];
    indirect[i] = sum;
  }
  return {(int)data.size(), csr, edges, indirect, data};
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
  q.pusher(temp, 0);

  std::vector<std::thread> v;
  for (int i = 0; i < num_threads; i++) {
    v.push_back(std::thread(dijkstra<T>, g, dist, q, i));
  }

  gettimeofday(&t1, 0);
  for (int i = 0; i < num_threads; i++) {
    v[i].join();
  }
  gettimeofday(&t2, 0);
  printf("Time:\t%f\n", deltaTime(t1, t2));

  delete dist;
}
template <typename T> int SharedQueue<T>::N = 0;

int main(int argc, char const* argv[]) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <num_threads> <file> <count>"
              << std::endl;
    exit(-1);
  }
  int num_threads = atoi(argv[1]);
  std::string filename = argv[2];
  int count = atoi(argv[3]);

  SharedQueue<QueueType>::N = num_threads;
  Graph g = init(filename, count);

  runDijkstra<MyQueue<QueueType> >(&g, num_threads, "Queue");
  runDijkstra<MyStack<QueueType> >(&g, num_threads, "Stack");
  runDijkstra<MyPriority<QueueType, Comparator> >(&g, num_threads,
                                                  "Priority Queue");
  runDijkstra<SharedQueue<QueueType> >(&g, num_threads,
                                       "Shared Priority Queue");

  return 0;
}

// compile
// g++ parallelsssp.cpp csr.cpp -std=c++0x -o project3 -pthread
