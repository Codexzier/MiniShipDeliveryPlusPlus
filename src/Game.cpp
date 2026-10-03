// Game.cpp - Spielschleife, Zustände (Hauptmenü/Spiel), Eingaben, Spiellogik und Zeichnen
#include "Game.h" // Eigene Deklarationen

#include <algorithm> // std::min, std::max
#include <cmath>     // std::fabs, std::sin
#include <cstdio>    // std::snprintf
#include <ctime>     // std::time, std::localtime
#include <iostream>  // std::cout, std::cerr

#include "Font.h"          // Schrift
#include "ImageIO.h"       // Dateien und Ordner
#include "SaveGame.h"      // Spielstand
#include "SpriteFactory.h" // Platzhalter-Grafiken

#ifndef MSD_SOURCE_DIR       // Falls CMake den Projektordner nicht mitgegeben hat ...
#define MSD_SOURCE_DIR "."   // ... wird das aktuelle Verzeichnis benutzt
#endif                       // Ende der Prüfung

namespace {                                   // Interne Konstanten
constexpr float PHYSICS_STEP = 1.0f / 120.0f; // Feste Schrittweite der Physik (120 Schritte pro Sekunde)
constexpr int MAX_PHYSICS_STEPS = 12;         // Höchstens so viele Schritte pro Bild (verhindert Hänger)
} // Ende des internen Namensraums

// Startet das Spiel: Initialisierung, Spielschleife, Aufräumen
int Game::run(int argc, char** argv) {                                       // Beginn von run
    (void)argc;                                                              // Argumente werden derzeit nicht gebraucht
    if (!findBaseDir(argc > 0 ? argv[0] : "")) {                             // Ordner mit den Spieldaten suchen
        std::cerr << "Fehler: Ordner 'data' mit spiel.txt wurde nicht gefunden.\n"; // Fehlermeldung
        return 1;                                                            // Mit Fehlercode beenden
    }                                                                        // Ende der Prüfung
    loadConfig();                                                            // Einstellungen lesen
    if (!initSdl()) return 1;                                                // Fenster anlegen (bei Fehler beenden)
    if (!m_items.load(ImageIO::joinPath(m_dataDir, "gegenstaende.txt")))     // Gegenstände laden
        std::cerr << "Warnung: data/gegenstaende.txt fehlt.\n";              // Warnung ausgeben
    PropertyFile animFile;                                                   // Datei mit den Animationen
    if (animFile.load(ImageIO::joinPath(m_dataDir, "animationen.txt"))) m_anims.load(animFile); // Animationen laden
    PropertyFile playerFile;                                                 // Datei mit den Figurwerten
    if (playerFile.load(ImageIO::joinPath(m_dataDir, "spieler.txt"))) m_player.loadStats(playerFile); // Figurwerte laden
    m_player.setAnimations(&m_anims);                                        // Animationen an die Figur geben
    if (!m_level.load(ImageIO::joinPath(m_dataDir, m_startLevel))) {         // Karte laden
        std::cerr << "Fehler: Karte " << m_startLevel << " fehlt.\n";        // Fehlermeldung
        shutdown();                                                          // Aufräumen
        return 1;                                                            // Beenden
    }                                                                        // Ende der Prüfung
    buildAssets();                                                           // Alle Bilder laden oder erzeugen
    layoutUi();                                                              // Bedienelemente anordnen
    m_player.reset(m_level.startX, m_level.groundY);                         // Figur für die Menüvorschau vorbereiten

    Uint64 frequency = SDL_GetPerformanceFrequency();                        // Ticks pro Sekunde des Zeitgebers
    Uint64 last = SDL_GetPerformanceCounter();                               // Zeitpunkt des letzten Bildes
    while (m_running) {                                                      // Spielschleife
        Uint64 frameStart = SDL_GetPerformanceCounter();                     // Beginn dieses Bildes
        float dt = static_cast<float>(frameStart - last) / static_cast<float>(frequency); // Vergangene Zeit in Sekunden
        last = frameStart;                                                   // Für das nächste Bild merken
        dt = std::min(dt, 0.1f);                                             // Große Sprünge (z.B. nach Fensterverschieben) begrenzen
        SDL_Event event;                                                     // Puffer für Ereignisse
        while (SDL_PollEvent(&event)) handleEvent(event);                    // Alle Ereignisse abarbeiten
        update(dt);                                                          // Spiel aktualisieren
        render();                                                            // Bild zeichnen
        present();                                                           // Bild anzeigen
        if (m_maxFps > 0) {                                                  // Bildrate begrenzen?
            float frameTime = static_cast<float>(SDL_GetPerformanceCounter() - frameStart) / static_cast<float>(frequency); // Dauer dieses Bildes
            float target = 1.0f / static_cast<float>(m_maxFps);              // Gewünschte Dauer pro Bild
            if (frameTime < target) SDL_Delay(static_cast<Uint32>((target - frameTime) * 1000.0f)); // Restzeit schlafen
        }                                                                    // Ende der Begrenzung
    }                                                                        // Ende der Spielschleife
    shutdown();                                                              // Aufräumen
    return 0;                                                                // Erfolgreich beendet
} // Ende von run

// Sucht den Grundordner, in dem data/spiel.txt liegt
bool Game::findBaseDir(const char* argv0) {                                  // Beginn von findBaseDir
    (void)argv0;                                                             // Wird nicht direkt benutzt
    std::vector<std::string> candidates;                                     // Mögliche Ordner
    char* exeDir = SDL_GetBasePath();                                        // Ordner der ausführbaren Datei
    if (exeDir) {                                                            // SDL konnte ihn ermitteln
        std::string dir = exeDir;                                            // In std::string umwandeln
        SDL_free(exeDir);                                                    // Speicher von SDL freigeben
        candidates.push_back(dir);                                           // Ordner der Programmdatei
        candidates.push_back(dir + "..");                                    // Darüberliegender Ordner (z.B. cmake-build-debug/..)
        candidates.push_back(dir + "../..");                                 // Zwei Ebenen darüber
    }                                                                        // Ende der Prüfung
    candidates.push_back(".");                                               // Aktuelles Arbeitsverzeichnis
    candidates.push_back(MSD_SOURCE_DIR);                                    // Projektordner aus CMake
    for (const std::string& dir : candidates) {                              // Alle Kandidaten prüfen
        if (ImageIO::fileExists(ImageIO::joinPath(dir, "data/spiel.txt"))) { // Liegt dort data/spiel.txt?
            m_baseDir = dir;                                                 // Grundordner merken
            m_dataDir = ImageIO::joinPath(dir, "data");                      // Datenordner
            m_savePath = ImageIO::joinPath(ImageIO::joinPath(dir, "speicher"), "spielstand.txt"); // Spielstand-Datei
            std::cout << "Spieldaten gefunden in: " << dir << "\n";          // Hinweis ausgeben
            return true;                                                     // Gefunden
        }                                                                    // Ende der Prüfung
    }                                                                        // Ende der Schleife
    return false;                                                            // Nichts gefunden
} // Ende von findBaseDir

