#include "ui_manager.h"
#include "asset_manager.h"
#include "session_storage.h"
#include "config.h"
#include <cmath>
#include <ctime>
#include <iostream>

// Use AUTO_LOGIN_ENABLED from config.h
// Set to 1 to enable, 0 to disable (for demo on same machine)
#ifndef AUTO_LOGIN_ENABLED
#define AUTO_LOGIN_ENABLED 0
#endif

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
      animation_manager(nullptr), is_bot_mode(false),
      current_match_id(0), waiting_for_match_ready(false),
      pending_challenge_id_(0),
      network(nullptr),
      login_username_entry(nullptr), login_password_entry(nullptr),
      register_username_entry(nullptr), register_password_entry(nullptr),
      register_display_name_entry(nullptr), register_confirm_entry(nullptr) {
    // Initialize ship placement tracking
    for (int i = 0; i < NUM_SHIPS; i++) {
        ships_placed[i] = false;
        ship_status_labels[i] = nullptr;
        ship_buttons[i] = nullptr;
    }

    // Create animation manager
    animation_manager = new AnimationManager();

    // Create network manager
    network = new ClientNetwork();
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

    // Cleanup network
    if (network) {
        delete network;
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
        std::cout << " Display: " << display << std::endl;
    }

    gtk_init(&argc, &argv);
    std::cout << "OK: GTK initialized" << std::endl;

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

    // 🎮 RETRO PIXEL THEME - HIGH CONTRAST CGA-Style
    GtkCssProvider* css_provider = gtk_css_provider_new();
    const char* css_data =
        "window {"
        "   background: #000000;"  /* Pure black for max contrast */
        "}"
        "button {"
        "   background: #1a1a1a;"  /* Dark gray */
        "   color: #FFFFFF;"       /* BRIGHT WHITE - high contrast */
        "   border: 3px solid #00FFFF;"  /* Bright cyan border */
        "   border-radius: 0px;"   /* Square corners - pixel style */
        "   padding: 10px 20px;"
        "   font-family: 'DejaVu Sans Mono', monospace;"
        "   font-weight: bold;"
        "   font-size: 14px;"
        "   box-shadow: 4px 4px 0px #00FFFF;"  /* Cyan pixel shadow */
        "}"
        "button:hover {"
        "   background: #333333;"
        "   color: #FFFF00;"       /* BRIGHT YELLOW on hover */
        "   border-color: #FFFF00;"
        "   box-shadow: 5px 5px 0px #FFFF00;"
        "}"
        "button:disabled {"
        "   background: #0a0a0a;"
        "   color: #555555;"       /* Darker gray for disabled */
        "   border-color: #333333;"
        "   box-shadow: 2px 2px 0px #333333;"
        "}"
        "label {"
        "   color: #FFFFFF;"       /* BRIGHT WHITE - max contrast */
        "   font-family: 'DejaVu Sans Mono', monospace;"
        "   font-size: 14px;"
        "}"
        ".title {"
        "   color: #FFFF00;"       /* BRIGHT YELLOW - retro title */
        "   font-family: 'DejaVu Sans Mono', monospace;"
        "   font-size: 36px;"
        "   font-weight: bold;"
        "   text-shadow: 4px 4px 0px #FF00FF;"  /* Magenta shadow */
        "}"
        ".glow-text {"
        "   color: #00FFFF;"       /* BRIGHT CYAN - accent */
        "   font-family: 'DejaVu Sans Mono', monospace;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "}"
        "entry {"
        "   background: #0a0a0a;"
        "   color: #FFFFFF;"       /* BRIGHT WHITE text */
        "   border: 3px solid #00FFFF;"
        "   border-radius: 0px;"
        "   padding: 10px;"
        "   font-family: 'DejaVu Sans Mono', monospace;"
        "   font-size: 14px;"
        "}"
        "entry:focus {"
        "   border-color: #FFFF00;"  /* Yellow on focus */
        "}"
        ".danger {"
        "   background: #660000;"  /* Dark red bg */
        "   color: #FFFFFF;"       /* White text */
        "   border-color: #FF0000;" /* BRIGHT RED border */
        "   box-shadow: 4px 4px 0px #FF0000;"
        "}"
        ".secondary {"
        "   background: #1a1a1a;"
        "   color: #00FFFF;"       /* Cyan text */
        "   border-color: #00FFFF;"
        "   box-shadow: 4px 4px 0px #00FFFF;"
        "}";

    gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    // Connect window close event handlers
    g_signal_connect(main_window, "delete-event", G_CALLBACK(onWindowDeleteEvent), this);
    g_signal_connect(main_window, "destroy", G_CALLBACK(onWindowDestroy), this);

    // Start animation timer - slower to reduce CPU usage
    std::cout << "TIME: Starting animation timer..." << std::endl;
    // IMPORTANT: Don't start animation timer yet - it will crash if boards aren't ready
    // animation_timer_id = g_timeout_add(100, animationCallback, this);

