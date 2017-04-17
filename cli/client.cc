#include "client.h"
#include <iostream>

using std::begin;
using std::end;

class first_screen : public menu::current {
	client* parent_;
	std::string title_;
	std::vector<std::unique_ptr<menu::impl::item>> items_;

	bool device_list_changed();
	void rebuild_list();

	template <typename C>
	size_t count_devices(const C& list, const shade::model::host& host, const shade::model::hw_info& hw, size_t& distinct_bridges)
	{
		size_t length = 0;
		auto const& selected = host.selected();

		for (auto const& item : list) {
			if (!selected.count(item->id()))
				continue;
			++length;
		}

		return length;
	}
public:
	first_screen(client* parent)
		: parent_{ parent }
	{
		rebuild_list();
	}

	void refresh() override;
	menu::label title() const override { return title_; }
	size_t count() const override { return items_.size(); }
	menu::item_type type(size_t i) override { return items_[i]->type(); }
	menu::label text(size_t i) override { return items_[i]->text(); }
	void call(size_t i) override { return items_[i]->call(); }
};

inline auto device_state(const std::shared_ptr<shade::model::light_source>& src)
{
	return [=](int bri) {
		constexpr int max_bri = 254;

		std::string out;
		out.reserve(2 + 4 + src->name().length());
		out.push_back('[');
		if (bri > shade::model::mode::max_value) {
			out.push_back(' ');
			out.push_back('-');
			out.push_back('-');
			out.push_back(' ');
		}
		else {
			char buf[10];
			sprintf(buf, "%3d%%", (bri * 100) / max_bri);
			out.append(buf);
		}
		out.push_back(']');
		out.push_back(' ');
		out.append(src->name());
		return out;
	};
}

