// Graphics.cpp - Software-Rendering: alle Pixel werden von der CPU berechnet
#include "Graphics.h" // Eigene Deklarationen

#include <algorithm> // std::fill, std::sort, std::min, std::max
#include <cmath>     // std::sqrt, std::floor, std::ceil
#include <cstdlib>   // std::abs für Ganzzahlen

// Legt ein Bild mit gegebener Größe an und füllt es mit einer Farbe
Image::Image(int w, int h, Color fill) { resize(w, h, fill); } // Konstruktor nutzt einfach resize

// Ändert die Größe des Bildes und füllt alle Pixel neu
void Image::resize(int w, int h, Color fill) {                  // Beginn von resize
    width = std::max(0, w);                                     // Breite übernehmen (nie negativ)
    height = std::max(0, h);                                    // Höhe übernehmen (nie negativ)
    pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), fill); // Speicher anlegen und füllen
    firstRow = 0;                                               // Sichtbarer Bereich beginnt oben
    lastRow = height - 1;                                       // ... und endet unten
} // Ende von resize

// Liest ein Pixel, außerhalb des Bildes wird "durchsichtig" geliefert
Color Image::get(int x, int y) const {                          // Beginn von get
    if (x < 0 || y < 0 || x >= width || y >= height) return TRANSPARENT; // Außerhalb -> transparent
    return pixels[static_cast<std::size_t>(y) * width + x];     // Pixel aus dem Speicher lesen
} // Ende von get

// Setzt ein Pixel direkt (ohne Mischung), außerhalb wird ignoriert
void Image::set(int x, int y, Color c) {                        // Beginn von set
    if (x < 0 || y < 0 || x >= width || y >= height) return;    // Außerhalb -> nichts tun
    pixels[static_cast<std::size_t>(y) * width + x] = c;        // Pixel in den Speicher schreiben
} // Ende von set

// Bestimmt die erste und letzte Zeile mit sichtbaren Pixeln (beschleunigt das Zeichnen)
void Image::computeBounds() {                                   // Beginn von computeBounds
    firstRow = height;                                          // Startwert: keine sichtbare Zeile gefunden
    lastRow = -1;                                               // Startwert: keine sichtbare Zeile gefunden
    for (int y = 0; y < height; ++y) {                          // Alle Zeilen durchgehen
        const Color* row = &pixels[static_cast<std::size_t>(y) * width]; // Zeiger auf den Zeilenanfang
        for (int x = 0; x < width; ++x) {                       // Alle Pixel der Zeile prüfen
            if (alphaOf(row[x]) != 0) {                         // Sichtbares Pixel gefunden
                firstRow = std::min(firstRow, y);               // Erste sichtbare Zeile merken
                lastRow = y;                                    // Letzte sichtbare Zeile aktualisieren
                break;                                          // Rest der Zeile muss nicht geprüft werden
            }                                                   // Ende der Prüfung
        }                                                       // Ende der Pixelschleife
    }                                                           // Ende der Zeilenschleife
    if (lastRow < 0) firstRow = 0;                              // Komplett leeres Bild: Bereich leer lassen
} // Ende von computeBounds

// Mischt eine (teil-)durchsichtige Farbe src auf die Farbe dst
Color blendPixel(Color dst, Color src) {                        // Beginn von blendPixel
    int a = alphaOf(src);                                       // Deckkraft der neuen Farbe
    if (a >= 255) return src;                                   // Voll deckend -> einfach ersetzen
    if (a <= 0) return dst;                                     // Völlig durchsichtig -> nichts ändern
    int inv = 255 - a;                                          // Anteil der alten Farbe
    int r = (redOf(src) * a + redOf(dst) * inv) / 255;          // Rot mischen
    int g = (greenOf(src) * a + greenOf(dst) * inv) / 255;      // Grün mischen
    int b = (blueOf(src) * a + blueOf(dst) * inv) / 255;        // Blau mischen
    int outA = std::max(alphaOf(dst), a);                       // Deckkraft des Ergebnisses
    return rgba(r, g, b, outA);                                 // Gemischte Farbe zurückgeben
} // Ende von blendPixel

