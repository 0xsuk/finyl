#include <stdio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <math.h>

#define SAMPLE_RATE 44100

void generate_sine_wave(float* buffer, int freq, int length) {
  for (int i = 0; i < length; ++i) {
    buffer[i] = sin(2 * M_PI * freq * i / SAMPLE_RATE);
  }
}

int main(int argc, char *argv[]) {
  if(argc < 3) {
    fprintf(stderr, "Usage: %s <speaker-device-name> <bluetooth-device-name>\n", argv[0]);
    return 1;
  }
  
  const char *speaker_device_name = argv[1];
  const char *bluetooth_device_name = argv[2];
  
  static const pa_sample_spec ss = {
    .format = PA_SAMPLE_FLOAT32,
    .rate = SAMPLE_RATE,
    .channels = 1
  };

  pa_simple *s_speaker = NULL, *s_bluetooth = NULL;
  int ret = 1;
  int error;

  if (!(s_speaker = pa_simple_new(NULL, "Speaker", PA_STREAM_PLAYBACK, speaker_device_name, "Playback", &ss, NULL, NULL, &error))) {
    fprintf(stderr, "PA Error: %s\n", pa_strerror(error));
    goto finish;
  }

  if (!(s_bluetooth = pa_simple_new(NULL, "Bluetooth", PA_STREAM_PLAYBACK, bluetooth_device_name, "Playback", &ss, NULL, NULL, &error))) {
    fprintf(stderr, "PA Error: %s\n", pa_strerror(error));
    goto finish;
  }

  float buffer[SAMPLE_RATE];
  generate_sine_wave(buffer, 440, SAMPLE_RATE);  // 440 Hz for speaker
  if (pa_simple_write(s_speaker, buffer, sizeof(buffer), &error) < 0) {
    fprintf(stderr, "PA Error: %s\n", pa_strerror(error));
    goto finish;
  }

  generate_sine_wave(buffer, 880, SAMPLE_RATE);  // 880 Hz for bluetooth
  if (pa_simple_write(s_bluetooth, buffer, sizeof(buffer), &error) < 0) {
    fprintf(stderr, "PA Error: %s\n", pa_strerror(error));
    goto finish;
  }

  ret = 0;

 finish:
  if (s_speaker) pa_simple_free(s_speaker);
  if (s_bluetooth) pa_simple_free(s_bluetooth);

  return ret;
}
