#pragma once
#include <cstdint>

namespace MaxsuPoise
{
	enum StaggerLevel : std::uint32_t
	{
		kNone = 0,
		kSmall,
		kMedium,
		kLarge,
		kLargest,
	};
}