// Erstellt eine Zeichenfläche für ein Zielbild
Canvas::Canvas(Image& target) : m_target(target) { resetClip(); } // Ziel merken und ganzen Bereich freigeben

// Begrenzt das Zeichnen auf ein Rechteck (wird mit der Bildgröße geschnitten)
void Canvas::setClip(const RectI& clip) {                       // Beginn von setClip
    int x0 = std::max(0, clip.x);                               // Linke Kante nicht kleiner als 0
    int y0 = std::max(0, clip.y);                               // Obere Kante nicht kleiner als 0
    int x1 = std::min(m_target.width, clip.x + clip.w);         // Rechte Kante nicht größer als Bildbreite
    int y1 = std::min(m_target.height, clip.y + clip.h);        // Untere Kante nicht größer als Bildhöhe
    m_clip = RectI{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)}; // Geschnittenes Rechteck speichern
} // Ende von setClip

void Canvas::resetClip() { m_clip = RectI{0, 0, m_target.width, m_target.height}; } // Ganzes Bild als Zeichenbereich

// Füllt das ganze Bild mit einer Farbe
void Canvas::clear(Color c) { std::fill(m_target.pixels.begin(), m_target.pixels.end(), c); } // Alle Pixel überschreiben

// Setzt ein Pixel und berücksichtigt Durchsichtigkeit und Zeichenbereich
void Canvas::putPixel(int x, int y, Color c) {                  // Beginn von putPixel
    if (!m_clip.contains(x, y)) return;                         // Außerhalb des Zeichenbereichs -> nichts tun
    Color& dst = m_target.pixels[static_cast<std::size_t>(y) * m_target.width + x]; // Referenz auf das Zielpixel
    dst = blendPixel(dst, c);                                   // Farbe aufmischen
} // Ende von putPixel

// Zeichnet ein gefülltes Rechteck
void Canvas::fillRect(int x, int y, int w, int h, Color c) {    // Beginn von fillRect
    int x0 = std::max(x, m_clip.x);                             // Linke Kante auf den Zeichenbereich begrenzen
    int y0 = std::max(y, m_clip.y);                             // Obere Kante begrenzen
    int x1 = std::min(x + w, m_clip.x + m_clip.w);              // Rechte Kante begrenzen
    int y1 = std::min(y + h, m_clip.y + m_clip.h);              // Untere Kante begrenzen
    if (x0 >= x1 || y0 >= y1) return;                           // Nichts sichtbar -> fertig
    int a = alphaOf(c);                                         // Deckkraft der Farbe
    if (a == 0) return;                                         // Unsichtbare Farbe -> nichts zeichnen
    for (int yy = y0; yy < y1; ++yy) {                          // Alle Zeilen des Rechtecks
        Color* row = &m_target.pixels[static_cast<std::size_t>(yy) * m_target.width]; // Zeiger auf den Zeilenanfang
        if (a == 255) {                                         // Voll deckende Farbe
            std::fill(row + x0, row + x1, c);                   // Schnell die ganze Zeile füllen
        } else {                                                // Teilweise durchsichtig
            for (int xx = x0; xx < x1; ++xx) row[xx] = blendPixel(row[xx], c); // Jedes Pixel mischen
        }                                                       // Ende der Unterscheidung
    }                                                           // Ende der Zeilenschleife
} // Ende von fillRect

// Zeichnet einen Rechteck-Rahmen mit gegebener Dicke
void Canvas::drawRect(int x, int y, int w, int h, Color c, int thickness) { // Beginn von drawRect
    fillRect(x, y, w, thickness, c);                            // Obere Kante
    fillRect(x, y + h - thickness, w, thickness, c);            // Untere Kante
    fillRect(x, y + thickness, thickness, h - 2 * thickness, c); // Linke Kante
    fillRect(x + w - thickness, y + thickness, thickness, h - 2 * thickness, c); // Rechte Kante
} // Ende von drawRect

// Waagerechte Linie von x0 bis x1 (einschließlich)
void Canvas::hLine(int x0, int x1, int y, Color c) {            // Beginn von hLine
    if (x1 < x0) std::swap(x0, x1);                             // Reihenfolge sicherstellen
    fillRect(x0, y, x1 - x0 + 1, 1, c);                         // Als 1 Pixel hohes Rechteck zeichnen
} // Ende von hLine