#if AUTO_LOGIN_ENABLED
    // Try auto-login if session exists
    if (SessionStorage::hasStoredSession()) {
        uint32_t user_id;
        std::string session_token, username, display_name;
        int32_t elo_rating;

        if (SessionStorage::loadSession(user_id, session_token, username, display_name, elo_rating)) {
            std::cout << "📋 Found stored session for: " << display_name << std::endl;
            std::cout << "🔐 Validating session with server..." << std::endl;

            // Connect to server first (use localhost for auto-login)
            const char* host = SERVER_HOST;
            int port = SERVER_PORT;
            network->connect(host, port, [this, session_token](bool connected, const std::string& error) {
                if (!connected) {
                    std::cerr << "❌ Failed to connect: " << error << std::endl;
                    SessionStorage::clearSession();
                    g_idle_add([](gpointer data) -> gboolean {
                        UIManager* ui = static_cast<UIManager*>(data);
                        ui->showScreen(SCREEN_LOGIN);
                        return G_SOURCE_REMOVE;
                    }, this);
                    return;
                }

                // Validate session token with server
                network->validateSession(session_token,
                    [this](bool valid, uint32_t user_id, const std::string& username,
                           const std::string& display_name, int32_t elo_rating, const std::string& error) {

                    g_idle_add([](gpointer data) -> gboolean {
                        auto* params = static_cast<std::tuple<UIManager*, bool, uint32_t, std::string, std::string, int32_t, std::string>*>(data);
                        UIManager* ui = std::get<0>(*params);
                        bool valid = std::get<1>(*params);

                        if (valid) {
                            std::cout << "✅ Session valid! Auto-login successful" << std::endl;

                            // Restore player info
                            ui->current_player.user_id = std::get<2>(*params);
                            ui->current_player.username = std::get<3>(*params);
                            ui->current_player.display_name = std::get<4>(*params);
                            ui->current_player.elo_rating = std::get<5>(*params);
                            ui->current_player.status = STATUS_ONLINE;

                            // Show main menu
                            ui->showScreen(SCREEN_MAIN_MENU);
                        } else {
                            std::cerr << "❌ Session expired: " << std::get<6>(*params) << std::endl;

                            // Clear expired session
                            SessionStorage::clearSession();

                            // Show login screen
                            ui->showScreen(SCREEN_LOGIN);
                        }

                        delete params;
                        return G_SOURCE_REMOVE;
                    }, new std::tuple<UIManager*, bool, uint32_t, std::string, std::string, int32_t, std::string>(
                        this, valid, user_id, username, display_name, elo_rating, error));
                });
            });

            // Show login screen initially (will be updated after validation)
            showScreen(SCREEN_LOGIN);
            return;
        } else {
            std::cout << "⚠️ Failed to load session, showing login screen" << std::endl;
            showScreen(SCREEN_LOGIN);
        }
    } else {
        // Show login screen by default (user must authenticate first)
        std::cout << "📋 Creating login screen..." << std::endl;
        showScreen(SCREEN_LOGIN);
    }
#else
    // Auto-login disabled - always show login screen
    std::cout << "📋 Auto-login disabled, showing login screen..." << std::endl;
    showScreen(SCREEN_LOGIN);
#endif

    std::cout << " Showing window..." << std::endl;
    gtk_widget_show_all(main_window);
    std::cout << "OK: Window shown successfully!" << std::endl;

    // Ensure window gets focus and cursor is visible
    gtk_window_present(GTK_WINDOW(main_window));

    // Make sure cursor is visible on the window
    GdkWindow* gdk_window = gtk_widget_get_window(main_window);
    if (gdk_window) {
        gdk_window_set_cursor(gdk_window, NULL);  // NULL = default cursor (visible)
    }

    // Start animation timer AFTER window is fully shown
    // OPTIMIZED: 200ms interval (5 FPS) instead of 100ms to reduce CPU usage
    g_idle_add(+[](gpointer data) -> gboolean {
        UIManager* ui = static_cast<UIManager*>(data);
        if (ui->animation_timer_id == 0) {
            std::cout << "TIME: Starting animation timer (200ms interval)..." << std::endl;
            ui->animation_timer_id = g_timeout_add(200, animationCallback, ui);
        }
        // Ensure cursor visibility again after window is realized
        GdkWindow* gdk_win = gtk_widget_get_window(ui->main_window);
        if (gdk_win) {
            gdk_window_set_cursor(gdk_win, NULL);
        }
        return FALSE;  // Don't repeat
    }, this);

    // Connect to server
    std::cout << "🌐 Connecting to game server..." << std::endl;
    connectToServer(SERVER_HOST, SERVER_PORT);
}

void UIManager::showScreen(UIScreen screen) {
    // Stop timers before destroying screen
    stopTurnTimer();

    // Remove current screen
    if (current_screen) {
        gtk_widget_destroy(current_screen);
        current_screen = nullptr;

        // Clear all widget pointers that belonged to the destroyed screen
        turn_timer_label = nullptr;
        fire_button = nullptr;
        status_label = nullptr;
        player_board_area = nullptr;
        opponent_board_area = nullptr;
        ready_battle_button = nullptr;
    }

    // Create new screen
    switch (screen) {
        case SCREEN_MAIN_MENU:
            current_screen = createMainMenuScreen();
            // AudioManager::getInstance().playBackgroundMusic();
            break;
        case SCREEN_LOGIN:
            current_screen = createLoginScreen();
            // AudioManager::getInstance().stopMusic();
            break;
        case SCREEN_REGISTER:
            current_screen = createRegisterScreen();
            // AudioManager::getInstance().stopMusic();
            break;
        case SCREEN_LOBBY:
            current_screen = createLobbyScreen();
            // std::cout << "[UI] 🎵 Starting funky lobby music..." << std::endl;
            // AudioManager::getInstance().playBackgroundMusic();
            break;
        case SCREEN_SHIP_PLACEMENT:
            current_screen = createShipPlacementScreen();
            // AudioManager::getInstance().stopMusic();
            break;
        case SCREEN_GAME:
            current_screen = createGameScreen();
            // Start turn timer when game begins
            is_player_turn = true;
            startTurnTimer(20); // 20 seconds per turn
            // std::cout << "[UI] 🎮 Starting intense battle music..." << std::endl;
            // AudioManager::getInstance().playGameMusic();
            break;
        case SCREEN_REPLAY:
            current_screen = createReplayScreen();
            // AudioManager::getInstance().stopMusic();
            break;
        case SCREEN_PROFILE:
            current_screen = createProfileScreen();
            // AudioManager::getInstance().stopMusic();
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

    // Redraw boards for wave animation (SAFE: check widget validity before queuing)
    // Only redraw if widget exists, is valid GTK widget, is realized, and has a window
    if (player_board_area && GTK_IS_WIDGET(player_board_area) &&
        gtk_widget_get_realized(player_board_area) &&
        gtk_widget_get_window(player_board_area)) {
        gtk_widget_queue_draw(player_board_area);
    }
    if (opponent_board_area && GTK_IS_WIDGET(opponent_board_area) &&
        gtk_widget_get_realized(opponent_board_area) &&
        gtk_widget_get_window(opponent_board_area)) {
        gtk_widget_queue_draw(opponent_board_area);
    }
}

void UIManager::run() {
    gtk_main();
}

void UIManager::quit() {
    gtk_main_quit();
}

void UIManager::resignMatch() {
    std::cout << "[UI] 🏳️ Player is resigning match..." << std::endl;

    // Stop music and play defeat sound
    // AudioManager::getInstance().stopMusic();
    // AudioManager::getInstance().playDefeatMusic();

    // Send RESIGN message to server if in multiplayer mode
    if (network && network->isConnected() && current_match_id > 0) {
        std::cout << "[UI] Sending RESIGN message to server (match_id=" << current_match_id << ")" << std::endl;
        network->sendResign(current_match_id);

        // Show surrender notification
        showNotification(
            "╔═══════════════════════════════╗\n"
            "║  🏳️  YOU SURRENDERED  🏳️       ║\n"
            "╠═══════════════════════════════╣\n"
            "║                               ║\n"
            "║   You have forfeited the      ║\n"
            "║   match and accepted          ║\n"
            "║   defeat.                     ║\n"
            "║                               ║\n"
            "║   Returning to lobby...       ║\n"
            "║                               ║\n"
            "╚═══════════════════════════════╝"
        );

        // Return to lobby after notification
        showScreen(SCREEN_LOBBY);
    } else {
        std::cout << "[UI] Not in multiplayer match, cannot resign" << std::endl;
    }
}

bool UIManager::handleApplicationExit() {
    // Check if in active match
    bool in_match = (current_match_id > 0 &&
                     network &&
                     network->isConnected());

    if (in_match) {
        // Show confirmation dialog
        GtkWidget* dialog = gtk_message_dialog_new(
            GTK_WINDOW(main_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_YES_NO,
            "You are in an active match!\n\n"
            "Exiting now will forfeit the game and your opponent will win.\n\n"
            "Are you sure you want to exit?"
        );

        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_YES) {
            // User confirmed exit - resign match
            std::cout << "[UI] Player confirmed exit during match - resigning" << std::endl;

            if (network && network->isConnected()) {
                network->sendResign(current_match_id);
                // Small delay to ensure message is sent
                usleep(100000);  // 100ms
            }

            gtk_main_quit();
            return false;  // Exit proceeding
        } else {
            // User cancelled exit
            std::cout << "[UI] Player cancelled exit" << std::endl;
            return true;  // Exit cancelled
        }
    } else {
        // Not in match - just quit
        gtk_main_quit();
        return false;
    }
}

gboolean UIManager::onWindowDeleteEvent(GtkWidget* /*widget*/, GdkEvent* /*event*/, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);
    return ui->handleApplicationExit() ? TRUE : FALSE;  // TRUE = cancel close
}

