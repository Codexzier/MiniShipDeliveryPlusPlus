// Game.h - Hauptklasse: Fenster, Spielschleife, Hauptmenü, Spielablauf und Popups
#pragma once // Header nur einmal einbinden

#include <SDL.h>  // SDL2 für Fenster, Eingaben und Software-Rendering
#include <string> // std::string
#include <vector> // std::vector

#include "Animation.h"    // Animationsdefinitionen
#include "Assets.h"       // Bilder
#include "Background.h"   // Parallax-Hintergrund
#include "Effects.h"      // Partikel und Texte
#include "Graphics.h"     // Bildspeicher
#include "Hud.h"          // Menüleiste und Inventar
#include "Items.h"        // Gegenstände
#include "Level.h"        // Karte
#include "Player.h"       // Spielfigur
#include "PropertyFile.h" // Einstellungen
#include "Shop.h"         // Shop-Menü
#include "Ui.h"           // Knöpfe und Popups

class Game {                                                                  // Beginn der Klasse Game
public:                                                                       // Öffentliche Schnittstelle
    int run(int argc, char** argv);                                           // Startet das Spiel und kehrt erst am Ende zurück

private:                                                                      // Interne Funktionen
    enum class Screen { MainMenu, Playing };                                  // Hauptmenü oder Spiel
    enum class PopupKind { None, QuitConfirm, LevelComplete, GameOver };      // Art des offenen Popups

    bool findBaseDir(const char* argv0);                                      // Ordner mit data/ suchen
    void loadConfig();                                                        // Einstellungen aus data/spiel.txt lesen
    bool initSdl();                                                           // Fenster und Renderer anlegen
    void buildAssets();                                                       // Alle Bilder laden oder erzeugen
    void layoutUi();                                                          // Positionen der Bedienelemente berechnen
    void shutdown();                                                          // Alles freigeben

    void handleEvent(const SDL_Event& event);                                 // Ein SDL-Ereignis verarbeiten
    void handleKeyDown(SDL_Scancode key);                                     // Tastendruck verarbeiten
    void handleClick(int mx, int my);                                         // Mausklick verarbeiten

    void update(float dt);                                                    // Spielzustand aktualisieren
    void updatePlaying(float dt);                                             // Spiel aktualisieren
    void handlePlayerEvents(const PlayerEvents& events);                      // Meldungen zu Spielerereignissen
    void checkWorld();                                                        // Münzen, Gräben, Autos und Ziel prüfen
    void updateCamera(float dt, bool snap);                                   // Kamera der Figur folgen lassen

    void render();                                                            // Ein Bild zeichnen
    void renderMenu(Canvas& canvas);                                          // Hauptmenü zeichnen
    void renderWorld(Canvas& canvas);                                         // Spielwelt zeichnen
    void renderHud(Canvas& canvas);                                           // Oberfläche über der Welt zeichnen
    void present();                                                           // Bild ins Fenster kopieren

    void startNewGame();                                                      // Neues Spiel beginnen
    void continueGame();                                                      // Letzten Spielstand laden
    void saveGame(bool showMessage);                                          // Spielstand speichern
    bool saveExists() const;                                                  // Gibt es einen Spielstand?
    void openPopup(PopupKind kind);                                           // Popup öffnen
    void popupChoice(int index);                                              // Knopf im Popup gewählt
    void menuActivate(int index);                                             // Knopf im Hauptmenü gewählt
    void interact();                                                          // Taste E
    void openShop(int index);                                                 // Shop betreten
    void closeShop();                                                         // Shop verlassen
    void buyItem(const std::string& id);                                      // Gegenstand kaufen
    void useSlot(int index);                                                  // Gegenstand im Inventar benutzen
    std::string currentPrompt() const;                                        // Aktueller Bedienhinweis
    float maxStamina() const;                                                 // Maximale Ausdauer inkl. Gegenstände
    bool hasHook() const;                                                     // Besitzt die Figur einen Enterhaken?
    void takeScreenshot();                                                    // Bildschirmfoto speichern (F12)

