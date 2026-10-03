// Hud.cpp - Zeichnen von Menüleiste, Inventar, Meldungen und Hinweisen
#include "Hud.h" // Eigene Deklarationen

#include <algorithm> // std::max, std::min
#include <cstdio>    // std::snprintf zum Formatieren von Zahlen
#include <ctime>     // std::time, std::localtime für die Uhrzeit

#include "Font.h" // Schrift
#include "Ui.h"   // Farben, Tooltips

// Berechnet die Positionen aller Elemente (alle Maße als Vielfaches der Skalierung s)
void Hud::layout(int screenW, int screenH, int s) {                         // Beginn von layout
    m_w = screenW;                                                          // Breite merken
    m_h = screenH;                                                          // Höhe merken
    m_s = s;                                                                // Skalierung merken
    m_bar = RectI{0, 0, screenW, 14 * s};                                   // Leiste über die ganze Breite
    m_quit = RectI{2 * s, s, 12 * s, 12 * s};                               // Beenden-Knopf ganz links
    m_staminaIcon = RectI{18 * s, 3 * s, 8 * s, 8 * s};                     // Blitz links
    m_staminaBar = RectI{28 * s, 3 * s - s / 2, 60 * s, 9 * s};             // Ausdauerbalken links
    int healthW = 80 * s;                                                   // Breite des Lebensbalkens
    int healthStart = screenW / 2 - (healthW + 10 * s) / 2;                 // Herz + Balken mittig ausrichten
    m_healthIcon = RectI{healthStart, 3 * s, 8 * s, 8 * s};                 // Herz in der Mitte
    m_healthBar = RectI{healthStart + 10 * s, 3 * s - s / 2, healthW, 9 * s}; // Lebensbalken in der Mitte
    int slot = 18 * s;                                                      // Größe eines Inventarfeldes
    int gap = 2 * s;                                                        // Abstand zwischen Feldern
    int pad = 3 * s;                                                        // Innenabstand des Fensters
    int panelW = 2 * pad + Inventory::COLUMNS * slot + (Inventory::COLUMNS - 1) * gap; // Fensterbreite (4 Felder)
    int panelH = 2 * pad + Inventory::ROWS * slot + (Inventory::ROWS - 1) * gap;       // Fensterhöhe (2 Reihen)
    m_inventoryPanel = RectI{4 * s, screenH - 4 * s - panelH, panelW, panelH}; // Fenster unten links
    for (int i = 0; i < Inventory::SIZE; ++i) {                             // Alle acht Felder
        int col = i % Inventory::COLUMNS;                                   // Spalte
        int row = i / Inventory::COLUMNS;                                   // Reihe
        m_slots[static_cast<std::size_t>(i)] = RectI{m_inventoryPanel.x + pad + col * (slot + gap), m_inventoryPanel.y + pad + row * (slot + gap), slot, slot}; // Feldposition
    }                                                                       // Ende der Schleife
} // Ende von layout

// Liefert die aktuelle Uhrzeit als "HH:MM:SS"
std::string Hud::clockText() {                                              // Beginn von clockText
    std::time_t now = std::time(nullptr);                                   // Aktuelle Zeit holen
    std::tm* local = std::localtime(&now);                                  // In lokale Zeit umrechnen
    char buffer[16];                                                        // Puffer für den Text
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec); // Formatieren
    return buffer;                                                          // Text zurückgeben
} // Ende von clockText

