#include "ui_manager.h"
#include "asset_manager.h"
#include <cmath>
#include <ctime>
#include <iostream>

#define CELL_SIZE 45
#define CELL_PADDING 2
#define BOARD_MARGIN 20
#define GLOW_RADIUS 15

UIManager::UIManager()
    : main_window(nullptr), current_screen(nullptr),
      player_board_area(nullptr), opponent_board_area(nullptr),
      chat_view(nullptr), status_label(nullptr), timer_label(nullptr), turn_indicator(nullptr),
      player_board_surface(nullptr), opponent_board_surface(nullptr),
      player_board(nullptr), opponent_board(nullptr), current_match(nullptr),
      animation_timer_id(0), glow_pulse(0.0), wave_offset(0.0),
      selected_ship_type(SHIP_CARRIER), current_orientation(HORIZONTAL),
      ready_battle_button(nullptr), hover_row(-1), hover_col(-1),
      is_player_turn(true), shots_fired(0), hits_count(0),
      shots_label_widget(nullptr), hits_label_widget(nullptr), accuracy_label_widget(nullptr),
      selected_target_row(-1), selected_target_col(-1), has_target_selected(false),
      fire_button(nullptr), turn_time_remaining(20), turn_timer_id(0), turn_timer_label(nullptr),
      animation_manager(nullptr), is_bot_mode(false) {
    // Initialize ship placement tracking
    for (int i = 0; i < NUM_SHIPS; i++) {
        ships_placed[i] = false;
        ship_status_labels[i] = nullptr;
        ship_buttons[i] = nullptr;
    }

    // Create animation manager
    animation_manager = new AnimationManager();
}

UIManager::~UIManager() {
    if (player_board_surface) {
        cairo_surface_destroy(player_board_surface);
    }
    if (opponent_board_surface) {
        cairo_surface_destroy(opponent_board_surface);
    }
    if (animation_timer_id > 0) {
        g_source_remove(animation_timer_id);
    }
    if (turn_timer_id > 0) {
        g_source_remove(turn_timer_id);
    }

    // Cleanup animation manager
    if (animation_manager) {
        delete animation_manager;
    }

    // Cleanup assets
    AssetManager::getInstance()->cleanup();
}

