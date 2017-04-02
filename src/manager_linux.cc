#include <sys/utsname.h>
#include <string>

namespace shade {
	std::string machine()
	{
		struct utsname buffer;
		if (uname(&buffer))
			return {};

		return buffer.nodename;
	}
}
