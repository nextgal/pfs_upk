#include <Windows.h>
#undef min
#undef max
#include <string>
using std::string;
using std::wstring;

wstring AnsiToUnicode(const string& source, int codePage)
{
	if (source.length() == 0) return wstring();
	if (source.length() > (size_t)std::numeric_limits<int>::max()) return wstring();

	int length = MultiByteToWideChar(codePage, 0, source.c_str(), (int)source.length(), NULL, 0);
	if (length <= 0) return wstring();
	wstring output(length, L'\0');
	if (MultiByteToWideChar(codePage, 0, source.c_str(), (int)source.length(), (LPWSTR)output.data(), (int)output.length() + 1) == 0) return wstring();

	return output;
}

string UnicodeToAnsi(const wstring& source, int codePage)
{
	if (source.length() == 0) return string();
	if (source.length() > (size_t)std::numeric_limits<int>::max()) return string();

	int length = WideCharToMultiByte(codePage, 0, source.c_str(), (int)source.length(), NULL, 0, NULL, NULL);
	if (length <= 0)return string();
	string output(length, '\0');
	if (WideCharToMultiByte(codePage, 0, source.c_str(), (int)source.length(), (LPSTR)output.data(), (int)output.length() + 1, NULL, NULL) == 0) return string();

	return output;
}