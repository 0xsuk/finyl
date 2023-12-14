#include "finyl.h"
#include "digger.h"
#include "cJSON.h"

char* finyl_output_path = "/home/null/.finyl-output"; //TODO

/* void free_playlist(playlist* pl) { */
/*   free(pl->name); */
/* } */

void free_track(finyl_track_meta* tm) {
  free(tm->title);
  free(tm->filepath);
  free(tm->filename);
}

/* void free_finyl_output(finyl_output* fo) { */
/*   free(fo->usb); */
/*   free(fo->error); */
/*   for (int i = 0; i<fo->playlists_size; i++) { */
/*     free_playlist(&fo->playlists[i]); */
/*   } */

/*   for (int i = 0; i<fo->tracks_size; i++) { */
/*     free_track(&fo->tracks[i]); */
/*   } */
/* } */

void generateRandomString(char* badge, size_t size) {
  char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  srand(time(NULL));
  
  int i;
  for (i = 0; i < size - 1; ++i) {
    int index = rand() % (sizeof(characters) - 1);
    badge[i] = characters[index];
  }
  badge[size - 1] = '\0';
}

/* void _print_playlist(playlist* p) { */
/*   printf("\t\tid=%d\n", p->id); */
/*   printf("\t\tname=%s\n", p->name); */
/* } */

/* void init_fo() { */
/*   fo.error = NULL; */
/*   fo.playlists_length = 0; */
/* } */

/* void free_fo() { */
/*   for (int i = 0; i<fo.playlists_length; i++) { */
/*     playlist* p = fo.playlists[i]; */
/*     free(p); */
/*   } */

/*   free(fo.error); */
/* } */

//copy only the dest amount of src to dest
void ncpy(char* dest, char* src, size_t size) {
  strncpy(dest, src, size);
  dest[size - 1] = '\0';;
}

void cJSON_ncpy(cJSON* json, char* key, char* dest, size_t size) {
  cJSON* itemj = cJSON_GetObjectItem(json, key);
  char* item = itemj->valuestring;
  ncpy(dest, item, size);
}

void cJSON_cpy(cJSON* json, char* key, char* dest) {
  cJSON* itemj = cJSON_GetObjectItem(json, key);
  char* item = itemj->valuestring;
  strcpy(dest, item);
}

void cJSON_malloc_cpy(cJSON* json, char* key, char* dest) {
  cJSON* itemj = cJSON_GetObjectItem(json, key);
  char* item = itemj->valuestring;
  dest = (char*)malloc(strlen(item) + 1);
  strcpy(dest, item);
}

//returns size, -1 for error;
int unmarshal_playlists(cJSON* json, finyl_playlist* pls) {
  cJSON* playlistsj = cJSON_GetObjectItem(json, "playlists");
  int playlists_size = cJSON_GetArraySize(playlistsj);
  pls = (finyl_playlist*)malloc(sizeof(finyl_playlist) * playlists_size);

  for (int i = 0; i<playlists_size; i++) {
    cJSON* pj = cJSON_GetArrayItem(playlistsj, i);
    cJSON_malloc_cpy(pj, "name", pls[i].name);
    pls[i].id = cJSON_GetObjectItem(pj, "id")->valueint;
  }

  return playlists_size;
}

int unmarshal_error(cJSON* json, char* error) {
  cJSON* errorj = cJSON_GetObjectItem(json, "error");
  if (cJSON_IsString(errorj)) {
    char* _error = errorj->valuestring;
    strcpy(error, _error);
  }

  return 0;
}

int run_command(FILE** fp, char* badge, char* usb, char* op) {
  char command[1000];
  snprintf(command, sizeof(command), "java -jar crate-digger/target/finyl-1.0-SNAPSHOT.jar %s %s %s", badge, usb, op);
  
  *fp = popen(command, "r");
  if (fp == NULL) {
    printf("failed to open stream for usb=%s and op=%s\n", usb, op);
    return -1;
  }

  return 0;
}

int check_command_error(FILE* fp) {
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


bool badge_valid(cJSON* json, char* badge)  {
  cJSON* badgej = cJSON_GetObjectItem(json, "badge");
  char* _badge = badgej->valuestring;

  if (strcmp(badge, _badge) == 0) {
    return true;
  }

  return false;
}

int run_digger(cJSON** json, char* usb, char* op) {
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


int get_playlists(finyl_playlist* pls, char* usb) {
  
  cJSON* json;
  if (run_digger(&json, usb, "playlists") == -1) {
    return -1;
  }
  
  unmarshal_playlists(json, pls);
  return 0;
}

/* int get_track(finyl_track* t, char* usb, int id) { */
/*   finyl_output fo; */
/*   char params[100]; */
/*   snprintf(params, sizeof(params), "track %d", id); */
/*   if (run_digger(usb, params, gn) != -1) { */
/*     return -1; */
/*   } */
/*   cJSON* json = read_file_malloc_json(finyl_output_path); */
  
/*   make_track(t, &fo); */
  
/*   cJSON_Delete(json); */
  
/*   return 0; */
/* } */

/* int get_all_tracks(char* usb) { */
/*   if (run_digger(usb, "all-tracks")) { */
/*     return -1; */
/*   } */
  
/*   return 0; */
/* } */

/* int get_playlist_track(char* usb, int id) { */
/*   char params[100]; */
/*   snprintf(params, sizeof(params), "playlist-track %d", id); */
/*   if (run_digger(usb, params)) { */
/*     return -1; */
/*   } */
  
/*   return 0; */
/* } */

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
