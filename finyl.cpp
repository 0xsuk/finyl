#include "finyl.h"
#include <alsa/asoundlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/wait.h>

bool finyl_running = true;

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

void finyl_free_in_track_meta(finyl_track_meta* tm) {
  free(tm->title);
  free(tm->filepath);
  free(tm->filename);
  for (int i = 0; i<tm->channels_size; i++) {
    free(tm->channel_filepaths[i]);
  }
}

void finyl_free_track_metas(finyl_track_meta* tms, int size) {
  for (int i = 0; i<size; i++) {
    finyl_free_in_track_meta(&tms[i]);
  }
  free(tms);
}

static void free_channel(finyl_channel channel, int chunks_size) {
  for (int i = 0; i<chunks_size; i++) {
    free(channel[i]);
  }
}

static void free_channels(finyl_channel* channels, int channels_size, int chunks_size) {
  for (int i = 0; i<channels_size; i++) {
    free_channel(channels[i], chunks_size);
  }
}

void finyl_free_in_track(finyl_track* t) {
  free_channels(t->channels, t->channels_size, t->chunks_size);
  finyl_free_in_track_meta(&t->meta);
  free(t->cues);
  free(t->beats);
}

void finyl_free_track(finyl_track* t) {
  finyl_free_in_track(t);
  free(t);
}

static finyl_sample* init_channel_buffer() {
  finyl_sample* buf =  (finyl_sample*)malloc(sizeof(finyl_sample) * period_size * 2);
   if (buf == NULL) {
     printf("init_channel_buffer is NULL\n");
     return NULL;
   }
   return buf;
}

static finyl_sample**  init_channel_buffers() {
  finyl_sample** bufs = (finyl_sample**)malloc(MAX_CHANNELS_SIZE * sizeof(finyl_sample*));
  if (bufs == NULL) {
    printf("failed to init_channel_buffers\n");
    return NULL;
  }
  for (int i = 0; i<MAX_CHANNELS_SIZE;i++) {
    bufs[i] = init_channel_buffer();
  }
  return bufs;
}

void finyl_init_track_meta(finyl_track_meta* tm) {
  tm->id = 0;
  tm->bpm = 0;
  tm->musickeyid = 0;
  tm->filesize = 0;
  tm->channels_size = 0;
}

void finyl_init_track(finyl_track* t) {
  finyl_init_track_meta(&t->meta);
  t->chunks_size = 0;
  t->length = 0;
  t->index = 0;
  t->cues_size = 0;
  t->beats_size = 0;
  t->speed = 1.0;
  t->channels_size = 0;
  t->playing = false;
  t->loop_in = -1;
  t->loop_out = -1;
}

int finyl_get_quantized_beat_index(finyl_track* t, int index) {
  if (t->beats_size <= 0) {
    return -1;
  }
  
  //44100 samples = 1sec
  //index samples = 1 / 44100 * index sec
  //= 1 / 44100 * index * 1000 millisec
  double nowtime = 1000.0 / 44100.0 * index;

  if (nowtime < t->beats[0].time) {
    return 0;
  }
  if (nowtime > t->beats[t->beats_size-1].time) {
    return t->beats_size-1;
  }
  for (int i = 1; i<t->beats_size-1; i++) {
    if (t->beats[i-1].time <= nowtime && nowtime <= t->beats[i].time) {
      // [i-1]      half       [i]
      //     -margin-  -margin-
      double margin = (t->beats[i].time - t->beats[i-1].time) / 2.0;
      double half = t->beats[i-1].time + margin;
      
      if (nowtime < half) {
        return i-1;
      } else {
        return i;
      }
    }
  }

  return -1;
}

int finyl_get_quantized_time(finyl_track* t, int index) {
  int i = finyl_get_quantized_beat_index(t, index);
  if (i == -1) {
    return -1;
  }
  return t->beats[i].time;
}

