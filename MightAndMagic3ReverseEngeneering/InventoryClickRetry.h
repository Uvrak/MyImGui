#pragma once
#include <chrono>
#include <cstdint>

namespace MightAndMagic3
{
    class InventoryClickRetry
    {
    public:
        using Clock = std::chrono::steady_clock;
        bool active() const { return m_active; }
        void reset() { m_active = false; }
        void arm(Clock::time_point now, std::uint64_t snapshot)
        {
            m_active = true;
            m_snapshot = snapshot;
            // MM3 can process the click after several 200 ms snapshots.
            // An earlier retry can toggle a newly selected item off again.
            m_due = now + std::chrono::milliseconds(1200);
        }
        bool poll(Clock::time_point now, std::uint64_t snapshot, bool confirmed)
        {
            if (!m_active || snapshot == m_snapshot) return false;
            if (confirmed) { reset(); return false; }
            if (now < m_due) return false;
            arm(now, snapshot);
            return true;
        }
    private:
        bool m_active = false;
        std::uint64_t m_snapshot = 0;
        Clock::time_point m_due{};
    };
}
