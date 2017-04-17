#pragma once
#include <string>

namespace shade {
	class cache;
}

namespace shade { namespace storage {
	static constexpr char confname[] = ".shade.cfg";
	std::string build_filename();
	void load(cache& view);
	void store(const cache& view);
} }