// Liest data/spiel.txt (Fenster, Grafik, Steuerung)
void Game::loadConfig() {                                                    // Beginn von loadConfig
    m_config.load(ImageIO::joinPath(m_dataDir, "spiel.txt"));                // Datei laden
    m_title = m_config.getString("Fenster", "titel", m_title);               // Fenstertitel
    m_windowW = std::max(320, m_config.getInt("Fenster", "breite", 1280));   // Fensterbreite
    m_windowH = std::max(180, m_config.getInt("Fenster", "hoehe", 720));     // Fensterhöhe
    m_fullscreen = m_config.getBool("Fenster", "vollbild", false);           // Vollbild
    m_maxFps = m_config.getInt("Fenster", "max_fps", 60);                    // Bildrate
    m_tile = clampValue(m_config.getInt("Grafik", "kachel_groesse", 128), 16, 256); // Spritegröße (128, 64 oder 32 empfohlen)
    int viewTiles = clampValue(m_config.getInt("Grafik", "sicht_breite_kacheln", 10), 4, 30); // Sichtbare Kacheln nebeneinander
    m_exportDummies = m_config.getBool("Grafik", "dummies_exportieren", true); // Vorlagen exportieren
    m_fbW = viewTiles * m_tile;                                              // Breite des Bildspeichers
    m_fbH = m_fbW * m_windowH / m_windowW;                                   // Höhe passend zum Seitenverhältnis des Fensters
    int uiSetting = m_config.getInt("Grafik", "ui_skalierung", 0);           // Gewünschte Oberflächengröße (0 = automatisch)
    m_ui = uiSetting > 0 ? uiSetting : std::max(1, m_fbW / 320);             // Automatisch: 320 Grundpixel Breite
    m_startLevel = m_config.getString("Spiel", "start_level", "level1.txt"); // Erste Karte
    auto key = [&](const char* name, SDL_Scancode fallback) {                // Hilfsfunktion: Tastenname -> Scancode
        std::string text = m_config.getString("Steuerung", name, "");        // Tastenname aus der Datei
        if (text.empty()) return fallback;                                   // Nicht angegeben -> Standard
        SDL_Scancode code = SDL_GetScancodeFromName(text.c_str());           // Von SDL übersetzen lassen
        return code == SDL_SCANCODE_UNKNOWN ? fallback : code;               // Unbekannt -> Standard
    };                                                                       // Ende der Hilfsfunktion
    m_keyLeft = key("links", SDL_SCANCODE_A);                                // A - nach links
    m_keyRight = key("rechts", SDL_SCANCODE_D);                              // D - nach rechts
    m_keyJump = key("springen", SDL_SCANCODE_SPACE);                         // Leertaste - springen
    m_keyInteract = key("interagieren", SDL_SCANCODE_E);                     // E - interagieren
    m_keyHook = key("enterhaken", SDL_SCANCODE_Q);                           // Q - Enterhaken
} // Ende von loadConfig

// Legt Fenster, Software-Renderer und Textur an (ohne Grafikbeschleuniger)
bool Game::initSdl() {                                                       // Beginn von initSdl
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");                         // Nur den Software-Renderer verwenden
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");                     // Fensterpuffer nicht über die GPU
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");                         // Beim Skalieren scharfe Pixel (nächster Nachbar)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {                    // SDL starten
        std::cerr << "SDL_Init fehlgeschlagen: " << SDL_GetError() << "\n";  // Fehlermeldung
        return false;                                                        // Abbrechen
    }                                                                        // Ende der Prüfung
    Uint32 flags = SDL_WINDOW_RESIZABLE;                                     // Fenstergröße darf verändert werden
    if (m_fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;                // Optional Vollbild
    m_window = SDL_CreateWindow(m_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_windowW, m_windowH, flags); // Fenster anlegen
    if (!m_window) {                                                         // Fehlgeschlagen?
        std::cerr << "Fenster konnte nicht erstellt werden: " << SDL_GetError() << "\n"; // Fehlermeldung
        return false;                                                        // Abbrechen
    }                                                                        // Ende der Prüfung
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);    // Software-Renderer anlegen
    if (!m_renderer) {                                                       // Fehlgeschlagen?
        std::cerr << "Renderer konnte nicht erstellt werden: " << SDL_GetError() << "\n"; // Fehlermeldung
        return false;                                                        // Abbrechen
    }                                                                        // Ende der Prüfung
    SDL_RenderSetLogicalSize(m_renderer, m_fbW, m_fbH);                      // Bildspeichergröße festlegen (SDL skaliert aufs Fenster)
    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_fbW, m_fbH); // Textur für unser Bild
    if (!m_texture) {                                                        // Fehlgeschlagen?
        std::cerr << "Textur konnte nicht erstellt werden: " << SDL_GetError() << "\n"; // Fehlermeldung
        return false;                                                        // Abbrechen
    }                                                                        // Ende der Prüfung
    SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_NONE);                  // Textur ohne Durchsichtigkeit kopieren
    m_frame.resize(m_fbW, m_fbH, rgba(0, 0, 0));                             // Bildspeicher anlegen
    std::cout << "Bildspeicher: " << m_fbW << "x" << m_fbH << " Pixel, Kachelgroesse " << m_tile << " Pixel\n"; // Hinweis ausgeben
    return true;                                                             // Erfolgreich
} // Ende von initSdl

// Lädt eigene Sprites oder erzeugt Platzhalter für alle Spielgrafiken
void Game::buildAssets() {                                                   // Beginn von buildAssets
    const int T = m_tile;                                                    // Kachelgröße
    m_assets.configure(ImageIO::joinPath(m_baseDir, "assets"), ImageIO::joinPath(m_baseDir, "assets_dummies"), T, m_exportDummies); // Ordner festlegen
    m_assets.loadOrGenerate("spieler", m_anims.columns() * T, m_anims.rows() * T, [&](Image& img) { SpriteFactory::drawPlayerSheet(img, T, m_anims); }); // Spielfigur
    int coinSize = std::max(8, T * 34 / 100);                                // Größe einer Münze
    m_assets.loadOrGenerate("muenze", coinSize * 6, coinSize, [](Image& img) { SpriteFactory::drawCoinStrip(img, 6); }); // Münze mit 6 Drehbildern
    const Color carColors[3] = {rgba(210, 50, 45), rgba(50, 110, 210), rgba(240, 190, 40)}; // Rot, Blau, Gelb
    for (int i = 0; i < 3; ++i) {                                            // Drei Autofarben
        Color body = carColors[i];                                           // Farbe dieses Autos
        m_assets.loadOrGenerate("auto_" + std::to_string(i), T * 13 / 10, T * 95 / 100, [body](Image& img) { SpriteFactory::drawCarFront(img, body); }); // Auto von vorne
    }                                                                        // Ende der Schleife
    m_assets.loadOrGenerate("ampel_rot", T / 2, T * 24 / 10, [T](Image& img) { SpriteFactory::drawTrafficLight(img, T, false); }); // Ampel rot
    m_assets.loadOrGenerate("ampel_gruen", T / 2, T * 24 / 10, [T](Image& img) { SpriteFactory::drawTrafficLight(img, T, true); }); // Ampel grün
    m_assets.loadOrGenerate("boden", T, T, [T](Image& img) { SpriteFactory::drawGroundTile(img, T, false); }); // Gehweg
    m_assets.loadOrGenerate("strasse", T, T, [T](Image& img) { SpriteFactory::drawGroundTile(img, T, true); }); // Straße
    m_assets.loadOrGenerate("ziel", T * 4, T * 26 / 10, [T](Image& img) { SpriteFactory::drawGoalFlag(img, T, 4); }); // Zielflagge
    int trashSize = std::max(6, T * 34 / 100);                               // Größe eines Müllteils
    for (int i = 0; i < 5; ++i) {                                            // Fünf Müllarten
        m_assets.loadOrGenerate("muell_" + std::to_string(i), trashSize, trashSize, [i](Image& img) { SpriteFactory::drawTrash(img, i); }); // Müll
    }                                                                        // Ende der Schleife
    int ringSize = std::max(6, T * 36 / 100);                                // Größe des Ankerrings
    m_assets.loadOrGenerate("anker", ringSize, ringSize, [](Image& img) { SpriteFactory::drawAnchorRing(img); }); // Ankerring
    int iconSize = 16 * m_ui;                                                // Größe der Gegenstandssymbole
    for (const std::string& id : m_items.ids()) {                            // Alle Gegenstände
        const ItemDef* def = m_items.find(id);                               // Definition
        std::string icon = def ? def->icon : "unbekannt";                    // Symbolname
        m_assets.loadOrGenerate("symbol_" + PropertyFile::toLower(id), iconSize, iconSize, [icon](Image& img) { SpriteFactory::drawItemIcon(img, icon); }); // Symbol
    }                                                                        // Ende der Schleife
    int hudSize = 8 * m_ui;                                                  // Größe der Leistensymbole
    m_assets.loadOrGenerate("hud_herz", hudSize, hudSize, [](Image& img) { SpriteFactory::drawHudIcon(img, "herz"); }); // Herz
    m_assets.loadOrGenerate("hud_blitz", hudSize, hudSize, [](Image& img) { SpriteFactory::drawHudIcon(img, "blitz"); }); // Blitz
    m_assets.loadOrGenerate("hud_coin", hudSize, hudSize, [](Image& img) { SpriteFactory::drawHudIcon(img, "coin"); }); // Münze
    for (const ShopSpot& shop : m_level.shops) {                             // Alle Shops der Karte
        std::string sign = shop.sign;                                        // Schildtext
        m_assets.loadOrGenerate("shop_" + shop.id, static_cast<int>(shop.width * static_cast<float>(T)), static_cast<int>(shop.height * static_cast<float>(T)), [T, sign](Image& img) { SpriteFactory::drawShop(img, T, sign); }); // Gebäude
    }                                                                        // Ende der Schleife
    m_background.build(m_assets, m_fbW, m_fbH, T, m_level.groundY);          // Hintergrundebenen
    std::cout << "Eigene Sprites geladen: " << m_assets.customCount() << ", neue Vorlagen exportiert: " << m_assets.exportedCount() << "\n"; // Hinweis
} // Ende von buildAssets

