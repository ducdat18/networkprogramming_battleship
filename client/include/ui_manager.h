#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "game_state.h"
#include "animation.h"
#include "client_network.h"
#include <string>
#include <functional>

// Color scheme - Nautical Maritime Theme
struct ColorScheme {
    // Ocean colors
    static constexpr double OCEAN_DEEP_R = 0.0;
    static constexpr double OCEAN_DEEP_G = 0.31;
    static constexpr double OCEAN_DEEP_B = 0.42;  // #004F6B - Deep ocean

    static constexpr double OCEAN_LIGHT_R = 0.18;
    static constexpr double OCEAN_LIGHT_G = 0.54;
    static constexpr double OCEAN_LIGHT_B = 0.69; // #2E8AB0 - Light ocean

    static constexpr double OCEAN_FOAM_R = 0.53;
    static constexpr double OCEAN_FOAM_G = 0.81;
    static constexpr double OCEAN_FOAM_B = 0.92;  // #87CEEB - Sky blue foam

    // Navy colors
    static constexpr double NAVY_R = 0.0;
    static constexpr double NAVY_G = 0.13;
    static constexpr double NAVY_B = 0.27;        // #002145 - Dark navy

    static constexpr double NAVAL_BLUE_R = 0.02;
    static constexpr double NAVAL_BLUE_G = 0.62;
    static constexpr double NAVAL_BLUE_B = 0.99;  // #059FFD - Vivid cerulean

    // Ship colors
    static constexpr double SHIP_GRAY_R = 0.45;
    static constexpr double SHIP_GRAY_G = 0.48;
    static constexpr double SHIP_GRAY_B = 0.52;   // #737A85 - Battleship gray

    static constexpr double SHIP_DARK_R = 0.25;
    static constexpr double SHIP_DARK_G = 0.27;
    static constexpr double SHIP_DARK_B = 0.30;   // #40454D - Dark ship hull

    static constexpr double DECK_R = 0.58;
    static constexpr double DECK_G = 0.49;
    static constexpr double DECK_B = 0.42;        // #947D6B - Wood deck

    // Accent colors
    static constexpr double CORAL_R = 1.0;
    static constexpr double CORAL_G = 0.50;
    static constexpr double CORAL_B = 0.31;       // #FF7F50 - Coral

    static constexpr double SAND_R = 0.93;
    static constexpr double SAND_G = 0.87;
    static constexpr double SAND_B = 0.73;        // #EEDEBA - Sand

    static constexpr double WHITE_R = 1.0;
    static constexpr double WHITE_G = 1.0;
    static constexpr double WHITE_B = 1.0;        // #FFFFFF

    // Game state colors
    static constexpr double HIT_R = 0.85;
    static constexpr double HIT_G = 0.24;
    static constexpr double HIT_B = 0.11;         // #D93D1C - Fire red

    static constexpr double MISS_R = 0.67;
    static constexpr double MISS_G = 0.84;
    static constexpr double MISS_B = 0.90;        // #ABD6E6 - Water splash

    static constexpr double EXPLOSION_R = 1.0;
    static constexpr double EXPLOSION_G = 0.65;
    static constexpr double EXPLOSION_B = 0.0;    // #FFA500 - Orange explosion
};

// UI Screens
enum UIScreen {
    SCREEN_MAIN_MENU,
    SCREEN_LOGIN,
    SCREEN_REGISTER,
    SCREEN_LOBBY,
    SCREEN_SHIP_PLACEMENT,
    SCREEN_GAME,
    SCREEN_REPLAY,
    SCREEN_PROFILE
};

// UI Manager class
class UIManager {
public:
    GtkWidget* main_window;
    GtkWidget* current_screen;
    GtkWidget* header_bar;

    // Game screen widgets
    GtkWidget* player_board_area;
    GtkWidget* opponent_board_area;
    GtkWidget* chat_view;
    GtkWidget* status_label;
    GtkWidget* timer_label;
    GtkWidget* turn_indicator;

    // Board rendering
    cairo_surface_t* player_board_surface;
    cairo_surface_t* opponent_board_surface;

    // Game state
    Board* player_board;
    Board* opponent_board;
    MatchState* current_match;
    PlayerInfo current_player;
    PlayerInfo opponent_player;

    // Animation states
    guint animation_timer_id;
    double glow_pulse;
    double wave_offset;

    // Ship placement state
    ShipType selected_ship_type;
    Orientation current_orientation;
    bool ships_placed[NUM_SHIPS];
    GtkWidget* ship_status_labels[NUM_SHIPS];
    GtkWidget* ship_buttons[NUM_SHIPS];
    GtkWidget* ready_battle_button;
    int hover_row;
    int hover_col;

    // Game state
    bool is_player_turn;
    int shots_fired;
    int hits_count;
    GtkWidget* shots_label_widget;
    GtkWidget* hits_label_widget;
    GtkWidget* accuracy_label_widget;

    // Target selection (not auto-fire)
    int selected_target_row;
    int selected_target_col;
    bool has_target_selected;
    GtkWidget* fire_button;

    // Turn timer
    int turn_time_remaining;  // seconds
    guint turn_timer_id;
    GtkWidget* turn_timer_label;

    // Animations
    AnimationManager* animation_manager;

