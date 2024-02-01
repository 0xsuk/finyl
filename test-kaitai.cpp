#include <stdio.h>
#include <fstream>
#include "rekordbox_pdb.h"

// https://github.com/mixxxdj/mixxx/blob/034eb6c56a9af855ef5ce824841a113752ea4726/src/library/rekordbox/rekordboxfeature.cpp#L493

int main() {
  std::string filepath = "/run/media/null/71CD-A534/PIONEER/rekordbox/export.pdb";
  std::ifstream ifs(filepath, std::ifstream::binary);
  kaitai::kstream ks(&ifs);

  rekordbox_pdb_t rekordboxDB = rekordbox_pdb_t(&ks);
  
  for (std::vector<rekordbox_pdb_t::table_t*>::iterator table = rekordboxDB.tables()->begin();
       table != rekordboxDB.tables()->end();
       ++table) {
    printf("enum : %d\n", (*table)->type());
  }
}
