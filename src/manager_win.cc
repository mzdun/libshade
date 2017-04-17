#include <utf8.hpp>
#include <windows.h>

namespace shade {
	std::string machine()
	{
		char16_t buffer[256] = {};
		DWORD dwSize = sizeof(buffer);
		if (!GetComputerNameExW(ComputerNameDnsHostname, (wchar_t*)buffer, &dwSize))
			return {};
		return utf::narrowed(buffer);
	}
}
