#include "fir.h"

int main(int argc, char** argv) {
  char* filter_name = argv[1];
  float elevation = atof(argv[2]);
  char* in_audio_file = argv[3];
  char* out_audio_file = argv[4];

  read_filter(filter_name);

  apply_surround(in_audio_file, out_audio_file, elevation);
}
