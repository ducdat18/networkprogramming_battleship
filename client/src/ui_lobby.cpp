#include "ui_manager.h"
#include <ctime>
#include <iostream>
#include <utility>

// Lobby screen data
struct LobbyData {
    UIManager* ui;
    GtkTreeView* player_tree_view;
    GtkListStore* player_list_store;
    GtkWidget* challenge_btn;
    GtkWidget* refresh_btn;
    uint32_t selected_user_id;
};

// Columns for player list TreeView
enum {
    COL_USER_ID,
    COL_DISPLAY_NAME,
    COL_ELO,
    COL_STATUS,
    COL_STATUS_TEXT,
    NUM_COLS
};

// Convert player status to display text
const char* getStatusText(PlayerStatus status) {
    switch (status) {
        case STATUS_AVAILABLE: return "Available";
        case STATUS_IN_GAME: return "In Game";
        case STATUS_BUSY: return "Busy";
        case STATUS_ONLINE: return "Online";
        default: return "Offline";
    }
}

// Callback when player list selection changes
static void on_player_selection_changed(GtkTreeSelection* selection, gpointer data) {
    LobbyData* lobby = static_cast<LobbyData*>(data);

    GtkTreeIter iter;
    GtkTreeModel* model;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        uint32_t user_id;
        int status;
        gtk_tree_model_get(model, &iter,
                          COL_USER_ID, &user_id,
                          COL_STATUS, &status,
                          -1);

        lobby->selected_user_id = user_id;

        // Enable challenge button only if player is available and not self
        bool can_challenge = (status == STATUS_AVAILABLE) &&
                            (user_id != lobby->ui->network->getUserId());
        gtk_widget_set_sensitive(lobby->challenge_btn, can_challenge);
    } else {
        lobby->selected_user_id = 0;
        gtk_widget_set_sensitive(lobby->challenge_btn, FALSE);
    }
}

// Callback when challenge button clicked
static void on_challenge_clicked(GtkButton* button, gpointer data) {
    LobbyData* lobby = static_cast<LobbyData*>(data);

    if (lobby->selected_user_id == 0) {
        return;
    }

    std::cout << "[LOBBY] Sending challenge to user " << lobby->selected_user_id << std::endl;

    // Disable button while sending
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

    lobby->ui->network->sendChallenge(lobby->selected_user_id, 60, false,
        [button](bool success, const std::string& error) {
            // Re-enable button
            gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);

            if (success) {
                std::cout << "[LOBBY] ✅ Challenge sent successfully!" << std::endl;
            } else {
                std::cerr << "[LOBBY] ❌ Failed to send challenge: " << error << std::endl;
            }
        });
}

// Callback when refresh button clicked
static void on_refresh_clicked(GtkButton* button, gpointer data) {
    LobbyData* lobby = static_cast<LobbyData*>(data);

    std::cout << "[LOBBY] Refreshing player list..." << std::endl;

    // Disable button while refreshing
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

    lobby->ui->network->requestPlayerList(
        [lobby, button](bool success, const std::vector<PlayerInfo_Message>& players) {
            // Re-enable button
            gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);

            if (!success) {
                std::cerr << "[LOBBY] ❌ Failed to get player list" << std::endl;
                return;
            }

            std::cout << "[LOBBY] ✅ Received " << players.size() << " players" << std::endl;

            // Clear existing list
            gtk_list_store_clear(lobby->player_list_store);

            // Add players to tree view
            for (const auto& player : players) {
                GtkTreeIter iter;
                gtk_list_store_append(lobby->player_list_store, &iter);
                gtk_list_store_set(lobby->player_list_store, &iter,
                                  COL_USER_ID, player.user_id,
                                  COL_DISPLAY_NAME, player.display_name,
                                  COL_ELO, player.elo_rating,
                                  COL_STATUS, (int)player.status,
                                  COL_STATUS_TEXT, getStatusText(player.status),
                                  -1);
            }
        });
}

