#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <gtk/gtk.h>
#include <cairo.h>
#include <map>
#include <string>
#include "game_state.h"

// Asset types
enum AssetType {
    ASSET_SHIP_CARRIER,
    ASSET_SHIP_BATTLESHIP,
    ASSET_SHIP_CRUISER,
    ASSET_SHIP_DESTROYER,
    ASSET_SHIP_SUBMARINE,
    ASSET_SHIP_PATROL,
    ASSET_SHIP_RESCUE,
    ASSET_WEAPON_BATTLESHIP,
    ASSET_WEAPON_CRUISER,
    ASSET_WEAPON_DESTROYER,
    ASSET_WEAPON_SUBMARINE,
    ASSET_EXPLOSION,
    ASSET_PLANE,
    ASSET_MISSILE,
    ASSET_DISPLAY,
    ASSET_SPLASH
};

/**
 * AssetManager - Manages loading and accessing PNG image assets
 *
 * This class loads all ship sprites, weapons, and effects from the assets folder
 * and provides easy access via Cairo surfaces for rendering.
 */
class AssetManager {
private:
    std::map<AssetType, cairo_surface_t*> surfaces;
    std::map<AssetType, std::string> asset_paths;

    // Singleton instance
    static AssetManager* instance;

    AssetManager();
    ~AssetManager();

    // Load a single PNG asset
    bool loadAsset(AssetType type, const std::string& path);

public:
    // Get singleton instance
    static AssetManager* getInstance();

    // Load all assets from disk
    bool loadAllAssets();

    // Get a loaded asset surface
    cairo_surface_t* getAsset(AssetType type);

    // Get ship asset based on ship type
    cairo_surface_t* getShipAsset(ShipType ship_type);

    // Get weapon asset based on ship type
    cairo_surface_t* getWeaponAsset(ShipType ship_type);

    // Draw an asset at position with optional scaling
    void drawAsset(cairo_t* cr, AssetType type, double x, double y,
                   double width = -1, double height = -1);

    // Draw ship at position
    void drawShip(cairo_t* cr, ShipType ship_type, double x, double y,
                  double width, double height);

    // Draw explosion with animation frame
    void drawExplosion(cairo_t* cr, double x, double y, double scale = 1.0);

    // Cleanup all loaded assets
    void cleanup();
};

#endif // ASSET_MANAGER_H
