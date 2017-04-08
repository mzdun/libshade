#include "shade/storage.h"
#include "json.hpp"
#include <algorithm>
#include <cstdio>
#include <memory>

#define JSON_STATIC_DECL(name) \
	template <> \
	struct translator<name> : struct_translator { \
		using my_type = name; \
		translator() { my_type::prepare(*this); } \
	}

#define NAMED_PROP(name, prop) \
	add_prop(name, &my_type::prop)

#define PROP(prop) NAMED_PROP(#prop, prop)
#define PRIV_PROP(prop) NAMED_PROP(#prop, prop ## _)

#define OPT_NAMED_PROP(name, prop) \
	add_opt_prop(name, &my_type::prop)

#define OPT_PROP(prop) OPT_NAMED_PROP(#prop, prop)
#define OPT_PRIV_PROP(prop) OPT_NAMED_PROP(#prop, prop ## _)

#define ITEM_NAMED_PROP(name, prop) \
	add_item_prop(name, &my_type::prop)

#define ITEM_PROP(prop) ITEM_NAMED_PROP(#prop, prop)

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

	JSON_STRUCT(shade::hw_info) {
		JSON_PROP(base);
		JSON_OPT_PROP(name);
		JSON_OPT_PROP(mac);
		JSON_OPT_PROP(modelid);
	}

	JSON_STATIC_DECL(shade::client);
	JSON_STATIC_DECL(shade::bridge_info);
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

	void client::prepare(json::struct_translator& tr)
	{
		using my_type = client;
		tr.OPT_PRIV_PROP(name);
		tr.OPT_PRIV_PROP(username);
		tr.OPT_PRIV_PROP(selected);
	}

	bool client::update(const std::string& user)
	{
		if (username_ == user)
			return false;
		username_ = user;
		return true;
	}

	bool client::is_selected(const std::string& dev) const
	{
		return selected_.count(dev) > 0;
	}

	void client::switch_selection(const std::string& dev)
	{
		auto sel = selected_.find(dev);
		if (sel == selected_.end())
			selected_.insert(dev);
		else
			selected_.erase(dev);

		store();
	}

	void bridge_info::prepare(json::struct_translator& tr)
	{
		using my_type = bridge_info;
		tr.PROP(hw);
		tr.OPT_PROP(lights);
		tr.OPT_PROP(groups);
		tr.OPT_PRIV_PROP(clients);
	}

	void bridge_info::set_client(const std::string& name, storage* stg)
	{
		using std::begin;
		using std::end;

		auto it = std::find_if(begin(clients_), end(clients_),
			[&](const auto& cli) { return cli.name() == name; });
		if (it == end(clients_))
			it = clients_.insert(end(clients_), client{ name });
		current_ = &*it;
		current_->set(stg);
	}

	void bridge_info::rebind_current()
	{
		using std::begin;
		using std::end;

		auto name = current_->name();

		auto it = std::find_if(begin(clients_), end(clients_),
			[&](const auto& cli) { return cli.name() == name; });
		current_ = &*it;
	}

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
			pair.second.set_client(clientid, this);
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
			bridge.set_client(clientid, this);
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
			bridge.set_client(clientid, this);
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
			bridge.set_client(clientid, this);
			needs_storing = bridge.get_current()->update(username);
		} else {
			auto& bridge = it->second;
			needs_storing = bridge.get_current()->update(username);
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
