#include <shade/model/light.h>

namespace shade { namespace model {
	bool light::operator == (const light& rhs) const
	{
		return *(const light_source*)this == rhs;
	}
} }