finyl_sample finyl_get_sample(finyl_track* t, finyl_channel c) {
  int position = (int)t->index;
  if (position >= t->length || position < 0) {
    return 0;
  }
  int ichunk = position / CHUNK_SIZE;
  int isample = position - (CHUNK_SIZE * ichunk);

  finyl_chunk chunk = c[ichunk];
  finyl_sample sample = chunk[isample];

  return sample;
}

finyl_sample finyl_get_sample1(finyl_channel c, int position) {
  int ichunk = position / CHUNK_SIZE;
  int isample = position - (CHUNK_SIZE * ichunk);

  finyl_chunk chunk = c[ichunk];
  finyl_sample sample = chunk[isample];
  return sample;
}

static finyl_chunk make_chunk() {
  finyl_chunk chunk = (finyl_chunk)malloc(CHUNK_SIZE * sizeof(finyl_sample));
  if (chunk == NULL) {
    printf("failed to allocate chunk\n");
  }
  return chunk;
}

static finyl_channel make_channel() {
  finyl_channel c = (finyl_channel)malloc(MAX_CHUNKS_SIZE * sizeof(finyl_chunk));
  if (c == NULL) {
    printf("failed to allocate channel");
  }
  return c;
}

bool file_exist(char* file) {
  if (access(file, F_OK) == -1) {
    return false;
  }

  return true;
}

static int open_pcm_stream(FILE** fp, char* filename) {
  if (!file_exist(filename)) {
    printf("File does not exist: %s\n", filename);
    return -1;
  }
  char command[1000];
  snprintf(command, sizeof(command), "ffmpeg -i \"%s\" -f s16le -ar 44100 -ac 1 -v quiet -", filename);
  *fp = popen(command, "r");
  if (*fp == NULL) {
    printf("failed to open stream for %s\n", filename);
    return -1;
  }

  return 0;
}

//reads from fp, updates chunks_size, length, channel
static int read_pcm(FILE* fp, finyl_channel channel, int* chunks_size, int* length) {
  while (1) {
    finyl_chunk chunk = make_chunk();
    if (chunk == NULL) {
      printf("read_pcm failed to allocate chunk\n");
      return -1;
    }
    
    size_t count = fread(chunk, sizeof(finyl_sample), CHUNK_SIZE, fp);
    channel[*chunks_size] = chunk;
    (*chunks_size)++;
    *length += count;
    if (count < CHUNK_SIZE) {
      return 0;
    }
    if (*chunks_size == MAX_CHUNKS_SIZE) {
      printf("File is too large\n");
      return 1;
    }
  }
}

static int read_channel(char* file, finyl_channel channel, int* chunks_size, int* length) {
  FILE* fp = NULL;
  if (open_pcm_stream(&fp, file) == -1)  {
    return -1;
  }
  if (fp == NULL) {
    return -1;
  }
  int status = read_pcm(fp, channel, chunks_size, length);
  if (status == -1) {
    return -1;
  } else if (status == 1) {
    return 1;
  }
  
  pclose(fp);
  return 0;
}

//each file corresponding to each channel
int finyl_read_channels_from_files(char** files, int channels_size, finyl_track* t) {
  for (int i = 0; i < channels_size; i++) {
    int chunks_size = 0;
    int length = 0;
    finyl_channel channel = make_channel();
    if (channel == NULL) {
      printf("failed to allocate memory in finyl_read_channels_from_files\n");
      free_channels(t->channels, t->channels_size, t->chunks_size);
      return -1;
    }
    int status = read_channel(files[i], channel, &chunks_size, &length);
    if (status == -1 || status == 1) {
      free_channel(channel, chunks_size);
      free_channels(t->channels, t->channels_size, t->chunks_size);
      return -1;
    }
    
    if (i == 0) {
      t->length = length;
      t->chunks_size = chunks_size;
    } else {
      if (length != t->length || chunks_size != t->chunks_size) {
        printf("all channels have to be the same length\n");
        free_channel(channel, chunks_size);
        free_channels(t->channels, t->channels_size, t->chunks_size);
        return -1;
      }
    }

    t->channels[i] = channel;
    t->channels_size++;
  }
  
  return 0;
}
//TODO: one file has amny channels (eg. flac)
void read_channels_from_file(char* file);

