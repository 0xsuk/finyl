
/* typedef struct { */
/*   int id; */
/*   char name[300]; */
/* } playlist; */

/* typedef struct { */
/*   char usb[300]; */
/*   char badge[badge_length]; */
/*   char* error; */
/*   int playlists_length; */
/*   playlist* playlists[max_playlists_length]; */
/*   track_meta** tracks; */
/*   track* track; */
/* } finyl_output; */

/* finyl_output fo; */

/* void generateRandomString(char* badge, size_t size) { */
/*   char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; */
/*   srand(time(NULL)); */
  
/*   int i; */
/*   for (i = 0; i < size - 1; ++i) { */
/*     int index = rand() % (sizeof(characters) - 1); */
/*     badge[i] = characters[index]; */
/*   } */
/*   badge[size - 1] = '\0'; */
/* } */

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

/* void _print_fo(finyl_output fo) { */
/*   printf("finyl output {\n"); */
/*   printf("\tusb: %s\n", fo.usb); */
/*   printf("\tbadge: %s\n", fo.badge); */
/*   printf("\terror: %s\n", fo.error); */
/*   printf("\tplaylists_length: %d\n", fo.playlists_length); */
/*   for (int i = 0; i < fo.playlists_length; i++) { */
/*     _print_playlist(fo.playlists[i]); */
/*   } */
/*   printf("}\n"); */
/* } */

/* void cJSON_ncpy(cJSON* json, char* key, char* dest, size_t size) { */
/*   cJSON* itemj = cJSON_GetObjectItem(json, key); */
/*   char* item = itemj->valuestring; */
/*   ncpy(dest, item, size); */
/* } */

/* int unmarshal_finyl_output(cJSON* json) { */
/*   cJSON_ncpy(json, "usb", fo.usb, sizeof(fo.usb)); */
/*   cJSON_ncpy(json, "badge", fo.badge, sizeof(fo.badge)); */
/*   cJSON* errorj = cJSON_GetObjectItem(json, "error"); */
/*   if (cJSON_IsString(errorj)) { */
/*     char* error = errorj->valuestring; */
/*     ncpy(fo.error, error, sizeof(fo.error)); */
/*     //do not return -1, because if badge is old, error is invalid; */
/*   } */
  
/*   cJSON* playlists = cJSON_GetObjectItem(json, "playlists"); */
/*   int playlists_length = cJSON_GetArraySize(playlists); */
/*   for (int i = 0; i<playlists_length; i++) { */
/*     playlist* p = (playlist*)malloc(sizeof(playlist)) ; */
    
/*     cJSON* pj = cJSON_GetArrayItem(playlists, i); */
/*     cJSON_ncpy(pj, "name", p->name, sizeof(p->name)); */
/*     p->id = cJSON_GetObjectItem(pj, "id")->valueint; */

/*     fo.playlists[i] = p; */
/*     fo.playlists_length++; */
/*   } */
  
/*   _print_fo(fo); */
  
/*   return 0; */
/* } */

/* int read_finyl_output() { */
/*   //if finyl-output json really updated from the old? check the badge */
/*   char* output = read_file_malloc(finyl_output_path); */
/*   cJSON* json = cJSON_Parse(output); */
  
/*   if (unmarshal_finyl_output(json) == -1) { */
/*     return -1; */
/*   } */
  
/*   cJSON_Delete(json); */
/*   free(output); */
/*   return 0; */
/* } */

/* int run_digger(char* usb, char* op) { */
/*   free_fo(); */
/*   init_fo(); */
  
/*   char command[1000]; */
/*   char badge[6]; */

/*   generateRandomString(badge, sizeof(badge)); */
  
/*   snprintf(command, sizeof(command), "java -jar crate-digger/target/finyl-1.0-SNAPSHOT.jar %s %s %s", badge, usb, op); */
  
/*   FILE* fp = popen(command, "r"); */
/*   if (fp == NULL) { */
/*     printf("failed to open stream for usb=%s and op=%s\n", usb, op); */
/*     return -1; */
/*   } */

/*   char error_out[10000]; */
/*   fread(error_out, 1, sizeof(error_out), fp); */
/*   int status = pclose(fp); */
/*   if (status == -1) { */
/*     printf("failed to close the stream in get_playlists\n"); */
/*     return -1; */
/*   } */
  
/*   status = WEXITSTATUS(status); */
/*   if (status == 1) { */
/*     //there is a fatal error, and error message is printed in error_out */
/*     printf("fatal error in run_digger. Error:\n"); */
/*     printf("%s\n", error_out); */
/*     return -1; */
/*   } */
  
/*   if (read_finyl_output() == -1) { */
/*     return -1; */
/*   } */
  
/*   //check badge here */
/*   if (strcmp(fo.badge, badge) != 0) { */
/*     printf("badge is not latest.  %s should be %s\n", fo.badge, badge); */
/*     return -1; */
/*   } */
  
/*   //check error messgae in fo */
/*   if (fo.error != NULL) { */
/*     printf("crate-digger error: %s\n", fo.error); */
/*     return -1; */
/*   } */
  
/*   return 0; */
/* } */

/* int get_playlists(playlist* p, char* usb) { */
/*   if (run_digger(usb, "playlists") == -1) { */
/*     return -1; */
/*   } */
  
  
  
/*   return 0; */
/* } */

/* int get_track(char* usb, int id) { */
/*   char params[100]; */
/*   snprintf(params, sizeof(params), "track %d", id); */
/*   if (run_digger(usb, params) != -1) { */
/*     return -1; */
/*   } */
  
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

/* char* read_file_malloc(char* filename) { */
/*   FILE* fp = fopen(filename, "rb"); */

/*   if (fp == NULL) { */
/*     printf("failed to read from file = %s\n", filename); */
/*     return NULL; */
/*   } */

/*   fseek(fp, 0, SEEK_END); */
/*   long file_size = ftell(fp); */
/*   fseek(fp, 0, SEEK_SET); */

/*   char* buffer = (char*)malloc(file_size + 1); */
/*   if (buffer == NULL) { */
/*     printf("failed to allocate memory for buffer in read_file\n"); */
/*     fclose(fp); */
/*     return NULL; */
/*   } */

/*   size_t count = fread((void*)buffer, 1, file_size, fp); */
/*   if (count != file_size) { */
/*     printf("Error reading a file in read_file_malloc\n"); */
/*     free(buffer); */
/*     fclose(fp); */
/*     return NULL; */
/*   } */

/*   buffer[file_size] = '\0'; */
/*   fclose(fp); */
/*   return buffer; */
/* } */

/* //copy only the dest amount of src to dest */
/* void ncpy(char* dest, char* src, size_t size) { */
/*   strncpy(dest, src, size); */
/*   dest[size - 1] = '\0';; */
/* } */

/* int main() { */
/*   printf("duh\n"); */
/* } */
