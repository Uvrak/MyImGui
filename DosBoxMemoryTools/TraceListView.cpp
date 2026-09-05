#include "TraceListView.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <cmath>
#include <cstdlib>

namespace DosBoxMemoryTools
{
	void TraceListView::draw(const char* filter)
	{
		if (!m_getCount || !m_getDiff || !m_getAddress)
			return;

		const size_t count = m_getCount();

		// Load persisted widths once
		if (!m_columnWidthsLoaded)
			loadColumnWidths();

		ImGui::BeginChild("TraceListView_child", ImVec2(0, 0), false);
		ImGui::Columns(4, "tl_cols", true);

		// Apply persisted widths
		ImGui::SetColumnWidth(0, m_columnWidths[0]);
		ImGui::SetColumnWidth(1, m_columnWidths[1]);
		ImGui::SetColumnWidth(2, m_columnWidths[2]);
		ImGui::SetColumnWidth(3, m_columnWidths[3]);

		// Header labels
		ImGui::Text("Idx"); ImGui::NextColumn();
		ImGui::Text("Diff"); ImGui::NextColumn();
		ImGui::Text("A"); ImGui::NextColumn();
		ImGui::Text("B"); ImGui::NextColumn();

		for (size_t i = 0; i < count; ++i)
		{
			// basic filter on address
			if (filter && filter[0])
			{
				char addrbuf[32];
				snprintf(addrbuf, sizeof(addrbuf), "0x%zX", m_getAddress(i));
				if (!strstr(addrbuf, filter))
					continue;
			}

			ImGui::Text("%zu", i);
			ImGui::NextColumn();

			auto diff = m_getDiff(i);
			if (diff.any())
				ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "●");
			else
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "●");
			ImGui::NextColumn();

			char abuf[64];
			snprintf(abuf, sizeof(abuf), "0x%zX", m_getAddress(i));
			ImGui::TextUnformatted(abuf);
			ImGui::NextColumn();

			char blabel[64];
			snprintf(blabel, sizeof(blabel), "#%zu %s", i, diff.any() ? "Diff" : "=");
			bool isSelected = (i == m_selectedIndex);
			if (ImGui::Selectable(blabel, isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				m_selectedIndex = i;
				if (m_onSelect) m_onSelect(i);
			}

			// If a scroll-to-selected was requested, and this item is selected, ensure it is visible
			if (m_requestScrollToSelected && isSelected)
			{
				ImGui::SetScrollHereY(0.5f);
				m_requestScrollToSelected = false;
			}
			ImGui::NextColumn();
		}

		// after rendering, capture column widths and persist if changed
		float w0 = ImGui::GetColumnWidth(0);
		float w1 = ImGui::GetColumnWidth(1);
		float w2 = ImGui::GetColumnWidth(2);
		float w3 = ImGui::GetColumnWidth(3);
		if (fabsf(w0 - m_columnWidths[0]) > 1.0f || fabsf(w1 - m_columnWidths[1]) > 1.0f || fabsf(w2 - m_columnWidths[2]) > 1.0f || fabsf(w3 - m_columnWidths[3]) > 1.0f)
		{
			m_columnWidths[0] = w0;
			m_columnWidths[1] = w1;
			m_columnWidths[2] = w2;
			m_columnWidths[3] = w3;
			saveColumnWidths();
		}

		ImGui::Columns(1);
		ImGui::EndChild();
	}

	void TraceListView::loadColumnWidths()
	{
		m_columnWidthsLoaded = true;
		// use a simple file under the user's temp directory for persistence
		char tmpbuf[32768] = {};
		size_t outLen = 0;
		errno_t err = getenv_s(&outLen, tmpbuf, sizeof(tmpbuf), "TEMP");
		if (err != 0 || outLen == 0)
			return;

		std::string path = std::string(tmpbuf) + "\\MyImGui_TraceListColumns.cfg";
		std::ifstream f(path);
		if (!f) return;

		for (int i = 0; i < 4; ++i)
		{
			float v = 0.0f;
			f >> v;
			if (f)
				m_columnWidths[i] = v;
	}

	}

	void TraceListView::saveColumnWidths() const
	{
		char tmpbuf[32768] = {};
		size_t outLen = 0;
		errno_t err = getenv_s(&outLen, tmpbuf, sizeof(tmpbuf), "TEMP");
		if (err != 0 || outLen == 0)
			return;
		std::string path = std::string(tmpbuf) + "\\MyImGui_TraceListColumns.cfg";
		std::ofstream f(path, std::ios::trunc);
		if (!f) return;
		for (int i = 0; i < 4; ++i)
		{
			f << m_columnWidths[i] << '\n';
		}
	}
} // namespace DosBoxMemoryTools
