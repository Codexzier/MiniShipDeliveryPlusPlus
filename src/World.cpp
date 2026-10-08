// World.cpp - Erzeugung und Darstellung der Inselwelt, Wegfindung zu Fuß
#include "World.h" // Eigene Deklarationen

#include <algorithm> // std::min, std::max, std::reverse
#include <cmath>     // std::sqrt, std::atan2, std::sin, std::cos
#include <cstdio>    // std::sscanf
#include <cstdlib>   // std::abs
#include <queue>     // std::priority_queue für A*

#include "Rasterizer.h" // Iso-Projektion

namespace { // Interne Hilfen

constexpr float INF = 1e9f;                                               // "Unendlich" für Abstände

// Richtungsvektor zu einem Richtungswort
void directionVector(const std::string& dir, int& dx, int& dy) {          // Beginn von directionVector
    std::string d = PropertyFile::toLower(dir);                           // Klein geschrieben
    if (d == "nord") { dx = 0; dy = -1; }                                 // Norden: -y
    else if (d == "sued") { dx = 0; dy = 1; }                             // Süden: +y
    else if (d == "west") { dx = -1; dy = 0; }                            // Westen: -x
    else { dx = 1; dy = 0; }                                              // Osten (Standard): +x
} // Ende von directionVector

// Ist die Bodenart Land (ohne Steg)?
bool isSolidLand(Terrain t) { return t == Terrain::Sand || t == Terrain::Grass || t == Terrain::Rock || t == Terrain::Path || t == Terrain::Plaza; } // Landarten

// Zeichnet eine Raute (Bodenkachel) zeilenweise
void fillDiamond(Canvas& c, float cx, float top, float w, float h, Color col) { // Beginn von fillDiamond
    int rows = static_cast<int>(h + 0.5f);                                // Anzahl der Zeilen
    int y0 = static_cast<int>(std::floor(top));                           // Oberste Zeile
    for (int r = 0; r <= rows; ++r) {                                     // Alle Zeilen (eine extra gegen Lücken)
        float rel = (static_cast<float>(r) + 0.5f) / h;                   // Relative Höhe 0..1
        float half = (rel < 0.5f ? rel : 1.0f - rel) * w + 0.75f;         // Halbe Breite der Zeile (leicht überlappend)
        if (half <= 0.0f) continue;                                       // Leere Zeile
        int x0 = static_cast<int>(std::floor(cx - half));                 // Linke Kante
        int x1 = static_cast<int>(std::ceil(cx + half));                  // Rechte Kante
        c.fillRect(x0, y0 + r, x1 - x0, 1, col);                          // Zeile füllen
    }                                                                     // Ende der Zeilenschleife
} // Ende von fillDiamond

// Multipliziert die Helligkeit einer Farbe
Color bright(Color c, float f) { return shade(c, f); }                    // Kurzform für shade

} // Ende des internen Namensraums

// Welt -> Bildschirm x
float Camera::toScreenX(float wx, float wy) const { return static_cast<float>(screenW) * 0.5f + Iso::screenX(wx - x, wy - y, tileWidth); } // Relativ zur Kamera
// Welt -> Bildschirm y
float Camera::toScreenY(float wx, float wy, float z) const { return static_cast<float>(screenH) * 0.5f + Iso::screenY(wx - x, wy - y, z, tileWidth); } // Relativ zur Kamera
// Bildschirm -> Boden
void Camera::toWorld(float sx, float sy, float& wx, float& wy) const {    // Beginn von toWorld
    float dx = 0.0f, dy = 0.0f;                                           // Abstand zur Kamera in Kacheln
    Iso::screenToGround(sx - static_cast<float>(screenW) * 0.5f, sy - static_cast<float>(screenH) * 0.5f, tileWidth, dx, dy); // Umrechnen
    wx = x + dx;                                                          // Weltposition x
    wy = y + dy;                                                          // Weltposition y
} // Ende von toWorld

// Kachel lesen (außerhalb der Karte: tiefes Wasser)
const Tile& World::tile(int x, int y) const {                             // Beginn von tile
    if (x < 0 || y < 0 || x >= m_w || y >= m_h) return m_outside;         // Außerhalb
    return m_tiles[static_cast<std::size_t>(y) * m_w + x];                // Kachel zurückgeben
} // Ende von tile

// Kachel zum Ändern
Tile& World::at(int x, int y) {                                           // Beginn von at
    if (x < 0 || y < 0 || x >= m_w || y >= m_h) return m_outside;         // Außerhalb (Änderungen gehen ins Leere)
    return m_tiles[static_cast<std::size_t>(y) * m_w + x];                // Kachel zurückgeben
} // Ende von at

// Ist hier Land oder ein Steg?
bool World::isLand(int x, int y) const {                                  // Beginn von isLand
    Terrain t = tile(x, y).terrain;                                       // Bodenart
    return isSolidLand(t) || t == Terrain::Pier;                          // Land oder Steg
} // Ende von isLand

bool World::walkable(int x, int y) const { return isLand(x, y) && !tile(x, y).blocked; } // Land und nicht belegt

// Wassertiefe an einer Position
float World::depthAt(float x, float y) const {                            // Beginn von depthAt
    const Tile& t = tile(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y))); // Kachel
    return isSolidLand(t.terrain) ? 0.0f : t.depth;                       // An Land 0
} // Ende von depthAt

// Kann ein Schiff mit diesem Tiefgang hier fahren?
bool World::sailable(float x, float y, float draft) const {               // Beginn von sailable
    if (x < 0.5f || y < 0.5f || x > static_cast<float>(m_w) - 0.5f || y > static_cast<float>(m_h) - 0.5f) return false; // Kartenrand
    const Tile& t = tile(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y))); // Kachel
    if (isSolidLand(t.terrain) || t.terrain == Terrain::Pier || t.blocked) return false; // Land, Steg oder Felsen
    return t.depth >= draft;                                              // Tief genug?
} // Ende von sailable

// Insel an einer Position
int World::islandAt(float x, float y) const {                             // Beginn von islandAt
    const Tile& t = tile(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y))); // Kachel
    return static_cast<int>(t.island) - 1;                                // -1 = keine Insel
} // Ende von islandAt

// Index einer Insel über ihre ID
int World::islandIndex(const std::string& id) const {                     // Beginn von islandIndex
    for (std::size_t i = 0; i < islands.size(); ++i) if (PropertyFile::toLower(islands[i].id) == PropertyFile::toLower(id) || PropertyFile::toLower(islands[i].name) == PropertyFile::toLower(id)) return static_cast<int>(i); // Gefunden
    return -1;                                                            // Nicht gefunden
} // Ende von islandIndex

// Nächstes Gebäude, dessen Türpunkt höchstens maxDist entfernt ist
int World::nearestBuilding(float x, float y, float maxDist) const {       // Beginn von nearestBuilding
    int best = -1;                                                        // Bester Kandidat
    float bestD = maxDist * maxDist;                                      // Quadrierter Höchstabstand
    for (std::size_t i = 0; i < buildings.size(); ++i) {                  // Alle Gebäude
        float dx = buildings[i].doorX - x, dy = buildings[i].doorY - y;   // Abstand
        float d = dx * dx + dy * dy;                                      // Quadriert
        if (d < bestD) { bestD = d; best = static_cast<int>(i); }         // Näher
    }                                                                     // Ende der Schleife
    return best;                                                          // Ergebnis
} // Ende von nearestBuilding

// Abstand zur nächsten Handelsroute
float World::routeDistance(float x, float y) const {                      // Beginn von routeDistance
    float best = INF;                                                     // Startwert
    for (const Route& r : routes) {                                       // Alle Routen
        for (std::size_t i = 0; i + 1 < r.points.size(); ++i) {           // Alle Abschnitte
            float ax = r.points[i].first, ay = r.points[i].second;        // Startpunkt
            float bx = r.points[i + 1].first, by = r.points[i + 1].second; // Endpunkt
            float vx = bx - ax, vy = by - ay;                             // Abschnittsvektor
            float len2 = vx * vx + vy * vy;                               // Quadrierte Länge
            float t = len2 > 0 ? clampValue(((x - ax) * vx + (y - ay) * vy) / len2, 0.0f, 1.0f) : 0.0f; // Nächster Punkt auf dem Abschnitt
            float dx = ax + vx * t - x, dy = ay + vy * t - y;             // Abstand
            best = std::min(best, std::sqrt(dx * dx + dy * dy));          // Minimum
        }                                                                 // Ende der Abschnitte
    }                                                                     // Ende der Routen
    return best;                                                          // Ergebnis
} // Ende von routeDistance