// Berechnet die Positionen von Menüknöpfen, Leiste, Inventar und Shop
void Game::layoutUi() {                                                      // Beginn von layoutUi
    const int s = m_ui;                                                      // Skalierung
    m_hud.layout(m_fbW, m_fbH, s);                                           // Menüleiste und Inventar
    m_shop.layout(m_fbW, m_fbH, m_hud.barHeight(), s);                       // Shop-Fenster
    m_menuButtons.clear();                                                   // Alte Menüknöpfe entfernen
    const char* labels[3] = {"Neues Spiel", "Letzter Spielstand", "Beenden"}; // Beschriftungen der drei Knöpfe
    int bw = 120 * s;                                                        // Knopfbreite
    int bh = 18 * s;                                                         // Knopfhöhe
    int y0 = m_fbH / 2 + 2 * s;                                              // Erster Knopf knapp unter der Bildmitte
    for (int i = 0; i < 3; ++i) {                                            // Drei Knöpfe
        m_menuButtons.push_back(Button{RectI{(m_fbW - bw) / 2, y0 + i * (bh + 5 * s), bw, bh}, labels[i], true}); // Knopf anlegen
    }                                                                        // Ende der Schleife
} // Ende von layoutUi

// Gibt alle SDL-Ressourcen frei
void Game::shutdown() {                                                      // Beginn von shutdown
    if (m_texture) SDL_DestroyTexture(m_texture);                            // Textur freigeben
    if (m_renderer) SDL_DestroyRenderer(m_renderer);                         // Renderer freigeben
    if (m_window) SDL_DestroyWindow(m_window);                               // Fenster schließen
    m_texture = nullptr;                                                     // Zeiger zurücksetzen
    m_renderer = nullptr;                                                    // Zeiger zurücksetzen
    m_window = nullptr;                                                      // Zeiger zurücksetzen
    SDL_Quit();                                                              // SDL beenden
} // Ende von shutdown

// Verarbeitet ein Ereignis von SDL
void Game::handleEvent(const SDL_Event& e) {                                 // Beginn von handleEvent
    switch (e.type) {                                                        // Je nach Ereignisart
    case SDL_QUIT:                                                           // Fenster wurde geschlossen
        if (m_screen == Screen::Playing && !m_levelDone && m_popupKind != PopupKind::GameOver) saveGame(false); // Fortschritt sichern
        m_running = false;                                                   // Spielschleife beenden
        break;                                                               // Ende QUIT
    case SDL_KEYDOWN:                                                        // Taste gedrückt
        if (e.key.repeat == 0) handleKeyDown(e.key.keysym.scancode);         // Automatische Wiederholung ignorieren
        break;                                                               // Ende KEYDOWN
    case SDL_MOUSEMOTION:                                                    // Maus bewegt
        m_mouseX = e.motion.x;                                               // Neue Position x (bereits im Bildspeicher-Maßstab)
        m_mouseY = e.motion.y;                                               // Neue Position y
        break;                                                               // Ende MOUSEMOTION
    case SDL_MOUSEBUTTONDOWN:                                                // Maustaste gedrückt
        m_mouseX = e.button.x;                                               // Position x beim Klick
        m_mouseY = e.button.y;                                               // Position y beim Klick
        if (e.button.button == SDL_BUTTON_LEFT) handleClick(m_mouseX, m_mouseY); // Nur die linke Taste auswerten
        break;                                                               // Ende MOUSEBUTTONDOWN
    default:                                                                 // Andere Ereignisse
        break;                                                               // Werden ignoriert
    }                                                                        // Ende der Fallunterscheidung
} // Ende von handleEvent

// Verarbeitet einen Tastendruck
void Game::handleKeyDown(SDL_Scancode key) {                                 // Beginn von handleKeyDown
    if (key == SDL_SCANCODE_F12) { takeScreenshot(); return; }               // F12: Bildschirmfoto
    if (m_popup.isOpen()) {                                                  // Ein Popup ist offen
        if (key == SDL_SCANCODE_LEFT || key == m_keyLeft) m_popup.moveSelection(-1); // Auswahl nach links
        else if (key == SDL_SCANCODE_RIGHT || key == m_keyRight) m_popup.moveSelection(1); // Auswahl nach rechts
        else if (key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER || key == m_keyJump || key == m_keyInteract) popupChoice(m_popup.selection()); // Bestätigen
        else if (key == SDL_SCANCODE_ESCAPE && m_popupKind == PopupKind::QuitConfirm) popupChoice(1); // Escape = "Nein"
        return;                                                              // Sonst nichts
    }                                                                        // Ende Popup
    if (m_screen == Screen::MainMenu) {                                      // Im Hauptmenü
        int count = static_cast<int>(m_menuButtons.size());                  // Anzahl der Knöpfe
        if (key == SDL_SCANCODE_UP || key == SDL_SCANCODE_W) {               // Nach oben
            do { m_menuSelection = (m_menuSelection + count - 1) % count; } while (!m_menuButtons[static_cast<std::size_t>(m_menuSelection)].enabled); // Vorherigen benutzbaren Knopf wählen
        } else if (key == SDL_SCANCODE_DOWN || key == SDL_SCANCODE_S) {      // Nach unten
            do { m_menuSelection = (m_menuSelection + 1) % count; } while (!m_menuButtons[static_cast<std::size_t>(m_menuSelection)].enabled); // Nächsten benutzbaren Knopf wählen
        } else if (key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER || key == SDL_SCANCODE_SPACE) { // Bestätigen
            menuActivate(m_menuSelection);                                   // Knopf auslösen
        }                                                                    // Ende der Unterscheidung
        return;                                                              // Fertig
    }                                                                        // Ende Hauptmenü
    if (m_shop.isOpen()) {                                                   // Shop ist offen
        if (key == m_keyInteract || key == SDL_SCANCODE_ESCAPE) closeShop(); // E oder Escape schließt den Shop
        else if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_8) useSlot(key - SDL_SCANCODE_1); // Gegenstand benutzen
        return;                                                              // Sonst nichts
    }                                                                        // Ende Shop
    if (key == SDL_SCANCODE_ESCAPE) { openPopup(PopupKind::QuitConfirm); return; } // Escape fragt nach dem Beenden
    if (key == m_keyJump) m_jumpPressed = true;                              // Sprung für den nächsten Physikschritt merken
    else if (key == m_keyHook) m_hookPressed = true;                         // Enterhaken für den nächsten Physikschritt merken
    else if (key == m_keyInteract) interact();                               // Interagieren
    else if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_8) useSlot(key - SDL_SCANCODE_1); // Tasten 1-8: Inventarfeld benutzen
} // Ende von handleKeyDown

