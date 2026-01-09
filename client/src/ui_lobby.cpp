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
            
            // Debug: Print all received players
            uint32_t current_user_id = lobby->ui->network->getUserId();
            std::cout << "[LOBBY] Current user ID: " << current_user_id << std::endl;
            for (const auto& player : players) {
                std::cout << "[LOBBY]   - Player: " << player.display_name 
                          << " (ID: " << player.user_id 
                          << ", Status: " << (int)player.status << ")" << std::endl;
            }

            // Clear existing list
            gtk_list_store_clear(lobby->player_list_store);

            // Add players to tree view (including self - no filtering)
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
    std::cout << "[LOBBY] updatePlayerStatus called for: " << update.display_name 
              << " (ID: " << update.user_id << ", Status: " << (int)update.status << ")" << std::endl;
    
    GtkTreeIter iter;
    gboolean valid;
    bool player_found = false;

    // Find player in tree view
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(lobby->player_list_store), &iter);
    while (valid) {
        uint32_t user_id;
        gtk_tree_model_get(GTK_TREE_MODEL(lobby->player_list_store), &iter,
                          COL_USER_ID, &user_id,
                          -1);

        if (user_id == update.user_id) {
            player_found = true;
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
                std::cout << "[LOBBY] Updated existing player: " << update.display_name << std::endl;
            }
            break;
        }

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(lobby->player_list_store), &iter);
    }

    // Player not found - add them
    if (!player_found && update.status != STATUS_OFFLINE) {
        std::cout << "[LOBBY] Player not in list, adding: " << update.display_name << std::endl;
        gtk_list_store_append(lobby->player_list_store, &iter);
        gtk_list_store_set(lobby->player_list_store, &iter,
                          COL_USER_ID, update.user_id,
                          COL_DISPLAY_NAME, update.display_name,
                          COL_ELO, update.elo_rating,
                          COL_STATUS, (int)update.status,
                          COL_STATUS_TEXT, getStatusText(update.status),
                          -1);
        std::cout << "[LOBBY] ✅ Added new player to list: " << update.display_name << std::endl;
    } else if (!player_found && update.status == STATUS_OFFLINE) {
        std::cout << "[LOBBY] Skipping offline player (not in list): " << update.display_name << std::endl;
    }
}

// Custom deleter for LobbyData (C++ class with new/delete)
static void lobby_data_destroy(gpointer data) {
    delete static_cast<LobbyData*>(data);
}

