#include "digger.h"
#include "dev.h"
#include <dirent.h>
#include "util.h"
#include "json.h"

template<typename T>
using unmarshal_func = void (*)(const json::node&, T&);

template<typename T>
static void make_objs(const json::node& n, std::vector<T>& arr, const std::string& key, unmarshal_func<T> f) {
  const auto* vec = json::get_if<json::array>(n, key);
  if (vec == nullptr) return;


  arr.resize(vec->size());
  for (int i = 0; i<vec->size(); i++) {
    const json::node* elem = (*vec)[i].get();
    f(*elem, arr[i]);
  }
}


void list_playlists() {
  std::vector<finyl_playlist> pls;
  auto err  = get_playlists(pls, usb);
  
  if (err) {
    return;
  }
  
  printf("\n");
  for (int i = 0; i<pls.size(); i++) {
    printf("%d %s\n", pls[i].id, pls[i].name.data());

  }
}

void list_playlist_tracks(int pid) {
  std::vector<finyl_track_meta> tms;
  auto err = get_playlist_tracks(tms, usb, pid);

  if (err) {
    return;
  }
  
  printf("\n");
  for (int i = 0; i<tms.size(); i++) {
    printf("%d %d %d %s\n", tms[i].id, tms[i].bpm, (int)tms[i].stem_filepaths.size(), tms[i].title.data());
  }
}

bool match(std::string_view hash, char* filename) {
  int dot = find_char_last(filename, '.');
  if (dot == -1) {
    return false;
  }
  char f[dot+1];
  strncpy(f, filename, dot);
  f[dot] = '\0';

  int hyphen = find_char_last(f, '-');
  if (hyphen == -1) {
    return false;
  }
  
  if (dot - hyphen == 33) {
    if (strncmp(hash.data(), &filename[hyphen+1], 32) == 0) {
      return true;
    }
  }
  return false;
}

void set_channels_filepaths(finyl_track_meta& tm, std::string_view root) {
  std::string hash = compute_md5(tm.filepath);

  DIR* d = opendir(root.data());
  struct dirent* dir;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (match(hash, dir->d_name)) {
        auto f = join_path(root.data(), dir->d_name);
        tm.stem_filepaths.push_back(std::move(f));
      }
    }
    closedir(d);
  }
}

static void unmarshal_playlist(const json::node& n, finyl_playlist& p) {
  p.name = *json::get_if<std::string>(n, "name");
  p.id = *json::get_if<float>(n, "id");
}

static void make_playlists(const json::node& n, std::vector<finyl_playlist>& pls) {
  make_objs(n, pls, "playlists", &unmarshal_playlist);
}

static void unmarshal_track_meta(const json::node& meta_n, finyl_track_meta& tm) {
  tm.id = *json::get_if<float>(meta_n, "id");
  tm.musickeyid = *json::get_if<float>(meta_n, "musickeyid");
  tm.bpm = *json::get_if<float>(meta_n, "tempo");
  tm.filesize = *json::get_if<float>(meta_n, "filesize");
  
  tm.title = *json::get_if<std::string>(meta_n, "title");
  tm.filepath = *json::get_if<std::string>(meta_n, "filepath");
  tm.filename = *json::get_if<std::string>(meta_n, "filename");
}

static void make_playlist_tracks(const json::node& n, std::vector<finyl_track_meta>& tms) {
  make_objs(n, tms, "tracks", &unmarshal_track_meta);
}

static void unmarshal_cue(const json::node& n, finyl_cue& cue) {
  cue.type = *json::get_if<float>(n, "type");
  cue.time = *json::get_if<float>(n, "time");
}

static void make_cues(const json::node& track_n, std::vector<finyl_cue>& cues) {
  make_objs(track_n, cues, "cues", &unmarshal_cue);
}

static void unmarshal_beat(const json::node& n, finyl_beat& beat) {
  beat.time = *json::get_if<float>(n, "time");
  beat.number = *json::get_if<float>(n, "num");
}

