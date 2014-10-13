#include "fir.h"

int main(int argc, char** argv) {
  char* filter_name = argv[1];
  char* in_audio_file_r = argv[2];
  char* in_audio_file_l = argv[3];
  char* out_audio_file_r = argv[4];
  char* out_audio_file_l = argv[5];

  read_filter(filter_name);

  int angle = -30; float elevation = 0;

  apply_filter(in_audio_file_r, in_audio_file_l, out_audio_file_r, out_audio_file_l, angle, elevation);
}