// Verarbeitet einen Klick mit der linken Maustaste
void Game::handleClick(int mx, int my) {                                     // Beginn von handleClick
    if (m_popup.isOpen()) {                                                  // Popup ist offen
        int index = m_popup.hit(mx, my);                                     // Getroffener Knopf
        if (index >= 0) popupChoice(index);                                  // Auswahl ausführen
        return;                                                              // Andere Klicks sind gesperrt
    }                                                                        // Ende Popup
    if (m_screen == Screen::MainMenu) {                                      // Hauptmenü
        for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {             // Alle Knöpfe prüfen
            if (m_menuButtons[i].enabled && m_menuButtons[i].rect.contains(mx, my)) menuActivate(static_cast<int>(i)); // Knopf auslösen
        }                                                                    // Ende der Schleife
        return;                                                              // Fertig
    }                                                                        // Ende Hauptmenü
    if (m_hud.quitButtonHit(mx, my)) { openPopup(PopupKind::QuitConfirm); return; } // Beenden-Knopf ganz links
    int slot = m_hud.inventorySlotAt(mx, my);                                // Inventarfeld unter der Maus
    if (slot >= 0) { useSlot(slot); return; }                                // Gegenstand benutzen
    if (m_shop.isOpen()) {                                                   // Shop ist offen
        std::string itemId;                                                  // Gewählter Gegenstand
        ShopMenu::Action action = m_shop.click(mx, my, itemId);              // Klick auswerten
        if (action == ShopMenu::Action::Close) closeShop();                  // Schließen
        else if (action == ShopMenu::Action::Buy) buyItem(itemId);           // Kaufen
    }                                                                        // Ende Shop
} // Ende von handleClick

// Aktualisiert den aktuellen Bildschirm
void Game::update(float dt) {                                                // Beginn von update
    m_time += dt;                                                            // Gesamtzeit erhöhen
    m_warnCooldown = std::max(0.0f, m_warnCooldown - dt);                    // Warn-Wartezeit verringern
    if (m_screen == Screen::MainMenu) {                                      // Hauptmenü
        m_menuScroll += dt * static_cast<float>(m_tile) * 0.8f;              // Hintergrund langsam vorbeiziehen lassen
        m_menuButtons[1].enabled = saveExists();                             // "Letzter Spielstand" nur mit Spielstand
        if (!m_menuButtons[static_cast<std::size_t>(m_menuSelection)].enabled) m_menuSelection = 0; // Auswahl auf benutzbaren Knopf
        return;                                                              // Fertig
    }                                                                        // Ende Hauptmenü
    updatePlaying(dt);                                                       // Spiel aktualisieren
} // Ende von update

// Aktualisiert das laufende Spiel
void Game::updatePlaying(float dt) {                                         // Beginn von updatePlaying
    m_hud.update(dt);                                                        // Meldungen altern lassen
    if (m_popup.isOpen() || m_shop.isOpen()) {                               // Popup oder Shop offen: Spielwelt pausiert
        m_physicsAccumulator = 0.0f;                                         // Keine Zeit ansammeln
        return;                                                              // Fertig
    }                                                                        // Ende Pause
    m_level.update(dt);                                                      // Ampeln und Autos
    const Uint8* keys = SDL_GetKeyboardState(nullptr);                       // Aktuell gedrückte Tasten
    PlayerInput input;                                                       // Eingaben für die Figur
    input.moveDir = (keys[m_keyRight] ? 1 : 0) - (keys[m_keyLeft] ? 1 : 0);  // A/D zu Richtung -1/0/1 zusammenfassen
    input.jump = m_jumpPressed;                                              // Sprungwunsch
    input.hook = m_hookPressed;                                              // Hakenwunsch
    m_physicsAccumulator += dt;                                              // Zeit ansammeln
    int steps = 0;                                                           // Anzahl der Schritte in diesem Bild
    while (m_physicsAccumulator >= PHYSICS_STEP && steps < MAX_PHYSICS_STEPS) { // Feste Physikschritte
        PlayerEvents events;                                                 // Ereignisse dieses Schritts
        m_player.update(PHYSICS_STEP, input, m_level, hasHook(), maxStamina(), events); // Figur bewegen
        handlePlayerEvents(events);                                          // Meldungen ausgeben
        checkWorld();                                                        // Welt prüfen
        input.jump = false;                                                  // Tastendrücke nur einmal auswerten
        input.hook = false;                                                  // Tastendrücke nur einmal auswerten
        m_physicsAccumulator -= PHYSICS_STEP;                                // Zeit verbrauchen
        ++steps;                                                             // Schritt zählen
        if (m_popup.isOpen()) break;                                         // Popup (Ziel/Game Over) stoppt die Physik
    }                                                                        // Ende der Physikschleife
    if (steps >= MAX_PHYSICS_STEPS) m_physicsAccumulator = 0.0f;             // Zu viel Rückstand verwerfen
    if (steps > 0) {                                                         // Es wurde mindestens ein Schritt gerechnet
        m_jumpPressed = false;                                               // Sprungwunsch verbraucht
        m_hookPressed = false;                                               // Hakenwunsch verbraucht
    }                                                                        // Ende der Prüfung
    m_player.updateAnimation(dt);                                            // Animation wählen
    m_effects.update(dt);                                                    // Effekte bewegen
    updateCamera(dt, false);                                                 // Kamera nachführen
} // Ende von updatePlaying

// Gibt Meldungen zu Ereignissen der Figur aus
void Game::handlePlayerEvents(const PlayerEvents& ev) {                      // Beginn von handlePlayerEvents
    if (ev.noHook && m_warnCooldown <= 0.0f) {                               // Q ohne Enterhaken
        m_hud.showMessage("Du brauchst einen Enterhaken - den gibt es im Shop!"); // Hinweis
        m_warnCooldown = 1.0f;                                               // Nicht sofort wiederholen
    }                                                                        // Ende
    if (ev.noStamina && m_warnCooldown <= 0.0f) {                            // Zu wenig Ausdauer
        m_hud.showMessage("Zu wenig Ausdauer! Kurz verschnaufen oder Energy trinken."); // Hinweis
        m_warnCooldown = 1.0f;                                               // Nicht sofort wiederholen
    }                                                                        // Ende
    if (ev.hookMissed) m_hud.showMessage("Kein Ankerpunkt in Reichweite.");  // Fehlwurf
} // Ende von handlePlayerEvents

