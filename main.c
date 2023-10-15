// Do complicated pointer business for lisp

#include <alsa/asoundlib.h>
#include <math.h>

int initialize_params (snd_pcm_t *pcm) {
  return snd_pcm_set_params(pcm,
                            SND_PCM_FORMAT_U16_LE,
                            SND_PCM_ACCESS_RW_INTERLEAVED,
                            1,
                            44100,
                            0, //no resampling
                            1000); // latency 1ms
}

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
  err = snd_pcm_set_params(handle,
                           SND_PCM_FORMAT_U16_LE,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           1,
                           44100,
                           0, //no resampling
                           1000); //1ms
  if (err < 0) {   /* 0.5sec */
    printf("Playback open error: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }
 
  snd_pcm_get_params(handle, &buffer_size, &period_size);
  printf("frames %ld, period size %ld\n", buffer_size, period_size);
  
  //generate sin
  unsigned int length = 44100 * 10;
  uint16_t buffer[length];
  for (unsigned int i = 0; i < length; ++i) {
    buffer[i] = (uint16_t)(32767.5 + 32767.5 * sin(2 * M_PI * 440 * i / 44100));
  }
  err = snd_pcm_writei(handle, buffer, length);
  if (err == -EPIPE) {
    printf("Underrun occurred: %s\n", snd_strerror(err));
  } else if (err < 0) {
    printf("error %s\n", snd_strerror(err));
  }
  
  err = snd_pcm_drain(handle);
  if (err < 0)
    printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
  snd_pcm_close(handle);
  return 0;
}
