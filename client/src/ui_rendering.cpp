#include "ui_manager.h"
#include "asset_manager.h"
#include <cmath>
#include <iostream>

#define CELL_SIZE 45
#define CELL_PADDING 2
#define BOARD_MARGIN 20

// Board rendering implementation
void UIManager::renderPlayerBoard(cairo_t* cr) {
    if (!cr) {
        std::cerr << "ERROR: renderPlayerBoard - cairo context is NULL!" << std::endl;
        return;
    }
    if (!player_board) {
        std::cerr << "DEBUG: renderPlayerBoard - player_board is NULL" << std::endl;
        return;
    }
    drawBoard(cr, player_board, true);

    // Draw ship placement preview (only in ship placement mode)
    if (ready_battle_button && hover_row >= 0 && hover_col >= 0 && !ships_placed[selected_ship_type]) {
        int length = getShipLength(selected_ship_type);
        Coordinate pos;
        pos.row = hover_row;
        pos.col = hover_col;

        // Check if placement is valid
        bool valid = player_board->isValidPlacement(selected_ship_type, pos, current_orientation);

        // Draw preview of ship
        cairo_set_line_width(cr, 2.0);
        for (int i = 0; i < length; i++) {
            int r = hover_row + (current_orientation == VERTICAL ? i : 0);
            int c = hover_col + (current_orientation == HORIZONTAL ? i : 0);

            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                double x = BOARD_MARGIN + c * CELL_SIZE;
                double y = BOARD_MARGIN + r * CELL_SIZE;

                // Draw preview box (green if valid, red if invalid)
                if (valid) {
                    cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.3);
                } else {
                    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.3);
                }
                cairo_rectangle(cr, x + CELL_PADDING, y + CELL_PADDING,
                              CELL_SIZE - CELL_PADDING * 2, CELL_SIZE - CELL_PADDING * 2);
                cairo_fill(cr);

                // Draw border
                if (valid) {
                    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
                } else {
                    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
                }
                cairo_rectangle(cr, x + CELL_PADDING, y + CELL_PADDING,
                              CELL_SIZE - CELL_PADDING * 2, CELL_SIZE - CELL_PADDING * 2);
                cairo_stroke(cr);
            }
        }
    }
}

void UIManager::renderOpponentBoard(cairo_t* cr) {
    if (!opponent_board) return;
    drawBoard(cr, opponent_board, false);

    // Draw active animations (explosions, splashes) on opponent board
    if (animation_manager && opponent_board) {
        for (int row = 0; row < BOARD_SIZE; row++) {
            for (int col = 0; col < BOARD_SIZE; col++) {
                const Animation* anim = animation_manager->getAnimationAt(row, col);
                if (anim && !anim->finished) {
                    double x = BOARD_MARGIN + col * CELL_SIZE + CELL_PADDING;
                    double y = BOARD_MARGIN + row * CELL_SIZE + CELL_PADDING;
                    double size = CELL_SIZE - CELL_PADDING * 2;

                    if (anim->type == ANIM_EXPLOSION) {
                        // Subtle explosion effect
                        double scale = sin(anim->progress * M_PI); // 0 -> 1 -> 0
                        double radius = scale * size * 0.45;
                        double cx = x + size/2;
                        double cy = y + size/2;

                        // Outer orange glow (reduced intensity)
                        cairo_pattern_t* glow = cairo_pattern_create_radial(cx, cy, 0, cx, cy, radius * 1.3);
                        cairo_pattern_add_color_stop_rgba(glow, 0, 1.0, 0.5, 0.0, scale * 0.6);
                        cairo_pattern_add_color_stop_rgba(glow, 0.5, 1.0, 0.3, 0.0, scale * 0.4);
                        cairo_pattern_add_color_stop_rgba(glow, 1, 0.8, 0.0, 0.0, 0);
                        cairo_set_source(cr, glow);
                        cairo_arc(cr, cx, cy, radius * 1.3, 0, 2 * M_PI);
                        cairo_fill(cr);
                        cairo_pattern_destroy(glow);

                        // Inner flash (more subtle)
                        cairo_pattern_t* flash = cairo_pattern_create_radial(cx, cy, 0, cx, cy, radius * 0.8);
                        cairo_pattern_add_color_stop_rgba(flash, 0, 1.0, 0.9, 0.6, scale * 0.7);
                        cairo_pattern_add_color_stop_rgba(flash, 0.5, 1.0, 0.6, 0.0, scale * 0.5);
                        cairo_pattern_add_color_stop_rgba(flash, 1, 0.9, 0.2, 0.0, 0);
                        cairo_set_source(cr, flash);
                        cairo_arc(cr, cx, cy, radius * 0.8, 0, 2 * M_PI);
                        cairo_fill(cr);
                        cairo_pattern_destroy(flash);

                    } else if (anim->type == ANIM_SPLASH) {
                        // Subtle water splash - expanding blue rings
                        double cx = x + size/2;
                        double cy = y + size/2;

                        for (int ring = 0; ring < 2; ring++) {
                            double radius = (5 + anim->progress * 12) + ring * 4;
                            double alpha = (1.0 - anim->progress) * (1.0 - ring * 0.4);

                            cairo_set_source_rgba(cr,
                                ColorScheme::OCEAN_FOAM_R,
                                ColorScheme::OCEAN_FOAM_G,
                                ColorScheme::OCEAN_FOAM_B,
                                alpha * 0.5);
                            cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
                            cairo_set_line_width(cr, 2.0);
                            cairo_stroke(cr);
                        }
                    }
                }
            }
        }
    }

    // Draw target selection highlight
    if (has_target_selected && selected_target_row >= 0 && selected_target_col >= 0) {
        double x = BOARD_MARGIN + selected_target_col * CELL_SIZE;
        double y = BOARD_MARGIN + selected_target_row * CELL_SIZE;

        // Pulsing glow effect for selected target
        double pulse = 0.5 + 0.3 * sin(glow_pulse * 2);
        cairo_set_source_rgba(cr, ColorScheme::CORAL_R, ColorScheme::CORAL_G, ColorScheme::CORAL_B, pulse);
        cairo_set_line_width(cr, 4.0);
        cairo_rectangle(cr, x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4);
        cairo_stroke(cr);

        // Crosshair on target
        cairo_set_source_rgba(cr, ColorScheme::CORAL_R, ColorScheme::CORAL_G, ColorScheme::CORAL_B, 0.8);
        cairo_set_line_width(cr, 2.0);
        double center_x = x + CELL_SIZE / 2;
        double center_y = y + CELL_SIZE / 2;
        // Horizontal line
        cairo_move_to(cr, x + 5, center_y);
        cairo_line_to(cr, x + CELL_SIZE - 5, center_y);
        cairo_stroke(cr);
        // Vertical line
        cairo_move_to(cr, center_x, y + 5);
        cairo_line_to(cr, center_x, y + CELL_SIZE - 5);
        cairo_stroke(cr);
    }
}