// Update player status in tree view
static void updatePlayerStatus(LobbyData* lobby, const PlayerStatusUpdate& update) {
    GtkTreeIter iter;
    gboolean valid;

    // Find player in tree view
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(lobby->player_list_store), &iter);
    while (valid) {
        uint32_t user_id;
        gtk_tree_model_get(GTK_TREE_MODEL(lobby->player_list_store), &iter,
                          COL_USER_ID, &user_id,
                          -1);

        if (user_id == update.user_id) {
            // If player went offline, remove them from the list
            if (update.status == STATUS_OFFLINE) {
                gtk_list_store_remove(lobby->player_list_store, &iter);
                std::cout << "[LOBBY] Removed offline player: " << update.display_name << std::endl;
            } else {
                // Update status
                gtk_list_store_set(lobby->player_list_store, &iter,
                                  COL_STATUS, (int)update.status,
                                  COL_STATUS_TEXT, getStatusText(update.status),
                                  COL_ELO, update.elo_rating,
                                  -1);
            }
            return;
        }

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(lobby->player_list_store), &iter);
    }

    // Player not found - add them
    if (update.status != STATUS_OFFLINE) {
        gtk_list_store_append(lobby->player_list_store, &iter);
        gtk_list_store_set(lobby->player_list_store, &iter,
                          COL_USER_ID, update.user_id,
                          COL_DISPLAY_NAME, update.display_name,
                          COL_ELO, update.elo_rating,
                          COL_STATUS, (int)update.status,
                          COL_STATUS_TEXT, getStatusText(update.status),
                          -1);
    }
}

