#include "EditorMiscBox.h"

#include "imgui.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace
{
    const std::filesystem::path
        miscDirectory =
        "resources/icons/misc";

    std::vector<std::string>
        loadMiscIds()
    {
        std::vector<std::string>
            miscIds;

        std::error_code error;

        if (!std::filesystem::exists(
            miscDirectory,
            error
        ))
        {
            return miscIds;
        }

        for (const auto& entry :
            std::filesystem::directory_iterator(
                miscDirectory,
                error
            ))
        {
            if (error)
            {
                break;
            }

            if (!entry.is_regular_file())
            {
                continue;
            }

            const std::string extension =
                entry.path().extension().string();

            if (extension != ".svg" &&
                extension != ".SVG")
            {
                continue;
            }

            miscIds.push_back(
                entry.path().stem().string()
            );
        }

        std::sort(
            miscIds.begin(),
            miscIds.end()
        );

        return miscIds;
    }

    SvgButtonDefinition makeButtonDefinition(
        const std::string& miscId,
        int miscIndex
    )
    {
        SvgButtonDefinition definition;

        definition.value =
            miscIndex;

        definition.iconPath =
            (
                miscDirectory /
                (miscId + ".svg")
                ).string();

        definition.tooltip =
            miscId;

        definition.rotation =
            SvgButtonRotation::None;

        return definition;
    }

    std::vector<SvgButtonDefinition>
        makeButtonDefinitions(
            const std::vector<std::string>&
            miscIds
        )
    {
        std::vector<SvgButtonDefinition>
            definitions;

        definitions.reserve(
            miscIds.size()
        );

        for (std::size_t index = 0;
            index < miscIds.size();
            ++index)
        {
            definitions.push_back(
                makeButtonDefinition(
                    miscIds[index],
                    static_cast<int>(index)
                )
            );
        }

        return definitions;
    }
}

EditorMiscBox::EditorMiscBox(
    SDL_Renderer* renderer
)
    : m_window(
        "Editor Misc Box",
        {
            .movable = true,
            .resizable = true,
            .collapsible = true,
            .closable = false,
            .titleBar = true,
            .autoResizeHeight = true
        }
    ),
    m_miscIds(
        loadMiscIds()
    ),
    m_buttonBar(
        renderer,
        makeButtonDefinitions(
            m_miscIds
        )
    )
{}


bool EditorMiscBox::draw(
    MapColorPalette& colorPalette,
    const ColorIdResolver& colorIdResolver,
    const SetColorIdCallback& setColorId,
	const RemoveColorCallback& removeColor  
)
{
    m_colorMenu.beginFrame();

    if (!colorPalette.colors().empty() &&
        colorIdResolver &&
        setColorId)
    {
        const std::string& defaultColorId =
            colorPalette.colors().front().id;

        for (const std::string& miscId :
            m_miscIds)
        {
            if (colorIdResolver(miscId).empty())
            {
                setColorId(
                    miscId,
                    defaultColorId
                );
            }
        }
    }

    m_colorMenu.synchronizeButtons(
        m_buttonBar,
        m_miscIds,
        colorPalette,
        colorIdResolver
    );

    ImGui::SetNextWindowSize(
        ImVec2(150.0f, 105.0f),
        ImGuiCond_FirstUseEver
    );

    const bool windowVisible =
        m_window.begin();

    bool miscButtonClicked = false;

    bool miscButtonRightClicked = false;

    if (windowVisible)
    {
        if (m_miscIds.empty())
        {
            ImGui::TextDisabled(
                "No misc SVG files"
            );
        }
        else
        {
            miscButtonClicked =
                m_buttonBar.draw(
                    m_activeMiscIndex,
                    [this,
                    &colorPalette,
                    &colorIdResolver,
                    &setColorId,
                    &removeColor,
                    &miscButtonRightClicked](
                        const SvgButtonDefinition&
                        definition,
                        ImVec2 buttonMin,
                        ImVec2 buttonMax
                        )
                    {
                        if (definition.value < 0 ||
                            definition.value >=
                            static_cast<int>(
                                m_miscIds.size()
                                ))
                        {
                            return false;
                        }

                        const std::string& miscId =
                            m_miscIds[
                                definition.value
                            ];

                        const std::string
                            assignedColorId =
                            colorIdResolver
                            ? colorIdResolver(miscId)
                            : std::string{};

                        const bool rightClicked =
                            ImGui::IsMouseHoveringRect(
                                buttonMin,
                                buttonMax
                            ) &&
                            ImGui::IsMouseClicked(
                                ImGuiMouseButton_Right
                            );

                        if (rightClicked)
                        {
                            m_activeMiscIndex =
                                definition.value;
                            miscButtonRightClicked = true;
                        }

                        const bool overlayClicked =
                            drawMiscOverlay(
                            definition,
                            buttonMin,
                            buttonMax,
                            colorPalette,
                            assignedColorId,
                            [this,
                            &setColorId,
                            &miscId,
                            miscIndex = definition.value](
                                const std::string& colorId
                                )
                            {
                                if (setColorId)
                                {
                                    setColorId(
                                        miscId,
                                        colorId
                                    );
                                }

                                m_activeMiscIndex =
                                    miscIndex;
                            },
                            removeColor
                        );
                        return rightClicked ||
                            overlayClicked;
                    }
                );
        }
    }

    m_window.end();

    return miscButtonClicked ||
        miscButtonRightClicked;
}

