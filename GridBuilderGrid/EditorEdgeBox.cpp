#include "EditorEdgeBox.h"

#include "imgui.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
    const std::filesystem::path
        edgeBoxSettingsPath =
        "../settings/editor_edge_box.cfg";

    const std::filesystem::path
        edgeDirectory =
        "resources/icons/edges";

    struct EdgeBoxSetting
    {
        std::string edgeId;
        bool enabled = true;
    };

    std::vector<EdgeBoxSetting>
        loadEdgeBoxSettings()
    {
        std::vector<EdgeBoxSetting>
            settings;

        std::ifstream file(
            edgeBoxSettingsPath
        );

        if (!file)
        {
            return settings;
        }

        std::string line;

        while (std::getline(file, line))
        {
            const std::size_t separator =
                line.find('=');

            if (separator ==
                std::string::npos)
            {
                continue;
            }

            EdgeBoxSetting setting;

            setting.edgeId =
                line.substr(
                    0,
                    separator
                );

            const std::string values =
                line.substr(
                    separator + 1
                );

            setting.enabled =
                values.empty() ||
                values[0] != '0';
            if (!setting.edgeId.empty())
            {
                settings.push_back(
                    std::move(setting)
                );
            }
        }

        return settings;
    }
    std::vector<std::string>
        loadEdgeIds()
    {
        std::vector<std::string>
            edgeIds;

        std::error_code error;

        if (!std::filesystem::exists(
            edgeDirectory,
            error
        ))
        {
            return edgeIds;
        }

        for (const auto& entry :
            std::filesystem::directory_iterator(
                edgeDirectory,
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

            edgeIds.push_back(
                entry.path().stem().string()
            );
        }

        std::sort(
            edgeIds.begin(),
            edgeIds.end()
        );

        const std::vector<EdgeBoxSetting>
            settings =
            loadEdgeBoxSettings();

        if (settings.empty())
        {
            return edgeIds;
        }

        std::vector<std::string>
            orderedEdgeIds;

        orderedEdgeIds.reserve(
            edgeIds.size()
        );

        for (const EdgeBoxSetting& setting :
            settings)
        {
            const auto position =
                std::find(
                    edgeIds.begin(),
                    edgeIds.end(),
                    setting.edgeId
                );

            if (position ==
                edgeIds.end())
            {
                continue;
            }

            orderedEdgeIds.push_back(
                *position
            );

            edgeIds.erase(
                position
            );
        }

        orderedEdgeIds.insert(
            orderedEdgeIds.end(),
            edgeIds.begin(),
            edgeIds.end()
        );

        return orderedEdgeIds;
    }

    SvgButtonDefinition makeButtonDefinition(
        const std::string& edgeId,
        int edgeIndex
    )
    {
        SvgButtonDefinition definition;

        definition.value =
            edgeIndex;

        definition.iconPath =
            (
                edgeDirectory /
                (edgeId + ".svg")
                ).string();

        definition.tooltip =
            edgeId;

        return definition;
    }
   

    std::vector<SvgButtonDefinition>
        makeButtonDefinitions(
            const std::vector<std::string>&
            edgeIds
        )
    {
        std::vector<SvgButtonDefinition>
            definitions;

        definitions.reserve(
            edgeIds.size()
        );

        for (std::size_t index = 0;
            index < edgeIds.size();
            ++index)
        {
            definitions.push_back(
                makeButtonDefinition(
                    edgeIds[index],
                    static_cast<int>(index)
                )
            );
        }
        return definitions; 
    }
}