// Zone einer bestimmten Art an einer Position
const Zone* World::zoneAt(float x, float y, const std::string& kind) const { // Beginn von zoneAt
    for (const Zone& z : zones) {                                         // Alle Zonen
        if (z.kind != kind) continue;                                     // Andere Art
        float dx = z.x - x, dy = z.y - y;                                 // Abstand
        if (dx * dx + dy * dy <= z.radius * z.radius) return &z;          // Innerhalb
    }                                                                     // Ende der Schleife
    return nullptr;                                                       // Keine Zone
} // Ende von zoneAt

// Zufälliger Punkt auf offener See mit Mindesttiefe
std::pair<float, float> World::randomSeaPoint(unsigned seed, float minDepth) const { // Beginn von randomSeaPoint
    SimpleRandom rnd(seed * 2654435761u + 17u);                           // Zufall
    for (int attempt = 0; attempt < 400; ++attempt) {                     // Mehrere Versuche
        float x = rnd.range(4.0f, static_cast<float>(m_w) - 4.0f);        // Zufälliges x
        float y = rnd.range(4.0f, static_cast<float>(m_h) - 4.0f);        // Zufälliges y
        const Tile& t = tile(static_cast<int>(x), static_cast<int>(y));   // Kachel
        if (!isSolidLand(t.terrain) && t.terrain != Terrain::Pier && !t.blocked && t.depth >= minDepth && t.coast >= 3) return {x, y}; // Geeignet
    }                                                                     // Ende der Versuche
    return {static_cast<float>(m_w) * 0.5f, static_cast<float>(m_h) * 0.5f}; // Ersatz: Kartenmitte
} // Ende von randomSeaPoint

// Ist eine rechteckige Fläche frei und gehört sie zur Insel?
bool World::areaFree(int x0, int y0, int w, int h, int island) const {    // Beginn von areaFree
    for (int y = y0; y < y0 + h; ++y) {                                   // Alle Zeilen
        for (int x = x0; x < x0 + w; ++x) {                               // Alle Spalten
            const Tile& t = tile(x, y);                                   // Kachel
            if (t.island != island + 1 || t.blocked) return false;        // Falsche Insel oder belegt
            if (t.terrain != Terrain::Grass && t.terrain != Terrain::Sand) return false; // Nur auf Gras oder Sand bauen
        }                                                                 // Ende der Spalten
    }                                                                     // Ende der Zeilen
    return true;                                                          // Frei
} // Ende von areaFree

// Erzeugt die Landmasse einer Insel mit unregelmäßiger Küste
void World::generateIsland(Island& isl, int index, unsigned seed) {       // Beginn von generateIsland
    SimpleRandom rnd(seed);                                               // Zufall je Insel
    float a = rnd.range(0.0f, 6.28f), b = rnd.range(0.0f, 6.28f), c = rnd.range(0.0f, 6.28f); // Phasen der Küstenwellen
    int r = static_cast<int>(isl.radius * 1.4f) + 2;                      // Suchbereich
    for (int y = static_cast<int>(isl.cy) - r; y <= static_cast<int>(isl.cy) + r; ++y) { // Zeilen
        for (int x = static_cast<int>(isl.cx) - r; x <= static_cast<int>(isl.cx) + r; ++x) { // Spalten
            float dx = static_cast<float>(x) + 0.5f - isl.cx;             // Abstand x zur Mitte
            float dy = static_cast<float>(y) + 0.5f - isl.cy;             // Abstand y zur Mitte
            float d = std::sqrt(dx * dx + dy * dy);                       // Abstand
            float th = std::atan2(dy, dx);                                // Winkel
            float rr = isl.radius * (1.0f + 0.15f * std::sin(3.0f * th + a) + 0.09f * std::sin(5.0f * th + b) + 0.05f * std::sin(8.0f * th + c)); // Unregelmäßiger Radius
            if (d >= rr) continue;                                        // Außerhalb -> Wasser bleibt
            Tile& t = at(x, y);                                           // Kachel
            t.terrain = Terrain::Grass;                                   // Land (Gras, Strand folgt später)
            t.island = static_cast<std::uint8_t>(index + 1);              // Inselnummer
            t.depth = 0.0f;                                               // Keine Wassertiefe
        }                                                                 // Ende der Spalten
    }                                                                     // Ende der Zeilen
} // Ende von generateIsland

// Berechnet Küstenabstände (Abstandstransformation in zwei Durchgängen) und daraus Wassertiefen
void World::computeDistances() {                                          // Beginn von computeDistances
    std::vector<float> toLand(m_tiles.size(), INF);                       // Abstand jeder Kachel zum Land
    std::vector<float> toWater(m_tiles.size(), INF);                      // Abstand jeder Kachel zum Wasser
    for (int y = 0; y < m_h; ++y) for (int x = 0; x < m_w; ++x) {         // Startwerte
        bool land = isSolidLand(tile(x, y).terrain);                      // Land?
        toLand[static_cast<std::size_t>(y) * m_w + x] = land ? 0.0f : INF; // Land hat Abstand 0 zum Land
        toWater[static_cast<std::size_t>(y) * m_w + x] = land ? INF : 0.0f; // Wasser hat Abstand 0 zum Wasser
    }                                                                     // Ende der Startwerte
    auto pass = [&](std::vector<float>& d) {                              // Zwei Durchgänge der Abstandstransformation
        const float D = 1.4142f;                                          // Diagonalabstand
        for (int y = 0; y < m_h; ++y) for (int x = 0; x < m_w; ++x) {     // Vorwärts (oben links nach unten rechts)
            float& v = d[static_cast<std::size_t>(y) * m_w + x];          // Aktueller Wert
            if (x > 0) v = std::min(v, d[static_cast<std::size_t>(y) * m_w + x - 1] + 1.0f); // Links
            if (y > 0) v = std::min(v, d[static_cast<std::size_t>(y - 1) * m_w + x] + 1.0f); // Oben
            if (x > 0 && y > 0) v = std::min(v, d[static_cast<std::size_t>(y - 1) * m_w + x - 1] + D); // Oben links
            if (x + 1 < m_w && y > 0) v = std::min(v, d[static_cast<std::size_t>(y - 1) * m_w + x + 1] + D); // Oben rechts
        }                                                                 // Ende vorwärts
        for (int y = m_h - 1; y >= 0; --y) for (int x = m_w - 1; x >= 0; --x) { // Rückwärts
            float& v = d[static_cast<std::size_t>(y) * m_w + x];          // Aktueller Wert
            if (x + 1 < m_w) v = std::min(v, d[static_cast<std::size_t>(y) * m_w + x + 1] + 1.0f); // Rechts
            if (y + 1 < m_h) v = std::min(v, d[static_cast<std::size_t>(y + 1) * m_w + x] + 1.0f); // Unten
            if (x + 1 < m_w && y + 1 < m_h) v = std::min(v, d[static_cast<std::size_t>(y + 1) * m_w + x + 1] + D); // Unten rechts
            if (x > 0 && y + 1 < m_h) v = std::min(v, d[static_cast<std::size_t>(y + 1) * m_w + x - 1] + D); // Unten links
        }                                                                 // Ende rückwärts
    };                                                                    // Ende der Hilfsfunktion
    pass(toLand);                                                         // Abstände zum Land
    pass(toWater);                                                        // Abstände zum Wasser
    for (int y = 0; y < m_h; ++y) for (int x = 0; x < m_w; ++x) {         // Alle Kacheln einordnen
        Tile& t = at(x, y);                                               // Kachel
        std::size_t i = static_cast<std::size_t>(y) * m_w + x;            // Index
        if (isSolidLand(t.terrain)) {                                     // Land
            t.coast = static_cast<std::uint8_t>(std::min(255.0f, toWater[i])); // Abstand zum Wasser
            if (t.terrain == Terrain::Grass && toWater[i] <= 1.6f) t.terrain = Terrain::Sand; // Strand am Rand
        } else {                                                          // Wasser
            t.coast = static_cast<std::uint8_t>(std::min(255.0f, toLand[i])); // Abstand zum Land
            float noise = static_cast<float>(t.shade % 16) * 0.08f;       // Leichte Unregelmäßigkeit
            t.depth = std::min(40.0f, 0.6f + toLand[i] * 2.0f + noise);   // Tiefe wächst mit dem Abstand zur Küste
            t.terrain = t.depth < 2.2f ? Terrain::Shallow : (t.depth < 7.0f ? Terrain::Water : Terrain::DeepWater); // Wasserart
        }                                                                 // Ende der Unterscheidung
    }                                                                     // Ende der Schleife
} // Ende von computeDistances