// Prüft Münzen, Stürze in Gräben, Autos und das Ziel
void Game::checkWorld() {                                                    // Beginn von checkWorld
    RectF box = m_player.bounds();                                           // Kollisionsbox der Figur
    bool active = m_player.state != PlayerState::Respawning;                 // Ist die Figur gerade im Spiel?
    for (Coin& coin : m_level.coins) {                                       // Alle Münzen
        if (coin.collected || !active) continue;                             // Schon eingesammelt oder Figur unsichtbar
        RectF coinBox{coin.x - 0.17f, coin.y - 0.17f, 0.34f, 0.34f};         // Kollisionsbox der Münze
        if (!box.intersects(coinBox)) continue;                              // Nicht berührt
        coin.collected = true;                                               // Einsammeln
        m_player.coins += coin.value;                                        // Coins gutschreiben
        m_effects.sparkle(coin.x, coin.y, rgba(255, 230, 120));              // Funkeln
        m_effects.floatingText(coin.x, coin.y - 0.3f, "+" + std::to_string(coin.value), Ui::GOLD); // "+5" anzeigen
    }                                                                        // Ende der Münzschleife

    if (active) {                                                            // Stürze nur prüfen, wenn die Figur sichtbar ist
        const Ditch* ditch = m_level.ditchAt(m_player.x);                    // Graben unter der Figur
        if (ditch && m_player.y > m_level.groundY + ditch->fallLine()) {     // Zu tief gefallen
            if (ditch->water) m_effects.splash(m_player.x, m_level.groundY + ditch->waterLevel); // Wasserspritzer
            m_player.health = std::max(0.0f, m_player.health - ditch->damage); // Schaden abziehen
            char text[96];                                                   // Puffer für die Meldung
            std::snprintf(text, sizeof(text), ditch->water ? "Platsch! Ins Wasser gefallen. (-%d Leben)" : "Autsch! In den Graben gefallen. (-%d Leben)", static_cast<int>(ditch->damage)); // Meldung
            m_hud.showMessage(text);                                         // Meldung zeigen
            bool fromLeft = m_player.lastSafeX < ditch->x + ditch->width * 0.5f; // Von welcher Seite kam die Figur?
            float respawnX = fromLeft ? ditch->x - 0.5f : ditch->x + ditch->width + 0.5f; // Sichere Stelle neben dem Graben
            m_player.startRespawn(respawnX, m_level.groundY);                // Wieder erscheinen lassen
            m_player.facing = fromLeft ? 1 : -1;                             // Beim Wiedererscheinen zum Graben schauen
            if (m_player.health <= 0.0f) openPopup(PopupKind::GameOver);     // Kein Leben mehr
            return;                                                          // Rest in diesem Schritt überspringen
        }                                                                    // Ende Sturz
    }                                                                        // Ende aktiv

    int street = active ? m_level.streetAt(m_player.x) : -1;                 // Straße unter der Figur
    bool onStreet = street >= 0 && m_player.y > m_level.groundY - 1.2f;      // Figur steht (oder hüpft) auf der Straße
    for (std::size_t i = 0; i < m_level.lights.size(); ++i) {                // Alle Ampeln
        TrafficLight& light = m_level.lights[i];                             // Aktuelle Ampel
        bool here = onStreet && static_cast<int>(i) == street;               // Ist die Figur auf dieser Straße?
        if (here && !light.green && light.phaseTimer >= light.clearance) {   // Figur läuft bei Rot über die Straße
            bool approaching = false;                                        // Kommt schon ein Auto?
            for (const Car& car : light.cars) if (Level::carDepth(car) < 1.0f) approaching = true; // Auto vor der Furt gefunden
            if (!approaching && light.canSpawnCar()) {                       // Kein Auto unterwegs
                light.spawnCar(0.25f);                                       // Ein Auto rast heran
                m_effects.floatingText(light.centerX(), m_level.groundY - 1.6f, "HUP! HUP!", Ui::RED); // Hupen anzeigen
            }                                                                // Ende Autoerzeugung
        }                                                                    // Ende Rot-Prüfung
        for (Car& car : light.cars) {                                        // Alle Autos dieser Ampel
            if (car.hitChecked || Level::carDepth(car) < 1.0f) continue;     // Erst prüfen, wenn das Auto die Furt erreicht
            car.hitChecked = true;                                           // Nur einmal pro Auto prüfen
            if (!here) continue;                                             // Figur nicht auf der Straße -> kein Unfall
            if (!m_player.damage(light.damage)) continue;                    // Figur gerade geschützt
            bool leftSide = m_player.x < light.centerX();                    // Auf welcher Seite ist der nächste Bordstein?
            m_player.x = leftSide ? light.x - 0.45f : light.x + light.streetWidth + 0.45f; // Auf den Bordstein zurückschieben
            m_player.vx = 0.0f;                                              // Bewegung stoppen
            m_player.vy = -4.0f;                                             // Kleiner Hüpfer durch den Schreck
            char text[96];                                                   // Puffer für die Meldung
            std::snprintf(text, sizeof(text), "Rot heißt warten! Ein Auto hat dich erwischt. (-%d Leben)", static_cast<int>(light.damage)); // Meldung
            m_hud.showMessage(text, 3.0f);                                   // Meldung zeigen
            m_effects.sparkle(m_player.x, m_player.y - 0.5f, rgba(255, 90, 80)); // Roter Effekt
            if (m_player.health <= 0.0f) openPopup(PopupKind::GameOver);     // Kein Leben mehr
        }                                                                    // Ende der Autoschleife
    }                                                                        // Ende der Ampelschleife

    if (active && !m_levelDone && m_player.x >= m_level.goalX - 0.1f) {      // Ziel erreicht?
        m_levelDone = true;                                                  // Merken
        m_effects.sparkle(m_level.goalX, m_level.groundY - 2.0f, Ui::GOLD);  // Feier-Effekt
        openPopup(PopupKind::LevelComplete);                                 // Popup "Level-Ende erreicht"
    }                                                                        // Ende Ziel
} // Ende von checkWorld

// Lässt die Kamera der Figur folgen (Figur etwas links von der Mitte)
void Game::updateCamera(float dt, bool snap) {                               // Beginn von updateCamera
    float viewW = static_cast<float>(m_fbW) / static_cast<float>(m_tile);    // Sichtbare Breite in Kacheln
    float target = m_player.x - viewW * 0.42f;                               // Gewünschte Kameraposition
    target = clampValue(target, 0.0f, std::max(0.0f, m_level.length - viewW)); // Nicht über die Kartenränder
    if (snap) m_camX = target;                                               // Sofort springen
    else m_camX = lerp(m_camX, target, std::min(1.0f, dt * 8.0f));           // Weich nachziehen
} // Ende von updateCamera

// Zeichnet das aktuelle Bild in den Bildspeicher
void Game::render() {                                                        // Beginn von render
    Canvas canvas(m_frame);                                                  // Zeichenfläche
    if (m_screen == Screen::MainMenu) renderMenu(canvas);                    // Hauptmenü
    else {                                                                   // Spiel
        renderWorld(canvas);                                                 // Welt
        renderHud(canvas);                                                   // Oberfläche
    }                                                                        // Ende der Unterscheidung
    if (m_popup.isOpen()) {                                                  // Popup offen
        canvas.darken(45);                                                   // Alles dahinter abdunkeln
        m_popup.draw(canvas, m_mouseX, m_mouseY, m_ui);                      // Popup zeichnen
    }                                                                        // Ende Popup
} // Ende von render

// Kopiert den Bildspeicher ins Fenster (SDL skaliert in Software auf die Fenstergröße)
void Game::present() {                                                       // Beginn von present
    SDL_UpdateTexture(m_texture, nullptr, m_frame.pixels.data(), m_fbW * static_cast<int>(sizeof(Color))); // Pixel in die Textur kopieren
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);                        // Schwarz für die Ränder
    SDL_RenderClear(m_renderer);                                             // Fenster löschen
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);                 // Textur ins Fenster kopieren
    SDL_RenderPresent(m_renderer);                                           // Anzeigen
} // Ende von present

