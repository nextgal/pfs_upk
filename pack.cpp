/*
This file is part of pfs_upk.

pfs_upk is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version. 

pfs_upk is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
pfs_upk. If not, see <https://www.gnu.org/licenses/>.
*/

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "pfs_upk.hpp"

using namespace std;

bool pack(std::string path) {
  const filesystem::path fs_path = path;
  vector<Artemis_Entry> index;
  Artemis_Header header;
  header.index_size = 0; // init
  header.file_count = 0;

  for (auto const &entry : filesystem::recursive_directory_iterator{fs_path}) {
    // construct list
    if (entry.is_directory() != true) {
      // get VFS path
#ifdef _WIN32
      wstring ir_path = entry.path().wstring();
      size_t pos = ir_path.find(fs_path.wstring());
      ir_path.erase(pos, fs_path.wstring().size() + 1); // + "\"
#else
      string ir_path = entry.path().u8string();
      size_t pos = ir_path.find(fs_path.u8string());
      ir_path.erase(pos, fs_path.string().size() + 1); // + "\"
#endif // _WIN32
      string vfs_path =
          filesystem::path(std::filesystem::path(ir_path)).u8string();
#ifdef _DEBUG
      cout << "vfs_path: " << vfs_path << endl;
#endif // _DEBUG

      // get info
      Artemis_Entry artemis_entry;
      artemis_entry.size = (uint32_t)filesystem::file_size(entry);
      artemis_entry.path = vfs_path;
#ifdef _WIN32
      artemis_entry.local_path = entry.path().generic_wstring();
#else
      artemis_entry.local_path = entry.path().generic_u8string();
#endif // _WIN32

      index.push_back(artemis_entry);

      header.index_size += (uint32_t)vfs_path.size();
      header.index_size += (4 * sizeof(uint32_t));

      header.file_count++;
    }
  }
  // cout << "File count: " << header.file_count << endl;

  // construct header
  header.pack_version = '8'; //	it's literal NOT just a int.
  header.magic[0] = ARCHIVE_MAGIC[0];
  header.magic[1] = ARCHIVE_MAGIC[1];

  // calculate index size
  header.index_size += 1 * sizeof(uint32_t); // index_size itself

  header.index_size += 1 * sizeof(uint32_t); // entry count in offset table
  header.index_size +=
      header.file_count * (2 * sizeof(uint32_t)); // offset table
  header.index_size += 2 * sizeof(uint32_t);      // offset table EOF
  header.index_size += 1 * sizeof(uint32_t);      // table pos

  // prepare index
  char *buf = new char[header.index_size +
                       sizeof(Artemis_Header)]; // buf = Artemis_Header +
                                                // Artemis_Index + Offset_Table
  char *memptr = buf;
  memcpy(buf, &header, sizeof(Artemis_Header));
  memptr += sizeof(Artemis_Header);
  char *posIndexStart = memptr - sizeof(uint32_t);

  uint32_t offset = header.index_size + sizeof(Artemis_Header) -
                    sizeof(uint32_t); // init offset

  for (auto &file : index) {
    uint32_t filename_size = (uint32_t)file.path.size();
    char *filename = new char[file.path.size()];
    memcpy(filename, file.path.c_str(), file.path.size());

    uint32_t reserved = 0x00000000;

    memcpy(memptr, &filename_size, sizeof(uint32_t)); // write filename size
    memptr += sizeof(uint32_t); // NEVER NOT forget to move the pointer!
    memcpy(memptr, filename, file.path.size()); // write filename
    delete[] filename;
    memptr += file.path.size();
    file.position = memptr;
    memcpy(memptr, &reserved, sizeof(uint32_t)); // write resevred info
    memptr += sizeof(uint32_t);
    memcpy(memptr, &offset, sizeof(uint32_t)); // write offset
    offset += file.size;                       // update file offset
    memptr += sizeof(uint32_t);
    memcpy(memptr, &(file.size), sizeof(uint32_t)); // write file size
    memptr += sizeof(uint32_t);
  }

  const char *posOffsetTable = memptr; // the end of index entry
  // write entry count for offset table
  uint32_t offset_count = header.file_count + 1;
  memcpy(memptr, &offset_count, sizeof(uint32_t));
  memptr += sizeof(uint32_t);

  // prepare offset table
  for (auto const &file : index) {
    uint32_t reserved = 0x00000000;
    uint32_t posOffset =
        uint32_t(file.position - posIndexStart); // not include index_size
    memcpy(memptr, &posOffset, sizeof(uint32_t));
    memptr += sizeof(uint32_t);
    memcpy(memptr, &reserved, sizeof(uint32_t));
    memptr += sizeof(uint32_t);
  }
  // write EOF of offset table
  memset(memptr, 0x00000000, sizeof(uint32_t) * 2);
  memptr += sizeof(uint32_t) * 2;
  // write table position
  uint32_t tablepos = uint32_t(
      posOffsetTable - (buf + sizeof(Artemis_Header) - sizeof(uint32_t)));
  memcpy(memptr, &tablepos, sizeof(uint32_t));
  memptr += sizeof(uint32_t);

  // Then,calculate the XOR key.
  //
  char xor_key[20] = {0x00};
  SHA1_CTX hash;
  char *xor_buf = new char[header.index_size];
  memcpy(xor_buf, posIndexStart, header.index_size);
  SHA1Init(&hash);
  SHA1Update(&hash, (unsigned char *)xor_buf, header.index_size);
  SHA1Final((unsigned char *)xor_key, &hash);
  delete[] xor_buf;

  // Well done, the index is prepared.
  // It's time to de the real things.
  // & the hard one (thanks to FS path parse)
  //

  string arcname = fs_path.filename().string(); // the PFS filename for output
#ifdef _DEBUG
  cout << "[DEBUG]fs_path: " << fs_path << endl;
  cout << "[DEBUG]fs_path.parent_path(): " << fs_path.parent_path() << endl;
  cout << "[DEBUG]arcname: " << arcname << endl;
  cout << fs_path.parent_path() / (arcname + ".pfs") << endl;
#endif
  cout << "Packing \"" << fs_path.string() << "\" ... ";
  cout << header.file_count << " file(s)." << endl;

  std::fstream arc(fs_path.parent_path() / (arcname + ".pfs"),
                   ios::binary | ios::out);

  if (arc.is_open() != true) {
    return false;
  } else {
    // write header & index
    arc.write(buf, (size_t)header.index_size + sizeof(Artemis_Header) -
                       sizeof(uint32_t));
    delete[] buf; // Okay to free buffer.

    for (auto const &file : index) {
      char *buf = new char[file.size];

#ifdef _WIN32
      fstream handle(filesystem::path(file.local_path).wstring(),
                     ios::in | ios::binary);
#else
      fstream handle(filesystem::path(file.local_path).string(),
                     ios::in | ios::binary);
#endif // _WIN32

      handle.read(buf, file.size);
      handle.close();
      // don't forget to XOR encrypt it.
      xorcrypt(buf, file.size, xor_key, 20);
      // write back
      arc.write(buf, file.size);

      delete[] buf;
    }
    arc.close();
  }

  return true;
}
