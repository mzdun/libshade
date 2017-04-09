#include "shade/model/updatable.h"
#include "shade/storage.h"
#include "json.hpp"

namespace shade { namespace model {
	void updatable::store()
	{
		stg_->store();
	}
} }