GtkWidget* UIManager::createLobbyScreen() {
    // Create lobby data (will be freed when screen is destroyed)
    LobbyData* lobby = new LobbyData();
    lobby->ui = this;
    lobby->selected_user_id = 0;

    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    g_object_set_data_full(G_OBJECT(main_box), "lobby_data", lobby, g_free);

    // Header
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_size_request(header, -1, 60);
    GdkRGBA header_bg = {0.0, 0.13, 0.27, 1.0};
    gtk_widget_override_background_color(header, GTK_STATE_FLAG_NORMAL, &header_bg);
    gtk_widget_set_margin_start(header, 20);
    gtk_widget_set_margin_end(header, 20);

    GtkWidget* title = gtk_label_new("NAVAL COMMAND CENTER");
    GtkStyleContext* title_context = gtk_widget_get_style_context(title);
    gtk_style_context_add_class(title_context, "title");

    // User info label (update with actual user data)
    char user_info_text[128];
    snprintf(user_info_text, sizeof(user_info_text), "%s | ELO: %d",
             network->getDisplayName().c_str(), network->getEloRating());
    GtkWidget* user_info = gtk_label_new(user_info_text);
    GtkStyleContext* user_context = gtk_widget_get_style_context(user_info);
    gtk_style_context_add_class(user_context, "glow-text");

    GtkWidget* logout_btn = gtk_button_new_with_label("LOGOUT");
    gtk_widget_set_size_request(logout_btn, 120, 40);
    GtkStyleContext* logout_context = gtk_widget_get_style_context(logout_btn);
    gtk_style_context_add_class(logout_context, "danger");
    g_signal_connect(logout_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        UIManager* ui = static_cast<UIManager*>(data);
        ui->network->logoutUser([ui](bool success) {
            if (success) {
                ui->showScreen(SCREEN_LOGIN);
            }
        });
    }), this);

    gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), logout_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), user_info, FALSE, FALSE, 0);

    // Content area
    GtkWidget* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(content, 20);
    gtk_widget_set_margin_end(content, 20);
    gtk_widget_set_margin_top(content, 20);
    gtk_widget_set_margin_bottom(content, 20);

    // Header with title and buttons
    GtkWidget* list_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget* players_label = gtk_label_new("🌐 ONLINE ADMIRALS");
    GtkStyleContext* players_context = gtk_widget_get_style_context(players_label);
    gtk_style_context_add_class(players_context, "glow-text");

    lobby->refresh_btn = gtk_button_new_with_label("🔄 REFRESH");
    gtk_widget_set_size_request(lobby->refresh_btn, 120, 35);
    g_signal_connect(lobby->refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), lobby);

    lobby->challenge_btn = gtk_button_new_with_label("⚔️  CHALLENGE");
    gtk_widget_set_size_request(lobby->challenge_btn, 150, 35);
    gtk_widget_set_sensitive(lobby->challenge_btn, FALSE);  // Disabled by default
    g_signal_connect(lobby->challenge_btn, "clicked", G_CALLBACK(on_challenge_clicked), lobby);

    gtk_box_pack_start(GTK_BOX(list_header), players_label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(list_header), lobby->challenge_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(list_header), lobby->refresh_btn, FALSE, FALSE, 0);

    // Create TreeView for player list
    lobby->player_list_store = gtk_list_store_new(NUM_COLS,
                                                   G_TYPE_UINT,     // USER_ID
                                                   G_TYPE_STRING,   // DISPLAY_NAME
                                                   G_TYPE_INT,      // ELO
                                                   G_TYPE_INT,      // STATUS
                                                   G_TYPE_STRING);  // STATUS_TEXT

    lobby->player_tree_view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(lobby->player_list_store)));
    g_object_unref(lobby->player_list_store);  // TreeView holds reference

    // Add columns
    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column;

    // Display Name column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Admiral", renderer,
                                                       "text", COL_DISPLAY_NAME,
                                                       NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(lobby->player_tree_view, column);

    // ELO column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("ELO", renderer,
                                                       "text", COL_ELO,
                                                       NULL);
    gtk_tree_view_append_column(lobby->player_tree_view, column);

    // Status column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Status", renderer,
                                                       "text", COL_STATUS_TEXT,
                                                       NULL);
    gtk_tree_view_append_column(lobby->player_tree_view, column);

    // Selection handling
    GtkTreeSelection* selection = gtk_tree_view_get_selection(lobby->player_tree_view);
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(selection, "changed", G_CALLBACK(on_player_selection_changed), lobby);

    // Scrolled window for tree view
    GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(lobby->player_tree_view));

    gtk_box_pack_start(GTK_BOX(content), list_header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(main_box), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content, TRUE, TRUE, 0);

    // Set up network callbacks for real-time updates
    network->setPlayerStatusCallback([this](const PlayerStatusUpdate& update) {
        // Update on GTK main thread
        g_idle_add(+[](gpointer data) -> gboolean {
            auto* pair = static_cast<std::pair<UIManager*, PlayerStatusUpdate*>*>(data);
            UIManager* ui = pair->first;
            PlayerStatusUpdate* update_ptr = pair->second;

            LobbyData* lobby_data = static_cast<LobbyData*>(
                g_object_get_data(G_OBJECT(ui->current_screen), "lobby_data"));

            if (lobby_data) {
                updatePlayerStatus(lobby_data, *update_ptr);
            }
            delete update_ptr;
            delete pair;
            return G_SOURCE_REMOVE;
        }, new std::pair<UIManager*, PlayerStatusUpdate*>(this, new PlayerStatusUpdate(update)));
    });

    network->setChallengeReceivedCallback([this](const ChallengeReceived& challenge) {
        // Show challenge dialog on GTK main thread
        g_idle_add(+[](gpointer data) -> gboolean {
            auto* challenge_ptr = static_cast<ChallengeReceived*>(data);
            UIManager* ui = static_cast<UIManager*>(g_object_get_data(G_OBJECT(data), "ui"));

            if (ui) {
                ui->showChallengeDialog(challenge_ptr->challenger_name, challenge_ptr->challenger_elo);

                // Store challenge ID for response
                g_object_set_data(G_OBJECT(ui->current_screen), "challenge_id",
                                 GUINT_TO_POINTER(challenge_ptr->challenge_id));
            }
            delete challenge_ptr;
            return G_SOURCE_REMOVE;
        }, new ChallengeReceived(challenge));
    });

    network->setMatchStartCallback([this](const MatchStartMessage& match) {
        // Transition to ship placement on GTK main thread
        g_idle_add(+[](gpointer data) -> gboolean {
            auto* match_ptr = static_cast<MatchStartMessage*>(data);
            UIManager* ui = static_cast<UIManager*>(g_object_get_data(G_OBJECT(data), "ui"));

            if (ui) {
                std::cout << "[LOBBY] 🎮 Match started! Transitioning to ship placement..." << std::endl;
                ui->showScreen(SCREEN_SHIP_PLACEMENT);
            }
            delete match_ptr;
            return G_SOURCE_REMOVE;
        }, new MatchStartMessage(match));
    });

    // Auto-request player list when screen loads
    g_idle_add(+[](gpointer data) -> gboolean {
        GtkButton* refresh_btn = GTK_BUTTON(data);
        g_signal_emit_by_name(refresh_btn, "clicked");
        return G_SOURCE_REMOVE;
    }, lobby->refresh_btn);

    return main_box;
}
