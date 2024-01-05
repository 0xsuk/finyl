#include "finyl.h"
#include "digger.h"
#include "cJSON.h"
#include "dev.h"
#include <dirent.h>
#include "util.h"

void list_playlists() {
  finyl_playlist* pls;
  int size = get_playlists(&pls, usb);

  if (size == -1) {
    return;
  }
  
  printf("\n");
  for (int i = 0; i<size; i++) {
    printf("%d %s\n", pls[i].id, pls[i].name);

  }

  free_playlists(pls, size);
}

void list_playlist_tracks(int pid) {
  finyl_track_meta* tms;
  int size = get_playlist_tracks(&tms, usb, pid);

  if (size == -1) {
    return;
  }
  
  printf("\n");
  for (int i = 0; i<size; i++) {
    printf("%d %d %d %s\n", tms[i].id, tms[i].bpm, tms[i].channels_size, tms[i].title);
  }

  finyl_free_track_metas(tms, size);
}

bool match(char* hash, char* filename) {
  int dot = find_char_last(filename, '.');
  if (dot == -1) {
    return false;
  }
  char f[dot+1];
  strncpy(f, filename, dot);
  f[dot] = '\0';

  int under = find_char_last(f, '-');
  if (under == -1) {
    return false;
  }
  
  if (dot - under == 33) {
    if (strncmp(hash, &filename[under+1], 32) == 0) {
      return true;
    }
  }
  return false;
}

void set_channels_filepaths(finyl_track_meta* tm, char* root) {
  char hash[32];
  compute_md5(tm->filepath, hash);

  DIR* d = opendir(root);
  struct dirent* dir;
  if (d) {
    int channels_size = 0;
    while ((dir = readdir(d)) != NULL) {
      if (match(hash, dir->d_name)) {
        int offset = 0;
        if (root[strlen(root)] != '/') {
          offset = 1;
        }
        int flen = strlen(root) + strlen(dir->d_name) + offset;
        char* f = (char*)malloc(flen + 1);
        join_path(f, root, dir->d_name);
        f[flen] = '\0';
        tm->channel_filepaths[channels_size] = f;
        channels_size++;
        tm->channels_size = channels_size;
      }
    }
    closedir(d);
  }
}

void free_playlists(finyl_playlist* pls, int size) {
  for (int i = 0; i<size; i++) {
    free(pls[i].name);
  }

  free(pls);
}

static int unmarshal_playlist(cJSON* pj, finyl_playlist* p) {
  cJSON_malloc_cpy(pj, "name", &p->name);
  p->id = cJSON_GetObjectItem(pj, "id")->valueint;

  return 0;
}

//returns size, -1 for error;
static int make_playlists(cJSON* json, finyl_playlist** pls) {
  cJSON* playlistsj = cJSON_GetObjectItem(json, "playlists");
  int playlists_size = cJSON_GetArraySize(playlistsj);
  *pls = (finyl_playlist*)malloc(sizeof(finyl_playlist) * playlists_size);

  finyl_playlist* _pls = *pls;
  
  for (int i = 0; i<playlists_size; i++) {
    cJSON* pj = cJSON_GetArrayItem(playlistsj, i);
    unmarshal_playlist(pj, &_pls[i]);
  }

  return playlists_size;
}

static int unmarshal_track_meta(cJSON* metaj, finyl_track_meta* tm) {
  tm->id = cJSON_GetObjectItem(metaj, "id")->valueint;
  tm->musickeyid = cJSON_GetObjectItem(metaj, "musickeyid")->valueint;
  tm->bpm = cJSON_GetObjectItem(metaj, "tempo")->valueint;
  tm->filesize = cJSON_GetObjectItem(metaj, "filesize")->valueint;
    
  cJSON_malloc_cpy(metaj, "title", &tm->title);
  cJSON_malloc_cpy(metaj, "filepath", &tm->filepath);
  cJSON_malloc_cpy(metaj, "filename", &tm->filename);
  
  return 0;
}

