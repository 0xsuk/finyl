//LIMITATIONS:
//demucs
//two-stems vocals


#include "finyl.h"
#include "util.h"
#include "dev.h"
#include "rekordbox.h"
#include "controller.h"
#include <stdlib.h>
#include <libgen.h>
#include <utility>
#include <string_view>

#define same(x,y) (strcmp(x, y) == 0)

std::string ext = "wav";

std::string get_filename(std::string filepath_cpy) {
  std::string base = basename(filepath_cpy.data());
  int dot = find_char_last(base.data(), '.');

  return base.substr(0, dot);
}

std::string gen_channel_filepath(std::string_view filepath, std::string_view root, std::string_view stem, std::string_view md5) {

  auto filename = get_filename(std::string(filepath));

  filename = filename + '-' + std::string(stem) + "-" + std::string(md5) + "." + ext;
  
  return join_path(root.data(), filename.data());
}

bool all_channel_files_exist(std::string_view filepath, std::string_view root, std::string_view md5) {
  {
    auto dst = gen_channel_filepath(filepath, root, "vocals", md5);
    printf("dst is %s\n", dst.data());
    if (!file_exist(dst)) {
      return false;
    }
  }
  {
    auto dst = gen_channel_filepath(filepath, root, "no_vocals", md5);
    printf("dst is %s\n", dst.data());
    if (!file_exist(dst)) {
      return false;
    }
  }
  return true;
}

void separate_track(const Usb& usb, std::string_view filepath) {
  char command[1000];
  char model[] = "hdemucs_mmi";
  auto root = join_path(usb.root.data(), "finyl/separated");
  auto neoroot = join_path(root.data(), model);
  
  auto md5 = compute_md5(filepath);
  snprintf(command, sizeof(command), "demucs -n %s --two-stems=vocals \"%s\" -o %s --filename {track}-{stem}-%s.{ext}", model, filepath.data(), root.data(), md5.data());
  printf("\nRunning:\n");
  printf("\t%s\n", command);
  
  if (all_channel_files_exist(filepath, neoroot, md5)) {
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
  if (auto err = close_command(fp)) {
    print_err(err);
    return;
  }
}

void demucs_track(const Usb& usb, char* tid) {
  int _tid = atoi(tid);
  finyl_track_meta tm;
  getTrackMeta1(tm, usb, _tid);
  separate_track(usb, tm.filepath);
}

void demucs_playlists(const Usb& usb, char* pid) {
  std::vector<finyl_track_meta> tms;
  int _pid = atoi(pid);
  getPlaylistTrackMetas(tms, usb, _pid);
  
  for (int i = 0; i<tms.size(); i++) {
    separate_track(usb, tms[i].filepath);
  }
}

void print_usage() {
  printf("usage: ./separate <usb> <operation>\n\n");
  printf("\n");
  printf("This program generates vocals&inst for all tracks in playlist in rekordbox USB and place them under <usb>/finyl/separated/, where the main finyl program looks for stem files.\n");
  printf("operation:\n");
  printf("\tlist-playlists: list playlists\n");
  printf("\tlist-playlist-tracks <id>: list tracks in playlist <id>\n");
  printf("\tdemucs-playlist <id>: run demucs for playlist <id>\n");
  printf("\tdemucs-track <id>: run demucs for track <id>\n");
}

int main(int argc, char **argv) {
  if (argc < 3) {
    print_usage();
    return 0;
  }

  int err = plug(argv[1]);
  if (err == 1) {
    return 1;
  }
  Usb& usb = gApp.controller->usbs[0];
  
  char* op = argv[2];

  if (same(op, "list-playlists")) {
    printPlaylists(usb);
  } else if (same(op, "list-playlist-tracks")) {
    if (argc < 4) {
      print_usage();
      return 0;
    }

    int id = atoi(argv[3]);
    printPlaylistTracks(usb, id);
  } else if (same(op, "demucs-playlist")) {
    if (argc < 4) {
      print_usage();
      return 0;
    }
    char* id = argv[3];
    demucs_playlists(usb, id);
  } else if (same(op, "demucs-track")) {
    if (argc < 4) {
      print_usage();
      return 0;
    }

    char* id = argv[3];
    demucs_track(usb, id);
  } else {
    print_usage();
  }
}
