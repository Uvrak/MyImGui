#include "DosBoxKeyBindings.h"

#include <fstream>

DosBoxKeyBindings::DosBoxKeyBindings()
    : m_bindings{
        DosBoxKeyBinding{
            DosBoxAction::SwitchView,
            "Switch DOSBox / GridBuilder",
            "Tab",
            ""
        },
        DosBoxKeyBinding{
            DosBoxAction::Forward,
            "Forward",
            "Up Arrow",
            "UP"
        },
        DosBoxKeyBinding{
            DosBoxAction::Backward,
            "Backward",
            "Down Arrow",
            "DOWN"
        },
        DosBoxKeyBinding{
            DosBoxAction::TurnLeft,
            "Turn Left",
            "Left Arrow",
            "LEFT"
        },
        DosBoxKeyBinding{
            DosBoxAction::TurnRight,
            "Turn Right",
            "Right Arrow",
            "RIGHT"
        },
        DosBoxKeyBinding{
            DosBoxAction::Order,
            "Order",
            "O",
            "O"
        },
        DosBoxKeyBinding{
            DosBoxAction::Protect,
            "Protect",
            "P",
            "P"
        },
        DosBoxKeyBinding{
            DosBoxAction::Rest,
            "Rest",
            "R",
            "R"
        },
        DosBoxKeyBinding{
            DosBoxAction::Search,
            "Search",
            "S",
            "S"
        },
        DosBoxKeyBinding{
            DosBoxAction::Bash,
            "Bash",
            "B",
            "B"
        },
        DosBoxKeyBinding{
            DosBoxAction::Unlock,
            "Unlock",
            "U",
            "U"
        },
        DosBoxKeyBinding{
            DosBoxAction::QuickReference,
            "Quick Reference",
            "Q",
            "Q"
        },
        DosBoxKeyBinding{
            DosBoxAction::ViewCharacter,
            "View Character",
            "#",
            "BACKSLASH"
        }
    }
{}

DosBoxKeyBinding&
DosBoxKeyBindings::binding(
    DosBoxAction action
)
{
    return m_bindings[
        static_cast<std::size_t>(
            action
            )
    ];
}

const DosBoxKeyBinding&
DosBoxKeyBindings::binding(
    DosBoxAction action
) const
{
    return m_bindings[
        static_cast<std::size_t>(
            action
            )
    ];
}

const std::array<
    DosBoxKeyBinding,
    static_cast<std::size_t>(
        DosBoxAction::Count
        )
>& DosBoxKeyBindings::bindings() const
{
    return m_bindings;
}

void DosBoxKeyBindings::setCustomKey(
    DosBoxAction action,
    ImGuiKey key
)
{
    binding(action).customKey =
        key;
}

void DosBoxKeyBindings::clearCustomKey(
    DosBoxAction action
)
{
    binding(action).customKey =
        ImGuiKey_None;
}

bool DosBoxKeyBindings::load(
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
        if (actionIndex ==
            m_bindings.size())
        {
            m_germanKeyboardForUsGame =
                keyValue != 0;

            continue;
        }

        if (actionIndex >=
            m_bindings.size())
        {
            continue;
        }

        ImGuiKey key =
            ImGuiKey_None;

        if (keyValue >=
            ImGuiKey_NamedKey_BEGIN &&
            keyValue <
            ImGuiKey_NamedKey_END)
        {
            key =
                static_cast<ImGuiKey>(
                    keyValue
                    );
        }

        m_bindings[actionIndex].
            customKey =
            key;
    }

    return true;
}

bool DosBoxKeyBindings::save(
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
            << static_cast<int>(
                m_bindings[index].
                customKey
                )
            << '\n';
    }

    file
        << m_bindings.size()
        << ' '
        << (
            m_germanKeyboardForUsGame
            ? 1
            : 0
            )
        << '\n';
    return true;
}

bool DosBoxKeyBindings::
germanKeyboardForUsGame() const
{
    return
        m_germanKeyboardForUsGame;
}

void DosBoxKeyBindings::
setGermanKeyboardForUsGame(
    bool enabled
)
{
    m_germanKeyboardForUsGame =
        enabled;
}