// Senkrechte Linie von y0 bis y1 (einschließlich)
void Canvas::vLine(int x, int y0, int y1, Color c) {            // Beginn von vLine
    if (y1 < y0) std::swap(y0, y1);                             // Reihenfolge sicherstellen
    fillRect(x, y0, 1, y1 - y0 + 1, c);                         // Als 1 Pixel breites Rechteck zeichnen
} // Ende von vLine

// Zeichnet eine Linie (Bresenham-Verfahren bei Dicke 1, sonst über Kreise)
void Canvas::line(int x0, int y0, int x1, int y1, Color c, int thickness) { // Beginn von line
    if (thickness > 1) {                                        // Dicke Linie gewünscht
        lineF(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x1), static_cast<float>(y1), c, static_cast<float>(thickness)); // Über lineF zeichnen
        return;                                                 // Fertig
    }                                                           // Ende der Dicke-Prüfung
    int dx = std::abs(x1 - x0);                                 // Abstand in x-Richtung
    int dy = -std::abs(y1 - y0);                                // Negativer Abstand in y-Richtung
    int sx = x0 < x1 ? 1 : -1;                                  // Schrittrichtung in x
    int sy = y0 < y1 ? 1 : -1;                                  // Schrittrichtung in y
    int err = dx + dy;                                          // Fehlerterm des Bresenham-Verfahrens
    while (true) {                                              // Schleife bis zum Endpunkt
        putPixel(x0, y0, c);                                    // Aktuelles Pixel setzen
        if (x0 == x1 && y0 == y1) break;                        // Endpunkt erreicht
        int e2 = 2 * err;                                       // Doppelter Fehler
        if (e2 >= dy) { err += dy; x0 += sx; }                  // Schritt in x-Richtung
        if (e2 <= dx) { err += dx; y0 += sy; }                  // Schritt in y-Richtung
    }                                                           // Ende der Schleife
} // Ende von line

// Zeichnet eine Linie mit Fließkomma-Koordinaten und beliebiger Dicke
void Canvas::lineF(float x0, float y0, float x1, float y1, Color c, float thickness) { // Beginn von lineF
    float dx = x1 - x0;                                         // Abstand in x
    float dy = y1 - y0;                                         // Abstand in y
    float length = std::sqrt(dx * dx + dy * dy);                // Länge der Linie
    int steps = std::max(1, static_cast<int>(std::ceil(length))); // Ein Schritt pro Pixel Länge
    int radius = static_cast<int>(thickness * 0.5f);            // Radius der Kreise aus der Dicke
    for (int i = 0; i <= steps; ++i) {                          // Alle Schritte entlang der Linie
        float t = static_cast<float>(i) / static_cast<float>(steps); // Position 0..1 auf der Linie
        int px = static_cast<int>(std::floor(x0 + dx * t + 0.5f)); // x-Koordinate runden
        int py = static_cast<int>(std::floor(y0 + dy * t + 0.5f)); // y-Koordinate runden
        if (radius <= 0) putPixel(px, py, c);                   // Dünne Linie: einzelnes Pixel
        else fillCircle(px, py, radius, c);                     // Dicke Linie: kleiner Kreis
    }                                                           // Ende der Schleife
} // Ende von lineF

// Zeichnet einen gefüllten Kreis
void Canvas::fillCircle(int cx, int cy, int r, Color c) {       // Beginn von fillCircle
    if (r <= 0) { putPixel(cx, cy, c); return; }                // Radius 0: nur ein Pixel
    float rr = (static_cast<float>(r) + 0.5f) * (static_cast<float>(r) + 0.5f); // Quadrat des Radius (+0.5 für runde Form)
    for (int dy = -r; dy <= r; ++dy) {                          // Alle Zeilen des Kreises
        int dx = static_cast<int>(std::sqrt(std::max(0.0f, rr - static_cast<float>(dy * dy)))); // Halbe Breite der Zeile
        fillRect(cx - dx, cy + dy, 2 * dx + 1, 1, c);           // Zeile des Kreises füllen
    }                                                           // Ende der Schleife
} // Ende von fillCircle