void UIManager::onWindowDestroy(GtkWidget* /*widget*/, gpointer /*data*/) {
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
    // Play notification sound
    // AudioManager::getInstance().playBeep();

    // Check if this is a victory/defeat message - Enhanced Pixel Art
    if (message.find("VICTORY") != std::string::npos || message.find("DEFEAT") != std::string::npos) {
        bool is_victory = message.find("VICTORY") != std::string::npos;

        // Play victory or defeat music
        if (is_victory) {
            std::cout << "[UI] 🎉 Victory!" << std::endl;
            // AudioManager::getInstance().playVictoryMusic();
        } else {
            std::cout << "[UI] 💔 Defeat..." << std::endl;
            // AudioManager::getInstance().playDefeatMusic();
        }

        // Create beautiful pixel art dialog
        GtkWidget* dialog = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(dialog), is_victory ? "VICTORY" : "DEFEAT");
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

        // Pure black background
        GdkRGBA bg = {0.0, 0.0, 0.0, 1.0};
        gtk_widget_override_background_color(dialog, GTK_STATE_FLAG_NORMAL, &bg);

        GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_widget_override_background_color(content, GTK_STATE_FLAG_NORMAL, &bg);
        gtk_widget_set_margin_start(content, 40);
        gtk_widget_set_margin_end(content, 40);
        gtk_widget_set_margin_top(content, 40);
        gtk_widget_set_margin_bottom(content, 30);

        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 25);

        // Victory or Defeat ASCII Art
        const char* ascii_art;
        if (is_victory) {
            ascii_art =
                "╔═══════════════════════════════════════╗\n"
                "║                                       ║\n"
                "║   ⭐  V I C T O R Y ! ! !  ⭐          ║\n"
                "║                                       ║\n"
                "║         🏆  CONGRATULATIONS  🏆       ║\n"
                "║                                       ║\n"
                "║   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄      ║\n"
                "║   ███████╗ ██╗    ██╗██╗███╗   ██╗   ║\n"
                "║   ██╔════╝ ██║    ██║██║████╗  ██║   ║\n"
                "║   █████╗   ██║ █╗ ██║██║██╔██╗ ██║   ║\n"
                "║   ██╔══╝   ██║███╗██║██║██║╚██╗██║   ║\n"
                "║   ███████╗ ╚███╔███╔╝██║██║ ╚████║   ║\n"
                "║   ╚══════╝  ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝   ║\n"
                "║   ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀      ║\n"
                "║                                       ║\n"
                "║    All enemy ships destroyed!         ║\n"
                "║                                       ║\n"
                "╚═══════════════════════════════════════╝";
        } else {
            ascii_art =
                "╔═══════════════════════════════════════╗\n"
                "║                                       ║\n"
                "║   ⚠️   D E F E A T . . .   ⚠️          ║\n"
                "║                                       ║\n"
                "║         💔  GAME  OVER  💔            ║\n"
                "║                                       ║\n"
                "║   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄      ║\n"
                "║   ██╗      ██████╗ ███████╗███████╗  ║\n"
                "║   ██║     ██╔═══██╗██╔════╝██╔════╝  ║\n"
                "║   ██║     ██║   ██║███████╗█████╗    ║\n"
                "║   ██║     ██║   ██║╚════██║██╔══╝    ║\n"
                "║   ███████╗╚██████╔╝███████║███████╗  ║\n"
                "║   ╚══════╝ ╚═════╝ ╚══════╝╚══════╝  ║\n"
                "║   ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀      ║\n"
                "║                                       ║\n"
                "║    All your ships destroyed!          ║\n"
                "║                                       ║\n"
                "╚═══════════════════════════════════════╝";
        }

        GtkWidget* label = gtk_label_new(ascii_art);
        GtkStyleContext* ctx = gtk_widget_get_style_context(label);
        gtk_style_context_add_class(ctx, is_victory ? "title" : "glow-text");

        gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(content), vbox);

        // Action buttons
        GtkWidget* action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
        gtk_widget_override_background_color(action_area, GTK_STATE_FLAG_NORMAL, &bg);
        gtk_widget_set_margin_bottom(action_area, 20);

        GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
        gtk_widget_set_halign(btn_box, GTK_ALIGN_CENTER);

        GtkWidget* menu_btn = gtk_button_new_with_label("[ LOBBY ]");
        gtk_widget_set_size_request(menu_btn, 150, 50);
        GtkStyleContext* menu_ctx = gtk_widget_get_style_context(menu_btn);
        gtk_style_context_add_class(menu_ctx, "secondary");

        GtkWidget* rematch_btn = gtk_button_new_with_label("[ REMATCH ]");
        gtk_widget_set_size_request(rematch_btn, 150, 50);

        gtk_box_pack_start(GTK_BOX(btn_box), menu_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_box), rematch_btn, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(action_area), btn_box);

        g_signal_connect(menu_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
            gtk_dialog_response(GTK_DIALOG(d), GTK_RESPONSE_CANCEL);
        }), dialog);

        g_signal_connect(rematch_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
            gtk_dialog_response(GTK_DIALOG(d), GTK_RESPONSE_OK);
        }), dialog);

        gtk_widget_show_all(dialog);
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_CANCEL) {
            showScreen(SCREEN_LOBBY);
        } else if (response == GTK_RESPONSE_OK) {
            if (player_board) player_board->clearBoard();
            if (opponent_board) {
                opponent_board->clearBoard();
                opponent_board->randomPlacement();
            }
            shots_fired = 0;
            hits_count = 0;
            is_player_turn = true;
            for (int i = 0; i < NUM_SHIPS; i++) {
                ships_placed[i] = false;
            }
            showScreen(SCREEN_SHIP_PLACEMENT);
        }
    } else {
        // Enhanced normal notification with pixel art
        GtkWidget* dialog = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "NOTIFICATION");
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 250);

        GdkRGBA bg = {0.0, 0.0, 0.0, 1.0};
        gtk_widget_override_background_color(dialog, GTK_STATE_FLAG_NORMAL, &bg);

        GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_widget_override_background_color(content, GTK_STATE_FLAG_NORMAL, &bg);
        gtk_widget_set_margin_start(content, 30);
        gtk_widget_set_margin_end(content, 30);
        gtk_widget_set_margin_top(content, 30);
        gtk_widget_set_margin_bottom(content, 20);

        GtkWidget* label = gtk_label_new(message.c_str());
        GtkStyleContext* ctx = gtk_widget_get_style_context(label);
        gtk_style_context_add_class(ctx, "glow-text");
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);

        gtk_container_add(GTK_CONTAINER(content), label);

        GtkWidget* action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
        gtk_widget_override_background_color(action_area, GTK_STATE_FLAG_NORMAL, &bg);

        GtkWidget* ok_btn = gtk_button_new_with_label("[ OK ]");
        gtk_widget_set_size_request(ok_btn, 120, 40);
        gtk_container_add(GTK_CONTAINER(action_area), ok_btn);

        g_signal_connect(ok_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
            gtk_dialog_response(GTK_DIALOG(d), GTK_RESPONSE_OK);
        }), dialog);

        gtk_widget_show_all(dialog);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
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
                          is_player_turn ? ">> YOUR TURN <<" : ">> OPPONENT TURN <<");
    }
}