// Zeichnet einen Füllbalken mit Zahlenwert
void Hud::drawBar(Canvas& canvas, const RectI& r, float value, float maxValue, Color fill) const { // Beginn von drawBar
    int s = m_s;                                                            // Skalierung
    canvas.fillRect(r.x, r.y, r.w, r.h, rgba(15, 15, 25));                  // Dunkler Hintergrund
    float ratio = maxValue > 0.0f ? clampValue(value / maxValue, 0.0f, 1.0f) : 0.0f; // Füllstand 0..1
    int filled = static_cast<int>(static_cast<float>(r.w - 2 * s) * ratio); // Gefüllte Breite
    canvas.fillRect(r.x + s, r.y + s, filled, r.h - 2 * s, fill);           // Füllung
    canvas.fillRect(r.x + s, r.y + s, filled, std::max(1, s), shade(fill, 1.3f)); // Glanzkante
    canvas.drawRect(r.x, r.y, r.w, r.h, rgba(200, 200, 215), std::max(1, s / 2 + s % 2)); // Rahmen
    char text[32];                                                          // Puffer für den Zahlenwert
    std::snprintf(text, sizeof(text), "%d/%d", static_cast<int>(value + 0.5f), static_cast<int>(maxValue + 0.5f)); // z.B. "75/100"
    Ui::drawCenteredText(canvas, r.x + r.w / 2, r.y + (r.h - 7 * s) / 2, text, Ui::TEXT, s); // Zahl mittig im Balken
} // Ende von drawBar

// Zeichnet die Menüleiste oben
void Hud::drawTopBar(Canvas& canvas, const Assets& assets, float health, float maxHealth, float stamina, float maxStamina, int coins, int mx, int my) const { // Beginn von drawTopBar
    int s = m_s;                                                            // Skalierung
    canvas.fillRect(m_bar.x, m_bar.y, m_bar.w, m_bar.h, rgba(22, 26, 42, 235)); // Hintergrund der Leiste
    canvas.fillRect(m_bar.x, m_bar.y + m_bar.h - s, m_bar.w, s, Ui::PANEL_BORDER); // Goldene Linie unten
    bool hoverQuit = m_quit.contains(mx, my);                               // Maus über dem Beenden-Knopf?
    canvas.fillRect(m_quit.x, m_quit.y, m_quit.w, m_quit.h, hoverQuit ? rgba(240, 80, 70) : rgba(170, 45, 45)); // Roter Knopf
    canvas.drawRect(m_quit.x, m_quit.y, m_quit.w, m_quit.h, rgba(255, 220, 210), s); // Heller Rahmen
    int m = 3 * s;                                                          // Abstand des Kreuzes zum Rand
    canvas.line(m_quit.x + m, m_quit.y + m, m_quit.x + m_quit.w - m - 1, m_quit.y + m_quit.h - m - 1, Ui::TEXT, std::max(1, s + s / 2)); // Kreuz-Strich 1
    canvas.line(m_quit.x + m_quit.w - m - 1, m_quit.y + m, m_quit.x + m, m_quit.y + m_quit.h - m - 1, Ui::TEXT, std::max(1, s + s / 2)); // Kreuz-Strich 2
    canvas.blit(assets.get("hud_blitz"), m_staminaIcon.x, m_staminaIcon.y); // Blitz-Symbol
    drawBar(canvas, m_staminaBar, stamina, maxStamina, stamina < 15.0f ? rgba(230, 90, 50) : rgba(245, 190, 40)); // Ausdauerbalken
    canvas.blit(assets.get("hud_herz"), m_healthIcon.x, m_healthIcon.y);    // Herz-Symbol
    float ratio = maxHealth > 0.0f ? health / maxHealth : 0.0f;             // Lebensanteil
    Color healthColor = ratio > 0.5f ? rgba(80, 200, 90) : (ratio > 0.25f ? rgba(230, 200, 50) : rgba(230, 60, 50)); // Grün, gelb oder rot
    drawBar(canvas, m_healthBar, health, maxHealth, healthColor);           // Lebensbalken
    std::string clock = clockText();                                        // Uhrzeit
    int clockX = m_w - 4 * s - Font::textWidth(clock, s);                   // Uhrzeit ganz rechts
    Font::drawTextShadow(canvas, clockX, 4 * s - s / 2, clock, Ui::TEXT, Ui::SHADOW, s); // Uhrzeit zeichnen
    std::string coinText = std::to_string(coins);                           // Anzahl der Coins
    int coinX = clockX - 10 * s - Font::textWidth(coinText, s);             // Coins links neben der Uhrzeit
    canvas.blit(assets.get("hud_coin"), coinX - 10 * s, 3 * s);             // Münz-Symbol
    Font::drawTextShadow(canvas, coinX, 4 * s - s / 2, coinText, Ui::GOLD, Ui::SHADOW, s); // Anzahl zeichnen
} // Ende von drawTopBar

