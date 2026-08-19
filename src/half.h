#pragma once

namespace half_float
{
	using half = std::uint16_t;

	template <class T>
	T half_cast(half a_value)
	{
		return static_cast<T>(DirectX::PackedVector::XMConvertHalfToFloat(a_value));
	}
}
