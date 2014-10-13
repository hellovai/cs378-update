#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include <map>

#define FIR_SIZE 199

void handle(char* name);
void read_filter(char *name);
float *read_audio(char* file_name);
void write_audio(char* file_name, int16_t* buffer);
void FIR(float* x, float* h, float* y, int size);
void apply_filter(char *in_r, char *in_l, char *out_r, char *out_l, int angle, float elevation);
void apply_filter_2(char *in, char *out_r, char *out_l, int angle, float elevation);
void apply_surround(char *in, char *out_r, char *out_l, float elevation);