void UIManager::initialize(int argc, char* argv[]) {
    std::cout << "🔧 Initializing GTK..." << std::endl;

    // Check if display is available
    const char* display = getenv("DISPLAY");
    if (!display || display[0] == '\0') {
        std::cerr << "⚠️ WARNING: DISPLAY environment variable not set!" << std::endl;
        std::cerr << "   You may need to install and run an X server (like VcXsrv or Xming)" << std::endl;
    } else {
        std::cout << "📺 Display: " << display << std::endl;
    }

    gtk_init(&argc, &argv);
    std::cout << "✅ GTK initialized" << std::endl;

    // Load all assets
    std::cout << "🎨 Loading all assets..." << std::endl;
    AssetManager* assets = AssetManager::getInstance();
    if (!assets->loadAllAssets()) {
        std::cerr << "⚠️ Warning: Some assets failed to load" << std::endl;
    }

    // Create main window with decorations for resize/maximize
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "BATTLESHIP ONLINE");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 700);
    gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(main_window), TRUE);

    // Keep window decorations for resize/maximize/fullscreen
    gtk_window_set_decorated(GTK_WINDOW(main_window), TRUE);

    // Apply nautical maritime theme
    GtkCssProvider* css_provider = gtk_css_provider_new();
    const char* css_data =
        "window {"
        "   background: linear-gradient(180deg, #004F6B 0%, #002145 100%);"
        "}"
        "button {"
        "   background: linear-gradient(to bottom, #2E8AB0, #059FFD);"
        "   color: white;"
        "   border: 2px solid #87CEEB;"
        "   border-radius: 8px;"
        "   padding: 12px 24px;"
        "   font-weight: bold;"
        "   font-size: 14px;"
        "}"
        "button:hover {"
        "   background: linear-gradient(to bottom, #059FFD, #87CEEB);"
        "   border-color: #EEDEBA;"
        "}"
        "button:disabled {"
        "   background: linear-gradient(to bottom, #555555, #333333);"
        "   color: #888888;"
        "   border-color: #444444;"
        "   opacity: 0.6;"
        "}"
        "label {"
        "   color: #EEDEBA;"
        "   font-size: 14px;"
        "}"
        ".title {"
        "   color: #87CEEB;"
        "   font-size: 32px;"
        "   font-weight: bold;"
        "}"
        ".glow-text {"
        "   color: #059FFD;"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "}"
        "entry {"
        "   background-color: rgba(0, 33, 69, 0.6);"
        "   color: #EEDEBA;"
        "   border: 2px solid #2E8AB0;"
        "   border-radius: 6px;"
        "   padding: 10px;"
        "}"
        ".danger {"
        "   background: linear-gradient(to bottom, #D93D1C, #A52A0C);"
        "   border-color: #FF7F50;"
        "}"
        ".secondary {"
        "   background: linear-gradient(to bottom, #737A85, #40454D);"
        "   border-color: #ABD6E6;"
        "}";

    gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    // Connect destroy signal
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Start animation timer - slower to reduce CPU usage
    std::cout << "⏱️ Starting animation timer..." << std::endl;
    // IMPORTANT: Don't start animation timer yet - it will crash if boards aren't ready
    // animation_timer_id = g_timeout_add(100, animationCallback, this);

    // Show main menu by default
    std::cout << "🎮 Creating main menu screen..." << std::endl;
    showScreen(SCREEN_MAIN_MENU);

    std::cout << "📺 Showing window..." << std::endl;
    gtk_widget_show_all(main_window);
    std::cout << "✅ Window shown successfully!" << std::endl;

    // Ensure window gets focus and cursor is visible
    gtk_window_present(GTK_WINDOW(main_window));

    // Make sure cursor is visible on the window
    GdkWindow* gdk_window = gtk_widget_get_window(main_window);
    if (gdk_window) {
        gdk_window_set_cursor(gdk_window, NULL);  // NULL = default cursor (visible)
    }

    // Start animation timer AFTER window is fully shown
    g_idle_add(+[](gpointer data) -> gboolean {
        UIManager* ui = static_cast<UIManager*>(data);
        if (ui->animation_timer_id == 0) {
            std::cout << "⏱️ Starting animation timer (delayed)..." << std::endl;
            ui->animation_timer_id = g_timeout_add(100, animationCallback, ui);
        }
        // Ensure cursor visibility again after window is realized
        GdkWindow* gdk_win = gtk_widget_get_window(ui->main_window);
        if (gdk_win) {
            gdk_window_set_cursor(gdk_win, NULL);
        }
        return FALSE;  // Don't repeat
    }, this);
}

void UIManager::showScreen(UIScreen screen) {
    // Remove current screen
    if (current_screen) {
        gtk_widget_destroy(current_screen);
        current_screen = nullptr;
    }

    // Create new screen
    switch (screen) {
        case SCREEN_MAIN_MENU:
            current_screen = createMainMenuScreen();
            break;
        case SCREEN_LOGIN:
            current_screen = createLoginScreen();
            break;
        case SCREEN_REGISTER:
            current_screen = createRegisterScreen();
            break;
        case SCREEN_LOBBY:
            current_screen = createLobbyScreen();
            break;
        case SCREEN_SHIP_PLACEMENT:
            current_screen = createShipPlacementScreen();
            break;
        case SCREEN_GAME:
            current_screen = createGameScreen();
            break;
        case SCREEN_REPLAY:
            current_screen = createReplayScreen();
            break;
        case SCREEN_PROFILE:
            current_screen = createProfileScreen();
            break;
    }

    if (current_screen) {
        gtk_container_add(GTK_CONTAINER(main_window), current_screen);
        gtk_widget_show_all(main_window);

        // Ensure cursor is visible when switching screens
        GdkWindow* gdk_window = gtk_widget_get_window(main_window);
        if (gdk_window) {
            gdk_window_set_cursor(gdk_window, NULL);
        }
    }
}