EditorEdgeBox::EditorEdgeBox(
    SDL_Renderer* renderer
)
    : m_window(
        "Editor Edge Box",
        {
            .movable = true,
            .resizable = true,
            .collapsible = true,
            .closable = false,
            .titleBar = true,
            .autoResizeHeight = true
        }
    ),
    m_edgeIds(
        loadEdgeIds()
    ),
    m_buttonBar(
        renderer,
        makeButtonDefinitions(
            m_edgeIds
        )
    )
{
    
    for (std::size_t index = 0;
        index < m_edgeIds.size();
        ++index)
    {
        m_enabledEdgeIndices.push_back(
            static_cast<int>(index)
        );
    }
    
    loadEnabledStates();
    
    m_lastButtonOrder =
        m_buttonBar.orderedValues();
    
    
}
bool EditorEdgeBox::draw(
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

        for (const std::string& edgeId :
            m_edgeIds)
        {
            if (colorIdResolver(edgeId).empty())
            {
                setColorId(
                    edgeId,
                    defaultColorId
                );
            }
        }
    }

    m_colorMenu.synchronizeButtons(
        m_buttonBar,
        m_edgeIds,
        colorPalette,
        colorIdResolver
    );

    ImGui::SetNextWindowSize(
        ImVec2(150.0f, 105.0f),
        ImGuiCond_FirstUseEver
    );

    const bool windowVisible =
        m_window.begin();

    bool edgeButtonClicked = false;
    bool edgeButtonRightClicked = false;

    if (windowVisible)
    {
        if (m_edgeIds.empty())
        {
            ImGui::TextDisabled(
                "No edge SVG files"
            );
        }
        else
        {
            edgeButtonClicked =
                m_buttonBar.draw(
                    m_activeEdgeIndex,
                    [this,
                    &colorPalette,
                    &colorIdResolver,
                    &setColorId,
                    &removeColor,
                    &edgeButtonRightClicked](
                        const SvgButtonDefinition& definition,
                        ImVec2 buttonMin,
                        ImVec2 buttonMax
                        )
                    {
                        if (definition.value < 0 ||
                            definition.value >=
                            static_cast<int>(
                                m_edgeIds.size()
                                ))
                        {
                            return false;
                        }

                        const std::string& edgeId =
                            m_edgeIds[
                                definition.value
                            ];

                        const std::string assignedColorId =
                            colorIdResolver
                            ? colorIdResolver(edgeId)
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
                            m_activeEdgeIndex =
                                definition.value;

                            edgeButtonRightClicked = true;
                        }

                        return drawEdgeOverlay(
                            definition,
                            buttonMin,
                            buttonMax,
                            colorPalette,
                            assignedColorId,
                            [this,
                            &setColorId,
                            &edgeId,
                            edgeIndex = definition.value](
                                const std::string& colorId
                                )
                            {
                                if (setColorId)
                                {
                                    setColorId(
                                        edgeId,
                                        colorId
                                    );
                                }

                                m_activeEdgeIndex =
                                    edgeIndex;
                            },
                            removeColor
                        );
                    }
                );

            const std::vector<int> currentOrder =
                m_buttonBar.orderedValues();

            if (currentOrder !=
                m_lastButtonOrder)
            {
                m_lastButtonOrder =
                    currentOrder;

                saveSettings();
            }
        }
    }

    m_window.end();

    return edgeButtonClicked ||
        edgeButtonRightClicked;
}

bool EditorEdgeBox::drawEdgeOverlay(
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
        m_activeEdgeIndex =
            definition.value;
    }

    const bool checkboxClicked =
        drawEdgeCheckbox(
            "##EdgeEnabled",
            definition.value,
            buttonMin,
            buttonMax
        );

    return rightClicked ||
        checkboxClicked;
}
bool EditorEdgeBox::drawEdgeCheckbox(
    const char* id,
    int edgeIndex,
    ImVec2 buttonMin,
    ImVec2 buttonMax
)
{
    const float size = 13.0f;
    const float margin = 3.0f;

    const ImVec2 boxMin(
        buttonMax.x - size - margin,
        buttonMin.y + margin
    );

    const ImVec2 boxMax(
        boxMin.x + size,
        boxMin.y + size
    );

    const ImVec2 mousePosition =
        ImGui::GetMousePos();


    const bool hovered =
        mousePosition.x >= boxMin.x &&
        mousePosition.x <= boxMax.x &&
        mousePosition.y >= boxMin.y &&
        mousePosition.y <= boxMax.y;

    const bool clicked =
        hovered &&
        ImGui::IsMouseClicked(
            ImGuiMouseButton_Left
        );

    bool enabled =
        isEdgeEnabled(
            edgeIndex
        );

    if (clicked)
    {
        enabled = !enabled;

        setEdgeEnabled(
            edgeIndex,
            enabled
        );
        saveSettings();
    }

    ImDrawList* drawList =
        ImGui::GetWindowDrawList();

    drawList->AddRectFilled(
        boxMin,
        boxMax,
        hovered
        ? IM_COL32(55, 55, 55, 240)
        : IM_COL32(25, 25, 25, 230),
        2.0f
    );

    drawList->AddRect(
        boxMin,
        boxMax,
        IM_COL32(255, 255, 255, 255),
        2.0f
    );

    if (enabled)
    {
        drawList->AddLine(
            ImVec2(
                boxMin.x + 2.5f,
                boxMin.y + 6.5f
            ),
            ImVec2(
                boxMin.x + 5.0f,
                boxMin.y + 10.0f
            ),
            IM_COL32(255, 220, 80, 255),
            2.0f
        );

        drawList->AddLine(
            ImVec2(
                boxMin.x + 5.0f,
                boxMin.y + 10.0f
            ),
            ImVec2(
                boxMin.x + 11.0f,
                boxMin.y + 2.5f
            ),
            IM_COL32(255, 220, 80, 255),
            2.0f
        );
    }

    return clicked;
}

