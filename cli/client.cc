#include "client.h"
#include <iostream>

void print(const shade::light_info& light, bool is_new)
{
	printf("        [%s]: '%s' (%s) %s [%d]", light.id.c_str(),
		light.name.c_str(), light.type.c_str(),
		light.state.on ? "ON" : "OFF",
		light.state.bri);

	if (is_new) printf(" NEW LIGHT");
	printf("\n");
}

void print(const shade::group_info& group, bool is_new)
{
	printf("        [%s]: '%s' (%s) %s [%d]", group.id.c_str(),
		group.name.c_str(),
		group.type == "Room" ? group.klass.c_str() : group.type.c_str(),
		group.state.any ? (group.state.on ? "ON" : "SOME") : "OFF",
		group.state.bri);

	if (is_new) printf(" NEW GROUP");
	printf("\n");
}

client::client(boost::asio::io_service& service)
	: menu_{ service }
{
}

void client::on_previous(std::unordered_map<std::string, shade::bridge_info> const& bridges)
{
	if (bridges.empty()) {
		printf("WAITING FOR BRIDGES...\n");
	} else {
		local_bridges_ = bridges;
		for (auto& bridge : local_bridges_) {
			bridge.second.selected.clear();
		}

		for (auto const& bridge : bridges) {
			printf("BRIDGE %s:\n", bridge.first.c_str());
			printf("    base:     %s\n", bridge.second.base.c_str());
			printf("    username: %s\n", bridge.second.username.c_str());
			printf("    name:     %s\n", bridge.second.name.c_str());
			printf("    mac:      %s\n", bridge.second.mac.c_str());
			printf("    modelid:  %s\n", bridge.second.modelid.c_str());
			if (!bridge.second.lights.empty()) {
				printf("    lights:\n");
				for (auto const& light : bridge.second.lights) {
					print(light, false);
				}
			}
			if (!bridge.second.groups.empty()) {
				printf("    groups:\n");
				for (auto const& group : bridge.second.groups) {
					print(group, false);
					for (auto& ref : group.lights) {
						printf("            -> %s\n", ref.c_str());
					}
				}
			}
			if (!bridge.second.selected.empty()) {
				printf("    selected:\n");
				for (auto const& ref : bridge.second.selected) {
					printf("        -> %s\n", ref.c_str());
				}
			}
		}
	}
}

void client::on_bridge(std::string const& bridgeid, bool known)
{
	auto const& bridge = manager_->bridge(bridgeid);
	auto& local = local_bridges_[bridgeid];
	printf("... >> %s %s\n", known ? "ACTIVATING" : "FOUND", bridgeid.c_str());
	if (bridge.base != local.base) {
		printf("    base: [%s]\n", bridge.base.c_str());
		local.base = bridge.base;
	}
}

void client::on_bridge_named(std::string const& bridgeid)
{
	auto const& bridge = manager_->bridge(bridgeid);
	auto& local = local_bridges_[bridgeid];
	printf("... >> GOT BASIC CONFIG\n");
	if (bridge.name != local.name) {
		printf("    name: [%s]\n", bridge.name.c_str());
		local.name = bridge.name;
	}
	if (bridge.mac != local.mac) {
		printf("    mac: [%s]\n", bridge.mac.c_str());
		local.mac = bridge.mac;
	}
	if (bridge.modelid != local.modelid) {
		printf("    modelid: [%s]\n", bridge.modelid.c_str());
		local.modelid = bridge.modelid;
	}
}

void client::on_connecting(std::string const&)
{
	printf("PLEASE PRESS THE BUTTON...\n");
}

void client::on_connected(std::string const& bridgeid)
{
	auto const& bridge = manager_->bridge(bridgeid);
	auto& local = local_bridges_[bridgeid];
	printf("THANK YOU.\n");
	printf("    username: [%s]\n", bridge.username.c_str());
}

void client::on_connect_timeout(std::string const& bridgeid)
{
	printf("BUTTON NOT PRESSED IN 30s.\n");
}

template <typename T>
static auto find_if(const T& container, const std::string& id)
{
	using std::begin;
	using std::end;

	return std::find_if(begin(container), end(container), [&](auto const& item) { return item.id == id; });
}