// Zeichnet das Hauptmenü
void Game::renderMenu(Canvas& canvas) {                                      // Beginn von renderMenu
    const int s = m_ui;                                                      // Skalierung
    const int T = m_tile;                                                    // Kachelgröße
    m_background.draw(canvas, m_menuScroll, m_time);                         // Hintergrund zieht vorbei
    const Image& ground = m_assets.get("boden");                             // Gehweg-Kachel
    int groundPx = static_cast<int>(m_level.groundY * static_cast<float>(T)); // Bodenkante
    int offset = static_cast<int>(m_menuScroll) % T;                         // Verschiebung der Kacheln
    for (int x = -offset; x < m_fbW; x += T) canvas.blit(ground, x, groundPx); // Gehweg über die ganze Breite
    canvas.fillRect(0, groundPx + T, m_fbW, m_fbH - groundPx - T, rgba(100, 70, 45)); // Erde darunter
    const Image& sheet = m_assets.get("spieler");                            // Sprite-Sheet der Figur
    const AnimationDef& walk = m_anims.get(PlayerAnim::WalkRight);           // Laufanimation nach rechts
    int frame = static_cast<int>(m_time * walk.fps) % walk.frames;           // Aktuelles Laufbild
    canvas.blitRegion(sheet, RectI{frame * T, walk.row * T, T, T}, m_fbW / 5 - T / 2, groundPx - T); // Figur läuft auf der Stelle

    std::string title = "Mini Ship Delivery";                                // Spielname
    int ts = 2 * s;                                                          // Doppelte Schriftgröße für den Titel
    int tw = Font::textWidth(title, ts);                                     // Titelbreite
    int tx = (m_fbW - tw) / 2;                                               // Titel waagerecht mittig
    int ty = m_fbH / 4 - 4 * s;                                              // Titel in der oberen Hälfte
    for (int dy = -1; dy <= 1; ++dy) {                                       // Dunkler Umriss: alle Nachbarpositionen
        for (int dx = -1; dx <= 1; ++dx) {                                   // ... in x und y
            if (dx != 0 || dy != 0) Font::drawText(canvas, tx + dx * s, ty + dy * s, title, rgba(40, 25, 20), ts); // Umriss zeichnen
        }                                                                    // Ende x
    }                                                                        // Ende y
    Font::drawText(canvas, tx + s, ty + 2 * s, title, rgba(40, 25, 20), ts); // Schatten
    Font::drawText(canvas, tx, ty, title, rgba(255, 200, 60), ts);           // Titel in Gold
    Ui::drawCenteredText(canvas, m_fbW / 2, ty + 22 * s, "Ein Pixelart-Lieferabenteuer", Ui::TEXT, s); // Untertitel

    for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {                 // Alle Menüknöpfe
        const Button& b = m_menuButtons[i];                                  // Aktueller Knopf
        bool mouseOver = b.rect.contains(m_mouseX, m_mouseY);                // Maus darüber?
        bool selected = static_cast<int>(i) == m_menuSelection;              // Per Tastatur gewählt?
        Ui::drawButton(canvas, b, b.enabled && (mouseOver || selected), s);  // Knopf zeichnen
    }                                                                        // Ende der Schleife
    Ui::drawCenteredText(canvas, m_fbW / 2, m_fbH - 21 * s, "A/D: Laufen   Leertaste: Springen", Ui::TEXT, s); // Steuerung Zeile 1
    Ui::drawCenteredText(canvas, m_fbW / 2, m_fbH - 11 * s, "E: Interagieren   Q: Enterhaken", Ui::TEXT, s); // Steuerung Zeile 2
} // Ende von renderMenu

// Zeichnet die Spielwelt
void Game::renderWorld(Canvas& canvas) {                                     // Beginn von renderWorld
    const int T = m_tile;                                                    // Kachelgröße
    m_background.draw(canvas, m_camX * static_cast<float>(T), m_time);       // Parallax-Hintergrund
    m_level.drawBack(canvas, m_assets, m_camX, m_time);                      // Alles hinter der Figur
    if (hasHook() && m_player.state == PlayerState::Normal) {                // Mögliches Ziel für den Enterhaken markieren
        if (const Anchor* a = m_player.findAnchor(m_level)) {                // Ankerpunkt in Reichweite?
            int ax = static_cast<int>((a->x - m_camX) * static_cast<float>(T)); // Bildschirmposition x
            int ay = static_cast<int>(a->y * static_cast<float>(T));         // Bildschirmposition y
            int r = T / 4 + static_cast<int>(std::sin(m_time * 6.0f) * static_cast<float>(T) / 20.0f); // Pulsierender Radius
            canvas.ring(ax, ay, r, std::max(1, T / 40), rgba(255, 230, 80, 220)); // Gelber Ring
            Font::drawTextShadow(canvas, ax + r + 3 * m_ui, ay - 4 * m_ui, "Q", Ui::GOLD, Ui::SHADOW, m_ui); // Tastenhinweis rechts neben dem Ring
        }                                                                    // Ende Ankerpunkt
    }                                                                        // Ende Markierung
    m_player.drawRope(canvas, m_camX, T);                                    // Seil
    m_player.draw(canvas, m_assets, m_camX, m_time);                         // Figur
    m_level.drawFront(canvas, m_assets, m_camX, m_time);                     // Wasser und Autos vorne
    m_effects.draw(canvas, m_camX, T, m_ui);                                 // Effekte
} // Ende von renderWorld

// Zeichnet Menüleiste, Inventar, Meldungen, Shop und Tooltips
void Game::renderHud(Canvas& canvas) {                                       // Beginn von renderHud
    if (m_shop.isOpen()) canvas.fillRect(0, 0, m_fbW, m_fbH, rgba(0, 0, 0, 110)); // Welt hinter dem Shop abdunkeln
    m_hud.drawTopBar(canvas, m_assets, m_player.health, m_player.stats().maxHealth, m_player.stamina, maxStamina(), m_player.coins, m_mouseX, m_mouseY); // Menüleiste oben
    m_hud.drawInventory(canvas, m_assets, m_inventory, m_mouseX, m_mouseY);  // Inventar unten links
    if (!m_shop.isOpen() && !m_popup.isOpen()) m_hud.drawPrompt(canvas, currentPrompt()); // Bedienhinweis
    if (m_shop.isOpen()) m_shop.draw(canvas, m_assets, m_items, m_inventory, m_player.coins, m_mouseX, m_mouseY); // Shop-Fenster
    m_hud.drawMessage(canvas);                                               // Meldung
    if (m_popup.isOpen()) return;                                            // Bei offenem Popup keine Tooltips
    m_shop.drawTooltip(canvas, m_items, m_inventory, m_player.coins, m_mouseX, m_mouseY); // Tooltip im Shop
    m_hud.drawTooltips(canvas, m_inventory, m_items, m_mouseX, m_mouseY);    // Tooltips von Leiste und Inventar
} // Ende von renderHud

