#include <shade/storage.h>
#include <shade/cache.h>
#include <shade/model/bridge.h>
#include "model/json.h"
#include "storage_internal.h"
#include <algorithm>
#include <cstdio>
#include <memory>

namespace json {
	JSON_STATIC_DECL(shade::model::bridge);
}

namespace shade { namespace storage {
	static auto& filename() {
		static auto name = build_filename();
		return name;
	}

	struct file {
		struct closer {
			void operator()(FILE* f) {
				std::fclose(f);
			}
		};
		using ptr = std::unique_ptr<FILE, closer>;

		static ptr open(const char* filename, const char* mode = "r") {
			return ptr{ std::fopen(filename, mode) };
		}
	};

	static inline std::string contents(FILE* f) {
		std::string out;
		char buffer[1024];
		while (size_t size = std::fread(buffer, 1, sizeof(buffer), f)) {
			out.append(buffer, size);
		}
		return out;
	}

	void load(cache& view)
	{
		auto in = file::open(filename().c_str());
		if (!in)
			return;

		std::decay_t<decltype(view.bridges())> bridges;

		auto object = json::from_string(contents(in.get()));
		if (!json::unpack(bridges, object)) {
			view.bridges({});
			return;
		}

		for (auto& pair : bridges) {
			pair.second->set_host(view.current_host());
			pair.second->from_storage(pair.first, view.browser());
		}

		view.bridges(std::move(bridges));
	}

	void store(const cache& view)
	{
		auto text = json::pack(view.bridges())
			.to_string(json::value::options::indented());

		auto out = file::open(filename().c_str(), "w");
		if (!out)
			return;
		fprintf(out.get(), "%s\n", text.c_str());
	}
} }
