#include "DisassemblySavedAddresses.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include "imgui.h"

namespace DosBoxMemoryTools
{
    using SavedAddress = DisassemblyState::SavedAddress;

    void DisassemblySavedAddresses::draw(DisassemblyState& state, const DisassemblyNavigate& goToAddress)
    {
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            160.0f
        );

        ImGui::InputText(
            "Name##SavedAddressName",
            state.m_sessionName,
            sizeof(state.m_sessionName)
        );

        ImGui::SameLine();

        if (ImGui::Button(
            "Save Address"
        ))
        {
            addSavedAddress(state);
        }

        ImGui::SetNextItemWidth(
            180.0f
        );

        if (ImGui::BeginCombo(
            "Saved Addresses",
            "Select..."
        ))
        {
            if (state.m_savedAddresses.empty())
            {
                ImGui::TextDisabled(
                    "No saved addresses."
                );
            }
            else
            {
                for (size_t i = 0;
                    i < state.m_savedAddresses.size();
                    ++i)
                {
                    const SavedAddress& entry =
                        state.m_savedAddresses[i];

                    char label[256];

                    if (entry.name.empty())
                    {
                        std::snprintf(
                            label,
                            sizeof(label),
                            "0x%05zX",
                            entry.address
                        );
                    }
                    else
                    {
                        std::snprintf(
                            label,
                            sizeof(label),
                            "0x%05zX - %s",
                            entry.address,
                            entry.name.c_str()
                        );
                    }

                    ImGui::PushID(
                        static_cast<int>(i)
                    );

                    bool deleteEntry = false;

                    if (ImGui::Selectable(
                        label
                    ))
                    {
                        goToAddress(
                            entry.address
                        );
                    }

                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem(
                            "Delete"
                        ))
                        {
                            deleteEntry = true;
                        }

                        ImGui::EndPopup();
                    }

                    if (deleteEntry)
                    {
                        state.m_savedAddresses.erase(
                            state.m_savedAddresses.begin() + i
                        );

                        saveSession(state);

                        ImGui::PopID();
                        break;
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndCombo();
        }

    }

    void DisassemblySavedAddresses::saveSession(DisassemblyState& state)
    {
        std::ofstream file(
            "../settings/dosbox_disassembly.cfg"
        );

        if (!file)
        {
            state.m_status =
                "Could not save disassembly session.";

            return;
        }

        for (const SavedAddress& entry :
            state.m_savedAddresses)
        {
            file << std::hex
                << entry.address
                << '\t'
                << entry.name
                << '\n';
        }

        state.m_status =
            "Disassembly session saved.";
    }

    void DisassemblySavedAddresses::loadSession(DisassemblyState& state)
    {
        state.m_savedAddresses.clear();

        std::ifstream file(
            "../settings/dosbox_disassembly.cfg"
        );

        if (!file)
        {
            state.m_status =
                "No disassembly session found.";

            return;
        }

        size_t address = 0;

        while (file >> std::hex >> address)
        {
            std::string name;

            std::getline(
                file,
                name
            );

            if (!name.empty() &&
                name[0] == '\t')
            {
                name.erase(
                    0,
                    1
                );
            }

            SavedAddress entry;

            entry.address =
                address;

            entry.name =
                name;

            state.m_savedAddresses.push_back(
                entry
            );
        }

        state.m_status =
            "Disassembly session loaded.";
    }

    void DisassemblySavedAddresses::addSavedAddress(DisassemblyState& state)
    {
        char* end = nullptr;

        const unsigned long long address =
            std::strtoull(
                state.m_addressText,
                &end,
                0
            );

        if (end == state.m_addressText ||
            *end != '\0')
        {
            state.m_status =
                "Invalid address.";

            return;
        }

        state.m_address =
            static_cast<size_t>(
                address
                );

        state.m_hasAddress = true;

        for (SavedAddress& entry :
            state.m_savedAddresses)
        {
            if (entry.address == state.m_address)
            {
                entry.name =
                    state.m_sessionName;

                saveSession(state);

                state.m_status =
                    "Address updated.";

                return;
            }
        }

        SavedAddress entry;

        entry.address =
            state.m_address;

        entry.name =
            state.m_sessionName;

        state.m_savedAddresses.push_back(
            entry
        );

        saveSession(state);

        state.m_status =
            "Address saved.";
    }

}
