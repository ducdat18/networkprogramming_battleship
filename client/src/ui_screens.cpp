#include "ui_manager.h"
#include <iostream>

#define CELL_SIZE 60
#define CELL_PADDING 3
#define BOARD_MARGIN 30

GtkWidget* UIManager::createGameScreen() {
    // Main container
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_margin_start(header, 20);
    gtk_widget_set_margin_end(header, 20);
    gtk_widget_set_margin_top(header, 10);
    gtk_widget_set_margin_bottom(header, 10);

    // Logo and title
    GtkWidget* title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget* logo_label = gtk_label_new("⚓");
    gtk_widget_set_name(logo_label, "logo");
    GtkWidget* title_label = gtk_label_new("BATTLESHIP ONLINE");
    GtkStyleContext* title_context = gtk_widget_get_style_context(title_label);
    gtk_style_context_add_class(title_context, "title");
    gtk_box_pack_start(GTK_BOX(title_box), logo_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(title_box), title_label, FALSE, FALSE, 0);

    // User info
    GtkWidget* user_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    GtkWidget* elo_label = gtk_label_new("⭐ ELO: 1250");
    GtkWidget* winrate_label = gtk_label_new("🏆 68%");
    GtkWidget* username_label = gtk_label_new("👤 Player_001");

    GtkStyleContext* elo_context = gtk_widget_get_style_context(elo_label);
    gtk_style_context_add_class(elo_context, "glow-text");

    gtk_box_pack_start(GTK_BOX(user_box), elo_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(user_box), winrate_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(user_box), username_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(header), title_box, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), user_box, FALSE, FALSE, 0);

    // Game content
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_margin_start(content_box, 20);
    gtk_widget_set_margin_end(content_box, 20);
    gtk_box_set_homogeneous(GTK_BOX(content_box), FALSE);

    // Left panel - Player board
    GtkWidget* left_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget* player_label = gtk_label_new("YOUR FLEET");
    GtkStyleContext* player_label_context = gtk_widget_get_style_context(player_label);
    gtk_style_context_add_class(player_label_context, "glow-text");

    player_board_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(player_board_area,
                                BOARD_SIZE * CELL_SIZE + BOARD_MARGIN * 2,
                                BOARD_SIZE * CELL_SIZE + BOARD_MARGIN * 2);
    gtk_widget_add_events(player_board_area,
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(player_board_area, "draw", G_CALLBACK(on_board_draw), this);

    GtkWidget* ships_status_label = gtk_label_new(
        "Aircraft Carrier [#####]\n"
        "Battleship [####]\n"
        "Cruiser [###]\n"
        "Submarine [###]\n"
        "Destroyer [##]"
    );
    gtk_label_set_justify(GTK_LABEL(ships_status_label), GTK_JUSTIFY_LEFT);

    gtk_box_pack_start(GTK_BOX(left_panel), player_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_panel), player_board_area, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(left_panel), ships_status_label, FALSE, FALSE, 0);

    // Center panel - Match info
    GtkWidget* center_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_size_request(center_panel, 400, -1);

    // Timer
    timer_label = gtk_label_new("TIME: 00:45");
    GtkStyleContext* timer_context = gtk_widget_get_style_context(timer_label);
    gtk_style_context_add_class(timer_context, "title");

    // Turn indicator
    turn_indicator = gtk_label_new("» YOUR TURN «");
    GtkStyleContext* turn_context = gtk_widget_get_style_context(turn_indicator);
    gtk_style_context_add_class(turn_context, "glow-text");

    // Match stats
    GtkWidget* stats_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(stats_box), TRUE);

    shots_label_widget = gtk_label_new("Shots\n0");
    hits_label_widget = gtk_label_new("Hits\n0");
    accuracy_label_widget = gtk_label_new("Accuracy\n0%");

    gtk_box_pack_start(GTK_BOX(stats_box), shots_label_widget, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(stats_box), hits_label_widget, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(stats_box), accuracy_label_widget, TRUE, TRUE, 0);

    // Action buttons
    GtkWidget* btn_fire = gtk_button_new_with_label("FIRE!");
    GtkWidget* btn_pause = gtk_button_new_with_label("Pause");
    GtkWidget* btn_draw = gtk_button_new_with_label("Offer Draw");
    GtkWidget* btn_resign = gtk_button_new_with_label("Resign");

    GtkStyleContext* btn_resign_context = gtk_widget_get_style_context(btn_resign);
    gtk_style_context_add_class(btn_resign_context, "danger");

    GtkStyleContext* btn_pause_context = gtk_widget_get_style_context(btn_pause);
    gtk_style_context_add_class(btn_pause_context, "secondary");

    GtkStyleContext* btn_draw_context = gtk_widget_get_style_context(btn_draw);
    gtk_style_context_add_class(btn_draw_context, "secondary");

    g_signal_connect(btn_fire, "clicked", G_CALLBACK(on_fire_clicked), this);
    g_signal_connect(btn_pause, "clicked", G_CALLBACK(on_pause_clicked), this);
    g_signal_connect(btn_draw, "clicked", G_CALLBACK(on_draw_offer_clicked), this);
    g_signal_connect(btn_resign, "clicked", G_CALLBACK(on_resign_clicked), this);

    // Chat
    GtkWidget* chat_frame = gtk_frame_new("Chat");
    GtkWidget* chat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    chat_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(chat_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(chat_view), GTK_WRAP_WORD);
    gtk_widget_set_size_request(chat_view, -1, 150);

    GtkWidget* chat_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(chat_scroll), chat_view);

    GtkWidget* chat_input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* chat_entry = gtk_entry_new();
    GtkWidget* chat_send_btn = gtk_button_new_with_label("Send");
    gtk_widget_set_size_request(chat_send_btn, 50, -1);

    gtk_box_pack_start(GTK_BOX(chat_input_box), chat_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(chat_input_box), chat_send_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(chat_box), chat_scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(chat_box), chat_input_box, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(chat_frame), chat_box);

    // Pack center panel
    gtk_box_pack_start(GTK_BOX(center_panel), timer_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(center_panel), turn_indicator, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(center_panel), stats_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(center_panel), btn_fire, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(center_panel), btn_pause, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(center_panel), btn_draw, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(center_panel), btn_resign, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(center_panel), chat_frame, TRUE, TRUE, 0);

    // Right panel - Opponent board
    GtkWidget* right_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget* opponent_label = gtk_label_new("ENEMY WATERS");
    GtkStyleContext* opponent_label_context = gtk_widget_get_style_context(opponent_label);
    gtk_style_context_add_class(opponent_label_context, "glow-text");

    opponent_board_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(opponent_board_area,
                                BOARD_SIZE * CELL_SIZE + BOARD_MARGIN * 2,
                                BOARD_SIZE * CELL_SIZE + BOARD_MARGIN * 2);
    gtk_widget_add_events(opponent_board_area,
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(opponent_board_area, "draw", G_CALLBACK(on_board_draw), this);
    g_signal_connect(opponent_board_area, "button-press-event",
                    G_CALLBACK(on_board_button_press), this);

    GtkWidget* opponent_info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget* opponent_name = gtk_label_new("Opponent_999");
    GtkWidget* opponent_elo = gtk_label_new("ELO: 1180");
    GtkWidget* opponent_winrate = gtk_label_new("WIN RATE: 65%");

    gtk_box_pack_start(GTK_BOX(opponent_info_box), opponent_name, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(opponent_info_box), opponent_elo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(opponent_info_box), opponent_winrate, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(right_panel), opponent_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_panel), opponent_board_area, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(right_panel), opponent_info_box, FALSE, FALSE, 0);

    // Pack all panels
    gtk_box_pack_start(GTK_BOX(content_box), left_panel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), center_panel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), right_panel, FALSE, FALSE, 0);

    // Footer
    GtkWidget* footer = gtk_label_new("🎮 Battleship Online v1.0 | Match ID: #BT-12345 | 📡 Ping: 45ms");
    gtk_widget_set_margin_top(footer, 10);
    gtk_widget_set_margin_bottom(footer, 10);

    // Pack everything
    gtk_box_pack_start(GTK_BOX(main_box), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), footer, FALSE, FALSE, 0);

    return main_box;
}
GtkWidget* UIManager::createMainMenuScreen() {
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Custom header with minimize and close buttons
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(header, -1, 40);
    GdkRGBA header_bg = {0.0, 0.13, 0.27, 1.0};
    gtk_widget_override_background_color(header, GTK_STATE_FLAG_NORMAL, &header_bg);

    GtkWidget* logo = gtk_label_new("◆");
    gtk_widget_set_name(logo, "logo");
    GtkWidget* header_title = gtk_label_new("BATTLESHIP ONLINE");
    GtkStyleContext* header_title_ctx = gtk_widget_get_style_context(header_title);
    gtk_style_context_add_class(header_title_ctx, "glow-text");

    GtkWidget* spacer = gtk_label_new("");
    gtk_widget_set_hexpand(spacer, TRUE);

    // Minimize button
    GtkWidget* minimize_btn = gtk_button_new_with_label("_");
    gtk_widget_set_size_request(minimize_btn, 40, 40);
    g_signal_connect(minimize_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        gtk_window_iconify(GTK_WINDOW(ui->main_window));
    }), this);

    // Close button
    GtkWidget* close_btn = gtk_button_new_with_label("X");
    gtk_widget_set_size_request(close_btn, 40, 40);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer) {
        gtk_main_quit();
    }), nullptr);

    gtk_box_pack_start(GTK_BOX(header), logo, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(header), header_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), spacer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(header), minimize_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), close_btn, FALSE, FALSE, 0);

    // Center content
    GtkWidget* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 40);
    gtk_widget_set_halign(content, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(content, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(content, 100);
    gtk_widget_set_margin_bottom(content, 100);

    // Title
    GtkWidget* title = gtk_label_new("◆ BATTLESHIP ◆");
    GtkStyleContext* title_context = gtk_widget_get_style_context(title);
    gtk_style_context_add_class(title_context, "title");

    GtkWidget* subtitle = gtk_label_new("Naval Warfare Strategy Game");
    GtkStyleContext* subtitle_context = gtk_widget_get_style_context(subtitle);
    gtk_style_context_add_class(subtitle_context, "glow-text");

    // Buttons
    GtkWidget* btn_vs_bot = gtk_button_new_with_label("» PLAY VS BOT «");
    gtk_widget_set_size_request(btn_vs_bot, 400, 80);
    g_signal_connect(btn_vs_bot, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->is_bot_mode = true;
        // Reset boards and stats
        ui->shots_fired = 0;
        ui->hits_count = 0;
        ui->is_player_turn = true;
        if (ui->player_board) ui->player_board->clearBoard();
        if (ui->opponent_board) ui->opponent_board->randomPlacement();
        ui->showScreen(SCREEN_SHIP_PLACEMENT);
    }), this);

    GtkWidget* btn_play_online = gtk_button_new_with_label("» PLAY ONLINE «");
    gtk_widget_set_size_request(btn_play_online, 400, 80);
    g_signal_connect(btn_play_online, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->is_bot_mode = false;
        ui->showScreen(SCREEN_LOGIN);
    }), this);

    GtkWidget* btn_exit = gtk_button_new_with_label("EXIT");
    gtk_widget_set_size_request(btn_exit, 400, 50);
    GtkStyleContext* exit_context = gtk_widget_get_style_context(btn_exit);
    gtk_style_context_add_class(exit_context, "secondary");
    g_signal_connect(btn_exit, "clicked", G_CALLBACK(+[](GtkButton*, gpointer) {
        gtk_main_quit();
    }), nullptr);

    gtk_box_pack_start(GTK_BOX(content), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), subtitle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), btn_vs_bot, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(content), btn_play_online, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(content), btn_exit, FALSE, FALSE, 10);

    // Footer
    GtkWidget* footer = gtk_label_new("Version 1.0 | Use Alt+Drag to move window");
    gtk_widget_set_margin_bottom(footer, 20);

    gtk_box_pack_start(GTK_BOX(main_box), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), footer, FALSE, FALSE, 0);

    return main_box;
}

