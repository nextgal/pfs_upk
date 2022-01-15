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

#include <cstdint>
bool xorcrypt(char *text, uint32_t text_length, char *key,
              uint32_t key_length) {

  for (uint32_t i = 0; i < text_length; ++i) {
    text[i] ^= key[i % key_length];
  }
  return true;
}