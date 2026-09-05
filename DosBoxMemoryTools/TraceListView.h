#pragma once

#include <functional>
#include <vector>
#include <string>

#include "TraceComparison.h"

namespace DosBoxMemoryTools
{
	struct RuntimeInstruction;

	class TraceListView
	{
	public:
		using SelectCallback = std::function<void(size_t)>;
		using GetCountFunc = std::function<size_t()>;
		using GetDiffFunc = std::function<TraceInstructionDifference(size_t)>;
		using GetAddressFunc = std::function<size_t(size_t)>;

		TraceListView() = default;

		void draw(const char* filter);

		void setOnSelect(SelectCallback cb) { m_onSelect = std::move(cb); }
		void setGetCount(GetCountFunc f) { m_getCount = std::move(f); }
		void setGetDiff(GetDiffFunc f) { m_getDiff = std::move(f); }
		void setGetAddress(GetAddressFunc f) { m_getAddress = std::move(f); }

		void setSelectedIndex(size_t idx) { m_selectedIndex = idx; }
		size_t selectedIndex() const { return m_selectedIndex; }
		void requestScrollToSelected() { m_requestScrollToSelected = true; }

	private:
		SelectCallback m_onSelect;
		GetCountFunc m_getCount;
		GetDiffFunc m_getDiff;
		GetAddressFunc m_getAddress;
		size_t m_selectedIndex = static_cast<size_t>(-1);
		bool m_requestScrollToSelected = false;

		// Persisted column widths (Idx, Diff, A, B)
		float m_columnWidths[4] = {50.0f, 40.0f, 200.0f, 200.0f};
		bool m_columnWidthsLoaded = false;

		void loadColumnWidths();
		void saveColumnWidths() const;
	};
}
