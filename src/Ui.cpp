// Ui.cpp - Zeichnen von Fenstern, Knöpfen, Balken, Texten und Tooltips mit dem Interface Pack
#include "Ui.h" // Eigene Deklarationen

#include <algorithm> // std::max, std::min

#include "Font.h"    // Pixelschrift
#include "ImageIO.h" // PNG laden

// Lädt alle benötigten Grafiken des Interface Packs
bool Ui::load(const std::string& interfaceDir) {                             // Beginn von load
    const char* names[] = {                                                  // Benutzte Dateien (ohne Endung)
        "panel_woodPaper", "panel_wood", "panel_woodDetail", "panel_woodPaperDetail", "panel_metal", "panel_metalDark", // Fenster
        "button_rectangleWood", "button_rectangleMetal", "button_rectangleRed", "button_woodBlank", "button_metalBlank", // Knöpfe
        "progress_green", "progress_red", "progress_blue", "progress", "minimapMap_woodRing", "round_woodPaper", // Balken und Rundes
        "minimapIcon_arrowA", "minimapIcon_arrowB", "minimapIcon_starYellow", "minimapIcon_jewelYellow", "minimapIcon_jewelRed", // Symbole
        "minimapIcon_exclamationYellow", "minimapIcon_exclamationRed", "minimapIcon_starWhite", "minimapDirection_N", // Symbole
        "minimapDirection_E", "minimapDirection_S", "minimapDirection_W", "bannerScroll", "bannerHanging"};   // Himmelsrichtungen und Banner
    bool all = true;                                                         // Wurde alles gefunden?
    for (const char* n : names) {                                            // Alle Dateien
        Image img;                                                           // Bild
        if (ImageIO::loadImage(ImageIO::joinPath(interfaceDir, std::string(n) + ".png"), img)) m_images[n] = img; // Laden
        else all = false;                                                    // Fehlt
    }                                                                        // Ende der Schleife
    return all;                                                              // Ergebnis
} // Ende von load

// Sucht eine geladene Grafik
const Image* Ui::get(const std::string& name) const {                        // Beginn von get
    auto it = m_images.find(name);                                           // Suchen
    return it == m_images.end() ? nullptr : &it->second;                     // Zeiger oder nullptr
} // Ende von get

// Beginnt ein neues Bild
void Ui::beginFrame(Canvas& canvas, UiInput& input) {                        // Beginn von beginFrame
    m_canvas = &canvas;                                                      // Zeichenfläche merken
    m_input = &input;                                                        // Eingaben merken
    m_lastPanels = m_panels;                                                 // Fenster des letzten Bildes merken
    m_panels.clear();                                                        // Neue Liste beginnen
    m_tooltip.clear();                                                       // Tooltip zurücksetzen
} // Ende von beginFrame

// Beendet das Bild und zeichnet den Tooltip ganz oben
void Ui::endFrame() {                                                        // Beginn von endFrame
    if (m_tooltip.empty() || !m_canvas) return;                              // Kein Tooltip
    int w = 0;                                                               // Breite
    for (const std::string& l : m_tooltip) w = std::max(w, textWidth(l, 2)); // Breiteste Zeile
    int h = static_cast<int>(m_tooltip.size()) * Font::lineHeight(2);        // Höhe
    RectI r{m_input->mouseX + 16, m_input->mouseY + 16, w + 32, h + 24};     // Rechteck neben der Maus
    if (r.x + r.w > m_canvas->width()) r.x = m_input->mouseX - r.w - 8;      // Rechts kein Platz
    if (r.y + r.h > m_canvas->height()) r.y = m_canvas->height() - r.h - 4;  // Unten kein Platz
    r.x = std::max(0, r.x);                                                  // Nicht links hinaus
    r.y = std::max(0, r.y);                                                  // Nicht oben hinaus
    panel(r, PanelStyle::Paper);                                             // Hintergrund
    int y = r.y + 12;                                                        // Erste Zeile
    for (std::size_t i = 0; i < m_tooltip.size(); ++i) {                     // Alle Zeilen
        text(r.x + 16, y, m_tooltip[i], i == 0 ? rgba(150, 70, 30) : UiColor::INK, 2); // Erste Zeile hervorgehoben
        y += Font::lineHeight(2);                                            // Nächste Zeile
    }                                                                        // Ende der Schleife
} // Ende von endFrame

// Prüft, ob ein Punkt über einem Fenster des letzten Bildes liegt
bool Ui::mouseOverUi(int x, int y) const {                                   // Beginn von mouseOverUi
    for (const RectI& r : m_lastPanels) if (r.contains(x, y)) return true;   // Treffer
    return false;                                                            // Kein Fenster
} // Ende von mouseOverUi