void UIManager::updateTimer(int seconds_remaining) {
    if (timer_label) {
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "[ TIME: %ds ]",
                seconds_remaining);
        gtk_label_set_text(GTK_LABEL(timer_label), time_str);
    }
}

// Game flow helper methods
void UIManager::switchTurn() {
    is_player_turn = !is_player_turn;

    if (turn_indicator) {
        if (is_player_turn) {
            gtk_label_set_text(GTK_LABEL(turn_indicator), ">> YOUR TURN <<");
        } else {
            gtk_label_set_text(GTK_LABEL(turn_indicator), ">> OPPONENT TURN <<");
        }
    }

    std::cout << (is_player_turn ? ">> YOUR TURN" : ">> OPPONENT TURN") << std::endl;

    // IMPORTANT: Restart timer for new turn (20 seconds per turn)
    startTurnTimer(20);
}

void UIManager::updateGameStats() {
    if (shots_label_widget) {
        char buf[32];
        snprintf(buf, sizeof(buf), "SHOTS: %d", shots_fired);
        gtk_label_set_text(GTK_LABEL(shots_label_widget), buf);
    }

    if (hits_label_widget) {
        char buf[32];
        snprintf(buf, sizeof(buf), "HITS: %d", hits_count);
        gtk_label_set_text(GTK_LABEL(hits_label_widget), buf);
    }

    if (accuracy_label_widget && shots_fired > 0) {
        char buf[32];
        int accuracy = (hits_count * 100) / shots_fired;
        snprintf(buf, sizeof(buf), "ACCURACY: %d%%", accuracy);
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

    std::cout << "SUNK: SUNK " << shipTypeToName(ship_type) << "!" << std::endl;
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
        std::cout << "Bot Bot HIT at " << (char)('A' + row) << (col + 1) << "!" << std::endl;
        // Bot continues turn on hit
        g_timeout_add(1000, botTurnCallback, this);  // Shoot again after 1s
    } else if (result == SHOT_SUNK) {
        std::cout << "Bot Bot SUNK your ship at " << (char)('A' + row) << (col + 1) << "!" << std::endl;
        // Bot continues turn on sunk
        g_timeout_add(1000, botTurnCallback, this);  // Shoot again after 1s
    } else {
        std::cout << "Bot MISS at " << (char)('A' + row) << (col + 1) << std::endl;
        switchTurn();  // Switch to player turn - timer restarts inside switchTurn()
    }

    // Redraw player board
    if (player_board_area) {
        gtk_widget_queue_draw(player_board_area);
    }

    // Check if player lost
    if (player_board->allShipsSunk()) {
        showNotification("DEFEAT! All your ships destroyed!");
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
                    std::cout << " Not your turn! Wait for opponent..." << std::endl;
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

    // Check if it's player's turn
    if (!ui->is_player_turn) {
        std::cout << "[UI] Not your turn!" << std::endl;
        ui->showNotification(">> NOT YOUR TURN <<\nWait for opponent...");
        return;
    }

    // Check if target is selected
    if (!ui->has_target_selected) {
        std::cout << "[UI] No target selected!" << std::endl;
        ui->showNotification(">> SELECT TARGET <<\nClick on enemy grid first!");
        return;
    }

    // Execute fire
    if (ui && ui->executeFireAtTarget()) {
        std::cout << "[UI] Shot fired at (" << ui->selected_target_row
                  << "," << ui->selected_target_col << ")" << std::endl;

        // Redraw boards
        if (ui->opponent_board_area && GTK_IS_WIDGET(ui->opponent_board_area)) {
            gtk_widget_queue_draw(ui->opponent_board_area);
        }
        if (ui->player_board_area && GTK_IS_WIDGET(ui->player_board_area)) {
            gtk_widget_queue_draw(ui->player_board_area);
        }
    } else {
        std::cout << "[UI] Fire failed!" << std::endl;
        ui->showNotification(">> INVALID SHOT <<\nSelect a different cell!");
    }
}

void on_resign_clicked(GtkButton* /*button*/, gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);
    if (ui->on_resign) {
        ui->on_resign();
    }
}