// Zeichnet einen Kreisring mit gegebener Dicke
void Canvas::ring(int cx, int cy, int r, int thickness, Color c) { // Beginn von ring
    float outer = (static_cast<float>(r) + 0.5f) * (static_cast<float>(r) + 0.5f); // Quadrat des Außenradius
    float innerR = std::max(0.0f, static_cast<float>(r - thickness) + 0.5f); // Innenradius
    float inner = innerR * innerR;                              // Quadrat des Innenradius
    for (int dy = -r; dy <= r; ++dy) {                          // Alle Zeilen
        for (int dx = -r; dx <= r; ++dx) {                      // Alle Spalten
            float d = static_cast<float>(dx * dx + dy * dy);    // Quadrierter Abstand zum Mittelpunkt
            if (d <= outer && d >= inner) putPixel(cx + dx, cy + dy, c); // Pixel liegt im Ring -> setzen
        }                                                       // Ende der Spaltenschleife
    }                                                           // Ende der Zeilenschleife
} // Ende von ring

// Zeichnet eine gefüllte Ellipse mit den Halbachsen rx und ry
void Canvas::fillEllipse(int cx, int cy, int rx, int ry, Color c) { // Beginn von fillEllipse
    if (rx <= 0 || ry <= 0) { fillRect(cx - std::max(rx, 0), cy - std::max(ry, 0), 2 * std::max(rx, 0) + 1, 2 * std::max(ry, 0) + 1, c); return; } // Entartete Ellipse als Linie
    float fry = static_cast<float>(ry) + 0.5f;                  // Halbachse y (+0.5 für runde Form)
    for (int dy = -ry; dy <= ry; ++dy) {                        // Alle Zeilen
        float t = static_cast<float>(dy) / fry;                 // Relative Höhe -1..1
        int dx = static_cast<int>((static_cast<float>(rx) + 0.5f) * std::sqrt(std::max(0.0f, 1.0f - t * t))); // Halbe Breite dieser Zeile
        fillRect(cx - dx, cy + dy, 2 * dx + 1, 1, c);           // Zeile füllen
    }                                                           // Ende der Schleife
} // Ende von fillEllipse

// Füllt ein beliebiges Vieleck mit dem Scanline-Verfahren
void Canvas::fillPolygon(const std::vector<std::pair<float, float>>& points, Color c) { // Beginn von fillPolygon
    if (points.size() < 3) return;                              // Weniger als 3 Punkte ergeben keine Fläche
    float minY = points[0].second;                              // Kleinste y-Koordinate (Startwert)
    float maxY = points[0].second;                              // Größte y-Koordinate (Startwert)
    for (const auto& p : points) {                              // Alle Punkte durchgehen
        minY = std::min(minY, p.second);                        // Minimum aktualisieren
        maxY = std::max(maxY, p.second);                        // Maximum aktualisieren
    }                                                           // Ende der Schleife
    int yStart = std::max(m_clip.y, static_cast<int>(std::floor(minY))); // Erste Zeile (im Zeichenbereich)
    int yEnd = std::min(m_clip.y + m_clip.h - 1, static_cast<int>(std::ceil(maxY))); // Letzte Zeile
    std::vector<float> crossings;                               // Schnittpunkte der aktuellen Zeile mit den Kanten
    for (int y = yStart; y <= yEnd; ++y) {                      // Alle Zeilen durchgehen
        float sy = static_cast<float>(y) + 0.5f;                // Mitte der Pixelzeile
        crossings.clear();                                      // Schnittpunkte der letzten Zeile verwerfen
        for (std::size_t i = 0; i < points.size(); ++i) {       // Alle Kanten durchgehen
            const auto& a = points[i];                          // Startpunkt der Kante
            const auto& b = points[(i + 1) % points.size()];    // Endpunkt der Kante (letzte Kante schließt das Vieleck)
            if ((a.second <= sy && b.second > sy) || (b.second <= sy && a.second > sy)) { // Schneidet die Kante diese Zeile?
                float t = (sy - a.second) / (b.second - a.second); // Relative Position des Schnittpunkts
                crossings.push_back(a.first + t * (b.first - a.first)); // x-Koordinate des Schnittpunkts merken
            }                                                   // Ende der Schnittprüfung
        }                                                       // Ende der Kantenschleife
        std::sort(crossings.begin(), crossings.end());          // Schnittpunkte von links nach rechts sortieren
        for (std::size_t i = 0; i + 1 < crossings.size(); i += 2) { // Immer zwei Schnittpunkte bilden einen Abschnitt
            int x0 = static_cast<int>(std::ceil(crossings[i] - 0.5f)); // Erstes Pixel des Abschnitts
            int x1 = static_cast<int>(std::floor(crossings[i + 1] - 0.5f)); // Letztes Pixel des Abschnitts
            if (x1 >= x0) fillRect(x0, y, x1 - x0 + 1, 1, c);   // Abschnitt füllen
        }                                                       // Ende der Abschnittsschleife
    }                                                           // Ende der Zeilenschleife
} // Ende von fillPolygon