    SDL_Window* m_window = nullptr;                                           // Fenster
    SDL_Renderer* m_renderer = nullptr;                                       // Software-Renderer von SDL
    SDL_Texture* m_texture = nullptr;                                         // Textur, in die das fertige Bild kopiert wird
    Image m_frame;                                                            // Unser Bildspeicher (hier zeichnet die CPU)

    std::string m_baseDir;                                                    // Grundordner (enthält data/ und assets/)
    std::string m_dataDir;                                                    // Ordner data/
    std::string m_savePath;                                                   // Pfad des Spielstands
    PropertyFile m_config;                                                    // Inhalt von data/spiel.txt
    std::string m_title = "Mini Ship Delivery";                               // Fenstertitel
    int m_windowW = 1280;                                                     // Fensterbreite
    int m_windowH = 720;                                                      // Fensterhöhe
    bool m_fullscreen = false;                                                // Vollbild?
    int m_maxFps = 60;                                                        // Bildrate-Begrenzung
    int m_tile = 128;                                                         // Kachel-/Spritegröße (128, 64 oder 32)
    int m_ui = 4;                                                             // Skalierung der Oberfläche
    int m_fbW = 1280;                                                         // Breite des Bildspeichers
    int m_fbH = 720;                                                          // Höhe des Bildspeichers
    bool m_exportDummies = true;                                              // Platzhalter als Vorlagen exportieren?
    std::string m_startLevel = "level1.txt";                                  // Erste Karte
    SDL_Scancode m_keyLeft = SDL_SCANCODE_A;                                  // Taste für links
    SDL_Scancode m_keyRight = SDL_SCANCODE_D;                                 // Taste für rechts
    SDL_Scancode m_keyJump = SDL_SCANCODE_SPACE;                              // Taste für Springen
    SDL_Scancode m_keyInteract = SDL_SCANCODE_E;                              // Taste für Interagieren
    SDL_Scancode m_keyHook = SDL_SCANCODE_Q;                                  // Taste für den Enterhaken

    Assets m_assets;                                                          // Alle Bilder
    Background m_background;                                                  // Parallax-Hintergrund
    Level m_level;                                                            // Aktuelle Karte
    Player m_player;                                                          // Spielfigur
    AnimationSet m_anims;                                                     // Animationsdefinitionen
    ItemDatabase m_items;                                                     // Alle Gegenstände
    Inventory m_inventory;                                                    // Inventar der Figur
    Hud m_hud;                                                                // Menüleiste und Inventaranzeige
    ShopMenu m_shop;                                                          // Shop-Menü
    Popup m_popup;                                                            // Popup-Nachricht
    PopupKind m_popupKind = PopupKind::None;                                  // Art des Popups
    Effects m_effects;                                                        // Effekte

    Screen m_screen = Screen::MainMenu;                                       // Aktueller Bildschirm
    bool m_running = true;                                                    // Läuft das Spiel noch?
    float m_time = 0.0f;                                                      // Laufzeit in Sekunden (für Animationen)
    float m_camX = 0.0f;                                                      // Kameraposition (linker Bildrand in Kacheln)
    float m_menuScroll = 0.0f;                                                // Verschiebung des Menühintergrunds in Pixel
    float m_physicsAccumulator = 0.0f;                                        // Gesammelte Zeit für feste Physikschritte
    int m_mouseX = -1000;                                                     // Mausposition x (im Bildspeicher)
    int m_mouseY = -1000;                                                     // Mausposition y (im Bildspeicher)
    bool m_jumpPressed = false;                                               // Leertaste seit dem letzten Schritt gedrückt?
    bool m_hookPressed = false;                                               // Q seit dem letzten Schritt gedrückt?
    bool m_levelDone = false;                                                 // Ziel erreicht?
    float m_warnCooldown = 0.0f;                                              // Wartezeit zwischen gleichen Warnungen
    std::vector<Button> m_menuButtons;                                        // Knöpfe des Hauptmenüs
    int m_menuSelection = 0;                                                  // Per Tastatur gewählter Menüknopf
}; // Ende der Klasse Game