inline auto device_brightness(const std::shared_ptr<shade::model::light_source>& source)
{
	return [=]() -> int {
		if (!source->on())
			return 0x100;
		return source->bri();
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

inline auto is_selected(shade::model::host& host, const std::string& deviceid)
{
	return [=]() -> bool { return host.is_selected(deviceid); };
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
	for (auto const& pair : manager->view().bridges()) {
		auto& selected = pair.second->host().selected();
		auto& local = parent_->local_[pair.first].selected;
		auto const& client = parent_->manager_->current_host();

		if (selected != local) {
			parent_->local_[pair.first].selected = selected;
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
	for (auto const& pair : manager->view().bridges()) {
		auto const& bridgeid = pair.first;
		auto const& bridge = pair.second;

		auto const& host = bridge->host();
		length += count_devices(bridge->groups(), host, bridge->hw(), distinct_bridges);
		length += count_devices(bridge->lights(), host, bridge->hw(), distinct_bridges);
	}
	items.reserve(length + 2);

	struct counter {
		bool first = true;
		void set(int& devices, std::string& title, const shade::model::bridge& bridge) {
			if (first) {
				first = false;
				title = devices ? "Multiple bridges" : bridge.hw().name;
				++devices;
			}
		}
	};
	int devices = 0;
	std::unordered_set<std::string> dummy;
	for (auto const& pair : manager->view().bridges()) {
		auto bridgeid = pair.first;
		auto const& bridge = pair.second;
		auto const& selection = bridge->host().selected();
		counter cnt;

		for (auto const& group : bridge->groups()) {
			auto id = group->id();
			if (!selection.count(id))
				continue;

			cnt.set(devices, title_, *bridge);
			items.push_back(item::simple(
				device_brightness(group),
				device_state(group),
				[=]() {}
			));
		}
	}
	items.push_back(item::separator());
	for (auto const& pair : manager->view().bridges()) {
		auto bridgeid = pair.first;
		auto const& bridge = pair.second;
		auto const& selection = bridge->host().selected();
		counter cnt;

		for (auto const& light : bridge->lights()) {
			auto id = light->id();
			if (!selection.count(id))
				continue;

			cnt.set(devices, title_, *bridge);
			items.push_back(item::simple(
				device_brightness(light),
				device_state(light),
				[=]() {}
			));
		}
	}

	if (!devices)
		title_ = "Choose devices";

	items.push_back(item::special(
		[=]() { return parent_->heartbeat_on(); },
		tick_out("Refresh state"),
		[=]() { parent_->heartbeat(); }));
	items.push_back(item::special("Select devices", [this]() { parent_->select_items(); }));

	items_.swap(items);
}

enum class source {
	same,
	added,
	changed,
	removed
};

void print(const shade::model::light& light, source type)
{
	printf("        [%s]: '%s' (%s) %s [%d%%]", light.id().c_str(),
		light.name().c_str(), light.type().c_str(),
		light.on() ? "ON" : "OFF",
		shade::model::mode::clamp(light.bri()) * 100 / shade::model::mode::max_value);

	switch (type) {
	case source::added: printf(" NEW LIGHT"); break;
	case source::removed: printf(" REMOVED LIGHT"); break;
	}

	printf("\n");
}

void print(const shade::model::group& group, source type)
{
	printf("        [%s]: '%s' (%s) %s [%d%%]", group.id().c_str(),
		group.name().c_str(),
		group.type() == "Room" ? group.klass().c_str() : group.type().c_str(),
		group.some() ? (group.on() ? "ON" : "SOME") : "OFF",
		shade::model::mode::clamp(group.bri()) * 100 / shade::model::mode::max_value);

	switch (type) {
	case source::added: printf(" NEW GROUP"); break;
	case source::removed: printf(" REMOVED GROUP"); break;
	}

	printf("\n");
}

void print(const shade::model::light_source& light, source type)
{
	if (light.is_group())
		print((const shade::model::group&)light, type);
	else
		print((const shade::model::light&)light, type);
}

client::client(boost::asio::io_service& service)
	: menu_{ service }
{
}

// shade::listener::manager
void client::onload(const shade::cache& view)
{
	if (view.bridges().empty()) {
		printf("WAITING FOR BRIDGES...\n");
	} else {
		for (auto& pair : view.bridges()) {
			auto& ref = local_[pair.first];
			ref.hw = pair.second->hw();
			ref.selected.clear();
		}

		for (auto const& bridge : view.bridges()) {
			printf("BRIDGE %s:\n", bridge.first.c_str());
			auto & hw = bridge.second->hw();
			auto & client = bridge.second->host();
			printf("    base:     %s\n", hw.base.c_str());
			printf("    name:     %s\n", hw.name.c_str());
			printf("    mac:      %s\n", hw.mac.c_str());
			printf("    modelid:  %s\n", hw.modelid.c_str());
			printf("    client:   %s\n", client.name().c_str());
			printf("    username: %s\n", client.username().c_str());
			if (!bridge.second->lights().empty()) {
				printf("    lights:\n");
				for (auto const& light : bridge.second->lights()) {
					print(*light, source::same);
				}
			}
			if (!bridge.second->groups().empty()) {
				printf("    groups:\n");
				for (auto const& group : bridge.second->groups()) {
					print(*group, source::same);
					for (auto& ref : group->lights()) {
						printf("            -> %s\n", ref->id().c_str());
					}
				}
			}
			if (!client.selected().empty()) {
				printf("    selected:\n");
				for (auto const& ref : client.selected())
					printf("        -> %s\n", ref.c_str());
			}
		}
	}

	menu_.show_menu(std::make_unique<first_screen>(this));
}

void client::onbridge(const std::shared_ptr<shade::model::bridge>& bridge)
{
	auto& local = local_[bridge->id()];
	printf("... >> GOT BASIC CONFIG\n");
	auto& hw = bridge->hw();
	if (hw.base != local.hw.base) {
		printf("    base: [%s]\n", hw.base.c_str());
		local.hw.base = hw.base;
	}
	if (hw.name != local.hw.name) {
		printf("    name: [%s]\n", hw.name.c_str());
		local.hw.name = hw.name;
	}
	if (hw.mac != local.hw.mac) {
		printf("    mac: [%s]\n", hw.mac.c_str());
		local.hw.mac = hw.mac;
	}
	if (hw.modelid != local.hw.modelid) {
		printf("    modelid: [%s]\n", hw.modelid.c_str());
		local.hw.modelid = hw.modelid;
	}
	menu_.refresh();

	if (bridge->host().username().empty()) {
		manager_->connect(bridge);
		return;
	}

	// TODO: heartbeat for bridge
}

// shade::listener::connection
void client::onconnecting(const std::shared_ptr<shade::model::bridge>&)
{
	printf("\nPLEASE PRESS THE BUTTON...\n");
}

void client::onconnected(const std::shared_ptr<shade::model::bridge>& bridge)
{
	printf("THANK YOU.\n");
	printf("    username: [%s]\n", bridge->host().username().c_str());
	menu_.refresh();
	// TODO: heartbeat for bridge
}

void client::onfailed(const std::shared_ptr<shade::model::bridge>&)
{
	printf("BUTTON NOT PRESSED IN 30s.\n");
	menu_.cancel();
}

void client::onlost(const std::shared_ptr<shade::model::bridge>& bridge)
{
	printf("CONNECTION LOST.\n");
	menu_.refresh();
}

// shade::listener::bridge
void client::update_start(const std::shared_ptr<shade::model::bridge>&)
{
	printf("... >> GOT CURRENT CONFIG\n");
}

void client::source_added(const std::shared_ptr<shade::model::light_source>& light)
{
	print(*light, source::added);
}

void client::source_removed(const std::shared_ptr<shade::model::light_source>& light)
{
	print(*light, source::removed);
}

void client::source_changed(const std::shared_ptr<shade::model::light_source>& light)
{
	print(*light, source::changed);
}

void client::update_end(const std::shared_ptr<shade::model::bridge>&)
{
	menu_.refresh();
}

bool client::heartbeat_on() const
{
	return fake_hb;
}

void client::heartbeat()
{
	fake_hb = !fake_hb;
}

void client::select_items()
{
	using namespace menu::impl;
	std::vector<std::unique_ptr<item>> items;
	size_t length = 0;
	for (auto const& bridge : manager_->view().bridges()) {
		length += bridge.second->groups().size();
		length += bridge.second->lights().size();
	}
	items.reserve(length + 1);

	for (auto& bridge : manager_->view().bridges()) {
		auto& host = bridge.second->host();
		for (auto const& group : bridge.second->groups()) {
			auto id = group->id();
			items.push_back(item::simple(
				is_selected(host, id),
				tick_out(group->name()),
				[&]() { host.switch_selection(id); manager_->store_cache(); }
			));
		}
	}
	items.push_back(item::separator());
	for (auto const& bridge : manager_->view().bridges()) {
		auto& host = bridge.second->host();
		for (auto const& light : bridge.second->lights()) {
			auto id = light->id();
			items.push_back(item::simple(
				is_selected(host, id),
				tick_out(light->name()),
				[&]() { host.switch_selection(id); manager_->store_cache(); }
			));
		}
	}
	menu_.show_menu(std::make_unique<current<>>("Select devices", std::move(items)));
}