// Macht Riffzonen flach und setzt Felsen ins Wasser
void World::applyZones() {                                                // Beginn von applyZones
    unsigned n = 0;                                                       // Zähler für den Zufall
    for (const Zone& z : zones) {                                         // Alle Zonen
        if (z.kind != "riff") continue;                                   // Nur Riffe
        SimpleRandom rnd(static_cast<std::uint32_t>(z.x * 31 + z.y * 17) + (++n)); // Zufall je Riff
        int r = static_cast<int>(z.radius) + 1;                           // Suchradius
        for (int y = static_cast<int>(z.y) - r; y <= static_cast<int>(z.y) + r; ++y) { // Zeilen
            for (int x = static_cast<int>(z.x) - r; x <= static_cast<int>(z.x) + r; ++x) { // Spalten
                Tile& t = at(x, y);                                       // Kachel
                if (isSolidLand(t.terrain)) continue;                     // Land bleibt
                float dx = static_cast<float>(x) + 0.5f - z.x, dy = static_cast<float>(y) + 0.5f - z.y; // Abstand
                float d = std::sqrt(dx * dx + dy * dy) / z.radius;        // Relativer Abstand 0..1
                if (d > 1.0f) continue;                                   // Außerhalb
                t.depth = std::min(t.depth, 0.5f + d * d * 2.5f + rnd.range(0.0f, 0.6f)); // Flach, in der Mitte am flachsten
                t.terrain = Terrain::Shallow;                             // Flachwasser
                if (d < 0.8f && rnd.nextFloat() < 0.12f) {                // Manchmal ein Felsen
                    WorldObject o;                                        // Neues Objekt
                    o.model = rnd.nextFloat() < 0.5f ? "felsen_wasser" : "felsen_wasser_gross"; // Felsmodell
                    o.x = static_cast<float>(x) + 0.5f;                   // Position x
                    o.y = static_cast<float>(y) + 0.5f;                   // Position y
                    o.angle = static_cast<float>(rnd.rangeInt(0, 3)) * 1.5708f; // Zufällige Drehung
                    objects.push_back(o);                                 // Speichern
                    t.blocked = true;                                     // Felsen blockiert Schiffe
                }                                                         // Ende Felsen
            }                                                             // Ende der Spalten
        }                                                                 // Ende der Zeilen
    }                                                                     // Ende der Zonen
} // Ende von applyZones

// Legt Steg, Liegeplatz, Marktplatz und Weg einer Insel an
void World::buildHarbor(Island& isl, int index) {                        // Beginn von buildHarbor
    int dx = 1, dy = 0;                                                   // Richtung des Hafens
    directionVector(isl.harborDir, dx, dy);                               // Aus dem Richtungswort
    int cx = static_cast<int>(isl.cx), cy = static_cast<int>(isl.cy);     // Mittelpunkt als Kachel
    int coastX = cx, coastY = cy;                                         // Letzte Landkachel in Hafenrichtung
    for (int k = 0; k < static_cast<int>(isl.radius * 2.0f); ++k) {       // Nach außen laufen
        int x = cx + dx * k, y = cy + dy * k;                             // Kachel
        if (!isSolidLand(tile(x, y).terrain)) break;                      // Wasser erreicht
        coastX = x; coastY = y;                                           // Letzte Landkachel merken
    }                                                                     // Ende der Suche
    const int pierLen = 4;                                                // Steglänge in Kacheln
    for (int k = 1; k <= pierLen; ++k) {                                  // Steg ins Wasser bauen
        Tile& t = at(coastX + dx * k, coastY + dy * k);                   // Kachel
        t.terrain = Terrain::Pier;                                        // Steg
        t.island = static_cast<std::uint8_t>(index + 1);                  // Gehört zur Insel
        t.depth = std::max(t.depth, 3.0f);                                // Wasser darunter
    }                                                                     // Ende des Stegs
    isl.pierBaseX = static_cast<float>(coastX) + 0.5f;                    // Stegbeginn x
    isl.pierBaseY = static_cast<float>(coastY) + 0.5f;                    // Stegbeginn y
    isl.pierEndX = static_cast<float>(coastX + dx * pierLen) + 0.5f;      // Stegende x
    isl.pierEndY = static_cast<float>(coastY + dy * pierLen) + 0.5f;      // Stegende y
    float perpX = static_cast<float>(-dy), perpY = static_cast<float>(dx); // Senkrecht zum Steg
    isl.dockX = isl.pierEndX + perpX * 1.6f - static_cast<float>(dx) * 0.6f; // Liegeplatz längsseits am Stegende
    isl.dockY = isl.pierEndY + perpY * 1.6f - static_cast<float>(dy) * 0.6f; // Liegeplatz y
    isl.dockAngle = std::atan2(static_cast<float>(dy), static_cast<float>(dx)); // Bug zeigt vom Land weg
    for (int y = static_cast<int>(isl.dockY) - 4; y <= static_cast<int>(isl.dockY) + 4; ++y) { // Hafenbecken vertiefen
        for (int x = static_cast<int>(isl.dockX) - 4; x <= static_cast<int>(isl.dockX) + 4; ++x) { // Spalten
            Tile& t = at(x, y);                                           // Kachel
            if (isSolidLand(t.terrain) || t.terrain == Terrain::Pier) continue; // Land und Steg bleiben
            t.depth = std::max(t.depth, 4.0f);                            // Mindestens 4 m
            if (t.terrain == Terrain::Shallow) t.terrain = Terrain::Water; // Kein Flachwasser mehr
        }                                                                 // Ende der Spalten
    }                                                                     // Ende der Zeilen
    for (int k = 0; k < 10; ++k) {                                        // Ausfahrt vom Liegeplatz nach außen vertiefen
        for (int s = -2; s <= 2; ++s) {                                   // Breite der Ausfahrt
            Tile& t = at(static_cast<int>(isl.dockX + static_cast<float>(dx * k) + perpX * s), static_cast<int>(isl.dockY + static_cast<float>(dy * k) + perpY * s)); // Kachel
            if (isSolidLand(t.terrain) || t.terrain == Terrain::Pier) continue; // Land und Steg bleiben
            t.depth = std::max(t.depth, 4.0f);                            // Mindestens 4 m
            if (t.terrain == Terrain::Shallow) t.terrain = Terrain::Water; // Kein Flachwasser mehr
        }                                                                 // Ende der Breite
    }                                                                     // Ende der Ausfahrt
    int px = coastX - dx * 4, py = coastY - dy * 4;                       // Mitte des Marktplatzes (landeinwärts)
    for (int y = py - 2; y <= py + 2; ++y) {                              // Platz 5x5
        for (int x = px - 2; x <= px + 2; ++x) {                          // Spalten
            Tile& t = at(x, y);                                           // Kachel
            if (t.island == index + 1 && isSolidLand(t.terrain)) t.terrain = Terrain::Plaza; // Pflaster
        }                                                                 // Ende der Spalten
    }                                                                     // Ende der Zeilen
    for (int k = 0; k <= 2; ++k) {                                        // Weg vom Steg zum Platz
        Tile& t = at(coastX - dx * k, coastY - dy * k);                   // Kachel
        if (isSolidLand(t.terrain)) t.terrain = Terrain::Path;            // Weg
    }                                                                     // Ende des Weges
    isl.plazaX = static_cast<float>(px) + 0.5f;                           // Platzmitte x
    isl.plazaY = static_cast<float>(py) + 0.5f;                           // Platzmitte y
    WorldObject lantern;                                                  // Laternen an den Platzecken
    lantern.model = "laterne";                                            // Modell
    lantern.z = LAND_HEIGHT;                                              // Auf dem Boden
    const int corners[4][2] = {{-2, -2}, {2, -2}, {-2, 2}, {2, 2}};       // Ecken
    for (const auto& c : corners) {                                       // Alle Ecken
        lantern.x = static_cast<float>(px + c[0]) + 0.5f;                 // Position x
        lantern.y = static_cast<float>(py + c[1]) + 0.5f;                 // Position y
        objects.push_back(lantern);                                       // Speichern
        at(px + c[0], py + c[1]).blocked = true;                          // Laterne blockiert die Kachel
    }                                                                     // Ende der Ecken
    WorldObject post;                                                     // Pfosten am Stegende
    post.model = "steg_pfosten";                                          // Modell
    post.z = 0.0f;                                                        // Steht im Wasser
    for (int k = 1; k <= pierLen; k += 3) {                               // Einige Pfosten entlang des Stegs
        post.x = static_cast<float>(coastX + dx * k) + 0.5f + perpX * 0.55f; // Seitlich neben dem Steg
        post.y = static_cast<float>(coastY + dy * k) + 0.5f + perpY * 0.55f; // Position y
        objects.push_back(post);                                          // Speichern
    }                                                                     // Ende der Pfosten
} // Ende von buildHarbor