GtkWidget* UIManager::createLobbyScreen() {
    // Create lobby data (will be freed when screen is destroyed)
    LobbyData* lobby = new LobbyData();
    lobby->ui = this;
    lobby->selected_user_id = 0;

    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    g_object_set_data_full(G_OBJECT(main_box), "lobby_data", lobby, lobby_data_destroy);

    // Header - PURE BLACK PIXEL STYLE
    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_size_request(header, -1, 50);
    GdkRGBA header_bg = {0.0, 0.0, 0.0, 1.0};  // Pure black for max contrast
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gtk_widget_override_background_color(header, GTK_STATE_FLAG_NORMAL, &header_bg);
    #pragma GCC diagnostic pop
    gtk_widget_set_margin_start(header, 10);
    gtk_widget_set_margin_end(header, 10);
    gtk_widget_set_margin_top(header, 8);
    gtk_widget_set_margin_bottom(header, 8);

    GtkWidget* title = gtk_label_new("╔═══ LOBBY ═══╗");
    GtkStyleContext* title_context = gtk_widget_get_style_context(title);
    gtk_style_context_add_class(title_context, "title");

    // User info label (pixel art style)
    char user_info_text[128];
    snprintf(user_info_text, sizeof(user_info_text), "< %s > ELO:%d",
             network->getDisplayName().c_str(), network->getEloRating());
    GtkWidget* user_info = gtk_label_new(user_info_text);
    GtkStyleContext* user_context = gtk_widget_get_style_context(user_info);
    gtk_style_context_add_class(user_context, "glow-text");

    GtkWidget* logout_btn = gtk_button_new_with_label("[ EXIT ]");
    gtk_widget_set_size_request(logout_btn, 100, 36);
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

    // Matchmaking section - PIXEL ART STYLE
    GtkWidget* matchmaking_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_bottom(matchmaking_box, 10);

    GtkWidget* matchmaking_label = gtk_label_new(">> RANKED MATCHMAKING");
    GtkStyleContext* mm_context = gtk_widget_get_style_context(matchmaking_label);
    gtk_style_context_add_class(mm_context, "glow-text");

    GtkWidget* queue_btn = gtk_button_new_with_label("[ JOIN QUEUE ]");
    gtk_widget_set_size_request(queue_btn, 140, 40);
    GtkStyleContext* queue_context = gtk_widget_get_style_context(queue_btn);
    gtk_style_context_add_class(queue_context, "primary");
    g_signal_connect(queue_btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        LobbyData* lobby = static_cast<LobbyData*>(data);
        const char* label = gtk_button_get_label(btn);
        
        if (g_strcmp0(label, "[ JOIN QUEUE ]") == 0) {
            // Join queue
            lobby->ui->network->joinQueue([lobby, btn](bool success, const std::string& error) {
                if (success) {
                    g_idle_add(+[](gpointer data) -> gboolean {
                        GtkButton* button = GTK_BUTTON(data);
                        gtk_button_set_label(button, "[ LEAVE QUEUE ]");
                        GtkStyleContext* ctx = gtk_widget_get_style_context(GTK_WIDGET(button));
                        gtk_style_context_remove_class(ctx, "primary");
                        gtk_style_context_add_class(ctx, "danger");
                        return G_SOURCE_REMOVE;
                    }, btn);
                    std::cout << "[LOBBY] Joined matchmaking queue" << std::endl;
                } else {
                    std::cerr << "[LOBBY] Failed to join queue: " << error << std::endl;
                }
            });
        } else {
            // Leave queue
            lobby->ui->network->leaveQueue([lobby, btn](bool success, const std::string& error) {
                if (success) {
                    g_idle_add(+[](gpointer data) -> gboolean {
                        GtkButton* button = GTK_BUTTON(data);
                        gtk_button_set_label(button, "[ JOIN QUEUE ]");
                        GtkStyleContext* ctx = gtk_widget_get_style_context(GTK_WIDGET(button));
                        gtk_style_context_remove_class(ctx, "danger");
                        gtk_style_context_add_class(ctx, "primary");
                        return G_SOURCE_REMOVE;
                    }, btn);
                    std::cout << "[LOBBY] Left matchmaking queue" << std::endl;
                } else {
                    std::cerr << "[LOBBY] Failed to leave queue: " << error << std::endl;
                }
            });
        }
    }), lobby);

    GtkWidget* queue_status_label = gtk_label_new("Not in queue");
    gtk_widget_set_margin_start(queue_status_label, 10);

    gtk_box_pack_start(GTK_BOX(matchmaking_box), matchmaking_label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(matchmaking_box), queue_status_label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(matchmaking_box), queue_btn, FALSE, FALSE, 0);

    // Header with title and buttons - PIXEL ART STYLE
    GtkWidget* list_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    GtkWidget* players_label = gtk_label_new(">> PLAYERS ONLINE");
    GtkStyleContext* players_context = gtk_widget_get_style_context(players_label);
    gtk_style_context_add_class(players_context, "glow-text");

    lobby->refresh_btn = gtk_button_new_with_label("[ RELOAD ]");
    gtk_widget_set_size_request(lobby->refresh_btn, 110, 34);
    g_signal_connect(lobby->refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), lobby);

    // Challenge button - pixel art design
    lobby->challenge_btn = gtk_button_new_with_label("[ FIGHT! ]");
    gtk_widget_set_size_request(lobby->challenge_btn, 110, 34);
    gtk_widget_set_sensitive(lobby->challenge_btn, FALSE);  // Disabled by default
    g_signal_connect(lobby->challenge_btn, "clicked", G_CALLBACK(on_challenge_clicked), lobby);

    GtkWidget* replay_btn = gtk_button_new_with_label("[ REPLAYS ]");
    gtk_widget_set_size_request(replay_btn, 110, 34);
    g_signal_connect(replay_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        LobbyData* lobby = static_cast<LobbyData*>(data);
        lobby->ui->showScreen(SCREEN_REPLAY);
    }), lobby);

    gtk_box_pack_start(GTK_BOX(list_header), players_label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(list_header), lobby->challenge_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(list_header), replay_btn, FALSE, FALSE, 0);
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

    // Add columns - SIMPLIFIED: Clean, minimal headers
    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column;

    // Display Name column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Player", renderer,
                                                       "text", COL_DISPLAY_NAME,
                                                       NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_column_set_min_width(column, 150);
    gtk_tree_view_append_column(lobby->player_tree_view, column);

    // ELO column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("ELO", renderer,
                                                       "text", COL_ELO,
                                                       NULL);
    gtk_tree_view_column_set_min_width(column, 80);
    gtk_tree_view_append_column(lobby->player_tree_view, column);

    // Status column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Status", renderer,
                                                       "text", COL_STATUS_TEXT,
                                                       NULL);
    gtk_tree_view_column_set_min_width(column, 100);
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

    gtk_box_pack_start(GTK_BOX(content), matchmaking_box, FALSE, FALSE, 0);
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

            std::cout << "[LOBBY] Player status callback received for: " << update_ptr->display_name 
                      << " (ID: " << update_ptr->user_id << ")" << std::endl;

            LobbyData* lobby_data = static_cast<LobbyData*>(
                g_object_get_data(G_OBJECT(ui->current_screen), "lobby_data"));

            if (lobby_data) {
                std::cout << "[LOBBY] lobby_data found, updating player status" << std::endl;
                updatePlayerStatus(lobby_data, *update_ptr);
            } else {
                std::cerr << "[LOBBY] WARNING: lobby_data is null! Current screen might not be lobby screen." << std::endl;
                std::cerr << "[LOBBY] This update will be lost. Player: " << update_ptr->display_name << std::endl;
            }
            delete update_ptr;
            delete pair;
            return G_SOURCE_REMOVE;
        }, new std::pair<UIManager*, PlayerStatusUpdate*>(this, new PlayerStatusUpdate(update)));
    });

    network->setChallengeReceivedCallback([this](const ChallengeReceived& challenge) {
        // Show challenge dialog on GTK main thread
        g_idle_add(+[](gpointer data) -> gboolean {
            auto* pair = static_cast<std::pair<UIManager*, ChallengeReceived*>*>(data);
            UIManager* ui = pair->first;
            ChallengeReceived* challenge_ptr = pair->second;

            if (ui) {
                uint32_t my_id = ui->network->getUserId();

                // Safety check: only the target (not the challenger) should see the challenge dialog
                if (my_id == challenge_ptr->challenger_id) {
                    std::cout << "[LOBBY] Ignoring CHALLENGE_RECEIVED on challenger side (user_id="
                              << my_id << ", challenge_id=" << challenge_ptr->challenge_id << ")" << std::endl;
                } else {
                    std::cout << "[LOBBY] Showing challenge dialog to target user_id=" << my_id
                              << " from: " << challenge_ptr->challenger_name
                              << " (challenge_id=" << challenge_ptr->challenge_id << ")" << std::endl;

                    // Store challenge ID even if current_screen is null (for later retrieval)
                    ui->pending_challenge_id_ = challenge_ptr->challenge_id;
                    if (ui->current_screen) {
                        g_object_set_data(G_OBJECT(ui->current_screen), "challenge_id",
                                         GUINT_TO_POINTER(challenge_ptr->challenge_id));
                    } else {
                        std::cerr << "[LOBBY] WARNING: current_screen is null, stored challenge_id in pending_challenge_id_" << std::endl;
                    }

                    ui->showChallengeDialog(challenge_ptr->challenger_name, challenge_ptr->challenger_elo);
                }
            } else {
                std::cerr << "[LOBBY] WARNING: UIManager is null in challenge callback" << std::endl;
            }
            delete challenge_ptr;
            delete pair;
            return G_SOURCE_REMOVE;
        }, new std::pair<UIManager*, ChallengeReceived*>(this, new ChallengeReceived(challenge)));
    });

    network->setMatchStartCallback([this](const MatchStartMessage& match) {
        // Transition to ship placement on GTK main thread
        struct CallbackData {
            UIManager* ui;
            uint32_t match_id;
            uint32_t opponent_id;
            std::string opponent_name;
        };

        g_idle_add(+[](gpointer data) -> gboolean {
            auto* cb_data = static_cast<CallbackData*>(data);

            if (cb_data->ui) {
                std::cout << "[LOBBY] 🎮 Match started! match_id=" << cb_data->match_id
                          << " vs " << cb_data->opponent_name << " Transitioning to ship placement..." << std::endl;
                cb_data->ui->current_match_id = cb_data->match_id;
                cb_data->ui->waiting_for_match_ready = false;

                // Store opponent info for rechallenge
                cb_data->ui->last_opponent_id_ = cb_data->opponent_id;
                cb_data->ui->last_opponent_name_ = cb_data->opponent_name;

                // Clear boards for new match - COMPLETE RESET
                std::cout << "[UI] Clearing boards for new match..." << std::endl;
                if (cb_data->ui->player_board) {
                    cb_data->ui->player_board->clearBoard();
                    std::cout << "[UI] Player board cleared" << std::endl;
                }
                if (cb_data->ui->opponent_board) {
                    cb_data->ui->opponent_board->clearBoard();
                    std::cout << "[UI] Opponent board cleared" << std::endl;
                }

                // Force invalidate old drawing areas (they'll be recreated)
                cb_data->ui->player_board_area = nullptr;
                cb_data->ui->opponent_board_area = nullptr;

                // Reset game stats for new match
                cb_data->ui->shots_fired = 0;
                cb_data->ui->hits_count = 0;

                // IMPORTANT: Reset ship placement UI state BEFORE creating new screen
                for (int i = 0; i < NUM_SHIPS; i++) {
                    cb_data->ui->ships_placed[i] = false;
                }

                // Reset ship placement widgets to null (they'll be recreated)
                for (int i = 0; i < NUM_SHIPS; i++) {
                    cb_data->ui->ship_status_labels[i] = nullptr;
                    cb_data->ui->ship_buttons[i] = nullptr;
                }
                cb_data->ui->ready_battle_button = nullptr;

                // Reset orientation and selection
                cb_data->ui->selected_ship_type = SHIP_CARRIER;
                cb_data->ui->current_orientation = HORIZONTAL;
                cb_data->ui->hover_row = -1;
                cb_data->ui->hover_col = -1;

                std::cout << "[UI] All state reset, transitioning to ship placement..." << std::endl;
                cb_data->ui->showScreen(SCREEN_SHIP_PLACEMENT);
            }
            delete cb_data;
            return G_SOURCE_REMOVE;
        }, new CallbackData{this, match.match_id, match.opponent_id, std::string(match.opponent_name)});
    });

    // Set up MATCH_READY callback to transition to game screen
    network->setMatchReadyCallback([this](const MatchStateMessage& state) {
        struct CallbackData {
            UIManager* ui;
            uint32_t current_turn_player_id;
        };

        g_idle_add(+[](gpointer data) -> gboolean {
            auto* cb_data = static_cast<CallbackData*>(data);
            std::cout << "[UI] 🎮 Both players ready! Starting match..." << std::endl;
            cb_data->ui->is_player_turn = (cb_data->current_turn_player_id == cb_data->ui->network->getUserId());
            cb_data->ui->showScreen(SCREEN_GAME);
            delete cb_data;
            return G_SOURCE_REMOVE;
        }, new CallbackData{this, state.current_turn_player_id});
    });

    // Set up MOVE_RESULT callback to handle shot results
    network->setMoveResultCallback([this](const MoveResultMessage& result) {
        struct CallbackData {
            UIManager* ui;
            MoveResultMessage msg;
        };

        g_idle_add(+[](gpointer data) -> gboolean {
            auto* cb_data = static_cast<CallbackData*>(data);
            UIManager* ui = cb_data->ui;

            if (!ui) {
                delete cb_data;
                return G_SOURCE_REMOVE;
            }

            uint32_t current_user_id = ui->network->getUserId();
            bool is_my_shot = (cb_data->msg.shooter_id == current_user_id);

            int row = static_cast<int>(cb_data->msg.target.row);
            int col = static_cast<int>(cb_data->msg.target.col);

            std::cout << "[UI] 📍 Move result: ";
            if (cb_data->msg.result == SHOT_MISS) {
                std::cout << "MISS at (" << row << "," << col << ")";
            } else if (cb_data->msg.result == SHOT_HIT) {
                std::cout << "HIT at (" << row << "," << col << ")!";
            } else if (cb_data->msg.result == SHOT_SUNK) {
                std::cout << "SUNK ship at (" << row << "," << col << ")!";
            }
            std::cout << " (shooter: " << cb_data->msg.shooter_id << ", is_my_shot: " << is_my_shot << ")" << std::endl;

            // Update the correct board based on who shot
            if (is_my_shot) {
                // This is my shot - update opponent's board (what I can see)
                if (ui->opponent_board) {
                    // Update cell state directly based on server result (don't use processShot)
                    // because opponent_board doesn't have ships, so processShot won't work correctly
                    int row = cb_data->msg.target.row;
                    int col = cb_data->msg.target.col;
                    
                    if (cb_data->msg.result == SHOT_MISS) {
                        ui->opponent_board->setCell(row, col, CELL_MISS);
                    } else if (cb_data->msg.result == SHOT_HIT) {
                        ui->opponent_board->setCell(row, col, CELL_HIT);
                    } else if (cb_data->msg.result == SHOT_SUNK) {
                        // Mark the entire sunk ship on opponent_board so the shooter sees the full ship
                        ui->opponent_board->setCell(row, col, CELL_SUNK);

                        // Infer ship orientation and extent from neighboring hits
                        int min_row = row, max_row = row;
                        int min_col = col, max_col = col;

                        bool horizontal = false;
                        bool vertical = false;
                        if (col > 0 && ui->opponent_board->getCell(row, col - 1) == CELL_HIT) horizontal = true;
                        if (col < BOARD_SIZE - 1 && ui->opponent_board->getCell(row, col + 1) == CELL_HIT) horizontal = true;
                        if (row > 0 && ui->opponent_board->getCell(row - 1, col) == CELL_HIT) vertical = true;
                        if (row < BOARD_SIZE - 1 && ui->opponent_board->getCell(row + 1, col) == CELL_HIT) vertical = true;

                        if (horizontal || !vertical) {
                            // Scan left
                            for (int c = col - 1; c >= 0; --c) {
                                CellState s = ui->opponent_board->getCell(row, c);
                                if (s == CELL_HIT) {
                                    ui->opponent_board->setCell(row, c, CELL_SUNK);
                                    min_col = c;
                                } else {
                                    break;
                                }
                            }
                            // Scan right
                            for (int c = col + 1; c < BOARD_SIZE; ++c) {
                                CellState s = ui->opponent_board->getCell(row, c);
                                if (s == CELL_HIT) {
                                    ui->opponent_board->setCell(row, c, CELL_SUNK);
                                    max_col = c;
                                } else {
                                    break;
                                }
                            }
                        }
                        if (vertical || !horizontal) {
                            // Scan up
                            for (int r = row - 1; r >= 0; --r) {
                                CellState s = ui->opponent_board->getCell(r, col);
                                if (s == CELL_HIT) {
                                    ui->opponent_board->setCell(r, col, CELL_SUNK);
                                    min_row = r;
                                } else {
                                    break;
                                }
                            }
                            // Scan down
                            for (int r = row + 1; r < BOARD_SIZE; ++r) {
                                CellState s = ui->opponent_board->getCell(r, col);
                                if (s == CELL_HIT) {
                                    ui->opponent_board->setCell(r, col, CELL_SUNK);
                                    max_row = r;
                                } else {
                                    break;
                                }
                            }
                        }

                        // Now add the ship object to opponent_board for visual rendering
                        ShipType ship_type = (ShipType)cb_data->msg.ship_sunk;
                        // Determine orientation based on extent in both directions
                        Orientation ship_orient = (max_col > min_col) ? HORIZONTAL :
                                                  (max_row > min_row) ? VERTICAL : HORIZONTAL;
                        Coordinate ship_pos;
                        ship_pos.row = min_row;
                        ship_pos.col = min_col;

                        // Place the sunk ship on opponent board (this will make it visible)
                        if (ui->opponent_board->placeShip(ship_type, ship_pos, ship_orient)) {
                            // Now mark it as sunk by hitting all its cells
                            int ship_length = getShipLength(ship_type);
                            for (int i = 0; i < ship_length; i++) {
                                Coordinate hit_pos;
                                hit_pos.row = ship_pos.row + (ship_orient == VERTICAL ? i : 0);
                                hit_pos.col = ship_pos.col + (ship_orient == HORIZONTAL ? i : 0);
                                ui->opponent_board->processShot(hit_pos); // This will mark ship as sunk
                            }
                            std::cout << "[UI] 💀 Revealed sunk " << shipTypeToName(ship_type)
                                     << " at (" << (int)ship_pos.row << "," << (int)ship_pos.col
                                     << ") " << (ship_orient == HORIZONTAL ? "HORIZONTAL" : "VERTICAL") << std::endl;
                        }
                    }
                    
                    // Update stats only for my shots
                    if (cb_data->msg.result == SHOT_HIT || cb_data->msg.result == SHOT_SUNK) {
                        ui->hits_count++;
                    }
                    ui->shots_fired++;
                    
                    // Redraw opponent board
                    if (ui->opponent_board_area) {
                        gtk_widget_queue_draw(ui->opponent_board_area);
                    }
                }
                
                // Handle turn logic based on result
                if (cb_data->msg.result == SHOT_MISS) {
                    // Missed - turn will switch to opponent
                    if (ui->fire_button) {
                        gtk_widget_set_sensitive(ui->fire_button, FALSE);
                    }
                    std::cout << "[UI] ⚠️ Missed! Turn will switch to opponent..." << std::endl;
                } else if (cb_data->msg.result == SHOT_HIT || cb_data->msg.result == SHOT_SUNK) {
                    // Hit or sunk - I continue my turn!
                    // Keep fire button enabled (TURN_UPDATE will confirm, but we can enable it now)
                    if (ui->fire_button) {
                        gtk_widget_set_sensitive(ui->fire_button, TRUE);
                    }
                    std::cout << "[UI] ✅ Hit! Continuing my turn - fire button enabled" << std::endl;
                }
            } else {
                // This is opponent's shot - update my board (where I was hit)
                if (ui->player_board) {
                    // Use processShot on player_board because it has ships
                    ui->player_board->processShot(cb_data->msg.target);
                    
                    // Redraw player board to show where opponent hit
                    if (ui->player_board_area) {
                        gtk_widget_queue_draw(ui->player_board_area);
                    }
                }
                
                // If opponent missed, turn will switch to me - wait for TURN_UPDATE
                if (cb_data->msg.result == SHOT_MISS) {
                    std::cout << "[UI] 🎯 Opponent missed! Turn will switch to me..." << std::endl;
                }
            }

            // Update stats display
            ui->updateGameStats();
            
            // Check for game over
            if (cb_data->msg.game_over) {
                std::cout << "[UI] 🏁 Game over! Winner: " << cb_data->msg.winner_id << std::endl;
                if (ui->fire_button) {
                    gtk_widget_set_sensitive(ui->fire_button, FALSE);
                }
            }

            delete cb_data;
            return G_SOURCE_REMOVE;
        }, new CallbackData{this, result});
    });

    // Set up TURN_UPDATE callback to handle turn changes
    network->setTurnUpdateCallback([this](const TurnUpdateMessage& turn) {
        struct CallbackData {
            UIManager* ui;
            TurnUpdateMessage msg;
        };

        g_idle_add(+[](gpointer data) -> gboolean {
            auto* cb_data = static_cast<CallbackData*>(data);
            UIManager* ui = cb_data->ui;

            if (ui) {
                ui->is_player_turn = (cb_data->msg.current_player_id == ui->network->getUserId());
                std::cout << "[UI] 🔄 Turn update: current_player=" << cb_data->msg.current_player_id 
                          << ", my_id=" << ui->network->getUserId()
                          << ", is_my_turn=" << (ui->is_player_turn ? "YES" : "NO") << std::endl;

                // Update turn indicator
                if (ui->turn_indicator) {
                    gtk_label_set_text(GTK_LABEL(ui->turn_indicator),
                                      ui->is_player_turn ? "YOUR TURN" : "OPPONENT'S TURN");
                }

                // Enable/disable fire button based on turn
                // IMPORTANT: Always update based on TURN_UPDATE, this is the source of truth
                if (ui->fire_button) {
                    gtk_widget_set_sensitive(ui->fire_button, ui->is_player_turn);
                    std::cout << "[UI] Fire button " << (ui->is_player_turn ? "ENABLED" : "DISABLED") << std::endl;
                }

                // Start turn timer only if it's my turn
                if (ui->is_player_turn) {
                    ui->startTurnTimer(cb_data->msg.time_left);
                } else {
                    // Stop timer if it's opponent's turn
                    ui->stopTurnTimer();
                }
            }

            delete cb_data;
            return G_SOURCE_REMOVE;
        }, new CallbackData{this, turn});
    });

    // Set up MATCH_END callback to handle game over
    network->setMatchEndCallback([this](const MatchEndMessage& end) {
        struct CallbackData {
            UIManager* ui;
            MatchEndMessage msg;
        };

        g_idle_add(+[](gpointer data) -> gboolean {
            auto* cb_data = static_cast<CallbackData*>(data);
            UIManager* ui = cb_data->ui;

            if (ui) {
                std::cout << "[UI] 🏁 Match ended! Winner: " << cb_data->msg.winner_id << std::endl;

                // Stop turn timer
                ui->stopTurnTimer();

                // Update ELO in network object (already done in handleMatchEnd, but ensure it's set)
                // The network object updates elo_rating_ when receiving MATCH_END

                // Determine result from player's perspective
                GameResult player_result;
                if (cb_data->msg.winner_id == 0) {
                    player_result = RESULT_DRAW;
                } else if (cb_data->msg.winner_id == ui->network->getUserId()) {
                    player_result = RESULT_WIN;
                } else {
                    player_result = RESULT_LOSS;
                }

                // Show result dialog with reason
                ui->showResultDialog(player_result, cb_data->msg.elo_change, cb_data->msg.reason_text);

                // After dialog, update lobby display with new ELO
                // The lobby screen will be shown by showResultDialog -> showScreen(LOBBY)
                // which will trigger a refresh
            }

            delete cb_data;
            return G_SOURCE_REMOVE;
        }, new CallbackData{this, end});
    });

    // Auto-request player list when screen loads
    g_idle_add(+[](gpointer data) -> gboolean {
        GtkButton* refresh_btn = GTK_BUTTON(data);
        g_signal_emit_by_name(refresh_btn, "clicked");
        return G_SOURCE_REMOVE;
    }, lobby->refresh_btn);

    return main_box;
}
