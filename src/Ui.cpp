// Ui.cpp - Zeichnen der Bedienelemente
#include "Ui.h" // Eigene Deklarationen

#include <algorithm> // std::max, std::min

#include "Font.h" // Schrift

namespace Ui { // Beginn des Namensraums

// Zeichnet ein Fenster mit Schatten und goldenem Rahmen
void drawPanel(Canvas& canvas, const RectI& r, int s) {                    // Beginn von drawPanel
    canvas.fillRect(r.x + 2 * s, r.y + 2 * s, r.w, r.h, rgba(0, 0, 0, 110)); // Schatten leicht versetzt
    canvas.fillRect(r.x, r.y, r.w, r.h, PANEL);                             // Hintergrund
    canvas.drawRect(r.x, r.y, r.w, r.h, PANEL_BORDER, s);                   // Rahmen außen
    canvas.drawRect(r.x + 2 * s, r.y + 2 * s, r.w - 4 * s, r.h - 4 * s, rgba(255, 255, 255, 30), s); // Feine Innenlinie
} // Ende von drawPanel

// Zeichnet einen Knopf (hervorgehoben, wenn die Maus darüber ist)
void drawButton(Canvas& canvas, const Button& b, bool hovered, int s) {    // Beginn von drawButton
    Color fill = !b.enabled ? rgba(60, 62, 75) : (hovered ? rgba(240, 150, 40) : rgba(50, 90, 160)); // Farbe je nach Zustand
    canvas.fillRect(b.rect.x + s, b.rect.y + s, b.rect.w, b.rect.h, rgba(0, 0, 0, 120)); // Schatten
    canvas.fillRect(b.rect.x, b.rect.y, b.rect.w, b.rect.h, fill);          // Knopffläche
    canvas.fillRect(b.rect.x, b.rect.y, b.rect.w, s, shade(fill, 1.3f));    // Helle Oberkante
    canvas.fillRect(b.rect.x, b.rect.y + b.rect.h - s, b.rect.w, s, shade(fill, 0.6f)); // Dunkle Unterkante
    canvas.drawRect(b.rect.x, b.rect.y, b.rect.w, b.rect.h, hovered && b.enabled ? GOLD : rgba(20, 20, 30), s); // Rahmen
    int textY = b.rect.y + (b.rect.h - 7 * s) / 2;                          // Text senkrecht mittig
    drawCenteredText(canvas, b.rect.x + b.rect.w / 2, textY, b.label, b.enabled ? TEXT : TEXT_DIM, s); // Beschriftung
} // Ende von drawButton

// Zeichnet einen Text waagerecht zentriert mit Schatten
void drawCenteredText(Canvas& canvas, int centerX, int y, const std::string& text, Color color, int s) { // Beginn von drawCenteredText
    int w = Font::textWidth(text, s);                                       // Textbreite
    Font::drawTextShadow(canvas, centerX - w / 2, y, text, color, SHADOW, s); // Text mittig zeichnen
} // Ende von drawCenteredText

// Zeichnet einen Tooltip rechts unterhalb der Maus (bleibt immer im Bild)
void drawTooltip(Canvas& canvas, int mx, int my, const std::vector<TooltipLine>& lines, int s) { // Beginn von drawTooltip
    if (lines.empty()) return;                                              // Nichts anzuzeigen
    int pad = 4 * s;                                                        // Innenabstand
    int w = 0;                                                              // Breite des breitesten Textes
    for (const TooltipLine& line : lines) w = std::max(w, Font::textWidth(line.text, s)); // Breiteste Zeile suchen
    int h = static_cast<int>(lines.size()) * Font::lineHeight(s);           // Höhe aller Zeilen
    RectI r{mx + 8 * s, my + 8 * s, w + 2 * pad, h + 2 * pad - 2 * s};      // Rechteck neben der Maus
    if (r.x + r.w > canvas.width()) r.x = mx - r.w - 4 * s;                 // Rechts kein Platz -> links von der Maus
    if (r.y + r.h > canvas.height()) r.y = canvas.height() - r.h - 2 * s;   // Unten kein Platz -> nach oben schieben
    r.x = std::max(0, r.x);                                                 // Nicht über den linken Rand
    r.y = std::max(0, r.y);                                                 // Nicht über den oberen Rand
    canvas.fillRect(r.x + s, r.y + s, r.w, r.h, rgba(0, 0, 0, 120));        // Schatten
    canvas.fillRect(r.x, r.y, r.w, r.h, rgba(15, 18, 30, 245));             // Hintergrund
    canvas.drawRect(r.x, r.y, r.w, r.h, PANEL_BORDER, s);                   // Rahmen
    int y = r.y + pad;                                                      // Erste Textzeile
    for (const TooltipLine& line : lines) {                                 // Alle Zeilen
        Font::drawText(canvas, r.x + pad, y, line.text, line.color, s);     // Zeile zeichnen
        y += Font::lineHeight(s);                                           // Nächste Zeile
    }                                                                       // Ende der Schleife
} // Ende von drawTooltip

} // Ende des Namensraums Ui

