#include <shade/model/group.h>
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
			printf("refs_translator: %s\n", v.to_string().c_str());

			if (!v.is<json::VECTOR>()) {
				clean(ctx);
				return true;
			}

			auto it = env.find("lights");
			if (it == env.end() || !it->second) {
				clean(ctx);
				return true;
			}

			printf("refs_translator: lights -> %p\n", it->second);
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

	void group::prepare(json::struct_translator& tr)
	{
		light_source::prepare(tr);
		using my_type = group;
		tr.OPT_PRIV_PROP(some);
		tr.OPT_PRIV_PROP(klass);
		tr.add(std::make_unique<refs_translator>());
	}
} }