// Zeichnet ein Bild in 9 Teilen: Ecken unverändert, Kanten und Mitte gestreckt
void Ui::nineSlice(const Image& img, const RectI& r, int m) {                // Beginn von nineSlice
    int sw = img.width, sh = img.height;                                     // Quellgröße
    m = std::min({m, r.w / 2, r.h / 2, sw / 2, sh / 2});                     // Rand nicht größer als das Bild
    const int sx[4] = {0, m, sw - m, sw};                                    // Quellgrenzen x
    const int sy[4] = {0, m, sh - m, sh};                                    // Quellgrenzen y
    const int dx[4] = {r.x, r.x + m, r.x + r.w - m, r.x + r.w};              // Zielgrenzen x
    const int dy[4] = {r.y, r.y + m, r.y + r.h - m, r.y + r.h};              // Zielgrenzen y
    for (int j = 0; j < 3; ++j) {                                            // Drei Streifen in y
        for (int i = 0; i < 3; ++i) {                                        // Drei Streifen in x
            RectI src{sx[i], sy[j], sx[i + 1] - sx[i], sy[j + 1] - sy[j]};   // Quellteil
            RectI dst{dx[i], dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j]};   // Zielteil
            if (src.w <= 0 || src.h <= 0 || dst.w <= 0 || dst.h <= 0) continue; // Leer
            m_canvas->blitScaled(img, src, dst);                             // Teil gestreckt zeichnen
        }                                                                    // Ende x
    }                                                                        // Ende y
} // Ende von nineSlice

// Zeichnet ein Fenster
void Ui::panel(const RectI& r, PanelStyle style) {                           // Beginn von panel
    const char* name = "panel_woodPaper";                                    // Standard
    switch (style) {                                                         // Je nach Art
    case PanelStyle::Paper: name = "panel_woodPaper"; break;                 // Papier mit Holzrand
    case PanelStyle::Wood: name = "panel_wood"; break;                       // Holz
    case PanelStyle::WoodPaper: name = "panel_woodPaperDetail"; break;       // Papier mit verziertem Rand
    case PanelStyle::Metal: name = "panel_metal"; break;                     // Metall hell
    case PanelStyle::MetalDark: name = "panel_metalDark"; break;             // Metall dunkel
    }                                                                        // Ende der Fallunterscheidung
    m_canvas->fillRect(r.x + 4, r.y + 6, r.w, r.h, rgba(0, 0, 0, 70));       // Schatten
    if (const Image* img = get(name)) nineSlice(*img, r, 22);                // Rahmenbild zeichnen
    else m_canvas->fillRect(r.x, r.y, r.w, r.h, rgba(240, 225, 190));        // Ersatz ohne Grafik
    m_panels.push_back(r);                                                   // Als Fenster merken
} // Ende von panel

// Maus über einem Rechteck?
bool Ui::hovered(const RectI& r) const { return m_input && r.contains(m_input->mouseX, m_input->mouseY); } // Prüfen

// Klick in ein Rechteck (verbraucht den Klick)
bool Ui::clickedIn(const RectI& r) {                                         // Beginn von clickedIn
    if (!m_input || !m_input->clicked || m_input->consumed || !hovered(r)) return false; // Kein Klick hier
    m_input->consumed = true;                                                // Klick verbraucht
    return true;                                                             // Geklickt
} // Ende von clickedIn

// Zeichnet einen Knopf und meldet einen Klick
bool Ui::button(const RectI& r, const std::string& label, bool enabled, bool highlighted) { // Beginn von button
    bool over = enabled && hovered(r);                                       // Maus darüber?
    const char* name = !enabled ? "button_rectangleMetal" : ((over || highlighted) ? "button_rectangleRed" : "button_rectangleWood"); // Aussehen
    if (const Image* img = get(name)) nineSlice(*img, r, 14);                // Knopfbild
    else m_canvas->fillRect(r.x, r.y, r.w, r.h, rgba(180, 120, 60));         // Ersatz
    int scale = textWidth(label, 2) <= r.w - 16 ? 2 : 1;                     // Schrift verkleinern, wenn der Text zu lang ist
    int tx = r.x + (r.w - textWidth(label, scale)) / 2, ty = r.y + (r.h - 7 * scale) / 2; // Beschriftung mittig
    if (!enabled) text(tx, ty, label, rgba(90, 90, 95), scale);              // Gesperrt: grau
    else if (over || highlighted) textShadow(tx, ty, label, UiColor::WHITE, scale); // Rot hinterlegt: weiß
    else text(tx, ty, label, UiColor::INK, scale);                           // Heller Holzknopf: dunkle Schrift
    m_panels.push_back(r);                                                   // Knopf gehört zur Oberfläche
    return enabled && clickedIn(r);                                          // Klick melden
} // Ende von button

// Kleiner runder Knopf (z.B. "+" und "-")
bool Ui::smallButton(const RectI& r, const std::string& label, bool enabled) { // Beginn von smallButton
    bool over = enabled && hovered(r);                                       // Maus darüber?
    if (const Image* img = get(enabled ? "button_woodBlank" : "button_metalBlank")) m_canvas->blitScaled(*img, RectI{0, 0, img->width, img->height}, r); // Knopfbild
    if (over) m_canvas->fillRect(r.x + 4, r.y + 4, r.w - 8, r.h - 8, rgba(255, 255, 255, 60)); // Aufhellen bei Maus
    textShadow(r.x + (r.w - textWidth(label, 2)) / 2, r.y + (r.h - 14) / 2, label, enabled ? UiColor::WHITE : rgba(90, 90, 95), 2); // Beschriftung
    m_panels.push_back(r);                                                   // Gehört zur Oberfläche
    return enabled && clickedIn(r);                                          // Klick melden
} // Ende von smallButton

