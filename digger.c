#include "finyl.h"
#include "digger.h"
#include "cJSON.h"

char* finyl_output_path = "/home/null/.finyl-output"; //TODO

void free_array(void *arr, int size, void(*free_func)(void*)) {
}

void free_playlist(finyl_playlist* pl) {
  free(pl->name);
}

void free_playlists(finyl_playlist* pls, int size) {
  for (int i = 0; i<size; i++) {
    free_playlist(&pls[i]);
  }

  free(pls);
}

void free_track_meta(finyl_track_meta* tm) {
  free(tm->title);
  free(tm->filepath);
  free(tm->filename);
}

void free_track_metas(finyl_track_meta* tms, int size) {
  for (int i = 0; i<size; i++) {
    free_track_meta(&tms[i]);
  }

  free(tms);
}

static void generateRandomString(char* badge, size_t size) {
  char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  srand(time(NULL));
  
  int i;
  for (i = 0; i < size - 1; ++i) {
    int index = rand() % (sizeof(characters) - 1);
    badge[i] = characters[index];
  }
  badge[size - 1] = '\0';
}

//copy only the dest amount of src to dest
static void ncpy(char* dest, char* src, size_t size) {
  strncpy(dest, src, size);
  dest[size - 1] = '\0';;
}

static void cJSON_ncpy(cJSON* json, char* key, char* dest, size_t size) {
  cJSON* itemj = cJSON_GetObjectItem(json, key);
  char* item = itemj->valuestring;
  ncpy(dest, item, size);
}

static void cJSON_cpy(cJSON* json, char* key, char* dest) {
  cJSON* itemj = cJSON_GetObjectItem(json, key);
  char* item = itemj->valuestring;
  strcpy(dest, item);
}

static void cJSON_malloc_cpy(cJSON* json, char* key, char** dest) {
  cJSON* itemj = cJSON_GetObjectItem(json, key);
  char* item = itemj->valuestring;
  *dest = (char*)malloc(strlen(item) + 1);
  strcpy(*dest, item);
}

static int unmarshal_playlist(cJSON* pj, finyl_playlist* p) {
  cJSON_malloc_cpy(pj, "name", &p->name);
  p->id = cJSON_GetObjectItem(pj, "id")->valueint;

  return 0;
}