bool EditorMiscBox::drawMiscOverlay(
    const SvgButtonDefinition& definition,
    ImVec2 buttonMin,
    ImVec2 buttonMax,
    MapColorPalette& colorPalette,
    const std::string& assignedColorId,
    const ColorMenu::AssignColorCallback& assignColor,
	const RemoveColorCallback& removeColor
)
{
    m_colorMenu.draw(
        definition,
        buttonMin,
        buttonMax,
        m_buttonBar,
        colorPalette,
        assignedColorId,
        assignColor,
		removeColor
    );

    const bool rightClicked =
        ImGui::IsMouseHoveringRect(
            buttonMin,
            buttonMax
        ) &&
        ImGui::IsMouseClicked(
            ImGuiMouseButton_Right
        );

    if (rightClicked)
    {
        m_activeMiscIndex =
            definition.value;
    }

    return rightClicked;
}

const std::string&
EditorMiscBox::activeMiscId() const
{
    static const std::string emptyId;

    if (m_activeMiscIndex < 0 ||
        m_activeMiscIndex >=
        static_cast<int>(
            m_miscIds.size()
            ))
    {
        return emptyId;
    }

    return m_miscIds[
        m_activeMiscIndex
    ];
}

void EditorMiscBox::setActiveMiscId(
    const std::string& miscId
)
{
    const auto position =
        std::find(
            m_miscIds.begin(),
            m_miscIds.end(),
            miscId
        );

    if (position == m_miscIds.end())
    {
        m_activeMiscIndex = -1;
        return;
    }

    m_activeMiscIndex =
        static_cast<int>(
            std::distance(
                m_miscIds.begin(),
                position
            )
        );
}

void EditorMiscBox::clearActiveMisc()
{
    m_activeMiscIndex = -1;
}

void EditorMiscBox::openColorMenu(
    const std::string& miscId,
    const std::string& assignedColorId,
    const ColorMenu::AssignColorCallback&
    assignColor
)
{
    const auto position =
        std::find(
            m_miscIds.begin(),
            m_miscIds.end(),
            miscId
        );

    if (position == m_miscIds.end())
    {
        return;
    }

    const int miscIndex =
        static_cast<int>(
            std::distance(
                m_miscIds.begin(),
                position
            )
            );

    m_activeMiscIndex =
        miscIndex;

    m_colorMenu.requestOpen(
        miscIndex,
        assignedColorId,
        assignColor
    );
}

SDL_Texture* EditorMiscBox::miscTexture(
    const std::string& miscId,
    int size
)
{
    (void)size;

    const auto position =
        std::find(
            m_miscIds.begin(),
            m_miscIds.end(),
            miscId
        );

    if (position == m_miscIds.end())
    {
        return nullptr;
    }

    const int miscIndex =
        static_cast<int>(
            std::distance(
                m_miscIds.begin(),
                position
            )
            );

    return m_buttonBar.texture(
        miscIndex
    );
}

void EditorMiscBox::refreshTextures()
{
    m_buttonBar.refreshTextures();
}

void EditorMiscBox::addOrRefreshMisc(
    const std::string& miscId
)
{
    const auto position =
        std::find(
            m_miscIds.begin(),
            m_miscIds.end(),
            miscId
        );

    if (position != m_miscIds.end())
    {
        m_buttonBar.refreshTextures();
        return;
    }

    const int miscIndex =
        static_cast<int>(
            m_miscIds.size()
            );

    SvgButtonDefinition definition =
        makeButtonDefinition(
            miscId,
            miscIndex
        );

    m_miscIds.push_back(
        miscId
    );

    m_buttonBar.addButton(
        std::move(definition)
    );

    m_activeMiscIndex =
        miscIndex;
}

bool EditorMiscBox::renameMisc(
    const std::string& oldMiscId,
    const std::string& newMiscId
)
{
    const auto position =
        std::find(
            m_miscIds.begin(),
            m_miscIds.end(),
            oldMiscId
        );

    if (position ==
        m_miscIds.end())
    {
        return false;
    }

    const int miscIndex =
        static_cast<int>(
            std::distance(
                m_miscIds.begin(),
                position
            )
            );

    SvgButtonDefinition definition =
        makeButtonDefinition(
            newMiscId,
            miscIndex
        );

    if (!m_buttonBar.replaceButton(
        miscIndex,
        std::move(definition)
    ))
    {
        return false;
    }

    *position =
        newMiscId;

    m_activeMiscIndex =
        miscIndex;

    return true;
}
