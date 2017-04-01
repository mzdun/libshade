#include "shade/storage.h"
#include <cstdlib>

namespace shade {
	constexpr char storage::confname[]; // why, gcc? It's a constexpr...
	std::string storage::build_filename() {
		std::string out;

		const auto home = std::getenv("HOME");

		if (home)
			out.append(home);
		else
			out.push_back('.');

		if (out.back() != '/')
			out.push_back('/');
		out.append(confname);

		return out;
	}
}
