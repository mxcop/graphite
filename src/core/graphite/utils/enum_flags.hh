#pragma once

#include <type_traits>

/* Define bitwise operators for enum class. */
#define ENUM_CLASS_FLAGS(Enum) \
    inline Enum operator~ (Enum a) { return static_cast<Enum>(~static_cast<std::underlying_type_t<Enum>>(a)); } \
    inline Enum operator| (Enum a, Enum b) { return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(a) | static_cast<std::underlying_type_t<Enum>>(b)); } \
    inline Enum operator& (Enum a, Enum b) { return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(a) & static_cast<std::underlying_type_t<Enum>>(b)); } \
    inline Enum operator^ (Enum a, Enum b) { return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(a) ^ static_cast<std::underlying_type_t<Enum>>(b)); } \
    inline Enum& operator|= (Enum& a, Enum b) { return reinterpret_cast<Enum&>(reinterpret_cast<std::underlying_type_t<Enum>&>(a) |= static_cast<std::underlying_type_t<Enum>>(b)); } \
    inline Enum& operator&= (Enum& a, Enum b) { return reinterpret_cast<Enum&>(reinterpret_cast<std::underlying_type_t<Enum>&>(a) &= static_cast<std::underlying_type_t<Enum>>(b)); } \
    inline Enum& operator^= (Enum& a, Enum b) { return reinterpret_cast<Enum&>(reinterpret_cast<std::underlying_type_t<Enum>&>(a) ^= static_cast<std::underlying_type_t<Enum>>(b)); } \
    inline bool has_flag(const Enum a, const Enum b) { return (a & b) == b; }
