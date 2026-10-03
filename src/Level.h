// Level.h - Die Karte: Boden, Gräben, Shop, Straße mit Fußgängerampel, Ankerpunkte, Münzen und Ziel
// Alle Positionen werden in Kacheln angegeben (1 Kachel = 1 Sprite-Breite), y wächst nach unten.
#pragma once // Header nur einmal einbinden

#include <string> // std::string
#include <vector> // std::vector

#include "Assets.h"   // Bilder für das Zeichnen
#include "Common.h"   // RectF
#include "Graphics.h" // Canvas

// Ein schwimmendes Müllteil im Wassergraben
struct TrashItem {         // Beginn der Struktur
    int type = 0;          // Art des Mülls (Flasche, Dose, Reifen, Tüte, Kiste)
    float offset = 0.0f;   // Position vom linken Grabenrand aus (Kacheln)
    float phase = 0.0f;    // Phasenverschiebung für das Schaukeln
    float speed = 1.0f;    // Geschwindigkeit des Schaukelns
}; // Ende der Struktur TrashItem

// Ein Graben (trocken oder mit Wasser)
struct Ditch {                       // Beginn der Struktur
    std::string id;                  // Objektname in der Datei
    float x = 0.0f;                  // Linker Rand
    float width = 1.0f;              // Breite
    float depth = 1.5f;              // Tiefe
    bool water = false;              // Ist Wasser im Graben?
    float waterLevel = 0.55f;        // Abstand der Wasseroberfläche unter der Bodenkante
    float damage = 10.0f;            // Schaden beim Hineinfallen
    std::vector<TrashItem> trash;    // Schwimmender Müll
    float fallLine() const { return water ? waterLevel + 0.15f : std::min(depth - 0.05f, 0.75f); } // Ab welcher Tiefe gilt man als hineingefallen
}; // Ende der Struktur Ditch

// Ein Shop, den man betreten kann (die Karte wechselt nicht, es öffnet sich ein Menü)
struct ShopSpot {                           // Beginn der Struktur
    std::string id;                         // Objektname in der Datei
    std::string name;                       // Anzeigename im Shop-Menü
    std::string sign;                       // Text auf dem Ladenschild
    float x = 0.0f;                         // Linke Kante des Gebäudes
    float width = 3.0f;                     // Breite des Gebäudes
    float height = 2.6f;                    // Höhe des Gebäudes
    std::vector<std::string> wares;         // Angebotene Gegenstände (IDs)
    float doorX() const { return x + width - width / 10.0f - 0.225f; } // Mitte der Tür (passend zur Grafik)
}; // Ende der Struktur ShopSpot

// Ein Auto, das bei Fußgänger-Rot über die Kreuzung fährt (von hinten auf den Betrachter zu)
struct Car {                       // Beginn der Struktur
    float progress = 0.0f;         // Fortschritt 0 (Horizont) bis 1 (vorne aus dem Bild)
    int variant = 0;               // Farbvariante
    bool hitChecked = false;       // Wurde der Zusammenstoß schon geprüft?
}; // Ende der Struktur Car

// Straße mit Fußgängerampel
struct TrafficLight {                     // Beginn der Struktur
    std::string id;                       // Objektname in der Datei
    float x = 0.0f;                       // Linker Rand der Straße
    float streetWidth = 2.5f;             // Breite der Straße
    float redTime = 7.0f;                 // Dauer der Rotphase (Sekunden)
    float greenTime = 5.0f;               // Dauer der Grünphase (Sekunden)
    float clearance = 1.0f;               // Räumzeit nach Beginn von Rot, bevor Autos kommen
    float carInterval = 1.6f;             // Abstand zwischen zwei Autos
    float carDuration = 1.4f;             // Fahrzeit eines Autos vom Horizont bis nach vorne
    float damage = 25.0f;                 // Schaden bei einem Zusammenstoß
    float buttonWait = 2.0f;              // Restliche Rotzeit nach Drücken des Tasters
    bool green = false;                   // Zeigt die Ampel Grün für Fußgänger?
    float phaseTimer = 0.0f;              // Zeit in der aktuellen Phase
    float carTimer = 0.0f;                // Zeit bis zum nächsten Auto
    bool buttonPressed = false;           // Wurde der Taster in dieser Rotphase gedrückt?
    int carCounter = 0;                   // Zähler für die Farbvarianten
    std::vector<Car> cars;                // Autos, die gerade fahren
    float poleX() const { return x - 0.35f; }                    // Position des Ampelmastes (linker Bordstein)
    float centerX() const { return x + streetWidth * 0.5f; }     // Straßenmitte
    float remaining() const { return (green ? greenTime : redTime) - phaseTimer; } // Restzeit der Phase
    bool canSpawnCar() const;              // Darf jetzt noch ein Auto losfahren?
    void spawnCar(float startProgress);    // Ein Auto starten
    void pressButton();                    // Fußgängertaster drücken
    void update(float dt);                 // Ampelphasen und Autos weiterlaufen lassen
}; // Ende der Struktur TrafficLight