gboolean UIManager::animationCallback(gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);
    ui->updateAnimations();
    return TRUE;
}

void UIManager::updateAnimations() {
    // Slow wave animation
    glow_pulse += 0.05;
    wave_offset += 1.0;

    if (glow_pulse > 2 * M_PI) glow_pulse = 0.0;
    if (wave_offset > 1000.0) wave_offset = 0.0;

    // Update animation manager for explosions/splashes
    if (animation_manager) {
        animation_manager->update();
    }

    // Redraw boards for wave animation (only if they exist AND are realized)
    if (player_board_area && GTK_IS_WIDGET(player_board_area) && gtk_widget_get_realized(player_board_area)) {
        gtk_widget_queue_draw(player_board_area);
    }
    if (opponent_board_area && GTK_IS_WIDGET(opponent_board_area) && gtk_widget_get_realized(opponent_board_area)) {
        gtk_widget_queue_draw(opponent_board_area);
    }
}

void UIManager::run() {
    gtk_main();
}

void UIManager::quit() {
    gtk_main_quit();
}

void UIManager::makeDraggable(GtkWidget* widget, GtkWidget* /*window*/) {
    // In Linux, Alt+Drag works by default to move windows
    // We just add visual hint that header is the title bar
    gtk_widget_set_tooltip_text(widget, "Use Alt+Drag to move window");
}

void UIManager::showErrorDialog(const std::string& message) {
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", message.c_str()
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void UIManager::showNotification(const std::string& message) {
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(main_window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", message.c_str()
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void UIManager::addChatMessage(const std::string& sender, const std::string& message, bool /*is_self*/) {
    if (!chat_view) return;

    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);

    std::string full_message = sender + ": " + message + "\n";
    gtk_text_buffer_insert(buffer, &end, full_message.c_str(), -1);

    // Auto-scroll to bottom
    GtkTextMark* mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(chat_view), mark, 0.0, TRUE, 0.0, 1.0);
}

void UIManager::updateTurnIndicator(bool is_player_turn) {
    if (turn_indicator) {
        gtk_label_set_text(GTK_LABEL(turn_indicator),
                          is_player_turn ? "🎯 YOUR TURN" : "⏳ OPPONENT'S TURN");
    }
}

void UIManager::updateTimer(int seconds_remaining) {
    if (timer_label) {
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "⏱️ %02d:%02d",
                seconds_remaining / 60, seconds_remaining % 60);
        gtk_label_set_text(GTK_LABEL(timer_label), time_str);
    }
}

// Game flow helper methods
void UIManager::switchTurn() {
    is_player_turn = !is_player_turn;

    if (turn_indicator) {
        if (is_player_turn) {
            gtk_label_set_text(GTK_LABEL(turn_indicator), "» YOUR TURN «");
        } else {
            gtk_label_set_text(GTK_LABEL(turn_indicator), "« OPPONENT'S TURN »");
        }
    }

    std::cout << (is_player_turn ? "» YOUR TURN" : "« OPPONENT'S TURN") << std::endl;
}

void UIManager::updateGameStats() {
    if (shots_label_widget) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Shots\n%d", shots_fired);
        gtk_label_set_text(GTK_LABEL(shots_label_widget), buf);
    }

    if (hits_label_widget) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Hits\n%d", hits_count);
        gtk_label_set_text(GTK_LABEL(hits_label_widget), buf);
    }

    if (accuracy_label_widget && shots_fired > 0) {
        char buf[32];
        int accuracy = (hits_count * 100) / shots_fired;
        snprintf(buf, sizeof(buf), "Accuracy\n%d%%", accuracy);
        gtk_label_set_text(GTK_LABEL(accuracy_label_widget), buf);
    }
}