// Verteilt Gebäude und Marktstände um den Platz
void World::placeBuildings(Island& isl, int index, const PropertyFile& f) { // Beginn von placeBuildings
    int dx = 1, dy = 0;                                                   // Hafenrichtung
    directionVector(isl.harborDir, dx, dy);                               // Aus dem Richtungswort
    int qx = -dy, qy = dx;                                                // Senkrecht dazu
    int px = static_cast<int>(isl.plazaX), py = static_cast<int>(isl.plazaY); // Platzmitte als Kachel
    std::vector<std::pair<std::string, std::string>> wanted;              // Gewünschte Gebäude (Art, Referenz)
    for (const std::string& g : f.getList(isl.id, "gebaeude")) {          // Feste Gebäude (Kontor, Museum ...)
        std::string low = PropertyFile::toLower(g);                       // Klein geschrieben
        if (low.rfind("hersteller:", 0) == 0) wanted.push_back({"hersteller", g.substr(11)}); // Hersteller
        else if (low.rfind("laden:", 0) == 0) wanted.push_back({"haendler", g.substr(6)}); // Händler mit eigenem Laden
        else wanted.push_back({low, ""});                                 // Normales Gebäude
    }                                                                     // Ende der Gebäudeliste
    const int slots[][2] = {{-5, 0}, {0, 5}, {0, -5}, {-5, 5}, {-5, -5}, {-9, 0}, {3, 6}, {3, -6}, {-9, 5}, {-9, -5}, {-5, 9}, {-5, -9}}; // Plätze (entlang Hafen, seitlich)
    std::size_t slot = 0;                                                 // Nächster zu prüfender Platz
    for (const auto& w : wanted) {                                        // Alle gewünschten Gebäude
        bool placed = false;                                              // Platz gefunden?
        while (!placed && slot < sizeof(slots) / sizeof(slots[0])) {      // Plätze durchprobieren
            int bx = px + dx * slots[slot][0] + qx * slots[slot][1];      // Mitte x
            int by = py + dy * slots[slot][0] + qy * slots[slot][1];      // Mitte y
            ++slot;                                                       // Nächster Platz
            if (!areaFree(bx - 1, by - 1, 3, 3, index)) continue;         // Fläche nicht frei
            Building b;                                                   // Neues Gebäude
            b.type = w.first;                                             // Art
            b.ref = PropertyFile::trim(w.second);                         // Referenz
            b.island = index;                                             // Insel
            b.x = static_cast<float>(bx) + 0.5f;                          // Mitte x
            b.y = static_cast<float>(by) + 0.5f;                          // Mitte y
            float ax = isl.plazaX - b.x, ay = isl.plazaY - b.y;           // Richtung zum Platz
            if (std::fabs(ax) > std::fabs(ay)) { ax = ax > 0 ? 1.0f : -1.0f; ay = 0.0f; } // Auf Hauptachse runden
            else { ay = ay > 0 ? 1.0f : -1.0f; ax = 0.0f; }               // Auf Hauptachse runden
            b.facing = std::atan2(ay, ax);                                // Tür zeigt zum Platz
            float sideX = ay, sideY = -ax;                                // Modell-x-Achse in der Welt (siehe Hausgenerator)
            b.doorX = b.x + ax * 2.1f - sideX * 0.75f;                    // Vor der Tür (die Tür sitzt in der linken Zelle)
            b.doorY = b.y + ay * 2.1f - sideY * 0.75f;                    // Vor der Tür y
            for (int y = by - 1; y <= by + 1; ++y) for (int x = bx - 1; x <= bx + 1; ++x) at(x, y).blocked = true; // Fläche belegen
            WorldObject o;                                                // Hausmodell
            o.model = "gebaeude_" + (b.ref.empty() ? b.type : PropertyFile::toLower(b.ref)); // z.B. gebaeude_kontor
            if (b.type == "haendler") o.model = "gebaeude_laden";         // Händlerladen
            b.model = o.model;                                            // Modell am Gebäude merken
            o.x = b.x;                                                    // Position x
            o.y = b.y;                                                    // Position y
            o.z = LAND_HEIGHT;                                            // Auf dem Boden
            o.angle = b.facing;                                           // Ausrichtung
            objects.push_back(o);                                         // Speichern
            if (b.type == "museum") isl.hasMuseum = true;                 // Museum vorhanden
            if (b.type == "werft") isl.hasShipyard = true;                // Werft vorhanden
            if (b.type == "taverne") isl.hasTavern = true;                // Taverne vorhanden
            isl.buildings.push_back(static_cast<int>(buildings.size()));  // Bei der Insel eintragen
            buildings.push_back(b);                                       // Speichern
            placed = true;                                                // Fertig
        }                                                                 // Ende der Platzsuche
    }                                                                     // Ende der Gebäude
    std::vector<std::string> stalls = f.getList(isl.id, "markt");         // Marktstände auf dem Platz
    for (std::size_t k = 0; k < stalls.size() && k < 3; ++k) {            // Höchstens drei Stände
        float off = (static_cast<float>(k) - static_cast<float>(std::min<std::size_t>(stalls.size(), 3) - 1) * 0.5f) * 2.0f; // Seitlicher Versatz
        Building b;                                                       // Neuer Stand
        b.type = "haendler";                                              // Händler
        b.ref = stalls[k];                                                // Händlerart
        b.island = index;                                                 // Insel
        b.x = isl.plazaX - static_cast<float>(dx) * 1.2f + static_cast<float>(qx) * off; // Position x (landseitige Platzhälfte)
        b.y = isl.plazaY - static_cast<float>(dy) * 1.2f + static_cast<float>(qy) * off; // Position y
        b.facing = std::atan2(static_cast<float>(dy), static_cast<float>(dx)); // Stand zeigt zum Hafen
        b.doorX = b.x + static_cast<float>(dx) * 1.1f;                    // Kunde steht vor dem Stand
        b.doorY = b.y + static_cast<float>(dy) * 1.1f;                    // Kunde y
        at(static_cast<int>(b.x), static_cast<int>(b.y)).blocked = true;  // Stand blockiert
        WorldObject o;                                                    // Standmodell
        o.model = k % 2 == 0 ? "marktstand_gruen" : "marktstand_rot";     // Abwechselnd grün und rot
        b.model = o.model;                                                // Modell am Stand merken
        o.x = b.x;                                                        // Position x
        o.y = b.y;                                                        // Position y
        o.z = LAND_HEIGHT;                                                // Auf dem Boden
        o.angle = b.facing;                                               // Ausrichtung
        objects.push_back(o);                                             // Speichern
        isl.buildings.push_back(static_cast<int>(buildings.size()));      // Bei der Insel eintragen
        buildings.push_back(b);                                           // Speichern
    }                                                                     // Ende der Stände
} // Ende von placeBuildings