static void gain_filter(finyl_sample* buf, double gain) {
  for (int i = 0; i<period_size*2;i=i+2) {
    buf[i] = gain * buf[i];
    buf[i+1] = gain * buf[i+1];
  }
}

static void make_channel_buffers(finyl_sample** channel_buffers, finyl_track* t) {
  for (int i = 0; i < period_size*2; i=i+2) {
    t->index += t->speed;

    if (t->loop_in != -1 && t->loop_out != -1 && t->index >= (t->loop_out - 1000)) {
      t->index = t->loop_in + t->index - t->loop_out;
    }
    
    if (t->index >= t->length) {
      t->playing = false;
    }

    for (int c = 0; c<t->channels_size; c++) {
      finyl_sample* buf = channel_buffers[c];
      buf[i] = finyl_get_sample(t, t->channels[c]);
      buf[i+1] = buf[i];
    }
  }
}

static int32_t clip_sample(int32_t s) {
  if (s > 32767) {
    s = 32767;
  } else if (s < -32768) {
    s = -32768;
  }
  return s;
}

static void add_and_clip_two_buffers(finyl_sample* dest, finyl_sample* src1, finyl_sample* src2) {
  for (int i = 0; i<period_size*2; i++) {
    int32_t sample = src1[i] + src2[i];
    dest[i] = clip_sample(sample);
  }
}

double a_gain = 1.0;
double a0_gain = 0.0;
double a1_gain = 0.0;
double b_gain = 1.0;
double b0_gain = 0.0;
double b1_gain = 0.0;
double a0_filter = 0.2;
double a1_filter = 1.0;
double b0_filter = 0.2;
double b1_filter = 1.0;

static void finyl_handle() {
  
  memset(abuffer, 0, period_size*2*sizeof(finyl_sample));
  memset(bbuffer, 0, period_size*2*sizeof(finyl_sample));
  
  if (adeck != NULL && adeck->playing) {
    make_channel_buffers(a_channel_buffers, adeck);
    gain_filter(a_channel_buffers[0], a0_gain);
    gain_filter(a_channel_buffers[1], a1_gain);
    add_and_clip_two_buffers(abuffer, a_channel_buffers[0], a_channel_buffers[1]);
  }
  
  if (bdeck != NULL && bdeck->playing) {
    make_channel_buffers(b_channel_buffers, bdeck);
    gain_filter(b_channel_buffers[0], b0_gain);
    gain_filter(b_channel_buffers[1], b1_gain);
    add_and_clip_two_buffers(bbuffer, b_channel_buffers[0], b_channel_buffers[1]);
  }
  
  add_and_clip_two_buffers(buffer, abuffer, bbuffer);
}

/* char* device = "hw:CARD=PCH,DEV=0";            /\* playback device *\/ */
char* device = "default";

static void setup_alsa_params(snd_pcm_t* handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size) {
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

static void cleanup_alsa(snd_pcm_t* handle) {
  int err = snd_pcm_drain(handle);
  if (err < 0) {
    printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
  }
  snd_pcm_close(handle);
  printf("alsa closed\n");
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

static void init_buffers() {
  int s = sizeof(finyl_sample) * period_size * 2;
  
  buffer = (finyl_sample*)malloc(s);
  
  abuffer = (finyl_sample*)malloc(s);
  a_channel_buffers = init_channel_buffers();
  bbuffer = (finyl_sample*)malloc(s);
  b_channel_buffers = init_channel_buffers();


  finyl_sample* buf0 = a_channel_buffers[0];
  memset(buf0, 0, period_size*2*sizeof(finyl_sample));
  finyl_sample* buf1 = a_channel_buffers[0];
  memset(buf1, 0, period_size*2*sizeof(finyl_sample));
}

static void init_decks(finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d) {
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
  
  while (finyl_running) {
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
      finyl_running = false;
      return;
    }
  }
  
  cleanup_alsa(handle);
}

