#include <shade/model/light.h>
#include <shade/hue_data.h>

namespace shade { namespace model {
	bool light::operator == (const light& rhs) const
	{
		return *(const light_source*)this == rhs;
	}

#define UPDATE_SOURCE(name, data) \
	if (name() != data) { \
		updated = true; \
		name(data); \
	}
	bool light::update(const std::string& key, hue::light json)
	{
		bool updated = false;
		auto mode = color_mode::from_json(json.state);
		auto brightness = mode::clamp(json.state.bri);

		UPDATE_SOURCE(id, json.uniqueid);
		UPDATE_SOURCE(name, json.name);
		UPDATE_SOURCE(type, json.modelid);
		UPDATE_SOURCE(on, json.state.on);
		UPDATE_SOURCE(bri, brightness);
		UPDATE_SOURCE(value, mode);

		return updated;
	}
#undef UPDATE_SOURCE
} }
