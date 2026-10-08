// Game.h - Das Spiel: Fenster, Hauptschleife, Steuerung, Darstellung, Menüs und HUD
//
// Die Klasse ist auf mehrere Dateien verteilt:
//   Game.cpp        Start, Hauptschleife, Eingaben, Menüablauf, Speichern, Punkteliste
//   GameUpdate.cpp  Bewegung von Figur und Schiff, Anlegen, Piraten, Seeungeheuer, Kanonen
//   GameRender.cpp  Zeichnen der Welt (Boden, Modelle, Licht, Wetter)
//   GameHud.cpp     HUD, Hauptmenü und alle Fenster (Kontor, Händler, Werft ...)
//   LoadShip.cpp    Minispiel "Load the Ship"
#pragma once // Header nur einmal einbinden

#include <SDL.h> // SDL2 für Fenster und Eingaben

#include <map>    // std::map
#include <string> // std::string
#include <vector> // std::vector

#include "Audio.h"        // Ton
#include "GameData.h"     // Feste Spieldaten
#include "GameState.h"    // Spielstand und Logik
#include "Graphics.h"     // Bildspeicher
#include "ModelLibrary.h" // 3D-Modelle
#include "PropertyFile.h" // Textdateien
#include "Ui.h"           // Oberfläche
#include "World.h"        // Inselwelt

// Bildschirme des Spiels
enum class Screen { Menu, Scores, Playing, LoadShip };             // Hauptmenü, Punkteliste, Spiel, Minispiel

// Fenster, die im Spiel geöffnet sein können
enum class Panel { None, Kontor, Trader, Producer, Museum, Shipyard, Tavern, Pier, Chart, ShipInfo, Pause, Newspaper, Help, Sunk }; // Alle Fenster

// Ein Piratenschiff
struct Pirate {                                   // Beginn der Struktur
    float x = 0.0f, y = 0.0f;                     // Position
    float angle = 0.0f;                           // Kurs
    float speed = 0.0f;                           // Geschwindigkeit
    float hp = 60.0f;                             // Lebenspunkte
    float reload = 0.0f;                          // Zeit bis zum nächsten Schuss
    int zone = -1;                                // Heimatzone
    int state = 0;                                // 0 = kreuzt, 1 = jagt, 2 = flieht, 3 = sinkt
    float tx = 0.0f, ty = 0.0f;                   // Wegpunkt beim Kreuzen
    float timer = 0.0f;                           // Allgemeiner Zeitgeber
    bool decided = false;                         // Hat er über einen Angriff entschieden?
    bool attack = false;                          // Greift er an?
}; // Ende der Struktur Pirate

// Ein Seeungeheuer
struct Monster {                                  // Beginn der Struktur
    float x = 0.0f, y = 0.0f;                     // Position
    float angle = 0.0f;                           // Blickrichtung
    float hp = 200.0f;                            // Lebenspunkte
    float bite = 0.0f;                            // Zeit bis zum nächsten Biss
    float emerge = 0.0f;                          // Auftauchen 0..1
    int zone = -1;                                // Heimatzone
    int state = 0;                                // 0 = jagt, 1 = taucht ab
}; // Ende der Struktur Monster

// Eine Kanonenkugel
struct Shot {                                     // Beginn der Struktur
    float x = 0.0f, y = 0.0f;                     // Position
    float vx = 0.0f, vy = 0.0f;                   // Geschwindigkeit
    float life = 0.0f;                            // Verbleibende Flugzeit
    float total = 1.0f;                           // Gesamte Flugzeit (für den Bogen)
    float damage = 0.0f;                          // Schaden
    bool fromPlayer = false;                      // Vom Spieler abgefeuert?
}; // Ende der Struktur Shot

// Ein Partikel (Gischt, Rauch, Spritzer)
struct Particle {                                 // Beginn der Struktur
    float x = 0.0f, y = 0.0f, z = 0.0f;           // Position
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;        // Geschwindigkeit
    float life = 1.0f;                            // Verbleibende Lebenszeit
    float maxLife = 1.0f;                         // Gesamte Lebenszeit
    float size = 3.0f;                            // Größe in Pixel
    Color color = 0xFFFFFFFFu;                    // Farbe
}; // Ende der Struktur Particle

// Eine Meldung oben in der Bildmitte
struct Notice {                                   // Beginn der Struktur
    std::string text;                             // Text
    float time = 5.0f;                            // Verbleibende Anzeigezeit
}; // Ende der Struktur Notice

