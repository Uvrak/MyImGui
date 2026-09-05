#pragma once

#include <functional>
#include <vector>

#include "MemoryScanner.h"

namespace DosBoxMemoryTools
{
	class TraceDetailView
	{
	public:
		TraceDetailView() = default;

		// Draw details for the selected index. Provide accessor lambdas to obtain traces and selected index.
		void draw(
			std::function<size_t()> getSelected,
			std::function<const std::vector<RuntimeInstruction>&()> getTraceA,
			std::function<const std::vector<RuntimeInstruction>&()> getTraceB
		);
	};
}
