// Background.h - Parallax-Hintergrund: ferne Ebenen bewegen sich langsamer und erzeugen Tiefe
#pragma once // Header nur einmal einbinden

#include <vector> // std::vector

#include "Assets.h"   // Bilder der Ebenen
#include "Graphics.h" // Canvas

class Background {                                                        // Beginn der Klasse Background
public:                                                                   // Öffentliche Schnittstelle
    void build(Assets& assets, int screenW, int screenH, int tile, float groundYTiles); // Ebenen erzeugen/laden
    void draw(Canvas& canvas, float cameraPx, float time) const;          // Hintergrund zeichnen
private:                                                                  // Interne Daten
    struct Layer {                                                        // Eine Hintergrundebene
        const Image* image = nullptr;                                     // Bild der Ebene (kachelbar)
        float factor = 0.5f;                                              // Parallax-Faktor (0 = steht, 1 = bewegt sich wie der Vordergrund)
        float drift = 0.0f;                                               // Eigene Bewegung in Pixel pro Sekunde (z.B. Wolken)
    };                                                                    // Ende der Struktur Layer
    std::vector<Layer> m_layers;                                          // Alle Ebenen von hinten nach vorne
    std::vector<Color> m_skyRows;                                         // Himmelsfarbe für jede Bildzeile
    int m_tile = 128;                                                     // Kachelgröße
}; // Ende der Klasse Background