// Ein Eintrag der Punkteliste
struct ScoreEntry {                               // Beginn der Struktur
    int score = 0;                                // Punkte
    int days = 0;                                 // Gespielte Tage
    int credits = 0;                              // Geld
    int explorer = 0;                             // Entdeckerpunkte
    std::string ship;                             // Schiff
}; // Ende der Struktur ScoreEntry

class Game {                                                                    // Beginn der Klasse
public:                                                                         // Öffentliche Schnittstelle
    bool init(std::string& error);                                              // Alles laden und das Fenster öffnen
    void run();                                                                 // Hauptschleife
    void shutdown();                                                            // Alles freigeben

private:                                                                        // Interne Funktionen
    // ---------- Game.cpp ----------
    void handleEvent(const SDL_Event& e);                                       // Ein Ereignis verarbeiten
    void handleKey(SDL_Scancode sc);                                            // Tastendruck im Spiel
    void update(float dt);                                                      // Spiel fortschreiben
    void render();                                                              // Bild zeichnen
    void present();                                                             // Bild auf den Bildschirm bringen
    void startNewGame();                                                        // Neues Spiel beginnen
    bool continueGame();                                                        // Spielstand laden
    void saveGame();                                                            // Spielstand speichern
    void leaveToMenu();                                                         // Zurück ins Hauptmenü (mit Wertung)
    void recordScore();                                                         // Punkte in die Liste eintragen
    void loadScores();                                                          // Punkteliste lesen
    void prewarm(const std::string& title);                                     // Sprites vorab rendern (mit Ladebalken)
    void loadingScreen(const std::string& title, float progress);               // Ladebildschirm zeichnen
    void resetTransient();                                                      // Piraten, Partikel usw. leeren
    void notify(const std::string& text);                                       // Meldung anzeigen
    void sound(const std::string& effect, int volume = 100);                    // Geräusch abspielen
    void updateMusic();                                                         // Musik und Hintergrundgeräusch wählen
    bool keyHeld(const std::string& action) const;                              // Wird die Taste einer Aktion gehalten?
    SDL_Scancode keyOf(const std::string& action) const;                        // Taste einer Aktion
    std::string keyName(const std::string& action) const;                       // Name der Taste einer Aktion
    std::string goodName(const std::string& id) const;                          // Anzeigename einer Ware
    std::string islandName(int island) const;                                   // Name einer Insel
    std::string buildingTitle(int building) const;                              // Anzeigename eines Gebäudes
    bool panelBlocksTime() const;                                               // Hält das offene Fenster die Zeit an?
    void openPanel(Panel p, int building = -1);                                 // Fenster öffnen

    // ---------- GameUpdate.cpp ----------
    void updatePlaying(float dt);                                               // Spielwelt fortschreiben
    void updateFigure(float dt);                                                // Figur bewegen
    void updateShip(float dt);                                                  // Schiff bewegen
    void updateCamera(float dt);                                                // Kamera nachführen
    void updatePirates(float dt);                                               // Piraten steuern
    void updateMonsters(float dt);                                              // Seeungeheuer steuern
    void updateShots(float dt);                                                 // Kanonenkugeln bewegen
    void updateParticles(float dt);                                             // Partikel bewegen
    void updateArtifacts();                                                     // Artefakte entdecken und einsammeln
    void interact();                                                            // Aktionstaste (E)
    void clickWorld(float wx, float wy, int sx, int sy);                        // Klick in die Welt (Figur läuft dorthin)
    int pickBuilding(int sx, int sy);                                           // Gebäude unter einem Bildschirmpunkt (pixelgenau)
    void enterBuilding(int building);                                           // Gebäude betreten
    void dock(int island);                                                      // Am Hafen anlegen
    void board();                                                               // An Bord gehen und ablegen
    void capsize();                                                             // Schiff kentert beim Ablegen
    void fireCannons();                                                         // Eigene Kanonen abfeuern
    void damageShip(float amount, const std::string& cause);                    // Schaden am eigenen Schiff
    void shipSunk(const std::string& cause);                                    // Schiff verloren
    void stealCargo();                                                          // Piraten entern und rauben
    int nearDock() const;                                                       // Hafen in Anlegeweite (-1 = keiner)
    int nearDoor() const;                                                       // Gebäude in Reichweite der Figur (-1 = keins)
    bool nearPier() const;                                                      // Steht die Figur am Stegende?
    void splash(float x, float y, int count, Color c);                          // Spritzer erzeugen
    void setTarget(float x, float y, const std::string& name);                  // Ziel setzen
    bool planRoute();                                                           // Seeweg zum Ziel für den Autopiloten berechnen

