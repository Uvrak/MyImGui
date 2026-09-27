#pragma once
#include <algorithm>
#include <cmath>

namespace ow3d
{
enum class OpeningState { Open, Opening, Closed, Closing, Locked };

// Shared by doors and windows. Locking never silently moves a leaf.
class OpeningMotion
{
    float m_progress = 1.f;
    float m_target = 1.f;
    bool m_locked = false;
    bool m_blocked = false;
public:
    static constexpr float Duration = .65f;
    float progress() const { return m_progress; }
    float target() const { return m_target; }
    bool locked() const { return m_locked; }
    bool blocked() const { return m_blocked; }
    OpeningState state() const
    {
        if (m_locked) return OpeningState::Locked;
        if (m_progress != m_target)
            return m_target > m_progress ? OpeningState::Opening : OpeningState::Closing;
        return m_progress == 1.f ? OpeningState::Open : OpeningState::Closed;
    }
    bool setOpen(bool open)
    {
        if (m_locked) return false;
        m_target = open ? 1.f : 0.f;
        m_blocked = false;
        return true;
    }
    bool toggle() { return setOpen(m_target < .5f); }
    bool setLocked(bool locked)
    {
        if (locked && (m_progress != 0.f || m_target != 0.f)) return false;
        m_locked = locked;
        return true;
    }
    template<class CanMove> void update(float dt, CanMove canMove)
    {
        if (!std::isfinite(dt) || dt <= 0.f) return;
        m_blocked = false;
        float remaining = std::min(dt, .1f) / Duration;
        while (!m_locked && remaining > 0.f && m_progress != m_target)
        {
            const float amount = std::min(remaining, .01f);
            remaining -= amount;
            const float next = m_progress + std::clamp(m_target - m_progress, -amount, amount);
            if (!canMove(next)) { m_blocked = true; break; }
            m_progress = next;
        }
    }
};
}
