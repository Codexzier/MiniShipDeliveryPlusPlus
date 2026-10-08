// Rasterizer.h - Software-3D-Renderer: zeichnet 3D-Modelle per CPU in isometrische Sprites
//
// Die Spielwelt benutzt eine isometrische 2:1-Projektion (Kamera 45 Grad gedreht, 30 Grad geneigt):
//   bildschirmX = (weltX - weltY) * kachelbreite / 2
//   bildschirmY = (weltX + weltY) * kachelbreite / 4 - hoehe * kachelbreite * 0,612
// Modelle (OBJ/FBX, y zeigt nach oben) werden so umgerechnet: weltX = x, weltY = z, hoehe = y.
#pragma once // Header nur einmal einbinden

#include <vector> // std::vector

#include "Graphics.h" // Image
#include "Math3D.h"   // Vec3, Mat4
#include "Mesh.h"     // Mesh

namespace Iso {                                                             // Projektionshilfen für die ganze Spielwelt
constexpr float HEIGHT_FACTOR = 0.61237244f;                                // Bildschirmpixel je Höheneinheit (mal Kachelbreite)
inline float screenX(float wx, float wy, float tileW) { return (wx - wy) * tileW * 0.5f; } // Welt -> Bildschirm x
inline float screenY(float wx, float wy, float z, float tileW) { return (wx + wy) * tileW * 0.25f - z * tileW * HEIGHT_FACTOR; } // Welt -> Bildschirm y
inline void screenToGround(float sx, float sy, float tileW, float& wx, float& wy) { // Bildschirm -> Boden (Höhe 0)
    float a = sx / (tileW * 0.5f);                                          // entspricht weltX - weltY
    float b = sy / (tileW * 0.25f);                                         // entspricht weltX + weltY
    wx = (a + b) * 0.5f;                                                    // weltX zurückrechnen
    wy = (b - a) * 0.5f;                                                    // weltY zurückrechnen
} // Ende von screenToGround
} // Ende des Namensraums Iso

// Ein gerendertes Bild mit Ankerpunkt (Bildposition des Modell-Ursprungs am Boden)
struct Sprite {                    // Beginn der Struktur
    Image image;                   // Pixel des Sprites
    int anchorX = 0;               // x des Ursprungs im Bild
    int anchorY = 0;               // y des Ursprungs im Bild
    bool valid() const { return !image.empty(); } // Wurde etwas gezeichnet?
}; // Ende der Struktur Sprite

// Einstellungen für einen Rendervorgang
struct RenderParams {                         // Beginn der Struktur
    float tileWidth = 128.0f;                 // Breite einer Bodenkachel in Pixel (bestimmt den Maßstab)
    float rotation = 0.0f;                    // Drehung um die senkrechte Weltachse (Radiant)
    int supersample = 2;                      // Kantenglättung: intern so viel größer rendern
    float ambient = 0.55f;                    // Grundhelligkeit
    float diffuse = 0.5f;                     // Anteil des gerichteten Sonnenlichts
    Color tint = rgba(255, 255, 255);         // Zusätzliche Einfärbung aller Farben
}; // Ende der Struktur RenderParams

// Rendert ein Modell. positions ersetzt optional die Eckpunkte (z.B. animierte Figur), transform wirkt im Modellraum.
Sprite renderMesh(const Mesh& mesh, const std::vector<Vec3>* positions, const Mat4& transform, const RenderParams& params);
