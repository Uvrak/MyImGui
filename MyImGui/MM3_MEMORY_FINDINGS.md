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


    MM3 Character Record

Record size:
0x12F = 303 bytes

Confirmed character name starts:
Crag Hack : 0x2C042
Maximus   : 0x2C171

0x2C171 - 0x2C042 = 0x12F

## Inventory / Item IDs

MM3 inventory slots contain separate values for the displayed
item symbol/type and the actual item ID.

### Confirmed example: Party Slot 1 / Inventory Slot 9

For Sir Canegm while he occupies party slot 1:

| Data | Address | Confirmed value |
|---|---:|---:|
| Inventory Slot 9 symbol/type | 0x2BF97 | 0x02 |
| Inventory Slot 9 item ID | 0x2BFF6 | 42 |

The item ID was verified by transferring the item to another
character and back:

- Transfer away: `0x2BFF6` changed from `42` to `0`.
- Transfer back: `0x2BFF6` changed from `0` to `42`.
- Writing `41` to `0x2BFF6` changed the item in Slot 9 to
  "Platten Rüstung".
- Therefore `0x2BFF6` is confirmed as the item ID byte for
  Inventory Slot 9.

### Confirmed Item IDs

| Item ID | German item name |
|---:|---|
| 41 | Platten Rüstung |
| 42 | Schild |

### Inventory symbol/type array

The inventory symbol/type bytes appear to start at `0x2BF8F`
for party slot 1.

With 18 inventory slots, the presumed range is:

`0x2BF8F` - `0x2BFA0`

Confirmed examples:

- `0x2BF8F` = Inventory Slot 1 symbol/type
- `0x2BF97` = Inventory Slot 9 symbol/type

Changing these bytes changes the displayed item symbol rather
than the actual item identity.

The complee 18-slot symbol/type range is not yet fully verified.

### Inventory slot layout

For party slot 1, the inventory slot data follows a confirmed
parallel layout.

| Inventory Slot | Symbol/Type | Item ID |
|---:|---:|---:|
| 1 | 0x2BF8F | 0x2BFEE |
| 9 | 0x2BF97 | 0x2BFF6 |

The distance between the symbol/type byte and the corresponding
item ID byte is `0x5F`.

Therefore the current inferred layout for all 18 inventory slots is:

- Symbol/type: `0x2BF8F + (slot - 1)`
- Item ID: `0x2BFEE + (slot - 1)`

The formula is confirmed for inventory slots 1 and 9.

Confirmed item IDs:

- `1` = Lang Schwert
- `5` = Messer
- `41` = Platten Rüstung
- `42` = Schild
- `43` = Helm

### Item categories

The item-name table is 1-based.

Observed category boundary:

- Item IDs 1-33: Weapons
- Item ID 34 onward: Armor begins

Further category boundaries still need to be mapped.