#include "TraceDetailView.h"
#include "imgui.h"
#include <cstdio>

namespace DosBoxMemoryTools
{
	void TraceDetailView::draw(
		std::function<size_t()> getSelected,
		std::function<const std::vector<RuntimeInstruction>&()> getTraceA,
		std::function<const std::vector<RuntimeInstruction>&()> getTraceB)
	{
		const size_t sel = getSelected ? getSelected() : static_cast<size_t>(-1);
		const auto& a = getTraceA();
		const auto& b = getTraceB();

		ImGui::BeginChild("TraceDetail_child", ImVec2(0, 0), true);
		if (sel != static_cast<size_t>(-1) && sel < a.size() && sel < b.size())
		{
			ImGui::Text("Index: %zu", sel);
			ImGui::Text("A: 0x%zX", a[sel].address);
			ImGui::SameLine();
			ImGui::Text("B: 0x%zX", b[sel].address);

			ImGui::Separator();
			ImGui::Text("A bytes:");
			for (size_t i = 0; i < a[sel].bytes.size(); ++i)
			{
				ImGui::SameLine();
				char buf[8];
				snprintf(buf, sizeof(buf), "%02X", a[sel].bytes[i]);
				ImGui::TextUnformatted(buf);
			}

			ImGui::Separator();
			ImGui::Text("B bytes:");
			for (size_t i = 0; i < b[sel].bytes.size(); ++i)
			{
				ImGui::SameLine();
				char buf[8];
				snprintf(buf, sizeof(buf), "%02X", b[sel].bytes[i]);
				ImGui::TextUnformatted(buf);
			}
		}
		ImGui::EndChild();
	}
}