void UIManager::markSunkShip(int /*row*/, int /*col*/, ShipType ship_type) {
    if (!opponent_board) return;

    // Find the ship that was sunk
    const Ship* ships = opponent_board->getShips();
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (ships[i].type == ship_type && ships[i].is_sunk) {
            // Mark all cells of the sunk ship as SUNK for visual distinction
            int length = ships[i].length;
            for (int j = 0; j < length; j++) {
                int r = ships[i].position.row + (ships[i].orientation == VERTICAL ? j : 0);
                int c = ships[i].position.col + (ships[i].orientation == HORIZONTAL ? j : 0);

                // Change from HIT to SUNK
                opponent_board->setCell(r, c, CELL_SUNK);
            }
            break;
        }
    }

    std::cout << "🔥 SUNK " << shipTypeToName(ship_type) << "!" << std::endl;
}

// Bot AI Implementation
void UIManager::botTakeTurn() {
    if (!player_board || is_player_turn || !is_bot_mode) return;

    // Random shooting strategy
    int row, col;
    int attempts = 0;
    bool valid_shot = false;

    while (!valid_shot && attempts < 100) {
        row = rand() % BOARD_SIZE;
        col = rand() % BOARD_SIZE;

        CellState cell = player_board->getCell(row, col);
        if (cell == CELL_EMPTY || cell == CELL_SHIP) {
            valid_shot = true;
        }
        attempts++;
    }

    if (!valid_shot) {
        // No valid shots left, bot loses
        std::cout << "Bot has no valid moves!" << std::endl;
        switchTurn();
        return;
    }

    // Process bot's shot
    Coordinate target;
    target.row = row;
    target.col = col;
    ShotResult result = player_board->processShot(target);

    // Log result
    if (result == SHOT_HIT) {
        std::cout << "🤖 Bot HIT at " << (char)('A' + row) << (col + 1) << "!" << std::endl;
        // Bot continues turn on hit
        g_timeout_add(1000, botTurnCallback, this);  // Shoot again after 1s
    } else if (result == SHOT_SUNK) {
        std::cout << "🤖 Bot SUNK your ship at " << (char)('A' + row) << (col + 1) << "!" << std::endl;
        // Bot continues turn on sunk
        g_timeout_add(1000, botTurnCallback, this);  // Shoot again after 1s
    } else {
        std::cout << "🤖 Bot MISS at " << (char)('A' + row) << (col + 1) << std::endl;
        switchTurn();  // Switch to player turn
    }

    // Redraw player board
    if (player_board_area) {
        gtk_widget_queue_draw(player_board_area);
    }

    // Check if player lost
    if (player_board->allShipsSunk()) {
        showNotification("💀 DEFEAT! All your ships destroyed!");
    }
}

gboolean UIManager::botTurnCallback(gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);
    ui->botTakeTurn();
    return FALSE;  // Don't repeat
}

// GTK Callbacks implementation
gboolean on_board_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);

    if (widget == ui->player_board_area) {
        ui->renderPlayerBoard(cr);
    } else if (widget == ui->opponent_board_area) {
        ui->renderOpponentBoard(cr);
    }

    return FALSE;
}