bool EditorEdgeBox::isEdgeEnabled(
    int edgeIndex
) const
{
    return std::find(
        m_enabledEdgeIndices.begin(),
        m_enabledEdgeIndices.end(),
        edgeIndex
    ) != m_enabledEdgeIndices.end();
}

void EditorEdgeBox::setEdgeEnabled(
    int edgeIndex,
    bool enabled
)
{
    const auto position =
        std::find(
            m_enabledEdgeIndices.begin(),
            m_enabledEdgeIndices.end(),
            edgeIndex
        );

    if (enabled)
    {
        if (position ==
            m_enabledEdgeIndices.end())
        {
            m_enabledEdgeIndices.push_back(
                edgeIndex
            );
        }
    }
    else if (position !=
        m_enabledEdgeIndices.end())
    {
        const bool wasActive =
            m_activeEdgeIndex ==
            edgeIndex;

        m_enabledEdgeIndices.erase(
            position
        );

        if (wasActive &&
            !m_enabledEdgeIndices.empty())
        {
            m_activeEdgeIndex =
                m_enabledEdgeIndices.front();
        }
    }
}

void EditorEdgeBox::saveSettings() const
{
    std::error_code error;

    std::filesystem::create_directories(
        edgeBoxSettingsPath.parent_path(),
        error
    );

    std::ofstream file(
        edgeBoxSettingsPath,
        std::ios::trunc
    );

    if (!file)
    {
        return;
    }

    const std::vector<int> order =
        m_buttonBar.orderedValues();

    for (const int edgeIndex :
    order)
    {
        if (edgeIndex < 0 ||
            edgeIndex >=
            static_cast<int>(
                m_edgeIds.size()
                ))
        {
            continue;
        }

        file
            << m_edgeIds[edgeIndex]
            << '='
            << (
                isEdgeEnabled(edgeIndex)
                ? 1
                : 0
                )
            << '\n';
    }
}

void EditorEdgeBox::loadEnabledStates()
{
    std::ifstream file(
        edgeBoxSettingsPath
    );

    if (!file)
    {
        return;
    }

    std::string line;

    while (std::getline(file, line))
    {
        const std::size_t separator =
            line.find('=');

        if (separator ==
            std::string::npos)
        {
            continue;
        }

        const std::string edgeId =
            line.substr(
                0,
                separator
            );

        std::string values =
            line.substr(
                separator + 1
            );

        std::replace(
            values.begin(),
            values.end(),
            ',',
            ' '
        );

        std::istringstream valueStream(
            values
        );

        int enabledValue = 1;

        if (!(valueStream >>
            enabledValue))
        {
            continue;
        }

        const bool enabled =
            enabledValue != 0;

        
        const auto iterator =
            std::find(
                m_edgeIds.begin(),
                m_edgeIds.end(),
                edgeId
            );

        if (iterator ==
            m_edgeIds.end())
        {
            continue;
        }

        const int edgeIndex =
            static_cast<int>(
                std::distance(
                    m_edgeIds.begin(),
                    iterator
                )
                );

        setEdgeEnabled(
            edgeIndex,
            enabled
        );
    }
}

const std::string&
EditorEdgeBox::activeEdgeId() const
{
    static const std::string emptyId;

    if (m_activeEdgeIndex < 0 ||
        m_activeEdgeIndex >=
        static_cast<int>(
            m_edgeIds.size()
            ))
    {
        return emptyId;
    }

    return m_edgeIds[
        m_activeEdgeIndex
    ];
}

SDL_Texture* EditorEdgeBox::edgeTexture(
    const std::string& edgeId,
    int size
)
{
    (void)size;

    const auto position =
        std::find(
            m_edgeIds.begin(),
            m_edgeIds.end(),
            edgeId
        );

    if (position == m_edgeIds.end())
    {
        return nullptr;
    }

    const int edgeIndex =
        static_cast<int>(
            std::distance(
                m_edgeIds.begin(),
                position
            )
            );

    return m_buttonBar.texture(
        edgeIndex
    );
}

