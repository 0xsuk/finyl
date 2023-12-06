#include "finyl_dev.h"
#include <alsa/asoundlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/wait.h>

finyl_track* adeck; //pointer to track, is a d
finyl_track* bdeck;
finyl_track* cdeck;
finyl_track* ddeck;

finyl_sample* buffer;
finyl_sample* abuffer;
finyl_sample** a_channel_buffers;
finyl_sample* bbuffer;
finyl_sample** b_channel_buffers;

snd_pcm_uframes_t period_size;

finyl_sample* init_channel_buffer() {
   return malloc(sizeof(finyl_sample) * period_size * 2);
}

finyl_sample**  init_channel_buffers() {
  finyl_sample** bufs = malloc(MAX_CHANNELS_SIZE * sizeof(finyl_sample*));
  for (int i = 0; i<MAX_CHANNELS_SIZE;i++) {
    bufs[i] = init_channel_buffer();
  }
  return bufs;
}

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
  t->speed = 1.0;
  t->channels_size = 0;
  t->playing = false;
}

void finyl_print_track(finyl_track* t) {
  printf("track {\n");
  printf("\tnchunks: %d\n", t->nchunks);
  printf("\tlength: %d\n", t->length);
  printf("\tindex: %lf\n", t->index);
  printf("\tspeed: %lf\n", t->speed);
  printf("\tchannels_size: %d\n", t->channels_size);
  printf("}\n");
}

finyl_sample finyl_get_sample(finyl_track* t, finyl_channel c) {
  int position = floor(t->index);
  int ichunk = position / chunk_size;
  int isample = position - (chunk_size * ichunk);

  finyl_chunk chunk = c[ichunk];
  finyl_sample sample = chunk[isample];
  return sample;
}

finyl_sample average_channels(finyl_track* t, int size) {
  finyl_sample s = 0;
  
  for (int i = 0; i<size; i++) {
    s += finyl_get_sample(t, t->channels[i]);
  }

  s = s/size;
  
  return s;
}

finyl_sample add_channels(finyl_track* t, int size) {
  finyl_sample s = 0;
  
  for (int i = 0; i<size; i++) {
    s += finyl_get_sample(t, t->channels[i]);
  }
  return s;
}

finyl_chunk make_chunk() {
  return malloc(chunk_size * sizeof(finyl_sample));
}

void free_chunks(finyl_channel channel, int channel_size) {
  for (int i = 0; i<channel_size;i++) {
    finyl_chunk chunk = channel[i];
    free(chunk);
  }
}

finyl_channel make_channel() {
  return malloc(chunks_size_max * sizeof(finyl_channel*));
}