// Dunkelt das ganze Bild um einen Prozentsatz ab
void Canvas::darken(int amountPercent) {                        // Beginn von darken
    int keep = clampValue(100 - amountPercent, 0, 100);         // Wie viel Prozent der Helligkeit bleiben
    for (Color& p : m_target.pixels) {                          // Alle Pixel durchgehen
        int r = redOf(p) * keep / 100;                          // Rot abdunkeln
        int g = greenOf(p) * keep / 100;                        // Grün abdunkeln
        int b = blueOf(p) * keep / 100;                         // Blau abdunkeln
        p = rgba(r, g, b, alphaOf(p));                          // Pixel zurückschreiben
    }                                                           // Ende der Schleife
} // Ende von darken

// Zeichnet ein komplettes Bild an die Position (dx, dy)
void Canvas::blit(const Image& src, int dx, int dy, bool flipX) { // Beginn von blit
    blitRegion(src, RectI{0, 0, src.width, src.height}, dx, dy, flipX); // Ganzes Bild als Ausschnitt zeichnen
} // Ende von blit

// Zeichnet einen Ausschnitt eines Bildes (z.B. ein Einzelbild aus einem Sprite-Sheet)
void Canvas::blitRegion(const Image& src, const RectI& srcRect, int dx, int dy, bool flipX) { // Beginn von blitRegion
    if (src.empty()) return;                                    // Leeres Bild -> nichts zu tun
    int rowStart = std::max(srcRect.y, src.firstRow);           // Unsichtbare Zeilen oben überspringen
    int rowEnd = std::min(srcRect.y + srcRect.h - 1, src.lastRow); // Unsichtbare Zeilen unten überspringen
    for (int sy = rowStart; sy <= rowEnd; ++sy) {               // Alle sichtbaren Quellzeilen
        int ty = dy + (sy - srcRect.y);                         // Zielzeile berechnen
        if (ty < m_clip.y || ty >= m_clip.y + m_clip.h) continue; // Zeile außerhalb des Zeichenbereichs
        if (sy < 0 || sy >= src.height) continue;               // Zeile außerhalb des Quellbildes
        const Color* srcRow = &src.pixels[static_cast<std::size_t>(sy) * src.width]; // Zeiger auf die Quellzeile
        Color* dstRow = &m_target.pixels[static_cast<std::size_t>(ty) * m_target.width]; // Zeiger auf die Zielzeile
        int txStart = std::max(dx, m_clip.x);                   // Erste sichtbare Zielspalte
        int txEnd = std::min(dx + srcRect.w, m_clip.x + m_clip.w); // Erste Spalte hinter dem sichtbaren Bereich
        for (int tx = txStart; tx < txEnd; ++tx) {              // Alle sichtbaren Zielspalten
            int ix = tx - dx;                                   // Spalte innerhalb des Ausschnitts
            int sx = srcRect.x + (flipX ? (srcRect.w - 1 - ix) : ix); // Quellspalte (bei Spiegelung von rechts gelesen)
            if (sx < 0 || sx >= src.width) continue;            // Spalte außerhalb des Quellbildes
            Color c = srcRow[sx];                               // Quellfarbe lesen
            int a = alphaOf(c);                                 // Deckkraft der Quellfarbe
            if (a == 255) dstRow[tx] = c;                       // Voll deckend -> direkt kopieren
            else if (a != 0) dstRow[tx] = blendPixel(dstRow[tx], c); // Teilweise durchsichtig -> mischen
        }                                                       // Ende der Spaltenschleife
    }                                                           // Ende der Zeilenschleife
} // Ende von blitRegion

