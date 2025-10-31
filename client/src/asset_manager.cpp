#include "asset_manager.h"
#include <iostream>

AssetManager* AssetManager::instance = nullptr;

AssetManager::AssetManager() {
    // Initialize asset paths mapping
    asset_paths[ASSET_SHIP_CARRIER] = "assets/icons/Carrier/ShipCarrierHull.png";
    asset_paths[ASSET_SHIP_BATTLESHIP] = "assets/icons/Battleship/ShipBattleshipHull.png";
    asset_paths[ASSET_SHIP_CRUISER] = "assets/icons/Cruiser/ShipCruiserHull.png";
    asset_paths[ASSET_SHIP_DESTROYER] = "assets/icons/Destroyer/ShipDestroyerHull.png";
    asset_paths[ASSET_SHIP_SUBMARINE] = "assets/icons/Submarine/ShipSubMarineHull.png";
    asset_paths[ASSET_SHIP_PATROL] = "assets/icons/PatrolBoat/ShipPatrolHull.png";
    asset_paths[ASSET_SHIP_RESCUE] = "assets/icons/Rescue Ship/ShipRescue.png";

    asset_paths[ASSET_WEAPON_BATTLESHIP] = "assets/icons/Battleship/WeaponBattleshipStandardGun.png";
    asset_paths[ASSET_WEAPON_CRUISER] = "assets/icons/Cruiser/WeaponCruiserStandardSTSM.png";
    asset_paths[ASSET_WEAPON_DESTROYER] = "assets/icons/Destroyer/WeaponDestroyerStandardGun.png";
    asset_paths[ASSET_WEAPON_SUBMARINE] = "assets/icons/Submarine/WeaponSubmarineStandard.png";

    asset_paths[ASSET_EXPLOSION] = "assets/icons/Explosion03.png";
    asset_paths[ASSET_PLANE] = "assets/icons/Plane/PlaneF-35Lightning2.png";
    asset_paths[ASSET_MISSILE] = "assets/icons/Plane/Missile.png";
    asset_paths[ASSET_DISPLAY] = "assets/icons/Display.png";
    asset_paths[ASSET_SPLASH] = "assets/icons/splash/splash.png";
}

AssetManager::~AssetManager() {
    cleanup();
}

AssetManager* AssetManager::getInstance() {
    if (!instance) {
        instance = new AssetManager();
    }
    return instance;
}

bool AssetManager::loadAsset(AssetType type, const std::string& path) {
    cairo_surface_t* surface = cairo_image_surface_create_from_png(path.c_str());

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        std::cerr << "Failed to load asset: " << path << std::endl;
        std::cerr << "Cairo error: " << cairo_status_to_string(cairo_surface_status(surface)) << std::endl;
        cairo_surface_destroy(surface);
        return false;
    }

    surfaces[type] = surface;
    std::cout << "✅ Loaded asset: " << path << std::endl;
    return true;
}

bool AssetManager::loadAllAssets() {
    std::cout << "🎨 Loading all assets..." << std::endl;

    bool all_loaded = true;
    for (const auto& pair : asset_paths) {
        if (!loadAsset(pair.first, pair.second)) {
            all_loaded = false;
            std::cerr << "❌ Failed to load asset type: " << pair.first << std::endl;
        }
    }

    if (all_loaded) {
        std::cout << "✅ All assets loaded successfully!" << std::endl;
    } else {
        std::cerr << "⚠️ Some assets failed to load" << std::endl;
    }

    return all_loaded;
}

cairo_surface_t* AssetManager::getAsset(AssetType type) {
    auto it = surfaces.find(type);
    if (it != surfaces.end()) {
        return it->second;
    }
    return nullptr;
}

cairo_surface_t* AssetManager::getShipAsset(ShipType ship_type) {
    switch (ship_type) {
        case SHIP_CARRIER:
            return getAsset(ASSET_SHIP_CARRIER);
        case SHIP_BATTLESHIP:
            return getAsset(ASSET_SHIP_BATTLESHIP);
        case SHIP_CRUISER:
            return getAsset(ASSET_SHIP_CRUISER);
        case SHIP_SUBMARINE:
            return getAsset(ASSET_SHIP_SUBMARINE);
        case SHIP_DESTROYER:
            return getAsset(ASSET_SHIP_DESTROYER);
        default:
            return nullptr;
    }
}

cairo_surface_t* AssetManager::getWeaponAsset(ShipType ship_type) {
    switch (ship_type) {
        case SHIP_BATTLESHIP:
            return getAsset(ASSET_WEAPON_BATTLESHIP);
        case SHIP_CRUISER:
            return getAsset(ASSET_WEAPON_CRUISER);
        case SHIP_DESTROYER:
            return getAsset(ASSET_WEAPON_DESTROYER);
        case SHIP_SUBMARINE:
            return getAsset(ASSET_WEAPON_SUBMARINE);
        default:
            return nullptr;
    }
}

void AssetManager::drawAsset(cairo_t* cr, AssetType type, double x, double y,
                             double width, double height) {
    cairo_surface_t* surface = getAsset(type);
    if (!surface) return;

    cairo_save(cr);

    // Get original dimensions
    double orig_width = cairo_image_surface_get_width(surface);
    double orig_height = cairo_image_surface_get_height(surface);

    // If width/height not specified, use original size
    if (width < 0) width = orig_width;
    if (height < 0) height = orig_height;

    // Calculate scale
    double scale_x = width / orig_width;
    double scale_y = height / orig_height;

    // Apply transformation
    cairo_translate(cr, x, y);
    cairo_scale(cr, scale_x, scale_y);

    // Draw the image
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    cairo_restore(cr);
}

void AssetManager::drawShip(cairo_t* cr, ShipType ship_type, double x, double y,
                            double width, double height) {
    if (!cr) return;

    cairo_surface_t* surface = getShipAsset(ship_type);
    if (!surface) return;

    cairo_save(cr);

    // Get original dimensions
    double orig_width = cairo_image_surface_get_width(surface);
    double orig_height = cairo_image_surface_get_height(surface);

    // Validate dimensions
    if (orig_width <= 0 || orig_height <= 0 || width <= 0 || height <= 0) {
        cairo_restore(cr);
        return;
    }

    // Calculate scale to fit in the cell
    double scale_x = width / orig_width;
    double scale_y = height / orig_height;

    // Apply transformation
    cairo_translate(cr, x, y);
    cairo_scale(cr, scale_x, scale_y);

    // Draw the ship
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    cairo_restore(cr);
}

void AssetManager::drawExplosion(cairo_t* cr, double x, double y, double scale) {
    if (!cr) return;

    cairo_surface_t* surface = getAsset(ASSET_EXPLOSION);
    if (!surface) return;

    cairo_save(cr);

    // Get original dimensions
    double orig_width = cairo_image_surface_get_width(surface);
    double orig_height = cairo_image_surface_get_height(surface);

    // Validate
    if (orig_width <= 0 || orig_height <= 0 || scale <= 0) {
        cairo_restore(cr);
        return;
    }

    // Center the explosion
    cairo_translate(cr, x - (orig_width * scale / 2), y - (orig_height * scale / 2));
    cairo_scale(cr, scale, scale);

    // Draw with some transparency for effect
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint_with_alpha(cr, 0.9);

    cairo_restore(cr);
}

void AssetManager::cleanup() {
    for (auto& pair : surfaces) {
        if (pair.second) {
            cairo_surface_destroy(pair.second);
            pair.second = nullptr;
        }
    }
    surfaces.clear();
    std::cout << "🧹 Assets cleaned up" << std::endl;
}
