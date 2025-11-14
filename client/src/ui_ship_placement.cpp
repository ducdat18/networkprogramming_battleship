#include "ui_manager.h"
#include <iostream>

// Ship placement implementation
void UIManager::selectShipForPlacement(ShipType type) {
    selected_ship_type = type;
    std::cout << "Selected ship: " << shipTypeToName(type) << std::endl;
}

void UIManager::toggleShipOrientation() {
    current_orientation = (current_orientation == HORIZONTAL) ? VERTICAL : HORIZONTAL;
    std::cout << "Orientation: " << (current_orientation == HORIZONTAL ? "HORIZONTAL" : "VERTICAL") << std::endl;

    // Redraw board to show preview with new orientation
    if (player_board_area) {
        gtk_widget_queue_draw(player_board_area);
    }
}

void UIManager::placeShipAt(int row, int col) {
    if (!player_board) return;

    Coordinate pos;
    pos.row = row;
    pos.col = col;

    // Try to place the ship
    if (player_board->placeShip(selected_ship_type, pos, current_orientation)) {
        ships_placed[selected_ship_type] = true;
        std::cout << "[OK] Placed " << shipTypeToName(selected_ship_type)
                  << " at " << (char)('A' + row) << (col + 1) << std::endl;

        updateShipStatusUI();

        // Auto-select next unplaced ship
        for (int i = 0; i < NUM_SHIPS; i++) {
            if (!ships_placed[i]) {
                selected_ship_type = (ShipType)i;
                break;
            }
        }

        // Redraw board
        if (player_board_area) {
            gtk_widget_queue_draw(player_board_area);
        }
    } else {
        std::cout << "[  ] Cannot place ship there!" << std::endl;
    }
}

void UIManager::randomPlaceAllShips() {
    if (!player_board) return;

    std::cout << "🎲 Random placement..." << std::endl;
    player_board->randomPlacement();

    // Mark all ships as placed
    for (int i = 0; i < NUM_SHIPS; i++) {
        ships_placed[i] = true;
    }

    updateShipStatusUI();

    // Redraw board
    if (player_board_area) {
        gtk_widget_queue_draw(player_board_area);
    }
}

void UIManager::updateShipStatusUI() {
    // Update ship status labels to show [OK] for placed ships
    // And gray out ship buttons for placed ships
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (ship_status_labels[i]) {
            gtk_label_set_text(GTK_LABEL(ship_status_labels[i]),
                              ships_placed[i] ? "[OK]" : "[  ]");
        }

        // Gray out (disable) ship button when placed
        if (ship_buttons[i]) {
            gtk_widget_set_sensitive(ship_buttons[i], !ships_placed[i]);
        }
    }

    // Enable/disable ready button based on all ships placed
    if (ready_battle_button) {
        gtk_widget_set_sensitive(ready_battle_button, allShipsPlaced());
    }

    std::cout << " UI Status updated" << std::endl;
}

bool UIManager::allShipsPlaced() {
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (!ships_placed[i]) {
            return false;
        }
    }
    return true;
}

void UIManager::clearAllShips() {
    if (!player_board) return;

    player_board->clearBoard();

    for (int i = 0; i < NUM_SHIPS; i++) {
        ships_placed[i] = false;
    }

    selected_ship_type = SHIP_CARRIER;
    updateShipStatusUI();

    // Force immediate redraw
    if (player_board_area && GTK_IS_WIDGET(player_board_area)) {
        gtk_widget_queue_draw(player_board_area);
        // Process pending events to force redraw immediately
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
    }

    std::cout << "🧹 All ships cleared!" << std::endl;
}

void UIManager::clearShip(ShipType type) {
    if (!player_board) return;

    // Remove the ship from board
    player_board->removeShip(type);

    // Mark as not placed
    ships_placed[type] = false;

    // Update UI
    updateShipStatusUI();

    // Force immediate redraw
    if (player_board_area && GTK_IS_WIDGET(player_board_area)) {
        gtk_widget_queue_draw(player_board_area);
        // Process pending events to force redraw immediately
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
    }

    std::cout << "🗑️ Cleared " << shipTypeToName(type) << std::endl;
}
