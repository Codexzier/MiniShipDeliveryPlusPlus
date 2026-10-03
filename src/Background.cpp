// Background.cpp - Erzeugen und Zeichnen der Parallax-Ebenen
#include "Background.h" // Eigene Deklarationen

#include <cmath> // std::fmod, std::floor

#include "SpriteFactory.h" // Platzhalter der Hintergrundebenen

// Erzeugt oder lädt alle Hintergrundebenen
void Background::build(Assets& assets, int screenW, int screenH, int tile, float groundYTiles) { // Beginn von build
    m_tile = tile;                                                          // Kachelgröße merken
    int layerW = screenW * 2;                                               // Ebenen sind doppelt so breit wie der Bildschirm
    int groundPx = static_cast<int>(groundYTiles * static_cast<float>(tile)); // Bodenkante in Pixel
    m_skyRows.resize(static_cast<std::size_t>(screenH));                    // Eine Farbe pro Bildzeile
    for (int y = 0; y < screenH; ++y) {                                     // Alle Zeilen
        float t = static_cast<float>(y) / static_cast<float>(std::max(1, groundPx)); // 0 oben, 1 an der Bodenkante
        m_skyRows[static_cast<std::size_t>(y)] = mixColor(rgba(70, 140, 225), rgba(200, 230, 250), t); // Farbverlauf von Blau zu Hellblau
    }                                                                       // Ende der Schleife
    m_layers.clear();                                                       // Alte Ebenen verwerfen
    const Image& clouds = assets.loadOrGenerate("hintergrund_wolken", layerW, screenH, [tile](Image& img) { SpriteFactory::drawCloudLayer(img, tile); }); // Wolken
    const Image& mountains = assets.loadOrGenerate("hintergrund_berge", layerW, screenH, [tile, groundPx](Image& img) { SpriteFactory::drawMountainLayer(img, tile, groundPx); }); // Berge
    const Image& city = assets.loadOrGenerate("hintergrund_stadt", layerW, screenH, [tile, groundPx](Image& img) { SpriteFactory::drawCityLayer(img, tile, groundPx); }); // Stadt
    const Image& houses = assets.loadOrGenerate("hintergrund_haeuser", layerW, screenH, [tile, groundPx](Image& img) { SpriteFactory::drawHouseLayer(img, tile, groundPx); }); // Häuser
    m_layers.push_back(Layer{&clouds, 0.05f, static_cast<float>(tile) * 0.12f}); // Wolken: fast still, treiben langsam
    m_layers.push_back(Layer{&mountains, 0.12f, 0.0f});                     // Berge: sehr langsam
    m_layers.push_back(Layer{&city, 0.3f, 0.0f});                           // Stadt: langsam
    m_layers.push_back(Layer{&houses, 0.55f, 0.0f});                        // Häuser: halb so schnell wie der Vordergrund
} // Ende von build

// Zeichnet Himmel, Sonne und alle Ebenen mit ihrem Parallax-Versatz
void Background::draw(Canvas& canvas, float cameraPx, float time) const {  // Beginn von draw
    int w = canvas.width();                                                 // Bildschirmbreite
    int h = canvas.height();                                                // Bildschirmhöhe
    for (int y = 0; y < h && y < static_cast<int>(m_skyRows.size()); ++y) { // Alle Zeilen
        canvas.fillRect(0, y, w, 1, m_skyRows[static_cast<std::size_t>(y)]); // Himmel zeilenweise füllen
    }                                                                       // Ende der Schleife
    int sunR = m_tile * 4 / 10;                                             // Radius der Sonne
    canvas.fillCircle(w * 82 / 100, h * 18 / 100, sunR + m_tile / 10, rgba(255, 240, 180, 120)); // Leuchtender Rand
    canvas.fillCircle(w * 82 / 100, h * 18 / 100, sunR, rgba(255, 230, 120)); // Sonne
    for (const Layer& layer : m_layers) {                                   // Alle Ebenen von hinten nach vorne
        if (!layer.image || layer.image->empty()) continue;                 // Fehlendes Bild überspringen
        int lw = layer.image->width;                                        // Breite der Ebene
        float offset = cameraPx * layer.factor + time * layer.drift;        // Verschiebung dieser Ebene
        int start = -static_cast<int>(std::fmod(offset, static_cast<float>(lw))); // Startposition (wiederholt sich)
        if (start > 0) start -= lw;                                         // Immer links vom Bildschirmrand beginnen
        for (int x = start; x < w; x += lw) canvas.blit(*layer.image, x, 0); // Ebene nebeneinander wiederholen
    }                                                                       // Ende der Ebenenschleife
} // Ende von draw