// Pause and draw offer removed per user request

// Target selection implementation
void UIManager::selectTarget(int row, int col) {
    selected_target_row = row;
    selected_target_col = col;
    has_target_selected = true;

    // Enable fire button
    if (fire_button) {
        gtk_widget_set_sensitive(fire_button, TRUE);
    }

    std::cout << " Target selected: " << (char)('A' + row) << (col + 1) << std::endl;
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
        std::cout << " Not your turn!" << std::endl;
        return false;
    }

    int row = selected_target_row;
    int col = selected_target_col;

    // In multiplayer mode, send move to server instead of processing locally
    if (network && network->isConnected() && current_match_id > 0) {
        std::cout << "[UI] Sending move to server: (" << row << "," << col << ")" << std::endl;
        
        // Disable fire button immediately to prevent multiple shots
        if (fire_button) {
            gtk_widget_set_sensitive(fire_button, FALSE);
        }
        
        network->sendMove(current_match_id, row, col);

        // Clear target selection
        clearTargetSelection();

        // The result will come back via MOVE_RESULT callback
        // Fire button will be re-enabled by MOVE_RESULT (if HIT) or TURN_UPDATE
        return true;
    }

    // Offline/bot mode: Process the shot locally
    Coordinate target;
    target.row = row;
    target.col = col;
    ShotResult result = opponent_board->processShot(target);

    // Update stats
    shots_fired++;

    // Play shot sound
    // AudioManager::getInstance().playShotSound();

    // Add animation based on result
    if (result == SHOT_HIT) {
        hits_count++;
        if (animation_manager) {
            animation_manager->addExplosion(row, col);
        }
        // Play hit sound effect
        // AudioManager::getInstance().playHitSound();
        std::cout << "💥 HIT at " << (char)('A' + row) << (col + 1) << "!" << std::endl;
    } else if (result == SHOT_SUNK) {
        hits_count++;
        // No explosion animation for sunk ships
        // Play ship sunk sound effect
        // AudioManager::getInstance().playSunkSound();
        std::cout << "🚢 SUNK at " << (char)('A' + row) << (col + 1) << "!" << std::endl;
    } else {
        if (animation_manager) {
            animation_manager->addSplash(row, col);
        }
        std::cout << "💦 MISS at " << (char)('A' + row) << (col + 1) << "!" << std::endl;
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
        showNotification("VICTORY! All enemy ships destroyed!");
        return true;
    }

    // Switch turn on miss
    if (result == SHOT_MISS) {
        switchTurn();  // This will auto-restart timer inside switchTurn()

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

    // Update label immediately - PIXEL STYLE
    if (turn_timer_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "[ TIME: %ds ]", turn_time_remaining);
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

    // Safety check: only update label if it still exists
    if (turn_timer_label && GTK_IS_WIDGET(turn_timer_label)) {
        gtk_label_set_text(GTK_LABEL(turn_timer_label), "TIME: --");
    }
}

