#include "shade/model/bridge.h"
#include "shade/hue_data.h"
#include "json.h"
#include <algorithm>
#include <cstdio>
#include <memory>

namespace json {
	JSON_STATIC_DECL(shade::model::light);
	JSON_STATIC_DECL(shade::model::group);

	JSON_STRUCT(shade::model::hw_info) {
		JSON_PROP(base);
		JSON_OPT_PROP(name);
		JSON_OPT_PROP(mac);
		JSON_OPT_PROP(modelid);
	}

	JSON_STATIC_DECL(shade::model::host);
}

namespace shade { namespace model {
	using std::begin;
	using std::end;

	struct lights_environment : json::named_translator, json::inplace_translator
	{
		std::string empty_;
		const std::string& name() const override { return empty_; }
		json::value pack(const void* ctx, json::ctx_env& env) override { return {}; }
		bool unpack(const json::value& v, void* ctx, json::ctx_env& env) override { return false; }
		void clean(void* ctx) const override { }
		inplace_translator* inplace() override { return this; }
		void pack(json::map& out, const void* ctx, json::ctx_env& env) override
		{
			auto& lights = static_cast<const bridge*>(ctx)->lights();
			env["lights"] = (void*)(const void*)&lights;
		}

		bool unpack(const json::map& out, void* ctx, json::ctx_env& env) override
		{
			auto& lights = static_cast<bridge*>(ctx)->lights();
			env["lights"] = (void*)&lights;
			return true;
		}
	};

	bridge::bridge(const std::string& id, io::http* browser)
		: id_{ id }
		, browser_{ browser }
	{
	}

	void bridge::from_storage(const std::string& id, io::http* browser)
	{
		id_ = id;
		browser_ = browser;
	}

	void bridge::set_host(const std::string& name)
	{
		using std::begin;
		using std::end;

		auto it = std::find_if(begin(hosts_), end(hosts_), [&](const auto& ptr) { return ptr && ptr->name() == name; });
		if (it != end(hosts_)) {
			current_ = *it;
			return;
		}

		hosts_.push_back(current_ = std::make_shared<model::host>(name));
	}

	void bridge::prepare(json::struct_translator& tr)
	{
		using my_type = bridge;
		tr.PRIV_PROP(hw);
		tr.OPT_PRIV_PROP(lights);
		tr.add(std::make_unique<lights_environment>());
		tr.OPT_PRIV_PROP(groups);
		tr.OPT_PRIV_PROP(hosts);
	}

	bool bridge::bridge_lights(
		std::unordered_map<std::string, hue::light> lights,
		std::unordered_map<std::string, hue::group> groups)
	{
		for (auto& group : groups) {
			for (auto& lightref : group.second.lights) {
				auto it = lights.find(lightref);
				if (it == lights.end()) {
					lightref.clear();
				}
				else
					lightref = it->second.uniqueid;
			}
		}

		return update_lights(std::move(lights)) || update_groups(std::move(groups));
	}

	bool bridge::update_lights(std::unordered_map<std::string, hue::light> lights)
	{
		bool needs_update = false;
		vector_shared<light> still_existing;
		for (auto const& source : lights_) {
			auto const& id = source->id();
			auto it = std::find_if(begin(lights), end(lights), [&id](const auto& pair) {
				return pair.second.uniqueid == id;
			});
			if (it == end(lights)) {
				// TODO: notify removal...
				continue;
			}
			auto updated = source->update(it->first, std::move(it->second));
			// TODO: if (updated) notify about updated source
			needs_update |= updated;
			still_existing.push_back(source);
		}

		for (auto const& in : lights) {
			auto const& id = in.second.uniqueid;
			if (id.empty())
				continue;

			printf("[NEW] Looking for '%s'... ", id.c_str());
			auto it = std::find_if(begin(lights_), end(lights_), [&id](const auto& ptr) {
				return ptr->id() == id;
			});

			if (it != end(lights_)) {
				printf("DONE [%s:%s]\n", (*it)->id().c_str(), (*it)->name().c_str());
				continue;
			}
			printf("DONE [--none--]\n");

			auto new_source = model::light::make(
				std::move(in.second.uniqueid),
				std::move(in.second.name),
				std::move(in.second.modelid),
				in.second.state.on,
				in.second.state.bri,
				model::color_mode::from_json(in.second.state)
			);
			needs_update = true;
			still_existing.push_back(new_source);
			// TODO: notify about new source
		}

		std::swap(lights_, still_existing);
		return needs_update;
	}

	bool bridge::update_groups(std::unordered_map<std::string, hue::group> groups)
	{
		bool needs_update = false;
		vector_shared<group> still_existing;
		for (auto const& source : groups_) {
			auto const& id = source->id();
			auto it = std::find_if(begin(groups), end(groups), [&id](const auto& pair) {
				return "group/" + pair.first == id;
			});
			if (it == end(groups)) {
				// TODO: notify removal...
				continue;
			}
			auto updated = source->update(it->first, std::move(it->second), lights_);
			// TODO: if (updated) notify about updated source
			needs_update |= updated;
			still_existing.push_back(source);
		}

		for (auto const& in : groups) {
			auto it = std::find_if(begin(groups_), end(groups_), [id = "group/" + in.first](const auto& ptr) {
				return ptr->id() == id;
			});

			if (it != end(groups_))
				continue;

			auto new_source = model::group::make(
				"group/" + in.first,
				std::move(in.second.name),
				std::move(in.second.type),
				std::move(in.second.klass),
				in.second.state.all_on,
				in.second.state.any_on,
				in.second.action.bri,
				model::color_mode::from_json(in.second.action),
				model::group::referenced(in.second.lights, lights_)
			);
			needs_update = true;
			still_existing.push_back(new_source);
			// TODO: notify about new source
		}

		std::swap(groups_, still_existing);
		return needs_update;
	}
} }