// Setzt Bäume, Steine, Büsche und Blumen je nach Landschaft
void World::decorate(Island& isl, int index, unsigned seed) {             // Beginn von decorate
    SimpleRandom rnd(seed);                                               // Zufall je Insel
    std::string style = PropertyFile::toLower(isl.landscape);             // Landschaftsstil
    int r = static_cast<int>(isl.radius * 1.4f) + 2;                      // Suchbereich
    for (int y = static_cast<int>(isl.cy) - r; y <= static_cast<int>(isl.cy) + r; ++y) { // Zeilen
        for (int x = static_cast<int>(isl.cx) - r; x <= static_cast<int>(isl.cx) + r; ++x) { // Spalten
            Tile& t = at(x, y);                                           // Kachel
            if (t.island != index + 1 || t.blocked) continue;             // Fremde oder belegte Kachel
            if (t.terrain != Terrain::Grass && t.terrain != Terrain::Sand) continue; // Nur auf Gras und Sand
            bool nearUse = false;                                         // Nähe zu Weg, Platz oder Steg?
            for (int yy = y - 1; yy <= y + 1 && !nearUse; ++yy) for (int xx = x - 1; xx <= x + 1; ++xx) { Terrain n = tile(xx, yy).terrain; if (n == Terrain::Path || n == Terrain::Plaza || n == Terrain::Pier) { nearUse = true; break; } } // Nachbarn prüfen
            if (nearUse) continue;                                        // Wege freihalten
            float roll = rnd.nextFloat();                                 // Würfeln
            std::string model;                                            // Gewähltes Modell
            bool solid = true;                                            // Blockiert das Objekt?
            bool sand = t.terrain == Terrain::Sand;                       // Strand?
            if (style == "palmen") {                                      // Palmeninsel
                if (roll < (sand ? 0.10f : 0.16f)) model = rnd.nextFloat() < 0.5f ? "palme" : "palme_kurz"; // Palmen
                else if (!sand && roll < 0.22f) { model = "busch"; solid = false; } // Büsche
                else if (!sand && roll < 0.26f) { model = "blumen"; solid = false; } // Blumen
                else if (roll < 0.28f) model = "stein";                   // Steine
            } else if (style == "wald") {                                 // Waldinsel
                if (!sand && roll < 0.22f) { float k = rnd.nextFloat(); model = k < 0.35f ? "baum" : (k < 0.7f ? "baum_eiche" : "baum_tanne"); } // Bäume
                else if (!sand && roll < 0.27f) { model = "baumstumpf"; } // Baumstümpfe (Holzfäller)
                else if (roll < 0.31f) { model = "busch"; solid = false; } // Büsche
                else if (roll < 0.33f) model = "stein";                   // Steine
            } else if (style == "grusel") {                               // Unheimliche Insel
                if (!sand && roll < 0.14f) model = rnd.nextFloat() < 0.5f ? "baum_tot" : "baum_tot_krumm"; // Tote Bäume
                else if (!sand && roll < 0.22f) model = rnd.nextFloat() < 0.5f ? "grabstein" : "grabstein_kreuz"; // Grabsteine
                else if (roll < 0.26f) model = "stein";                   // Steine
            } else {                                                      // Stadtinsel
                if (!sand && roll < 0.08f) model = rnd.nextFloat() < 0.5f ? "baum" : "baum_rund"; // Wenige Bäume
                else if (!sand && roll < 0.13f) { model = "busch"; solid = false; } // Büsche
                else if (!sand && roll < 0.17f) { model = "blumen"; solid = false; } // Blumen
                else if (sand && roll < 0.05f) model = "palme_kurz";      // Palmen am Strand
            }                                                             // Ende der Stile
            if (model.empty()) continue;                                  // Nichts gesetzt
            WorldObject o;                                                // Neues Objekt
            o.model = model;                                              // Modell
            o.x = static_cast<float>(x) + rnd.range(0.35f, 0.65f);        // Position x mit leichter Streuung
            o.y = static_cast<float>(y) + rnd.range(0.35f, 0.65f);        // Position y
            o.z = LAND_HEIGHT;                                            // Auf dem Boden
            o.angle = static_cast<float>(rnd.rangeInt(0, 3)) * 1.5708f;   // Zufällige Drehung
            objects.push_back(o);                                         // Speichern
            if (solid) t.blocked = true;                                  // Kachel belegen
        }                                                                 // Ende der Spalten
    }                                                                     // Ende der Zeilen
} // Ende von decorate

// Setzt Bojen entlang der Handelsrouten
void World::placeBuoys() {                                                // Beginn von placeBuoys
    for (const Route& r : routes) {                                       // Alle Routen
        float carry = 3.0f;                                               // Abstand bis zur nächsten Boje
        for (std::size_t i = 0; i + 1 < r.points.size(); ++i) {           // Alle Abschnitte
            float ax = r.points[i].first, ay = r.points[i].second;        // Start
            float bx = r.points[i + 1].first, by = r.points[i + 1].second; // Ende
            float len = std::sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay)); // Länge
            for (float s = carry; s < len; s += 7.0f) {                   // Alle 7 Kacheln eine Boje
                float t = s / len;                                        // Anteil
                float x = ax + (bx - ax) * t, y = ay + (by - ay) * t;     // Position
                float nx = -(by - ay) / len * 1.2f, ny = (bx - ax) / len * 1.2f; // Seitlich versetzt
                if (depthAt(x + nx, y + ny) < 3.0f) continue;             // Nicht ins Flache
                WorldObject o;                                            // Neue Boje
                o.model = "boje";                                         // Modell
                o.x = x + nx;                                             // Position x
                o.y = y + ny;                                             // Position y
                o.bob = true;                                             // Schaukelt
                objects.push_back(o);                                     // Speichern
                carry = s + 7.0f - len;                                   // Rest für den nächsten Abschnitt
            }                                                             // Ende der Bojen
        }                                                                 // Ende der Abschnitte
    }                                                                     // Ende der Routen
} // Ende von placeBuoys

