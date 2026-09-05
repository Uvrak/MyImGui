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

		void setLoadA(Action a) { m_loadA = std::move(a); }
		void setLoadB(Action a) { m_loadB = std::move(a); }
		void setSaveA(Action a) { m_saveA = std::move(a); }
		void setPrevDiff(Action a) { m_prevDiff = std::move(a); }
		void setNextDiff(Action a) { m_nextDiff = std::move(a); }
		void setFocusFilter(Action a) { m_focusFilter = std::move(a); }
		void setSideBySideState(bool v) { m_sideBySide = v; }
		void setOnToggleSideBySide(std::function<void(bool)> cb) { m_onToggleSideBySide = std::move(cb); }

	private:
		Action m_loadA;
		Action m_loadB;
		Action m_saveA;
		Action m_saveB;
		Action m_prevDiff;
		Action m_nextDiff;
		Action m_focusFilter;
		bool m_sideBySide = true;
		std::function<void(bool)> m_onToggleSideBySide;
	};
}
