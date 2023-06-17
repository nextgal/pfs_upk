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

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "pfs_upk.hpp"
#include "MemoryStream.h"

using namespace std;

bool pack(const filesystem::path& path) {
	const filesystem::path fs_path = path;
	vector<Artemis_Entry> index;
	Artemis_Header header;
	header.index_size = 0; // init
	header.file_count = 0;

	for (auto const& entry : filesystem::recursive_directory_iterator{ fs_path }) {
		// construct list
		if (entry.is_directory() != true) {
			// get VFS path
			puString ir_path = puString(entry.path());
			size_t pos = ir_path.find(puString(fs_path));
			ir_path.erase(pos, fs_path.wstring().size() + 1); // + "\"
			puString vfs_path =
				puString(filesystem::path(std::filesystem::path(ir_path)));
#ifdef _DEBUG
#ifdef _WIN32
			wcout << L"vfs_path: " << vfs_path << endl;
#else
			cout << "vfs_path: " << vfs_path << endl;
#endif // _WIN32

#endif // _DEBUG

			// get info
			Artemis_Entry artemis_entry;
			artemis_entry.size = (uint32_t)filesystem::file_size(entry);
#ifdef _WIN32
			artemis_entry.path = UnicodeToAnsi(vfs_path, CP_UTF8);
#else
			artemis_entry.path = vfs_path;
#endif // _WIN32
			artemis_entry.local_path = puString(entry.path());

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
	MemoryStream* buf = new MemoryStream(new BYTE[header.index_size + sizeof(Artemis_Header)], 
		header.index_size + sizeof(Artemis_Header)); // buf = Artemis_Header +
												  // Artemis_Index + Offset_Table
	//BYTE* memptr = buf->GetPtr();
	buf->Write((BYTE*) & header, sizeof(Artemis_Header));
	BYTE* posIndexStart = buf->GetPtr() - sizeof(uint32_t);

	uint32_t offset = header.index_size + sizeof(Artemis_Header) -
		sizeof(uint32_t); // init offset

	for (auto& file : index) {
		uint32_t filename_size = (uint32_t)file.path.size();

		MemoryStream* filename = new MemoryStream(new BYTE[file.path.size()], file.path.size());
		filename->Write(file.path.data(), file.path.size());

		uint32_t reserved = 0x00000000;

		buf->Write(&filename_size, sizeof(uint32_t));
		buf->Write(filename);
		
		filename->Dispose();
		file.position = (char*)buf->GetPtr();

		buf->Write(&reserved, sizeof(uint32_t));		// write resevred info
		buf->Write(&offset, sizeof(uint32_t));			// write offset
		
		offset += file.size;							// update file offset

		buf->Write(&(file.size), sizeof(uint32_t));		// write file size
	}

	const char* posOffsetTable = (char*)buf->GetPtr();	// the end of index entry
	// write entry count for offset table
	uint32_t offset_count = header.file_count + 1;
	buf->Write(&offset_count, sizeof(uint32_t));

	// prepare offset table
	for (auto const& file : index) {
		uint32_t reserved = 0x00000000;
		uint32_t posOffset =
			uint32_t(file.position - (char*)posIndexStart); // not include index_size
		buf->Write(&posOffset, sizeof(uint32_t));
		buf->Write(&reserved, sizeof(uint32_t));
	}
	// write EOF of offset table
	buf->Write(0x00000000, sizeof(uint32_t) * 2);
	// write table position
	uint32_t tablepos = uint32_t(
		posOffsetTable - ((char*)buf->GetBase() + sizeof(Artemis_Header) - sizeof(uint32_t)));
	buf->Write(&tablepos, sizeof(uint32_t));

	// Then,calculate the XOR key.
	//
	char xor_key[20] = { 0x00 };
	SHA1_CTX hash;
	char* xor_buf = new char[header.index_size];
	memcpy(xor_buf, posIndexStart, header.index_size);
	SHA1Init(&hash);
	SHA1Update(&hash, (unsigned char*)xor_buf, header.index_size);
	SHA1Final((unsigned char*)xor_key, &hash);
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
	}
	else {
		// write header & index
		arc.write((char*)buf->GetBase(), (size_t)header.index_size + sizeof(Artemis_Header) -
			sizeof(uint32_t));
		buf->Dispose(); // Okay to free buffer.

		for (auto const& file : index) {
			char* buf = new char[file.size];

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