// Lädt die Weltbeschreibung und erzeugt daraus die Karte
bool World::load(const std::string& path, std::string& error) {           // Beginn von load
    PropertyFile f;                                                       // Datei-Objekt
    if (!f.load(path)) { error = "data/welt.txt fehlt"; return false; }   // Fehler
    m_w = clampValue(f.getInt("Welt", "breite", 128), 48, 400);           // Breite
    m_h = clampValue(f.getInt("Welt", "hoehe", 128), 48, 400);            // Höhe
    unsigned seed = static_cast<unsigned>(f.getInt("Welt", "seed", 42));  // Startwert für den Zufall
    startIsland = f.getString("Welt", "start_insel", "");                 // Startinsel
    m_tiles.assign(static_cast<std::size_t>(m_w) * m_h, Tile());          // Alles tiefes Wasser
    SimpleRandom rnd(seed);                                               // Zufall
    for (Tile& t : m_tiles) t.shade = static_cast<std::uint8_t>(rnd.rangeInt(0, 255)); // Farbabweichung je Kachel
    islands.clear(); buildings.clear(); objects.clear(); zones.clear(); routes.clear(); // Alte Daten löschen
    for (const std::string& id : f.sectionsWithPrefix("Insel")) {         // Alle Inseln
        Island isl;                                                       // Neue Insel
        isl.id = id;                                                      // ID
        isl.name = f.getString(id, "name", id);                           // Name
        isl.type = f.getString(id, "typ", "dorf");                        // Art
        isl.landscape = f.getString(id, "landschaft", "palmen");          // Bewuchs
        isl.cx = f.getFloat(id, "x", 50.0f);                              // Mitte x
        isl.cy = f.getFloat(id, "y", 50.0f);                              // Mitte y
        isl.radius = clampValue(f.getFloat(id, "radius", 8.0f), 6.0f, 30.0f); // Radius
        isl.harborDir = f.getString(id, "hafen", "ost");                  // Hafenrichtung
        isl.produces = f.getList(id, "produziert");                       // Eigene Waren
        isl.demands = f.getList(id, "bedarf");                            // Gefragte Waren
        islands.push_back(isl);                                           // Speichern
    }                                                                     // Ende der Inseln
    if (islands.empty()) { error = "Keine Inseln in data/welt.txt"; return false; } // Mindestens eine Insel
    for (std::size_t i = 0; i < islands.size(); ++i) generateIsland(islands[i], static_cast<int>(i), seed + static_cast<unsigned>(i) * 101u); // Landmassen
    for (const std::string& id : f.sectionNames()) {                      // Zonen
        std::string low = PropertyFile::toLower(id);                      // Klein geschrieben
        std::string kind = low.rfind("riff", 0) == 0 ? "riff" : (low.rfind("piraten", 0) == 0 ? "piraten" : (low.rfind("monster", 0) == 0 ? "monster" : "")); // Art
        if (kind.empty()) continue;                                       // Keine Zone
        Zone z;                                                           // Neue Zone
        z.kind = kind;                                                    // Art
        z.x = f.getFloat(id, "x", 50.0f);                                 // Mitte x
        z.y = f.getFloat(id, "y", 50.0f);                                 // Mitte y
        z.radius = f.getFloat(id, "radius", 6.0f);                        // Radius
        z.strength = f.getFloat(id, "staerke", 1.0f);                     // Stärke
        zones.push_back(z);                                               // Speichern
    }                                                                     // Ende der Zonen
    computeDistances();                                                   // Küsten und Tiefen
    applyZones();                                                         // Riffe
    for (std::size_t i = 0; i < islands.size(); ++i) buildHarbor(islands[i], static_cast<int>(i)); // Häfen
    for (std::size_t i = 0; i < islands.size(); ++i) placeBuildings(islands[i], static_cast<int>(i), f); // Gebäude
    for (std::size_t i = 0; i < islands.size(); ++i) decorate(islands[i], static_cast<int>(i), seed * 7u + static_cast<unsigned>(i)); // Bewuchs
    for (const std::string& id : f.sectionsWithPrefix("Route")) {         // Handelsrouten
        int a = islandIndex(f.getString(id, "von", ""));                  // Startinsel
        int b = islandIndex(f.getString(id, "nach", ""));                 // Zielinsel
        if (a < 0 || b < 0) continue;                                     // Ungültig
        Route r;                                                          // Neue Route
        r.name = f.getString(id, "name", islands[static_cast<std::size_t>(a)].name + " - " + islands[static_cast<std::size_t>(b)].name); // Name
        const Island& ia = islands[static_cast<std::size_t>(a)];          // Startinsel
        const Island& ib = islands[static_cast<std::size_t>(b)];          // Zielinsel
        float ax = ia.dockX + std::cos(ia.dockAngle) * 4.0f, ay = ia.dockY + std::sin(ia.dockAngle) * 4.0f; // Vor dem Hafen A
        r.points.push_back({ax, ay});                                     // Startpunkt
        for (const std::string& wp : f.getList(id, "ueber")) {            // Zwischenpunkte "x y"
            float x = 0, y = 0;                                           // Koordinaten
            if (std::sscanf(wp.c_str(), "%f %f", &x, &y) == 2) r.points.push_back({x, y}); // Speichern
        }                                                                 // Ende der Zwischenpunkte
        r.points.push_back({ib.dockX + std::cos(ib.dockAngle) * 4.0f, ib.dockY + std::sin(ib.dockAngle) * 4.0f}); // Vor dem Hafen B
        routes.push_back(r);                                              // Speichern
    }                                                                     // Ende der Routen
    placeBuoys();                                                         // Bojen
    return true;                                                          // Erfolgreich
} // Ende von load

// Sucht einen Fußweg mit dem A*-Verfahren (8 Richtungen)
std::vector<std::pair<float, float>> World::findPath(float sx, float sy, float tx, float ty) const { // Beginn von findPath
    std::vector<std::pair<float, float>> path;                            // Ergebnis
    int startX = static_cast<int>(sx), startY = static_cast<int>(sy);     // Startkachel
    int goalX = static_cast<int>(tx), goalY = static_cast<int>(ty);       // Zielkachel
    if (!walkable(goalX, goalY)) {                                        // Ziel nicht begehbar -> nächste begehbare Kachel suchen
        float best = INF;                                                 // Bester Abstand
        int bx = -1, by = -1;                                             // Bester Kandidat
        for (int y = goalY - 2; y <= goalY + 2; ++y) for (int x = goalX - 2; x <= goalX + 2; ++x) { // Umgebung
            if (!walkable(x, y)) continue;                                // Nicht begehbar
            float d = static_cast<float>((x - goalX) * (x - goalX) + (y - goalY) * (y - goalY)); // Abstand
            if (d < best) { best = d; bx = x; by = y; }                   // Merken
        }                                                                 // Ende der Umgebung
        if (bx < 0) return path;                                          // Nichts gefunden
        goalX = bx; goalY = by;                                           // Neues Ziel
        tx = static_cast<float>(bx) + 0.5f; ty = static_cast<float>(by) + 0.5f; // Zielpunkt
    }                                                                     // Ende der Zielkorrektur
    if (startX == goalX && startY == goalY) { path.push_back({tx, ty}); return path; } // Schon da
    std::vector<float> cost(m_tiles.size(), INF);                         // Bisherige Kosten
    std::vector<int> from(m_tiles.size(), -1);                            // Vorgänger
    using Node = std::pair<float, int>;                                   // (Schätzung, Index)
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open; // Offene Liste (kleinste Schätzung zuerst)
    auto idx = [&](int x, int y) { return y * m_w + x; };                 // Index berechnen
    auto heuristic = [&](int x, int y) { float dx = std::fabs(static_cast<float>(x - goalX)), dy = std::fabs(static_cast<float>(y - goalY)); return std::max(dx, dy) + 0.414f * std::min(dx, dy); }; // Abstandsschätzung
    if (startX < 0 || startY < 0 || startX >= m_w || startY >= m_h) return path; // Start außerhalb
    cost[static_cast<std::size_t>(idx(startX, startY))] = 0.0f;           // Start kostet nichts
    open.push({heuristic(startX, startY), idx(startX, startY)});          // Start in die offene Liste
    int expanded = 0;                                                     // Zähler gegen Endlosschleifen
    const int ndx[8] = {1, -1, 0, 0, 1, 1, -1, -1};                       // Nachbarn x
    const int ndy[8] = {0, 0, 1, -1, 1, -1, 1, -1};                       // Nachbarn y
    bool found = false;                                                   // Ziel erreicht?
    while (!open.empty() && expanded < 30000) {                           // Suchschleife
        int cur = open.top().second;                                      // Bester Knoten
        open.pop();                                                       // Entfernen
        ++expanded;                                                       // Zählen
        int cx = cur % m_w, cy = cur / m_w;                               // Koordinaten
        if (cx == goalX && cy == goalY) { found = true; break; }          // Ziel erreicht
        for (int k = 0; k < 8; ++k) {                                     // Alle Nachbarn
            int nx = cx + ndx[k], ny = cy + ndy[k];                       // Nachbar
            if (!walkable(nx, ny)) continue;                              // Nicht begehbar
            if (k >= 4 && (!walkable(cx + ndx[k], cy) || !walkable(cx, cy + ndy[k]))) continue; // Keine Ecken schneiden
            float step = k >= 4 ? 1.4142f : 1.0f;                         // Schrittkosten
            float nc = cost[static_cast<std::size_t>(cur)] + step;        // Neue Kosten
            int ni = idx(nx, ny);                                         // Index
            if (nc >= cost[static_cast<std::size_t>(ni)]) continue;       // Kein besserer Weg
            cost[static_cast<std::size_t>(ni)] = nc;                      // Kosten merken
            from[static_cast<std::size_t>(ni)] = cur;                     // Vorgänger merken
            open.push({nc + heuristic(nx, ny), ni});                      // In die offene Liste
        }                                                                 // Ende der Nachbarn
    }                                                                     // Ende der Suche
    if (!found) return path;                                              // Kein Weg
    int cur = idx(goalX, goalY);                                          // Vom Ziel zurückverfolgen
    while (cur >= 0 && cur != idx(startX, startY)) {                      // Bis zum Start
        path.push_back({static_cast<float>(cur % m_w) + 0.5f, static_cast<float>(cur / m_w) + 0.5f}); // Kachelmitte
        cur = from[static_cast<std::size_t>(cur)];                        // Vorgänger
    }                                                                     // Ende der Rückverfolgung
    std::reverse(path.begin(), path.end());                               // Richtige Reihenfolge
    if (!path.empty()) path.back() = {tx, ty};                            // Genauer Zielpunkt
    return path;                                                          // Weg zurückgeben
} // Ende von findPath

