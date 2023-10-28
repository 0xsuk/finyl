// Do complicated pointer business for lisp

#include <alsa/asoundlib.h>
#include <math.h>
#include <stdbool.h>

/* sysdefault:CARD=XDJRX */
/* char* device = "sysdefault:CARD=PCH"; */
char* device = "plughw:CARD=PCH,DEV=0";            /* playback device */

#define maximum_chunks  32
#define chunk_size 2097152 //(2048 * 1024);

typedef struct {
  unsigned short* chunks[maximum_chunks];
  int nchunks;
  int length;
  double index;
  double speed;
} track;

track left_track;
track right_track;

void init_track(track* t) {
  t->nchunks = 0;
  t->length = 0;
  t->index = 0;
  t->speed = 0;
}

void _print_track(track* t) {
  printf("track {\n");
  printf("\tnchunks: %d\n", t->nchunks);
  printf("\tlength: %d\n", t->length);
  printf("\tindex: %lf\n", t->index);
  printf("\tspeed: %lf\n", t->speed);
  printf("}\n");
}

unsigned short get_sample(track* t) {
  int position = floor(t->index);
  int ichunk = position / chunk_size;
  int isample = position - (chunk_size * ichunk);
  unsigned short* chunk = t->chunks[ichunk];
  return chunk[isample];
}

void reset_index(track* t) {
  t->index = 0;
}

unsigned short* make_chunk() {
  unsigned short* chunk = malloc(chunk_size * sizeof(unsigned short));
  if (chunk == NULL) return NULL;
  return chunk;
}

FILE* open_pcm_stream(char* filename) {
  char command[256];
  snprintf(command, sizeof(command), "ffmpeg -i %s -f u16le -ar 44100 -ac 1 -", filename);
  FILE* fp = popen(command, "r");
  if (fp == NULL) {
    printf("popen failed\n");
    exit(1);
  }

  return fp;
}

void read_pcm(FILE* fp, track* t) {
  unsigned short* chunk = make_chunk();
  while (1) {
    unsigned long count = fread(chunk, sizeof(unsigned short), chunk_size, fp);
    t->chunks[t->nchunks] = chunk;
    t->nchunks++;
    t->length += count;
    if (count < chunk_size) {
      break;
    }
    if (t->nchunks == maximum_chunks) {
      printf("File is too large");
      break;
    }
    chunk = make_chunk();
    /* fwrite(path, 1, 1, stdout); */
  }
  
  _print_track(t);
}

void read_file(char* filename, track* t) {
  FILE* fp = open_pcm_stream(filename);
  read_pcm(fp, t);
  pclose(fp);
}

void setup_alsa_params(snd_pcm_t* handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size) {
  snd_pcm_hw_params_t* hw_params;
  snd_pcm_hw_params_malloc(&hw_params);
  snd_pcm_hw_params_any(handle, hw_params);
  snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_U16_LE);
  snd_pcm_hw_params_set_rate(handle, hw_params, 44100, 0);
  snd_pcm_hw_params_set_channels(handle, hw_params, 2); // each channel for each track
  snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, buffer_size);
  snd_pcm_hw_params_set_period_size_near(handle, hw_params, period_size, 0);
  snd_pcm_hw_params(handle, hw_params);
  snd_pcm_hw_params_free(hw_params);
  snd_pcm_get_params(handle, buffer_size, period_size);
}

void run(track* t, snd_pcm_t* handle, snd_pcm_uframes_t period_size) {
  t->speed = 1;
  int err = 0;
  unsigned short buffer[period_size*2];
  while (1) {
    for (unsigned int i = 0; i < period_size*2; i=i+2) {
      buffer[i] = 0;
      buffer[i+1] = get_sample(t);
      t->index = t->index + t->speed;
      if (t->index >= t->length) {
        reset_index(t);
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
}

void setup_alsa(snd_pcm_t** handle, snd_pcm_uframes_t* buffer_size, snd_pcm_uframes_t* period_size) {
  int err;
  if ((err = snd_pcm_open(handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error!: %s\n", snd_strerror(err));
    exit(1);
  }
  setup_alsa_params(*handle, buffer_size, period_size);
  printf("buffer_size %ld, period_size %ld\n", *buffer_size, *period_size);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: ./final filename\n");
    exit(0);
  }
  track t;
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 512;
  snd_pcm_uframes_t period_size = 256;
  setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  init_track(&t);
  read_file(argv[1], &t);
  run(&t, handle, period_size);
  return 0;
}
