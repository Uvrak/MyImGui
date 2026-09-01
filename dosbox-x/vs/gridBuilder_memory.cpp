#include "gridbuilder_memory.h"

#include <algorithm>

#include "mem.h"
#include <utility>
#include <iterator>
#include <cstdlib>

bool GridBuilderMemory::captureSnapshot()
{
    constexpr size_t SnapshotLimit =
        640U * 1024U;

    const size_t snapshotSize =
        std::min(
            MemSize,
            SnapshotLimit
        );

    if(snapshotSize == 0)
    {
        m_snapshot.clear();
        return false;
    }

    m_previousSnapshot =
        std::move(
            m_snapshot
        );

    m_snapshot.resize(
        snapshotSize
    );

    for(size_t address = 0;
        address < snapshotSize;
        ++address)
    {
        m_snapshot[address] =
            phys_readb(
                static_cast<PhysPt>(
                    address
                    )
            );
    }

    return true;
}

bool GridBuilderMemory::hasSnapshot() const
{
    return !m_snapshot.empty();
}

const std::vector<uint8_t>&
GridBuilderMemory::snapshot() const
{
    return m_snapshot;
}

void GridBuilderMemory::resetCandidates()
{
    m_candidateAddresses.clear();
    m_candidatesInitialized = false;

    m_snapshot.clear();
    m_previousSnapshot.clear();
}

std::vector<size_t>
GridBuilderMemory::changedAddresses() const
{
    std::vector<size_t> addresses;

    if(m_previousSnapshot.size() !=
        m_snapshot.size())
    {
        return addresses;
    }

    for(size_t address = 0;
        address < m_snapshot.size();
        ++address)
    {
        if(m_previousSnapshot[address] !=
            m_snapshot[address])
        {
            addresses.push_back(
                address
            );
        }
    }

    return addresses;
}

void GridBuilderMemory::refineChangedAddresses(
    const std::vector<uint8_t>&
    expectedDifferences
)
{
    if(m_previousSnapshot.size() !=
        m_snapshot.size())
    {
        return;
    }

    std::vector<size_t> changed;

    for(size_t address = 0;
        address < m_snapshot.size();
        ++address)
    {
        const int previousValue =
            m_previousSnapshot[address];

        const int currentValue =
            m_snapshot[address];

        const int difference =
            std::abs(
                currentValue -
                previousValue
            );

        const bool differenceAccepted =
            expectedDifferences.empty()
            ? difference != 0
            : std::find(
                expectedDifferences.begin(),
                expectedDifferences.end(),
                static_cast<uint8_t>(
                    difference
                    )
            ) != expectedDifferences.end();

        if(differenceAccepted)
        {
            changed.push_back(
                address
            );
        }
    }

    if(changed.empty())
    {
        m_candidateAddresses.clear();
        return;
    }

    if(!m_candidatesInitialized)
    {
        m_candidateAddresses =
            std::move(
                changed
            );

        m_candidatesInitialized = true;

        return;
    }

    std::vector<size_t> refined;

    std::set_intersection(
        m_candidateAddresses.begin(),
        m_candidateAddresses.end(),
        changed.begin(),
        changed.end(),
        std::back_inserter(
            refined
        )
    );

    m_candidateAddresses =
        std::move(
            refined
        );
    }

void GridBuilderMemory::
refineUnchangedAddresses()
{
    if(m_previousSnapshot.size() !=
        m_snapshot.size())
    {
        return;
    }

    std::vector<size_t> unchanged;

    for(size_t address = 0;
        address < m_snapshot.size();
        ++address)
    {
        if(m_previousSnapshot[address] ==
            m_snapshot[address])
        {
            unchanged.push_back(
                address
            );
        }
    }

    if(!m_candidatesInitialized)
    {
        m_candidateAddresses =
            std::move(
                unchanged
            );

        m_candidatesInitialized =
            true;

        return;
    }

    std::vector<size_t> refined;

    std::set_intersection(
        m_candidateAddresses.begin(),
        m_candidateAddresses.end(),
        unchanged.begin(),
        unchanged.end(),
        std::back_inserter(
            refined
        )
    );

    m_candidateAddresses =
        std::move(
            refined
        );
}

const std::vector<size_t>&
GridBuilderMemory::candidateAddresses() const
{
    return m_candidateAddresses;
}
