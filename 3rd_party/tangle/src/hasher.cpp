/*
 * Copyright (C) 2016 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <tangle/msg/hasher.h>
#include <algorithm>

namespace tangle { namespace msg {

	namespace consts {
		template <size_t sizeof_size_t> struct fnv_const;
		template <>
		struct fnv_const<4> {
			static constexpr uint32_t offset = 2166136261U;
			static constexpr uint32_t prime = 16777619U;
		};

		template <>
		struct fnv_const<8> {
			static constexpr uint64_t offset = 14695981039346656037ULL;
			static constexpr uint64_t prime = 1099511628211ULL;
		};
	};

	hasher::hasher()
		: m_value { consts::fnv_const<sizeof(size_t)>::offset }
	{
	}

	hasher& hasher::append(const void* buffer, size_t length)
	{
		constexpr auto prime = consts::fnv_const<sizeof(size_t)>::prime;

		for (auto data = (const char*)buffer; length; ++data, --length) {
			m_value ^= (size_t)*data;
			m_value *= prime;
		}
		return *this;
	}

	size_t hasher::hash(const char* s, size_t l)
	{
		return hasher { }.append(s, l).value();
	}
}};
