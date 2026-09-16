#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

// MM3's own software cursor, verified in the German GOG version. The complete
// signature deliberately rejects other builds instead of guessing an address.
class MM3CursorPatch
{
public:
    enum class Status { Inactive, Searching, Ambiguous, Visible, Hidden };

    std::size_t address() const { return m_start ? m_start + OpcodeOffset : 0; }
    Status status() const { return m_status; }

    template<class WriteByte>
    void update(const uint8_t* memory, std::size_t size, bool mm3Active,
                bool hidden, WriteByte writeByte)
    {
        const std::size_t end = (std::min)(size, std::size_t{0xA0000});
        if (!memory || end < SignatureSize)
        {
            m_start = 0;
            m_status = mm3Active ? Status::Searching : Status::Inactive;
            return;
        }

        if (m_start && !matches(memory, end, m_start))
            m_start = 0; // The overlay was replaced; never restore unknown code.

        if (!mm3Active)
        {
            // MM3 may have released or repurposed its memory. Restoration is
            // only allowed by an explicit visibility change while MM3 runs.
            m_start = 0;
            m_status = Status::Inactive;
            return;
        }

        if (!m_start)
        {
            std::size_t candidate = 0;
            for (std::size_t offset = 0x10000; offset <= end - SignatureSize; ++offset)
            {
                if (!matches(memory, end, offset))
                    continue;
                if (candidate)
                {
                    m_status = Status::Ambiguous;
                    return; // Multiple copies cannot be distinguished safely.
                }
                candidate = offset;
            }
            m_start = candidate;
        }

        if (!m_start)
        {
            m_status = Status::Searching;
            return;
        }

        const uint8_t opcode = hidden ? HiddenOpcode : OriginalOpcode;
        if (memory[address()] != opcode)
            writeByte(address(), opcode);
        m_status = hidden ? Status::Hidden : Status::Visible;
    }

private:
    // CALL sprite_draw (E8 42 08) -> TEST AX,0842h (A9 42 08).
    // Both consume three bytes; TEST has no memory or stack effects. Only its
    // flags change, and all callers of this cursor routine overwrite them.
    // Background save/restore, INT 33h input polling and screen copies still run.
    static constexpr uint8_t OriginalOpcode = 0xE8;
    static constexpr uint8_t HiddenOpcode = 0xA9;
    static constexpr std::size_t OpcodeOffset = 22;
    static constexpr std::size_t SignatureSize = 30;
    static const uint8_t* signature()
    {
        static constexpr uint8_t bytes[] = {
            0x8C,0xC8,0x8E,0xD8,0x8B,0x1E,0x3E,0x0B,
            0x8B,0x0E,0x3C,0x0B,0x2B,0xFF,0x8E,0x06,
            0xF6,0x09,0xC5,0x36,0x4C,0x0B,0xE8,0x42,
            0x08,0x5F,0x5E,0x1F,0x5D,0xC3
        };
        return bytes;
    }

    static bool matches(const uint8_t* memory, std::size_t end, std::size_t start)
    {
        if (start > end - SignatureSize)
            return false;
        for (std::size_t i = 0; i < SignatureSize; ++i)
        {
            const uint8_t byte = memory[start + i];
            if (i == OpcodeOffset)
            {
                if (byte != OriginalOpcode && byte != HiddenOpcode)
                    return false;
            }
            else if (byte != signature()[i])
                return false;
        }
        return true;
    }

    std::size_t m_start = 0;
    Status m_status = Status::Inactive;
};
