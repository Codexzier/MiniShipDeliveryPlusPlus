// World.h - Die Seekarte: Kachelraster mit Wasser und Inseln, Häfen, Gebäuden, Bewuchs, Zonen und Handelsrouten
//
// Koordinaten: x zeigt nach Osten (Bildschirm rechts unten), y nach Süden (Bildschirm links unten), Einheit = 1 Kachel.
#pragma once // Header nur einmal einbinden

#include <cstdint> // std::uint8_t
#include <string>  // std::string
#include <utility> // std::pair
#include <vector>  // std::vector

#include "Graphics.h"     // Canvas
#include "PropertyFile.h" // data/welt.txt

// Bodenart einer Kachel
enum class Terrain : std::uint8_t { DeepWater, Water, Shallow, Sand, Grass, Rock, Path, Plaza, Pier }; // Tiefes Wasser ... Steg

// Eine Kachel
struct Tile {                                 // Beginn der Struktur
    Terrain terrain = Terrain::DeepWater;     // Bodenart
    float depth = 30.0f;                      // Wassertiefe in Metern (0 an Land)
    std::uint8_t island = 0;                  // Inselnummer + 1 (0 = keine Insel)
    bool blocked = false;                     // Durch ein Objekt (Haus, Baum) belegt
    std::uint8_t coast = 255;                 // Abstand zur nächsten Landkachel (für Wasser) bzw. zum Wasser (für Land)
    std::uint8_t shade = 0;                   // Zufällige Farbabweichung
}; // Ende der Struktur Tile

// Ein festes Objekt in der Welt (Baum, Haus, Boje ...)
struct WorldObject {                          // Beginn der Struktur
    std::string model;                        // Modellname aus data/modelle.txt
    float x = 0.0f;                           // Position x
    float y = 0.0f;                           // Position y
    float z = 0.0f;                           // Höhe (Land liegt etwas höher als Wasser)
    float angle = 0.0f;                       // Blickrichtung in Radiant
    int steps = 4;                            // Anzahl gerenderter Drehungen
    bool bob = false;                         // Schaukelt im Wasser?
}; // Ende der Struktur WorldObject

// Ein Gebäude oder Marktstand mit Funktion
struct Building {                             // Beginn der Struktur
    std::string type;                         // kontor, museum, werft, taverne, haendler, hersteller
    std::string ref;                          // ID des Händlers oder Herstellers
    std::string name;                         // Anzeigename
    int island = -1;                          // Index der Insel
    float x = 0.0f;                           // Mitte x
    float y = 0.0f;                           // Mitte y
    float doorX = 0.0f;                       // Interaktionspunkt x (vor der Tür)
    float doorY = 0.0f;                       // Interaktionspunkt y
    float facing = 0.0f;                      // Blickrichtung der Tür (Radiant)
    std::string figure;                       // Figur, die vor der Tür steht
    std::string model;                        // Modell des Gebäudes oder Marktstands (zum Anklicken)
}; // Ende der Struktur Building

// Eine Insel
struct Island {                               // Beginn der Struktur
    std::string id;                           // Objektname in der Datei
    std::string name;                         // Anzeigename
    std::string type;                         // stadt, dorf oder wild
    std::string landscape;                    // Bewuchs: palmen, wald, stadt, grusel
    float cx = 0.0f;                          // Mittelpunkt x
    float cy = 0.0f;                          // Mittelpunkt y
    float radius = 8.0f;                      // Radius
    std::string harborDir = "ost";            // Richtung des Hafens (nord, ost, sued, west)
    float pierBaseX = 0.0f, pierBaseY = 0.0f; // Beginn des Stegs an Land
    float pierEndX = 0.0f, pierEndY = 0.0f;   // Ende des Stegs (hier geht man an Bord)
    float dockX = 0.0f, dockY = 0.0f;         // Liegeplatz des Schiffs
    float dockAngle = 0.0f;                   // Ausrichtung des Schiffs am Liegeplatz
    float plazaX = 0.0f, plazaY = 0.0f;       // Mitte des Marktplatzes
    std::vector<std::string> produces;        // Hier billig: eigene Waren
    std::vector<std::string> demands;         // Hier teuer: gefragte Waren
    std::vector<int> buildings;               // Gebäude der Insel
    bool hasMuseum = false;                   // Gibt es ein Museum?
    bool hasShipyard = false;                 // Gibt es eine Werft?
    bool hasTavern = false;                   // Gibt es eine Taverne?
}; // Ende der Struktur Island

// Eine besondere Zone auf See
struct Zone {                                 // Beginn der Struktur
    std::string kind;                         // riff, piraten oder monster
    float x = 0.0f;                           // Mittelpunkt x
    float y = 0.0f;                           // Mittelpunkt y
    float radius = 6.0f;                      // Radius
    float strength = 1.0f;                    // Stärke (z.B. Anzahl Piraten)
}; // Ende der Struktur Zone