void EditorEdgeBox::refreshTextures()
{
    m_buttonBar.refreshTextures();
}
void EditorEdgeBox::addOrRefreshEdge(
    const std::string& edgeId
)
{
    const auto position =
        std::find(
            m_edgeIds.begin(),
            m_edgeIds.end(),
            edgeId
        );

    if (position != m_edgeIds.end())
    {
        return;
    }

    const int edgeIndex =
        static_cast<int>(
            m_edgeIds.size()
            );

    SvgButtonDefinition definition =
        makeButtonDefinition(
            edgeId,
            edgeIndex
        );

    m_edgeIds.push_back(
        edgeId
    );

    m_enabledEdgeIndices.push_back(
        edgeIndex
    );

    m_buttonBar.addButton(
        std::move(definition)
    );

    m_activeEdgeIndex =
        edgeIndex;
}

bool EditorEdgeBox::renameEdge(
    const std::string& oldEdgeId,
    const std::string& newEdgeId
)
{
    const auto position =
        std::find(
            m_edgeIds.begin(),
            m_edgeIds.end(),
            oldEdgeId
        );

    if (position ==
        m_edgeIds.end())
    {
        return false;
    }

    const int edgeIndex =
        static_cast<int>(
            std::distance(
                m_edgeIds.begin(),
                position
            )
            );

    SvgButtonDefinition definition =
        makeButtonDefinition(
            newEdgeId,
            edgeIndex
        );

    if (!m_buttonBar.replaceButton(
        edgeIndex,
        std::move(definition)
    ))
    {
        return false;
    }

    *position =
        newEdgeId;

    m_activeEdgeIndex =
        edgeIndex;

    saveSettings();

    return true;
}

void EditorEdgeBox::setActiveEdgeId(
    const std::string& edgeId
)
{
    const auto position =
        std::find(
            m_edgeIds.begin(),
            m_edgeIds.end(),
            edgeId
        );

    if (position == m_edgeIds.end())
    {
        return;
    }

    m_activeEdgeIndex =
        static_cast<int>(
            std::distance(
                m_edgeIds.begin(),
                position
            )
            );
}

void EditorEdgeBox::clearActiveEdge()
{
    m_activeEdgeIndex = -1;
}

void EditorEdgeBox::openColorMenu(
    const std::string& edgeId,
    const std::string& assignedColorId,
    const ColorMenu::AssignColorCallback&
    assignColor
)
{
    const auto position =
        std::find(
            m_edgeIds.begin(),
            m_edgeIds.end(),
            edgeId
        );

    if (position == m_edgeIds.end())
    {
        return;
    }

    const int edgeIndex =
        static_cast<int>(
            std::distance(
                m_edgeIds.begin(),
                position
            )
            );

    m_activeEdgeIndex =
        edgeIndex;

    m_colorMenu.requestOpen(
        edgeIndex,
        assignedColorId,
        assignColor
    );
}


bool EditorEdgeBox::isColorMenuOpen() const
{
    return m_colorMenu.isOpen();
}

void EditorEdgeBox::cycleActiveEdge(
    int direction
)
{
    if (m_enabledEdgeIndices.empty() ||
        direction == 0)
    {
        return;
    }

    const double currentTime =
        ImGui::GetTime();

    constexpr double cycleDelay =
        0.15;

    if (m_lastEdgeCycleTime >= 0.0 &&
        currentTime -
        m_lastEdgeCycleTime <
        cycleDelay)
    {
        return;
    }

    const auto position =
        std::find(
            m_enabledEdgeIndices.begin(),
            m_enabledEdgeIndices.end(),
            m_activeEdgeIndex
        );

    if (position ==
        m_enabledEdgeIndices.end())
    {
        m_activeEdgeIndex =
            m_enabledEdgeIndices.front();

        m_lastEdgeCycleTime =
            currentTime;

        return;
    }

    const int currentIndex =
        static_cast<int>(
            std::distance(
                m_enabledEdgeIndices.begin(),
                position
            )
            );

    const int count =
        static_cast<int>(
            m_enabledEdgeIndices.size()
            );

    const int nextIndex =
        direction > 0
        ? (currentIndex + 1) % count
        : (currentIndex - 1 + count) %
        count;

    m_activeEdgeIndex =
        m_enabledEdgeIndices[
            nextIndex
        ];

    m_lastEdgeCycleTime =
        currentTime;
}