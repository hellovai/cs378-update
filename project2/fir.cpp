#include "fir.h"

#define FILTERSIZE 199
#define TIME 220500 // 5 seconds

std::map< int, std::map<float, std::map<char, float*> > > fir_filter;

void handle(char* name) {
  printf("File could not be opened: %s\n", name);
  exit(-1);
}

void read_filter(char *name) {
  std::fstream fin(name);
  if (fin.is_open()) {
    int angle;
    float elevation;
    char letter;
    while( fin >> angle) {
      fin >> elevation >> letter;
      fir_filter[angle][elevation][letter] = (float *) malloc(sizeof(float) * (FIR_SIZE + 1));
      for (int i = 0; i <= FIR_SIZE; i++)
        fin >> fir_filter[angle][elevation][letter][i];
    }
    fin.close();
  } else {
    handle(name);
  }
}

long lSize;

float *read_audio(char* file_name) {
  FILE * pFile;
  int16_t * buffer;
  size_t result;
  pFile = fopen ( file_name , "rb" );
  if (pFile == NULL) handle(file_name);
  fseek(pFile, 0, SEEK_END);
  lSize = ftell(pFile) / 2;
  rewind(pFile);
  buffer = (int16_t*) malloc(sizeof(int16_t) * lSize);
  fread(buffer,2, lSize, pFile);
  fclose(pFile);

  float* buffer_ft = (float*) malloc(sizeof(float) * lSize);
  for (int i = 0; i < lSize; i++)
    buffer_ft[i] = (float) buffer[i];
  free(buffer);
  return buffer_ft;
}

void write_audio(char* file_name, int16_t* buffer) {
  FILE * pFile;
  pFile = fopen ( file_name , "wb" );
  if (pFile == NULL) handle(file_name);
  fwrite(buffer, 2, lSize, pFile);
  fclose(pFile);
}

void write_audio_2(char* file_name, int16_t* buffer) {
  FILE * pFile;
  pFile = fopen ( file_name , "wb" );
  if (pFile == NULL) handle(file_name);
  fwrite(buffer, 2, lSize * 2, pFile);
  fclose(pFile);
}

void FIR(float* x, float* h, float* y, int size) {
  float zeroArr[1] = {0.0};
  for (int i = FIR_SIZE + 1; i < size; i+=4) {
    __m128 yi = _mm_load1_ps(&zeroArr[0]);
    for (int j = 0; j <= FIR_SIZE; j+=4) {
      //y[i] += h[FIR_SIZE - j] * x[i + j - FIR_SIZE];
      __m128 h199, h198, h197, h196;
      __m128 xPart1;  //x[0], x[1], x[2], x[3]
      __m128 xPart2; //x[4], x[5], x[6], x[7] ; this is __m128 x4567;
      h199 = _mm_load1_ps(&h[FILTERSIZE]);
      h198 = _mm_load1_ps(&h[FILTERSIZE-j-1]);
      h197 = _mm_load1_ps(&h[FILTERSIZE-j-2]);
      h196 = _mm_load1_ps(&h[FILTERSIZE-j-3]);
      xPart1 = _mm_load_ps(&x[i + j - FILTERSIZE - 1]);
      xPart2 = _mm_load_ps(&x[i + j - FILTERSIZE + 3]);

      //shuffle
      __m128 x3344 = _mm_shuffle_ps(xPart1, xPart2, _MM_SHUFFLE(0,0,4,4)); //x{3, 3, 4, 4}
      __m128 x1234 = _mm_shuffle_ps(xPart1, x3344, _MM_SHUFFLE(2,0,2,1)); //x{1, 2, 3, 4}

      __m128 x2345 = _mm_shuffle_ps(xPart1, xPart2, _MM_SHUFFLE(1,0,3,2)); //x{2, 3, 4, 5}

      __m128 x3456 = _mm_shuffle_ps(x3344, xPart2, _MM_SHUFFLE(2,1,2,0)); //x{3, 4, 5, 6}

      __m128 iMult0 = _mm_mul_ps(h199, x1234);
      __m128 iMult1 = _mm_mul_ps(h198, x2345);
      __m128 iMult2 = _mm_mul_ps(h197, x3456);
      __m128 iMult3 = _mm_mul_ps(h196, xPart2);

      __m128 addedAll = _mm_add_ps(iMult3, _mm_add_ps(iMult2, _mm_add_ps(iMult1, iMult0))); //sum each multiplied vector
      yi = _mm_add_ps(yi, addedAll);
      /*if (i == size-1) {
        printf("FIR_SIZE - j is %d, i+j-FIR_SIZE is %d\n", FIR_SIZE - j, i+j-FIR_SIZE);
      }*/
      //printf("j = %d\n", j);
    }

    _mm_store_ps(&y[i], yi);
  }
}

