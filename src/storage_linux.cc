#include <shade/storage.h>
#include <cstdlib>
#include "storage_internal.h"

namespace shade { namespace storage {
	// constexpr char storage::confname[]; // why, gcc? It's a constexpr...
	std::string build_filename() {
		std::string out;

		const auto home = std::getenv("HOME");

		if (home)
			out.append(home);
		else
			out.push_back('.');

		if (out.back() != '/')
			out.push_back('/');
		out.append(SHADE_STORAGE_CONFNAME);

		return out;
	}
} }
