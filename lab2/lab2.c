#include <stdio.h>
#include <stdlib.h>
#include <papi.h>

#define NUM_PAPI_COUNTERS 4

int papi_counters[NUM_PAPI_COUNTERS] =
  { PAPI_TOT_INS, PAPI_FP_INS, PAPI_L1_DCM, PAPI_L2_TCM };
long long int papi_values_[NUM_PAPI_COUNTERS];

int N = 1;
int **A, **B, **C;

void handle_error (int retval) {
  printf("PAPI error %d: %s\n", retval, PAPI_strerror(retval));
  exit(1);
}

void init_papi() {
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT && retval < 0) {
    printf("PAPI library version mismatch!\n");
    exit(1);
  }
  if (retval < 0) handle_error(retval);
}

void begin_papi() {
  int EventSet = PAPI_NULL;
  int rv;
  /* Create the Event Set */
  if ((rv = PAPI_start_counters(papi_counters, NUM_PAPI_COUNTERS)) != PAPI_OK)
    handle_error(rv);
}

void end_papi(int EventSet) {
  long_long retval;
  int rv;

  /* get the values */
  if ((rv = PAPI_read_counters(papi_values_, NUM_PAPI_COUNTERS)) != PAPI_OK)
    handle_error(rv);
}

void clearCache() {
  char* a = (char*) _mm_malloc(sizeof(char) * 1024 * 1024 * 16,16);
  char* b = (char*) _mm_malloc(sizeof(char) * 1024 * 1024 * 16,16);

  memcpy(a,b,sizeof(char) * 1024 * 1024 * 16);
  memcpy(b,a,sizeof(char) * 1024 * 1024 * 16);
  _mm_free(a);
  _mm_free(b);
}


void init() {
  A = new *int[N];
  B = new *int[N];
  C = new *int[N];
  for (int i = 0; i < N; i++) {
    A[i] = new int[N];
    B[i] = new int[N];
    C[i] = new int[N];
    for (int j = 0; j < N; j++)
      A[i][j] = rand() % (N * N) + 1;
      B[i][j] = rand() % (N * N) + 1;
      C[i][j] = 0;
  }
}

int multiply() {
  for ( i = 0 ; i < N ; i++ )
    for ( j = 0 ; j < N ; j++ )
      for ( k = 0 ; k < N ; k++ )
        C[i][j] += A[i][k]*B[k][j];
}


int main(int argc, char** argv) {
  init_papi();
  N = atoi(argv[1]);
  init();
  clearCache();
  begin_papi();
  multiply();
  end_papi();
  std::cout << N << " ";
  for (int i = 0; i < NUM_PAPI_COUNTERS; i++) {
    std::cout << papi_values_[i] << " ";
  }
  std::cout << std:endl;
return 0;
}