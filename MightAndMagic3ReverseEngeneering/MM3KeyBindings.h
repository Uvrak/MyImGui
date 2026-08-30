#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace MightAndMagic3
{
    enum class KeyAction
    {
        ToggleButtonMode,
        Forward,
        Backward,
        TurnLeft,
        TurnRight,
        Cancel,
        ButtonUp,
        ButtonDown,
        ButtonLeft,
        ButtonRight,
        ActivateButton,
        Count
    };

    struct KeyBinding
    {
        KeyAction action =
            KeyAction::Forward;

        const char* functionName = "";

        int defaultKey = 0;
        int customKey = 0;
    };

    class KeyBindings
    {
    public:
        KeyBindings();

        KeyBinding& binding(
            KeyAction action
        );

        const KeyBinding& binding(
            KeyAction action
        ) const;

        int key(
            KeyAction action
        ) const;

        void setDefaultKey(
            KeyAction action,
            int key
        );

        void setCustomKey(
            KeyAction action,
            int key
        );

        void clearCustomKey(
            KeyAction action
        );

        const std::array<
            KeyBinding,
            static_cast<std::size_t>(
                KeyAction::Count
                )
        >& bindings() const;

        bool load(
            const std::string& filename
        );

        bool save(
            const std::string& filename
        ) const;

    private:
        static constexpr std::size_t
            BindingCount =
            static_cast<std::size_t>(
                KeyAction::Count
                );

        std::array<
            KeyBinding,
            BindingCount
        > m_bindings;
    };
}