// Placeholder implementations for other screens
GtkWidget* UIManager::createLoginScreen() {
    // Main container
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Custom header bar with drag support
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(header, -1, 50);
    GdkRGBA header_bg = {0.0, 0.13, 0.27, 1.0}; // Navy
    gtk_widget_override_background_color(header, GTK_STATE_FLAG_NORMAL, &header_bg);

    // Make header draggable to move window
    makeDraggable(header, main_window);

    // Logo and title in header
    GtkWidget* header_title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(header_title_box, 20);
    GtkWidget* logo = gtk_label_new("◆");  // Simple diamond instead of emoji
    GtkWidget* header_title = gtk_label_new("BATTLESHIP ONLINE");
    GtkStyleContext* header_context = gtk_widget_get_style_context(header_title);
    gtk_style_context_add_class(header_context, "glow-text");
    gtk_box_pack_start(GTK_BOX(header_title_box), logo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header_title_box), header_title, FALSE, FALSE, 0);

    // Minimize button
    GtkWidget* minimize_btn = gtk_button_new_with_label("_");
    gtk_widget_set_size_request(minimize_btn, 50, 50);
    GtkStyleContext* min_context = gtk_widget_get_style_context(minimize_btn);
    gtk_style_context_add_class(min_context, "secondary");
    g_signal_connect(minimize_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        GtkWidget* window = GTK_WIDGET(data);
        gtk_window_iconify(GTK_WINDOW(window));
    }), main_window);

    // Close button
    GtkWidget* close_btn = gtk_button_new_with_label("X");
    gtk_widget_set_size_request(close_btn, 50, 50);
    GtkStyleContext* close_context = gtk_widget_get_style_context(close_btn);
    gtk_style_context_add_class(close_context, "danger");
    g_signal_connect(close_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->quit();
    }), this);

    gtk_box_pack_start(GTK_BOX(header), header_title_box, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(header), close_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), minimize_btn, FALSE, FALSE, 0);

    // Content area
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 25);
    gtk_widget_set_halign(content_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(content_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(content_box, 50);
    gtk_widget_set_margin_bottom(content_box, 50);

    // Title
    GtkWidget* title = gtk_label_new("◆ WELCOME ADMIRAL ◆");
    GtkStyleContext* title_context = gtk_widget_get_style_context(title);
    gtk_style_context_add_class(title_context, "title");

    GtkWidget* subtitle = gtk_label_new("Prepare for Naval Combat");
    GtkStyleContext* subtitle_context = gtk_widget_get_style_context(subtitle);
    gtk_style_context_add_class(subtitle_context, "glow-text");

    // Input fields
    GtkWidget* username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Username");
    gtk_widget_set_size_request(username_entry, 400, 50);

    GtkWidget* password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_widget_set_size_request(password_entry, 400, 50);

    // Buttons
    GtkWidget* login_btn = gtk_button_new_with_label("» ENTER BATTLE «");
    gtk_widget_set_size_request(login_btn, 400, 60);

    GtkWidget* register_btn = gtk_button_new_with_label("ENLIST NOW");
    gtk_widget_set_size_request(register_btn, 400, 45);
    GtkStyleContext* reg_context = gtk_widget_get_style_context(register_btn);
    gtk_style_context_add_class(reg_context, "secondary");

    // Footer info
    GtkWidget* footer = gtk_label_new("Version 1.0 | Multiplayer Naval Warfare");

    gtk_box_pack_start(GTK_BOX(content_box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), subtitle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), username_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), password_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), login_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), register_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), footer, FALSE, FALSE, 0);

    // Connect signals
    g_signal_connect(login_btn, "clicked",
                    G_CALLBACK(+[](GtkButton*, gpointer data) {
                        UIManager* ui = static_cast<UIManager*>(data);
                        ui->showScreen(SCREEN_LOBBY);
                    }), this);

    g_signal_connect(register_btn, "clicked",
                    G_CALLBACK(+[](GtkButton*, gpointer data) {
                        UIManager* ui = static_cast<UIManager*>(data);
                        ui->showScreen(SCREEN_REGISTER);
                    }), this);

    gtk_box_pack_start(GTK_BOX(main_box), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);

    return main_box;
}

