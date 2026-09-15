#include "pch.h"
#include "TraceDifferenceBaseline.h"
#include "TraceAlignment.h"

#include <algorithm>
#include <fstream>

namespace DosBoxMemoryTools
{
    void TraceDifferenceBaseline::add(
        const std::vector<RuntimeInstruction>& traceA,
        const std::vector<RuntimeInstruction>& traceB
    )
    {
        const std::vector<TraceAlignment> alignments =
            TraceAligner::align(
                traceA,
                traceB
            );

        for (const TraceAlignment& alignment :
            alignments)
        {
            if (!alignment.synchronized)
            {
                if (alignment.indexA >= traceA.size())
                {
                    continue;
                }

                const size_t address =
                    traceA[alignment.indexA].address;

                auto existing =
                    std::find_if(
                        m_entries.begin(),
                        m_entries.end(),
                        [address](
                            const TraceDifferenceBaselineEntry& entry
                            )
                        {
                            return entry.address ==
                                address;
                        }
                    );

                if (existing ==
                    m_entries.end())
                {
                    TraceDifferenceBaselineEntry entry;

                    entry.address =
                        address;

                    entry.controlFlow =
                        true;

                    m_entries.push_back(
                        entry
                    );
                }
                else
                {
                    existing->controlFlow =
                        true;
                }

                continue;
            }

            if (alignment.indexA >= traceA.size() ||
                alignment.indexB >= traceB.size())
            {
                continue;
            }

            const RuntimeInstruction& instructionA =
                traceA[alignment.indexA];

            const RuntimeInstruction& instructionB =
                traceB[alignment.indexB];

            const TraceInstructionDifference difference =
                compareTraceInstructions(
                    instructionA,
                    instructionB
                );

            if (!difference.any())
            {
                continue;
            }

            const size_t address =
                instructionA.address;

            auto existing =
                std::find_if(
                    m_entries.begin(),
                    m_entries.end(),
                    [address](
                        const TraceDifferenceBaselineEntry& entry
                        )
                    {
                        return entry.address ==
                            address;
                    }
                );

            if (existing ==
                m_entries.end())
            {
                TraceDifferenceBaselineEntry entry;

                entry.address =
                    address;

                entry.difference =
                    difference;

                m_entries.push_back(
                    entry
                );

                continue;
            }

            existing->difference.address |=
                difference.address;

            existing->difference.ax |=
                difference.ax;

            existing->difference.bx |=
                difference.bx;

            existing->difference.cx |=
                difference.cx;

            existing->difference.dx |=
                difference.dx;

            existing->difference.si |=
                difference.si;

            existing->difference.di |=
                difference.di;

            existing->difference.bp |=
                difference.bp;

            existing->difference.sp |=
                difference.sp;

            existing->difference.ds |=
                difference.ds;

            existing->difference.es |=
                difference.es;

            existing->difference.ss |=
                difference.ss;
        }
    }

    TraceInstructionDifference
        TraceDifferenceBaseline::removeKnown(
            size_t address,
            const TraceInstructionDifference& difference
        ) const
    {
        TraceInstructionDifference result =
            difference;

        const auto existing =
            std::find_if(
                m_entries.begin(),
                m_entries.end(),
                [address](
                    const TraceDifferenceBaselineEntry& entry
                    )
                {
                    return entry.address ==
                        address;
                }
            );

        if (existing ==
            m_entries.end())
        {
            return result;
        }

        result.address &=
            !existing->difference.address;

        result.ax &=
            !existing->difference.ax;

        result.bx &=
            !existing->difference.bx;

        result.cx &=
            !existing->difference.cx;

        result.dx &=
            !existing->difference.dx;

        result.si &=
            !existing->difference.si;

        result.di &=
            !existing->difference.di;

        result.bp &=
            !existing->difference.bp;

        result.sp &=
            !existing->difference.sp;

        result.ds &=
            !existing->difference.ds;

        result.es &=
            !existing->difference.es;

        result.ss &=
            !existing->difference.ss;

        return result;
    }

    bool TraceDifferenceBaseline::containsControlFlow(
        size_t address
    ) const
    {
        const auto existing =
            std::find_if(
                m_entries.begin(),
                m_entries.end(),
                [address](
                    const TraceDifferenceBaselineEntry& entry
                    )
                {
                    return entry.address ==
                        address;
                }
            );

        return existing != m_entries.end() &&
            existing->controlFlow;
    }

    void TraceDifferenceBaseline::clear()
    {
        m_entries.clear();
    }

    size_t TraceDifferenceBaseline::size() const
    {
        return m_entries.size();
    }

    bool TraceDifferenceBaseline::save(
        const std::filesystem::path& path
    ) const
    {
        std::ofstream file(
            path,
            std::ios::trunc
        );

        if (!file)
        {
            return false;
        }

        file << "TraceDifferenceBaseline 1\n";
        file << m_entries.size() << '\n';

        for (const auto& entry : m_entries)
        {
            const auto& difference =
                entry.difference;

            file
                << entry.address << ' '
                << difference.address << ' '
                << difference.ax << ' '
                << difference.bx << ' '
                << difference.cx << ' '
                << difference.dx << ' '
                << difference.si << ' '
                << difference.di << ' '
                << difference.bp << ' '
                << difference.sp << ' '
                << difference.ds << ' '
                << difference.es << ' '
                << difference.ss << '\n';
        }

        return file.good();
    }

    bool TraceDifferenceBaseline::load(
        const std::filesystem::path& path
    )
    {
        std::ifstream file(
            path
        );

        if (!file)
        {
            return false;
        }

        std::string header;

        std::getline(
            file,
            header
        );

        if (header !=
            "TraceDifferenceBaseline 1")
        {
            return false;
        }

        size_t count = 0;

        if (!(file >> count))
        {
            return false;
        }

        std::vector<TraceDifferenceBaselineEntry>
            entries;

        entries.reserve(
            count
        );

        for (size_t index = 0;
            index < count;
            ++index)
        {
            TraceDifferenceBaselineEntry entry;

            if (!(file
                >> entry.address
                >> entry.difference.address
                >> entry.difference.ax
                >> entry.difference.bx
                >> entry.difference.cx
                >> entry.difference.dx
                >> entry.difference.si
                >> entry.difference.di
                >> entry.difference.bp
                >> entry.difference.sp
                >> entry.difference.ds
                >> entry.difference.es
                >> entry.difference.ss))
            {
                return false;
            }

            entries.push_back(
                entry
            );
        }

        m_entries =
            std::move(entries);

        return true;
    }

} // namespace DosBoxMemoryTools