template <typename T>
void print_list(T& local, const T& update, const char* title)
{
	using std::begin;
	using std::end;

	bool removed = false;
	bool header = false;
	for (auto& loc : local) {
		auto it = find_if(update, loc.id);
		if (it == end(update)) {
			if (!header) {
				header = true;
				printf("    %s:\n", title);
			}
			printf("        [%s] REMOVED\n", loc.id.c_str());
			removed = true;
			continue;
		}
		if (*it != loc) {
			loc = *it;
			if (!header) {
				header = true;
				printf("    %s:\n", title);
			}
			print(loc, false);
		}
	}

	if (removed) {
		auto it = std::remove_if(begin(local), end(local), [&](const auto& item) { return find_if(update, item.id) == end(update); });
		local.erase(it, end(local));
		local.shrink_to_fit();
	}

	for (auto const& upd : update) {
		auto it = find_if(local, upd.id);
		if (it == end(local)) {
			if (!header) {
				header = true;
				printf("    %s:\n", title);
			}
			print(upd, true);
			local.push_back(upd);
			continue;
		}
	}
}

class first_screen : public menu::current {
	client* parent_;
	std::string title_;
	std::vector<std::unique_ptr<menu::impl::item>> items_;

	bool device_list_changed();
	void rebuild_list();
	template <typename C>
	size_t count_devices(const C& list, const shade::bridge_info& bridge, size_t& distinct_bridges, bool& first)
	{
		size_t length = 0;
		auto const& selected = bridge.selected;

		for (auto const& item : list) {
			if (!selected.count(item.id))
				continue;
			++length;

			if (first) {
				first = false;
				title_ = distinct_bridges ? "Multiple bridges" : bridge.name;
				++distinct_bridges;
			}
		}

		return length;
	}
public:
	first_screen(client* parent)
		: parent_{ parent }
	{
	}

	void refresh() override;
	menu::label title() const override { return title_; }
	size_t count() const override { return items_.size(); }
	menu::item_type type(size_t i) override { return items_[i]->type(); }
	menu::label text(size_t i) override { return items_[i]->text(); }
	void call(size_t i) override { return items_[i]->call(); }
};

inline auto device_state(const std::string& text)
{
	return [=](int bri) {
		constexpr int max_bri = 254;

		std::string out;
		out.reserve(2 + 4 + text.length());
		out.push_back('[');
		if (bri > max_bri) {
			out.push_back(' ');
			out.push_back('-');
			out.push_back('-');
			out.push_back(' ');
		} else {
			char buf[10];
			sprintf(buf, "%3d%%", (bri * 100) / max_bri);
			out.append(buf);
		}
		out.push_back(']');
		out.push_back(' ');
		out.append(text);
		return out;
	};
}

inline auto device_brightness(shade::manager* manager, const std::string& bridgeid, const std::string& deviceid)
{
	return [=]() -> int {
		auto& bridge = manager->bridge(bridgeid);

		for (auto const& item : bridge.groups) {
			if (deviceid != item.id)
				continue;
			if (!item.state.on)
				return 0x100;
			return item.state.bri;
		}

		for (auto const& item : bridge.lights) {
			if (deviceid != item.id)
				continue;
			if (!item.state.on)
				return 0x100;
			return item.state.bri;
		}

		return 0x100;
	};
}

inline auto tick_out(const std::string& text)
{
	return [=](bool selected) {
		std::string out;
		out.reserve(5 + text.length());
		out.push_back('[');
		out.push_back(selected ? 'X' : ' ');
		out.push_back(']');
		out.push_back(' ');
		out.append(text);
		return out;
	};
}

inline auto is_selected(shade::manager* manager, const std::string& bridgeid, const std::string& deviceid)
{
	return [=]() -> bool { return manager->is_selected(bridgeid, deviceid); };
}

void first_screen::refresh()
{
	if (device_list_changed()) {
		rebuild_list();
		return;
	}

	for (auto& item : items_)
		item->refresh();
}

