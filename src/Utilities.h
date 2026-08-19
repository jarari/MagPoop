#pragma once

namespace F4
{
	inline REL::Relocation<RE::NiPoint3*> ptr_PlayerAdjust{ REL::ID{ 988646, 2703465 } };
}

inline std::string SplitString(const std::string& a_value, const std::string& a_delimiter, std::string& a_remainder)
{
	const auto position = a_value.find(a_delimiter);
	if (position == std::string::npos) {
		a_remainder.clear();
		return a_value;
	}
	a_remainder = a_value.substr(position + a_delimiter.length());
	return a_value.substr(0, position);
}

inline void _MESSAGE(const char* a_format, ...)
{
	char buffer[2048]{};
	va_list args;
	va_start(args, a_format);
	vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, a_format, args);
	va_end(args);
	REX::INFO("{}", buffer);
}

inline bool Visit(RE::NiAVObject* a_object, const std::function<bool(RE::NiAVObject*)>& a_visitor)
{
	if (!a_object || a_visitor(a_object)) {
		return a_object != nullptr;
	}
	if (auto* node = a_object->IsNode()) {
		for (auto& child : node->children) {
			if (child && Visit(child.get(), a_visitor)) {
				return true;
			}
		}
	}
	return false;
}

template <class T>
T SafeWrite64Function(std::uintptr_t a_address, T a_replacement)
{
	static_assert(sizeof(T) >= sizeof(std::uintptr_t));
	std::uintptr_t replacement{};
	std::memcpy(&replacement, &a_replacement, sizeof(replacement));
	const auto originalAddress = *reinterpret_cast<std::uintptr_t*>(a_address);
	REL::WriteSafeData(a_address, replacement);
	T original{};
	std::memcpy(&original, &originalAddress, sizeof(originalAddress));
	return original;
}