// Zeichnet einen waagerechten Fortschrittsbalken
void Ui::progress(const RectI& r, float value, BarColor color) {             // Beginn von progress
    value = clampValue(value, 0.0f, 1.0f);                                   // Auf 0..1 begrenzen
    m_canvas->fillRect(r.x, r.y, r.w, r.h, rgba(60, 40, 25));                // Dunkler Hintergrund
    m_canvas->drawRect(r.x, r.y, r.w, r.h, rgba(30, 20, 10), 2);             // Rahmen
    const char* name = color == BarColor::Green ? "progress_green" : (color == BarColor::Red ? "progress_red" : "progress_blue"); // Balkenbild
    int fill = static_cast<int>(static_cast<float>(r.w - 6) * value);        // Gefüllte Breite
    if (fill <= 0) return;                                                   // Leer
    if (const Image* img = get(name)) {                                      // Bild vorhanden
        RectI dst{r.x + 3, r.y + 3, fill, r.h - 6};                          // Zielbereich
        RectI src{0, 0, img->width, img->height};                            // Ganzes Bild (senkrechter Balken)
        int cap = std::min(img->width / 2, dst.w / 2);                       // Abgerundete Enden
        m_canvas->blitScaled(*img, RectI{0, 0, img->width / 2, img->height}, RectI{dst.x, dst.y, cap, dst.h}); // Linkes Ende
        m_canvas->blitScaled(*img, RectI{img->width / 2, 0, 1, img->height}, RectI{dst.x + cap, dst.y, std::max(0, dst.w - 2 * cap), dst.h}); // Mitte
        m_canvas->blitScaled(*img, RectI{img->width - img->width / 2, 0, img->width / 2, img->height}, RectI{dst.x + dst.w - cap, dst.y, cap, dst.h}); // Rechtes Ende
        (void)src;                                                           // Nicht weiter benötigt
    } else {                                                                 // Ersatz
        m_canvas->fillRect(r.x + 3, r.y + 3, fill, r.h - 6, rgba(90, 190, 90)); // Einfacher Balken
    }                                                                        // Ende der Unterscheidung
} // Ende von progress

int Ui::textWidth(const std::string& t, int scale) { return Font::textWidth(t, scale); } // Textbreite

void Ui::text(int x, int y, const std::string& t, Color c, int scale) { Font::drawText(*m_canvas, x, y, t, c, scale); } // Text zeichnen

// Zentrierter Text
void Ui::textCentered(int cx, int y, const std::string& t, Color c, int scale) { // Beginn von textCentered
    Font::drawText(*m_canvas, cx - textWidth(t, scale) / 2, y, t, c, scale); // Halbe Breite nach links versetzt
} // Ende von textCentered

// Text mit Schatten (für helle Schrift auf bunten Hintergründen)
void Ui::textShadow(int x, int y, const std::string& t, Color c, int scale) { // Beginn von textShadow
    Font::drawTextShadow(*m_canvas, x, y, t, c, UiColor::SHADOW, scale);     // Mit dunklem Schatten
} // Ende von textShadow

// Umbrochener Text, liefert die benutzte Höhe
int Ui::textWrapped(int x, int y, int width, const std::string& t, Color c, int scale) { // Beginn von textWrapped
    std::vector<std::string> lines = Font::wrap(t, width, scale);            // Zeilen umbrechen
    for (std::size_t i = 0; i < lines.size(); ++i) text(x, y + static_cast<int>(i) * Font::lineHeight(scale), lines[i], c, scale); // Zeilen zeichnen
    return static_cast<int>(lines.size()) * Font::lineHeight(scale);         // Gesamthöhe
} // Ende von textWrapped

// Zeichnet ein Symbol zentriert (optional gedreht)
void Ui::icon(const std::string& name, int cx, int cy, float angle) {        // Beginn von icon
    const Image* img = get(name);                                            // Bild suchen
    if (!img) return;                                                        // Fehlt
    if (angle == 0.0f) m_canvas->blit(*img, cx - img->width / 2, cy - img->height / 2); // Ungedreht
    else m_canvas->blitRotated(*img, cx, cy, angle);                         // Gedreht
} // Ende von icon

// Zeichnet ein Symbol zentriert in einer bestimmten Größe
void Ui::iconScaled(const std::string& name, int cx, int cy, int size) {     // Beginn von iconScaled
    const Image* img = get(name);                                            // Bild suchen
    if (!img) return;                                                        // Fehlt
    m_canvas->blitScaled(*img, RectI{0, 0, img->width, img->height}, RectI{cx - size / 2, cy - size / 2, size, size}); // Skaliert zeichnen
} // Ende von iconScaled

void Ui::image(const Image& img, int x, int y) { m_canvas->blit(img, x, y); } // Bild zeichnen

void Ui::tooltip(const std::vector<std::string>& lines) { m_tooltip = lines; } // Tooltip für das Ende des Bildes merken