GtkWidget* UIManager::createRegisterScreen() {
    // Similar to login but with confirm password
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header with close button
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(header, -1, 50);
    GdkRGBA header_bg = {0.0, 0.13, 0.27, 1.0};
    gtk_widget_override_background_color(header, GTK_STATE_FLAG_NORMAL, &header_bg);

    GtkWidget* header_title = gtk_label_new("⚓ ENLIST - CREATE ACCOUNT");
    gtk_widget_set_margin_start(header_title, 20);
    GtkStyleContext* header_context = gtk_widget_get_style_context(header_title);
    gtk_style_context_add_class(header_context, "glow-text");

    GtkWidget* back_btn = gtk_button_new_with_label("← BACK");
    gtk_widget_set_size_request(back_btn, 100, 50);
    GtkStyleContext* back_context = gtk_widget_get_style_context(back_btn);
    gtk_style_context_add_class(back_context, "secondary");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->showScreen(SCREEN_LOGIN);
    }), this);

    gtk_box_pack_start(GTK_BOX(header), header_title, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), back_btn, FALSE, FALSE, 0);

    // Content
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(content_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(content_box, GTK_ALIGN_CENTER);

    GtkWidget* username = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username), "Username (min 3 characters)");
    gtk_widget_set_size_request(username, 400, 50);

    GtkWidget* password = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password), "Password (min 6 characters)");
    gtk_entry_set_visibility(GTK_ENTRY(password), FALSE);
    gtk_widget_set_size_request(password, 400, 50);

    GtkWidget* confirm = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(confirm), "Confirm Password");
    gtk_entry_set_visibility(GTK_ENTRY(confirm), FALSE);
    gtk_widget_set_size_request(confirm, 400, 50);

    GtkWidget* register_btn = gtk_button_new_with_label("🎖️ CREATE ACCOUNT");
    gtk_widget_set_size_request(register_btn, 400, 60);
    g_signal_connect(register_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->showScreen(SCREEN_LOGIN);
    }), this);

    gtk_box_pack_start(GTK_BOX(content_box), username, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), password, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), confirm, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), register_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_box), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);

    return main_box;
}

