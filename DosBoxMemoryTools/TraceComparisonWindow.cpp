#include "TraceComparisonWindow.h"

#include "imgui.h"

#include <cstdio>

#include <fstream>
#include <string>
#include <windows.h>
#include <commdlg.h>

namespace DosBoxMemoryTools
{
	void TraceComparisonWindow::draw(
		const std::vector<RuntimeInstruction>& /*traceAParam*/,
		const std::vector<RuntimeInstruction>& /*traceBParam*/,
		char* /*targetText*/,
		size_t /*targetTextSize*/,
		PhysicalAddrResolver /*tryGetPhysicalMemoryAddress*/)
	{
		ImGui::Text("Trace A: %s", traceAFilename()[0] ? traceAFilename() : "<not loaded>");
		ImGui::SameLine();
		ImGui::Text("Count: %zu", m_traceA.size());

		ImGui::Text("Trace B: %s", traceBFilename()[0] ? traceBFilename() : "<not loaded>");
		ImGui::SameLine();
		ImGui::Text("Count: %zu", m_traceB.size());

		const size_t count = (std::min)(m_traceA.size(), m_traceB.size());

		ImGui::Separator();

		for (size_t i = 0; i < count; ++i)
		{
			const TraceInstructionDifference diff = compareTraceInstructions(m_traceA[i], m_traceB[i]);
		char label[128];
		if (diff.any())
		{
			snprintf(label, sizeof(label), "#%zu: Difference", i);
		}
		else
		{
			snprintf(label, sizeof(label), "#%zu: Equal", i);
		}

		const bool isSelected = (i == m_selectedTraceIndex);
		if (ImGui::Selectable(label, isSelected))
		{
			setSelectedTraceIndex(i);
		}

		// If this entry is selected and a scroll was requested, bring it into view once
		if (isSelected && takeScrollToSelectedTrace())
		{
			ImGui::SetScrollHereY();
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
				}
				else
				{
					setTraceB(std::move(trace));
					setTraceBFilename(filename);
				}

				return true;
			}
		}

		return false;
	}

}
