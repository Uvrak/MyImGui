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
	if (!m_traceA.empty() &&
		!m_traceB.empty())
	{
		selectFirstDifference();
	}
}

void TraceComparisonWindow::draw()
{
	// Toolbar component
	// show filenames and counts; display each trace filepath on its own line
	if (ImGui::Button("Load A"))
	{
		openAndLoadTrace(true);
	}

	ImGui::SameLine();

	ImGui::Text(
		"A: %s   Records: %zu",
		traceAFilename()[0]
		? traceAFilename()
		: "<not loaded>",
		m_traceA.size()
	);

	if (ImGui::Button("Load B"))
	{
		openAndLoadTrace(false);
	}

	ImGui::SameLine();

	ImGui::Text(
		"B: %s   Records: %zu",
		traceBFilename()[0]
		? traceBFilename()
		: "<not loaded>",
		m_traceB.size()
	);

	// draw toolbar (callbacks wired in drawToolbar)
	drawToolbar();

	ImGui::SameLine();
	// Keyboard shortcuts
	ImGuiIO& io = ImGui::GetIO();
	bool focusFilterRequested = false;
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F))
		focusFilterRequested = true;

	handleKeyboardNavigation();	

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

	const bool scrollToSelected =
		takeScrollToSelectedTrace();

	const auto displayEntries =
		TraceComparisonFilter::build(
			m_traceA,
			m_traceB
		);

	if (m_sideBySide)
	{
		// render A and B lists side-by-side
		ImGui::Columns(2, "side_by_side", true);

		drawTraceSide(
			"ListA",
			m_traceA,
			displayEntries,
			scrollToSelected
		);

		ImGui::NextColumn();

		drawTraceSide(
			"ListB",
			m_traceB,
			displayEntries,
			scrollToSelected
		);

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

void TraceComparisonWindow::selectFirstDifference()
{
	const size_t count =
		(std::min)(
			m_traceA.size(),
			m_traceB.size()
			);

	for (size_t i = count;
		i > 0;
		--i)
	{
		const size_t index =
			i - 1;

		const TraceInstructionDifference difference =
			compareTraceInstructions(
				m_traceA[index],
				m_traceB[index]
			);

		if (difference.any())
		{
			const size_t firstIndex =
				findDifferenceStart(
					index
				);

			setSelectedTraceIndex(
				firstIndex
			);

			setScrollToSelectedTrace(
				true
			);

			return;
		}
	}
}

size_t TraceComparisonWindow::findDifferenceStart(
	size_t index
) const
{
	if (index >= m_traceA.size() ||
		index >= m_traceB.size())
	{
		return index;
	}

	const TraceInstructionDifference difference =
		compareTraceInstructions(
			m_traceA[index],
			m_traceB[index]
		);

	while (index > 0)
	{
		const size_t previousIndex =
			index - 1;

		bool sameDifferenceValues =
			true;

		if (difference.ax)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.ax ==
				m_traceA[index].registers.ax &&
				m_traceB[previousIndex].registers.ax ==
				m_traceB[index].registers.ax;
		}

		if (difference.bx)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.bx ==
				m_traceA[index].registers.bx &&
				m_traceB[previousIndex].registers.bx ==
				m_traceB[index].registers.bx;
		}

		if (difference.cx)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.cx ==
				m_traceA[index].registers.cx &&
				m_traceB[previousIndex].registers.cx ==
				m_traceB[index].registers.cx;
		}

		if (difference.dx)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.dx ==
				m_traceA[index].registers.dx &&
				m_traceB[previousIndex].registers.dx ==
				m_traceB[index].registers.dx;
		}

		if (difference.si)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.si ==
				m_traceA[index].registers.si &&
				m_traceB[previousIndex].registers.si ==
				m_traceB[index].registers.si;
		}

		if (difference.di)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.di ==
				m_traceA[index].registers.di &&
				m_traceB[previousIndex].registers.di ==
				m_traceB[index].registers.di;
		}

		if (difference.bp)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.bp ==
				m_traceA[index].registers.bp &&
				m_traceB[previousIndex].registers.bp ==
				m_traceB[index].registers.bp;
		}

		if (difference.sp)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.sp ==
				m_traceA[index].registers.sp &&
				m_traceB[previousIndex].registers.sp ==
				m_traceB[index].registers.sp;
		}

		if (difference.ds)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.ds ==
				m_traceA[index].registers.ds &&
				m_traceB[previousIndex].registers.ds ==
				m_traceB[index].registers.ds;
		}

		if (difference.es)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.es ==
				m_traceA[index].registers.es &&
				m_traceB[previousIndex].registers.es ==
				m_traceB[index].registers.es;
		}

		if (difference.ss)
		{
			sameDifferenceValues &=
				m_traceA[previousIndex].registers.ss ==
				m_traceA[index].registers.ss &&
				m_traceB[previousIndex].registers.ss ==
				m_traceB[index].registers.ss;
		}

		if (!sameDifferenceValues)
		{
			break;
		}

		index =
			previousIndex;
	}

	return index;
}

