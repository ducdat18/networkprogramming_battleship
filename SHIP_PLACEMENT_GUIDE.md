# 🚢 Ship Placement Implementation Guide

## ✅ What Was Implemented (Phase 2A - Ship Placement)

### Overview
The ship placement system allows players to interactively place their fleet on the game board before battle. This implementation connects the UI to the existing game logic in `common/src/game_state.cpp`.

---

## 🎯 Features Implemented

### 1. Interactive Ship Selection
**Files Modified**: `client/src/ui_manager.cpp` (lines 1005-1035)

- **Ship Buttons**: Each ship is now a clickable button
- **Ship Types**:
  - Aircraft Carrier (5 cells)
  - Battleship (4 cells)
  - Cruiser (3 cells)
  - Submarine (3 cells)
  - Destroyer (2 cells)
- **Status Indicators**: ❌ (not placed) → ✅ (placed)

**Code**:
```cpp
GtkWidget* ship_button = gtk_button_new_with_label(ships[i]);
g_object_set_data(G_OBJECT(ship_button), "ship_type", GINT_TO_POINTER(i));
g_signal_connect(ship_button, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);
    int ship_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "ship_type"));
    ui->selectShipForPlacement((ShipType)ship_type);
}), this);
```

### 2. Ship Rotation
**Files Modified**: `client/src/ui_manager.cpp` (lines 1041-1046, 1127-1135)

- **Rotate Button**: Toggle between horizontal and vertical orientation
- **Visual Feedback**: Preview updates in real-time when orientation changes

**Implementation**:
```cpp
void UIManager::toggleShipOrientation() {
    current_orientation = (current_orientation == HORIZONTAL) ? VERTICAL : HORIZONTAL;
    std::cout << "Orientation: " << (current_orientation == HORIZONTAL ? "HORIZONTAL" : "VERTICAL") << std::endl;

    if (player_board_area) {
        gtk_widget_queue_draw(player_board_area);  // Update preview
    }
}
```

### 3. Click-to-Place Mechanics
**Files Modified**: `client/src/ui_manager.cpp` (lines 612-634, 1137-1167)

- **Board Click Handler**: Detects which cell was clicked
- **Placement Logic**: Validates position and places ship
- **Auto-Selection**: Automatically selects next unplaced ship

**Key Function**:
```cpp
void UIManager::placeShipAt(int row, int col) {
    if (!player_board) return;

    Coordinate pos;
    pos.row = row;
    pos.col = col;

    if (player_board->placeShip(selected_ship_type, pos, current_orientation)) {
        ships_placed[selected_ship_type] = true;
        std::cout << "✅ Placed " << shipTypeToName(selected_ship_type)
                  << " at " << (char)('A' + row) << (col + 1) << std::endl;

        updateShipStatusUI();

        // Auto-select next unplaced ship
        for (int i = 0; i < NUM_SHIPS; i++) {
            if (!ships_placed[i]) {
                selected_ship_type = (ShipType)i;
                break;
            }
        }

        gtk_widget_queue_draw(player_board_area);
    } else {
        std::cout << "❌ Cannot place ship there!" << std::endl;
    }
}
```

### 4. Visual Preview System
**Files Modified**: `client/src/ui_manager.cpp` (lines 355-401, 636-660)

- **Hover Detection**: Tracks mouse position over board
- **Real-time Preview**: Shows ship outline before placement
- **Color Coding**:
  - **Green** (semi-transparent): Valid placement
  - **Red** (semi-transparent): Invalid placement

**Preview Rendering**:
```cpp
// Draw ship placement preview
if (ready_battle_button && hover_row >= 0 && hover_col >= 0 && !ships_placed[selected_ship_type]) {
    int length = getShipLength(selected_ship_type);
    Coordinate pos;
    pos.row = hover_row;
    pos.col = hover_col;

    bool valid = player_board->isValidPlacement(selected_ship_type, pos, current_orientation);

    // Draw preview boxes for each cell
    for (int i = 0; i < length; i++) {
        int r = hover_row + (current_orientation == VERTICAL ? i : 0);
        int c = hover_col + (current_orientation == HORIZONTAL ? i : 0);

        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
            // Green if valid, red if invalid
            if (valid) {
                cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.3);
            } else {
                cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.3);
            }
            cairo_rectangle(cr, x + CELL_PADDING, y + CELL_PADDING,
                          CELL_SIZE - CELL_PADDING * 2, CELL_SIZE - CELL_PADDING * 2);
            cairo_fill(cr);
        }
    }
}
```