static void make_beats(const json::node& track_n, std::vector<finyl_beat>& beats) {
  make_objs(track_n, beats, "beats", &unmarshal_beat);
}

static void unmarshal_track(const json::node& n, finyl_track& t) {
  auto track_n = json::get_node(n, "track");
  auto meta_n = json::get_node(*track_n, "t");

  unmarshal_track_meta(*meta_n, t.meta);
  make_cues(*track_n, t.cues);
  make_beats(*track_n, t.beats);
}

static void make_track(const json::node& n,finyl_track& t) {
  unmarshal_track(n, t);

  std::string root = join_path(usb.data(), "finyl/separated/hdemucs_mmi");
  set_channels_filepaths(t.meta, root);
}

static std::string unmarshal_error(const json::node& n) {
  auto ret = json::get_if<std::string>(n, "error");

  if (!ret) return "";
  return *ret;
}

static error run_command(FILE** fp, std::string_view badge, std::string_view usb, std::string_view op) {
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
    return error(std::string("failed to open stream for usb=") + usb.data() + "op=" + op.data(), ERR::CANT_OPEN_COMMAND);
  }

  return {};
}

error close_command(FILE* fp) {
  char error_out[10000];
  fread(error_out, 1, sizeof(error_out), fp);
  int status = pclose(fp);
  if (status == -1) {
    return error("failed to close the command stream", ERR::CANT_CLOSE_COMMAND);
  }
  status = WEXITSTATUS(status);
  if (status == 1) {
    return error(std::move(error_out), ERR::COMMAND_FAILED);
  }

  return noerror;
}

static bool badge_valid(const json::node& n, std::string_view badge)  {
  const auto* s = json::get_if<std::string>(n, "badge");
  
  if (s == nullptr) return false;

  return (*s == badge);
}

//1 for not valid badge. -1 for command error
static std::pair<std::unique_ptr<json::node>, error> run_digger(std::string_view usb, std::string_view op) {
  std::string badge = generate_random_string(5);

  {
    FILE* fp;
    auto err = run_command(&fp, badge, usb, op);
    if (err.has) return {nullptr, err};
  
    if (auto err = close_command(fp); err.has) {
      return {nullptr, err};
    }
  }
  
  auto p = json::parser(get_finyl_output_path());
  
  auto [n, err] = p.parse();
  if (err != json::STATUS::OK) return {nullptr, ERR::JSON_FAILED};

  if (!badge_valid(*n, badge)) {
    return {nullptr, ERR::BADGE_NOT_VALID};
  }

  return {std::move(n), noerror};
}


template<typename T>
using make_func = void (*)(const json::node&, T&);

template<typename T>
static error get_stuff(T& stuff, const std::string_view usb, const std::string& op, make_func<T> f) {
  const auto [nodeptr, err] = run_digger(usb, op);
  
  if (err) return err;
  
  if (auto err_str = unmarshal_error(*nodeptr); err_str != "") {
    return error(std::move(err_str), ERR::JSON_FAILED);
  }
  
  f(*nodeptr, stuff);
  return noerror;
}

error get_playlists(std::vector<finyl_playlist>& pls, std::string_view usb) {
  return get_stuff(pls, usb, "playlists", &make_playlists);
}

error get_track(finyl_track& t, std::string_view usb, int tid) {
  char op[30];
  snprintf(op, sizeof(op), "track %d", tid);
  return get_stuff(t, usb, op, &make_track);
}

error get_playlist_tracks(std::vector<finyl_track_meta>& tms, std::string_view usb, int pid) {
  char op[30];
  snprintf(op, sizeof(op), "playlist-tracks %d", pid);
  if (auto err = get_stuff(tms, usb, op, &make_playlist_tracks)) return err;
  
  std::string root = join_path(usb.data(), "finyl/separated/hdemucs_mmi");
  for (int i = 0; i<tms.size(); i++) {
    set_channels_filepaths(tms[i], root);
  }
  
  return noerror;
}
