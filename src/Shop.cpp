// Shop.cpp - Zeichnen und Bedienen des Shop-Menüs
#include "Shop.h" // Eigene Deklarationen

#include <algorithm> // std::max

#include "Font.h" // Schrift

// Berechnet die Position des Fensters und der 16 Felder
void ShopMenu::layout(int screenW, int screenH, int topBarHeight, int s) {  // Beginn von layout
    m_s = s;                                                                // Skalierung merken
    int slot = 22 * s;                                                      // Feldgröße
    int gap = 2 * s;                                                        // Abstand zwischen Feldern
    int grid = COLUMNS * slot + (COLUMNS - 1) * gap;                        // Breite und Höhe des Rasters
    int panelW = grid + 20 * s;                                             // Fensterbreite
    int panelH = 16 * s + grid + 13 * s + 14 * s + 5 * s;                   // Fensterhöhe (Titel, Raster, Hinweis, Knopf, Rand)
    int y = topBarHeight + 18 * s;                                          // Unter der Zeile für Meldungen beginnen
    if (y + panelH > screenH - 2 * s) y = std::max(topBarHeight, screenH - 2 * s - panelH); // Auf kleinen Bildschirmen nach oben schieben
    m_panel = RectI{(screenW - panelW) / 2, y, panelW, panelH};             // Fenster waagerecht mittig
    for (int i = 0; i < COLUMNS * ROWS; ++i) {                              // Alle Felder
        int col = i % COLUMNS;                                              // Spalte
        int row = i / COLUMNS;                                              // Reihe
        m_slots[static_cast<std::size_t>(i)] = RectI{m_panel.x + 10 * s + col * (slot + gap), m_panel.y + 16 * s + row * (slot + gap), slot, slot}; // Feldposition
    }                                                                       // Ende der Schleife
    int buttonW = 60 * s;                                                   // Knopfbreite
    m_closeButton = Button{RectI{m_panel.x + (panelW - buttonW) / 2, m_panel.y + 16 * s + grid + 13 * s, buttonW, 14 * s}, "Schließen", true}; // Knopf unten
} // Ende von layout

// Öffnet den Shop mit seinen Waren
void ShopMenu::open(const ShopSpot& shop) {                                 // Beginn von open
    m_open = true;                                                          // Sichtbar machen
    m_name = shop.name;                                                     // Namen übernehmen
    m_wares = shop.wares;                                                   // Waren übernehmen
    if (m_wares.size() > static_cast<std::size_t>(COLUMNS * ROWS)) m_wares.resize(COLUMNS * ROWS); // Höchstens 16 Waren
} // Ende von open

// Liefert das Feld unter der Maus
int ShopMenu::slotAt(int mx, int my) const {                                // Beginn von slotAt
    for (int i = 0; i < COLUMNS * ROWS; ++i) {                              // Alle Felder prüfen
        if (m_slots[static_cast<std::size_t>(i)].contains(mx, my)) return i; // Treffer
    }                                                                       // Ende der Schleife
    return -1;                                                              // Kein Feld
} // Ende von slotAt

// Wertet einen Mausklick aus
ShopMenu::Action ShopMenu::click(int mx, int my, std::string& itemId) const { // Beginn von click
    if (!m_open) return Action::None;                                       // Geschlossen -> nichts
    if (m_closeButton.rect.contains(mx, my)) return Action::Close;          // Schließen-Knopf
    int index = slotAt(mx, my);                                             // Feld unter der Maus
    if (index >= 0 && index < static_cast<int>(m_wares.size())) {           // Feld mit Ware getroffen
        itemId = m_wares[static_cast<std::size_t>(index)];                  // Ware zurückgeben
        return Action::Buy;                                                 // Kaufwunsch
    }                                                                       // Ende der Prüfung
    if (!m_panel.contains(mx, my)) return Action::Close;                    // Klick neben das Fenster schließt den Shop
    return Action::None;                                                    // Sonst nichts
} // Ende von click