// Zeichnet das Inventar (4 Felder nebeneinander, 2 Reihen) unten links
void Hud::drawInventory(Canvas& canvas, const Assets& assets, const Inventory& inventory, int mx, int my) const { // Beginn von drawInventory
    int s = m_s;                                                            // Skalierung
    const RectI& p = m_inventoryPanel;                                      // Fensterposition
    canvas.fillRect(p.x, p.y, p.w, p.h, rgba(22, 26, 42, 200));             // Halbdurchsichtiger Hintergrund
    canvas.drawRect(p.x, p.y, p.w, p.h, Ui::PANEL_BORDER, s);               // Goldener Rahmen
    for (int i = 0; i < Inventory::SIZE; ++i) {                             // Alle Felder
        const RectI& r = m_slots[static_cast<std::size_t>(i)];              // Feldposition
        bool hovered = r.contains(mx, my);                                  // Maus über dem Feld?
        canvas.fillRect(r.x, r.y, r.w, r.h, rgba(12, 14, 24, 230));         // Dunkles Feld
        canvas.drawRect(r.x, r.y, r.w, r.h, hovered ? Ui::GOLD : rgba(90, 95, 120), s); // Rahmen (golden bei Maus)
        const InventorySlot& slot = inventory.slot(i);                      // Inhalt des Feldes
        if (slot.empty()) continue;                                         // Leeres Feld -> fertig
        std::string iconName = "symbol_" + PropertyFile::toLower(slot.itemId); // Name des Symbols
        const Image& icon = assets.get(iconName);                           // Symbol holen
        canvas.blit(icon, r.x + (r.w - icon.width) / 2, r.y + (r.h - icon.height) / 2); // Symbol mittig
        if (slot.count > 1) {                                               // Mehrere Stück?
            std::string count = std::to_string(slot.count);                 // Anzahl als Text
            Font::drawTextShadow(canvas, r.x + r.w - Font::textWidth(count, s) - s, r.y + r.h - 8 * s, count, Ui::TEXT, Ui::SHADOW, s); // Anzahl unten rechts
        }                                                                   // Ende Anzahl
    }                                                                       // Ende der Schleife
} // Ende von drawInventory

// Liefert die Nummer des Inventarfeldes unter der Maus
int Hud::inventorySlotAt(int mx, int my) const {                            // Beginn von inventorySlotAt
    for (int i = 0; i < Inventory::SIZE; ++i) {                             // Alle Felder prüfen
        if (m_slots[static_cast<std::size_t>(i)].contains(mx, my)) return i; // Treffer
    }                                                                       // Ende der Schleife
    return -1;                                                              // Kein Feld
} // Ende von inventorySlotAt

// Zeichnet Tooltips für den Beenden-Knopf und die Inventarfelder
void Hud::drawTooltips(Canvas& canvas, const Inventory& inventory, const ItemDatabase& items, int mx, int my) const { // Beginn von drawTooltips
    int s = m_s;                                                            // Skalierung
    if (m_quit.contains(mx, my)) {                                          // Maus über dem Beenden-Knopf
        Ui::drawTooltip(canvas, mx, my, {{"Spiel beenden", Ui::TEXT}}, s);  // Kurzer Hinweis
        return;                                                             // Fertig
    }                                                                       // Ende Beenden-Tooltip
    int index = inventorySlotAt(mx, my);                                    // Feld unter der Maus
    if (index < 0) return;                                                  // Kein Feld -> kein Tooltip
    const InventorySlot& slot = inventory.slot(index);                      // Inhalt des Feldes
    if (slot.empty()) return;                                               // Leeres Feld -> kein Tooltip
    const ItemDef* def = items.find(slot.itemId);                           // Gegenstand nachschlagen
    if (!def) return;                                                       // Unbekannt -> kein Tooltip
    std::vector<TooltipLine> lines;                                         // Tooltip-Zeilen
    lines.push_back({def->name, Ui::GOLD});                                 // Name
    for (const std::string& l : Font::wrap(def->description, 150 * s, s)) lines.push_back({l, Ui::TEXT}); // Beschreibung
    if (def->consumable) lines.push_back({"Klick oder Taste " + std::to_string(index + 1) + ": benutzen", Ui::GREEN}); // Benutzbar
    else if (def->effect == ItemEffect::Hook) lines.push_back({"Taste Q: Enterhaken werfen", Ui::GREEN}); // Enterhaken-Hinweis
    else lines.push_back({"Wirkt automatisch", Ui::TEXT_DIM});              // Passiver Gegenstand
    Ui::drawTooltip(canvas, mx, my, lines, s);                              // Tooltip zeichnen
} // Ende von drawTooltips

