#include<vector>
#include<memory>
#include"MemoryStream.h"
#include<cstring>

using namespace std;

BYTE* MemoryStream::GetBase()
{
	return _base.get();
}
const uint MemoryStream::GetPosition() const
{
	return Position;
}
const size_t MemoryStream::GetSize() const
{
	return Length;
}
const BYTE* MemoryStream::GetBase() const
{
	return _base.get();
}
BYTE* MemoryStream::GetPtr()
{
	return _base.get() + Position;
}
const BYTE* MemoryStream::GetPtr() const
{
	return _base.get() + Position;
}

void MemoryStream::Close()
{
	_base.reset();
}
void MemoryStream::Dispose()
{
	Close();
}
long MemoryStream::Seek(unsigned int _offset, int _Mode)
{
	switch (_Mode)
	{
	case MEM_BEG:
	{
		Position = _offset;
		break;
	}
	case MEM_CUR:
	{
		Position += _offset;
		break;
	}
	case MEM_END:
	{
		Position = Length - _offset;
		break;
	}
	default:
	{
		break;
	}
	}
	return Position;
}
long MemoryStream::Read(vector<BYTE>& _buffer, unsigned int _size)
{
	Read(_buffer.data(), _size);
	return _size;
}
long MemoryStream::Read(void* _buffer, unsigned int _size)
{
	memcpy(_buffer, _base.get() + Position, _size);
	Position += _size;
	return _size;
}
template<typename _Ty>
void MemoryStream::Write(const vector<_Ty>& _source)
{
	Write(_source.data(), _source.size());
}
template<typename _Ty>
void MemoryStream::Write(size_t _Pos, const _Ty* _Src, size_t _Size)
{
	if (Position + _Size * sizeof(_Ty) / sizeof(BYTE) > Length) return;
	memcpy(_base.get() + _Pos, _Src, _Size * sizeof(_Ty) / sizeof(BYTE));
	Position += _Size;
}
template<typename _Ty>
void MemoryStream::Write(const _Ty* _Src, size_t _Size)
{
	Write(Position, (BYTE*)_Src, _Size);
}
void MemoryStream::Write(const MemoryStream& _Src)
{
	Write(Position, _Src.GetBase(), _Src.Length);
}
void MemoryStream::Write(const MemoryStream* _Src)
{
	Write(*(_Src));
}
void MemoryStream::Write(const char* _Src, size_t _Size)
{
	Write(Position, (BYTE*)_Src, _Size);
}
void MemoryStream::Write(const unsigned int* _Src, size_t _Size)
{
	Write(Position, (BYTE*)_Src, _Size);
}
void MemoryStream::Write(const int _Val, size_t _Size)
{
	const BYTE* _Ptr = (BYTE*)&_Val;
	for (size_t i = 0; i < _Size; i++)
	{
		Write(_Ptr, sizeof(BYTE));
	}
}
MemoryStream::MemoryStream(BYTE* _Ptr, unsigned int _size)
{
	Position = 0;
	Length = _size;
	_base = std::unique_ptr<BYTE>(_Ptr);
}
MemoryStream::MemoryStream(const vector<BYTE>& _source)
{
	std::unique_ptr<BYTE> _real(new BYTE[_source.size()]);
	Length = _source.size();
	Position = 0;
	memcpy(_real.get(), _source.data(), _source.size() * sizeof(BYTE));
	_base = std::move(_real);
}
MemoryStream::~MemoryStream()
{
	this->Dispose();
}
