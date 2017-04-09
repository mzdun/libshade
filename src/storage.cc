#include "shade/storage.h"
#include "model/json.h"
#include <algorithm>
#include <cstdio>
#include <memory>

namespace json {
	JSON_STATIC_DECL(shade::model::bridge);
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
		if (!json::unpack(known_bridges, object)) {
			known_bridges.clear();
			return;
		}

		for (auto& pair : known_bridges) {
			pair.second.set_host(clientid, this);
		}
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
			auto& bridge = known_bridges[id];
			bridge.set_host(clientid, this);
			bridge.hw.base = base;
			needs_storing = true;
		} else {
			if (it->second.hw.base != base) {
				it->second.hw.base = base;
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
			bridge.set_host(clientid, this);
			bridge.seen = true;
			bridge.hw.name = name;
			bridge.hw.mac = mac;
			bridge.hw.modelid = modelid;
			needs_storing = true;
		} else {
			auto& bridge = it->second;
			bridge.seen = true;
			if (bridge.hw.name != name
				|| bridge.hw.mac != mac
				|| bridge.hw.modelid != modelid) {
				bridge.hw.name = name;
				bridge.hw.mac = mac;
				bridge.hw.modelid = modelid;
				needs_storing = true;
			}
		}
	}

	void storage::bridge_connected(const std::string& id, const std::string& username)
	{
		store_at_exit needs_storing{ this };
		auto it = known_bridges.find(id);
		if (it == known_bridges.end()) {
			auto& bridge = known_bridges[id];
			bridge.set_host(clientid, this);
			needs_storing = bridge.get_current()->update(username);
		} else {
			auto& bridge = it->second;
			needs_storing = bridge.get_current()->update(username);
		}
	}

	void storage::bridge_config(const std::string& id,
		vector_shared<model::light> lights,
		vector_shared<model::group> groups)
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