gboolean UIManager::turnTimerCallback(gpointer data) {
    UIManager* ui = static_cast<UIManager*>(data);

    ui->turn_time_remaining--;

    // Update label with safety check - PIXEL STYLE
    if (ui->turn_timer_label && GTK_IS_WIDGET(ui->turn_timer_label)) {
        char buf[32];
        snprintf(buf, sizeof(buf), "[ TIME: %ds ]", ui->turn_time_remaining);
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
    std::cout << "TIME EXPIRED! Turn skipped." << std::endl;

    // Clear any selected target
    clearTargetSelection();

    // In network mode, report timeout to server (server will handle turn switch)
    if (!is_bot_mode && current_match_id > 0 && network) {
        std::cout << "[UI] Reporting timeout to server for match " << current_match_id << std::endl;
        network->sendTurnTimeout(current_match_id);
        // DO NOT call switchTurn() locally - wait for server's TURN_UPDATE
        // Timer will restart when we receive TURN_UPDATE from server
    } else if (is_bot_mode) {
        // Bot mode: handle locally as before
        switchTurn();

        if (!is_player_turn) {
            g_timeout_add(1000, UIManager::botTurnCallback, this);
        } else {
            startTurnTimer(20);
        }
    }
}

// ==================== Network Integration ====================

void UIManager::connectToServer(const std::string& host, int port) {
    std::cout << "[UI] Connecting to server..." << std::endl;

    network->connect(host, port, [this](bool connected, const std::string& error) {
        if (connected) {
            std::cout << "[UI] Connected to server" << std::endl;
        } else {
            std::cerr << "[UI] Connection failed: " << error << std::endl;
            showErrorDialog("Connection Error", "Failed to connect to server:\n" + error);
        }
    });
}

void UIManager::handleRegisterResponse(bool success, uint32_t user_id, const std::string& error) {
    // Schedule GTK operations in main thread
    struct CallbackData {
        UIManager* ui;
        bool success;
        uint32_t user_id;
        std::string error;
    };

    CallbackData* data = new CallbackData{this, success, user_id, error};

    g_idle_add([](gpointer user_data) -> gboolean {
        CallbackData* data = static_cast<CallbackData*>(user_data);

        if (data->success) {
            std::cout << "[UI] Registration successful! User ID: " << data->user_id << std::endl;
            data->ui->showInfoDialog("Registration Successful",
                          "Account created successfully!\nYou can now login with your credentials.");
            // Switch to login screen
            data->ui->showScreen(SCREEN_LOGIN);
        } else {
            std::cerr << "[UI] Registration failed: " << data->error << std::endl;
            data->ui->showErrorDialog("Registration Failed", data->error);
        }

        delete data;
        return FALSE;  // Don't repeat
    }, data);
}

void UIManager::handleLoginResponse(bool success, uint32_t user_id, const std::string& display_name,
                                    int32_t elo_rating, const std::string& session_token, const std::string& error) {
    // Schedule GTK operations in main thread
    struct CallbackData {
        UIManager* ui;
        bool success;
        uint32_t user_id;
        std::string display_name;
        int32_t elo_rating;
        std::string session_token;
        std::string error;
    };

    CallbackData* data = new CallbackData{this, success, user_id, display_name, elo_rating, session_token, error};

    g_idle_add([](gpointer user_data) -> gboolean {
        CallbackData* data = static_cast<CallbackData*>(user_data);

        if (data->success) {
            std::cout << "[UI] Login successful!" << std::endl;
            std::cout << "  User ID: " << data->user_id << std::endl;
            std::cout << "  Display Name: " << data->display_name << std::endl;
            std::cout << "  ELO: " << data->elo_rating << std::endl;

            // Update current player info
            data->ui->current_player.user_id = data->user_id;
            data->ui->current_player.username = data->display_name;
            data->ui->current_player.display_name = data->display_name;
            data->ui->current_player.elo_rating = data->elo_rating;
            data->ui->current_player.status = STATUS_ONLINE;

#if AUTO_LOGIN_ENABLED
            // Save session for auto-login
            SessionStorage::saveSession(data->user_id,
                                       data->session_token,
                                       data->display_name,  // Using display_name as username
                                       data->display_name,
                                       data->elo_rating);
            std::cout << "[UI] Session saved for auto-login" << std::endl;
#else
            std::cout << "[UI] Auto-login disabled, session not saved" << std::endl;
#endif

            // Show main menu
            data->ui->showScreen(SCREEN_MAIN_MENU);
        } else {
            std::cerr << "[UI] Login failed: " << data->error << std::endl;
            data->ui->showErrorDialog("Login Failed", data->error);
        }

        delete data;
        return FALSE;  // Don't repeat
    }, data);

    // Suppress unused warning
    (void)session_token;
}

void UIManager::handleLogoutResponse(bool success) {
    // Schedule GTK operations in main thread
    struct CallbackData {
        UIManager* ui;
        bool success;
    };

    CallbackData* data = new CallbackData{this, success};

    g_idle_add([](gpointer user_data) -> gboolean {
        CallbackData* data = static_cast<CallbackData*>(user_data);

        if (data->success) {
            std::cout << "[UI] Logout successful" << std::endl;

#if AUTO_LOGIN_ENABLED
            // Clear saved session
            SessionStorage::clearSession();
            std::cout << "[UI] Session cleared" << std::endl;
#else
            std::cout << "[UI] Auto-login disabled, no session to clear" << std::endl;
#endif

            data->ui->showScreen(SCREEN_LOGIN);
        } else {
            std::cerr << "[UI] Logout failed" << std::endl;
        }

        delete data;
        return FALSE;  // Don't repeat
    }, data);
}

void UIManager::showErrorDialog(const std::string& title, const std::string& message) {
    // Play error sound
    // AudioManager::getInstance().playErrorSound();

    // Create beautiful error dialog with ASCII art
    GtkWidget* dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "ERROR");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 350);

    // Red-tinted black background
    GdkRGBA bg = {0.1, 0.0, 0.0, 1.0};
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gtk_widget_override_background_color(dialog, GTK_STATE_FLAG_NORMAL, &bg);
    #pragma GCC diagnostic pop

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_widget_override_background_color(content, GTK_STATE_FLAG_NORMAL, &bg);
    gtk_widget_set_margin_start(content, 30);
    gtk_widget_set_margin_end(content, 30);
    gtk_widget_set_margin_top(content, 30);
    gtk_widget_set_margin_bottom(content, 20);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    // Error ASCII art
    GtkWidget* ascii_label = gtk_label_new(
        "╔══════════════════════════════════╗\n"
        "║                                  ║\n"
        "║     ⚠️  ERROR DETECTED  ⚠️        ║\n"
        "║                                  ║\n"
        "║   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄     ║\n"
        "║   █████╗ ██████╗ ██████╗        ║\n"
        "║   ██╔══╝ ██╔══██╗██╔══██╗       ║\n"
        "║   █████╗ ██████╔╝██████╔╝       ║\n"
        "║   ██╔══╝ ██╔══██╗██╔══██╗       ║\n"
        "║   █████║ ██║  ██║██║  ██║       ║\n"
        "║   ╚════╝ ╚═╝  ╚═╝╚═╝  ╚═╝       ║\n"
        "║   ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀     ║\n"
        "║                                  ║\n"
        "╚══════════════════════════════════╝"
    );
    GtkStyleContext* ascii_ctx = gtk_widget_get_style_context(ascii_label);
    gtk_style_context_add_class(ascii_ctx, "title");

    // Title and message
    GtkWidget* title_label = gtk_label_new(title.c_str());
    GtkStyleContext* title_ctx = gtk_widget_get_style_context(title_label);
    gtk_style_context_add_class(title_ctx, "glow-text");

    GtkWidget* msg_label = gtk_label_new(message.c_str());
    gtk_label_set_line_wrap(GTK_LABEL(msg_label), TRUE);
    gtk_label_set_justify(GTK_LABEL(msg_label), GTK_JUSTIFY_CENTER);
    GtkStyleContext* msg_ctx = gtk_widget_get_style_context(msg_label);
    gtk_style_context_add_class(msg_ctx, "glow-text");

    gtk_box_pack_start(GTK_BOX(vbox), ascii_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), msg_label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // OK button
    GtkWidget* action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
    gtk_widget_override_background_color(action_area, GTK_STATE_FLAG_NORMAL, &bg);

    GtkWidget* ok_btn = gtk_button_new_with_label("[ OK ]");
    gtk_widget_set_size_request(ok_btn, 120, 40);
    GtkStyleContext* btn_ctx = gtk_widget_get_style_context(ok_btn);
    gtk_style_context_add_class(btn_ctx, "danger");
    gtk_container_add(GTK_CONTAINER(action_area), ok_btn);

    g_signal_connect(ok_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        gtk_dialog_response(GTK_DIALOG(d), GTK_RESPONSE_OK);
    }), dialog);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void UIManager::showInfoDialog(const std::string& title, const std::string& message) {
    // Play success sound
    // AudioManager::getInstance().playSuccessSound();

    // Create beautiful info dialog with ASCII art
    GtkWidget* dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "SUCCESS");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 350);

    // Green-tinted black background
    GdkRGBA bg = {0.0, 0.1, 0.0, 1.0};
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gtk_widget_override_background_color(dialog, GTK_STATE_FLAG_NORMAL, &bg);
    #pragma GCC diagnostic pop

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_widget_override_background_color(content, GTK_STATE_FLAG_NORMAL, &bg);
    gtk_widget_set_margin_start(content, 30);
    gtk_widget_set_margin_end(content, 30);
    gtk_widget_set_margin_top(content, 30);
    gtk_widget_set_margin_bottom(content, 20);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    // Success ASCII art
    GtkWidget* ascii_label = gtk_label_new(
        "╔══════════════════════════════════╗\n"
        "║                                  ║\n"
        "║    ✅  SUCCESS  ✅                ║\n"
        "║                                  ║\n"
        "║   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄     ║\n"
        "║    ██████╗ ██╗  ██╗              ║\n"
        "║   ██╔═══██╗██║ ██╔╝              ║\n"
        "║   ██║   ██║█████╔╝               ║\n"
        "║   ██║   ██║██╔═██╗               ║\n"
        "║   ╚██████╔╝██║  ██╗              ║\n"
        "║    ╚═════╝ ╚═╝  ╚═╝              ║\n"
        "║   ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀     ║\n"
        "║                                  ║\n"
        "╚══════════════════════════════════╝"
    );
    GtkStyleContext* ascii_ctx = gtk_widget_get_style_context(ascii_label);
    gtk_style_context_add_class(ascii_ctx, "title");

    // Title and message
    GtkWidget* title_label = gtk_label_new(title.c_str());
    GtkStyleContext* title_ctx = gtk_widget_get_style_context(title_label);
    gtk_style_context_add_class(title_ctx, "glow-text");

    GtkWidget* msg_label = gtk_label_new(message.c_str());
    gtk_label_set_line_wrap(GTK_LABEL(msg_label), TRUE);
    gtk_label_set_justify(GTK_LABEL(msg_label), GTK_JUSTIFY_CENTER);
    GtkStyleContext* msg_ctx = gtk_widget_get_style_context(msg_label);
    gtk_style_context_add_class(msg_ctx, "glow-text");

    gtk_box_pack_start(GTK_BOX(vbox), ascii_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), msg_label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // OK button
    GtkWidget* action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
    gtk_widget_override_background_color(action_area, GTK_STATE_FLAG_NORMAL, &bg);

    GtkWidget* ok_btn = gtk_button_new_with_label("[ OK ]");
    gtk_widget_set_size_request(ok_btn, 120, 40);
    gtk_container_add(GTK_CONTAINER(action_area), ok_btn);

    g_signal_connect(ok_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        gtk_dialog_response(GTK_DIALOG(d), GTK_RESPONSE_OK);
    }), dialog);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void UIManager::showResultDialog(GameResult result, int elo_change, const char* reason) {
    // Create custom dialog - like challenge dialog with proper black background
    GtkWidget* dialog = gtk_dialog_new();

    const char* window_title;
    const char* ascii_title;
    GdkRGBA bg_color = {0.0, 0.0, 0.0, 1.0};  // Pure black

    switch (result) {
        case RESULT_WIN:
            window_title = "VICTORY";
            ascii_title = "╔════════════════════════╗\n"
                         "║  ⭐ >> VICTORY! << ⭐  ║\n"
                         "╚════════════════════════╝";
            bg_color = {0.0, 0.15, 0.0, 1.0};  // Dark green for victory
            break;
        case RESULT_LOSS:
            window_title = "DEFEAT";
            ascii_title = "╔════════════════════════╗\n"
                         "║  💀 >> DEFEAT << 💀    ║\n"
                         "╚════════════════════════╝";
            bg_color = {0.15, 0.0, 0.0, 1.0};  // Dark red for defeat
            break;
        case RESULT_DRAW:
        default:
            window_title = "DRAW";
            ascii_title = "╔════════════════════════╗\n"
                         "║  🤝 >> DRAW << 🤝      ║\n"
                         "╚════════════════════════╝";
            bg_color = {0.05, 0.05, 0.05, 1.0};  // Dark gray for draw
            break;
    }

    gtk_window_set_title(GTK_WINDOW(dialog), window_title);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 350);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    // Set background color
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gtk_widget_override_background_color(dialog, GTK_STATE_FLAG_NORMAL, &bg_color);
    #pragma GCC diagnostic pop

    // Content area with same background
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_widget_override_background_color(content_area, GTK_STATE_FLAG_NORMAL, &bg_color);
    gtk_widget_set_margin_start(content_area, 30);
    gtk_widget_set_margin_end(content_area, 30);
    gtk_widget_set_margin_top(content_area, 30);
    gtk_widget_set_margin_bottom(content_area, 20);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);

    // Title with ASCII art
    GtkWidget* title_label = gtk_label_new(ascii_title);
    GtkStyleContext* title_context = gtk_widget_get_style_context(title_label);
    gtk_style_context_add_class(title_context, "title");

    // Reason text
    const char* reason_display = reason ? reason : "Match ended";
    char reason_text[256];
    snprintf(reason_text, sizeof(reason_text), "\n%s\n", reason_display);
    GtkWidget* reason_label = gtk_label_new(reason_text);
    GtkStyleContext* reason_context = gtk_widget_get_style_context(reason_label);
    gtk_style_context_add_class(reason_context, "glow-text");
    gtk_label_set_justify(GTK_LABEL(reason_label), GTK_JUSTIFY_CENTER);

    // ELO change info
    char elo_text[256];
    int new_elo = network->getEloRating();
    if (elo_change > 0) {
        snprintf(elo_text, sizeof(elo_text),
                 "═════════════════════\n"
                 "ELO CHANGE: +%d ⬆️\n"
                 "NEW ELO: %d\n"
                 "═════════════════════",
                 elo_change, new_elo);
    } else if (elo_change < 0) {
        snprintf(elo_text, sizeof(elo_text),
                 "═════════════════════\n"
                 "ELO CHANGE: %d ⬇️\n"
                 "NEW ELO: %d\n"
                 "═════════════════════",
                 elo_change, new_elo);
    } else {
        snprintf(elo_text, sizeof(elo_text),
                 "═════════════════════\n"
                 "NO ELO CHANGE ⚖️\n"
                 "CURRENT ELO: %d\n"
                 "═════════════════════",
                 new_elo);
    }
    GtkWidget* elo_label = gtk_label_new(elo_text);
    GtkStyleContext* elo_context = gtk_widget_get_style_context(elo_label);
    gtk_style_context_add_class(elo_context, "glow-text");
    gtk_label_set_justify(GTK_LABEL(elo_label), GTK_JUSTIFY_CENTER);

    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), reason_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), elo_label, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    // Action area - custom buttons
    GtkWidget* action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
    gtk_widget_override_background_color(action_area, GTK_STATE_FLAG_NORMAL, &bg_color);
    gtk_widget_set_margin_start(action_area, 30);
    gtk_widget_set_margin_end(action_area, 30);
    gtk_widget_set_margin_bottom(action_area, 20);

    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    GtkWidget* lobby_btn = gtk_button_new_with_label("[ RETURN TO LOBBY ]");
    gtk_widget_set_size_request(lobby_btn, 200, 45);
    GtkStyleContext* lobby_context = gtk_widget_get_style_context(lobby_btn);
    gtk_style_context_add_class(lobby_context, "primary");

    gtk_box_pack_start(GTK_BOX(button_box), lobby_btn, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(action_area), button_box);

    // Connect button signal
    g_signal_connect(lobby_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_OK);
    }), dialog);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    // Return to lobby
    showScreen(SCREEN_LOBBY);
}

