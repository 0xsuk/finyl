#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "finyl.h"

char* finyl_output_path = "/home/null/.finyl-output"; //TODO
finyl_track t;


#include <termios.h>

double max(double a, double b) {
  return a>b ? a : b;
}

double min(double a, double b) {
  return a<b ? a : b;
}

void handleKey(char x) {
  switch (x) {
  case ' ':
    t.playing = !t.playing;
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
  }
  printf("got char = %c\n", x);
  printf("a0_gain is %lf\n", a0_gain);
  printf("a1_gain is %lf\n", a1_gain);
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
  
  pthread_t key_thread;
  if (pthread_create(&key_thread, NULL, key_input, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }

  snd_pcm_t* handle;
  snd_pcm_uframes_t buffer_size = 1024 * 2;
  snd_pcm_uframes_t period_size = 1024;
  finyl_setup_alsa(&handle, &buffer_size, &period_size);
  printf("buffer_size %ld, period_size %ld\n", buffer_size, period_size);
  
  finyl_init_track(&t);

  char* files[2];
  files[0] = "vocals.wav";
  files[1] = "no_vocals.wav";
  if (finyl_read_channels_from_files(files, 2, &t) == -1) {
    printf("you failed\n");
    return -1;
  }
  
  finyl_print_track(&t);

  finyl_run(&t, NULL, NULL, NULL, handle, buffer_size, period_size);

  pthread_join(key_thread, NULL);
}
