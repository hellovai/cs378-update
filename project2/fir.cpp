#include "fir.h"

#define FILTERSIZE 199

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

void floatToInt( float *input, int16_t *output, int length ) {
  for (int i = 0; i < length; i++ ) {
    if ( input[i] > 32767.0 ) {
      input[i] = 32767.0;
    } else if ( input[i] < -32768.0 ) {
      input[i] = -32768.0;
    }
    output[i] = (int16_t)input[i];
  }
}

void apply_filter(char *in_r, char *in_l, char *out_r, char *out_l, int angle, float elevation) {
  float* lpFilter = fir_filter[angle][elevation]['L'];
  float* lnFilter = fir_filter[-angle][elevation]['L'];
  float* rpFilter = fir_filter[angle][elevation]['R'];
  float* rnFilter = fir_filter[-angle][elevation]['R'];
  // std::ofstream foutl(out_l, std::ios::out | std::ios::binary);

  float * l_stream = read_audio(in_r);
  float * r_stream = read_audio(in_l);
  int size = lSize;

  float * lo_stream = (float*) malloc(sizeof(float) * size);
  int16_t * lo_stream_clip = (int16_t*) malloc(sizeof(int16_t) * size);

  float * ro_stream = (float*) malloc(sizeof(float) * size);
  int16_t * ro_stream_clip = (int16_t*) malloc(sizeof(int16_t) * size);

  for (int i = 0; i < size; i++) {
    lo_stream[i] = 0.0;
    ro_stream[i] = 0.0;
  }

  FIR(l_stream, lnFilter, lo_stream, size);
  FIR(r_stream, lpFilter, lo_stream, size);
  floatToInt(lo_stream, lo_stream_clip, size);

  FIR(l_stream, rnFilter, ro_stream, size);
  FIR(r_stream, rpFilter, ro_stream, size);
  floatToInt(ro_stream, ro_stream_clip, size);
  

  //write_audio(out_l, lo_stream_clip);
  //write_audio(out_r, ro_stream_clip);

  free(lo_stream_clip);
  free(ro_stream_clip);
  free(lo_stream);
  free(ro_stream);
  free(l_stream);
  free(r_stream);
}

void apply_filter_2(char *in, char *out_r, char *out_l, int angle, float elevation) {
  float* lpFilter = fir_filter[angle][elevation]['L'];
  float* lnFilter = fir_filter[-angle][elevation]['L'];
  float* rpFilter = fir_filter[angle][elevation]['R'];
  float* rnFilter = fir_filter[-angle][elevation]['R'];
  // std::ofstream foutl(out_l, std::ios::out | std::ios::binary);

  float * a_stream = read_audio(in);
  int size = lSize;

  float * lo_stream = (float*) malloc(sizeof(float) * size);
  int16_t * lo_stream_clip = (int16_t*) malloc(sizeof(int16_t) * size);

  float * ro_stream = (float*) malloc(sizeof(float) * size);
  int16_t * ro_stream_clip = (int16_t*) malloc(sizeof(int16_t) * size);

  for (int i = 0; i < size; i++) {
    lo_stream[i] = 0.0;
    ro_stream[i] = 0.0;
  }

  // FIR(a_stream, lnFilter, lo_stream, size);
  FIR(a_stream, lpFilter, lo_stream, size);
  floatToInt(lo_stream, lo_stream_clip, size);

  // FIR(a_stream, rnFilter, ro_stream, size);
  FIR(a_stream, rpFilter, ro_stream, size);
  floatToInt(ro_stream, ro_stream_clip, size);

  write_audio(out_l, lo_stream_clip);
  write_audio(out_r, ro_stream_clip);

  free(lo_stream_clip);
  free(ro_stream_clip);
  free(lo_stream);
  free(ro_stream);
  free(a_stream);
}

void apply_surround(char *in, char *out_r, char *out_l, float elevation) {
  float * a_stream = read_audio(in); int size = lSize;

  float * lo_stream = (float*) malloc(sizeof(float) * size);
  int16_t * lo_stream_clip = (int16_t*) malloc(sizeof(int16_t) * size);

  float * ro_stream = (float*) malloc(sizeof(float) * size);
  int16_t * ro_stream_clip = (int16_t*) malloc(sizeof(int16_t) * size);

  for (int i = 0; i < size; i++) {
    lo_stream[i] = 0.0;
    ro_stream[i] = 0.0;
  }
  #define FRAME 44100
  #define TIME 220500

  int angle[48] = {-80, -65, -55, -45, -40, -35, -30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 55, 65, 80,65,55,45,40,35,30,25,20,15,10,5,0,-5,-10,-15,-20,-25,-30,-35,-40,-45,-55,-65};
  for (int i = 0; i < size - TIME; i += TIME ) {
    int random = rand() % 48;
    float* lpFilter = fir_filter[ angle[random] ][elevation]['L'];
    float* rpFilter = fir_filter[ angle[random] ][elevation]['R'];

    FIR(a_stream + i, lpFilter, lo_stream + i, TIME);
    floatToInt(lo_stream + i, lo_stream_clip + i, TIME);

    FIR(a_stream + i, rpFilter, ro_stream + i, TIME);
    floatToInt(ro_stream + i, ro_stream_clip + i, TIME);
  }

  write_audio(out_l, lo_stream_clip);
  write_audio(out_r, ro_stream_clip);

  free(lo_stream_clip);
  free(ro_stream_clip);
  free(lo_stream);
  free(ro_stream);
  free(a_stream);
}