    // ---------- GameRender.cpp ----------
    void renderWorld(Canvas& c);                                                // Welt mit allen Objekten
    void renderLighting(Canvas& c);                                             // Tageszeit und Lichter
    void renderWeather(Canvas& c);                                              // Regen, Nebel, Sturm
    void buildChart();                                                          // Seekarte vorab zeichnen

    // ---------- GameHud.cpp ----------
    void renderHud();                                                           // HUD je nach Modus
    void hudClock();                                                            // Oben links: Zeit, Tag/Nacht, Termin
    void hudHealth();                                                           // Oben rechts: Gesundheit / Rumpf, Geld
    void hudWindDepth();                                                        // Unten links (Schiff): Wind und Wassertiefe
    void hudDialog();                                                           // Unten links (Figur): Dialog
    void hudTarget();                                                           // Unten rechts: Richtungspfeil zum Ziel
    void hudNotices();                                                          // Meldungen oben in der Mitte
    void renderMenu();                                                          // Hauptmenü
    void renderScores();                                                        // Punkteliste
    void renderPanel();                                                         // Offenes Fenster zeichnen
    void panelKontor();                                                         // Kontor
    void panelTrader();                                                         // Händler
    void panelProducer();                                                       // Hersteller
    void panelMuseum();                                                         // Museum
    void panelShipyard();                                                       // Werft
    void panelTavern();                                                         // Taverne
    void panelPier();                                                           // Steg / Schiff
    void panelChart();                                                          // Seekarte
    void panelShipInfo();                                                       // Schiff, Crew, Ladung
    void panelPause();                                                          // Pause
    void panelNewspaper();                                                      // Hafenzeitung
    void panelHelp();                                                           // Steuerung
    void panelSunk();                                                           // Schiff verloren
    RectI panelFrame(int w, int h, const std::string& title, PanelStyle style = PanelStyle::WoodPaper); // Fensterrahmen mit Titel und Schließen-Knopf
    bool tabs(const RectI& r, const std::vector<std::string>& names, int& current); // Reiterleiste
    void drawGoodIcon(const std::string& good, int cx, int cy, int size);       // Ware als kleines 3D-Symbol
    void drawModelIcon(const std::string& model, int cx, int cy, int size);     // Beliebiges Modell als Symbol
    void drawArrow(int cx, int cy, float screenAngle, int len, Color c);        // Pfeil (Bildschirmwinkel)
    std::string goodTooltip(const std::string& good) const;                     // Kurzinfo zu einer Ware
    void ensureScroll(int count, int visible);                                  // Rollposition begrenzen

    // ---------- LoadShip.cpp ----------
    void openLoadShip(bool departAfter);                                        // Minispiel öffnen
    void updateLoadShip(float dt);                                              // Minispiel fortschreiben
    void renderLoadShip();                                                      // Minispiel zeichnen
    void loadShipClick(int mx, int my, bool right);                             // Klick im Minispiel
    RectI holdRect() const;                                                     // Bereich des Laderaums auf dem Bildschirm
    int holdCellSize() const;                                                   // Pixel je Laderaumfeld