static int make_track_meta(cJSON* metaj, finyl_track_meta* tm) {
  finyl_init_track_meta(tm);
  unmarshal_track_meta(metaj, tm);
  return 0;
}

static int make_track_metas(cJSON* tracksj, finyl_track_meta** tms) {
  int tms_size = cJSON_GetArraySize(tracksj);
  *tms = (finyl_track_meta*)malloc(sizeof(finyl_track_meta) * tms_size);
  finyl_track_meta* _tms = *tms;

  for (int i = 0; i<tms_size; i++) {
    cJSON* metaj = cJSON_GetArrayItem(tracksj, i);
    make_track_meta(metaj, &_tms[i]);
  }

  return tms_size;
}

static int make_playlist_tracks(cJSON* json, finyl_track_meta** tms) {
  cJSON* tracksj = cJSON_GetObjectItem(json, "tracks");
  int tms_size = make_track_metas(tracksj, tms);
  return tms_size;
}

static int unmarshal_cue(cJSON* cuej, finyl_cue* cue) {
  cJSON* typej = cJSON_GetObjectItem(cuej, "type");
  cue->type = typej->valueint;
  cJSON* timej = cJSON_GetObjectItem(cuej, "time");
  cue->time = timej->valueint;

  return 0;
}

static int make_cues(cJSON* trackj, std::vector<finyl_cue>* cues) {
  cJSON* cuesj = cJSON_GetObjectItem(trackj, "cues");
  int cues_size = cJSON_GetArraySize(cuesj);
  
  cues->resize(cues_size);
  
  for (int i = 0; i<cues_size; i++) {
    cJSON* cuej = cJSON_GetArrayItem(cuesj, i);
    unmarshal_cue(cuej, &((*cues)[i]));
  }

  return cues_size;
}

static int unmarshal_beat(cJSON* beatj, finyl_beat* beat) {
  cJSON* timej = cJSON_GetObjectItem(beatj, "time");
  beat->time = timej->valueint;
  cJSON* numberj = cJSON_GetObjectItem(beatj, "num");
  beat->number = numberj->valueint;

  return 0;
}

static int make_beats(cJSON* trackj, std::vector<finyl_beat>* beats) {
  cJSON* beatsj = cJSON_GetObjectItem(trackj, "beats");
  int beats_size = cJSON_GetArraySize(beatsj);
  
  beats->resize(beats_size);
  
  for (int i = 0; i<beats_size; i++) {
    cJSON* beatj = cJSON_GetArrayItem(beatsj, i);
    unmarshal_beat(beatj, &(*beats)[i]);
  }

  return beats_size;
}

static int unmarshal_track(cJSON* json, finyl_track* t) {
  cJSON* trackj = cJSON_GetObjectItem(json, "track");
  cJSON* metaj = cJSON_GetObjectItem(trackj, "t"); //meta

  unmarshal_track_meta(metaj, &t->meta);
  make_cues(trackj, &t->cues);
  make_beats(trackj, &t->beats);
  return 0;
}

static int make_track(cJSON* json, finyl_track* t) {
  finyl_init_track(t);
  unmarshal_track(json, t);

  char root[500];
  join_path(root, usb, "finyl/separated/hdemucs_mmi");
  set_channels_filepaths(&t->meta, root);
    
  return 0;
}

static int unmarshal_error(cJSON* json, char* error) {
  cJSON* errorj = cJSON_GetObjectItem(json, "error");
  if (cJSON_IsString(errorj)) {
    char* _error = errorj->valuestring;
    strcpy(error, _error);
    return 1;
  }

  return 0;
}

