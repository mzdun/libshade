#include "shade/storage.h"
#include <windows.h>
#include <memory>

namespace shade {
	auto getenv(const char* name) {
		auto size = GetEnvironmentVariableA(name, nullptr, 0);
		if (size) {
			auto buf = std::make_unique<char[]>(size);
			GetEnvironmentVariableA(name, buf.get(), size);
			return buf;
		}
		return std::unique_ptr<char[]>{};
	}

	std::string storage::build_filename() {
		std::string out;

		auto home = getenv("HOME");

		if (home)
			out.append(home.get());
		else {
			auto drive = getenv("HOMEDRIVE");
			auto path = getenv("HOMEPATH");

			if (!drive || !path)
				return {};

			out.append(drive.get());
			out.append(path.get());
		}

		if (out.back() != '\\')
			out.push_back('\\');
		out.append(confname);

		return out;
	}
}