void merge(float* l, float* r, float* o) {
  __m128 left, right, firstFour, secondFour;
  for (int i = 8; i < lSize * 2; i+= 8) {
    left = _mm_load_ps(&l[(i-8) / 2]); // l {0, 1, 2, 3}
    right = _mm_load_ps(&r[(i-8) / 2]); // r {0, 1, 2, 3}

    //shuffle the data to get l[0] r[0] l[1] r[1]
    firstFour = _mm_shuffle_ps(left, right, _MM_SHUFFLE(1,0,1,0));

    //shuffle the data to get l[2] r[2] l[3] r[3]
    secondFour = _mm_shuffle_ps(left, right, _MM_SHUFFLE(3,2,3,2));

    _mm_store_ps(&o[i-8], firstFour);
    _mm_store_ps(&o[i-4], secondFour);
  }
}

void floatToInt( float *input, int16_t *output, int length ) {
  float max = 32767.0; float min = -32768.0;
  __m128 maxT = _mm_load1_ps(&max);
  __m128 minT = _mm_load1_ps(&min);
  __m128 in, in_max, in_min;
  for (int i = 4; i < length; i+= 4 ) {
    in = _mm_load_ps(&input[i]);
    in_min = _mm_cmplt_ps(in, minT);
    in_max = _mm_cmpgt_ps(in, maxT);
    in = _mm_or_ps(_mm_and_ps(in_min, minT), _mm_andnot_ps(in_min, _mm_or_ps(_mm_and_ps(in_max, maxT), _mm_andnot_ps(in_max, in))));
    _mm_store_ps(&input[i], in);
  }

  for (int i = 0; i < length; i++) {
    output[i] = (int16_t) input[i];
  }
}

double deltaTime(struct timeval t1, struct timeval t2) {
  struct timeval ret;
  timersub(&t2, &t1, &ret);
  return (double)ret.tv_sec + (double)ret.tv_usec / 1000000.0;
}

void apply_filter(char *in_r, char *in_l, char *out, int angle, float elevation) {
  float* lpFilter = fir_filter[angle][elevation]['L'];
  float* lnFilter = fir_filter[-angle][elevation]['L'];
  float* rpFilter = fir_filter[angle][elevation]['R'];
  float* rnFilter = fir_filter[-angle][elevation]['R'];
  // std::ofstream foutl(out_l, std::ios::out | std::ios::binary);

  float * l_stream = read_audio(in_r);
  float * r_stream = read_audio(in_l);
  int size = lSize;

  float * lo_stream = (float*) malloc(sizeof(float) * size);
  float * ro_stream = (float*) malloc(sizeof(float) * size);

  float * o_stream = (float*) malloc(sizeof(float) * size * 2);
  int16_t * o_stream_clip = (int16_t*) malloc(sizeof(int16_t) * size * 2);

  for (int i = 0; i < size; i++) {
    lo_stream[i] = 0.0;
    ro_stream[i] = 0.0;
  }

  double timeTotal;
  struct timeval t1, t2;
  gettimeofday(&t1, 0);
  FIR(l_stream, lnFilter, lo_stream, size);
  FIR(r_stream, lpFilter, lo_stream, size);
  FIR(l_stream, rnFilter, ro_stream, size);
  FIR(r_stream, rpFilter, ro_stream, size);
  merge(lo_stream, ro_stream, o_stream);
  floatToInt(o_stream, o_stream_clip, size * 2);
  gettimeofday(&t2, 0);
  timeTotal = deltaTime(t1, t2);
  printf("Time taken for filter: %f\n", timeTotal);

  write_audio_2(out, o_stream_clip);

  free(o_stream_clip);
  free(o_stream);
  free(lo_stream);
  free(ro_stream);
  free(l_stream);
  free(r_stream);
}

void apply_surround(char *in, char *out, float elevation) {
  float * a_stream = read_audio(in); int size = lSize;

  float * lo_stream = (float*) malloc(sizeof(float) * size);
  float * ro_stream = (float*) malloc(sizeof(float) * size);

  float * o_stream = (float*) malloc(sizeof(float) * size);
  int16_t * o_stream_clip = (int16_t*) malloc(sizeof(int16_t) * size * 2);

  for (int i = 0; i < size; i++) {
    lo_stream[i] = 0.0;
    ro_stream[i] = 0.0;
  }

  int angle[48] = {-80, -65, -55, -45, -40, -35, -30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 55, 65, 80,65,55,45,40,35,30,25,20,15,10,5,0,-5,-10,-15,-20,-25,-30,-35,-40,-45,-55,-65};

  double timeTotal;
  struct timeval t1, t2;
  gettimeofday(&t1, 0);
  for (int i = 0; i < size - TIME; i += TIME ) {
    int random = rand() % 48;
    float* lpFilter = fir_filter[ angle[random] ][elevation]['L'];
    float* rpFilter = fir_filter[ angle[random] ][elevation]['R'];

    FIR(a_stream + i, lpFilter, lo_stream + i, TIME);
    FIR(a_stream + i, rpFilter, ro_stream + i, TIME);
  }
  merge(lo_stream, ro_stream, o_stream);
  floatToInt(o_stream, o_stream_clip, size * 2);
  gettimeofday(&t2, 0);
  timeTotal = deltaTime(t1, t2);
  printf("Time taken for filter: %f\n", timeTotal);

  write_audio(out, o_stream_clip);

  free(o_stream_clip);
  free(o_stream);
  free(lo_stream);
  free(ro_stream);
  free(a_stream);
}
