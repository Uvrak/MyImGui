#pragma once

#include <vector>
#include <functional>
#include <cstring>

#include "TraceComparison.h"

namespace DosBoxMemoryTools
{
	class TraceComparisonWindow
	{
	public:
		using PhysicalAddrResolver = std::function<bool(const RuntimeInstruction&, size_t&)>;

		TraceComparisonWindow() = default;

		void draw(
			const std::vector<RuntimeInstruction>& traceA,
			const std::vector<RuntimeInstruction>& traceB,
			char* targetText,
			size_t targetTextSize,
			PhysicalAddrResolver tryGetPhysicalMemoryAddress
		);

		static bool loadTraceFromFile(const std::string& filename, std::vector<RuntimeInstruction>& trace);
		static bool saveTraceToFile(const std::string& filename, const std::vector<RuntimeInstruction>& trace);

		// Accessors for traces and filenames to allow gradual migration
		const std::vector<RuntimeInstruction>& traceA() const { return m_traceA; }
		const std::vector<RuntimeInstruction>& traceB() const { return m_traceB; }

		const char* traceAFilename() const { return m_traceAFilename[0] ? m_traceAFilename : ""; }
		const char* traceBFilename() const { return m_traceBFilename[0] ? m_traceBFilename : ""; }

		void setTraceA(std::vector<RuntimeInstruction> t) { m_traceA = std::move(t); }
		void setTraceB(std::vector<RuntimeInstruction> t) { m_traceB = std::move(t); }

		void setTraceAFilename(const char* fn) { if (fn) strncpy_s(m_traceAFilename, sizeof(m_traceAFilename), fn, _TRUNCATE); }
		void setTraceBFilename(const char* fn) { if (fn) strncpy_s(m_traceBFilename, sizeof(m_traceBFilename), fn, _TRUNCATE); }

	private:
		std::vector<RuntimeInstruction> m_traceA;
		std::vector<RuntimeInstruction> m_traceB;

		char m_traceAFilename[4096] = {};
		char m_traceBFilename[4096] = {};
		// Selected instruction index for A/B comparison and scroll flag
		size_t m_selectedTraceIndex = static_cast<size_t>(-1);
		bool m_scrollToSelectedTrace = false;

	public:
		size_t selectedTraceIndex() const { return m_selectedTraceIndex; }
		void setSelectedTraceIndex(size_t v) { m_selectedTraceIndex = v; }
		void setScrollToSelectedTrace(bool v) { m_scrollToSelectedTrace = v; }
		bool takeScrollToSelectedTrace() { bool v = m_scrollToSelectedTrace; m_scrollToSelectedTrace = false; return v; }
	public:
		// Open file dialog and load into A or B
		bool openAndLoadTrace(bool forA);
		// Open file dialog and save A or B
		bool openAndSaveTrace(bool forA);
	};
}
