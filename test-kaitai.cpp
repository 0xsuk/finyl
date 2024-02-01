#include <stdio.h>
#include <fstream>
#include "rekordbox_pdb.h"

// https://github.com/mixxxdj/mixxx/blob/034eb6c56a9af855ef5ce824841a113752ea4726/src/library/rekordbox/rekordboxfeature.cpp#L493

int main() {
  std::string filepath = "/run/media/null/71CD-A534/PIONEER/rekordbox/export.pdb";
  std::ifstream ifs(filepath, std::ifstream::binary);
  kaitai::kstream ks(&ifs);

  rekordbox_pdb_t rekordboxDB = rekordbox_pdb_t(&ks);
  constexpr int totalTables = 8;
  rekordbox_pdb_t::page_type_t tableOrder[totalTables] = {
    rekordbox_pdb_t::PAGE_TYPE_KEYS,
    rekordbox_pdb_t::PAGE_TYPE_GENRES,
    rekordbox_pdb_t::PAGE_TYPE_ARTISTS,
    rekordbox_pdb_t::PAGE_TYPE_ALBUMS,
    rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_ENTRIES,
    rekordbox_pdb_t::PAGE_TYPE_TRACKS,
    rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_TREE,
    rekordbox_pdb_t::PAGE_TYPE_HISTORY};
  
  for (int tableOrderIndex = 0; tableOrderIndex < totalTables; tableOrderIndex++) {
    for (std::vector<rekordbox_pdb_t::table_t*>::iterator table = rekordboxDB.tables()->begin();
         table != rekordboxDB.tables()->end();
         ++table) {
      printf("enum : %d\n", (*table)->type());
      if ((*table)->type() != tableOrder[tableOrderIndex]) {
        continue;
      }

      uint16_t lastIndex = (*table)->last_page()->index();
      rekordbox_pdb_t::page_ref_t* currentRef = (*table)->first_page();
    }
  }
}