void UIManager::drawBoard(cairo_t* cr, Board* board, bool is_player_board) {
    int width = BOARD_SIZE * CELL_SIZE + BOARD_MARGIN * 2;
    int height = BOARD_SIZE * CELL_SIZE + BOARD_MARGIN * 2;

    // Draw animated wave background
    drawWaveBackground(cr, width, height);

    // Draw grid cells (without ships)
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            double x = BOARD_MARGIN + col * CELL_SIZE;
            double y = BOARD_MARGIN + row * CELL_SIZE;

            CellState state = board->getCell(row, col);
            drawCell(cr, row, col, state, is_player_board);

            // Draw grid lines with ocean foam color
            cairo_set_source_rgba(cr, ColorScheme::NAVAL_BLUE_R,
                                     ColorScheme::NAVAL_BLUE_G,
                                     ColorScheme::NAVAL_BLUE_B, 0.5);
            cairo_set_line_width(cr, 1.5);
            cairo_rectangle(cr, x, y, CELL_SIZE, CELL_SIZE);
            cairo_stroke(cr);
        }
    }

    // Draw ships as stretched sprites (after all cells)
    // Only show ships on player's board, or on opponent board if sunk
    if (board) {
        const Ship* ships = board->getShips();
        if (ships) {
            for (int i = 0; i < NUM_SHIPS; i++) {
                // Check if ship is placed (position is valid)
                if (ships[i].position.row >= 0 && ships[i].position.col >= 0) {
                    // Show all player ships, or only sunk opponent ships
                    if (is_player_board || ships[i].is_sunk) {
                        drawShip(cr, ships[i]);
                    }
                }
            }
        }
    }

    // Draw coordinate labels with sand color
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12);
    cairo_set_source_rgb(cr, ColorScheme::SAND_R,
                             ColorScheme::SAND_G,
                             ColorScheme::SAND_B);

    // Column labels (1-10)
    for (int col = 0; col < BOARD_SIZE; col++) {
        char label[3];
        snprintf(label, sizeof(label), "%d", col + 1);
        double x = BOARD_MARGIN + col * CELL_SIZE + CELL_SIZE / 2 - 5;
        cairo_move_to(cr, x, BOARD_MARGIN - 5);
        cairo_show_text(cr, label);
    }

    // Row labels (A-J)
    for (int row = 0; row < BOARD_SIZE; row++) {
        char label[2] = {(char)('A' + row), '\0'};
        double y = BOARD_MARGIN + row * CELL_SIZE + CELL_SIZE / 2 + 5;
        cairo_move_to(cr, 5, y);
        cairo_show_text(cr, label);
    }
}