// Ankerpunkt für den Enterhaken
struct Anchor {                    // Beginn der Struktur
    std::string id;                // Objektname in der Datei
    float x = 0.0f;                // Position x
    float y = 0.0f;                // Position y (berechnet aus der Höhe über dem Boden)
    bool frame = true;             // Stahlgestell zeichnen?
    float frameWidth = 4.8f;       // Abstand der beiden Gestellpfosten
}; // Ende der Struktur Anchor

// Eine Münze zum Einsammeln
struct Coin {                      // Beginn der Struktur
    std::string id;                // Objektname in der Datei (für den Spielstand)
    float x = 0.0f;                // Mittelpunkt x
    float y = 0.0f;                // Mittelpunkt y
    int value = 5;                 // Wert in Coins
    bool collected = false;        // Schon eingesammelt?
}; // Ende der Struktur Coin

// Konstanten für die perspektivische Straße
constexpr float ROAD_DEPTH_FAR = 0.3f;   // Tiefenfaktor am Horizont
constexpr float ROAD_DEPTH_NEAR = 1.7f;  // Tiefenfaktor, wenn das Auto vorne aus dem Bild fährt
constexpr float ROAD_HORIZON = 1.7f;     // Abstand des Straßenhorizonts über der Bodenkante (Kacheln)

class Level {                                                                // Beginn der Klasse Level
public:                                                                      // Öffentliche Schnittstelle
    bool load(const std::string& path);                                      // Karte aus Textdatei laden
    void resetRuntime();                                                     // Münzen und Ampeln zurücksetzen
    void update(float dt);                                                   // Ampeln, Autos usw. aktualisieren

    const std::vector<RectF>& solids() const { return m_solids; }           // Feste Flächen für Kollisionen
    const Ditch* ditchAt(float x) const;                                     // Graben an Position x (oder nullptr)
    int streetAt(float x) const;                                             // Index der Straße an Position x (oder -1)
    static float carDepth(const Car& car);                                   // Tiefenfaktor eines Autos

    void drawBack(Canvas& canvas, const Assets& assets, float camX, float time) const;  // Alles hinter der Spielfigur
    void drawFront(Canvas& canvas, const Assets& assets, float camX, float time) const; // Alles vor der Spielfigur

    std::string name = "Level";             // Name der Karte
    std::string fileName;                   // Dateiname ohne Pfad (für den Spielstand)
    float length = 50.0f;                   // Länge der Karte
    float groundY = 4.4f;                   // Höhe der Bodenkante (von oben)
    float startX = 1.5f;                    // Startposition der Spielfigur
    float goalX = 45.0f;                    // Position des Ziels
    std::vector<Ditch> ditches;             // Alle Gräben
    std::vector<ShopSpot> shops;            // Alle Shops
    std::vector<TrafficLight> lights;       // Alle Straßen mit Ampel
    std::vector<Anchor> anchors;            // Alle Ankerpunkte
    std::vector<Coin> coins;                // Alle Münzen

private:                                                                     // Interne Hilfsfunktionen
    void buildSolids();                                                      // Kollisionsflächen aus Gräben berechnen
    void drawRoad(Canvas& canvas, const TrafficLight& light, float camX, int tile) const; // Perspektivische Straße zeichnen
    void drawCar(Canvas& canvas, const Assets& assets, const TrafficLight& light, const Car& car, float camX) const; // Ein Auto zeichnen
    std::vector<RectF> m_solids;                                             // Feste Flächen
    std::vector<TrafficLight> m_initialLights;                               // Ausgangszustand der Ampeln (für Neustart)
}; // Ende der Klasse Level
