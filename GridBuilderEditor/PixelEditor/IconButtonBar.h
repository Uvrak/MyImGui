#pragma once

#include <string>
#include <vector>

struct IconButtonDefinition
{
    int value = 0;

    std::string icon;
    std::string tooltip;

    float width = 48.0f;
    float height = 48.0f;
};

class IconButtonBar
{
public:
    explicit IconButtonBar(
        std::vector<IconButtonDefinition>
        definitions
    );

    bool draw(
        int& activeValue
    );

private:
    struct Button
    {
        IconButtonDefinition definition;
        std::string id;
    };

    std::vector<Button> m_buttons;
};
