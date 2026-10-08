// Ui.h - Bedienoberfläche mit den Grafiken des Kenney "Interface Pack" (Sofortmodus: zeichnen und klicken in einem Schritt)
#pragma once // Header nur einmal einbinden

#include <map>    // std::map für die geladenen Grafiken
#include <string> // std::string
#include <vector> // std::vector

#include "Graphics.h" // Canvas und Image

// Eingaben der Maus für ein Bild
struct UiInput {                     // Beginn der Struktur
    int mouseX = -1000;              // Mausposition x
    int mouseY = -1000;              // Mausposition y
    bool clicked = false;            // Linke Taste in diesem Bild gedrückt
    bool rightClicked = false;       // Rechte Taste in diesem Bild gedrückt
    int wheel = 0;                   // Mausrad (positiv = nach oben)
    bool consumed = false;           // Hat die Oberfläche den Klick schon verarbeitet?
}; // Ende der Struktur UiInput

// Arten von Fenstern
enum class PanelStyle { Paper, Wood, WoodPaper, Metal, MetalDark }; // Papier, Holz, Holz mit Papier, Metall hell/dunkel

// Farben der Fortschrittsbalken
enum class BarColor { Green, Red, Blue };                          // Grün, Rot, Blau

namespace UiColor {                                                // Häufig benutzte Textfarben
constexpr Color INK = rgba(70, 45, 25);                            // Dunkelbraune Schrift auf Papier
constexpr Color INK_LIGHT = rgba(130, 100, 70);                    // Helle Schrift auf Papier
constexpr Color WHITE = rgba(250, 248, 240);                       // Weiße Schrift
constexpr Color GOLD = rgba(255, 210, 80);                         // Gold
constexpr Color RED = rgba(200, 60, 50);                           // Rot
constexpr Color GREEN = rgba(60, 150, 70);                         // Grün
constexpr Color SHADOW = rgba(20, 15, 10, 200);                    // Schatten
} // Ende des Namensraums UiColor

class Ui {                                                                         // Beginn der Klasse Ui
public:                                                                            // Öffentliche Schnittstelle
    bool load(const std::string& interfaceDir);                                    // Grafiken aus "Interface Pack/PNG/Retina" laden
    void beginFrame(Canvas& canvas, UiInput& input);                               // Neues Bild beginnen
    void endFrame();                                                               // Tooltip zeichnen, Bild abschließen
    bool mouseOverUi(int x, int y) const;                                          // Liegt ein Punkt über einem Fenster des letzten Bildes?

    void panel(const RectI& r, PanelStyle style);                                  // Fenster zeichnen
    bool button(const RectI& r, const std::string& label, bool enabled = true, bool highlighted = false); // Knopf (true = geklickt)
    bool smallButton(const RectI& r, const std::string& label, bool enabled = true); // Kleiner runder Knopf
    void progress(const RectI& r, float value, BarColor color);                    // Fortschrittsbalken (0..1)
    void text(int x, int y, const std::string& t, Color c, int scale = 2);         // Text
    void textCentered(int cx, int y, const std::string& t, Color c, int scale = 2); // Zentrierter Text
    void textShadow(int x, int y, const std::string& t, Color c, int scale = 2);   // Text mit Schatten
    int textWrapped(int x, int y, int width, const std::string& t, Color c, int scale = 2); // Umbrochener Text, liefert Höhe
    void icon(const std::string& name, int cx, int cy, float angle = 0.0f);        // Symbol (zentriert, optional gedreht)
    void iconScaled(const std::string& name, int cx, int cy, int size);            // Symbol zentriert auf eine Größe skaliert
    void image(const Image& img, int x, int y);                                    // Beliebiges Bild
    void tooltip(const std::vector<std::string>& lines);                           // Tooltip an der Maus (am Ende des Bildes gezeichnet)
    bool hovered(const RectI& r) const;                                            // Maus über einem Rechteck?
    void addArea(const RectI& r) { m_panels.push_back(r); }                        // Bereich zur Oberfläche zählen (Klicks gehen nicht in die Welt)
    bool clickedIn(const RectI& r);                                                // Wurde in das Rechteck geklickt? (verbraucht den Klick)
    Canvas& canvas() { return *m_canvas; }                                         // Aktuelle Zeichenfläche
    UiInput& input() { return *m_input; }                                          // Aktuelle Eingaben
    static int textWidth(const std::string& t, int scale = 2);                     // Breite eines Textes

private:                                                                           // Interne Daten
    void nineSlice(const Image& img, const RectI& r, int margin);                  // 9-Teile-Skalierung eines Rahmenbildes
    const Image* get(const std::string& name) const;                               // Geladene Grafik suchen
    std::map<std::string, Image> m_images;                                         // Geladene Grafiken
    Canvas* m_canvas = nullptr;                                                    // Zeichenfläche des aktuellen Bildes
    UiInput* m_input = nullptr;                                                    // Eingaben des aktuellen Bildes
    std::vector<RectI> m_panels;                                                   // Fenster dieses Bildes
    std::vector<RectI> m_lastPanels;                                               // Fenster des letzten Bildes
    std::vector<std::string> m_tooltip;                                            // Tooltip-Zeilen
}; // Ende der Klasse Ui
