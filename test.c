#include <stdio.h>
#include "finyl.h"
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>


char* usb = "/media/null/22BC-F655/";
/* sysdefault:CARD=XDJRX */
/* char* device = "sysdefault:CARD=PCH"; */

void* thread(finyl_track* t) {
  /* sleep(5); */
  printf("testing thread\n");
  /* t->playing = false; */
  pthread_exit("ret");
}

/* void vol(unsigned long period_size, finyl_sample* buffer, finyl_track* a, finyl_track* b, finyl_track* c, finyl_track* d) { */
  /* for (int i = 0; i<period_size*2; i=i+2) { */
    /* buffer[i] = buffer[i]; */
    /* buffer[i] = buffer[i]; */
  /* } */
/* } */

int main() {
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  snd_pcm_uframes_t period_size = 1024;
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  
  finyl_track t;
  finyl_init_track(&t);

  /* int a_cbs_size = 1; */
  /* finyl_track_callback a_cbs[1]; */
  /* a_cbs[0] = finyl_track_callback_play; */
  /* finyl_set_track_callbacks(a_cbs, a_cbs_size, finyl_a); */
  
  /* int master_cbs_size = 1; */
  /* finyl_master_callback master_cbs[1]; */
  /* master_cbs[0] = vol; */
  /* finyl_set_master_callbacks(master_cbs, master_cbs_size); */
  
  char* files[2];
  files[0] = "vocals.wav";
  files[1] = "no_vocals.wav";
  if (finyl_read_channels_from_files(files, 2, &t) == -1) {
    printf("you failed\n");
    return -1;
  }
  
  finyl_print_track(&t);
  
  t.playing = true;
  finyl_run(&t, NULL, NULL, NULL, handle, buffer_size, period_size);

}



/* finyl_channel_callback* a0_callbacks; */
/* int a0_callbacks_size; */
/* finyl_track_callback* a_callbacks; */
/* int a_callbacks_size; */
/* finyl_master_callback* master_callbacks; */
/* int master_callbacks_size; */

/* int finyl_set_channel_callbacks(finyl_channel_callback* cbs, int size, finyl_track_target tt, finyl_channel_target ct) { */
/*   if (tt == finyl_a) { */
/*     if (ct == finyl_0) { */
/*       a0_callbacks = cbs; */
/*       a0_callbacks_size = size; */
/*     } */
/*   } */
/*   return 0; */
/* } */

/* int finyl_set_track_callbacks(finyl_track_callback* cbs, int size, finyl_track_target tt) { */
/*   if (tt == finyl_a) { */
/*     a_callbacks = cbs; */
/*     a_callbacks_size = size; */
/*   } */
/*   return 0; */
/* } */

/* int finyl_set_master_callbacks(finyl_master_callback* cbs, int size) { */
/*   master_callbacks = cbs; */
/*   master_callbacks_size = size; */
/*   return 0; */
/* } */

/* header */
/* typedef void (*finyl_channel_callback)(unsigned long period_size, finyl_sample* buffer, finyl_track* t, int channel_index); */

/* typedef void (*finyl_track_callback)(unsigned long period_size, finyl_sample* buffer, finyl_track* t); */

/* typedef void (*finyl_master_callback)(unsigned long period_size, finyl_sample* buffer, finyl_track* a, finyl_track* b, finyl_track*c, finyl_track* d); */

/* int finyl_set_channel_callbacks(finyl_channel_callback* cbs, int size, finyl_track_target tt, finyl_channel_target ct); */

/* int finyl_set_track_callbacks(finyl_track_callback* cbs, int size, finyl_track_target tt); */

/* int finyl_set_master_callbacks(finyl_master_callback* cbs, int size); */

