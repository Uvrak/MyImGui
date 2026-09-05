#include "TraceComparisonWindow.h"
#include "TraceComparisonPersistence.h"

#include "imgui.h"

#include <cstdio>

#include <fstream>
#include <string>
#include <windows.h>
#include <commdlg.h>

namespace DosBoxMemoryTools
{
namespace
{
	const std::filesystem::path comparisonSnapshots[] = {
		"../settings/trace_comparison_A.snapshot",
		"../settings/trace_comparison_B.snapshot"
	};
}

TraceComparisonWindow::TraceComparisonWindow()
{
	for (size_t slot = 0; slot < 2; ++slot)
	{
		std::string filename;
		auto& trace = slot == 0 ? m_traceA : m_traceB;
		const auto result = TraceComparisonPersistence::restore(comparisonSnapshots[slot], trace, filename);
		if (result == TraceComparisonPersistence::RestoreResult::Loaded)
		{
			if (slot == 0)
			{
				setTraceAFilename(filename.c_str());
				m_hasLoadedTraceA = true;
			}
			else setTraceBFilename(filename.c_str());
		}
		else if (result == TraceComparisonPersistence::RestoreResult::Invalid)
			m_persistenceErrors[slot] = "Saved trace could not be restored (invalid or unreadable file).";
	}
}

void TraceComparisonWindow::draw(
	const std::vector<RuntimeInstruction>& /*traceAParam*/,
	const std::vector<RuntimeInstruction>& /*traceBParam*/,
	char* /*targetText*/,
	size_t /*targetTextSize*/,
	PhysicalAddrResolver /*tryGetPhysicalMemoryAddress*/)
{
	// Toolbar component
	// show filenames and counts; display each trace filepath on its own line
	ImGui::Text("A: %s", traceAFilename()[0] ? traceAFilename() : "<not loaded>");
	ImGui::Text("B: %s", traceBFilename()[0] ? traceBFilename() : "<not loaded>");
	ImGui::Text("A: %zu   B: %zu", m_traceA.size(), m_traceB.size());

	// draw toolbar (callbacks wired in drawToolbar)
	drawToolbar();

	ImGui::SameLine();
	// Keyboard shortcuts
	ImGuiIO& io = ImGui::GetIO();
	bool focusFilterRequested = false;
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F))
		focusFilterRequested = true;

	// Navigate diffs with F8 (Shift+F8 = prev)
	if (!ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_F8))
	{
		if (io.KeyShift)
		{
			// Prev Diff
			const size_t cnt = (std::min)(m_traceA.size(), m_traceB.size());
			if (cnt > 0)
			{
				size_t idx = (selectedTraceIndex() == static_cast<size_t>(-1)) ? cnt : selectedTraceIndex();
				while (idx > 0)
				{
					--idx;
					if (compareTraceInstructions(m_traceA[idx], m_traceB[idx]).any())
					{
						setSelectedTraceIndex(idx);
						setScrollToSelectedTrace(true);
						break;
					}
				}
			}
		}
		else
		{
			// Next Diff
			const size_t cnt = (std::min)(m_traceA.size(), m_traceB.size());
			if (cnt > 0)
			{
				size_t start = (selectedTraceIndex() == static_cast<size_t>(-1)) ? 0 : (selectedTraceIndex() + 1);
				for (size_t idx = start; idx < cnt; ++idx)
				{
					if (compareTraceInstructions(m_traceA[idx], m_traceB[idx]).any())
					{
						setSelectedTraceIndex(idx);
						setScrollToSelectedTrace(true);
						break;
					}
				}
			}
		}
	}

	// Save shortcut: Ctrl+S (Shift+Ctrl+S -> save B)
	if (!ImGui::IsAnyItemActive() && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
	{
		if (io.KeyShift)
			openAndSaveTrace(false);
		else
			openAndSaveTrace(true);
	}

	// Up/Down navigation in the list
	if (!ImGui::IsAnyItemActive())
	{
		if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
		{
			const size_t cnt = (std::min)(m_traceA.size(), m_traceB.size());
			if (cnt > 0)
			{
				if (selectedTraceIndex() == static_cast<size_t>(-1))
					setSelectedTraceIndex(cnt - 1);
				else if (selectedTraceIndex() > 0)
					setSelectedTraceIndex(selectedTraceIndex() - 1);
				setScrollToSelectedTrace(true);
			}
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
		{
			const size_t cnt = (std::min)(m_traceA.size(), m_traceB.size());
			if (cnt > 0)
			{
				if (selectedTraceIndex() == static_cast<size_t>(-1))
					setSelectedTraceIndex(0);
				else if (selectedTraceIndex() + 1 < cnt)
					setSelectedTraceIndex(selectedTraceIndex() + 1);
				setScrollToSelectedTrace(true);
			}
		}
	}

	static char filter[256] = {};
	if (focusFilterRequested)
		ImGui::SetKeyboardFocusHere();
	ImGui::SetNextItemWidth(200);
	ImGui::InputTextWithHint("##filter", "Filter (addr or text)", filter, sizeof(filter));

	ImGui::Separator();

	// Make toolbar fixed: place the scrolling content into its own child so the toolbar above does not scroll away
	ImGui::BeginChild("TraceContentScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	const size_t count = (std::min)(m_traceA.size(), m_traceB.size());

	// Main split: left list and right detail
	// List component
	// configure list view callbacks to use our traces and diff function
	m_listView.setGetCount([&]() { return (std::min)(m_traceA.size(), m_traceB.size()); });
	m_listView.setGetAddress([&](size_t idx) { return m_traceA[idx].address; });
	m_listView.setGetDiff([&](size_t idx) { return compareTraceInstructions(m_traceA[idx], m_traceB[idx]); });
	m_listView.setOnSelect([&](size_t idx) { setSelectedTraceIndex(idx); setScrollToSelectedTrace(false); });

	// Keep list selection state in sync
	m_listView.setSelectedIndex(selectedTraceIndex());
	if (m_sideBySide)
	{
		// render A and B lists side-by-side
		ImGui::Columns(2, "side_by_side", true);

		// Left: A list
		ImGui::BeginChild("ListA", ImVec2(0, 0), false);
		// reuse list view but show A-specific snippets by temporarily wiring callbacks
		m_listView.draw(filter);
		ImGui::EndChild();
		ImGui::NextColumn();

		// Right: B detail / list
		ImGui::BeginChild("ListB", ImVec2(0, 0), false);
		// render B-specific snippets inline (simple table)
		const size_t count = (std::min)(m_traceA.size(), m_traceB.size());
		for (size_t i = 0; i < count; ++i)
		{
			const auto diff = compareTraceInstructions(m_traceA[i], m_traceB[i]);
			char buf[128];
			snprintf(buf, sizeof(buf), "#%zu 0x%zX %s", i, m_traceB[i].address, diff.any() ? "Diff" : "=");
			const bool isSel = (i == selectedTraceIndex());
			if (ImGui::Selectable(buf, isSel))
				setSelectedTraceIndex(i);
	}

		ImGui::EndChild();

		ImGui::Columns(1);
	}
	else
	{
		m_listView.draw(filter);

		ImGui::SameLine();

		ImGui::BeginChild("DetailPane", ImVec2(0, 300), true);
		// detail component
		m_detailView.draw(
			[&]() { return selectedTraceIndex(); },
			[&]() -> const std::vector<RuntimeInstruction>& { return m_traceA; },
			[&]() -> const std::vector<RuntimeInstruction>& { return m_traceB; }
		);
		ImGui::EndChild();
	}

	// close the outer TraceContentScroll child
	ImGui::EndChild();
}


void TraceComparisonWindow::drawToolbar()
{
	// wire toolbar callbacks to existing actions
	m_toolbar.setLoadA([this]() { openAndLoadTrace(true); });
	m_toolbar.setLoadB([this]() { openAndLoadTrace(false); });
	m_toolbar.setSaveA([this]() { openAndSaveTrace(true); });
	m_toolbar.setPrevDiff([this]() {
		const size_t count = (std::min)(m_traceA.size(), m_traceB.size());
		if (count > 0)
		{
			size_t idx = (selectedTraceIndex() == static_cast<size_t>(-1)) ? count : selectedTraceIndex();
			while (idx > 0)
			{
				--idx;
				if (compareTraceInstructions(m_traceA[idx], m_traceB[idx]).any())
				{
					setSelectedTraceIndex(idx);
					setScrollToSelectedTrace(true);
					break;
				}
			}
		}
	});
	m_toolbar.setNextDiff([this]() {
		const size_t count = (std::min)(m_traceA.size(), m_traceB.size());
		if (count > 0)
		{
			size_t start = (selectedTraceIndex() == static_cast<size_t>(-1)) ? 0 : (selectedTraceIndex() + 1);
			for (size_t idx = start; idx < count; ++idx)
			{
				if (compareTraceInstructions(m_traceA[idx], m_traceB[idx]).any())
				{
					setSelectedTraceIndex(idx);
					setScrollToSelectedTrace(true);
					break;
				}
			}
		}
	});
	m_toolbar.setFocusFilter([&]() { ImGui::SetKeyboardFocusHere(); });
	// ensure toolbar reflects current state
	m_toolbar.setSideBySideState(m_sideBySide);
	m_toolbar.setOnToggleSideBySide([&](bool v) { m_sideBySide = v; });
	m_toolbar.draw();
	for (size_t slot = 0; slot < 2; ++slot)
		if (!m_persistenceErrors[slot].empty())
			ImGui::TextWrapped("%s: %s", slot == 0 ? "A" : "B", m_persistenceErrors[slot].c_str());
}

bool TraceComparisonWindow::loadTraceFromFile(const std::string& filename, std::vector<RuntimeInstruction>& trace)
	{
		std::ifstream file(filename);

		if (!file)
		{
			return false;
		}

		std::string header;
		std::getline(file, header);

		if (header != "ExecutionTrace 1")
		{
			return false;
		}

		size_t instructionCount = 0;
		if (!(file >> instructionCount))
		{
			return false;
		}

		std::vector<RuntimeInstruction> loadedTrace;
		loadedTrace.reserve(instructionCount);

		for (size_t instructionIndex = 0; instructionIndex < instructionCount; ++instructionIndex)
		{
			RuntimeInstruction instruction;

			if (!(file
				>> instruction.address
				>> instruction.cs
				>> instruction.ip
				>> instruction.registers.ax
				>> instruction.registers.bx
				>> instruction.registers.cx
				>> instruction.registers.dx
				>> instruction.registers.si
				>> instruction.registers.di
				>> instruction.registers.bp
				>> instruction.registers.sp
				>> instruction.registers.ds
				>> instruction.registers.es
				>> instruction.registers.ss))
			{
				return false;
			}

			for (uint8_t& byte : instruction.bytes)
			{
				unsigned int value = 0;
				if (!(file >> value) || value > 0xff)
				{
					return false;
				}
				byte = static_cast<uint8_t>(value);
			}

			loadedTrace.push_back(instruction);
		}

		trace = std::move(loadedTrace);
		return true;
	}

	bool TraceComparisonWindow::saveTraceToFile(const std::string& filename, const std::vector<RuntimeInstruction>& trace)
	{
		std::ofstream file(filename);

		if (!file)
		{
			return false;
		}

		file << "ExecutionTrace 1\n";
		file << trace.size() << '\n';

		for (const auto& instruction : trace)
		{
			file
				<< instruction.address << ' '
				<< instruction.cs << ' '
				<< instruction.ip << ' '

				<< instruction.registers.ax << ' '
				<< instruction.registers.bx << ' '
				<< instruction.registers.cx << ' '
				<< instruction.registers.dx << ' '

				<< instruction.registers.si << ' '
				<< instruction.registers.di << ' '
				<< instruction.registers.bp << ' '
				<< instruction.registers.sp << ' '

				<< instruction.registers.ds << ' '
				<< instruction.registers.es << ' '
				<< instruction.registers.ss;

			for (uint8_t b : instruction.bytes)
			{
				file << ' ' << static_cast<unsigned int>(b);
			}

			file << '\n';
		}

		return true;
	}

	bool TraceComparisonWindow::openAndSaveTrace(bool forA)
	{
		char filename[4096] = {};

		OPENFILENAMEA dialog{};
		dialog.lStructSize = sizeof(dialog);
		dialog.lpstrFile = filename;
		dialog.nMaxFile = sizeof(filename);
		dialog.lpstrFilter = "Execution Trace (*.trace)\0*.trace\0All Files (*.*)\0*.*\0";
		dialog.nFilterIndex = 1;
		dialog.lpstrDefExt = "trace";
		dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

		if (GetSaveFileNameA(&dialog))
		{
			const auto& trace = forA ? m_traceA : m_traceB;
			if (saveTraceToFile(filename, trace))
			{
				if (forA)
					setTraceAFilename(filename);
				else
					setTraceBFilename(filename);

				return true;
			}
		}

		return false;
	}

	bool TraceComparisonWindow::openAndLoadTrace(bool forA)
	{
		char filename[4096] = {};

		OPENFILENAMEA dialog{};
		dialog.lStructSize = sizeof(dialog);
		dialog.lpstrFile = filename;
		dialog.nMaxFile = sizeof(filename);
		dialog.lpstrFilter = "Execution Trace (*.trace)\0*.trace\0All Files (*.*)\0*.*\0";
		dialog.nFilterIndex = 1;
		dialog.lpstrDefExt = "trace";
		dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&dialog))
		{
			std::vector<RuntimeInstruction> trace;

			if (loadTraceFromFile(filename, trace))
			{
				if (forA)
				{
					setTraceA(std::move(trace));
					setTraceAFilename(filename);
					m_hasLoadedTraceA = true;
				}
				else
				{
					setTraceB(std::move(trace));
					setTraceBFilename(filename);
				}

				const size_t slot = forA ? 0 : 1;
				if (TraceComparisonPersistence::save(comparisonSnapshots[slot],
					forA ? m_traceA : m_traceB, filename))
					m_persistenceErrors[slot].clear();
				else
					m_persistenceErrors[slot] = "Trace loaded, but saving the snapshot failed. Previous snapshot retained.";

				return true;
			}
		}

		return false;
	}

}