void free_channels(finyl_channel* channels, int channels_size) {
  for (int i = 0; i<channels_size; i++) {
    finyl_channel channel = channels[i];
    free(channel);
  }
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
  finyl_chunk chunk = make_chunk();
  while (1) {
    size_t count = fread(chunk, sizeof(finyl_sample), chunk_size, fp);
    channel[*nchunks] = chunk;
    (*nchunks)++;
    *length += count;
    if (count < chunk_size) {
      return 0;
    }
    if (*nchunks == chunks_size_max) {
      printf("File is too large\n");
      free_chunks(channel, *nchunks);
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
int finyl_read_channels_from_files(char** files, int channels_size, finyl_track* t) {
  for (int i = 0; i < channels_size; i++) {
    int nchunks = 0;
    int length = 0;
    finyl_channel channel = make_channel();
    if (channel == NULL) {
      printf("failed to allocate memory in finyl_read_channels_from_files\n");
      return -1;
    }
    if (read_channel(files[i], channel, &nchunks, &length) == -1) {
      //free all channels
      free(channel);
      free_channels(t->channels, t->channels_size);
      return -1;
    }
    if (i == 0) {
      t->length = length;
      t->nchunks = nchunks;
    } else {
      if (length != t->length || nchunks != t->nchunks) {
        printf("all channels have to be the same length\n");
        free(channel);
        free_channels(t->channels, t->channels_size);
        return -1;
      }
    }

    t->channels[i] = channel;
    t->channels_size++;
  }
  
  return 0;
}
//one file has amny channels (eg. flac)
void read_channels_from_file(char* file);

void gain_filter(finyl_sample* buf, double gain) {
  for (int i = 0; i<period_size*2;i=i+2) {
    buf[i] = gain * buf[i];
    buf[i+1] = gain * buf[i+1];
  }
}

void make_channel_buffer(finyl_sample* buf, finyl_track* t, finyl_channel channel) {
  for (int i = 0; i < period_size*2; i=i+2) {
    t->index += t->speed;

    if (t->index >= t->length) {
      t->playing = false;
      buf[i] = 0;
      buf[i+1] = 0;
      continue;
    }

    buf[i] = finyl_get_sample(t, channel);
    buf[i+1] = buf[i];
  }
}

void sum_channel_buffers(finyl_sample* track_buffer, finyl_sample** channel_buffers, int channels_size) {
  for (int c = 0; c<channels_size;c++) {
    finyl_sample* channel_buffer = channel_buffers[c];
    for (int i = 0; i<period_size*2;i=i+2) {
      track_buffer[i] += channel_buffer[i];
      track_buffer[i+1] += channel_buffer[i+1];
    }
  }
}

double a0_gain = 1.0;
double a1_gain = 1.0;

void finyl_handle() {
  if (!adeck->playing) {
    memset(buffer, 0, period_size*2*sizeof(finyl_sample));
    return;
  }
  make_channel_buffer(a_channel_buffers[0], adeck, adeck->channels[0]);
  make_channel_buffer(a_channel_buffers[1], adeck, adeck->channels[1]);
  
  /* gain_filter(a_channel_buffers[0], a0_gain); */
  /* gain_filter(a_channel_buffers[1], a1_gain); */
  sum_channel_buffers(abuffer, a_channel_buffers, adeck->channels_size);
  /* for (int i = 0; i<a_callbacks_size; i++) { */
    /* finyl_track_callback cb = a_callbacks[i]; */
    /* cb(period_size, abuffer, adeck); */
  /* } */
  
  /* for (int i = 0; i<period_size*2; i=i+2) { */
    /* buffer[i] = abuffer[i]; */
    /* buffer[i+1] = abuffer[i+1]; */
  /* } */
  
  /* for (int i = 0; i<master_callbacks_size; i++) { */
    /* finyl_master_callback cb = master_callbacks[i]; */
    
    /* cb(period_size, buffer, adeck, bdeck, cdeck, ddeck); */
  /* } */
}

/* char* device = "hw:CARD=PCH,DEV=0";            /\* playback device *\/ */
char* device = "default";

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

void init_buffers() {
  int s = sizeof(finyl_sample) * period_size * 2;
  
  buffer = (finyl_sample*)malloc(s);
  
  abuffer = (finyl_sample*)malloc(s);
  a_channel_buffers = init_channel_buffers();
  bbuffer = (finyl_sample*)malloc(s);
  b_channel_buffers = init_channel_buffers();
}

void init_decks(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d) {
  adeck = a;
  bdeck = b;
  cdeck = c;
  ddeck = d;
}

void finyl_run(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d, snd_pcm_t* handle, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t _period_size) {
  int err = 0;
  period_size = _period_size;
  init_decks(a, b, c, d);
  init_buffers();
  
  while (1) {
    finyl_handle(); //handle effects of each tracks
    
    //handle channels
    //handle tracks
    //handle master
    
    err = snd_pcm_writei(handle, buffer, period_size);
    if (err == -EPIPE) {
      printf("Underrun occurred: %s\n", snd_strerror(err));
      snd_pcm_prepare(handle);
    } else if (err == -EAGAIN) {
      printf("eagain\n");
    } else if (err < 0) {
      printf("error %s\n", snd_strerror(err));
      cleanup_alsa(handle);
      return;
    }
  }
  
  cleanup_alsa(handle);
}