// Öffnet ein Popup und berechnet seine Größe aus Text und Knöpfen
void Popup::open(const std::string& title, const std::string& message, const std::vector<std::string>& buttons, int screenW, int screenH, int s) { // Beginn von open
    m_open = true;                                                          // Sichtbar machen
    m_title = title;                                                        // Überschrift merken
    int width = std::min(screenW - 20 * s, 210 * s);                        // Fensterbreite
    m_lines = Font::wrap(message, width - 20 * s, s);                       // Text umbrechen
    int buttonH = 16 * s;                                                   // Knopfhöhe
    int height = 8 * s + Font::lineHeight(s) + 6 * s + static_cast<int>(m_lines.size()) * Font::lineHeight(s) + 8 * s + buttonH + 8 * s; // Gesamthöhe
    m_rect = RectI{(screenW - width) / 2, (screenH - height) / 2, width, height}; // Mittig platzieren
    m_buttons.clear();                                                      // Alte Knöpfe entfernen
    int count = static_cast<int>(buttons.size());                           // Anzahl der Knöpfe
    int buttonW = 60 * s;                                                   // Knopfbreite
    int gap = 10 * s;                                                       // Abstand zwischen den Knöpfen
    int total = count * buttonW + (count - 1) * gap;                        // Gesamtbreite aller Knöpfe
    int bx = m_rect.x + (m_rect.w - total) / 2;                             // Erster Knopf (mittig)
    int by = m_rect.y + m_rect.h - 8 * s - buttonH;                         // Knöpfe unten im Fenster
    for (int i = 0; i < count; ++i) {                                       // Alle Knöpfe anlegen
        m_buttons.push_back(Button{RectI{bx + i * (buttonW + gap), by, buttonW, buttonH}, buttons[static_cast<std::size_t>(i)], true}); // Knopf speichern
    }                                                                       // Ende der Schleife
    m_selection = 0;                                                        // Erster Knopf ausgewählt
} // Ende von open

// Liefert den Knopf unter der Maus
int Popup::hit(int mx, int my) const {                                      // Beginn von hit
    if (!m_open) return -1;                                                 // Geschlossen -> nichts
    for (std::size_t i = 0; i < m_buttons.size(); ++i) {                    // Alle Knöpfe prüfen
        if (m_buttons[i].rect.contains(mx, my)) return static_cast<int>(i); // Treffer
    }                                                                       // Ende der Schleife
    return -1;                                                              // Kein Treffer
} // Ende von hit

// Verschiebt die Tastaturauswahl (mit Umlauf)
void Popup::moveSelection(int delta) {                                      // Beginn von moveSelection
    int count = static_cast<int>(m_buttons.size());                         // Anzahl der Knöpfe
    if (count == 0) return;                                                 // Keine Knöpfe
    m_selection = (m_selection + delta + count) % count;                    // Auswahl verschieben
} // Ende von moveSelection

// Zeichnet das Popup
void Popup::draw(Canvas& canvas, int mx, int my, int s) const {             // Beginn von draw
    if (!m_open) return;                                                    // Nur wenn sichtbar
    Ui::drawPanel(canvas, m_rect, s);                                       // Fenster
    int y = m_rect.y + 8 * s;                                               // Erste Zeile
    Ui::drawCenteredText(canvas, m_rect.x + m_rect.w / 2, y, m_title, Ui::GOLD, s); // Überschrift
    y += Font::lineHeight(s) + 6 * s;                                       // Abstand nach der Überschrift
    for (const std::string& line : m_lines) {                               // Alle Textzeilen
        Ui::drawCenteredText(canvas, m_rect.x + m_rect.w / 2, y, line, Ui::TEXT, s); // Zeile zeichnen
        y += Font::lineHeight(s);                                           // Nächste Zeile
    }                                                                       // Ende der Schleife
    int hovered = hit(mx, my);                                              // Knopf unter der Maus
    for (std::size_t i = 0; i < m_buttons.size(); ++i) {                    // Alle Knöpfe
        bool active = static_cast<int>(i) == hovered || (hovered < 0 && static_cast<int>(i) == m_selection); // Hervorheben?
        Ui::drawButton(canvas, m_buttons[i], active, s);                    // Knopf zeichnen
    }                                                                       // Ende der Schleife
} // Ende von draw