static int run_command(FILE** fp, char* badge, char* usb, char* op) {
  char exec[128] = "";
  if (is_raspi()) {
    strcat(exec, "./finyl-digger");
  } else {
    strcat(exec, "java -jar crate-digger/target/finyl-1.0-SNAPSHOT.jar");
  }
  char command[1000];
  snprintf(command, sizeof(command), "%s %s %s %s", exec, badge, usb, op);
  
  *fp = popen(command, "r");
  if (fp == NULL) {
    printf("failed to open stream for usb=%s and op=%s\n", usb, op);
    return -1;
  }

  return 0;
}

int close_command(FILE* fp) {
  char error_out[10000];
  fread(error_out, 1, sizeof(error_out), fp);
  int status = pclose(fp);
  if (status == -1) {
    printf("failed to close the command stream\n");
    return -1;
  }
  status = WEXITSTATUS(status);
  if (status == 1) {
    //there is a fatal error, and error message is printed in error_out
    printf("Error:\n");
    printf("%s\n", error_out);
    return -1;
  }

  return 0;
}

static bool badge_valid(cJSON* json, char* badge)  {
  cJSON* badgej = cJSON_GetObjectItem(json, "badge");
  char* _badge = badgej->valuestring;

  if (strcmp(badge, _badge) == 0) {
    return true;
  }

  return false;
}

//1 for not valid badge. -1 for command error
static int run_digger(cJSON** json, char* usb, char* op) {
  FILE* fp;
  char badge[6];
  generateRandomString(badge, sizeof(badge));

  if (run_command(&fp, badge, usb, op) == -1) {
    return -1;
  }
  
  if (close_command(fp) == -1) {
    return -1;
  }
  
  *json = read_file_malloc_json(get_finyl_output_path());
  
  if (!badge_valid(*json, badge)) {
    return 1;
  }
  
  return 0;
}


int has_error(cJSON* json) {
  char error[1000];
  int haserror = unmarshal_error(json, error);
  if (haserror) {
    printf("crate-digger error: %s\n", error);
    return 1;
  }

  return 0;
}

//return size of playlists, or -1 for error
int get_playlists(finyl_playlist** pls, char* usb) {
  cJSON* json;
  int status = run_digger(&json, usb, "playlists");
  if (status == -1) return -1;
  if (status == 1) {
    cJSON_Delete(json);
    return -1;
  }
  
  if (has_error(json)) {
    cJSON_Delete(json);
    return -1;
  }
  
  int playlists_size = make_playlists(json, pls);
  cJSON_Delete(json);
  return playlists_size;
}

int get_track(finyl_track* t, char* usb, int tid) {
  cJSON* json;
  char op[30];
  snprintf(op, sizeof(op), "track %d", tid);
  int status = run_digger(&json, usb, op);
  if (status == -1) return -1;
  if (status == 1) {
    cJSON_Delete(json);
    return -1;
  }

  if (has_error(json)) {
    cJSON_Delete(json);
    return -1;
  }
  
  make_track(json, t);
  cJSON_Delete(json);
  
  return 0;
}

//TODO
/* int get_all_tracks(finyl_track** ts, char* usb) { */
  /* cJSON* json; */
  /* if (run_digger(&json, usb, "all-tracks") == -1) { */
    /* return -1; */
  /* } */
  
  /* cJSON_Delete(json); */
  /* return 0; */
/* } */

int get_playlist_tracks(finyl_track_meta** tms, char* usb, int pid) {
  cJSON* json;
  char op[30];
  snprintf(op, sizeof(op), "playlist-tracks %d", pid);
  int status = run_digger(&json, usb, op);
  if (status == -1) return -1;
  if (status == 1) {
    cJSON_Delete(json);
    return -1;
  }
  if (has_error(json)) {
    cJSON_Delete(json);
    return -1;
  }
  
  int tms_size = make_playlist_tracks(json, tms);
  cJSON_Delete(json);

  finyl_track_meta* _tms = *tms;
  char root[500];
  join_path(root, usb, "finyl/separated/hdemucs_mmi");
  for (int i = 0; i<tms_size; i++) {
    set_channels_filepaths(&_tms[i], root);
  }
  
  return tms_size;
}