// Prüft, ob ein Schiff auf gerader Linie fahren kann (mit etwas Abstand zur Küste)
bool World::clearLine(float ax, float ay, float bx, float by, float draft) const { // Beginn von clearLine
    float len = std::sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay));  // Länge
    int steps = std::max(1, static_cast<int>(len / 0.4f));                // Prüfpunkte alle 0,4 Kacheln
    for (int i = 0; i <= steps; ++i) {                                    // Alle Prüfpunkte
        float t = static_cast<float>(i) / static_cast<float>(steps);      // Anteil
        float x = ax + (bx - ax) * t, y = ay + (by - ay) * t;             // Punkt
        if (!sailable(x, y, draft + 0.3f)) return false;                  // Zu flach oder Hindernis
    }                                                                     // Ende der Prüfpunkte
    return true;                                                          // Frei
} // Ende von clearLine

// Sucht einen Seeweg (A* über Wasserkacheln), meidet Küsten und flaches Wasser und glättet das Ergebnis
std::vector<std::pair<float, float>> World::findSeaPath(float sx, float sy, float tx, float ty, float draft) const { // Beginn von findSeaPath
    std::vector<std::pair<float, float>> path;                            // Ergebnis
    if (clearLine(sx, sy, tx, ty, draft)) { path.push_back({tx, ty}); return path; } // Gerade Linie frei: fertig
    int startX = static_cast<int>(sx), startY = static_cast<int>(sy);     // Startkachel
    int goalX = static_cast<int>(tx), goalY = static_cast<int>(ty);       // Zielkachel
    if (startX < 0 || startY < 0 || startX >= m_w || startY >= m_h) return path; // Außerhalb
    auto ok = [&](int x, int y) {                                         // Darf das Schiff diese Kachel benutzen?
        if (!sailable(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, draft + 0.3f)) return false; // Zu flach oder Hindernis
        const Tile& t = tile(x, y);                                       // Kachel
        bool nearEnds = std::abs(x - goalX) + std::abs(y - goalY) < 7 || std::abs(x - startX) + std::abs(y - startY) < 4; // Nähe von Start oder Ziel
        return t.coast >= 2 || nearEnds;                                  // Abstand zur Küste (außer an den Enden)
    };                                                                    // Ende von ok
    std::vector<float> cost(m_tiles.size(), INF);                         // Bisherige Kosten
    std::vector<int> from(m_tiles.size(), -1);                            // Vorgänger
    using Node = std::pair<float, int>;                                   // (Schätzung, Index)
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open; // Offene Liste
    auto idx = [&](int x, int y) { return y * m_w + x; };                 // Index
    auto heuristic = [&](int x, int y) { float dx = std::fabs(static_cast<float>(x - goalX)), dy = std::fabs(static_cast<float>(y - goalY)); return std::max(dx, dy) + 0.414f * std::min(dx, dy); }; // Schätzung
    cost[static_cast<std::size_t>(idx(startX, startY))] = 0.0f;           // Start
    open.push({heuristic(startX, startY), idx(startX, startY)});          // In die offene Liste
    const int ndx[8] = {1, -1, 0, 0, 1, 1, -1, -1};                       // Nachbarn x
    const int ndy[8] = {0, 0, 1, -1, 1, -1, 1, -1};                       // Nachbarn y
    bool found = false;                                                   // Ziel erreicht?
    int expanded = 0;                                                     // Zähler gegen Endlosschleifen
    while (!open.empty() && expanded < 60000) {                           // Suchschleife
        int cur = open.top().second;                                      // Bester Knoten
        open.pop();                                                       // Entfernen
        ++expanded;                                                       // Zählen
        int cx = cur % m_w, cy = cur / m_w;                               // Koordinaten
        if (cx == goalX && cy == goalY) { found = true; break; }          // Ziel erreicht
        for (int k = 0; k < 8; ++k) {                                     // Nachbarn
            int nx = cx + ndx[k], ny = cy + ndy[k];                       // Nachbar
            if (nx < 0 || ny < 0 || nx >= m_w || ny >= m_h || !ok(nx, ny)) continue; // Nicht befahrbar
            if (k >= 4 && (!ok(cx + ndx[k], cy) || !ok(cx, cy + ndy[k]))) continue; // Keine Ecken schneiden
            const Tile& t = tile(nx, ny);                                 // Kachel
            float step = (k >= 4 ? 1.4142f : 1.0f) + (t.coast < 4 ? 0.6f : 0.0f) + (t.terrain == Terrain::Shallow ? 1.0f : 0.0f); // Küste und Flachwasser kosten mehr
            float nc = cost[static_cast<std::size_t>(cur)] + step;        // Neue Kosten
            int ni = idx(nx, ny);                                         // Index
            if (nc >= cost[static_cast<std::size_t>(ni)]) continue;       // Kein besserer Weg
            cost[static_cast<std::size_t>(ni)] = nc;                      // Merken
            from[static_cast<std::size_t>(ni)] = cur;                     // Vorgänger
            open.push({nc + heuristic(nx, ny), ni});                      // Einreihen
        }                                                                 // Ende der Nachbarn
    }                                                                     // Ende der Suche
    if (!found) return path;                                              // Kein Seeweg
    std::vector<std::pair<float, float>> raw;                             // Ungeglätteter Weg
    for (int cur = idx(goalX, goalY); cur >= 0 && cur != idx(startX, startY); cur = from[static_cast<std::size_t>(cur)]) raw.push_back({static_cast<float>(cur % m_w) + 0.5f, static_cast<float>(cur / m_w) + 0.5f}); // Rückverfolgen
    std::reverse(raw.begin(), raw.end());                                 // Richtige Reihenfolge
    if (!raw.empty()) raw.back() = {tx, ty};                              // Genauer Zielpunkt
    float px = sx, py = sy;                                               // Letzter fester Punkt
    std::size_t i = 0;                                                    // Aktueller Index
    while (i < raw.size()) {                                              // Glätten: so weit wie möglich geradeaus
        std::size_t j = raw.size() - 1;                                   // Vom Ende her probieren
        while (j > i && !clearLine(px, py, raw[j].first, raw[j].second, draft)) --j; // Weitesten sichtbaren Punkt suchen
        path.push_back(raw[j]);                                           // Wegpunkt
        px = raw[j].first; py = raw[j].second;                            // Neuer Ausgangspunkt
        i = j + 1;                                                        // Weiter danach
    }                                                                     // Ende des Glättens
    return path;                                                          // Ergebnis
} // Ende von findSeaPath

