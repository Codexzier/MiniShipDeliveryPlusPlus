// Rasterizer.cpp - Dreiecke per CPU mit Tiefenpuffer zeichnen (kein Grafikbeschleuniger)
#include "Rasterizer.h" // Eigene Deklarationen

#include <algorithm> // std::min, std::max, std::fill
#include <cmath>     // std::floor, std::ceil
#include <limits>    // std::numeric_limits

namespace { // Interne Hilfen

// Ein in Bildschirmkoordinaten umgerechneter Eckpunkt
struct ScreenVertex { // Beginn der Struktur
    float x;          // Bildschirm x (in Supersampling-Pixeln)
    float y;          // Bildschirm y
    float depth;      // Nähe zur Kamera (größer = näher)
    Vec3 world;       // Weltposition (für die Flächennormale)
}; // Ende der Struktur ScreenVertex

const Vec3 TO_CAMERA = normalize(Vec3(0.61237f, 0.61237f, 0.5f));  // Richtung vom Modell zur Kamera (Welt: x, y, Höhe)
const Vec3 TO_LIGHT = normalize(Vec3(-0.2f, 0.45f, 0.87f));        // Richtung zur Sonne (von oben links)

// Liest eine Texturfarbe an der Stelle (u, v) (nächster Nachbar, wiederholend)
Color sampleTexture(const Image& tex, float u, float v) {          // Beginn von sampleTexture
    int x = static_cast<int>(std::floor(u * static_cast<float>(tex.width))) % tex.width; // Spalte
    int y = static_cast<int>(std::floor((1.0f - v) * static_cast<float>(tex.height))) % tex.height; // Zeile (v zeigt nach oben)
    if (x < 0) x += tex.width;                                      // Negative Werte umklappen
    if (y < 0) y += tex.height;                                     // Negative Werte umklappen
    return tex.pixels[static_cast<std::size_t>(y) * tex.width + x]; // Pixel zurückgeben
} // Ende von sampleTexture

// Multipliziert eine Farbe mit einer Helligkeit
Color shadeColor(Color c, float s) {                               // Beginn von shadeColor
    int r = std::min(255, static_cast<int>(static_cast<float>(redOf(c)) * s));   // Rot
    int g = std::min(255, static_cast<int>(static_cast<float>(greenOf(c)) * s)); // Grün
    int b = std::min(255, static_cast<int>(static_cast<float>(blueOf(c)) * s));  // Blau
    return rgba(r, g, b, 255);                                      // Voll deckend
} // Ende von shadeColor

} // Ende des internen Namensraums