// Zeichnet das Shop-Fenster mit allen Waren
void ShopMenu::draw(Canvas& canvas, const Assets& assets, const ItemDatabase& items, const Inventory& inventory, int coins, int mx, int my) const { // Beginn von draw
    if (!m_open) return;                                                    // Nur wenn offen
    int s = m_s;                                                            // Skalierung
    Ui::drawPanel(canvas, m_panel, s);                                      // Fenster
    Ui::drawCenteredText(canvas, m_panel.x + m_panel.w / 2, m_panel.y + 5 * s, m_name, Ui::GOLD, s); // Titel
    int hovered = slotAt(mx, my);                                           // Feld unter der Maus
    for (int i = 0; i < COLUMNS * ROWS; ++i) {                              // Alle 16 Felder
        const RectI& r = m_slots[static_cast<std::size_t>(i)];              // Feldposition
        canvas.fillRect(r.x, r.y, r.w, r.h, rgba(12, 14, 24, 235));         // Dunkles Feld
        canvas.drawRect(r.x, r.y, r.w, r.h, i == hovered ? Ui::GOLD : rgba(90, 95, 120), s); // Rahmen
        if (i >= static_cast<int>(m_wares.size())) continue;                // Leeres Feld
        const ItemDef* def = items.find(m_wares[static_cast<std::size_t>(i)]); // Gegenstand nachschlagen
        if (!def) continue;                                                 // Unbekannte Ware überspringen
        const Image& icon = assets.get("symbol_" + PropertyFile::toLower(def->id)); // Symbol
        canvas.blit(icon, r.x + (r.w - icon.width) / 2, r.y + s);          // Symbol oben im Feld
        bool owned = def->unique && inventory.has(def->id);                 // Einzigartig und schon gekauft?
        std::string price = std::to_string(def->price);                     // Preis als Text
        Color priceColor = owned ? Ui::TEXT_DIM : (coins >= def->price ? Ui::GOLD : Ui::RED); // Farbe je nach Geldbeutel
        int pw = Font::textWidth(price, s);                                 // Breite des Preises
        canvas.fillRect(r.x + r.w - pw - 3 * s, r.y + r.h - 9 * s, pw + 2 * s, 8 * s, rgba(10, 10, 18, 220)); // Dunkles Preisschild
        Font::drawText(canvas, r.x + r.w - pw - 2 * s, r.y + r.h - 8 * s, price, priceColor, s); // Preis unten rechts
        if (owned) {                                                        // Schon gekauft: grüner Haken
            canvas.line(r.x + 3 * s, r.y + 6 * s, r.x + 5 * s, r.y + 8 * s, Ui::GREEN, s * 2); // Kurzer Strich des Hakens
            canvas.line(r.x + 5 * s, r.y + 8 * s, r.x + 9 * s, r.y + 3 * s, Ui::GREEN, s * 2); // Langer Strich des Hakens
        }                                                                   // Ende Haken
    }                                                                       // Ende der Feldschleife
    Ui::drawCenteredText(canvas, m_panel.x + m_panel.w / 2, m_slots[12].y + m_slots[12].h + 3 * s, "Klick: kaufen", Ui::TEXT_DIM, s); // Hinweis
    Ui::drawButton(canvas, m_closeButton, m_closeButton.rect.contains(mx, my), s); // Schließen-Knopf
} // Ende von draw

// Zeichnet den Tooltip der Ware unter der Maus: Name, Beschreibung und Preis
void ShopMenu::drawTooltip(Canvas& canvas, const ItemDatabase& items, const Inventory& inventory, int coins, int mx, int my) const { // Beginn von drawTooltip
    if (!m_open) return;                                                    // Nur wenn offen
    int index = slotAt(mx, my);                                             // Feld unter der Maus
    if (index < 0 || index >= static_cast<int>(m_wares.size())) return;     // Kein Feld mit Ware
    const ItemDef* def = items.find(m_wares[static_cast<std::size_t>(index)]); // Gegenstand nachschlagen
    if (!def) return;                                                       // Unbekannt
    int s = m_s;                                                            // Skalierung
    std::vector<TooltipLine> lines;                                         // Tooltip-Zeilen
    lines.push_back({def->name, Ui::GOLD});                                 // Name
    for (const std::string& l : Font::wrap(def->description, 150 * s, s)) lines.push_back({l, Ui::TEXT}); // Kurze Beschreibung
    lines.push_back({"Preis: " + std::to_string(def->price) + " Coins", Ui::GOLD}); // Kosten in Coins
    if (def->unique && inventory.has(def->id)) lines.push_back({"Bereits gekauft", Ui::GREEN}); // Schon im Besitz
    else if (coins < def->price) lines.push_back({"Zu wenig Coins", Ui::RED}); // Nicht genug Geld
    Ui::drawTooltip(canvas, mx, my, lines, s);                              // Tooltip zeichnen
} // Ende von drawTooltip
