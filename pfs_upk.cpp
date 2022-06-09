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

int main(int argc, char *argv[]) {
  try {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif // _WIN32
#ifndef _TESTMODE
    // check params
    if (argc < 2) {
      cout << "Artemis C++ Packer ---- A Simple C++ Artemis Engine Archive "
              "Packer / Unpacker."
           << endl;
      cout << "Copyright 2022-present @ryank231231." << endl;
      cout << "Usage:" << endl;
      cout << argv[0] << " "
           << "<ARTEMIS ARCHIVE> / <FOLDER>" << endl;

      exit(EXIT_SUCCESS);
    }
#else
    argv[1] = "C:\\vn_temp\\保\\pfs";
#endif
    // check is path or file
    filesystem::path path;
    try {
      path = filesystem::canonical(argv[1]);
    } catch (
        filesystem::filesystem_error) { // when the path is already canonical.
      path = argv[1];
    }

    if (filesystem::is_directory(path)) {
      if (pack(path.string()) == false) {
        cout << "Pack failed!" << endl;
        exit(EXIT_FAILURE);
      }
    } else {
      if (unpack(path.string()) == false) {
        cout << "Unpack failed!" << endl;
        exit(EXIT_FAILURE);
      }
    }

    exit(EXIT_SUCCESS);
  } catch (std::exception &e) {
    cout << "Caught a C++ exception. ";
    cout << e.what() << endl;
  }
}