// Rendert ein Modell in ein Sprite
Sprite renderMesh(const Mesh& mesh, const std::vector<Vec3>* positions, const Mat4& transform, const RenderParams& params) { // Beginn von renderMesh
    Sprite sprite;                                                          // Ergebnis
    const std::vector<Vec3>& pos = positions ? *positions : mesh.positions; // Welche Eckpunkte benutzt werden
    if (mesh.triangles.empty() || pos.empty()) return sprite;               // Nichts zu zeichnen
    const int ss = std::max(1, params.supersample);                         // Supersampling-Faktor
    const float tw = params.tileWidth * static_cast<float>(ss);             // Kachelbreite in internen Pixeln
    const float c = std::cos(params.rotation);                              // Kosinus der Drehung
    const float s = std::sin(params.rotation);                              // Sinus der Drehung

    // 1. Alle Eckpunkte in Welt- und Bildschirmkoordinaten umrechnen
    std::vector<ScreenVertex> sv(pos.size());                               // Umgerechnete Punkte
    float minX = std::numeric_limits<float>::max(), minY = minX;            // Kleinste Bildschirmkoordinaten
    float maxX = -minX, maxY = -minX;                                       // Größte Bildschirmkoordinaten
    for (std::size_t i = 0; i < pos.size(); ++i) {                          // Alle Punkte
        Vec3 p = transform.transformPoint(pos[i]);                          // Modelltransformation (Maßstab, Teile-Position)
        float wx = p.x * c - p.z * s;                                       // Weltx nach Drehung (Modell-z wird Welt-y)
        float wy = p.x * s + p.z * c;                                       // Welty nach Drehung
        float wz = p.y;                                                     // Modell-y ist die Höhe
        ScreenVertex& v = sv[i];                                            // Referenz auf den Zielpunkt
        v.x = Iso::screenX(wx, wy, tw);                                     // Bildschirm x
        v.y = Iso::screenY(wx, wy, wz, tw);                                 // Bildschirm y
        v.depth = (wx + wy) * 0.61237f + wz * 0.5f;                         // Nähe zur Kamera
        v.world = Vec3(wx, wy, wz);                                         // Weltposition merken
        minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);             // Grenzen x aktualisieren
        minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);             // Grenzen y aktualisieren
    }                                                                       // Ende der Punktschleife

    // 2. Zielbild (intern vergrößert) mit Rand anlegen
    int margin = 2 * ss;                                                    // Rand um das Modell
    int offX = static_cast<int>(std::floor(-minX)) + margin;                // Verschiebung, damit alles ins Bild passt
    int offY = static_cast<int>(std::floor(-minY)) + margin;                // Verschiebung y
    int width = static_cast<int>(std::ceil(maxX - minX)) + 2 * margin + 1;  // Bildbreite
    int height = static_cast<int>(std::ceil(maxY - minY)) + 2 * margin + 1; // Bildhöhe
    width += (ss - width % ss) % ss;                                        // Auf ein Vielfaches von ss aufrunden
    height += (ss - height % ss) % ss;                                      // Auf ein Vielfaches von ss aufrunden
    if (width > 8192 || height > 8192) return sprite;                       // Unsinnig große Modelle abfangen
    std::vector<Color> color(static_cast<std::size_t>(width) * height, TRANSPARENT); // Farbpuffer
    std::vector<float> zbuf(static_cast<std::size_t>(width) * height, -std::numeric_limits<float>::max()); // Tiefenpuffer

    // 3. Alle Dreiecke zeichnen
    for (const Triangle& t : mesh.triangles) {                              // Alle Dreiecke
        const ScreenVertex& a = sv[static_cast<std::size_t>(t.v[0])];       // Ecke A
        const ScreenVertex& b = sv[static_cast<std::size_t>(t.v[1])];       // Ecke B
        const ScreenVertex& d = sv[static_cast<std::size_t>(t.v[2])];       // Ecke C
        float ax = a.x + offX, ay = a.y + offY;                             // A im Bild
        float bx = b.x + offX, by = b.y + offY;                             // B im Bild
        float cx = d.x + offX, cy = d.y + offY;                             // C im Bild
        float area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);         // Doppelte Dreiecksfläche (Vorzeichen = Umlaufsinn)
        if (std::fabs(area) < 1e-6f) continue;                              // Entartete Dreiecke überspringen
        Vec3 n = normalize(cross(b.world - a.world, d.world - a.world));    // Flächennormale in Weltkoordinaten
        if (dot(n, TO_CAMERA) < 0.0f) n = n * -1.0f;                        // Zur Kamera zeigende Seite verwenden (beidseitig)
        float light = params.ambient + params.diffuse * std::max(0.0f, dot(n, TO_LIGHT)); // Helligkeit der Fläche
        const Material& mat = mesh.materials[static_cast<std::size_t>(std::max(0, std::min(t.material, static_cast<int>(mesh.materials.size()) - 1)))]; // Material
        const Image* tex = (mat.texture && !mat.texture->empty() && t.uv[0] >= 0 && t.uv[1] >= 0 && t.uv[2] >= 0) ? mat.texture.get() : nullptr; // Textur benutzen?
        Color flat = shadeColor(multiplyColor(mat.color, params.tint), light); // Fertige Farbe ohne Textur
        float u0 = 0, v0 = 0, u1 = 0, v1 = 0, u2 = 0, v2 = 0;               // Texturkoordinaten der Ecken
        if (tex) {                                                          // Mit Textur
            u0 = mesh.uvs[t.uv[0] * 2]; v0 = mesh.uvs[t.uv[0] * 2 + 1];      // UV von A
            u1 = mesh.uvs[t.uv[1] * 2]; v1 = mesh.uvs[t.uv[1] * 2 + 1];      // UV von B
            u2 = mesh.uvs[t.uv[2] * 2]; v2 = mesh.uvs[t.uv[2] * 2 + 1];      // UV von C
        }                                                                   // Ende Textur
        int x0 = std::max(0, static_cast<int>(std::floor(std::min({ax, bx, cx})))); // Linke Grenze des Dreiecks
        int x1 = std::min(width - 1, static_cast<int>(std::ceil(std::max({ax, bx, cx})))); // Rechte Grenze
        int y0 = std::max(0, static_cast<int>(std::floor(std::min({ay, by, cy})))); // Obere Grenze
        int y1 = std::min(height - 1, static_cast<int>(std::ceil(std::max({ay, by, cy})))); // Untere Grenze
        float invArea = 1.0f / area;                                        // Kehrwert der Fläche
        for (int y = y0; y <= y1; ++y) {                                    // Alle Zeilen des Dreiecks
            float py = static_cast<float>(y) + 0.5f;                        // Pixelmitte y
            for (int x = x0; x <= x1; ++x) {                                // Alle Spalten
                float px = static_cast<float>(x) + 0.5f;                    // Pixelmitte x
                float w0 = ((bx - px) * (cy - py) - (by - py) * (cx - px)) * invArea; // Gewicht von A
                float w1 = ((cx - px) * (ay - py) - (cy - py) * (ax - px)) * invArea; // Gewicht von B
                float w2 = 1.0f - w0 - w1;                                  // Gewicht von C
                if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;          // Pixel liegt außerhalb
                float z = w0 * a.depth + w1 * b.depth + w2 * d.depth;       // Tiefe am Pixel
                std::size_t idx = static_cast<std::size_t>(y) * width + x;  // Index im Puffer
                if (z <= zbuf[idx]) continue;                               // Verdeckt -> überspringen
                zbuf[idx] = z;                                              // Neue Tiefe merken
                if (tex) {                                                  // Texturierte Fläche
                    float u = w0 * u0 + w1 * u1 + w2 * u2;                  // u am Pixel
                    float v = w0 * v0 + w1 * v1 + w2 * v2;                  // v am Pixel
                    color[idx] = shadeColor(multiplyColor(sampleTexture(*tex, u, v), multiplyColor(mat.color, params.tint)), light); // Texturfarbe beleuchten
                } else {                                                    // Einfarbige Fläche
                    color[idx] = flat;                                      // Vorberechnete Farbe
                }                                                           // Ende der Unterscheidung
            }                                                               // Ende der Spaltenschleife
        }                                                                   // Ende der Zeilenschleife
    }                                                                       // Ende der Dreiecksschleife

    // 4. Verkleinern (Kantenglättung) und Sprite füllen
    int outW = width / ss;                                                  // Endbreite
    int outH = height / ss;                                                 // Endhöhe
    sprite.image.resize(outW, outH);                                        // Bild anlegen
    for (int y = 0; y < outH; ++y) {                                        // Alle Zielzeilen
        for (int x = 0; x < outW; ++x) {                                    // Alle Zielspalten
            int r = 0, g = 0, b = 0, a = 0;                                 // Summen (vormultipliziert)
            for (int sy = 0; sy < ss; ++sy) {                               // Alle Unterpixel in y
                for (int sx = 0; sx < ss; ++sx) {                           // Alle Unterpixel in x
                    Color px = color[static_cast<std::size_t>(y * ss + sy) * width + (x * ss + sx)]; // Unterpixel
                    int pa = alphaOf(px);                                   // Deckkraft
                    r += redOf(px) * pa; g += greenOf(px) * pa; b += blueOf(px) * pa; a += pa; // Aufsummieren
                }                                                           // Ende x
            }                                                               // Ende y
            if (a == 0) continue;                                           // Völlig durchsichtig -> bleibt leer
            sprite.image.set(x, y, rgba(r / a, g / a, b / a, a / (ss * ss))); // Mittelwert eintragen
        }                                                                   // Ende der Spaltenschleife
    }                                                                       // Ende der Zeilenschleife
    sprite.image.computeBounds();                                           // Sichtbaren Bereich bestimmen
    sprite.anchorX = offX / ss;                                             // Ursprung x im Endbild
    sprite.anchorY = offY / ss;                                             // Ursprung y im Endbild
    return sprite;                                                          // Fertiges Sprite
} // Ende von renderMesh
