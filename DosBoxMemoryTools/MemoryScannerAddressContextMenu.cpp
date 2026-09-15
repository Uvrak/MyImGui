#include "pch.h"
#include "MemoryScannerAddressContextMenu.h"
#include "MemoryScannerWindow.h"

#include <cstdio>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    void MemoryScannerAddressContextMenu::drawPinnedOnly(MemoryScannerWindow& window, size_t address, uint32_t currentValue)
    {
        if (ImGui::IsItemClicked(
            ImGuiMouseButton_Right
        ))
        {
            window.m_descriptionAddress =
                address;

            const auto description =
                window.m_pinnedDescriptions.find(
                    address
                );

            if (description !=
                window.m_pinnedDescriptions.end())
            {
                std::snprintf(
                    window.m_descriptionBuffer,
                    sizeof(window.m_descriptionBuffer),
                    "%s",
                    description->second.c_str()
                );
            }
            else
            {
                window.m_descriptionBuffer[0] =
                    '\0';
            }
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem(
                "Unpin Address"
            ))
            {
                window.m_pinnedAddresses.erase(
                    address
                );

                window.m_scanner.unpinAddress(
                    address
                );

                window.savePinnedAddresses();

                window.m_selectedAddresses.erase(
                    address
                );

                window.m_selectedAddresses.clear();
            }

            if (ImGui::MenuItem(
                "Write Value..."
            ))
            {
                window.m_writeAddress =
                    address;

                window.m_writeValue =
                    currentValue;

                window.m_showWriteValuePopup =
                    true;
            }
            ImGui::Separator();

            ImGui::SetNextItemWidth(
                220.0f
            );

            ImGui::InputText(
                "Description",
                window.m_descriptionBuffer,
                sizeof(window.m_descriptionBuffer)
            );

            if (ImGui::Button(
                "Save Description"
            ))
            {
                window.m_pinnedDescriptions[
                    window.m_descriptionAddress
                ] = window.m_descriptionBuffer;

                window.savePinnedAddresses();
            }

            if (ImGui::MenuItem(
                "Copy Address"
            ))
            {
                char copyText[32];

                std::snprintf(
                    copyText,
                    sizeof(copyText),
                    "0x%05zX",
                    address
                );

                ImGui::SetClipboardText(
                    copyText
                );
            }

            ImGui::Separator();

            if (ImGui::MenuItem(
                "Copy Current Value"
            ))
            {
                char valueText[32];

                std::snprintf(
                    valueText,
                    sizeof(valueText),
                    "%u",
                    static_cast<unsigned int>(
                        currentValue
                        )
                );

                ImGui::SetClipboardText(
                    valueText
                );
            }

            if (ImGui::MenuItem(
                "Copy All"
            ))
            {
                char text[128];

                std::snprintf(
                    text,
                    sizeof(text),
                    "0x%05zX  Previous: -  Current: %u  Difference: -",
                    address,
                    static_cast<unsigned int>(
                        currentValue
                        )
                );

                ImGui::SetClipboardText(
                    text
                );
            }

            ImGui::EndPopup();
        }
    }

    void MemoryScannerAddressContextMenu::drawCandidate(MemoryScannerWindow& window, const MemoryCandidate& candidate, bool pinned)
    {
        if (ImGui::IsItemClicked(
            ImGuiMouseButton_Right
        ))
        
        {
            window.m_descriptionAddress =
                candidate.address;

            const auto description =
                window.m_pinnedDescriptions.find(
                    candidate.address
                );

            if (description !=
                window.m_pinnedDescriptions.end())
            {
                std::snprintf(
                    window.m_descriptionBuffer,
                    sizeof(window.m_descriptionBuffer),
                    "%s",
                    description->second.c_str()
                );
            }
            else
            {
                window.m_descriptionBuffer[0] =
                    '\0';
            }
        }

        if (ImGui::BeginPopupContextItem())
        {

            if (ImGui::MenuItem(
                "Write Value..."
            ))
            {
                window.m_writeAddress =
                    candidate.address;

                window.m_writeValue =
                    candidate.currentValue;

                window.m_showWriteValuePopup =
                    true;
            }

            ImGui::Separator();

            if (pinned)
            {
                if (ImGui::MenuItem(
                    "Unpin Address"
                ))
                {
                    window.m_pinnedAddresses.erase(
                        candidate.address
                    );

                    window.m_scanner.unpinAddress(
                        candidate.address
                    );
                    window.savePinnedAddresses();
                }
            }
            else
            {
                    if (ImGui::MenuItem(
                    "Pin Address"
                ))
                {
                    window.m_pinnedAddresses.insert(
                        candidate.address
                    );

                    window.m_scanner.pinAddress(
                        candidate.address
                    );
                    window.savePinnedAddresses();
                }
            }

            if (pinned)
            {
                ImGui::Separator();

                ImGui::SetNextItemWidth(
                    220.0f
                );

                ImGui::InputText(
                    "Description",
                    window.m_descriptionBuffer,
                    sizeof(window.m_descriptionBuffer)
                );

                if (ImGui::Button(
                    "Save Description"
                ))
                {
                    window.m_pinnedDescriptions[
                        window.m_descriptionAddress
                    ] = window.m_descriptionBuffer;

                    window.savePinnedAddresses();
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem(
                "Copy Address"
            ))
            {
                char addressText[32];

                std::snprintf(
                    addressText,
                    sizeof(addressText),
                    "0x%05zX",
                    candidate.address
                );

                ImGui::SetClipboardText(
                    addressText
                );
            }

           

            if (ImGui::MenuItem(
                "Copy Previous Value"
            ))
            {
                char valueText[32];

                std::snprintf(
                    valueText,
                    sizeof(valueText),
                    "%u",
                    static_cast<unsigned int>(
                        candidate.previousValue
                        )
                );

                ImGui::SetClipboardText(
                    valueText
                );
            }
            if (ImGui::MenuItem(
                "Copy Difference"
            ))
            {
                const int difference =
                    static_cast<int>(
                        candidate.currentValue
                        ) -
                    static_cast<int>(
                        candidate.previousValue
                        );

                char valueText[32];

                std::snprintf(
                    valueText,
                    sizeof(valueText),
                    "%d",
                    difference
                );

                ImGui::SetClipboardText(
                    valueText
                );
            }

            if (ImGui::MenuItem(
                "Copy All"
            ))
            {
                const int difference =
                    static_cast<int>(
                        candidate.currentValue
                        ) -
                    static_cast<int>(
                        candidate.previousValue
                        );

                char text[128];

                std::snprintf(
                    text,
                    sizeof(text),
                    "0x%05zX  Previous: %u  Current: %u  Difference: %d",
                    candidate.address,
                    static_cast<unsigned int>(
                        candidate.previousValue
                        ),
                    static_cast<unsigned int>(
                        candidate.currentValue
                        ),
                    difference
                );

                ImGui::SetClipboardText(
                    text
                );
            }

            ImGui::EndPopup();
        }

    }
}
