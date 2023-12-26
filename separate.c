//LIMITATIONS:
//demucs
//mp3 only
//two-stems vocals


#include "finyl.h"
#include "util.h"
#include "dev.h"
#include "digger.h"
#include <stdlib.h>
#include <libgen.h>

#define same(x,y) (strcmp(x, y) == 0)

void get_filename(int* dot, int* baselen, char* dst, char* filepath) {
  char filepath_cpy[strlen(filepath)+1];
  strcpy(filepath_cpy, filepath);
  char* base = basename(filepath_cpy);
  printf("base %s\n", base);
  *dot = find_char_last(base, '.');
  ncpy(dst, base, *dot);

  *baselen = strlen(base);
}

void gen_channel_filepath(char* dst, char* filepath, char* root, char* stem, char* md5) {
  char filename[strlen(filepath)+100];
  int dot;
  int baselen;
  get_filename(&dot, &baselen, filename, filepath);
  int til_filename = strlen(filepath) - baselen;
  
  int len = strlen(filename);
  filename[len] = '-';
  filename[len+1] = '\0';

  strcat(filename, stem);
  strcat(filename, "-");
  strcat(filename, md5);
  /* strcat(filename, &filepath[strlen(filepath) - baselen + dot]); //add extension */
  strcat(filename, ".mp3");

  join_path(dst, root, filename);
}

bool all_channel_files_exist(char* filepath, char* root, char* md5) {
  //TODO vocal no_vocal
  {
    char dst[1000];
    gen_channel_filepath(dst, filepath, root, "vocals", md5);
    printf("dst is %s\n", dst);
    if (!file_exist(dst)) {
      return false;
    }
  }
  {
    char dst[1000];
    gen_channel_filepath(dst, filepath, root, "no_vocals", md5);
    printf("dst is %s\n", dst);
    if (!file_exist(dst)) {
      return false;
    }
  }
  return true;
}

void separate_track(finyl_track_meta* tm) {
  char command[1000];
  char model[] = "hdemucs_mmi";
  char root[1000];
  join_path(root, usb, "finyl/separated");
  char neoroot[1000];
  join_path(neoroot, root, model);
  
  char md5[33];
  compute_md5(tm->filepath, md5);
  snprintf(command, sizeof(command), "demucs -n %s --two-stems=vocals \"%s\" -o %s --filename {track}-{stem}-%s.{ext} --mp3", model, tm->filepath, root, md5);
  printf("\nRunning:\n");
  printf("\t%s\n", command);
  
  if (all_channel_files_exist(tm->filepath, neoroot, md5)) {
    printf("skipping\n");
    return;
  }
  
  FILE* fp = popen(command, "r");
  if (fp == NULL) {
    printf("\n[error] failed to open command\n");
    return;
  }
  char buffer[1024];
  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    printf("%s", buffer);
  }
  if (close_command(fp) == -1) {
    return;
  }
}

void demucs_track(char* tid) {
  int _tid = atoi(tid);
  finyl_track t;
  if (get_track(&t, usb, _tid) == -1) {
    printf("abort\n");
    return;
  }

  separate_track(&t.meta);
  finyl_free_in_track(&t);
}

void demucs_playlists(char* pid) {
  finyl_track_meta* tms;
  int _pid = atoi(pid);
  int tms_size = get_playlist_tracks(&tms, usb, _pid);
  if (tms_size == -1) {
    return;
  }
  
  for (int i = 0; i<tms_size; i++) {
    separate_track(&tms[i]);
  }
    
  finyl_free_track_metas(tms, tms_size);
}

void print_usage() {
  printf("usage: ./separate <usb> <operation>\n\n");
  printf("operation:\n");
  printf("\tlist-playlists: list playlists\n");
  printf("\tlist-playlist-tracks <id>: list tracks in playlist <id>\n");
  printf("\tdemucs-playlist <id>: run demucs for playlist <id>\n");
  printf("\tdemucs-track <id>: run demucs for track <id>\n");
}

int main(int argc, char **argv) {
  char* filepath = "/media/null/71CD-A534/Contents/あいみょん/愛を伝えたいだとか - EP/01 愛を伝えたいだとか.m4a";

  char md5[33];
  compute_md5(filepath, md5);
  
  bool is = all_channel_files_exist(filepath, "/media/null/71CD-A534/finyl/separated/hdemucs_mmi/", md5);
  printf("is %d\n", is);

  if (argc < 3) {
    print_usage();
    return 0;
  }
  
  usb = argv[1];
  
  char* op = argv[2];

  if (same(op, "list-playlists")) {
    list_playlists();
  } else if (same(op, "list-playlist-tracks")) {
    if (argc < 4) {
      print_usage();
      return 0;
    }

    int id = atoi(argv[3]);
    list_playlist_tracks(id);
  } else if (same(op, "demucs-playlist")) {
    if (argc < 4) {
      print_usage();
      return 0;
    }
    char* id = argv[3];
    demucs_playlists(id);
  } else if (same(op, "demucs-track")) {
    if (argc < 4) {
      print_usage();
      return 0;
    }

    char* id = argv[3];
    demucs_track(id);
  } else {
    print_usage();
  }
}
