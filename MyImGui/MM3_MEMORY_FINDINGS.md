# Might and Magic III - Memory Findings

## Party TP

Character data appears to be organized by party slot.
The TP address distance between party slots is 0x12F (303 bytes).

| Party Slot | TP Address |
|------------|------------|
| 1 | 0x2C037 |
| 2 | 0x2C166 |
| 3 | 0x2C295 |
| 4 | 0x2C3C4 |
| 5 | 0x2C4F3 |
| 6 | 0x2C622 |

Important:
These addresses belong to party positions, not specific characters.
Changing the party order moves the character values between these slots.

TP values were verified using memory writes and the MM3 character display.


## Equipment / Inventory Investigation

Address under investigation:

0x1BF8F

Writing values to this address changes the equipment/item symbol.

Observed values:

| Value | Observed Symbol / Type |
|------:|------------------------|
| 0 | Empty |
| 1 | Left Hand |
| 2 | Right Hand |
| 3 | Armor |
| 4 | Crossbow |
| 5 | Helmet |
| 6 | Left Glove |
| 7 | Medal |
| 8 | Ring |
| 9 | Boots |
| 10 | Robe |
| 11 | Necklace |
| 12 | Belt |
| 13 | Two-Handed |

Exact purpose of 0x1BF8F is not yet confirmed.
It appears to represent an equipment/item symbol or equipment type,
but should NOT yet be treated as an item ID.


## Memory Scanner

Implemented:

- Persistent pinned addresses and descriptions
- Description sorting
- Write Value popup
- DOSBox-X WRITE IPC command
- Memory writes using phys_writeb()
- Automatic snapshot refresh after writing

Next:

- Add Byte / Short / Int value types
- Find MM3 X coordinate
- Find MM3 Y coordinate
- Find MM3 facing direction
- Continue investigating MM3 inventory/item structures


## Item Explorer

Created generic, game-independent foundation:

- Item.h
- ItemSource.h
- ItemExplorerWindow.h/.cpp

Created MM3 source:

- MightAndMagic3ItemSource.h/.cpp

Current MM3 source contains only a test item.
Real MM3 item data still needs to be identified.

Goal:

Memory Scanner
    -> identify runtime item data

MM3 ItemSource
    -> interpret MM3 item data

ItemExplorerWindow
    -> generic UI usable by MM3, MM1, Wizardry, etc.