gboolean on_board_button_press(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);

    int col = (event->x - BOARD_MARGIN) / CELL_SIZE;
    int row = (event->y - BOARD_MARGIN) / CELL_SIZE;

    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
        if (event->button == 1) { // Left click
            // Ship placement mode: click on player board to place ships
            if (widget == ui->player_board_area && ui->ready_battle_button != nullptr) {
                ui->placeShipAt(row, col);
            }
            // Game mode: click on opponent board to SELECT target (not auto-fire)
            else if (widget == ui->opponent_board_area && ui->opponent_board) {
                // Check if it's player's turn
                if (!ui->is_player_turn) {
                    std::cout << "⏸ Not your turn! Wait for opponent..." << std::endl;
                    return TRUE;
                }

                // Check if cell can be targeted
                CellState cell = ui->opponent_board->getCell(row, col);
                if (cell == CELL_EMPTY || cell == CELL_SHIP) {
                    // Select this cell as target
                    ui->selectTarget(row, col);
                    gtk_widget_queue_draw(widget);
                } else {
                    std::cout << "Already shot at " << (char)('A' + row) << (col + 1) << std::endl;
                }
            }
            // Fallback: use callback
            else if (ui->on_cell_clicked) {
                ui->on_cell_clicked(row, col);
            }
        }
        // Right-click removed - use delete buttons instead
    }

    return TRUE;
}

gboolean on_board_motion_notify(GtkWidget* widget, GdkEventMotion* event, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);

    // Update hover position for ship placement preview
    if (widget == ui->player_board_area && ui->ready_battle_button) {
        int col = (event->x - BOARD_MARGIN) / CELL_SIZE;
        int row = (event->y - BOARD_MARGIN) / CELL_SIZE;

        if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
            if (ui->hover_row != row || ui->hover_col != col) {
                ui->hover_row = row;
                ui->hover_col = col;
                gtk_widget_queue_draw(widget);
            }
        } else {
            if (ui->hover_row != -1 || ui->hover_col != -1) {
                ui->hover_row = -1;
                ui->hover_col = -1;
                gtk_widget_queue_draw(widget);
            }
        }
    }

    return TRUE;
}

void on_fire_clicked(GtkButton* /*button*/, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);
    if (ui && ui->executeFireAtTarget()) {
        // Shot was fired successfully
        if (ui->opponent_board_area) {
            gtk_widget_queue_draw(ui->opponent_board_area);
        }
        if (ui->player_board_area) {
            gtk_widget_queue_draw(ui->player_board_area);
        }
    }
}

void on_resign_clicked(GtkButton* /*button*/, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);
    if (ui->on_resign) {
        ui->on_resign();
    }
}

void on_draw_offer_clicked(GtkButton* /*button*/, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);
    if (ui->on_draw_offer) {
        ui->on_draw_offer();
    }
}

void on_pause_clicked(GtkButton* /*button*/, gpointer /*data*/) {
    std::cout << "⏸ Pause requested" << std::endl;
}

// Target selection implementation
void UIManager::selectTarget(int row, int col) {
    selected_target_row = row;
    selected_target_col = col;
    has_target_selected = true;

    // Enable fire button
    if (fire_button) {
        gtk_widget_set_sensitive(fire_button, TRUE);
    }

    std::cout << "🎯 Target selected: " << (char)('A' + row) << (col + 1) << std::endl;
}

void UIManager::clearTargetSelection() {
    selected_target_row = -1;
    selected_target_col = -1;
    has_target_selected = false;

    // Disable fire button
    if (fire_button) {
        gtk_widget_set_sensitive(fire_button, FALSE);
    }
}

