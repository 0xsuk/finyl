#include <alsa/asoundlib.h>

void check_formats(snd_pcm_t *handle) {
  snd_pcm_hw_params_t *params;
  snd_pcm_hw_params_alloca(&params);

  if (snd_pcm_hw_params_any(handle, params) < 0) {
    printf("Cannot configure this PCM device.\n");
    return;
  }

  for (int format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
    if (snd_pcm_hw_params_test_format(handle, params, format) == 0) {
      printf("Format %s is supported.\n", snd_pcm_format_name((snd_pcm_format_t)format));
    }
  }
}

int main(int argc, char **argv) {
  snd_pcm_t *handle;
  int err;

  if (argc < 2) {
    printf("usage: ./listdevice \"device name\"\n");
    return 0;
  }
  printf("argv1 %s\n", argv[1]);
  
  if ((err = snd_pcm_open(&handle, argv[1], SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error: %s\n", snd_strerror(err));
    return -1;
  }

  check_formats(handle);

  snd_pcm_close(handle);
  return 0;
}
