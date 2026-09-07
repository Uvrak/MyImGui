#pragma once

#include <functional>

namespace DosBoxMemoryTools
{
	class TraceComparisonToolbar
	{
	public:
		using Action = std::function<void()>;

		TraceComparisonToolbar() = default;

		void draw();
		void setSave(Action a)
		{
			m_save =
				std::move(a);
		}
		void setPrevDiff(Action a) { m_prevDiff = std::move(a); }
		void setNextDiff(Action a) { m_nextDiff = std::move(a); }
		void setFocusFilter(Action a) { m_focusFilter = std::move(a); }

	private:
		Action m_save;
		Action m_prevDiff;
		Action m_nextDiff;
		Action m_focusFilter;
	};
}
