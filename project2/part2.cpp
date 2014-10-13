#include "fir.h"

int main(int argc, char** argv) {
  char* filter_name = argv[1];
  int angle = atoi(argv[2]);
  float elevation = atof(argv[3]);
  char* in_audio_file = argv[4];
  char* out_audio_file_r = argv[5];
  char* out_audio_file_l = argv[6];

  read_filter(filter_name);

  // apply_filter_2(in_audio_file, out_audio_file_r, out_audio_file_l, angle, elevation);
  apply_surround(in_audio_file, out_audio_file_r, out_audio_file_l, elevation);
}
