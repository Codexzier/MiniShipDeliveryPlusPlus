// Font.h - Eingebaute Pixel-Schriftart (5x9 Pixel pro Zeichen) mit Unterstützung für Umlaute
#pragma once // Header nur einmal einbinden

#include <cstdint> // std::uint32_t für Unicode-Zeichennummern
#include <string>  // std::string für Texte
#include <vector>  // std::vector für Zeilenlisten

#include "Graphics.h" // Canvas zum Zeichnen

namespace Font {                                   // Alle Schrift-Funktionen liegen im Namensraum Font
constexpr int GLYPH_WIDTH = 5;                     // Breite eines Zeichens in Pixel (unskaliert)
constexpr int GLYPH_HEIGHT = 9;                    // Höhe eines Zeichens inklusive Unterlänge
constexpr int ADVANCE = 6;                         // Abstand von Zeichen zu Zeichen (Breite + 1 Pixel Lücke)
constexpr int LINE_HEIGHT = 10;                    // Abstand von Zeile zu Zeile

std::vector<std::uint32_t> decodeUtf8(const std::string& text); // UTF-8-Text in Unicode-Zeichennummern zerlegen
void drawText(Canvas& canvas, int x, int y, const std::string& text, Color color, int scale); // Text zeichnen
void drawTextShadow(Canvas& canvas, int x, int y, const std::string& text, Color color, Color shadow, int scale); // Text mit Schatten
int textWidth(const std::string& text, int scale); // Breite eines Textes in Pixel
int lineHeight(int scale);                         // Zeilenhöhe in Pixel
std::vector<std::string> wrap(const std::string& text, int maxWidth, int scale); // Text in Zeilen umbrechen
} // Ende des Namensraums Font
