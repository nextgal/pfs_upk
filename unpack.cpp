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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "pfs_upk.hpp"

using namespace std;

bool unpack(std::string path) {
  Artemis_Header header;

  ifstream arc(path, ios::in | ios::binary);
  arc.read((char *)&header, sizeof(header));

  // check if a vaild Artemis archive
  if (memcmp(ARCHIVE_MAGIC, header.magic, ARCHIVE_MAGIC_SIZE)) { // Check magic
    cout << "Invaild Artemis PFS archive!" << endl;
    return false;
  }
  cout << "Found ";

  // look for PFS revision
  switch (header.pack_version) {
  case '2':
    cout << "vaild PFS Version 2 archive." << endl;
    break;
  case '6':
    cout << "vaild PFS Version 6 archive." << endl;
    break;
  case '8':
    cout << "vaild PFS Version 8 archive." << endl;
    break;
  default:
    cout << "a unknown PFS version,and the program will exit." << endl;
    return false;
  }

  // read for file counts & index size
  cout << "Index size: 0x" << std::hex << header.index_size << "\t";
  cout << "File Count: 0x" << std::hex << header.file_count << endl;
  cout << endl;

  // add it into vector!
  vector<Artemis_Entry> index(header.file_count);

  // A PFS Entry:
  // uint32_t     FILENAME_LENGTH
  // char         FILENAME[FILENAME_LENGTH]
  // uint32_t     RESERVED    // keep 0x00000000
  // uint32_t     OFFSET
  // uint32_t     FILE_SIZE
  //

  for (uint32_t i = 1; i <= header.file_count; ++i) {
    Artemis_Entry entry;

    // get path length
    uint32_t path_length = 0;
    arc.read((char *)&path_length, sizeof(uint32_t));

    // get path

    char *buf = new char[MAX_PATH];
    arc.read(buf, path_length); // NOTICE: it's UTF-8 string.
    buf[path_length] = 0x00;    // cut off string with 0x00
    entry.path = buf;
    delete[] buf;

    // reserved or unknown.
    arc.seekg(sizeof(uint32_t), ios::cur);

    // get size and offset
    arc.read(reinterpret_cast<char *>(&entry.offset), sizeof(uint32_t));
    arc.read(reinterpret_cast<char *>(&entry.size), sizeof(uint32_t));

#ifdef _DEBUG
    // pring filename for debugging
    // cout << entry.path;
    // cout << std::hex << "\t" << "Offset: 0x" << entry.offset << "\t" <<
    // "Size: 0x" << entry.size << endl;
#endif // _DEBUG

    // add to index
    index.push_back(entry);
  }

  // get basepath for unpack
  filesystem::path pfspath(path);         // /foo/bar/foobar.pfs
  filesystem::path stem = pfspath.stem(); // foobar
  pfspath = pfspath.parent_path();        // /foo/bar/
  filesystem::path new_fs_prefix =
      pfspath / stem / ""; // extract to /foo/bar/foobar/<VFS_PATH>
#ifdef DEBUG
  cout << "FIles will oupput to: " << new_fs_prefix << endl;
#endif // DEBUG

  // compute SHA1 for XOR decrypt
  // read header (again)
  char *header_buf = new char[header.index_size];
  arc.seekg(sizeof(Artemis_Header) - sizeof(uint32_t), ios::beg);
  arc.read(header_buf, header.index_size);
  char xor_key[20] = {0};
  SHA1_CTX hash;
  SHA1Init(&hash);
  SHA1Update(&hash, (unsigned char *)header_buf, header.index_size);
  SHA1Final((unsigned char *)xor_key, &hash);
  delete[] header_buf;

#ifdef _WIN32
  uint32_t counter = 0;
#endif // _WIN32

  for (vector<Artemis_Entry>::iterator it = index.begin(); it != index.end();
       it++) {
    Artemis_Entry file = *it;
    // skip if entry is empty
    if (file.offset == 0) {
      continue;
    } else {
      // create folders when not exist
      filesystem::path vfs_subfix = filesystem::u8path(file.path);
      filesystem::path newpath = new_fs_prefix;
#ifndef _WIN32
      string vfs_subfix_posix = vfs_subfix.string();
      std::replace(vfs_subfix_posix.begin(), vfs_subfix_posix.end(), '\\', '/');
      newpath /= filesystem::u8path(vfs_subfix_posix);
#else
      newpath /= vfs_subfix;
#endif
      filesystem::create_directories(newpath.parent_path());
      // open file
#ifdef _WIN32
      // extra infomation for Windows
      wchar_t Buffer[32];
      swprintf(Buffer, 32, L"%u / %u", counter, header.file_count);
      SetConsoleTitleW(Buffer);
      counter++;
#endif // _WIN32
      fstream handle(newpath, ios::binary | ios::out);
      cout << "Extracting \"" << file.path.c_str() << "\" ..." << endl;
      if (handle.is_open() != true) {
        cout << "We encountered a problem while extracting the file." << endl;
        return false;
      } else {
        BYTE *buf = new BYTE[file.size];
        arc.seekg(file.offset, ios::beg); // seek to offset
        arc.read((char *)buf, file.size);
        // decrypt
        xorcrypt((char *)buf, file.size, xor_key, 20);
        // write back
        handle.write((char *)buf, file.size);
        delete[] buf;
        handle.close();
      }
    }
  }
  return true;
}