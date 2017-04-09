#pragma once

namespace shade {
	class storage;
}

namespace shade { namespace model {
	class updatable {
		storage* stg_ = nullptr;
	public:
		void set(storage* stg) { stg_ = stg; }
		storage* get_storage() const { return stg_; }
	protected:
		updatable() = default;
		updatable(storage* stg) : stg_{ stg } {}
		void store();
	};
} }