// Liefert den passenden Bedienhinweis für die aktuelle Position der Figur
std::string Game::currentPrompt() const {                                    // Beginn von currentPrompt
    if (m_player.state == PlayerState::HookSwing) return "A/D: Schwung holen   Leertaste/Q: Loslassen"; // Am Seil
    if (m_player.state != PlayerState::Normal) return "";                    // Andere Zustände ohne Hinweis
    for (const ShopSpot& shop : m_level.shops) {                             // Alle Shops
        if (std::fabs(m_player.x - shop.doorX()) < 0.6f) return "E: " + shop.name + " betreten"; // Vor der Tür
    }                                                                        // Ende der Schleife
    for (const TrafficLight& light : m_level.lights) {                       // Alle Ampeln
        if (std::fabs(m_player.x - light.poleX()) < 0.8f) {                  // Am Ampelmast
            if (light.green) return "Grün - du kannst gehen!";               // Grün
            return light.buttonPressed ? "Signal kommt gleich ..." : "Rot! E: Ampel-Taster drücken"; // Rot
        }                                                                    // Ende der Prüfung
    }                                                                        // Ende der Schleife
    if (hasHook() && m_player.findAnchor(m_level)) return "Q: Enterhaken werfen"; // Ankerpunkt in Reichweite
    if (!hasHook()) {                                                        // Ohne Enterhaken
        for (const Ditch& d : m_level.ditches) {                             // Alle Gräben
            if (d.width > 2.2f && m_player.x > d.x - 1.6f && m_player.x < d.x) return "Zu breit! Hier hilft nur ein Enterhaken."; // Zu breiter Graben voraus
        }                                                                    // Ende der Schleife
    }                                                                        // Ende der Prüfung
    return "";                                                               // Kein Hinweis
} // Ende von currentPrompt

// Startet ein neues Spiel
void Game::startNewGame() {                                                  // Beginn von startNewGame
    m_level.resetRuntime();                                                  // Münzen und Ampeln zurücksetzen
    m_inventory.clear();                                                     // Inventar leeren
    m_player.reset(m_level.startX, m_level.groundY);                         // Figur an den Start
    m_effects.clear();                                                       // Effekte entfernen
    m_popup.close();                                                         // Popup schließen
    m_popupKind = PopupKind::None;                                           // Kein Popup
    m_shop.close();                                                          // Shop schließen
    m_levelDone = false;                                                     // Ziel noch nicht erreicht
    m_jumpPressed = false;                                                   // Keine alten Tastendrücke
    m_hookPressed = false;                                                   // Keine alten Tastendrücke
    m_physicsAccumulator = 0.0f;                                             // Physikzeit zurücksetzen
    m_screen = Screen::Playing;                                              // Ins Spiel wechseln
    updateCamera(0.0f, true);                                                // Kamera sofort zur Figur
    m_hud.showMessage("Tipp: Im Shop gibt es einen Enterhaken!", 4.0f);      // Erster Hinweis
} // Ende von startNewGame

// Lädt den letzten Spielstand
void Game::continueGame() {                                                  // Beginn von continueGame
    SaveData data;                                                           // Spielstanddaten
    if (!SaveGame::read(m_savePath, data)) {                                 // Lesen fehlgeschlagen
        startNewGame();                                                      // Dann neues Spiel
        m_hud.showMessage("Kein Spielstand gefunden - neues Spiel gestartet."); // Hinweis
        return;                                                              // Fertig
    }                                                                        // Ende der Prüfung
    startNewGame();                                                          // Grundzustand herstellen
    m_player.x = clampValue(data.x, 0.5f, m_level.goalX - 1.0f);             // Position (nie hinter dem Ziel)
    m_player.lastSafeX = m_player.x;                                         // Als sichere Position merken
    m_player.health = clampValue(data.health, 1.0f, m_player.stats().maxHealth); // Leben
    m_player.coins = std::max(0, data.coins);                                // Coins
    for (int i = 0; i < Inventory::SIZE; ++i) {                              // Inventar übernehmen
        const InventorySlot& slot = data.slots[static_cast<std::size_t>(i)]; // Gespeichertes Feld
        if (!slot.empty() && m_items.find(slot.itemId)) m_inventory.setSlot(i, slot); // Nur bekannte Gegenstände
    }                                                                        // Ende der Schleife
    m_player.stamina = clampValue(data.stamina, 0.0f, maxStamina());         // Ausdauer (nach dem Inventar, wegen Bonus)
    for (Coin& coin : m_level.coins) {                                       // Eingesammelte Münzen markieren
        for (const std::string& id : data.collectedCoins) if (PropertyFile::toLower(id) == PropertyFile::toLower(coin.id)) coin.collected = true; // Gefunden -> eingesammelt
    }                                                                        // Ende der Schleife
    updateCamera(0.0f, true);                                                // Kamera zur Figur
    m_hud.showMessage("Spielstand vom " + data.savedAt + " geladen.", 3.0f); // Hinweis
} // Ende von continueGame

// Speichert den aktuellen Spielstand
void Game::saveGame(bool showMessage) {                                      // Beginn von saveGame
    SaveData data;                                                           // Neue Daten
    data.level = m_level.fileName;                                           // Karte
    data.x = m_player.lastSafeX;                                             // Letzte sichere Position (nie über einem Graben)
    data.health = m_player.health;                                           // Leben
    data.stamina = m_player.stamina;                                         // Ausdauer
    data.coins = m_player.coins;                                             // Coins
    for (int i = 0; i < Inventory::SIZE; ++i) data.slots[static_cast<std::size_t>(i)] = m_inventory.slot(i); // Inventar
    for (const Coin& coin : m_level.coins) if (coin.collected) data.collectedCoins.push_back(coin.id); // Eingesammelte Münzen
    std::size_t slash = m_savePath.find_last_of('/');                        // Ordner des Spielstands
    if (slash != std::string::npos) ImageIO::makeDirectories(m_savePath.substr(0, slash)); // Ordner anlegen
    bool ok = SaveGame::write(m_savePath, data);                             // Datei schreiben
    if (showMessage) m_hud.showMessage(ok ? "Spielstand gespeichert." : "Speichern fehlgeschlagen!"); // Rückmeldung
} // Ende von saveGame

bool Game::saveExists() const { return ImageIO::fileExists(m_savePath); }    // Gibt es eine Spielstand-Datei?

// Öffnet ein Popup der gewünschten Art
void Game::openPopup(PopupKind kind) {                                       // Beginn von openPopup
    m_popupKind = kind;                                                      // Art merken
    switch (kind) {                                                          // Je nach Art
    case PopupKind::QuitConfirm:                                             // Beenden bestätigen
        m_popup.open("Spiel beenden?", "Möchtest du das Spiel wirklich beenden? Dein Spielstand wird gespeichert.", {"Ja", "Nein"}, m_fbW, m_fbH, m_ui); // Popup öffnen
        m_popup.moveSelection(1);                                            // "Nein" vorauswählen (Schutz vor Versehen)
        break;                                                               // Ende
    case PopupKind::LevelComplete:                                           // Ziel erreicht
        m_popup.open("Level-Ende erreicht!", "Du hast das Ende der " + m_level.name + " erreicht. Möchtest du von vorne beginnen?", {"Ja", "Nein"}, m_fbW, m_fbH, m_ui); // Popup öffnen
        break;                                                               // Ende
    case PopupKind::GameOver:                                                // Kein Leben mehr
        m_popup.open("Keine Lebensenergie mehr!", "Möchtest du von vorne beginnen?", {"Ja", "Nein"}, m_fbW, m_fbH, m_ui); // Popup öffnen
        break;                                                               // Ende
    default:                                                                 // Kein Popup
        m_popup.close();                                                     // Schließen
        break;                                                               // Ende
    }                                                                        // Ende der Fallunterscheidung
} // Ende von openPopup

// Führt die Auswahl im Popup aus (0 = "Ja", 1 = "Nein")
void Game::popupChoice(int index) {                                          // Beginn von popupChoice
    PopupKind kind = m_popupKind;                                            // Art merken
    m_popup.close();                                                         // Popup schließen
    m_popupKind = PopupKind::None;                                           // Kein Popup mehr
    if (kind == PopupKind::QuitConfirm) {                                    // Beenden-Frage
        if (index == 0) {                                                    // "Ja"
            m_shop.close();                                                  // Shop schließen
            saveGame(false);                                                 // Spielstand sichern
            m_screen = Screen::MainMenu;                                     // Zurück ins Hauptmenü
            m_menuSelection = 0;                                             // Erster Knopf gewählt
        }                                                                    // Bei "Nein" geht das Spiel weiter
    } else if (kind == PopupKind::LevelComplete || kind == PopupKind::GameOver) { // Ziel oder Game Over
        if (index == 0) startNewGame();                                      // "Ja": von vorne beginnen
        else m_screen = Screen::MainMenu;                                    // "Nein": zurück ins Hauptmenü
    }                                                                        // Ende der Unterscheidung
} // Ende von popupChoice

