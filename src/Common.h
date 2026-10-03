// Common.h - Gemeinsame Grundtypen und kleine Hilfsfunktionen für das ganze Spiel
#pragma once // Diese Datei nur ein einziges Mal pro Übersetzungseinheit einbinden

#include <algorithm> // std::min und std::max
#include <cmath>     // Mathematische Funktionen wie std::sin, std::sqrt
#include <cstdint>   // Ganzzahltypen mit fester Breite (std::uint32_t)

using Color = std::uint32_t; // Eine Farbe wird als 32-Bit-Zahl im Format 0xAARRGGBB gespeichert

constexpr float PI = 3.14159265358979f; // Kreiszahl Pi für Winkelberechnungen

// Baut aus den Kanälen Rot, Grün, Blau und Alpha (je 0..255) eine Farbe zusammen
constexpr Color rgba(int r, int g, int b, int a = 255) { // Standardmäßig ist die Farbe voll deckend
    return (static_cast<Color>(a & 255) << 24) | // Alpha in die obersten 8 Bit schieben
           (static_cast<Color>(r & 255) << 16) | // Rot in die Bits 16..23 schieben
           (static_cast<Color>(g & 255) << 8) |  // Grün in die Bits 8..15 schieben
           static_cast<Color>(b & 255);          // Blau bleibt in den untersten 8 Bit
} // Ende von rgba

constexpr Color TRANSPARENT = 0x00000000u; // Vollständig durchsichtige Farbe

inline int alphaOf(Color c) { return static_cast<int>((c >> 24) & 255); } // Alpha-Kanal einer Farbe lesen
inline int redOf(Color c) { return static_cast<int>((c >> 16) & 255); }   // Rot-Kanal einer Farbe lesen
inline int greenOf(Color c) { return static_cast<int>((c >> 8) & 255); }  // Grün-Kanal einer Farbe lesen
inline int blueOf(Color c) { return static_cast<int>(c & 255); }          // Blau-Kanal einer Farbe lesen

// Begrenzt einen Wert auf den Bereich [lo, hi]
template <typename T>                         // Funktioniert für int, float usw.
inline T clampValue(T v, T lo, T hi) {        // v = Wert, lo = Untergrenze, hi = Obergrenze
    return v < lo ? lo : (v > hi ? hi : v);   // Zu kleine Werte auf lo, zu große auf hi setzen
} // Ende von clampValue

// Lineare Interpolation zwischen a und b (t = 0 ergibt a, t = 1 ergibt b)
inline float lerp(float a, float b, float t) { return a + (b - a) * t; } // Formel der linearen Interpolation

// Bewegt einen Wert um höchstens "delta" in Richtung Zielwert
inline float approach(float value, float target, float delta) { // Für sanftes Beschleunigen/Abbremsen
    if (value < target) return std::min(value + delta, target);  // Von unten an das Ziel heranführen
    return std::max(value - delta, target);                      // Von oben an das Ziel heranführen
} // Ende von approach

// Mischt zwei Farben: t = 0 ergibt a, t = 1 ergibt b
inline Color mixColor(Color a, Color b, float t) { // Wird für Farbverläufe benutzt
    t = clampValue(t, 0.0f, 1.0f);                                                // t auf 0..1 begrenzen
    int r = static_cast<int>(lerp(static_cast<float>(redOf(a)), static_cast<float>(redOf(b)), t));       // Rot mischen
    int g = static_cast<int>(lerp(static_cast<float>(greenOf(a)), static_cast<float>(greenOf(b)), t));   // Grün mischen
    int bl = static_cast<int>(lerp(static_cast<float>(blueOf(a)), static_cast<float>(blueOf(b)), t));    // Blau mischen
    int al = static_cast<int>(lerp(static_cast<float>(alphaOf(a)), static_cast<float>(alphaOf(b)), t));  // Alpha mischen
    return rgba(r, g, bl, al);                                                    // Gemischte Farbe zurückgeben
} // Ende von mixColor

// Hellt eine Farbe auf (factor > 1) oder dunkelt sie ab (factor < 1), Alpha bleibt erhalten
inline Color shade(Color c, float factor) { // Für Schattierungen der Platzhalter-Grafiken
    int r = clampValue(static_cast<int>(static_cast<float>(redOf(c)) * factor), 0, 255);   // Rot skalieren
    int g = clampValue(static_cast<int>(static_cast<float>(greenOf(c)) * factor), 0, 255); // Grün skalieren
    int b = clampValue(static_cast<int>(static_cast<float>(blueOf(c)) * factor), 0, 255);  // Blau skalieren
    return rgba(r, g, b, alphaOf(c));                                                     // Neue Farbe zurückgeben
} // Ende von shade

// Ändert nur den Alpha-Kanal einer Farbe
inline Color withAlpha(Color c, int a) { return (c & 0x00FFFFFFu) | (static_cast<Color>(a & 255) << 24); } // Alpha ersetzen

// Rechteck mit Fließkomma-Koordinaten (für Spielwelt in Kachel-Einheiten)
struct RectF {     // Beginn der Struktur RectF
    float x = 0;   // Linke Kante
    float y = 0;   // Obere Kante
    float w = 0;   // Breite
    float h = 0;   // Höhe
    float right() const { return x + w; }  // Rechte Kante berechnen
    float bottom() const { return y + h; } // Untere Kante berechnen
    bool intersects(const RectF& o) const { // Prüft, ob sich zwei Rechtecke echt überlappen
        return x < o.x + o.w && x + w > o.x && y < o.y + o.h && y + h > o.y; // Überlappung auf beiden Achsen nötig
    } // Ende von intersects
}; // Ende der Struktur RectF

// Rechteck mit ganzzahligen Koordinaten (für Bildschirm-Pixel)
struct RectI {   // Beginn der Struktur RectI
    int x = 0;   // Linke Kante in Pixel
    int y = 0;   // Obere Kante in Pixel
    int w = 0;   // Breite in Pixel
    int h = 0;   // Höhe in Pixel
    bool contains(int px, int py) const { // Prüft, ob ein Punkt (z.B. der Mauszeiger) im Rechteck liegt
        return px >= x && py >= y && px < x + w && py < y + h; // Punkt muss innerhalb aller vier Kanten liegen
    } // Ende von contains
}; // Ende der Struktur RectI

// Einfacher deterministischer Zufallsgenerator, damit Platzhalter bei jedem Start gleich aussehen
class SimpleRandom {                       // Beginn der Klasse SimpleRandom
public:                                    // Öffentliche Funktionen
    explicit SimpleRandom(std::uint32_t seed) : m_state(seed ? seed : 1u) {} // Startwert setzen (nie 0)
    std::uint32_t next() {                 // Liefert die nächste Zufallszahl
        m_state ^= m_state << 13;          // Xorshift-Schritt 1
        m_state ^= m_state >> 17;          // Xorshift-Schritt 2
        m_state ^= m_state << 5;           // Xorshift-Schritt 3
        return m_state;                    // Neuen Zustand als Zufallszahl zurückgeben
    } // Ende von next
    float nextFloat() { return static_cast<float>(next() % 100000u) / 100000.0f; } // Zufallszahl zwischen 0 und 1
    float range(float lo, float hi) { return lo + (hi - lo) * nextFloat(); }       // Zufallszahl zwischen lo und hi
    int rangeInt(int lo, int hi) { return lo + static_cast<int>(next() % static_cast<std::uint32_t>(hi - lo + 1)); } // Ganzzahl in [lo, hi]
private:                                   // Interne Daten
    std::uint32_t m_state;                 // Aktueller Zustand des Generators
}; // Ende der Klasse SimpleRandom