// Zeichnet einen Bildausschnitt skaliert in ein Zielrechteck (nächster Nachbar)
void Canvas::blitScaled(const Image& src, const RectI& srcRect, const RectI& dstRect, bool flipX) { // Beginn von blitScaled
    if (src.empty() || dstRect.w <= 0 || dstRect.h <= 0 || srcRect.w <= 0 || srcRect.h <= 0) return; // Ungültige Größen abfangen
    int yStart = std::max(dstRect.y, m_clip.y);                 // Erste sichtbare Zielzeile
    int yEnd = std::min(dstRect.y + dstRect.h, m_clip.y + m_clip.h); // Ende der sichtbaren Zielzeilen
    int xStart = std::max(dstRect.x, m_clip.x);                 // Erste sichtbare Zielspalte
    int xEnd = std::min(dstRect.x + dstRect.w, m_clip.x + m_clip.w); // Ende der sichtbaren Zielspalten
    for (int ty = yStart; ty < yEnd; ++ty) {                    // Alle sichtbaren Zielzeilen
        int sy = srcRect.y + (ty - dstRect.y) * srcRect.h / dstRect.h; // Passende Quellzeile berechnen
        if (sy < 0 || sy >= src.height) continue;               // Außerhalb des Quellbildes
        const Color* srcRow = &src.pixels[static_cast<std::size_t>(sy) * src.width]; // Zeiger auf die Quellzeile
        Color* dstRow = &m_target.pixels[static_cast<std::size_t>(ty) * m_target.width]; // Zeiger auf die Zielzeile
        for (int tx = xStart; tx < xEnd; ++tx) {                // Alle sichtbaren Zielspalten
            int ix = (tx - dstRect.x) * srcRect.w / dstRect.w;  // Passende Spalte im Ausschnitt
            int sx = srcRect.x + (flipX ? (srcRect.w - 1 - ix) : ix); // Quellspalte (ggf. gespiegelt)
            if (sx < 0 || sx >= src.width) continue;            // Außerhalb des Quellbildes
            Color c = srcRow[sx];                               // Quellfarbe lesen
            int a = alphaOf(c);                                 // Deckkraft
            if (a == 255) dstRow[tx] = c;                       // Voll deckend -> kopieren
            else if (a != 0) dstRow[tx] = blendPixel(dstRow[tx], c); // Teilweise durchsichtig -> mischen
        }                                                       // Ende der Spaltenschleife
    }                                                           // Ende der Zeilenschleife
} // Ende von blitScaled

// Erzeugt eine skalierte Kopie eines Bildes
Image scaleImage(const Image& src, int newWidth, int newHeight) { // Beginn von scaleImage
    Image result(newWidth, newHeight);                          // Neues, leeres Bild in Zielgröße
    Canvas canvas(result);                                      // Zeichenfläche für das neue Bild
    canvas.blitScaled(src, RectI{0, 0, src.width, src.height}, RectI{0, 0, newWidth, newHeight}); // Skaliert hineinzeichnen
    result.computeBounds();                                     // Sichtbaren Bereich bestimmen
    return result;                                              // Neues Bild zurückgeben
} // Ende von scaleImage

// Erzeugt eine waagerecht gespiegelte Kopie eines Bildes
Image mirrorImage(const Image& src) {                           // Beginn von mirrorImage
    Image result(src.width, src.height);                        // Neues Bild gleicher Größe
    for (int y = 0; y < src.height; ++y) {                      // Alle Zeilen
        for (int x = 0; x < src.width; ++x) {                   // Alle Spalten
            result.set(src.width - 1 - x, y, src.get(x, y));    // Pixel an die gespiegelte Position kopieren
        }                                                       // Ende der Spaltenschleife
    }                                                           // Ende der Zeilenschleife
    result.computeBounds();                                     // Sichtbaren Bereich bestimmen
    return result;                                              // Gespiegeltes Bild zurückgeben
} // Ende von mirrorImage
