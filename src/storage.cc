#include "shade/storage.h"
#include "json.hpp"
#include <cstdio>
#include <memory>

namespace json {
	JSON_STRUCT(shade::light_state) {
		JSON_PROP(on);
		JSON_PROP(bri);
		JSON_PROP(hue);
		JSON_PROP(sat);
		JSON_PROP(ct);
		JSON_PROP(xy);
		JSON_PROP(mode);
	};

	JSON_STRUCT_(shade::group_state, shade::light_state) {
		JSON_PROP(any);
	};

	JSON_STRUCT(shade::info_base) {
		JSON_PROP(id);
		JSON_PROP(name);
		JSON_PROP(type);
	};

	JSON_STRUCT_(shade::light_info, shade::info_base) {
		JSON_PROP(state);
	};

	JSON_STRUCT_(shade::group_info, shade::info_base) {
		JSON_OPT_NAMED_PROP("class", klass);
		JSON_PROP(state);
		JSON_PROP(lights);
	};

	JSON_STRUCT(shade::bridge_info) {
		JSON_PROP(base);
		JSON_OPT_PROP(username);
		JSON_OPT_PROP(name);
		JSON_OPT_PROP(mac);
		JSON_OPT_PROP(modelid);
		JSON_OPT_PROP(lights);
		JSON_OPT_PROP(groups);
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
		if (!object.is<json::MAP>())
			return;

		if (!json::unpack(known_bridges, object))
			known_bridges.clear();
	}

	void storage::store()
	{
		auto text = json::pack(known_bridges)
			.to_string(json::value::options::indented());

		auto out = file::open(filename().c_str(), "w");
		if (!out)
			return;
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

	void storage::bridge_named(const std::string& id,
		const std::string& name, const std::string& mac,
		const std::string& modelid)
	{
		store_at_exit needs_storing{ this };
		auto it = known_bridges.find(id);
		if (it == known_bridges.end()) {
			auto& bridge = known_bridges[id];
			bridge.name = name;
			bridge.mac = mac;
			bridge.modelid = modelid;
			needs_storing = true;
		} else {
			auto& bridge = it->second;
			if (bridge.name != name
				|| bridge.mac != mac
				|| bridge.modelid != modelid) {
				bridge.name = name;
				bridge.mac = mac;
				bridge.modelid = modelid;
				needs_storing = true;
			}
		}
	}

	void storage::bridge_connected(const std::string& id, const std::string& username)
	{
		store_at_exit needs_storing{ this };
		auto it = known_bridges.find(id);
		if (it == known_bridges.end()) {
			known_bridges[id].username = username;
			needs_storing = true;
		} else {
			if (it->second.username != username) {
				it->second.username = username;
				needs_storing = true;
			}
		}
	}

	void storage::bridge_config(const std::string& id,
		std::vector<light_info> lights,
		std::vector<group_info> groups)
	{
		store_at_exit needs_storing{ this };
		auto it = known_bridges.find(id);
		if (it == known_bridges.end()) {
			auto& bridge = known_bridges[id];
			bridge.lights = std::move(lights);
			bridge.groups = std::move(groups);
			needs_storing = true;
		} else {
			auto& bridge = it->second;
			if (bridge.lights != lights
				|| bridge.groups != groups) {
				bridge.lights = std::move(lights);
				bridge.groups = std::move(groups);
				needs_storing = true;
			}
		}
	}

}