    // Game mode
    bool is_bot_mode;  // true = vs Bot, false = vs Online player

public:
    // Callbacks
    std::function<void(int, int)> on_cell_clicked;
    std::function<void(const std::string&)> on_chat_send;
    std::function<void()> on_resign;
    std::function<void()> on_draw_offer;
    UIManager();
    ~UIManager();

    // Window management
    void initialize(int argc, char* argv[]);
    void showScreen(UIScreen screen);
    void run();
    void quit();

    // Screen creation
    GtkWidget* createMainMenuScreen();
    GtkWidget* createLoginScreen();
    GtkWidget* createRegisterScreen();
    GtkWidget* createLobbyScreen();
    GtkWidget* createShipPlacementScreen();
    GtkWidget* createGameScreen();
    GtkWidget* createReplayScreen();
    GtkWidget* createProfileScreen();

    // Board rendering
    void renderPlayerBoard(cairo_t* cr);
    void renderOpponentBoard(cairo_t* cr);
    void drawBoard(cairo_t* cr, Board* board, bool is_player_board);
    void drawCell(cairo_t* cr, int row, int col, CellState state, bool is_player_board);
    void drawShip(cairo_t* cr, const Ship& ship);
    void drawGlowEffect(cairo_t* cr, double x, double y, double radius);
    void drawWaveBackground(cairo_t* cr, int width, int height);

    // UI updates
    void updatePlayerInfo(const PlayerInfo& info);
    void updateOpponentInfo(const PlayerInfo& info);
    void updateTurnIndicator(bool is_player_turn);
    void updateTimer(int seconds_remaining);
    void updateMatchStats(int shots_fired, int hits, double accuracy);
    void updateShipStatus(const Ship* ships, int count);

    // Chat
    void addChatMessage(const std::string& sender, const std::string& message, bool is_self);

    // Dialogs
    void showResultDialog(GameResult result, int elo_change);
    void showChallengeDialog(const std::string& opponent_name, int opponent_elo);
    void showErrorDialog(const std::string& message);
    void showNotification(const std::string& message);

    // Animation
    static gboolean animationCallback(gpointer data);
    void updateAnimations();

    // Setters
    void setPlayerBoard(Board* board) { player_board = board; }
    void setOpponentBoard(Board* board) { opponent_board = board; }
    void setCurrentMatch(MatchState* match) { current_match = match; }

    // Callback registration
    void setOnCellClicked(std::function<void(int, int)> callback) { on_cell_clicked = callback; }
    void setOnChatSend(std::function<void(const std::string&)> callback) { on_chat_send = callback; }
    void setOnResign(std::function<void()> callback) { on_resign = callback; }
    void setOnDrawOffer(std::function<void()> callback) { on_draw_offer = callback; }

    // Helper for draggable header
    static void makeDraggable(GtkWidget* widget, GtkWidget* window);

    // Ship placement helpers
    void selectShipForPlacement(ShipType type);
    void toggleShipOrientation();
    void placeShipAt(int row, int col);
    void randomPlaceAllShips();
    void updateShipStatusUI();
    bool allShipsPlaced();
    void clearAllShips();
    void clearShip(ShipType type);

    // Game flow helpers
    void switchTurn();
    void updateGameStats();
    void markSunkShip(int row, int col, ShipType type);

    // Bot AI
    void botTakeTurn();
    static gboolean botTurnCallback(gpointer data);

    // Target selection
    void selectTarget(int row, int col);
    void clearTargetSelection();
    bool executeFireAtTarget();

    // Turn timer
    void startTurnTimer(int seconds);
    void stopTurnTimer();
    static gboolean turnTimerCallback(gpointer data);
    void onTurnTimerExpired();

    // Network
    ClientNetwork* network;
    void connectToServer(const std::string& host, int port);
    void handleRegisterResponse(bool success, uint32_t user_id, const std::string& error);
    void handleLoginResponse(bool success, uint32_t user_id, const std::string& display_name,
                            int32_t elo_rating, const std::string& session_token, const std::string& error);
    void handleLogoutResponse(bool success);
    void showErrorDialog(const std::string& title, const std::string& message);
    void showInfoDialog(const std::string& title, const std::string& message);

    // Login/Register screen widgets (need to access in callbacks)
    GtkWidget* login_username_entry;
    GtkWidget* login_password_entry;
    GtkWidget* register_username_entry;
    GtkWidget* register_password_entry;
    GtkWidget* register_display_name_entry;
    GtkWidget* register_confirm_entry;
};

// GTK signal handlers (must be extern "C")
extern "C" {
    gboolean on_board_draw(GtkWidget* widget, cairo_t* cr, gpointer data);
    gboolean on_board_button_press(GtkWidget* widget, GdkEventButton* event, gpointer data);
    gboolean on_board_motion_notify(GtkWidget* widget, GdkEventMotion* event, gpointer data);
    void on_login_clicked(GtkButton* button, gpointer data);
    void on_register_clicked(GtkButton* button, gpointer data);
    void on_challenge_send_clicked(GtkButton* button, gpointer data);
    void on_chat_send_clicked(GtkButton* button, gpointer data);
    void on_fire_clicked(GtkButton* button, gpointer data);
    void on_resign_clicked(GtkButton* button, gpointer data);
    void on_draw_offer_clicked(GtkButton* button, gpointer data);
    void on_pause_clicked(GtkButton* button, gpointer data);
    void on_rematch_clicked(GtkButton* button, gpointer data);
}

#endif // UI_MANAGER_H