//returns size, -1 for error;
static int unmarshal_playlists(cJSON* json, finyl_playlist** pls) {
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

static int unmarshal_track_meta1(cJSON* trackj, finyl_track_meta** tm) {
  cJSON* metaj = cJSON_GetObjectItem(trackj, "t"); //meta
  *tm = (finyl_track_meta*)malloc(sizeof(finyl_track_meta));
  unmarshal_track_meta(metaj, *tm);

  return 0;
}

static int unmarshal_track_metas(cJSON* tracksj, finyl_track_meta** tms) {
  int tms_size = cJSON_GetArraySize(tracksj);
  *tms = (finyl_track_meta*)malloc(sizeof(finyl_track_meta) * tms_size);
  finyl_track_meta* _tms = *tms;

  for (int i = 0; i<tms_size; i++) {
    cJSON* metaj = cJSON_GetArrayItem(tracksj, i);
    unmarshal_track_meta(metaj, &_tms[i]);
  }

  return tms_size;
}

static int unmarshal_playlist_tracks(cJSON* json, finyl_track_meta** tms) {
  cJSON* tracksj = cJSON_GetObjectItem(json, "tracks");
  int tms_size = unmarshal_track_metas(tracksj, tms);
  return tms_size;
}

static int unmarshal_cue(cJSON* cuej, finyl_cue* cue) {
  cJSON* typej = cJSON_GetObjectItem(cuej, "type");
  cue->type = typej->valueint;
  cJSON* timej = cJSON_GetObjectItem(cuej, "time");
  cue->time = timej->valueint;

  return 0;
}

static int unmarshal_cues(cJSON* trackj, finyl_cue** cues) {
  cJSON* cuesj = cJSON_GetObjectItem(trackj, "cues");
  int cues_size = cJSON_GetArraySize(cuesj);
  *cues = (finyl_cue*)malloc(sizeof(finyl_cue) * cues_size);
  
  finyl_cue* _cues = *cues;
  for (int i = 0; i<cues_size; i++) {
    cJSON* cuej = cJSON_GetArrayItem(cuesj, i);
    unmarshal_cue(cuej, &_cues[i]);
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

static int unmarshal_beats(cJSON* trackj, finyl_beat** beats) {
  cJSON* beatsj = cJSON_GetObjectItem(trackj, "beats");
  int beats_size = cJSON_GetArraySize(beatsj);
  *beats = (finyl_beat*)malloc(sizeof(finyl_beat) * beats_size);

  finyl_beat* _beats = *beats;

  for (int i = 0; i<beats_size; i++) {
    cJSON* beatj = cJSON_GetArrayItem(beatsj, i);
    unmarshal_beat(beatj, &_beats[i]);
  }

  return beats_size;
}

static int unmarshal_track(cJSON* json, finyl_track* t) {
  cJSON* trackj = cJSON_GetObjectItem(json, "track");

  unmarshal_track_meta1(trackj, &t->meta);
  t->cues_size = unmarshal_cues(trackj, &t->cues);
  t->beats_size = unmarshal_beats(trackj, &t->beats);
  return 0;
}

static int unmarshal_error(cJSON* json, char* error) {
  cJSON* errorj = cJSON_GetObjectItem(json, "error");
  if (cJSON_IsString(errorj)) {
    char* _error = errorj->valuestring;
    strcpy(error, _error);
  }

  return 0;
}

static int run_command(FILE** fp, char* badge, char* usb, char* op) {
  char command[1000];
  snprintf(command, sizeof(command), "java -jar crate-digger/target/finyl-1.0-SNAPSHOT.jar %s %s %s", badge, usb, op);
  
  *fp = popen(command, "r");
  if (fp == NULL) {
    printf("failed to open stream for usb=%s and op=%s\n", usb, op);
    return -1;
  }

  return 0;
}

static int check_command_error(FILE* fp) {
  char error_out[10000];
  fread(error_out, 1, sizeof(error_out), fp);
  int status = pclose(fp);
  if (status == -1) {
    printf("failed to close the stream in get_playlists\n");
    return -1;
  }
  status = WEXITSTATUS(status);
  if (status == 1) {
    //there is a fatal error, and error message is printed in error_out
    printf("fatal error in run_digger. Error:\n");
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

static int run_digger(cJSON** json, char* usb, char* op) {
  FILE* fp;
  char badge[6];
  generateRandomString(badge, sizeof(badge));

  if (run_command(&fp, badge, usb, op) == -1) {
    return -1;
  }
  
  if (check_command_error(fp) == -1) {
    return -1;
  }
  
  *json = read_file_malloc_json(finyl_output_path);
  
  if (!badge_valid(*json, badge)) {
    return -1;
  }
  
  return 0;
}


//return size of playlists, or -1 for error
int get_playlists(finyl_playlist** pls, char* usb) {
  cJSON* json;
  if (run_digger(&json, usb, "playlists") == -1) {
    return -1;
  }
  
  int playlists_size = unmarshal_playlists(json, pls);
  cJSON_Delete(json);
  return playlists_size;
}

int get_track(finyl_track* t, char* usb, int tid) {
  cJSON* json;
  char op[30];
  snprintf(op, sizeof(op), "track %d", tid);
  if (run_digger(&json, usb, op) == -1) {
    return -1;
  }

  unmarshal_track(json, t);
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

int get_playlist_tracks(finyl_track_meta** tracks, char* usb, int pid) {
  cJSON* json;
  char op[30];
  snprintf(op, sizeof(op), "playlist-tracks %d", pid);
  if (run_digger(&json, usb, op) == -1) {
    return -1;
  }
  
  int tracks_size = unmarshal_playlist_tracks(json, tracks);
  cJSON_Delete(json);
  return tracks_size;
}

char* read_file_malloc(char* filename) {
  FILE* fp = fopen(filename, "rb");

  if (fp == NULL) {
    printf("failed to read from file = %s\n", filename);
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char* buffer = (char*)malloc(file_size + 1);
  if (buffer == NULL) {
    printf("failed to allocate memory for buffer in read_file\n");
    fclose(fp);
    return NULL;
  }

  size_t count = fread((void*)buffer, 1, file_size, fp);
  if (count != file_size) {
    printf("Error reading a file in read_file_malloc\n");
    free(buffer);
    fclose(fp);
    return NULL;
  }

  buffer[file_size] = '\0';
  fclose(fp);
  return buffer;
}

cJSON* read_file_malloc_json(char* file) {
  char* output = read_file_malloc(file);
  cJSON* json  = cJSON_Parse(output);

  free(output); //TODO safe?
  return json;
}