bool first_screen::device_list_changed()
{
	bool changed = false;
	auto manager = parent_->manager_;
	for (auto const& pair : manager->bridges()) {
		auto& selected = pair.second.selected;
		auto& local = parent_->local_bridges_[pair.first].selected;

		if (selected != local) {
			local = selected;
			changed = true;
		}
	}

	return changed;
}

void first_screen::rebuild_list()
{
	using namespace menu::impl;
	auto manager = parent_->manager_;

	title_.clear();

	std::vector<std::unique_ptr<item>> items;
	size_t length = 0;
	size_t distinct_bridges = 0;
	for (auto const& pair : manager->bridges()) {
		auto const& bridgeid = pair.first;
		auto const& bridge = pair.second;

		auto first = true;

		length += count_devices(bridge.groups, bridge, distinct_bridges, first);
		length += count_devices(bridge.lights, bridge, distinct_bridges, first);
	}
	items.reserve(length + 2);

	struct counter {
		bool first = true;
		void set(int& devices, std::string& title, const shade::bridge_info& bridge) {
			if (first) {
				first = false;
				title = devices ? "Multiple bridges" : bridge.name;
				++devices;
			}
		}
	};
	int devices = 0;
	for (auto const& pair : manager->bridges()) {
		auto bridgeid = pair.first;
		auto const& bridge = pair.second;
		counter cnt;

		for (auto const& group : bridge.groups) {
			auto id = group.id;
			if (!bridge.selected.count(id))
				continue;

			cnt.set(devices, title_, bridge);
			items.push_back(item::simple(
				device_brightness(manager, bridgeid, id),
				device_state(group.name),
				[=]() {}
			));
		}
	}
	items.push_back(item::separator());
	for (auto const& pair : manager->bridges()) {
		auto bridgeid = pair.first;
		auto const& bridge = pair.second;
		counter cnt;

		for (auto const& light : bridge.lights) {
			auto id = light.id;
			if (!bridge.selected.count(id))
				continue;

			cnt.set(devices, title_, bridge);
			items.push_back(item::simple(
				device_brightness(manager, bridgeid, id),
				device_state(light.name),
				[=]() {}
			));
		}
	}

	if (!devices)
		title_ = "Choose devices";

	auto dummy = std::make_shared<bool>(true);
	items.push_back(item::special(
		[dummy]() { return false; },
		tick_out("Refresh state"),
		[dummy]() {}));
	items.push_back(item::special("Select devices", [this]() { parent_->select_items(); }));

	items_.swap(items);
}

void client::on_refresh(std::string const& bridgeid)
{
	auto const& bridge = manager_->bridge(bridgeid);
	auto& local = local_bridges_[bridgeid];
	printf("... >> GOT CURRENT CONFIG\n");
	print_list(local.lights, bridge.lights, "lights");
	print_list(local.groups, bridge.groups, "groups");

	menu_.show_menu(std::make_unique<first_screen>(this));
}

void client::select_items()
{
	using namespace menu::impl;
	std::vector<std::unique_ptr<item>> items;
	size_t length = 0;
	for (auto const& bridge : manager_->bridges()) {
		length += bridge.second.groups.size();
		length += bridge.second.lights.size();
	}
	items.reserve(length + 1);

	for (auto const& bridge : manager_->bridges()) {
		auto bridgeid = bridge.first;
		for (auto const& group : bridge.second.groups) {
			auto id = group.id;
			items.push_back(item::simple(
				is_selected(manager_, bridgeid, id),
				tick_out(group.name),
				[=]() { switch_device(bridgeid, id); }
			));
		}
	}
	items.push_back(item::separator());
	for (auto const& bridge : manager_->bridges()) {
		auto bridgeid = bridge.first;
		for (auto const& light : bridge.second.lights) {
			auto id = light.id;
			items.push_back(item::simple(
				is_selected(manager_, bridgeid, id),
				tick_out(light.name),
				[=]() { switch_device(bridgeid, id); }
			));
		}
	}
	menu_.show_menu(std::make_unique<current<>>("Select devices", std::move(items)));
}

void client::switch_device(const std::string& bridgeid, const std::string& id)
{
	manager_->switch_selection(bridgeid, id);
}
