#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "finyl.h"

char* finyl_output_path = "/home/null/.finyl-output"; //TODO

#include <termios.h>

void print_track(finyl_track* t) {
  printf("\n:\n");
  printf("\tplaying: %d\n", t->playing);
  printf("\tspeed:%lf\n", t->speed);
  /* printf("\tgain:%lf\n", a0_gain); */
}

double max(double a, double b) {
  return a>b ? a : b;
}

double min(double a, double b) {
  return a<b ? a : b;
}

void slide_right(finyl_track* t) {
  double backup = t->speed;
  double y = 0;
  double x = 0;
  while (y>=0) {
    y  = -x*(x-2);
    t->speed = y;
    x = 0.01 + x;
    usleep(1000);
    printf("t is %lf\n", t->speed);
  }
  t->speed = backup;
}

void handleKey(char x) {
  switch (x) {
  case 'h':
    bdeck->playing = !bdeck->playing;
    break;
  case 'g':
    adeck->playing = !adeck->playing;
    break;
  case 'j':
    a0_gain = max(a0_gain-0.1, 0);
    break;
  case 'k':
    a0_gain = min(a0_gain+0.1, 1);
    break;
  case 'n':
    a1_gain = max(a1_gain-0.1, 0);
    break;
  case 'm':
    a1_gain = min(a1_gain+0.1, 1);
    break;
  case 'c':
    adeck->index = 0;
    break;
  case 'a':
    adeck->speed = adeck->speed + 0.01;
    break;
  case 's':
    adeck->speed = adeck->speed - 0.01;
    break;
  case 't':
    slide_right(adeck);
  }
  print_track(adeck);
  print_track(bdeck);
}

int key_input() {
  int c;   
  static struct termios oldt, newt;

  /*tcgetattr gets the parameters of the current terminal
    STDIN_FILENO will tell tcgetattr that it should write the settings
    of stdin to oldt*/
  tcgetattr( STDIN_FILENO, &oldt);
  /*now the settings will be copied*/
  newt = oldt;

  /*ICANON normally takes care that one line at a time will be processed
    that means it will return if it sees a "\n" or an EOF or an EOL*/
  newt.c_lflag &= ~(ICANON);          

  /*Those new settings will be set to STDIN
    TCSANOW tells tcsetattr to change attributes immediately. */
  tcsetattr( STDIN_FILENO, TCSANOW, &newt);

  /*This is your part:
    I choose 'e' to end input. Notice that EOF is also turned off
    in the non-canonical mode*/
  while((c=getchar())!= 'e')      
    handleKey(c);                 

  /*restore the old settings*/
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt);


  return 0;
}


int main(int argc, char **argv) {
  
  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  snd_pcm_uframes_t period_size = 1024;
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  
  finyl_track ta;
  finyl_init_track(&ta);
  adeck = &ta;
  ta.index = 44100 * 20;

  char* files_a[2];
  files_a[0] = "birth_voc.wav";
  files_a[1] = "birth_novoc.wav";
  if (finyl_read_channels_from_files(files_a, 2, &ta) == -1) {
    printf("you failed\n");
    return -1;
  }
  finyl_print_track(&ta);

  finyl_track tb;
  finyl_init_track(&tb);
  bdeck = &tb;
  tb.index = 44100 * 30;
  
  char* files_b[2];
  files_b[0] = "vocals.wav";
  files_b[1] = "no_vocals.wav";
  if (finyl_read_channels_from_files(files_b, 2, &tb) == -1) {
    printf("you failed\n");
    return -1;
  }
  finyl_print_track(&tb);
  
  pthread_t key_thread;
  if (pthread_create(&key_thread, NULL, key_input, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }


  
  finyl_run(adeck, bdeck, NULL, NULL, handle, buffer_size, period_size);

  pthread_join(key_thread, NULL);
}
