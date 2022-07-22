/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_HASH_H_
#define _SPRING_HASH_H_

#include "Sync/HsiehHash.h"
#include <string>
#include <type_traits>
#include <memory>

namespace spring {
	template<typename T>
	struct synced_hash {
		std::uint32_t operator()(const T& s) const;
	};

	template<>
	struct synced_hash<std::int64_t> {
	public:
		std::uint32_t operator()(const std::int64_t& i) const
		{
			return static_cast<std::uint32_t>(i) ^ static_cast<std::uint32_t>(i >> 32);
		}
	};

	template<>
	struct synced_hash<std::uint64_t> {
	public:
		std::uint32_t operator()(const std::int64_t& i) const
		{
			return static_cast<std::uint32_t>(i) ^ static_cast<std::uint32_t>(i >> 32);
		}
	};

	template<>
	struct synced_hash<std::string> {
	public:
		std::uint32_t operator()(const std::string& s) const
		{
			return HsiehHash(&s.data()[0], static_cast<std::uint32_t>(s.size()), 0);
		}
	};

	template<>
	struct synced_hash<std::pair<int, int>> {
	public:
		std::uint32_t operator()(const std::pair<int, int>& i) const
		{
			return static_cast<std::uint32_t>(i.first) ^ static_cast<std::uint32_t>(i.second);
		}
	};

	// Best effort implementation for types not covered explicitly
	template<typename T>
	inline uint32_t synced_hash<T>::operator()(const T& s) const {
		if constexpr (std::is_integral<T>::value && sizeof(T) <= sizeof(uint32_t)) {
			return static_cast<std::uint32_t>(s);
		}
		else {
			static_assert(std::has_unique_object_representations<T>::value, "synced_hash not auto-implemented for this type");
			return HsiehHash(std::addressof(s), sizeof(T), 0);
		}
	}

	static inline std::uint32_t LiteHash(const void* p, unsigned size, std::uint32_t cs0 = 0) {
		std::uint32_t cs = cs0;

		switch (size) {
		case 1:
			cs += *(const unsigned char*)p;
			cs ^= cs << 10;
			cs += cs >> 1;
			break;
		case 2:
			cs += *(const unsigned short*)(const char*)p;
			cs ^= cs << 11;
			cs += cs >> 17;
			break;
		case 3:
			// just here to make the switch statements contiguous (so it can be optimized)
			for (unsigned i = 0; i < 3; ++i) {
				cs += *(const unsigned char*)p + i;
				cs ^= cs << 10;
				cs += cs >> 1;
			}
			break;
		case 4:
			cs += *(const unsigned int*)(const char*)p;
			cs ^= cs << 16;
			cs += cs >> 11;
			break;
		default:
		{
			unsigned i = 0;
			for (; i < (size & ~3) / 4; ++i) {
				cs += *(reinterpret_cast<const unsigned int*>(p) + i);
				cs ^= cs << 16;
				cs += cs >> 11;
			}
			for (; i < size; ++i) {
				cs += *(const unsigned char*)p + i;
				cs ^= cs << 10;
				cs += cs >> 1;
			}
			break;
		}
		}
		return cs;
	}

	template<typename T>
	static inline std::uint32_t LiteHash(const T& p, std::uint32_t cs0 = 0) { return LiteHash(&p, sizeof(T), cs0); }

	template<typename T>
	static inline std::uint32_t LiteHash(const T* p, std::uint32_t cs0 = 0) { return LiteHash( p, sizeof(T), cs0); }
}

#endif //_SPRING_HASH_H_