    // ---------- Daten ----------
    SDL_Window* m_window = nullptr;                    // Fenster
    SDL_Renderer* m_renderer = nullptr;                // Software-Renderer von SDL (nur zum Anzeigen)
    SDL_Texture* m_texture = nullptr;                  // Textur, in die das fertige Bild kopiert wird
    Image m_frame;                                     // Bildspeicher (wird per CPU gefüllt)
    int m_w = 1280, m_h = 720;                         // Bildgröße
    int m_maxFps = 60;                                 // Bildrate
    bool m_running = true;                             // Läuft das Spiel?
    std::string m_root;                                // Projektordner (data/, assets/, speicher/)
    PropertyFile m_config;                             // data/spiel.txt
    std::map<std::string, SDL_Scancode> m_keys;        // Tastenbelegung je Aktion
    Settings m_set;                                    // Spielwerte
    GameData m_data;                                   // Feste Spieldaten
    World m_world;                                     // Inselwelt
    ModelLibrary m_lib;                                // Modelle und Sprites
    Ui m_ui;                                           // Oberfläche
    UiInput m_input;                                   // Mauseingaben des aktuellen Bildes
    Audio m_audio;                                     // Ton
    GameState m_state;                                 // Spielstand
    Camera m_cam;                                      // Kamera
    std::vector<float> m_zoomLevels;                   // Erlaubte Kachelbreiten
    int m_zoomIndex = 1;                               // Aktuelle Zoomstufe
    int m_shipSteps = 32;                              // Drehstufen der Schiffe
    Screen m_screen = Screen::Menu;                    // Aktueller Bildschirm
    Panel m_panel = Panel::None;                       // Offenes Fenster
    int m_panelBuilding = -1;                          // Gebäude des offenen Fensters
    int m_tab = 0;                                     // Reiter im Fenster
    int m_scroll = 0;                                  // Rollposition in Listen
    float m_time = 0.0f;                               // Laufzeit in Sekunden (Animationen)
    float m_menuTime = 0.0f;                           // Laufzeit im Menü
    std::vector<Notice> m_notices;                     // Meldungen
    std::vector<std::string> m_log;                    // Letzte Meldungen (Dialogfenster)
    std::string m_dialog;                              // Text im Dialogfenster der Figur
    std::string m_prompt;                              // Hinweis für die Aktionstaste
    std::vector<ScoreEntry> m_scores;                  // Punkteliste
    bool m_hasSave = false;                            // Gibt es einen Spielstand?
    std::string m_sunkText;                            // Text im Fenster "Schiff verloren"
    // Figur
    std::vector<std::pair<float, float>> m_path;       // Weg der Figur (Klicksteuerung)
    int m_pendingBuilding = -1;                        // Gebäude, das nach Ankunft betreten wird
    bool m_pendingPier = false;                        // Nach Ankunft den Steg öffnen
    float m_figAngle = 0.0f;                           // Blickrichtung der Figur
    bool m_figMoving = false;                          // Läuft die Figur?
    // Schiff
    float m_shipSpeed = 0.0f;                          // Geschwindigkeit
    int m_throttle = 0;                                // Fahrstufe -1..3 (Segel bzw. Maschine)
    bool m_engineOn = false;                           // Hilfsmaschine an (Hybrid)
    bool m_autopilot = false;                          // Kurs automatisch halten
    std::vector<std::pair<float, float>> m_seaPath;    // Wegpunkte des Autopiloten
    float m_repath = 0.0f;                             // Zeit bis zur nächsten Wegprüfung
    float m_cannonReload = 0.0f;                       // Nachladen der eigenen Kanonen
    float m_dockAnim = 0.0f;                           // Anlegeanimation (1 -> 0)
    float m_dockFromX = 0.0f, m_dockFromY = 0.0f, m_dockFromA = 0.0f; // Startpunkt der Anlegeanimation
    float m_stormWarn = 0.0f;                          // Zeit seit der letzten Sturmwarnung
    float m_groundWarn = 0.0f;                         // Zeit seit der letzten Auflaufmeldung
    float m_hitFlash = 0.0f;                           // Roter Blitz bei Treffern
    float m_wakeTimer = 0.0f;                          // Zeitgeber für Kielwasser
    float m_autosave = 0.0f;                           // Spielstunden bis zum nächsten automatischen Speichern
    // Gegner und Effekte
    std::vector<Pirate> m_pirates;                     // Piraten
    std::vector<Monster> m_monsters;                   // Seeungeheuer
    std::vector<Shot> m_shots;                         // Kanonenkugeln
    std::vector<Particle> m_particles;                 // Partikel
    std::map<int, float> m_zoneCooldown;               // Wartezeit bis zum nächsten Piraten je Zone
    unsigned m_rand = 12345u;                          // Zufallszustand für Spielereignisse
    float randf();                                     // Zufallszahl 0..1
    // Seekarte und Symbole
    Image m_chart;                                     // Vorab gezeichnete Seekarte
    float m_chartTile = 6.4f;                          // Kachelbreite der Seekarte
    std::map<std::string, Image> m_icons;              // Zwischenspeicher der Symbole
    // Minispiel
    int m_lsHeld = -1;                                 // Index der gehaltenen Einheit im Steglager (-1 = keine)
    Cargo m_lsCargo;                                   // Gehaltene Einheit
    bool m_lsHolding = false;                          // Wird etwas gehalten?
    int m_lsRot = 0;                                   // Drehung der gehaltenen Einheit
    bool m_lsDepart = false;                           // Nach dem Beladen ablegen?
    float m_lsCapsize = -1.0f;                         // Kenteranimation (-1 = aus)
    float m_lsHeelShown = 0.0f;                        // Angezeigte Schlagseite (weich nachgeführt)
    std::string m_lsMessage;                           // Hinweis im Minispiel
}; // Ende der Klasse Game