bool UIManager::executeFireAtTarget() {
    if (!has_target_selected || !opponent_board) {
        std::cout << "⚠️ No target selected!" << std::endl;
        return false;
    }

    if (!is_player_turn) {
        std::cout << "⏸ Not your turn!" << std::endl;
        return false;
    }

    int row = selected_target_row;
    int col = selected_target_col;

    // Process the shot
    Coordinate target;
    target.row = row;
    target.col = col;
    ShotResult result = opponent_board->processShot(target);

    // Update stats
    shots_fired++;

    // Add animation based on result
    if (result == SHOT_HIT) {
        hits_count++;
        if (animation_manager) {
            animation_manager->addExplosion(row, col);
        }
        std::cout << "💥 HIT at " << (char)('A' + row) << (col + 1) << "!" << std::endl;
    } else if (result == SHOT_SUNK) {
        hits_count++;
        // No explosion animation for sunk ships
        std::cout << "🔥 SUNK at " << (char)('A' + row) << (col + 1) << "!" << std::endl;
    } else {
        if (animation_manager) {
            animation_manager->addSplash(row, col);
        }
        std::cout << "💦 MISS at " << (char)('A' + row) << (col + 1) << std::endl;
    }

    // Handle sunk ship
    if (result == SHOT_SUNK) {
        // Find which ship was sunk
        const Ship* ships = opponent_board->getShips();
        for (int i = 0; i < NUM_SHIPS; i++) {
            if (ships[i].is_sunk) {
                // Check if this is the newly sunk ship
                for (int j = 0; j < ships[i].length; j++) {
                    int sr = ships[i].position.row + (ships[i].orientation == VERTICAL ? j : 0);
                    int sc = ships[i].position.col + (ships[i].orientation == HORIZONTAL ? j : 0);
                    if (sr == row && sc == col) {
                        markSunkShip(row, col, (ShipType)ships[i].type);
                        // Removed ship sink animation
                        break;
                    }
                }
            }
        }
    }

    // Update stats display
    updateGameStats();

    // Clear target selection
    clearTargetSelection();

    // Check if all ships sunk
    if (opponent_board->allShipsSunk()) {
        stopTurnTimer();
        showNotification("🎉 VICTORY! All enemy ships destroyed!");
        return true;
    }

    // Switch turn on miss
    if (result == SHOT_MISS) {
        switchTurn();
        stopTurnTimer();

        // If in bot mode and now bot's turn, trigger bot AI after 1 second
        if (is_bot_mode && !is_player_turn) {
            g_timeout_add(1000, UIManager::botTurnCallback, this);
        } else if (!is_bot_mode) {
            // Start timer for opponent's turn
            startTurnTimer(20);
        }
    } else {
        // Hit or sunk - player continues, restart timer
        stopTurnTimer();
        startTurnTimer(20);
    }

    return true;
}

// Turn timer implementation
void UIManager::startTurnTimer(int seconds) {
    // Stop existing timer
    stopTurnTimer();

    turn_time_remaining = seconds;

    // Update label immediately
    if (turn_timer_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "⏱️ %d s", turn_time_remaining);
        gtk_label_set_text(GTK_LABEL(turn_timer_label), buf);
    }

    // Start 1-second timer
    turn_timer_id = g_timeout_add(1000, turnTimerCallback, this);
}

void UIManager::stopTurnTimer() {
    if (turn_timer_id > 0) {
        g_source_remove(turn_timer_id);
        turn_timer_id = 0;
    }

    if (turn_timer_label) {
        gtk_label_set_text(GTK_LABEL(turn_timer_label), "⏱️ --");
    }
}

gboolean UIManager::turnTimerCallback(gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);

    ui->turn_time_remaining--;

    // Update label
    if (ui->turn_timer_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "⏱️ %d s", ui->turn_time_remaining);
        gtk_label_set_text(GTK_LABEL(ui->turn_timer_label), buf);
    }

    // Check if time expired
    if (ui->turn_time_remaining <= 0) {
        ui->onTurnTimerExpired();
        return FALSE;  // Stop timer
    }

    return TRUE;  // Continue timer
}

void UIManager::onTurnTimerExpired() {
    std::cout << "⏰ Time expired! Turn skipped." << std::endl;

    // Clear any selected target
    clearTargetSelection();

    // Switch turn
    switchTurn();

    // If in bot mode and now bot's turn, trigger bot AI
    if (is_bot_mode && !is_player_turn) {
        g_timeout_add(1000, UIManager::botTurnCallback, this);
    } else if (!is_bot_mode) {
        // Start timer for opponent's turn
        startTurnTimer(20);
    } else {
        // Player's turn again, start timer
        startTurnTimer(20);
    }
}