void UIManager::drawCell(cairo_t* cr, int row, int col, CellState state, bool is_player_board) {
    double x = BOARD_MARGIN + col * CELL_SIZE + CELL_PADDING;
    double y = BOARD_MARGIN + row * CELL_SIZE + CELL_PADDING;
    double size = CELL_SIZE - CELL_PADDING * 2;

    AssetManager* assets = AssetManager::getInstance();

    // Base cell color - ocean water
    cairo_set_source_rgba(cr, ColorScheme::OCEAN_DEEP_R,
                             ColorScheme::OCEAN_DEEP_G,
                             ColorScheme::OCEAN_DEEP_B, 0.3);
    cairo_rectangle(cr, x, y, size, size);
    cairo_fill(cr);

    // Draw cell state
    switch (state) {
        case CELL_SHIP: {
            // Ships are now drawn as stretched sprites in drawBoard()
            // Just show water here
            break;
        }

        case CELL_HIT: {
            // Draw small red X marker to indicate hit
            cairo_set_source_rgb(cr, ColorScheme::HIT_R, ColorScheme::HIT_G, ColorScheme::HIT_B);
            cairo_set_line_width(cr, 3.0);
            double cx = x + size/2;
            double cy = y + size/2;
            double mark_size = 10;
            cairo_move_to(cr, cx - mark_size, cy - mark_size);
            cairo_line_to(cr, cx + mark_size, cy + mark_size);
            cairo_move_to(cr, cx + mark_size, cy - mark_size);
            cairo_line_to(cr, cx - mark_size, cy + mark_size);
            cairo_stroke(cr);
            break;
        }

        case CELL_MISS: {
            // Draw small blue circle for miss
            cairo_set_source_rgb(cr, ColorScheme::MISS_R,
                                   ColorScheme::MISS_G,
                                   ColorScheme::MISS_B);
            cairo_arc(cr, x + size/2, y + size/2, 6, 0, 2 * M_PI);
            cairo_fill(cr);
            break;
        }

        case CELL_SUNK: {
            // Sunk ship - stretched sprite is drawn in drawBoard() with dark overlay
            // Explosion effect disabled to prevent crash/freeze
            // assets->drawExplosion(cr, x + size/2, y + size/2, size / 60.0);

            // Red outline to emphasize
            cairo_set_source_rgba(cr, ColorScheme::HIT_R,
                                    ColorScheme::HIT_G,
                                    ColorScheme::HIT_B, 0.8);
            cairo_set_line_width(cr, 2);
            cairo_rectangle(cr, x + 1, y + 1, size - 2, size - 2);
            cairo_stroke(cr);
            break;
        }

        case CELL_EMPTY:
        default:
            break;
    }
}

void UIManager::drawGlowEffect(cairo_t* cr, double x, double y, double radius) {
    // Subtle ocean glow
    cairo_set_source_rgba(cr, ColorScheme::OCEAN_FOAM_R,
                             ColorScheme::OCEAN_FOAM_G,
                             ColorScheme::OCEAN_FOAM_B, 0.3);
    cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
    cairo_fill(cr);
}

void UIManager::drawWaveBackground(cairo_t* cr, int width, int height) {
    // Ocean gradient background
    cairo_pattern_t* grad = cairo_pattern_create_linear(0, 0, 0, height);
    cairo_pattern_add_color_stop_rgb(grad, 0, ColorScheme::OCEAN_LIGHT_R, ColorScheme::OCEAN_LIGHT_G, ColorScheme::OCEAN_LIGHT_B);
    cairo_pattern_add_color_stop_rgb(grad, 0.5, ColorScheme::OCEAN_DEEP_R, ColorScheme::OCEAN_DEEP_G, ColorScheme::OCEAN_DEEP_B);
    cairo_pattern_add_color_stop_rgb(grad, 1, ColorScheme::NAVY_R, ColorScheme::NAVY_G, ColorScheme::NAVY_B);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_set_source(cr, grad);
    cairo_fill(cr);
    cairo_pattern_destroy(grad);

    // Subtle wave lines
    cairo_set_line_width(cr, 1.0);
    for (int i = 0; i < 5; i++) {
        double y = (i * height / 5.0);
        cairo_set_source_rgba(cr, ColorScheme::OCEAN_FOAM_R, ColorScheme::OCEAN_FOAM_G, ColorScheme::OCEAN_FOAM_B, 0.1);

        cairo_move_to(cr, 0, y);
        for (int x = 0; x <= width; x += 10) {
            double wave_y = y + 5 * sin((x + wave_offset * 0.5) * 0.05);
            cairo_line_to(cr, x, wave_y);
        }
        cairo_stroke(cr);
    }
}

