// Ui.h - Einfache Bedienelemente: Knöpfe, Fenster (Panels), Tooltips und Popup-Nachrichten
#pragma once // Header nur einmal einbinden

#include <string> // std::string
#include <vector> // std::vector

#include "Graphics.h" // Canvas

// Ein anklickbarer Knopf
struct Button {                 // Beginn der Struktur
    RectI rect;                 // Position und Größe in Pixel
    std::string label;          // Beschriftung
    bool enabled = true;        // Ist der Knopf benutzbar?
}; // Ende der Struktur Button

// Eine Zeile in einem Tooltip
struct TooltipLine {            // Beginn der Struktur
    std::string text;           // Text der Zeile
    Color color;                // Farbe der Zeile
}; // Ende der Struktur TooltipLine

namespace Ui {                                              // Gemeinsame Farben und Zeichenfunktionen
constexpr Color PANEL = rgba(28, 32, 50, 240);              // Hintergrund von Fenstern
constexpr Color PANEL_BORDER = rgba(230, 195, 110);         // Rahmen von Fenstern
constexpr Color TEXT = rgba(245, 245, 250);                 // Normale Schrift
constexpr Color TEXT_DIM = rgba(150, 155, 175);             // Ausgegraute Schrift
constexpr Color GOLD = rgba(255, 210, 70);                  // Goldene Schrift (Namen, Preise)
constexpr Color RED = rgba(255, 110, 100);                  // Rote Schrift (Warnungen)
constexpr Color GREEN = rgba(120, 230, 130);                // Grüne Schrift (Erfolg)
constexpr Color SHADOW = rgba(10, 10, 20);                  // Schatten hinter Text

void drawPanel(Canvas& canvas, const RectI& rect, int scale);                     // Fenster mit Rahmen
void drawButton(Canvas& canvas, const Button& button, bool hovered, int scale);   // Knopf
void drawTooltip(Canvas& canvas, int mouseX, int mouseY, const std::vector<TooltipLine>& lines, int scale); // Tooltip neben der Maus
void drawCenteredText(Canvas& canvas, int centerX, int y, const std::string& text, Color color, int scale); // Zentrierter Text
} // Ende des Namensraums Ui

// Popup-Nachricht mit Titel, Text und Knöpfen (z.B. "Spiel wirklich beenden?")
class Popup {                                                                        // Beginn der Klasse Popup
public:                                                                              // Öffentliche Schnittstelle
    void open(const std::string& title, const std::string& message, const std::vector<std::string>& buttons, int screenW, int screenH, int scale); // Öffnen
    void close() { m_open = false; }                                                 // Schließen
    bool isOpen() const { return m_open; }                                           // Ist das Popup sichtbar?
    int hit(int mouseX, int mouseY) const;                                           // Welcher Knopf liegt unter der Maus? (-1 = keiner)
    void moveSelection(int delta);                                                   // Auswahl per Tastatur verschieben
    int selection() const { return m_selection; }                                    // Per Tastatur ausgewählter Knopf
    void draw(Canvas& canvas, int mouseX, int mouseY, int scale) const;              // Zeichnen
private:                                                                             // Interne Daten
    bool m_open = false;                                                             // Sichtbar?
    std::string m_title;                                                             // Überschrift
    std::vector<std::string> m_lines;                                                // Umgebrochene Textzeilen
    std::vector<Button> m_buttons;                                                   // Knöpfe
    RectI m_rect;                                                                    // Position des Fensters
    int m_selection = 0;                                                             // Tastaturauswahl
}; // Ende der Klasse Popup
