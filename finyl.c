#include "finyl_dev.h"
#include <alsa/asoundlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/wait.h>

void finyl_init_track_meta(finyl_track_meta* tm) {
  tm->id = -1;
  tm->tempo = 0;
  tm->musickeyid = -1;
  tm->filesize = 0;
}

void finyl_init_track(finyl_track* t) {
  finyl_init_track_meta(&t->meta);
  t->nchunks = 0;
  t->length = 0;
  t->index = 0;
  t->speed = 0;
  t->channels_count = 0;
}

void finyl_print_track(finyl_track* t) {
  printf("track {\n");
  printf("\tnchunks: %d\n", t->nchunks);
  printf("\tlength: %d\n", t->length);
  printf("\tindex: %lf\n", t->index);
  printf("\tspeed: %lf\n", t->speed);
  printf("\tchannels_count: %d\n", t->channels_count);
  printf("}\n");
}

finyl_sample get_sample(finyl_track* t, finyl_channel c) {
  int position = floor(t->index);
  int ichunk = position / chunk_size;
  int isample = position - (chunk_size * ichunk);
  finyl_chunk* chunk = c[ichunk];
  return *chunk[isample];
}

finyl_sample average_channels(finyl_track* t, int count) {
  finyl_sample s = 0;
  
  for (int i = 0; i<count; i++) {
    s += get_sample(t, t->channels[i]);
  }

  s = s/count;
  
  return s;
}

finyl_chunk* make_chunk() {
  return (finyl_chunk*)malloc(chunk_size * sizeof(finyl_sample));
}

int free_chunks(finyl_channel channel) {
  
}

finyl_channel make_channel() {
  return malloc(32 * sizeof(finyl_channel*));
}

int open_pcm_stream(FILE** fp, char* filename) {
  char command[1000];
  snprintf(command, sizeof(command), "ffmpeg -i %s -f s16le -ar 44100 -ac 1 -v quiet -", filename);
  *fp = popen(command, "r");
  if (*fp == NULL) {
    printf("failed to open stream for %s\n", filename);
    return -1;
  }

  return 0;
}

//reads from fp, updates nchunks, length, channel
int read_pcm(FILE* fp, finyl_channel channel, int* nchunks, int* length) {
  finyl_chunk* chunk = make_chunk();
  while (1) {
    size_t count = fread(chunk, sizeof(finyl_sample), chunk_size, fp);
    channel[*nchunks] = chunk;
    (*nchunks)++;
    *length += count;
    if (count < chunk_size) {
      return 0;
    }
    if (*nchunks == maximum_chunks) {
      printf("File is too large");
      //TODO
      free_chunks(channel);
      return -1;
    }
    chunk = make_chunk();
  }
}

int read_channel(char* file, finyl_channel channel, int* nchunks, int* length) {
  FILE* fp = NULL;
  open_pcm_stream(&fp, file);
  if (fp == NULL) {
    return -1;
  }
  if (read_pcm(fp, channel, nchunks, length) == -1) {
    return -1;
  }
  pclose(fp);
  return 0;
}

//each file corresponding to each channel
int finyl_read_channels_from_files(char** files, int channels_length, finyl_track* t) {
  for (int i = 0; i < channels_length; i++) {
    int nchunks = 0;
    int length = 0;
    finyl_channel channel = make_channel();
    if (channel == NULL) {
      printf("failed to allocate memory in finyl_read_channels_from_files\n");
      return -1;
    }
    if (read_channel(files[i], channel, &nchunks, &length) == -1) {
      free(channel);
      return -1;
    }
    if (i == 0) {
      t->length = length;
      t->nchunks = nchunks;
    } else {
      if (length != t->length | nchunks != t->nchunks) {
        printf("all channels have to be the same length\n");
        free(channel);
        return -1;
      }
    }

    t->channels_count++;
  }
  
  return 0;
}
//one file has amny channels (eg. flac)
void read_channels_from_file(char* file);

finyl_process_callback* a_callbacks;
finyl_process_callback* b_callbacks;
finyl_process_callback* c_callbacks;
finyl_process_callback* d_callbacks;

int finyl_set_process_callback(finyl_process_callback cb, finyl_deck_enum e, int channel_index) {
  bool valid = (-1 <= channel_index) && (channel_index < 10);
  if (!valid) {
    printf("finyl: channel_index = %d should be -1 to 9\n", channel_index);
    return -1;
  }

  bool all = channel_index == -1;
  
  return 0;
}

int finyl_remove_process_callback(finyl_process_callback cb, finyl_deck_enum e, int channel_index) {
  //
  return 0;
}

void handle_deck(finyl_deck* x, snd_pcm_uframes_t period_size) {
  //call callbacks for deck x, modify sample
  //deck.e

  x->t->index = x->t->speed;
  if (x->t->index >= x->t->length) {
    x->t->index = 0;
  }
}

void finyl_handle(finyl_deck* a, finyl_deck* b, finyl_deck* c, finyl_deck* d, finyl_sample* buffer, snd_pcm_uframes_t period_size) {
  for (signed int i = 0; i < period_size*2; i=i+2) {
    handle_deck(a, period_size);
    /* handle_deck(b, period_size); */
    
    //even sample i is for headphone.
    //odd sample i+1 is for speaker.
    buffer[i] = a->t->index;
    buffer[i+1] = a->t->index;
  }
}

char* device = "default";            /* playback device */

void setup_alsa_params(snd_pcm_t* handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size) {
  snd_pcm_hw_params_t* hw_params;
  snd_pcm_hw_params_malloc(&hw_params);
  snd_pcm_hw_params_any(handle, hw_params);
  snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_rate(handle, hw_params, 44100, 0);
  snd_pcm_hw_params_set_channels(handle, hw_params, 2);
  snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, buffer_size);
  snd_pcm_hw_params_set_period_size_near(handle, hw_params, period_size, 0);
  snd_pcm_hw_params(handle, hw_params);
  snd_pcm_hw_params_free(hw_params);
  snd_pcm_get_params(handle, buffer_size, period_size);
}

void cleanup_alsa(snd_pcm_t* handle) {
  int err = snd_pcm_drain(handle);
  if (err < 0) {
    printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
  }
  snd_pcm_close(handle);
  printf("closed\n");
}

void finyl_setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size) {
  int err;
  if ((err = snd_pcm_open(handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error!: %s\n", snd_strerror(err));
    exit(1);
  }
  setup_alsa_params(*handle, buffer_size, period_size);
  printf("buffer_size %ld, period_size %ld\n", *buffer_size, *period_size);
}

void finyl_run(finyl_deck* a, finyl_deck* b, finyl_deck* c, finyl_deck* d, snd_pcm_t* handle, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t period_size) {
  int err = 0;
  finyl_sample buffer[period_size*2];
  finyl_sample abuffer[period_size*2];
  finyl_sample bbuffer[period_size*2];
  finyl_sample cbuffer[period_size*2];
  finyl_sample dbuffer[period_size*2];
  while (1) {
    finyl_handle(a, b, c, d, buffer, period_size); //handle effects of each tracks
    
    err = snd_pcm_writei(handle, buffer, period_size);
    if (err == -EPIPE) {
      printf("Underrun occurred: %s\n", snd_strerror(err));
      snd_pcm_prepare(handle);
    } else if (err == -EAGAIN) {
    } else if (err < 0) {
      printf("error %s\n", snd_strerror(err));
      cleanup_alsa(handle);
      return;
    }
  }
  
  cleanup_alsa(handle);
}