// Führt einen Knopf des Hauptmenüs aus
void Game::menuActivate(int index) {                                         // Beginn von menuActivate
    if (index == 0) startNewGame();                                          // "Neues Spiel"
    else if (index == 1 && saveExists()) continueGame();                     // "Letzter Spielstand"
    else if (index == 2) m_running = false;                                  // "Beenden"
} // Ende von menuActivate

// Taste E: mit Shop oder Ampel interagieren
void Game::interact() {                                                      // Beginn von interact
    if (m_player.state != PlayerState::Normal) return;                       // Nur im normalen Zustand
    for (std::size_t i = 0; i < m_level.shops.size(); ++i) {                 // Alle Shops
        if (std::fabs(m_player.x - m_level.shops[i].doorX()) < 0.6f && m_player.onGround) { // Vor der Tür?
            openShop(static_cast<int>(i));                                   // Shop öffnen
            return;                                                          // Fertig
        }                                                                    // Ende der Prüfung
    }                                                                        // Ende der Schleife
    for (TrafficLight& light : m_level.lights) {                             // Alle Ampeln
        if (std::fabs(m_player.x - light.poleX()) < 0.8f) {                  // Am Ampelmast?
            if (light.green) m_hud.showMessage("Die Ampel ist schon grün - los geht's!"); // Schon grün
            else if (light.buttonPressed) m_hud.showMessage("Geduld - das Signal kommt gleich."); // Schon gedrückt
            else {                                                           // Taster drücken
                light.pressButton();                                         // Rotphase verkürzen
                m_hud.showMessage("Taster gedrückt - gleich wird es grün."); // Rückmeldung
            }                                                                // Ende der Unterscheidung
            return;                                                          // Fertig
        }                                                                    // Ende der Prüfung
    }                                                                        // Ende der Schleife
} // Ende von interact

// Öffnet das Shop-Menü (die Karte bleibt, das Spiel pausiert)
void Game::openShop(int index) {                                             // Beginn von openShop
    m_shop.open(m_level.shops[static_cast<std::size_t>(index)]);             // Shop mit seinen Waren öffnen
    m_player.vx = 0.0f;                                                      // Figur bleibt stehen
} // Ende von openShop

// Schließt den Shop und speichert automatisch
void Game::closeShop() {                                                     // Beginn von closeShop
    m_shop.close();                                                          // Fenster schließen
    saveGame(true);                                                          // Automatisch speichern
} // Ende von closeShop

// Kauft einen Gegenstand im Shop
void Game::buyItem(const std::string& id) {                                  // Beginn von buyItem
    const ItemDef* def = m_items.find(id);                                   // Gegenstand nachschlagen
    if (!def) return;                                                        // Unbekannt -> nichts tun
    if (def->unique && m_inventory.has(def->id)) { m_hud.showMessage("Du besitzt bereits: " + def->name); return; } // Schon vorhanden
    if (m_player.coins < def->price) { m_hud.showMessage("Nicht genug Coins für " + def->name + "!"); return; } // Zu teuer
    if (!m_inventory.add(*def)) { m_hud.showMessage("Dein Inventar ist voll!"); return; } // Kein Platz
    m_player.coins -= def->price;                                            // Bezahlen
    if (def->effect == ItemEffect::Hook) m_hud.showMessage("Enterhaken gekauft! Mit Q an Ankerpunkten benutzen.", 3.5f); // Hinweis zum Enterhaken
    else m_hud.showMessage(def->name + " gekauft!");                         // Allgemeine Rückmeldung
} // Ende von buyItem

// Benutzt den Gegenstand in einem Inventarfeld
void Game::useSlot(int index) {                                              // Beginn von useSlot
    if (m_screen != Screen::Playing || index < 0 || index >= Inventory::SIZE) return; // Ungültig
    const InventorySlot& slot = m_inventory.slot(index);                     // Feld
    if (slot.empty()) return;                                                // Leer -> nichts
    const ItemDef* def = m_items.find(slot.itemId);                          // Gegenstand nachschlagen
    if (!def) return;                                                        // Unbekannt -> nichts
    switch (def->effect) {                                                   // Je nach Wirkung
    case ItemEffect::Stamina:                                                // Ausdauer auffüllen
        m_player.stamina = std::min(maxStamina(), m_player.stamina + def->value); // Ausdauer erhöhen
        m_effects.floatingText(m_player.x, m_player.y - 1.0f, "+" + std::to_string(static_cast<int>(def->value)) + " Ausdauer", rgba(250, 210, 60)); // Anzeige
        if (def->consumable) m_inventory.removeOne(index);                   // Verbrauchen
        m_hud.showMessage(def->name + " getrunken - Ausdauer aufgefüllt!");  // Rückmeldung
        break;                                                               // Ende
    case ItemEffect::Health:                                                 // Leben auffüllen
        m_player.health = std::min(m_player.stats().maxHealth, m_player.health + def->value); // Leben erhöhen
        if (def->consumable) m_inventory.removeOne(index);                   // Verbrauchen
        m_hud.showMessage(def->name + " benutzt - Leben aufgefüllt!");       // Rückmeldung
        break;                                                               // Ende
    case ItemEffect::Hook:                                                   // Enterhaken
        m_hud.showMessage("Enterhaken: Mit Q werfen, wenn ein Ankerpunkt in Reichweite ist."); // Hinweis
        break;                                                               // Ende
    default:                                                                 // Passive Gegenstände
        m_hud.showMessage(def->name + " wirkt automatisch.");                // Hinweis
        break;                                                               // Ende
    }                                                                        // Ende der Fallunterscheidung
} // Ende von useSlot

float Game::maxStamina() const { return m_player.stats().maxStamina + m_inventory.sumEffect(ItemEffect::MaxStamina, m_items); } // Grundwert + Bonus
bool Game::hasHook() const { return m_inventory.hasEffect(ItemEffect::Hook, m_items); } // Enterhaken im Inventar?

// Speichert den aktuellen Bildspeicher als BMP im Ordner screenshots/
void Game::takeScreenshot() {                                                // Beginn von takeScreenshot
    std::string dir = ImageIO::joinPath(m_baseDir, "screenshots");           // Zielordner
    ImageIO::makeDirectories(dir);                                           // Ordner anlegen
    std::time_t now = std::time(nullptr);                                    // Aktuelle Zeit
    std::tm* local = std::localtime(&now);                                   // Lokale Zeit
    char name[64];                                                           // Puffer für den Dateinamen
    std::snprintf(name, sizeof(name), "bild_%04d%02d%02d_%02d%02d%02d_%03d.bmp", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, static_cast<int>(SDL_GetTicks() % 1000)); // Dateiname mit Zeitstempel
    std::string path = ImageIO::joinPath(dir, name);                         // Vollständiger Pfad
    bool ok = ImageIO::saveBMP(path, m_frame);                               // Speichern
    std::cout << (ok ? "Bildschirmfoto gespeichert: " : "Bildschirmfoto fehlgeschlagen: ") << path << "\n"; // Konsolenhinweis
    if (m_screen == Screen::Playing) m_hud.showMessage(ok ? "Bildschirmfoto gespeichert." : "Bildschirmfoto fehlgeschlagen."); // Rückmeldung im Spiel
} // Ende von takeScreenshot
