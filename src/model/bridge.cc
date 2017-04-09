#include "shade/model/bridge.h"
#include "json.h"
#include <algorithm>
#include <cstdio>
#include <memory>

namespace json {
	JSON_STRUCT(shade::model::light_state) {
		JSON_PROP(on);
		JSON_PROP(bri);
		JSON_PROP(hue);
		JSON_PROP(sat);
		JSON_PROP(ct);
		JSON_PROP(xy);
		JSON_PROP(mode);
	};

	JSON_STRUCT_(shade::model::group_state, shade::model::light_state) {
		JSON_PROP(any);
	};

	JSON_STRUCT(shade::model::info_base) {
		JSON_PROP(id);
		JSON_PROP(name);
		JSON_PROP(type);
	};

	JSON_STRUCT_(shade::model::light_info, shade::model::info_base) {
		JSON_PROP(state);
	};

	JSON_STRUCT_(shade::model::group_info, shade::model::info_base) {
		JSON_OPT_NAMED_PROP("class", klass);
		JSON_PROP(state);
		JSON_PROP(lights);
	};

	JSON_STRUCT(shade::model::hw_info) {
		JSON_PROP(base);
		JSON_OPT_PROP(name);
		JSON_OPT_PROP(mac);
		JSON_OPT_PROP(modelid);
	}

	JSON_STATIC_DECL(shade::model::host);
}

namespace shade { namespace model {
	void bridge::prepare(json::struct_translator& tr)
	{
		using my_type = bridge;
		tr.PROP(hw);
		tr.OPT_PROP(lights);
		tr.OPT_PROP(groups);
		tr.OPT_PRIV_PROP(hosts);
	}

	void bridge::set_host(const std::string& name, storage* stg)
	{
		using std::begin;
		using std::end;

		auto it = std::find_if(begin(hosts_), end(hosts_),
			[&](const auto& cli) { return cli.name() == name; });
		if (it == end(hosts_))
			it = hosts_.insert(end(hosts_), host{ name });
		current_ = &*it;
		current_->set(stg);
	}

	void bridge::set_host_from(const host& upstream)
	{
		using std::begin;
		using std::end;

		auto it = std::find_if(begin(hosts_), end(hosts_),
			[&](const auto& cli) { return cli.name() == upstream.name(); });
		if (it == end(hosts_))
			it = hosts_.insert(end(hosts_), host{ upstream.name() });
		current_ = &*it;
		current_->set(upstream.get_storage());
	}

	void bridge::rebind_current()
	{
		using std::begin;
		using std::end;

		auto name = current_->name();

		auto it = std::find_if(begin(hosts_), end(hosts_),
			[&](const auto& cli) { return cli.name() == name; });
		current_ = &*it;
	}
} }