GtkWidget* UIManager::createLobbyScreen() {
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_size_request(header, -1, 60);
    GdkRGBA header_bg = {0.0, 0.13, 0.27, 1.0};
    gtk_widget_override_background_color(header, GTK_STATE_FLAG_NORMAL, &header_bg);
    gtk_widget_set_margin_start(header, 20);
    gtk_widget_set_margin_end(header, 20);

    GtkWidget* title = gtk_label_new("⚓ NAVAL COMMAND CENTER");
    GtkStyleContext* title_context = gtk_widget_get_style_context(title);
    gtk_style_context_add_class(title_context, "title");

    GtkWidget* user_info = gtk_label_new("Admiral_001 | ⭐ ELO: 1200");
    GtkStyleContext* user_context = gtk_widget_get_style_context(user_info);
    gtk_style_context_add_class(user_context, "glow-text");

    GtkWidget* logout_btn = gtk_button_new_with_label("🚪 LOGOUT");
    gtk_widget_set_size_request(logout_btn, 120, 40);
    GtkStyleContext* logout_context = gtk_widget_get_style_context(logout_btn);
    gtk_style_context_add_class(logout_context, "danger");
    g_signal_connect(logout_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->showScreen(SCREEN_LOGIN);
    }), this);

    gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), logout_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), user_info, FALSE, FALSE, 0);

    // Content area
    GtkWidget* content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_margin_start(content, 20);
    gtk_widget_set_margin_end(content, 20);
    gtk_widget_set_margin_top(content, 20);
    gtk_widget_set_margin_bottom(content, 20);

    // Left panel - Online players
    GtkWidget* left_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(left_panel, 500, -1);

    GtkWidget* players_label = gtk_label_new("🌐 ONLINE ADMIRALS");
    GtkStyleContext* players_context = gtk_widget_get_style_context(players_label);
    gtk_style_context_add_class(players_context, "glow-text");

    GtkWidget* players_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(players_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(players_scroll, -1, 600);

    GtkWidget* players_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Sample players
    for (int i = 1; i <= 10; i++) {
        GtkWidget* player_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_start(player_row, 10);
        gtk_widget_set_margin_end(player_row, 10);
        gtk_widget_set_margin_top(player_row, 5);
        gtk_widget_set_margin_bottom(player_row, 5);

        char player_info[128];
        snprintf(player_info, sizeof(player_info),
                 "👤 Admiral_%03d | ⭐ %d | 🏆 %d%%",
                 100 + i, 1100 + i * 50, 50 + i * 3);
        GtkWidget* info_label = gtk_label_new(player_info);

        GtkWidget* challenge_btn = gtk_button_new_with_label("⚔️ CHALLENGE");
        gtk_widget_set_size_request(challenge_btn, 150, 35);
        g_signal_connect(challenge_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
            UIManager* ui = static_cast<UIManager*>(data);
            ui->showScreen(SCREEN_SHIP_PLACEMENT);
        }), this);

        gtk_box_pack_start(GTK_BOX(player_row), info_label, TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(player_row), challenge_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(players_list), player_row, FALSE, FALSE, 0);
    }

    gtk_container_add(GTK_CONTAINER(players_scroll), players_list);
    gtk_box_pack_start(GTK_BOX(left_panel), players_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_panel), players_scroll, TRUE, TRUE, 0);

    // Right panel - Leaderboard
    GtkWidget* right_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget* leaderboard_label = gtk_label_new("🏆 TOP ADMIRALS");
    GtkStyleContext* leaderboard_context = gtk_widget_get_style_context(leaderboard_label);
    gtk_style_context_add_class(leaderboard_context, "glow-text");

    GtkWidget* leaderboard_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(leaderboard_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget* leaderboard_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    for (int i = 1; i <= 20; i++) {
        char rank_info[128];
        const char* medal = i == 1 ? "🥇" : i == 2 ? "🥈" : i == 3 ? "🥉" : "🎖️";
        snprintf(rank_info, sizeof(rank_info),
                 "%s #%d - Admiral_%03d - ELO: %d",
                 medal, i, i, 2000 - i * 50);
        GtkWidget* rank_label = gtk_label_new(rank_info);
        gtk_widget_set_margin_start(rank_label, 10);
        gtk_widget_set_margin_top(rank_label, 3);
        gtk_widget_set_margin_bottom(rank_label, 3);
        gtk_box_pack_start(GTK_BOX(leaderboard_list), rank_label, FALSE, FALSE, 0);
    }

    gtk_container_add(GTK_CONTAINER(leaderboard_scroll), leaderboard_list);
    gtk_box_pack_start(GTK_BOX(right_panel), leaderboard_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_panel), leaderboard_scroll, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(content), left_panel, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(content), right_panel, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(main_box), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content, TRUE, TRUE, 0);

    return main_box;
}