void TraceComparisonWindow::drawTraceSide(
	const char* childId,
	const std::vector<RuntimeInstruction>& trace,
	const std::vector<TraceComparisonDisplayEntry>& displayEntries,
	bool scrollToSelected
)
{
	ImGui::BeginChild(
		childId,
		ImVec2(0, 0),
		false
	);

	for (const auto& entry :
		displayEntries)
	{
		if (entry.collapsedCount > 0)
		{
			ImGui::Separator();

			ImGui::TextDisabled(
				"... %zu identical records ...",
				entry.collapsedCount
			);

			continue;
		}

		const size_t index =
			entry.traceIndex;

		const bool isSelected =
			index ==
			selectedTraceIndex();

		const TraceInstructionDifference difference =
			compareTraceInstructions(
				m_traceA[index],
				m_traceB[index]
			);

		ImGui::Separator();

		m_recordView.draw(
			index,
			trace[index],
			difference,
			isSelected
		);

		if (scrollToSelected &&
			isSelected)
		{
			ImGui::SetScrollHereY(
				0.5f
			);
		}
	}

	ImGui::EndChild();
}

void TraceComparisonWindow::handleKeyboardNavigation()
{
	if (ImGui::IsAnyItemActive())
	{
		return;
	}

	ImGuiIO& io =
		ImGui::GetIO();

	const size_t count =
		(std::min)(
			m_traceA.size(),
			m_traceB.size()
			);

	if (ImGui::IsKeyPressed(
		ImGuiKey_F8
	))
	{
		if (io.KeyShift)
		{
			selectPreviousDifference();
		}
		else
		{
			selectNextDifference();
		}
	}

	if (io.KeyCtrl &&
		ImGui::IsKeyPressed(
			ImGuiKey_S
		))
	{
		openAndSaveTrace(
			!io.KeyShift
		);
	}

	if (ImGui::IsKeyPressed(
		ImGuiKey_UpArrow
	))
	{
		if (count == 0)
		{
			return;
		}

		if (selectedTraceIndex() ==
			static_cast<size_t>(-1))
		{
			setSelectedTraceIndex(
				count - 1
			);
		}
		else if (selectedTraceIndex() > 0)
		{
			setSelectedTraceIndex(
				selectedTraceIndex() - 1
			);
		}

		setScrollToSelectedTrace(
			true
		);
	}
	else if (ImGui::IsKeyPressed(
		ImGuiKey_DownArrow
	))
	{
		if (count == 0)
		{
			return;
		}

		if (selectedTraceIndex() ==
			static_cast<size_t>(-1))
		{
			setSelectedTraceIndex(
				0
			);
		}
		else if (selectedTraceIndex() + 1 <
			count)
		{
			setSelectedTraceIndex(
				selectedTraceIndex() + 1
			);
		}

		setScrollToSelectedTrace(
			true
		);
	}
}

void TraceComparisonWindow::selectPreviousDifference()
{
	const size_t count =
		(std::min)(
			m_traceA.size(),
			m_traceB.size()
			);

	if (count == 0)
	{
		return;
	}

	size_t selected =
		selectedTraceIndex();

	if (selected ==
		static_cast<size_t>(-1) ||
		selected >= count)
	{
		selected =
			count - 1;
	}

	if (compareTraceInstructions(
		m_traceA[selected],
		m_traceB[selected]
	).any())
	{
		const size_t blockStart =
			findDifferenceStart(
				selected
			);

		if (blockStart < selected)
		{
			setSelectedTraceIndex(
				blockStart
			);

			setScrollToSelectedTrace(
				true
			);

			return;
		}
	}

	size_t index =
		selected;

	while (index > 0)
	{
		--index;

		if (compareTraceInstructions(
			m_traceA[index],
			m_traceB[index]
		).any())
		{
			const size_t blockStart =
				findDifferenceStart(
					index
				);

			setSelectedTraceIndex(
				blockStart
			);

			setScrollToSelectedTrace(
				true
			);

			return;
		}
	}
}

void TraceComparisonWindow::selectNextDifference()
{
	const size_t count =
		(std::min)(
			m_traceA.size(),
			m_traceB.size()
			);

	if (count == 0)
	{
		return;
	}

	size_t selected =
		selectedTraceIndex();

	if (selected ==
		static_cast<size_t>(-1) ||
		selected >= count)
	{
		selected =
			0;
	}

	size_t index =
		selected + 1;

	while (index < count)
	{
		if (compareTraceInstructions(
			m_traceA[index],
			m_traceB[index]
		).any())
		{
			const size_t blockStart =
				findDifferenceStart(
					index
				);

			if (blockStart <= selected)
			{
				++index;
				continue;
			}

			setSelectedTraceIndex(
				blockStart
			);

			setScrollToSelectedTrace(
				true
			);

			return;
		}

		++index;
	}
}

void TraceComparisonWindow::drawToolbar()
{
	m_toolbar.setSaveA(
		[this]()
		{
			openAndSaveTrace(
				true
			);
		}
	);

	m_toolbar.setPrevDiff(
		[this]()
		{
			selectPreviousDifference();
		}
	);

	m_toolbar.setNextDiff(
		[this]()
		{
			selectNextDifference();
		}
	);

	m_toolbar.setFocusFilter(
		[&]()
		{
			ImGui::SetKeyboardFocusHere();
		}
	);

	m_toolbar.setSideBySideState(
		m_sideBySide
	);

	m_toolbar.setOnToggleSideBySide(
		[&](bool value)
		{
			m_sideBySide =
				value;
		}
	);

	m_toolbar.draw();

	for (size_t slot = 0;
		slot < 2;
		++slot)
	{
		if (!m_persistenceErrors[slot].empty())
		{
			ImGui::TextWrapped(
				"%s: %s",
				slot == 0
				? "A"
				: "B",
				m_persistenceErrors[slot].c_str()
			);
		}
	}
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

				if (!m_traceA.empty() &&
					!m_traceB.empty())
				{
					selectFirstDifference();
				}

				return true;
			}
		}

		return false;
	}

}
