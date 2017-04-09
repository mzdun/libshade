#include <shade/model/light_source.h>
#include "model/json.h"

namespace json {
}

namespace shade { namespace model {
	namespace mode {
		color::color() = default;
		color::color(const color&) = default;
		color::color(color&&) = default;
		color& color::operator=(const color&) = default;
		color& color::operator=(color&&) = default;

		bool color::operator == (const color& rhs) const
		{
			if (mode_ != rhs.mode_)
				return false;

			switch (mode_) {
			case mode::hue_sat:
				return hue_.hue == rhs.hue_.hue
					&& hue_.sat == rhs.hue_.sat;
			case mode::ct:
				return ct_.val == rhs.ct_.val;
			case mode::xy:
				return xy_.x == rhs.xy_.x
					&& xy_.y == rhs.xy_.y;
			}

			return false;
		}

		color::color(const hue_sat& col)
			: mode_{ mode::hue_sat }
			, hue_{ col }
		{
		}

		color::color(hue_sat&& col)
			: mode_{ mode::hue_sat }
			, hue_{ std::move(col) }
		{
		}

		color::color(const ct& col)
			: mode_{ mode::ct }
			, ct_{ col }
		{
		}

		color::color(ct&& col)
			: mode_{ mode::ct }
			, ct_{ std::move(col) }
		{
		}

		color::color(const xy& col)
			: mode_{ mode::xy }
			, xy_{ col }
		{
		}

		color::color(xy&& col)
			: mode_{ mode::xy }
			, xy_{ std::move(col) }
		{
		}
	}

	bool light_source::operator == (const light_source& rhs) const
	{
		return id_ == rhs.id_
			&& name_ == rhs.name_
			&& type_ == rhs.type_
			&& on_ == rhs.on_
			&& brightness_ == rhs.brightness_
			&& value_ == rhs.value_;
	}

	struct mode_translator : json::named_translator, json::inplace_translator
	{
		std::string empty_;
		const std::string& name() const override { return empty_; }
		json::value pack(const void* ctx, json::ctx_env& env) override { return {}; }
		bool unpack(const json::value& v, void* ctx, json::ctx_env& env) override { return false; }
		void clean(void* ctx) const override { }
		inplace_translator* inplace() override { return this; }
		void pack(json::map& out, const void* ctx, json::ctx_env& env) override
		{
			static_cast<const light_source*>(ctx)->value().visit(mode::combine(
				[&](const mode::hue_sat& hue) { out.add("hue", hue.hue); out.add("sat", hue.sat); },
				[&](const mode::xy& xy) { out.add("x", xy.x); out.add("y", xy.y); },
				[&](const mode::ct& ct) { out.add("ct", ct.val); }
				));
		}

		bool unpack(const json::map& out, void* ctx, json::ctx_env& env) override
		{
			auto& src = *static_cast<light_source*>(ctx);

			auto it1 = out.find("ct");
			if (it1 != out.end() && it1->second.is<json::INTEGER>()) {
				src.value(mode::ct{
					mode::clamp((int)it1->second.as<json::INTEGER>())
				});
				return true;
			}

			it1 = out.find("hue");
			auto it2 = out.find("sat");
			if (it1 != out.end() && it1->second.is<json::INTEGER>()
				&& it2 != out.end() && it2->second.is<json::INTEGER>()) {
				src.value(mode::hue_sat{
					mode::clamp((int)it1->second.as<json::INTEGER>()),
					mode::clamp((int)it2->second.as<json::INTEGER>())
				});
				return true;
			}

			it1 = out.find("x");
			it2 = out.find("y");
			if (it1 != out.end() && it1->second.is<json::FLOAT>()
				&& it2 != out.end() && it2->second.is<json::FLOAT>()) {
				src.value(mode::xy{
					it1->second.as<json::FLOAT>(),
					it2->second.as<json::FLOAT>()
				});
				return true;
			}

			return true; // color is optional
		}
	};

	void light_source::prepare(json::struct_translator& tr)
	{
		using my_type = light_source;
		tr.PRIV_PROP(id);
		tr.PRIV_PROP(name);
		tr.PRIV_PROP(type);
		tr.OPT_PRIV_PROP(on);
		tr.OPT_NAMED_PROP("bri", brightness_);
		tr.add(std::make_unique<mode_translator>());
	}

} }
