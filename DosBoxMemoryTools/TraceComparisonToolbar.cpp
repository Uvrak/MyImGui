#include "TraceComparisonToolbar.h"
#include "imgui.h"

namespace DosBoxMemoryTools
{
	void TraceComparisonToolbar::draw()
	{
		if (ImGui::SmallButton("Load A") && m_loadA) m_loadA();
		ImGui::SameLine();
	if (ImGui::SmallButton("Load B") && m_loadB) m_loadB();
	ImGui::SameLine();
	// Draw Side-by-side checkbox next to Load B when toolbar is used embedded
	ImGui::SetNextItemWidth(120.0f);
	bool side = m_sideBySide;
	if (ImGui::Checkbox("Side-by-side##TraceToolbarEmbedded", &side))
	{
		m_sideBySide = side;
		if (m_onToggleSideBySide)
			m_onToggleSideBySide(side);
	}
	ImGui::SameLine();
	// Replace Save A / Save B with single Save button that saves left/main dataset
	if (ImGui::SmallButton("Save") && m_saveA) m_saveA();
	ImGui::SameLine();
		ImGui::SameLine();
		if (ImGui::SmallButton("Prev Diff") && m_prevDiff) m_prevDiff();
		ImGui::SameLine();
		if (ImGui::SmallButton("Next Diff") && m_nextDiff) m_nextDiff();

	// (Checkbox rendered next to Load B above; no duplicate here)

		// allow focusing filter via toolbar if requested
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1) && m_focusFilter)
			m_focusFilter();
	}
}