### 5. Random Placement
**Files Modified**: `client/src/ui_manager.cpp` (lines 1048-1055, 1169-1186)

- **One-Click Setup**: Instantly place all ships randomly
- **Uses Game Logic**: Calls `Board::randomPlacement()` from `common/src/game_state.cpp`
- **Validation**: Ensures no overlaps or out-of-bounds placements

**Implementation**:
```cpp
void UIManager::randomPlaceAllShips() {
    if (!player_board) return;

    std::cout << "🎲 Random placement..." << std::endl;
    player_board->randomPlacement();

    // Mark all ships as placed
    for (int i = 0; i < NUM_SHIPS; i++) {
        ships_placed[i] = true;
    }

    updateShipStatusUI();
    gtk_widget_queue_draw(player_board_area);
}
```

### 6. Ready Button Logic
**Files Modified**: `client/src/ui_manager.cpp` (lines 1057-1066, 1188-1201)

- **State Management**: Button disabled until all ships are placed
- **Validation**: Checks `allShipsPlaced()` before allowing battle
- **Transition**: Moves to game screen when ready

**Code**:
```cpp
GtkWidget* ready_btn = gtk_button_new_with_label("READY FOR BATTLE!");
ready_battle_button = ready_btn;
gtk_widget_set_sensitive(ready_btn, FALSE);  // Disabled initially

g_signal_connect(ready_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);
    if (ui->allShipsPlaced()) {
        ui->showScreen(SCREEN_GAME);
    }
}), this);
```

---

## 📁 Files Modified

### Header File: `client/include/ui_manager.h`

**New Member Variables** (lines 116-123):
```cpp
// Ship placement state
ShipType selected_ship_type;
Orientation current_orientation;
bool ships_placed[NUM_SHIPS];
GtkWidget* ship_status_labels[NUM_SHIPS];
GtkWidget* ready_battle_button;
int hover_row;
int hover_col;
```

**New Methods** (lines 194-200):
```cpp
void selectShipForPlacement(ShipType type);
void toggleShipOrientation();
void placeShipAt(int row, int col);
void randomPlaceAllShips();
void updateShipStatusUI();
bool allShipsPlaced();
void clearAllShips();
```

### Implementation File: `client/src/ui_manager.cpp`

**Constructor Update** (lines 11-25):
- Initialize ship placement state variables
- Set default orientation to HORIZONTAL
- Initialize ship status tracking arrays

**New Event Handlers**:
- `on_board_button_press()` - Detects ship placement vs. game mode (lines 612-634)
- `on_board_motion_notify()` - Tracks hover for preview (lines 636-660)

**Ship Placement Screen Updates** (lines 945-1089):
- Ship selection buttons with click handlers
- Rotate button connection
- Random placement button connection
- Ready button state management
- Status label references stored

**New Helper Methods** (lines 1121-1230):
- All 7 ship placement helper methods implemented

---

## 🎮 User Experience Flow

### Step-by-Step Usage:

1. **Navigate to Ship Placement**:
   - Login Screen → Lobby → Click "CHALLENGE" → Ship Placement Screen

2. **Place Ships**:
   - **Option A - Manual Placement**:
     1. Click a ship button (e.g., "Aircraft Carrier (5 cells)")
     2. Hover over the board - see green/red preview
     3. Click "ROTATE SHIP" to change orientation
     4. Click on the board to place
     5. Repeat for all 5 ships

   - **Option B - Random Placement**:
     1. Click "RANDOM PLACEMENT" button
     2. All ships placed instantly

3. **Ready for Battle**:
   - When all ships are placed (all ✅), "READY FOR BATTLE!" button becomes enabled
   - Click it to proceed to the game screen

### Visual Feedback:

