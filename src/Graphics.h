// Graphics.h - Bildspeicher (Image) und Zeichenfunktionen (Canvas)
// Alles wird von der CPU in einen Speicherbereich gezeichnet, es wird kein Grafikbeschleuniger benutzt.
#pragma once // Header nur einmal einbinden

#include <utility> // std::pair für Punktlisten
#include <vector>  // std::vector als Pixelspeicher

#include "Common.h" // Color, RectI und Hilfsfunktionen

// Ein Bild: Breite, Höhe und eine Liste aller Pixel (zeilenweise von oben links)
struct Image {                                             // Beginn der Struktur Image
    int width = 0;                                         // Breite in Pixel
    int height = 0;                                        // Höhe in Pixel
    std::vector<Color> pixels;                             // Pixelfarben (width * height Einträge)
    int firstRow = 0;                                      // Erste Zeile, die sichtbare Pixel enthält
    int lastRow = -1;                                      // Letzte Zeile, die sichtbare Pixel enthält

    Image() = default;                                     // Leeres Bild
    Image(int w, int h, Color fill = TRANSPARENT);         // Bild mit Größe und Füllfarbe anlegen
    void resize(int w, int h, Color fill = TRANSPARENT);   // Größe ändern und neu füllen
    bool empty() const { return width <= 0 || height <= 0; } // Hat das Bild keine Pixel?
    Color get(int x, int y) const;                         // Pixel lesen (außerhalb -> transparent)
    void set(int x, int y, Color c);                       // Pixel setzen (außerhalb wird ignoriert)
    void computeBounds();                                  // firstRow/lastRow neu berechnen
}; // Ende der Struktur Image

// Mischt eine Farbe mit Alpha-Kanal auf ein Zielpixel
Color blendPixel(Color dst, Color src);                    // Standard-Alpha-Mischung

// Zeichenfläche: zeichnet Formen und Bilder in ein Ziel-Image
class Canvas {                                             // Beginn der Klasse Canvas
public:                                                    // Öffentliche Schnittstelle
    explicit Canvas(Image& target);                        // Zeichenfläche für ein Zielbild anlegen
    int width() const { return m_target.width; }           // Breite des Ziels
    int height() const { return m_target.height; }         // Höhe des Ziels
    Image& target() { return m_target; }                   // Zugriff auf das Zielbild

    void setClip(const RectI& clip);                       // Zeichnen auf einen Bereich begrenzen
    void resetClip();                                      // Begrenzung aufheben (ganzes Bild)

    void clear(Color c);                                   // Ganzes Bild mit einer Farbe füllen
    void putPixel(int x, int y, Color c);                  // Einzelnes Pixel setzen (mit Alpha-Mischung)
    void fillRect(int x, int y, int w, int h, Color c);    // Gefülltes Rechteck (Alpha < 255 wird gemischt)
    void drawRect(int x, int y, int w, int h, Color c, int thickness = 1); // Rechteck-Rahmen
    void hLine(int x0, int x1, int y, Color c);            // Waagerechte Linie
    void vLine(int x, int y0, int y1, Color c);            // Senkrechte Linie
    void line(int x0, int y0, int x1, int y1, Color c, int thickness = 1); // Beliebige Linie
    void lineF(float x0, float y0, float x1, float y1, Color c, float thickness); // Linie mit Fließkomma-Koordinaten
    void fillCircle(int cx, int cy, int r, Color c);       // Gefüllter Kreis
    void ring(int cx, int cy, int r, int thickness, Color c); // Kreisring
    void fillEllipse(int cx, int cy, int rx, int ry, Color c); // Gefüllte Ellipse
    void fillPolygon(const std::vector<std::pair<float, float>>& points, Color c); // Gefülltes Vieleck
    void darken(int amountPercent);                        // Ganzes Bild abdunkeln (für Popups)

    void blit(const Image& src, int dx, int dy, bool flipX = false); // Ganzes Bild zeichnen
    void blitRegion(const Image& src, const RectI& srcRect, int dx, int dy, bool flipX = false); // Ausschnitt zeichnen
    void blitScaled(const Image& src, const RectI& srcRect, const RectI& dstRect, bool flipX = false); // Skaliert zeichnen

private:                                                   // Interne Daten
    Image& m_target;                                       // Zielbild, in das gezeichnet wird
    RectI m_clip;                                          // Aktueller Zeichenbereich
}; // Ende der Klasse Canvas

// Skaliert ein Bild mit "nächster Nachbar" (Pixel bleiben scharf, passend für Pixelart)
Image scaleImage(const Image& src, int newWidth, int newHeight); // Neues, skaliertes Bild zurückgeben
// Spiegelt ein Bild waagerecht
Image mirrorImage(const Image& src);                       // Gespiegeltes Bild zurückgeben