// Draw a complete ship as ONE beautiful shape across multiple cells
void UIManager::drawShip(cairo_t* cr, const Ship& ship) {
    if (!cr) return;

    // Check if ship is placed
    if (ship.position.row < 0 || ship.position.col < 0) return;

    // Calculate position and size
    int start_row = ship.position.row;
    int start_col = ship.position.col;
    int length = ship.length;

    double x, y, width, height;

    if (ship.orientation == HORIZONTAL) {
        // Horizontal ship - stretch across columns
        x = BOARD_MARGIN + start_col * CELL_SIZE + CELL_PADDING;
        y = BOARD_MARGIN + start_row * CELL_SIZE + CELL_PADDING;
        width = length * CELL_SIZE - CELL_PADDING * 2;
        height = CELL_SIZE - CELL_PADDING * 2;
    } else {
        // Vertical ship - stretch across rows
        x = BOARD_MARGIN + start_col * CELL_SIZE + CELL_PADDING;
        y = BOARD_MARGIN + start_row * CELL_SIZE + CELL_PADDING;
        width = CELL_SIZE - CELL_PADDING * 2;
        height = length * CELL_SIZE - CELL_PADDING * 2;
    }

    cairo_save(cr);

    // Choose color based on ship type
    double r, g, b;
    switch (ship.type) {
        case SHIP_CARRIER:      r = 0.3; g = 0.35; b = 0.4; break;  // Dark gray
        case SHIP_BATTLESHIP:   r = 0.4; g = 0.42; b = 0.45; break; // Medium gray
        case SHIP_CRUISER:      r = 0.35; g = 0.4; b = 0.42; break; // Blue-gray
        case SHIP_SUBMARINE:    r = 0.25; g = 0.3; b = 0.35; break; // Dark blue-gray
        case SHIP_DESTROYER:    r = 0.45; g = 0.47; b = 0.5; break;  // Light gray
        default:                r = 0.5; g = 0.5; b = 0.5; break;
    }

    // If sunk, darken significantly
    if (ship.is_sunk) {
        r *= 0.3; g *= 0.3; b *= 0.3;
    }

    // Draw ship hull with gradient
    cairo_pattern_t* pat = cairo_pattern_create_linear(x, y, x, y + height);
    cairo_pattern_add_color_stop_rgb(pat, 0, r * 1.2, g * 1.2, b * 1.2);
    cairo_pattern_add_color_stop_rgb(pat, 0.5, r, g, b);
    cairo_pattern_add_color_stop_rgb(pat, 1, r * 0.8, g * 0.8, b * 0.8);
    cairo_set_source(cr, pat);

    // Rounded rectangle for ship
    double radius = 8;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI/2, 0);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI/2);
    cairo_arc(cr, x + radius, y + height - radius, radius, M_PI/2, M_PI);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);

    // Ship border (outline)
    cairo_set_source_rgb(cr, r * 0.6, g * 0.6, b * 0.6);
    cairo_set_line_width(cr, 3.0);
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI/2, 0);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI/2);
    cairo_arc(cr, x + radius, y + height - radius, radius, M_PI/2, M_PI);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_stroke(cr);

    // Draw deck details (windows/hatches)
    cairo_set_source_rgba(cr, 0.2, 0.25, 0.3, 0.8);
    int num_segments = length;
    double segment_width = (ship.orientation == HORIZONTAL ? width : height) / num_segments;

    for (int i = 0; i < num_segments; i++) {
        double sx = (ship.orientation == HORIZONTAL) ? x + i * segment_width + 8 : x + 8;
        double sy = (ship.orientation == VERTICAL) ? y + i * segment_width + 8 : y + 8;
        double sw = (ship.orientation == HORIZONTAL) ? segment_width - 16 : width - 16;
        double sh = (ship.orientation == VERTICAL) ? segment_width - 16 : height - 16;

        if (sw > 4 && sh > 4) {
            cairo_rectangle(cr, sx, sy, sw, sh);
            cairo_fill(cr);
        }
    }

    // If sunk, add X marks and smoke
    if (ship.is_sunk) {
        cairo_set_source_rgba(cr, ColorScheme::HIT_R, ColorScheme::HIT_G, ColorScheme::HIT_B, 0.9);
        cairo_set_line_width(cr, 4.0);

        // Draw X across the ship
        cairo_move_to(cr, x + 5, y + 5);
        cairo_line_to(cr, x + width - 5, y + height - 5);
        cairo_move_to(cr, x + width - 5, y + 5);
        cairo_line_to(cr, x + 5, y + height - 5);
        cairo_stroke(cr);
    }

    cairo_restore(cr);
}
