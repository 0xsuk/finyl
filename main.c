// Do complicated pointer business for lisp

#include <alsa/asoundlib.h>
#include <math.h>
#include <stdbool.h>
#include <pigpio.h>

/* sysdefault:CARD=XDJRX */
/* char* device = "sysdefault:CARD=PCH"; */
char* device = "hw:CARD=Headphones,DEV=0";            /* playback device */

#define maximum_chunks  32
#define chunk_size 2097152 //(2048 * 1024);
#define AS5600_I2C_ADDR 0x36

//angle is 0 to 4095
//duration delta is the same as period of snd_pcm_writei, meaning duration between each snd_pcm_writei is the duration delta
//Song plays at the normal speed, when record spins at the normal speed. (Note record's normal spin speed is irrelevant to the original bpm of the song)
/* int d_std_angle = 10; //for duration delta, angle change is normally 10. If so, song plays at the original speed. If delta angle is less than d_std_angle, song is playing slow */
double samples_per_angle = 10; // number of samples that corresponds to 1 angle, when record spins at the normal speed. 22 = 44100 / 2000, angle change per sec is 2000
int left_sensor = 0;
int left_angle = 0;
int right_sensor = 0;

typedef struct {
  signed short* chunks[maximum_chunks];
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

signed short get_sample(track* t) {
  int position = floor(t->index);
  int ichunk = position / chunk_size;
  int isample = position - (chunk_size * ichunk);
  signed short* chunk = t->chunks[ichunk];
  return chunk[isample];
}

void reset_index(track* t) {
  t->index = 0;
}

signed short* make_chunk() {
  signed short* chunk = malloc(chunk_size * sizeof(signed short));
  if (chunk == NULL) return NULL;
  return chunk;
}

FILE* open_pcm_stream(char* filename) {
  char command[256];
  snprintf(command, sizeof(command), "ffmpeg -i %s -f s16le -ar 44100 -ac 1 -", filename);
  FILE* fp = popen(command, "r");
  if (fp == NULL) {
    printf("popen failed\n");
    exit(1);
  }

  return fp;
}

void read_pcm(FILE* fp, track* t) {
  signed short* chunk = make_chunk();
  while (1) {
    signed long count = fread(chunk, sizeof(signed short), chunk_size, fp);
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
  snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_rate(handle, hw_params, 44100, 0);
  snd_pcm_hw_params_set_channels(handle, hw_params, 2); // each channel for each track
  snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, buffer_size);
  snd_pcm_hw_params_set_period_size_near(handle, hw_params, period_size, 0);
  snd_pcm_hw_params(handle, hw_params);
  snd_pcm_hw_params_free(hw_params);
  snd_pcm_get_params(handle, buffer_size, period_size);
}

int read_angle() {
  int high = i2cReadByteData(left_sensor, 0x0E);
  int low = i2cReadByteData(left_sensor, 0x0F);
  if (high < 0 || low < 0) {
    fprintf(stderr, "Failed to read from as5600\n");
    return -1;
  }
  int angle = (high << 8) | low; // 0 to 4095
  printf("angle %d\n", angle);
  return angle;
}

void run(track* t, snd_pcm_t* handle, snd_pcm_uframes_t period_size) {
  t->speed = 1;
  int err = 0;
  signed short buffer[period_size*2];
  while (1) {
    int angle = read_angle();
    if (angle < 0) {
      break;
    }
    int delta_angle = angle - left_angle;
    if (delta_angle > 2048) {
      //if angle is like 4095 and left_angle = 0
      delta_angle = delta_angle - 4096;
    }
    if (delta_angle < -2048) {
      //if angle is like 0 and left_angle = 4095
      delta_angle = delta_angle + 4096;
    }
    left_angle = angle;
    double nsamples = samples_per_angle * delta_angle;
    double delta_index = nsamples / period_size;
    
    printf("delta_index = %.10f\n", delta_index);
    
    for (signed int i = 0; i < period_size*2; i=i+2) {
      buffer[i] = 0;
      buffer[i+1] = get_sample(t);
      t->index = t->index + delta_index;
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

void init_sensor() {
  if (gpioInitialise() < 0) {
    fprintf(stderr, "Failed to initialize pigpio\n");
    exit(1);
  }
  left_sensor = i2cOpen(1, AS5600_I2C_ADDR, 0);

  if (left_sensor < 0) {
    fprintf(stderr, "Failed to open i2c connection\n");
    exit(1);
  }
}

int main(int argc, char** argv) {
  init_sensor();
  if (argc != 2) {
    printf("Usage: ./final filename\n");
    exit(0);
  }
  track t;
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  snd_pcm_uframes_t period_size = 1024;
  setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  init_track(&t);
  read_file(argv[1], &t);
  run(&t, handle, period_size);
  return 0;
}
