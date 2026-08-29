#pragma once

#include "imgui.h"

#include <array>
#include <cstddef>

#include <string>

enum class DosBoxAction
{
    SwitchView,
    Forward,
    Backward,
    TurnLeft,
    TurnRight,
    Order,
    Protect,
    Rest,
    Search,
    Bash,
    Unlock,
    QuickReference,
    ViewCharacter,
    Count
};

struct DosBoxKeyBinding
{
    DosBoxAction action =
        DosBoxAction::Forward;

    const char* functionName = "";
    const char* defaultKeyName = "";
    const char* dosCommand = "";

    ImGuiKey customKey =
        ImGuiKey_None;
};

class DosBoxKeyBindings
{
public:
    DosBoxKeyBindings();

    DosBoxKeyBinding& binding(
        DosBoxAction action
    );

    const DosBoxKeyBinding& binding(
        DosBoxAction action
    ) const;

    const std::array<
        DosBoxKeyBinding,
        static_cast<std::size_t>(
            DosBoxAction::Count
            )
    >& bindings() const;

    void setCustomKey(
        DosBoxAction action,
        ImGuiKey key
    );

    void clearCustomKey(
        DosBoxAction action
    );

    bool load(
        const std::string& filename
    );

    bool save(
        const std::string& filename
    ) const;

    bool germanKeyboardForUsGame() const;

    void setGermanKeyboardForUsGame(
        bool enabled
    );

private:
    static constexpr std::size_t
        BindingCount =
        static_cast<std::size_t>(
            DosBoxAction::Count
            );

    std::array<
        DosBoxKeyBinding,
        BindingCount
    > m_bindings;

    bool m_germanKeyboardForUsGame =
        false;
};