// Eine sichere Handelsroute zwischen zwei Häfen
struct Route {                                // Beginn der Struktur
    std::string name;                         // Name
    std::vector<std::pair<float, float>> points; // Wegpunkte
}; // Ende der Struktur Route

// Einfache Kamera (Mittelpunkt in Weltkoordinaten)
struct Camera {                               // Beginn der Struktur
    float x = 0.0f;                           // Mittelpunkt x
    float y = 0.0f;                           // Mittelpunkt y
    float tileWidth = 96.0f;                  // Zoom: Kachelbreite in Pixel
    int screenW = 1280;                       // Bildschirmbreite
    int screenH = 720;                        // Bildschirmhöhe
    float toScreenX(float wx, float wy) const; // Welt -> Bildschirm x
    float toScreenY(float wx, float wy, float z) const; // Welt -> Bildschirm y
    void toWorld(float sx, float sy, float& wx, float& wy) const; // Bildschirm -> Boden
}; // Ende der Struktur Camera

constexpr float LAND_HEIGHT = 0.12f;          // Land liegt so viele Kacheln höher als das Wasser

class World {                                                                   // Beginn der Klasse
public:                                                                         // Öffentliche Schnittstelle
    bool load(const std::string& path, std::string& error);                     // Welt aus data/welt.txt erzeugen
    int width() const { return m_w; }                                           // Breite in Kacheln
    int height() const { return m_h; }                                          // Höhe in Kacheln
    const Tile& tile(int x, int y) const;                                       // Kachel (außerhalb: tiefes Wasser)
    bool isLand(int x, int y) const;                                            // Begehbarer Boden (inkl. Steg)?
    bool walkable(int x, int y) const;                                          // Kann die Figur hier laufen?
    float depthAt(float x, float y) const;                                      // Wassertiefe an einer Position
    bool sailable(float x, float y, float draft) const;                         // Kann ein Schiff hier fahren?
    int islandAt(float x, float y) const;                                       // Insel an einer Position (-1 = keine)
    int nearestBuilding(float x, float y, float maxDist) const;                 // Nächstes Gebäude (Türpunkt)
    float routeDistance(float x, float y) const;                                // Abstand zur nächsten Handelsroute
    const Zone* zoneAt(float x, float y, const std::string& kind) const;        // Zone einer Art an einer Position
    std::vector<std::pair<float, float>> findPath(float sx, float sy, float tx, float ty) const; // Weg zu Fuß (A*)
    std::vector<std::pair<float, float>> findSeaPath(float sx, float sy, float tx, float ty, float draft) const; // Seeweg für den Autopiloten (A*, geglättet)
    bool clearLine(float ax, float ay, float bx, float by, float draft) const;  // Freie Fahrt auf einer geraden Linie?
    void drawTerrain(Canvas& canvas, const Camera& cam, float time) const;      // Boden und Wasser zeichnen
    std::pair<float, float> randomSeaPoint(unsigned seed, float minDepth) const; // Zufälliger Punkt auf offener See

    std::vector<Island> islands;                                                // Alle Inseln
    std::vector<Building> buildings;                                            // Alle Gebäude
    std::vector<WorldObject> objects;                                           // Alle festen Objekte
    std::vector<Zone> zones;                                                    // Alle Zonen
    std::vector<Route> routes;                                                  // Alle Handelsrouten
    std::string startIsland;                                                    // Startinsel (ID)
    int islandIndex(const std::string& id) const;                               // Index einer Insel über ihre ID

private:                                                                        // Interne Funktionen
    Tile& at(int x, int y);                                                     // Kachel zum Ändern
    void generateIsland(Island& isl, int index, unsigned seed);                 // Landmasse einer Insel erzeugen
    void computeDistances();                                                    // Küstenabstände und Wassertiefen berechnen
    void buildHarbor(Island& isl, int index);                                   // Steg, Platz und Weg anlegen
    void placeBuildings(Island& isl, int index, const PropertyFile& f);         // Gebäude um den Platz verteilen
    void decorate(Island& isl, int index, unsigned seed);                       // Bäume, Steine und Blumen setzen
    void applyZones();                                                          // Riffe flacher machen und mit Felsen versehen
    void placeBuoys();                                                          // Bojen entlang der Handelsrouten
    bool areaFree(int x0, int y0, int w, int h, int island) const;              // Ist eine Fläche frei und auf der Insel?

    int m_w = 128;                                                              // Breite
    int m_h = 128;                                                              // Höhe
    std::vector<Tile> m_tiles;                                                  // Alle Kacheln
    Tile m_outside;                                                             // Kachel außerhalb der Karte
}; // Ende der Klasse World
