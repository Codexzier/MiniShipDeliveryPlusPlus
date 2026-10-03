// Shop.h - Shop-Menü mit 4x4 Feldern, Tooltips (Name, Beschreibung, Preis) und Kaufen per Mausklick
#pragma once // Header nur einmal einbinden

#include <array>  // std::array für die 16 Felder
#include <string> // std::string
#include <vector> // std::vector

#include "Assets.h"   // Symbole der Gegenstände
#include "Graphics.h" // Canvas
#include "Items.h"    // Gegenstände und Inventar
#include "Level.h"    // ShopSpot
#include "Ui.h"       // Knöpfe

class ShopMenu {                                                                // Beginn der Klasse ShopMenu
public:                                                                         // Öffentliche Schnittstelle
    static constexpr int COLUMNS = 4;                                           // Felder nebeneinander
    static constexpr int ROWS = 4;                                              // Reihen
    enum class Action { None, Close, Buy };                                     // Ergebnis eines Mausklicks

    void layout(int screenW, int screenH, int topBarHeight, int scale);         // Positionen berechnen
    void open(const ShopSpot& shop);                                            // Shop öffnen
    void close() { m_open = false; }                                            // Shop schließen
    bool isOpen() const { return m_open; }                                      // Ist der Shop offen?
    Action click(int mouseX, int mouseY, std::string& itemId) const;            // Mausklick auswerten
    void draw(Canvas& canvas, const Assets& assets, const ItemDatabase& items, const Inventory& inventory, int coins, int mouseX, int mouseY) const; // Zeichnen
    void drawTooltip(Canvas& canvas, const ItemDatabase& items, const Inventory& inventory, int coins, int mouseX, int mouseY) const; // Tooltip zeichnen
private:                                                                        // Interne Daten
    int slotAt(int mouseX, int mouseY) const;                                   // Feld unter der Maus
    bool m_open = false;                                                        // Offen?
    std::string m_name;                                                         // Name des Shops
    std::vector<std::string> m_wares;                                           // Angebotene Gegenstände
    RectI m_panel;                                                              // Fensterposition
    std::array<RectI, COLUMNS * ROWS> m_slots;                                  // Die 16 Felder
    Button m_closeButton;                                                       // Knopf "Schließen"
    int m_s = 1;                                                                // Skalierung
}; // Ende der Klasse ShopMenu