// Zeigt eine kurze Meldung unter der Menüleiste an
void Hud::showMessage(const std::string& text, float seconds) {             // Beginn von showMessage
    m_message = text;                                                       // Text merken
    m_messageTime = seconds;                                                // Anzeigedauer merken
} // Ende von showMessage

void Hud::update(float dt) { m_messageTime = std::max(0.0f, m_messageTime - dt); } // Restzeit der Meldung verringern

// Zeichnet die aktuelle Meldung (blendet am Ende aus)
void Hud::drawMessage(Canvas& canvas) const {                               // Beginn von drawMessage
    if (m_messageTime <= 0.0f || m_message.empty()) return;                 // Keine Meldung
    int s = m_s;                                                            // Skalierung
    int alpha = static_cast<int>(255.0f * std::min(1.0f, m_messageTime / 0.5f)); // Ausblenden in der letzten halben Sekunde
    std::vector<std::string> lines = Font::wrap(m_message, m_w - 24 * s, s); // Lange Meldungen auf mehrere Zeilen umbrechen
    int w = 0;                                                              // Breite der längsten Zeile
    for (const std::string& line : lines) w = std::max(w, Font::textWidth(line, s)); // Längste Zeile suchen
    int h = static_cast<int>(lines.size()) * Font::lineHeight(s) + 3 * s;   // Höhe aller Zeilen plus Rand
    RectI r{m_w / 2 - w / 2 - 5 * s, m_bar.h + 5 * s, w + 10 * s, h};       // Hintergrundkasten
    canvas.fillRect(r.x, r.y, r.w, r.h, rgba(20, 22, 35, alpha * 220 / 255)); // Kasten
    canvas.drawRect(r.x, r.y, r.w, r.h, withAlpha(Ui::PANEL_BORDER, alpha), s); // Rahmen
    int y = r.y + 3 * s;                                                    // Erste Textzeile
    for (const std::string& line : lines) {                                 // Alle Zeilen
        Font::drawText(canvas, r.x + 5 * s, y, line, withAlpha(Ui::TEXT, alpha), s); // Zeile zeichnen
        y += Font::lineHeight(s);                                           // Nächste Zeile
    }                                                                       // Ende der Schleife
} // Ende von drawMessage

// Zeichnet einen Hinweis unten in der Mitte (z.B. "E: Shop betreten")
void Hud::drawPrompt(Canvas& canvas, const std::string& text) const {       // Beginn von drawPrompt
    if (text.empty()) return;                                               // Kein Hinweis
    int s = m_s;                                                            // Skalierung
    int w = Font::textWidth(text, s);                                       // Textbreite
    RectI r{m_w / 2 - w / 2 - 5 * s, m_h - 22 * s, w + 10 * s, 13 * s};     // Hintergrundkasten
    canvas.fillRect(r.x, r.y, r.w, r.h, rgba(20, 22, 35, 220));             // Kasten
    canvas.drawRect(r.x, r.y, r.w, r.h, Ui::GOLD, s);                       // Goldener Rahmen
    Font::drawText(canvas, r.x + 5 * s, r.y + 3 * s, text, Ui::TEXT, s);    // Text
} // Ende von drawPrompt
