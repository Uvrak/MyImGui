#pragma once
#include <array>
namespace CanegmEquipment {
inline constexpr int itemCount=10;
inline constexpr std::array<const char*,itemCount> itemIds{"helmet","armor","trousers","boot-right","boot-left","trousers-1","trousers-2","trousers-3","trousers-4","trousers-5"};
inline constexpr bool trousers(int i){return i==2||(i>=5&&i<itemCount);}
inline constexpr bool boots(int i){return i==3||i==4;}
inline constexpr int category(int i){return trousers(i)?2:i;}
}
