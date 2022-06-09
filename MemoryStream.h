#pragma once
#define MEM_BEG 0
#define MEM_CUR 1
#define MEM_END 2
#include<iostream>
#include<vector>
#include<memory>
#include"csharp_types.h"


class MemoryStream
{
private:
	std::unique_ptr<BYTE> _base;
public:
	uint Position = 0;
	uint Length = 0;

	BYTE* GetBase();
	const BYTE* GetBase() const;
	const uint GetPosition() const;
	const size_t GetSize() const;
	BYTE* GetPtr();
	const BYTE* GetPtr() const;

	long Seek(unsigned int _offset, int _Mode = MEM_BEG);

	long Read(std::vector<BYTE>& _buffer, unsigned int _size);
	long Read(void* _buffer, unsigned int _size);

	template<typename _Ty>
	void Write(const std::vector<_Ty>& _source);
	template<typename _Ty>
	void Write(size_t _Pos, const _Ty* _Src, size_t _Size);
	template<typename _Ty>
	void Write(const _Ty* _Src, size_t _Size);
	void Write(const MemoryStream& _Src);			//Similar with CopyTo();
	void Write(const MemoryStream* _Src);			//Similar with CopyTo();
	void Write(const char* _Src, size_t _Size);		//Special Judgement, don't know why doesn't work with template
	void Write(const unsigned int* _Src, size_t _Size);
	void Write(const int _Val, size_t _Size);		//Similar with memset(), Param _Val only receives int and will be converted to BYTE;

	void Insert(const size_t _offset, const std::vector<BYTE>& _source);
	void Insert(const size_t _offset, const void* _source, size_t _Size);

	void Erase(const size_t _offset, size_t _Size);

	MemoryStream(BYTE* _Ptr, unsigned int _size);
	MemoryStream(const std::vector<BYTE>& _source);
	~MemoryStream();
	void Close();
	void Dispose();
};