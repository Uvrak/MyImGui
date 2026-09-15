#include "TraceComparisonWindow.h"
#include "TraceComparisonPersistence.h"
#include "TraceAlignment.h"
#include "TraceDifferenceNavigation.h"

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
	const std::filesystem::path comparisonPaths[] = {
	"../settings/trace_comparison_A.cfg",
	"../settings/trace_comparison_B.cfg"
	};
}

TraceComparisonWindow::TraceComparisonWindow()
{
	for (size_t slot = 0;
		slot < 2;
		++slot)
	{
		std::string filename;

		if (!TraceComparisonPersistence::loadPath(
			comparisonPaths[slot],
			filename
		))
		{
			continue;
		}

		std::vector<RuntimeInstruction> trace;

		if (!loadTraceFromFile(
			filename,
			trace
		))
		{
			m_persistenceErrors[slot] =
				"Last trace file could not be loaded.";

			continue;
		}

		if (slot == 0)
		{
			setTraceA(
				std::move(trace)
			);

			setTraceAFilename(
				filename.c_str()
			);

			m_hasLoadedTraceA =
				true;
		}
		else
		{
			setTraceB(
				std::move(trace)
			);

			setTraceBFilename(
				filename.c_str()
			);
		}
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

	if (ImGui::Checkbox(
		"Collapse identical",
		&m_collapseIdentical
	))
	{
		m_scrollToSelectedTrace =
			true;
	}

	ImGui::NewLine();

	if (ImGui::SmallButton(
		"Add to Baseline"
	))
	{
		m_differenceBaseline.add(
			m_traceA,
			m_traceB
		);
	}

	ImGui::SameLine();

	if (ImGui::SmallButton(
		"Clear Baseline"
	))
	{
		m_differenceBaseline.clear();
	}

	ImGui::SameLine();

	ImGui::Text(
		"Baseline: %zu",
		m_differenceBaseline.size()
	);

	ImGui::SameLine();

	ImGui::Checkbox(
		"Ignore Baseline",
		&m_ignoreDifferenceBaseline
	);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(200);
	ImGui::InputTextWithHint("##filter", "Filter (addr or text)", filter, sizeof(filter));

	ImGui::Separator();

	// Make toolbar fixed: place the scrolling content into its own child so the toolbar above does not scroll away
	ImGui::BeginChild("TraceContentScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	const size_t count = (std::min)(m_traceA.size(), m_traceB.size());

	std::vector<TraceComparisonDisplayEntry>
		displayEntries;

	if (m_collapseIdentical)
	{
		displayEntries =
			TraceComparisonFilter::build(
				m_traceA,
				m_traceB
			);
	}
	else
	{
		displayEntries.reserve(
			count
		);

		for (size_t index = 0;
			index < count;
			++index)
		{
			TraceComparisonDisplayEntry entry{};

			entry.indexA =
				index;

			entry.indexB =
				index;

			displayEntries.push_back(
				entry
			);
		}
	}

	const bool scrollToSelected =
		takeScrollToSelectedTrace();

	drawTraceRows(
		displayEntries,
		scrollToSelected
	);

	// close the outer TraceContentScroll child
	ImGui::EndChild();
}

void TraceComparisonWindow::selectFirstDifference()
{
	if (m_traceA.empty() ||
		m_traceB.empty())
	{
		return;
	}

	const std::vector<TraceAlignment> alignments =
		TraceAligner::align(
			m_traceA,
			m_traceB
		);

	const size_t firstDifference =
		TraceDifferenceNavigation::findPreviousDifference(
			m_traceA,
			m_traceB,
			alignments,
			m_traceA.size(),
			[this](
				const RuntimeInstruction& instructionA,
				const RuntimeInstruction& instructionB
				)
			{
				return compareInstructions(
					instructionA,
					instructionB
				);
			}
		);

	if (firstDifference ==
		static_cast<size_t>(-1))
	{
		return;
	}

	setSelectedTraceIndex(
		firstDifference
	);

	for (const TraceAlignment& alignment :
		alignments)
	{
		if (alignment.indexA ==
			firstDifference)
		{
			m_selectedTraceIndexB =
				alignment.indexB;

			break;
		}
	}

	setScrollToSelectedTrace(
		true
	);
}

TraceInstructionDifference
TraceComparisonWindow::compareInstructions(
	const RuntimeInstruction& instructionA,
	const RuntimeInstruction& instructionB
) const
{
	TraceInstructionDifference difference =
		compareTraceInstructions(
			instructionA,
			instructionB
		);

	if (!m_ignoreDifferenceBaseline)
	{
		return difference;
	}

	return m_differenceBaseline.removeKnown(
		instructionA.address,
		difference
	);
}

void TraceComparisonWindow::setSelectedDatasetA(
	bool selectedA
)
{
	m_selectedDatasetA =
		selectedA;
}

void TraceComparisonWindow::drawTraceRows(
	const std::vector<TraceComparisonDisplayEntry>& displayEntries,
	bool scrollToSelected
)
{
	ImGuiListClipper clipper;

	clipper.Begin(
		static_cast<int>(
			displayEntries.size()
			)
	);

	if (scrollToSelected)
	{
		for (size_t displayIndex = 0;
			displayIndex < displayEntries.size();
			++displayIndex)
		{
			const auto& entry =
				displayEntries[displayIndex];

			if (entry.hasA &&
				entry.indexA == m_selectedTraceIndex)
			{
				clipper.IncludeItemByIndex(
					static_cast<int>(displayIndex)
				);

				break;
			}
		}
	}

	while (clipper.Step())
	{
		for (int displayIndex = clipper.DisplayStart;
			displayIndex < clipper.DisplayEnd;
			++displayIndex)
		{
			const auto& entry =
				displayEntries[
					static_cast<size_t>(displayIndex)
				];

			ImGui::Columns(
				2,
				"TraceRow",
				false
			);

			if (entry.hasA)
			{
				ImGui::PushID("A");

				if (m_recordView.draw(
					entry.indexA,
					m_traceA[entry.indexA],
					entry.hasA && entry.hasB
					? compareInstructions(
						m_traceA[entry.indexA],
						m_traceB[entry.indexB]
					)
					: TraceInstructionDifference{},
					entry.indexA == m_selectedTraceIndex
				))
				{
					setSelectedTraceIndex(
						entry.indexA
					);
				}

				ImGui::PopID();
			}
			else
			{
				ImGui::TextDisabled(
					"<no record>"
				);
			}

			ImGui::NextColumn();

			if (entry.hasB)
			{
				ImGui::PushID("B");

				m_recordView.draw(
					entry.indexB,
					m_traceB[entry.indexB],
					entry.hasA && entry.hasB
					? compareInstructions(
						m_traceA[entry.indexA],
						m_traceB[entry.indexB]
					)
					: TraceInstructionDifference{},
					entry.indexB == m_selectedTraceIndexB
				);

				ImGui::PopID();
			}
			else
			{
				ImGui::TextDisabled(
					"<no record>"
				);
			}

			ImGui::Columns(1);

			if (scrollToSelected &&
				entry.hasA &&
				entry.indexA == m_selectedTraceIndex)
			{
				ImGui::SetScrollHereY(
					0.5f
				);
			}
		}
	}
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
	if (m_traceA.empty() ||
		m_traceB.empty())
	{
		return;
	}

	const std::vector<TraceAlignment> alignments =
		TraceAligner::align(
			m_traceA,
			m_traceB
		);

	size_t selected =
		selectedTraceIndex();

	if (selected ==
		static_cast<size_t>(-1))
	{
		selected =
			m_traceA.size();
	}

	const size_t previousIndex =
		TraceDifferenceNavigation::findPreviousDifference(
			m_traceA,
			m_traceB,
			alignments,
			selected,
			[this](
				const RuntimeInstruction& instructionA,
				const RuntimeInstruction& instructionB
				)
			{
				return compareInstructions(
					instructionA,
					instructionB
				);
			}
		);

	if (previousIndex ==
		static_cast<size_t>(-1))
	{
		return;
	}

	setSelectedTraceIndex(
		previousIndex
	);

	for (const TraceAlignment& alignment :
		alignments)
	{
		if (alignment.indexA ==
			previousIndex)
		{
			m_selectedTraceIndexB =
				alignment.indexB;

			break;
		}
	}

	setScrollToSelectedTrace(
		true
	);
}

void TraceComparisonWindow::selectNextDifference()
{
	if (m_traceA.empty() ||
		m_traceB.empty())
	{
		return;
	}

	const std::vector<TraceAlignment> alignments =
		TraceAligner::align(
			m_traceA,
			m_traceB
		);

	size_t selected =
		selectedTraceIndex();

	if (selected ==
		static_cast<size_t>(-1))
	{
		selected =
			0;
	}

	const size_t nextIndex =
		TraceDifferenceNavigation::findNextDifference(
			m_traceA,
			m_traceB,
			alignments,
			selected,
			[this](
				const RuntimeInstruction& instructionA,
				const RuntimeInstruction& instructionB
				)
			{
				return compareInstructions(
					instructionA,
					instructionB
				);
			}
		);

	if (nextIndex ==
		static_cast<size_t>(-1))
	{
		return;
	}

	setSelectedTraceIndex(
		nextIndex
	);

	for (const TraceAlignment& alignment :
		alignments)
	{
		if (alignment.indexA ==
			nextIndex)
		{
			m_selectedTraceIndexB =
				alignment.indexB;

			break;
		}
	}

	setScrollToSelectedTrace(
		true
	);
}

void TraceComparisonWindow::drawToolbar()
{
	m_toolbar.setSave(
		[this]()
		{
			openAndSaveTrace(
				m_selectedDatasetA
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

	m_toolbar.draw();

	const std::vector<TraceAlignment> alignments =
		TraceAligner::align(
			m_traceA,
			m_traceB
		);

	size_t divergenceCount = 0;

	for (const TraceAlignment& alignment :
		alignments)
	{
		if (!alignment.synchronized)
		{
			++divergenceCount;
		}
	}
	ImGui::SameLine();

	ImGui::Text(
		"Desyncs: %zu",
		divergenceCount
	);

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
		const auto& trace =
			forA
			? m_traceA
			: m_traceB;

		if (trace.empty())
		{
			return false;
		}

		char filename[4096] = {};

		std::snprintf(
			filename,
			sizeof(filename),
			"0x%zX__%zu.trace",
			trace.front().address,
			trace.size()
		);

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

				TraceComparisonPersistence::savePath(
					comparisonPaths[
						forA ? 0 : 1
					],
					filename
				);

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
