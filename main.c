// Do complicated pointer business for lisp

#include <alsa/asoundlib.h>
#include <math.h>

char *device = "plughw:CARD=PCH,DEV=0";            /* playback device */


int main(void)
{
  int err;
  snd_pcm_t *handle;
  snd_pcm_uframes_t buffer_size;
  snd_pcm_uframes_t period_size;
  
  if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error!: %s\n", snd_strerror(err));
    return -1;
  }

  snd_pcm_hw_params_t *hw_params;
  snd_pcm_hw_params_malloc(&hw_params);
  snd_pcm_hw_params_any(handle, hw_params);
  snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_U16_LE);
  snd_pcm_hw_params_set_rate(handle, hw_params, 44100, 0);
  snd_pcm_hw_params_set_channels(handle, hw_params, 1);
  buffer_size = 256;
  snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, &buffer_size);
  period_size = 128;
  snd_pcm_hw_params_set_period_size_near(handle, hw_params, &period_size, 0);
  snd_pcm_hw_params(handle, hw_params);
  snd_pcm_hw_params_free(hw_params);
  
  if (err < 0) {   /* 0.5sec */
    printf("Playback open error: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }
  
  snd_pcm_get_params(handle, &buffer_size, &period_size);
  printf("frames %ld, period size %ld\n", buffer_size, period_size);
  
  //generate sin

  double phase = 0;
  while (1) {
    unsigned short buffer[period_size];
    for (unsigned int i = 0; i < period_size; ++i) {
      phase += (2.0 * M_PI * 100.0 / 44100.0);
      buffer[i] = (unsigned short)(32767.5 + 32767.5 * sin(phase));
      if (phase >= 2.0 * M_PI) {
        phase -= 2.0 * M_PI;
      }
    }

    err = snd_pcm_writei(handle, buffer, period_size);
    if (err == -EPIPE) {
      printf("Underrun occurred: %s\n", snd_strerror(err));
      snd_pcm_prepare(handle);
    } else if (err == -EAGAIN) {
    } else if (err < 0) {
      printf("error %s\n", snd_strerror(err));
      goto cleanup;
    }
  }
  
 cleanup:
  err = snd_pcm_drain(handle);
  if (err < 0)
    printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
  snd_pcm_close(handle);
  printf("closed\n");
  return 0;
}