void UIManager::showChallengeDialog(const std::string& opponent_name, int opponent_elo) {
    // Create custom dialog - Pixel style with proper background
    GtkWidget* dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "CHALLENGE");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 300);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    // Set pure black background for dialog
    GdkRGBA dialog_bg = {0.0, 0.0, 0.0, 1.0};
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gtk_widget_override_background_color(dialog, GTK_STATE_FLAG_NORMAL, &dialog_bg);
    #pragma GCC diagnostic pop

    // Content area with black background
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_widget_override_background_color(content_area, GTK_STATE_FLAG_NORMAL, &dialog_bg);
    gtk_widget_set_margin_start(content_area, 30);
    gtk_widget_set_margin_end(content_area, 30);
    gtk_widget_set_margin_top(content_area, 30);
    gtk_widget_set_margin_bottom(content_area, 20);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    // ASCII Art Title
    GtkWidget* title_label = gtk_label_new(
        "╔═══════════════════════════╗\n"
        "║  >> INCOMING CHALLENGE << ║\n"
        "╚═══════════════════════════╝"
    );
    GtkStyleContext* title_context = gtk_widget_get_style_context(title_label);
    gtk_style_context_add_class(title_context, "title");

    // Opponent info - bright white text
    char opponent_text[256];
    snprintf(opponent_text, sizeof(opponent_text),
             "\n< %s >\nELO: %d\n\nwants to battle!",
             opponent_name.c_str(), opponent_elo);
    GtkWidget* opponent_label = gtk_label_new(opponent_text);
    GtkStyleContext* opp_context = gtk_widget_get_style_context(opponent_label);
    gtk_style_context_add_class(opp_context, "glow-text");
    gtk_label_set_justify(GTK_LABEL(opponent_label), GTK_JUSTIFY_CENTER);

    // Prompt - cyan text
    GtkWidget* warning_label = gtk_label_new("\n>> Do you accept the challenge? <<");
    GtkStyleContext* warn_context = gtk_widget_get_style_context(warning_label);
    gtk_style_context_add_class(warn_context, "glow-text");

    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), opponent_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), warning_label, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    // Action area - custom buttons with pixel style
    GtkWidget* action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
    gtk_widget_override_background_color(action_area, GTK_STATE_FLAG_NORMAL, &dialog_bg);
    gtk_widget_set_margin_start(action_area, 30);
    gtk_widget_set_margin_end(action_area, 30);
    gtk_widget_set_margin_bottom(action_area, 20);

    // Custom button box
    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    GtkWidget* decline_btn = gtk_button_new_with_label("[ DECLINE ]");
    gtk_widget_set_size_request(decline_btn, 150, 45);
    GtkStyleContext* decline_context = gtk_widget_get_style_context(decline_btn);
    gtk_style_context_add_class(decline_context, "danger");

    GtkWidget* accept_btn = gtk_button_new_with_label("[ ACCEPT ]");
    gtk_widget_set_size_request(accept_btn, 150, 45);

    gtk_box_pack_start(GTK_BOX(button_box), decline_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), accept_btn, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(action_area), button_box);

    // Connect button signals
    g_signal_connect(decline_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_REJECT);
    }), dialog);

    g_signal_connect(accept_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_ACCEPT);
    }), dialog);

    gtk_widget_show_all(dialog);

    // Run dialog and get response
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    // Get challenge ID from current screen; fallback to stored pending_challenge_id_
    uint32_t challenge_id = 0;
    if (current_screen) {
        challenge_id = GPOINTER_TO_UINT(
            g_object_get_data(G_OBJECT(current_screen), "challenge_id"));
    }
    if (challenge_id == 0 && pending_challenge_id_ != 0) {
        challenge_id = pending_challenge_id_;
    }

    if (challenge_id == 0) {
        std::cerr << "[UI] Error: No challenge ID found (current_screen=null and no pending ID)" << std::endl;
        return;
    }

    // Send response to server
    bool accept = (response == GTK_RESPONSE_ACCEPT);
    std::cout << "[UI] Challenge " << (accept ? "ACCEPTED" : "DECLINED") << std::endl;
    network->respondToChallenge(challenge_id, accept);

    // Clear challenge ID
    if (current_screen) {
        g_object_set_data(G_OBJECT(current_screen), "challenge_id", GUINT_TO_POINTER(0));
    }
    pending_challenge_id_ = 0;
}
