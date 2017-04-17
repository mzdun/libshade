#include <shade/model/group.h>
#include <shade/hue_data.h>
#include <algorithm>
#include "model/json.h"

namespace shade { namespace model {
	bool group::operator == (const group& rhs) const
	{
		return *(const light_source*)this == rhs
			&& some_ == rhs.some_
			&& klass_ == rhs.klass_
			&& range_equal(lights_, rhs.lights_);
	}

	struct refs_translator : json::named_translator
	{
		std::string name_ = "lights";
		const std::string& name() const override { return name_; }
		virtual bool optional() const { return true; }
		json::value pack(const void* ctx, json::ctx_env& env) override
		{
			json::vector v;
			for (auto & ref : static_cast<const model::group*>(ctx)->lights())
				v.add(ref->id());
			return v;
		}
		bool unpack(const json::value& v, void* ctx, json::ctx_env& env) override
		{
			if (!v.is<json::VECTOR>()) {
				clean(ctx);
				return true;
			}

			auto it = env.find("lights");
			if (it == env.end() || !it->second) {
				clean(ctx);
				return true;
			}

			auto& all = *static_cast<const vector_shared<light>*>(it->second);

			vector_shared<light> out;
			json::vector in{ v };
			out.reserve(in.size());

			for (auto const& ref : in) {
				auto& sref = ref.as<json::STRING>();
				std::shared_ptr<light> ptr;
				for (auto& light : all) {
					if (light->id() == sref) {
						out.push_back(light);
						break;
					}
				}
			}
			static_cast<model::group*>(ctx)->lights(std::move(out));
			return true;
		}
		void clean(void* ctx) const override
		{
			static_cast<model::group*>(ctx)->lights({});
		}
	};

	static inline bool equal(const vector_shared<light>& lhs, const vector_shared<light>& rhs) {
		if (lhs.size() != rhs.size())
			return false;

		for (auto& l : lhs) {
			auto const& id = l->id();
			auto it = std::find_if(begin(rhs), end(rhs), [&id](auto& r) {
				return id == r->id();
			});
			if (it == end(rhs))
				return false;
		}
		return true;
	}

#define UPDATE_SOURCE(name, data) \
	if (name() != data) { \
		updated = true; \
		name(data); \
	}
	bool group::update(const std::string& key, hue::group json, const vector_shared<light>& resource)
	{
		bool updated = false;
		auto mode = color_mode::from_json(json.action);
		auto brightness = mode::clamp(json.action.bri);
		auto key_id = "group/" + key;
		auto refs = referenced(json.lights, resource);

		UPDATE_SOURCE(index, key);
		UPDATE_SOURCE(id, key_id);
		UPDATE_SOURCE(name, json.name);
		UPDATE_SOURCE(type, json.type);
		UPDATE_SOURCE(on, json.state.all_on);
		UPDATE_SOURCE(bri, brightness);
		UPDATE_SOURCE(value, mode);

		UPDATE_SOURCE(klass, json.klass);
		UPDATE_SOURCE(some, json.state.any_on);

		if (!equal(lights(), refs)) {
			updated = true;
			lights(refs);
		}

		return updated;
	}
#undef UPDATE_SOURCE

	void group::prepare(json::struct_translator& tr)
	{
		light_source::prepare(tr);
		using my_type = group;
		tr.OPT_PRIV_PROP(some);
		tr.OPT_PRIV_PROP(klass);
		tr.add(std::make_unique<refs_translator>());
	}

	vector_shared<light> group::referenced(const std::vector<std::string>& lights, const vector_shared<light>& resource)
	{
		vector_shared<model::light> refs;
		refs.reserve(lights.size());
		for (auto const& light : lights) {
			for (auto& ref : resource) {
				if (ref->id() == light)
					refs.push_back(ref);
			}
		}

		return refs;
	}

} }
