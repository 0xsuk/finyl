#include "finyl.h"
#include "digger.h"
#include "cJSON.h"
#include "dev.h"
#include <dirent.h>
#include "util.h"


template<typename T>
using unmarshal_func = void (*)(cJSON*, T&);

template<typename T>
static void make_objs(cJSON* json, std::vector<T>& arr, char* key, unmarshal_func<T> f) {
  cJSON* listj = cJSON_GetObjectItem(json, key);
  int size = cJSON_GetArraySize(listj);

  arr.resize(size);
  for (int i = 0; i<size; i++) {
    cJSON* j = cJSON_GetArrayItem(listj, i);
    f(j, arr[i]);
  }
}


void list_playlists() {
  std::vector<finyl_playlist> pls;
  int status = get_playlists(pls, usb);
  
  if (status == -1) {
    return;
  }
  
  printf("\n");
  for (int i = 0; i<pls.size(); i++) {
    printf("%d %s\n", pls[i].id, pls[i].name.data());

  }
}

void list_playlist_tracks(int pid) {
  std::vector<finyl_track_meta> tms;
  int status = get_playlist_tracks(tms, usb, pid);

  if (status == -1) {
    return;
  }
  
  printf("\n");
  for (int i = 0; i<tms.size(); i++) {
    printf("%d %d %d %s\n", tms[i].id, tms[i].bpm, (int)tms[i].channel_filepaths.size(), tms[i].title.data());
  }
}

bool match(std::string hash, char* filename) {
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
    if (strncmp(hash.data(), &filename[under+1], 32) == 0) {
      return true;
    }
  }
  return false;
}

void set_channels_filepaths(finyl_track_meta& tm, std::string root) {
  std::string hash = compute_md5(tm.filepath);

  DIR* d = opendir(root.data());
  struct dirent* dir;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (match(hash, dir->d_name)) {
        auto f = join_path(root.data(), dir->d_name);
        tm.channel_filepaths.push_back(f);
      }
    }
    closedir(d);
  }
}

static void unmarshal_playlist(cJSON* pj, finyl_playlist& p) {
  p.name = cJSON_get_string(pj, "name");
  p.id = cJSON_GetObjectItem(pj, "id")->valueint;
}

static void make_playlists(cJSON* json, std::vector<finyl_playlist>& pls) {
  make_objs(json, pls, "playlists", &unmarshal_playlist);
}

static void unmarshal_track_meta(cJSON* metaj, finyl_track_meta& tm) {
  tm.id = cJSON_GetObjectItem(metaj, "id")->valueint;
  tm.musickeyid = cJSON_GetObjectItem(metaj, "musickeyid")->valueint;
  tm.bpm = cJSON_GetObjectItem(metaj, "tempo")->valueint;
  tm.filesize = cJSON_GetObjectItem(metaj, "filesize")->valueint;
    
  tm.title = cJSON_get_string(metaj, "title");
  tm.filepath = cJSON_get_string(metaj, "filepath");
  tm.filename = cJSON_get_string(metaj, "filename");
}

static void make_playlist_tracks(cJSON* json, std::vector<finyl_track_meta>& tms) {
  make_objs(json, tms, "tracks", &unmarshal_track_meta);
}

static void unmarshal_cue(cJSON* cuej, finyl_cue& cue) {
  cJSON* typej = cJSON_GetObjectItem(cuej, "type");
  cue.type = typej->valueint;
  cJSON* timej = cJSON_GetObjectItem(cuej, "time");
  cue.time = timej->valueint;
}

static void make_cues(cJSON* trackj, std::vector<finyl_cue>& cues) {
  make_objs(trackj, cues, "cues", &unmarshal_cue);
}

static void unmarshal_beat(cJSON* beatj, finyl_beat& beat) {
  cJSON* timej = cJSON_GetObjectItem(beatj, "time");
  beat.time = timej->valueint;
  cJSON* numberj = cJSON_GetObjectItem(beatj, "num");
  beat.number = numberj->valueint;
}

static void make_beats(cJSON* trackj, std::vector<finyl_beat>& beats) {
  make_objs(trackj, beats, "beats", &unmarshal_beat);
}

static void unmarshal_track(cJSON* json, finyl_track& t) {
  cJSON* trackj = cJSON_GetObjectItem(json, "track");
  cJSON* metaj = cJSON_GetObjectItem(trackj, "t"); //meta

  unmarshal_track_meta(metaj, t.meta);
  make_cues(trackj, t.cues);
  make_beats(trackj, t.beats);
}

static void make_track(cJSON* json, finyl_track& t) {
  unmarshal_track(json, t);

  std::string root = join_path(usb.data(), "finyl/separated/hdemucs_mmi");
  set_channels_filepaths(t.meta, root);
}

static std::string unmarshal_error(cJSON* json) {
  cJSON* errorj = cJSON_GetObjectItem(json, "error");
  if (cJSON_IsString(errorj)) {
    return errorj->valuestring;
  }

  return "";
}

static int run_command(FILE** fp, std::string badge, std::string usb, std::string op) {
  char exec[128] = "";
  if (is_raspi()) {
    strcat(exec, "./finyl-digger");
  } else {
    strcat(exec, "java -jar crate-digger/target/finyl-1.0-SNAPSHOT.jar");
  }
  char command[1000];
  snprintf(command, sizeof(command), "%s %s %s %s", exec, badge.data(), usb.data(), op.data());
  
  *fp = popen(command, "r");
  if (fp == nullptr) {
    printf("failed to open stream for usb=%s and op=%s\n", usb.data(), op.data());
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

static bool badge_valid(cJSON* json, std::string badge)  {
  if (badge == cJSON_get_string(json, "badge")) {
    return true;
  }
  return false;
}

//1 for not valid badge. -1 for command error
static int run_digger(cJSON** json ,std::string usb, std::string op) {
  std::string badge = generate_random_string(5);

  FILE* fp;
  auto status = run_command(&fp, badge, usb, op);
  if (status == -1) {
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
  auto error = unmarshal_error(json);
  if (error != "") {
    printf("crate-digger error: %s\n", error.data());
    return 1;
  }

  return 0;
}

int get_playlists(std::vector<finyl_playlist>& pls, std::string usb) {
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
  
  make_playlists(json, pls);
  cJSON_Delete(json);
  return 0;
}

int get_track(finyl_track& t, std::string usb, int tid) {
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

int get_playlist_tracks(std::vector<finyl_track_meta>& tms, std::string usb, int pid) {
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
  
  make_playlist_tracks(json, tms);
  cJSON_Delete(json);

  std::string root = join_path(usb.data(), "finyl/separated/hdemucs_mmi");
  for (int i = 0; i<tms.size(); i++) {
    set_channels_filepaths(tms[i], root);
  }
  
  return 0;
}
