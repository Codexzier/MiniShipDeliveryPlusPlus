// Hud.h - Menüleiste oben (Beenden, Ausdauer, Leben, Coins, Uhrzeit) und Inventar unten links
#pragma once // Header nur einmal einbinden

#include <array>  // std::array für die Inventarfelder
#include <string> // std::string

#include "Assets.h"   // Symbole
#include "Graphics.h" // Canvas
#include "Items.h"    // Inventar und Gegenstände

class Hud {                                                                     // Beginn der Klasse Hud
public:                                                                         // Öffentliche Schnittstelle
    void layout(int screenW, int screenH, int scale);                           // Positionen aller Elemente berechnen
    int barHeight() const { return m_bar.h; }                                   // Höhe der Menüleiste
    void drawTopBar(Canvas& canvas, const Assets& assets, float health, float maxHealth, float stamina, float maxStamina, int coins, int mouseX, int mouseY) const; // Menüleiste zeichnen
    bool quitButtonHit(int mouseX, int mouseY) const { return m_quit.contains(mouseX, mouseY); } // Beenden-Knopf getroffen?
    void drawInventory(Canvas& canvas, const Assets& assets, const Inventory& inventory, int mouseX, int mouseY) const; // Inventar zeichnen
    int inventorySlotAt(int mouseX, int mouseY) const;                          // Feldnummer unter der Maus (-1 = keins)
    void drawTooltips(Canvas& canvas, const Inventory& inventory, const ItemDatabase& items, int mouseX, int mouseY) const; // Tooltips für Leiste und Inventar
    void showMessage(const std::string& text, float seconds = 2.5f);            // Kurze Meldung anzeigen
    void update(float dt);                                                      // Meldung altern lassen
    void drawMessage(Canvas& canvas) const;                                     // Meldung zeichnen
    void drawPrompt(Canvas& canvas, const std::string& text) const;             // Hinweis "E: ..." zeichnen
    static std::string clockText();                                             // Aktuelle Uhrzeit als Text
private:                                                                        // Interne Daten
    void drawBar(Canvas& canvas, const RectI& r, float value, float maxValue, Color fill) const; // Füllbalken zeichnen
    int m_w = 0;                                                                // Bildschirmbreite
    int m_h = 0;                                                                // Bildschirmhöhe
    int m_s = 1;                                                                // Skalierung der Oberfläche
    RectI m_bar;                                                                // Menüleiste
    RectI m_quit;                                                               // Beenden-Knopf ganz links
    RectI m_staminaIcon;                                                        // Blitz-Symbol
    RectI m_staminaBar;                                                         // Ausdauerbalken
    RectI m_healthIcon;                                                         // Herz-Symbol
    RectI m_healthBar;                                                          // Lebensbalken (Mitte)
    RectI m_inventoryPanel;                                                     // Inventarfenster
    std::array<RectI, Inventory::SIZE> m_slots;                                 // Inventarfelder
    std::string m_message;                                                      // Aktuelle Meldung
    float m_messageTime = 0.0f;                                                 // Restzeit der Meldung
}; // Ende der Klasse Hud
