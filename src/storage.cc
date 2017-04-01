#include "shade/storage.h"
#include "json.hpp"
#include <cstdio>
#include <memory>

namespace json {
	JSON_STRUCT(shade::bridge_info) {
		JSON_PROP(base);
		JSON_OPT_PROP(username);
	}
}

namespace shade {
	class store_at_exit {
		bool store = false;
		storage* parent;
	public:
		store_at_exit(storage* parent) : parent{ parent } {}
		~store_at_exit() {
			if (store)
				parent->store();
		}

		auto& operator=(bool b) {
			store = b;
			return *this;
		}

		explicit operator bool() const {
			return store;
		}
	};

	struct file {
		struct closer {
			void operator()(FILE* f){
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
	void storage::load()
	{
		auto in = file::open(filename().c_str());
		if (!in)
			return;

		auto object = json::from_string(contents(in.get()));
		if (object.get_type() != json::MAP)
			return;

		if (!json::unpack(known_bridges, object))
			known_bridges.clear();
	}

	void storage::store()
	{
		auto out = file::open(filename().c_str(), "w");
		if (!out)
			return;

		auto text = json::pack(known_bridges)
			.to_string(json::value::options::indented());
		fprintf(out.get(), "%s\n", text.c_str());
	}

	bool storage::bridge_located(const std::string& id, const std::string& base) {
		store_at_exit needs_storing{ this };
		auto it = known_bridges.find(id);
		if (it == known_bridges.end()) {
			known_bridges[id].base = base;
			needs_storing = true;
		} else {
			if (it->second.base != base) {
				it->second.base = base;
				needs_storing = true;
			}
		}

		// if storage needs refreshing, this is a new/refreshed bridge
		return !!needs_storing;
	}
}
