#include "MightAndMagic1SpellWindow.h"

#include "imgui.h"

namespace
{
    struct Mm1Spell
    {
        int level;
        int number;

        const char* name;
        const char* cost;
        const char* usage;
        const char* target;
        const char* description;
    };

    constexpr Mm1Spell ClericSpells[] =
    {
        {
            1,
            1,
            "Awaken",
            "1 SP",
            "Combat",
            "Sleeping party members",
            "Awakens all sleeping members of the party."
        },
        {
            1,
            2,
            "Bless",
            "1 SP",
            "Combat",
            "Entire party",
            "Improves the party's accuracy for the duration of combat."
        },
        {
            1,
            3,
            "Blind",
            "1 SP",
            "Combat",
            "One monster",
            "Blinds one monster and reduces its chance to hit."
        },
        {
            1,
            4,
            "First Aid",
            "1 SP",
            "Anytime",
            "One character",
            "Restores 8 hit points to one character."
        },
        {
            1,
            5,
            "Light",
            "1 SP",
            "Non-combat",
            "Entire party",
            "Adds one light factor. Multiple castings accumulate."
        },

                    {
                1,
                6,
                "Power Cure",
                "Caster level SP + 1 Gem",
                "Anytime",
                "One character",
                "Restores 1-10 hit points per experience level of the caster."
            },
            {
                1,
                7,
                "Protection From Fear",
                "1 SP",
                "Anytime",
                "Entire party",
                "Increases the party's resistance to fear for one day."
            },
            {
                1,
                8,
                "Turn Undead",
                "1 SP",
                "Combat",
                "All undead monsters",
                "Attempts to destroy undead enemies based on the caster's level."
            }
    };

    constexpr Mm1Spell SorcererSpells[] =
    {
        {
            1,
            1,
            "Awaken",
            "1 SP",
            "Combat",
            "Sleeping party members",
            "Awakens all sleeping members of the party."
        },
        {
            1,
            2,
            "Detect Magic",
            "1 SP",
            "Non-combat",
            "Caster",
            "Identifies magical items and shows their remaining charges."
        },
        {
            1,
            3,
            "Energy Blast",
            "Caster level SP + 1 Gem",
            "Combat",
            "One monster",
            "Deals 1-4 energy damage per experience level of the caster."
        },
        {
            1,
            4,
            "Flame Arrow",
            "1 SP",
            "Combat",
            "One monster",
            "Deals 1-6 fire damage unless the monster resists fire."
        },
        {
            1,
            5,
            "Leather Skin",
            "1 SP",
            "Anytime",
            "Entire party",
            "Improves the party's protection from attacks for one day."
        },
        {
            1,
            6,
            "Light",
            "1 SP",
            "Non-combat",
            "Entire party",
            "Adds one light factor. Multiple castings accumulate."
        },
        {
            1,
            7,
            "Location",
            "1 SP",
            "Non-combat",
            "Entire party",
            "Shows the current map sector, coordinates and facing direction."
        },
        {
            1,
            8,
            "Sleep",
            "1 SP",
            "Combat",
            "Up to five monsters",
            "Prevents affected monsters from acting until they are damaged or recover."
        }
    };
}

void MightAndMagic1SpellWindow::draw(
    bool* isOpen
)
{
    if (!isOpen ||
        !*isOpen)
    {
        return;
    }

    if (!ImGui::Begin(
        "MM1 Spell Reference",
        isOpen
    ))
    {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted(
        "Might and Magic 1 spells"
    );

    ImGui::Separator();

    const char* spellTypes[] =
    {
        "Cleric",
        "Sorcerer"
    };

    ImGui::Combo(
        "Spell type",
        &m_spellType,
        spellTypes,
        2
    );

    ImGui::SliderInt(
        "Maximum spell level",
        &m_maxSpellLevel,
        0,
        7
    );

    ImGui::Separator();

    const Mm1Spell* spells =
        m_spellType == 0
        ? ClericSpells
        : SorcererSpells;

    constexpr int spellCount =
        static_cast<int>(
            sizeof(ClericSpells) /
            sizeof(ClericSpells[0])
            );

    if (m_maxSpellLevel >= 1)
    {
        if (ImGui::BeginTable(
            "##MM1Spells",
            4,
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable
        ))
        {
            ImGui::TableSetupColumn("Spell");
            ImGui::TableSetupColumn("Cost");
            ImGui::TableSetupColumn("Usage");
            ImGui::TableSetupColumn("Target");
            ImGui::TableHeadersRow();

            for (int index = 0;
                index < spellCount;
                ++index)
            {
                const Mm1Spell& spell =
                    spells[index];
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text(
                    "%d-%d %s",
                    spell.level,
                    spell.number,
                    spell.name
                );

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(
                    spell.cost
                );

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(
                    spell.usage
                );

                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(
                    spell.target
                );

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip(
                        "%s",
                        spell.description
                    );
                }
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}
