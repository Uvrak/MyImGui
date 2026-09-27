#pragma once
// Small game-side data adapter for the copied CharacterEditor backpack UI.
// Character fitting and editor-only model manipulation are not game inventory state.
#include "EquipmentCatalog.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
namespace Equipment {
struct Item { const char* id; const char* label; bool available = true; };
struct Fit { bool worn = false; };
struct Inventory {
    std::array<Item,itemCount> items{{
        {"helmet","Lederhelm"},{"armor","Lederruestung"},{"trousers","Lederhose"},
        {"boot-right","Stiefelpaar"},{"boot-left","Stiefelpaar"},
        {"trousers-1","Dunkle Schnuerhose"},{"trousers-2","Leinenhose"},
        {"trousers-3","Gruene Bundhose"},{"trousers-4","Verstaerkte Lederhose"},
        {"trousers-5","Blaue Kniehose"}}};
    std::array<Fit,itemCount> fits{};
    unsigned mask() const { unsigned m=0;for(int i=0;i<itemCount;++i)if(fits[i].worn)m|=1u<<i;return m; }
    bool equip(int slot,bool worn) {
        if(slot<0||slot>=itemCount||!items[slot].available)return false;
        if(boots(slot))fits[3].worn=fits[4].worn=worn;
        else {if(worn&&trousers(slot))for(int i=0;i<itemCount;++i)if(trousers(i))fits[i].worn=false;fits[slot].worn=worn;}
        return true;
    }
};
}