GtkWidget* UIManager::createShipPlacementScreen() {
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_size_request(header, -1, 60);
    GdkRGBA header_bg = {0.0, 0.13, 0.27, 1.0};
    gtk_widget_override_background_color(header, GTK_STATE_FLAG_NORMAL, &header_bg);
    gtk_widget_set_margin_start(header, 20);
    gtk_widget_set_margin_end(header, 20);

    GtkWidget* title = gtk_label_new("DEPLOY YOUR FLEET");
    GtkStyleContext* title_context = gtk_widget_get_style_context(title);
    gtk_style_context_add_class(title_context, "title");

    GtkWidget* instruction = gtk_label_new("Position your ships strategically");
    GtkStyleContext* inst_context = gtk_widget_get_style_context(instruction);
    gtk_style_context_add_class(inst_context, "glow-text");

    gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), instruction, FALSE, FALSE, 0);

    // Content
    GtkWidget* content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 30);
    gtk_widget_set_halign(content, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(content, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(content, 50);
    gtk_widget_set_margin_end(content, 50);

    // Left - Board
    GtkWidget* left_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget* board_label = gtk_label_new("YOUR WATERS");
    GtkStyleContext* board_context = gtk_widget_get_style_context(board_label);
    gtk_style_context_add_class(board_context, "glow-text");

    player_board_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(player_board_area, 660, 660);
    gtk_widget_add_events(player_board_area, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(player_board_area, "draw", G_CALLBACK(on_board_draw), this);
    g_signal_connect(player_board_area, "button-press-event", G_CALLBACK(on_board_button_press), this);
    g_signal_connect(player_board_area, "motion-notify-event", G_CALLBACK(on_board_motion_notify), this);

    gtk_box_pack_start(GTK_BOX(left_panel), board_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_panel), player_board_area, FALSE, FALSE, 0);

    // Right - Ship selection
    GtkWidget* right_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_size_request(right_panel, 350, -1);

    GtkWidget* ships_label = gtk_label_new("AVAILABLE SHIPS");
    GtkStyleContext* ships_context = gtk_widget_get_style_context(ships_label);
    gtk_style_context_add_class(ships_context, "glow-text");

    // Ship list
    const char* ships[] = {
        "Aircraft Carrier (5 cells)",
        "Battleship (4 cells)",
        "Cruiser (3 cells)",
        "Submarine (3 cells)",
        "Destroyer (2 cells)"
    };

    GtkWidget* ships_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    for (int i = 0; i < 5; i++) {
        GtkWidget* ship_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

        // Create clickable button for ship selection
        GtkWidget* ship_button = gtk_button_new_with_label(ships[i]);
        gtk_widget_set_size_request(ship_button, 250, 35);

        // Store the ship type in button data
        g_object_set_data(G_OBJECT(ship_button), "ship_type", GINT_TO_POINTER(i));
        g_signal_connect(ship_button, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            UIManager* ui = static_cast<UIManager*>(data);
            int ship_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "ship_type"));
            ui->selectShipForPlacement((ShipType)ship_type);
        }), this);

        GtkWidget* placed_label = gtk_label_new("❌");
        ship_status_labels[i] = placed_label;  // Store reference
        ship_buttons[i] = ship_button;  // Store button for graying out

        gtk_box_pack_start(GTK_BOX(ship_row), ship_button, TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(ship_row), placed_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(ships_box), ship_row, FALSE, FALSE, 0);
    }

    // Controls
    GtkWidget* controls_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(controls_box, 20);

    GtkWidget* rotate_btn = gtk_button_new_with_label("ROTATE SHIP");
    gtk_widget_set_size_request(rotate_btn, -1, 50);
    g_signal_connect(rotate_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->toggleShipOrientation();
    }), this);

    GtkWidget* random_btn = gtk_button_new_with_label("RANDOM PLACEMENT");
    gtk_widget_set_size_request(random_btn, -1, 50);
    GtkStyleContext* random_context = gtk_widget_get_style_context(random_btn);
    gtk_style_context_add_class(random_context, "secondary");
    g_signal_connect(random_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->randomPlaceAllShips();
    }), this);

    GtkWidget* ready_btn = gtk_button_new_with_label("READY FOR BATTLE!");
    gtk_widget_set_size_request(ready_btn, -1, 60);
    ready_battle_button = ready_btn;  // Store reference
    gtk_widget_set_sensitive(ready_btn, FALSE);  // Disabled until all ships placed
    g_signal_connect(ready_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        if (ui->allShipsPlaced()) {
            ui->showScreen(SCREEN_GAME);
        }
    }), this);

    GtkWidget* back_btn = gtk_button_new_with_label("← BACK TO LOBBY");
    gtk_widget_set_size_request(back_btn, -1, 40);
    GtkStyleContext* back_context = gtk_widget_get_style_context(back_btn);
    gtk_style_context_add_class(back_context, "danger");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->showScreen(SCREEN_LOBBY);
    }), this);

    gtk_box_pack_start(GTK_BOX(controls_box), rotate_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), random_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), ready_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), back_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(right_panel), ships_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_panel), ships_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_panel), controls_box, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), left_panel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), right_panel, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_box), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content, TRUE, TRUE, 0);

    return main_box;
}

GtkWidget* UIManager::createReplayScreen() {
    return gtk_label_new("Replay Screen - TODO");
}

GtkWidget* UIManager::createProfileScreen() {
    return gtk_label_new("Profile Screen - TODO");
}
