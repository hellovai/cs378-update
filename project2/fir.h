#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <sys/time.h>
#include "xmmintrin.h"

#include <map>

#define FIR_SIZE 199

void handle(char* name);
void read_filter(char *name);
float *read_audio(char* file_name);
void merge(float* l, float* r, float* o);
void write_audio(char* file_name, int16_t* buffer);
void write_audio_2(char* file_name, int16_t* buffer);
void FIR(float* x, float* h, float* y, int size);
void apply_filter(char *in_r, char *in_l, char *out, int angle, float elevation);
void apply_surround(char *in, char *out, float elevation);
double deltaTime(struct timeval t1, struct timeval t2);