- **Ship Status**: ❌ → ✅ as ships are placed
- **Hover Preview**:
  - **Green overlay**: Valid placement position
  - **Red overlay**: Invalid placement (out of bounds or overlapping)
- **Console Output**: Real-time placement confirmations

---

## 🔧 Technical Details

### Validation Logic
The placement validation uses `Board::isValidPlacement()` from `common/src/game_state.cpp`:

```cpp
bool Board::isValidPlacement(ShipType type, Coordinate pos, Orientation orientation) {
    int length = getShipLength(type);

    // Check bounds
    if (pos.row < 0 || pos.col < 0) return false;
    if (orientation == HORIZONTAL && pos.col + length > BOARD_SIZE) return false;
    if (orientation == VERTICAL && pos.row + length > BOARD_SIZE) return false;

    // Check for overlaps
    for (int i = 0; i < length; i++) {
        int r = pos.row + (orientation == VERTICAL ? i : 0);
        int c = pos.col + (orientation == HORIZONTAL ? i : 0);
        if (grid[r][c] != CELL_EMPTY) {
            return false;
        }
    }

    return true;
}
```

### Board State Management
- **Player Board**: Empty by default, ships added through UI
- **Ship Tracking**: `ships_placed[]` array tracks which ships are on the board
- **Auto-progression**: After placing a ship, next ship auto-selected

### Rendering Pipeline
1. `renderPlayerBoard()` - Main rendering entry point
2. `drawBoard()` - Draw grid and ships
3. Preview overlay - Draw hover preview if in placement mode
4. `gtk_widget_queue_draw()` - Trigger re-render on changes

---

## 🐛 Edge Cases Handled

1. **Out of Bounds**: Preview shows red, placement rejected
2. **Ship Overlap**: Preview shows red, placement rejected
3. **All Ships Placed**: Further placements prevented
4. **Orientation Change**: Preview updates immediately
5. **Mouse Leave Board**: Hover preview disappears
6. **Ship Re-selection**: Can click already-placed ship (won't affect board)

---

## 🚀 Build & Test

### Build:
```bash
make clean
make client
```

### Run:
```bash
./bin/battleship_client
```

### Test Flow:
1. Navigate: Login → Lobby → Challenge → Ship Placement
2. Try manual placement with rotation
3. Try random placement
4. Verify ready button only enables when all ships placed
5. Proceed to game screen

---

## 📊 Status Update

### Completion: **70% → 75%**

**What Works Now**:
- ✅ All UI screens implemented
- ✅ **Ship placement fully functional** (NEW)
- ✅ **Interactive ship selection** (NEW)
- ✅ **Rotation and preview** (NEW)
- ✅ **Random placement** (NEW)
- ✅ Validation and visual feedback
- ✅ Smooth user experience

**Still TODO**:
- ⚠️ Shooting mechanics (click to fire)
- ⚠️ Hit/miss detection display
- ⚠️ Turn-based gameplay
- ⚠️ Networking layer
- ⚠️ Server implementation

---

## 🎯 Next Priority: Shooting Mechanics

The next phase is to implement the shooting mechanics on the game screen:

1. Click handler on opponent board
2. Call `Board::processShot()`
3. Update board visuals (hit/miss)
4. Turn-based logic
5. Win/loss detection

**Files to modify**:
- `client/src/ui_manager.cpp` - Game screen click handler
- `client/src/main.cpp` - Connect shooting callback

---

## 💡 Key Learnings

1. **State Management**: UIManager tracks ship placement state separately from Board
2. **Event Handling**: Different behavior for same click event based on screen context
3. **Visual Feedback**: Real-time preview crucial for good UX
4. **Auto-progression**: Selecting next ship automatically improves workflow
5. **Validation**: Client-side validation prevents invalid placements

---

## 🔗 Related Files

- `common/include/game_state.h` - Board and ship structures
- `common/src/game_state.cpp` - Game logic (placement, validation)
- `common/include/protocol.h` - Ship types and orientations
- `UI_FIXES_SUMMARY.md` - UI improvements documentation
- `GAME_COMPLETION_GUIDE.md` - Overall project roadmap

---

Made with ⚓ by Claude Code
Last updated: 2025-10-30
Version: Phase 2A Complete
