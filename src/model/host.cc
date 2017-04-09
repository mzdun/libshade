#include "shade/model/host.h"
#include "json.h"
#include <algorithm>
#include <cstdio>
#include <memory>

namespace shade { namespace model {
	void host::prepare(json::struct_translator& tr)
	{
		using my_type = host;
		tr.OPT_PRIV_PROP(name);
		tr.OPT_PRIV_PROP(username);
		tr.OPT_PRIV_PROP(selected);
	}

	bool host::update(const std::string& user)
	{
		if (username_ == user)
			return false;
		username_ = user;
		return true;
	}

	bool host::is_selected(const std::string& dev) const
	{
		return selected_.count(dev) > 0;
	}

	void host::switch_selection(const std::string& dev)
	{
		auto sel = selected_.find(dev);
		if (sel == selected_.end())
			selected_.insert(dev);
		else
			selected_.erase(dev);

		store();
	}
} }