// Zeichnet Wasser und Land im sichtbaren Bereich
void World::drawTerrain(Canvas& canvas, const Camera& cam, float time) const { // Beginn von drawTerrain
    const float tw = cam.tileWidth;                                       // Kachelbreite
    const float th = tw * 0.5f;                                           // Kachelhöhe
    const float lift = LAND_HEIGHT * tw * Iso::HEIGHT_FACTOR;             // Höhe des Landes in Pixel
    float cornersX[4], cornersY[4];                                       // Bildschirmecken in Weltkoordinaten
    cam.toWorld(0, 0, cornersX[0], cornersY[0]);                          // Oben links
    cam.toWorld(static_cast<float>(cam.screenW), 0, cornersX[1], cornersY[1]); // Oben rechts
    cam.toWorld(0, static_cast<float>(cam.screenH), cornersX[2], cornersY[2]); // Unten links
    cam.toWorld(static_cast<float>(cam.screenW), static_cast<float>(cam.screenH), cornersX[3], cornersY[3]); // Unten rechts
    int x0 = static_cast<int>(std::floor(std::min({cornersX[0], cornersX[1], cornersX[2], cornersX[3]}))) - 2; // Kleinstes x
    int x1 = static_cast<int>(std::ceil(std::max({cornersX[0], cornersX[1], cornersX[2], cornersX[3]}))) + 2;  // Größtes x
    int y0 = static_cast<int>(std::floor(std::min({cornersY[0], cornersY[1], cornersY[2], cornersY[3]}))) - 2; // Kleinstes y
    int y1 = static_cast<int>(std::ceil(std::max({cornersY[0], cornersY[1], cornersY[2], cornersY[3]}))) + 2;  // Größtes y
    float sxOff = static_cast<float>(cam.screenW) * 0.5f;                 // Bildmitte x
    float syOff = static_cast<float>(cam.screenH) * 0.5f;                 // Bildmitte y

    // 1. Wasser (alle Kacheln, die nicht festes Land sind)
    for (int y = y0; y <= y1; ++y) {                                      // Zeilen
        for (int x = x0; x <= x1; ++x) {                                  // Spalten
            const Tile& t = tile(x, y);                                   // Kachel
            if (isSolidLand(t.terrain)) continue;                         // Land später
            float sx = sxOff + Iso::screenX(static_cast<float>(x) - cam.x, static_cast<float>(y) - cam.y, tw); // Obere Ecke x
            float sy = syOff + Iso::screenY(static_cast<float>(x) - cam.x, static_cast<float>(y) - cam.y, 0.0f, tw); // Obere Ecke y
            if (sx < -tw || sx > static_cast<float>(cam.screenW) + tw || sy < -th * 2 || sy > static_cast<float>(cam.screenH) + th) continue; // Unsichtbar
            float d = clampValue(t.depth / 22.0f, 0.0f, 1.0f);            // Tiefe 0..1
            Color deep = rgba(22, 78, 140), mid = rgba(36, 122, 178), shallow = rgba(78, 178, 192); // Wasserfarben
            Color col = d < 0.15f ? mixColor(shallow, mid, d / 0.15f) : mixColor(mid, deep, (d - 0.15f) / 0.85f); // Farbe nach Tiefe
            float wave = std::sin(time * 1.3f + static_cast<float>(x) * 0.9f + static_cast<float>(y) * 0.55f) * 0.035f; // Leichtes Schimmern
            col = bright(col, 0.97f + wave + static_cast<float>(t.shade % 8) * 0.006f); // Helligkeit variieren
            fillDiamond(canvas, sx, sy, tw, th, col);                     // Kachel zeichnen
            if (t.coast <= 1 && t.terrain != Terrain::Pier) {             // Schaum an der Küste
                int a = 50 + static_cast<int>(40.0f * std::sin(time * 2.0f + static_cast<float>(x + y))); // Pulsierende Deckkraft
                fillDiamond(canvas, sx, sy + th * 0.25f, tw * 0.5f, th * 0.5f, rgba(235, 250, 255, a)); // Heller Fleck
            } else if (((x * 7 + y * 13 + static_cast<int>(time * 0.8f)) % 17) == 0 && t.depth > 3.0f) { // Gelegentliche Wellenkämme
                canvas.fillRect(static_cast<int>(sx - tw * 0.15f), static_cast<int>(sy + th * 0.5f), static_cast<int>(tw * 0.3f), 2, rgba(210, 235, 250, 140)); // Kurzer heller Strich
            }                                                             // Ende der Effekte
        }                                                                 // Ende der Spalten
    }                                                                     // Ende der Zeilen

    // 2. Land und Stege von hinten nach vorne (wegen der Seitenflächen)
    for (int s = x0 + y0; s <= x1 + y1; ++s) {                            // Diagonalen (x + y = s)
        for (int x = std::max(x0, s - y1); x <= std::min(x1, s - y0); ++x) { // Alle Kacheln der Diagonale
            int y = s - x;                                                // Zugehöriges y
            const Tile& t = tile(x, y);                                   // Kachel
            bool pier = t.terrain == Terrain::Pier;                       // Steg?
            if (!isSolidLand(t.terrain) && !pier) continue;               // Wasser schon gezeichnet
            float sx = sxOff + Iso::screenX(static_cast<float>(x) - cam.x, static_cast<float>(y) - cam.y, tw); // Obere Ecke x
            float sy = syOff + Iso::screenY(static_cast<float>(x) - cam.x, static_cast<float>(y) - cam.y, LAND_HEIGHT, tw); // Obere Ecke y (angehoben)
            if (sx < -tw || sx > static_cast<float>(cam.screenW) + tw || sy < -th * 2 || sy > static_cast<float>(cam.screenH) + th) continue; // Unsichtbar
            Color top;                                                    // Farbe der Oberfläche
            const Island* isl = t.island > 0 ? &islands[t.island - 1] : nullptr; // Insel
            bool spooky = isl && PropertyFile::toLower(isl->landscape) == "grusel"; // Unheimliche Insel?
            switch (t.terrain) {                                          // Je nach Bodenart
            case Terrain::Sand: top = rgba(232, 212, 156); break;         // Sand
            case Terrain::Grass: top = spooky ? rgba(96, 118, 86) : rgba(116, 178, 84); break; // Gras
            case Terrain::Rock: top = rgba(150, 148, 140); break;         // Fels
            case Terrain::Path: top = rgba(198, 168, 120); break;         // Weg
            case Terrain::Plaza: top = ((x + y) % 2) ? rgba(188, 182, 170) : rgba(176, 170, 158); break; // Pflaster im Schachbrett
            default: top = rgba(150, 102, 62); break;                     // Steg (Holz)
            }                                                             // Ende der Fallunterscheidung
            top = bright(top, 0.95f + static_cast<float>(t.shade % 16) * 0.007f); // Leichte Variation
            Color side = pier ? rgba(96, 62, 38) : (t.terrain == Terrain::Sand ? rgba(190, 160, 105) : rgba(122, 88, 58)); // Seitenfarbe
            const Tile& front1 = tile(x, y + 1);                          // Nachbar vorne links
            const Tile& front2 = tile(x + 1, y);                          // Nachbar vorne rechts
            bool open1 = !isSolidLand(front1.terrain) && front1.terrain != Terrain::Pier; // Vorne links Wasser?
            bool open2 = !isSolidLand(front2.terrain) && front2.terrain != Terrain::Pier; // Vorne rechts Wasser?
            if (open1) {                                                  // Linke Seitenfläche
                std::vector<std::pair<float, float>> face = {{sx - tw * 0.5f, sy + th * 0.5f}, {sx, sy + th}, {sx, sy + th + lift}, {sx - tw * 0.5f, sy + th * 0.5f + lift}}; // Parallelogramm
                canvas.fillPolygon(face, bright(side, 1.05f));            // Hellere linke Seite
            }                                                             // Ende links
            if (open2) {                                                  // Rechte Seitenfläche
                std::vector<std::pair<float, float>> face = {{sx, sy + th}, {sx + tw * 0.5f, sy + th * 0.5f}, {sx + tw * 0.5f, sy + th * 0.5f + lift}, {sx, sy + th + lift}}; // Parallelogramm
                canvas.fillPolygon(face, bright(side, 0.8f));             // Dunklere rechte Seite
            }                                                             // Ende rechts
            fillDiamond(canvas, sx, sy, tw, th, top);                     // Oberfläche
            if (pier) {                                                   // Planken des Stegs andeuten
                for (int k = 1; k < 4; ++k) {                             // Drei Fugen
                    float f = static_cast<float>(k) / 4.0f;               // Anteil
                    canvas.line(static_cast<int>(sx - tw * 0.5f * (1 - f)), static_cast<int>(sy + th * 0.5f * (1 + f) - th * 0.5f), static_cast<int>(sx + tw * 0.5f * f), static_cast<int>(sy + th * 0.5f * f), rgba(110, 72, 44)); // Fuge
                }                                                         // Ende der Fugen
            }                                                             // Ende Steg
        }                                                                 // Ende der Diagonale
    }                                                                     // Ende der Diagonalen
} // Ende von drawTerrain
