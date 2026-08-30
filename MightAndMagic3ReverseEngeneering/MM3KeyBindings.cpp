#include "MM3KeyBindings.h"

#include <fstream>

namespace MightAndMagic3
{
    KeyBindings::KeyBindings()
        : m_bindings{
            KeyBinding{
                KeyAction::ToggleButtonMode,
                "Button Mode",
                0
            },
            KeyBinding{
                KeyAction::Forward,
                "Forward",
                0
            },
            KeyBinding{
                KeyAction::Backward,
                "Backward",
                0
            },
            KeyBinding{
                KeyAction::TurnLeft,
                "Turn Left",
                0
            },
            KeyBinding{
                KeyAction::TurnRight,
                "Turn Right",
                0
            },
            KeyBinding{
                KeyAction::Cancel,
                "Cancel",
                0
            },
            KeyBinding{
                KeyAction::ButtonUp,
                "Button Up",
                0
            },
            KeyBinding{
                KeyAction::ButtonDown,
                "Button Down",
                0
            },
            KeyBinding{
                KeyAction::ButtonLeft,
                "Button Left",
                0
            },
            KeyBinding{
                KeyAction::ButtonRight,
                "Button Right",
                0
            },
            KeyBinding{
                KeyAction::ActivateButton,
                "Activate Button",
                0
            }
        }
    {}

    KeyBinding& KeyBindings::binding(
        KeyAction action
    )
    {
        return m_bindings[
            static_cast<std::size_t>(
                action
                )
        ];
    }

    const KeyBinding& KeyBindings::binding(
        KeyAction action
    ) const
    {
        return m_bindings[
            static_cast<std::size_t>(
                action
                )
        ];
    }

    int KeyBindings::key(
        KeyAction action
    ) const
    {
        const KeyBinding& item =
            binding(action);

        if (item.customKey !=
            0)
        {
            return item.customKey;
        }

        return item.defaultKey;
    }

    void KeyBindings::setDefaultKey(
        KeyAction action,
        int key
    )
    {
        binding(action).defaultKey =
            key;
    }

    void KeyBindings::setCustomKey(
        KeyAction action,
        int key
    )
    {
        binding(action).customKey =
            key;
    }

    void KeyBindings::clearCustomKey(
        KeyAction action
    )
    {
        binding(action).customKey =
            0;
    }

    const std::array<
        KeyBinding,
        static_cast<std::size_t>(
            KeyAction::Count
            )
    >& KeyBindings::bindings() const
    {
        return m_bindings;
    }

    bool KeyBindings::load(
        const std::string& filename
    )
    {
        std::ifstream file(
            filename
        );

        if (!file)
        {
            return false;
        }

        std::size_t actionIndex = 0;
        int keyValue = 0;

        while (file >>
            actionIndex >>
            keyValue)
        {
            if (actionIndex >=
                m_bindings.size())
            {
                continue;
            }

            m_bindings[actionIndex].
                customKey =
                keyValue;
                    
        }

        return true;
    }

    bool KeyBindings::save(
        const std::string& filename
    ) const
    {
        std::ofstream file(
            filename
        );

        if (!file)
        {
            return false;
        }

        for (std::size_t index = 0;
            index < m_bindings.size();
            ++index)
        {
            file
                << index
                << ' '
                << 
                    m_bindings[index].
                    customKey
                    
                << '\n';
        }

        return true;
    }
}
