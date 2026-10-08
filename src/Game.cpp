// Game.cpp - Start, Hauptschleife, Eingaben, Menüablauf, Speichern und Punkteliste
#include "Game.h" // Eigene Deklarationen

#include <algorithm> // std::sort, std::min, std::max
#include <cmath>     // std::floor
#include <ctime>     // std::time für den Zufallsstart
#include <iostream>  // Fehlerausgabe

#include "Font.h"    // Text im Ladebildschirm
#include "ImageIO.h" // Pfade und Dateien

namespace { // Interne Hilfen

// Sucht den Projektordner mit data/spiel.txt
std::string findRoot() {                                                     // Beginn von findRoot
    std::vector<std::string> candidates;                                     // Mögliche Ordner
    if (char* base = SDL_GetBasePath()) {                                    // Ordner der ausführbaren Datei
        std::string b = base;                                                // Als Text
        SDL_free(base);                                                      // Speicher von SDL freigeben
        candidates.push_back(b);                                             // Direkt dort
        candidates.push_back(ImageIO::joinPath(b, ".."));                    // Eine Ebene höher (z.B. build/)
    }                                                                        // Ende Basisordner
    candidates.push_back(".");                                               // Aktueller Ordner
    candidates.push_back("..");                                              // Übergeordneter Ordner
#ifdef MSD_SOURCE_DIR                                                        // Quellordner aus CMake
    candidates.push_back(MSD_SOURCE_DIR);                                    // Quellordner
#endif                                                                       // Ende Quellordner
    for (const std::string& c : candidates) if (ImageIO::fileExists(ImageIO::joinPath(c, "data/spiel.txt"))) return c; // Ersten Treffer nehmen
    return ".";                                                              // Ersatz
} // Ende von findRoot

} // Ende des internen Namensraums

// Zufallszahl 0..1 für Spielereignisse
float Game::randf() {                                                        // Beginn von randf
    m_rand = m_rand * 1664525u + 1013904223u;                                // Lineare Kongruenz
    return static_cast<float>((m_rand >> 8) & 0xFFFFFF) / static_cast<float>(0x1000000); // Auf 0..1 abbilden
} // Ende von randf

// Lädt alles und öffnet das Fenster
bool Game::init(std::string& error) {                                        // Beginn von init
    m_root = findRoot();                                                     // Projektordner
    std::string dataDir = ImageIO::joinPath(m_root, "data");                 // Datenordner
    std::string assetDir = ImageIO::joinPath(m_root, "assets");              // Asset-Ordner
    if (!m_config.load(ImageIO::joinPath(dataDir, "spiel.txt"))) { error = "data/spiel.txt nicht gefunden"; return false; } // Einstellungen
    m_set.load(m_config);                                                    // Spielwerte übernehmen
    m_w = clampValue(m_config.getInt("Fenster", "breite", 1280), 640, 3840);  // Breite
    m_h = clampValue(m_config.getInt("Fenster", "hoehe", 720), 400, 2160);   // Höhe
    m_maxFps = clampValue(m_config.getInt("Fenster", "max_fps", 60), 10, 240); // Bildrate
    const char* keyDefaults[][2] = {{"hoch", "W"}, {"runter", "S"}, {"links", "A"}, {"rechts", "D"}, {"aktion", "E"}, {"seekarte", "M"}, {"schiffsinfo", "I"}, {"kanone", "Space"}, {"autopilot", "T"}, {"maschine", "F"}, {"zeitung", "Z"}, {"hilfe", "F1"}, {"pause", "Escape"}, {"drehen", "R"}}; // Standardtasten
    for (const auto& kd : keyDefaults) {                                     // Alle Aktionen
        SDL_Scancode sc = SDL_GetScancodeFromName(m_config.getString("Steuerung", kd[0], kd[1]).c_str()); // Taste aus der Datei
        if (sc == SDL_SCANCODE_UNKNOWN) sc = SDL_GetScancodeFromName(kd[1]); // Ungültig -> Standard
        m_keys[kd[0]] = sc;                                                  // Speichern
    }                                                                        // Ende der Tasten
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) { error = std::string("SDL_Init: ") + SDL_GetError(); return false; } // SDL starten
    Uint32 flags = SDL_WINDOW_SHOWN;                                         // Fenster sichtbar
    if (m_config.getBool("Fenster", "vollbild", false)) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; // Vollbild
    m_window = SDL_CreateWindow(m_config.getString("Fenster", "titel", "Mini Ship Delivery").c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_w, m_h, flags); // Fenster öffnen
    if (!m_window) { error = std::string("SDL_CreateWindow: ") + SDL_GetError(); return false; } // Fehler
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);    // Software-Renderer (kein Grafikbeschleuniger)
    if (!m_renderer) { error = std::string("SDL_CreateRenderer: ") + SDL_GetError(); return false; } // Fehler
    SDL_RenderSetLogicalSize(m_renderer, m_w, m_h);                          // Bild bei Vollbild passend skalieren
    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_w, m_h); // Textur für das fertige Bild
    if (!m_texture) { error = std::string("SDL_CreateTexture: ") + SDL_GetError(); return false; } // Fehler
    m_frame.resize(m_w, m_h, rgba(0, 0, 0));                                 // Bildspeicher anlegen
    loadingScreen("Lade Spieldaten ...", 0.05f);                             // Erster Ladebildschirm
    if (!m_data.load(dataDir, error)) return false;                          // Waren, Schiffe, Crew ...
    if (!m_world.load(ImageIO::joinPath(dataDir, "welt.txt"), error)) return false; // Inselwelt erzeugen
    loadingScreen("Lade 3D-Modelle ...", 0.3f);                              // Fortschritt
    m_lib.load(ImageIO::joinPath(dataDir, "modelle.txt"), ImageIO::joinPath(dataDir, "figuren.txt"), assetDir); // Modelle
    m_lib.setSupersample(clampValue(m_config.getInt("Grafik", "kantenglaettung", 2), 1, 3)); // Kantenglättung
    m_shipSteps = clampValue(m_config.getInt("Grafik", "schiff_drehstufen", 32), 8, 64); // Drehstufen der Schiffe
    if (!m_ui.load(ImageIO::joinPath(assetDir, m_config.getString("Grafik", "oberflaeche", "Interface Pack/PNG/Retina")))) std::cout << "Hinweis: Nicht alle Oberflächengrafiken gefunden.\n"; // Oberfläche
    if (!m_audio.init(ImageIO::joinPath(dataDir, "audio.txt"), assetDir)) std::cout << "Ton: " << m_audio.status() << "\n"; // Ton (optional)
    else std::cout << m_audio.status() << "\n";                              // Statusmeldung
    for (const std::string& z : m_config.getList("Grafik", "zoomstufen")) m_zoomLevels.push_back(clampValue(std::strtof(z.c_str(), nullptr), 24.0f, 256.0f)); // Zoomstufen
    if (m_zoomLevels.empty()) m_zoomLevels = {64.0f, 96.0f, 128.0f};         // Standard
    std::sort(m_zoomLevels.begin(), m_zoomLevels.end());                     // Aufsteigend
    float startTile = m_config.getFloat("Grafik", "kachel_groesse", 96.0f);  // Startzoom
    m_zoomIndex = 0;                                                         // Nächste Stufe suchen
    for (std::size_t i = 0; i < m_zoomLevels.size(); ++i) if (std::fabs(m_zoomLevels[i] - startTile) < std::fabs(m_zoomLevels[static_cast<std::size_t>(m_zoomIndex)] - startTile)) m_zoomIndex = static_cast<int>(i); // Nächste Stufe
    m_cam.screenW = m_w; m_cam.screenH = m_h; m_cam.tileWidth = m_zoomLevels[static_cast<std::size_t>(m_zoomIndex)]; // Kamera einrichten
    m_state.attach(&m_data, &m_world, &m_set);                               // Spielstand mit den Daten verbinden
    buildChart();                                                            // Seekarte vorbereiten
    loadScores();                                                            // Punkteliste
    m_hasSave = ImageIO::fileExists(ImageIO::joinPath(m_root, "speicher/spielstand.txt")); // Spielstand vorhanden?
    int start = std::max(0, m_world.islandIndex(m_world.startIsland));       // Startinsel für den Menühintergrund
    m_cam.x = m_world.islands[static_cast<std::size_t>(start)].plazaX;       // Kamera x
    m_cam.y = m_world.islands[static_cast<std::size_t>(start)].plazaY;       // Kamera y
    m_rand = static_cast<unsigned>(std::time(nullptr));                      // Zufall starten
    loadingScreen("Bereite Menü vor ...", 0.8f);                             // Fortschritt
    prewarm("Bereite Menü vor ...");                                         // Sprites der Startinsel vorbereiten
    updateMusic();                                                           // Menümusik
    return true;                                                             // Erfolgreich
} // Ende von init

// Gibt alles frei
void Game::shutdown() {                                                      // Beginn von shutdown
    if (m_texture) SDL_DestroyTexture(m_texture);                            // Textur
    if (m_renderer) SDL_DestroyRenderer(m_renderer);                         // Renderer
    if (m_window) SDL_DestroyWindow(m_window);                               // Fenster
    m_texture = nullptr; m_renderer = nullptr; m_window = nullptr;           // Zeiger zurücksetzen
    SDL_Quit();                                                              // SDL beenden
} // Ende von shutdown

// Hauptschleife
void Game::run() {                                                           // Beginn von run
    Uint32 last = SDL_GetTicks();                                            // Zeit des letzten Bildes
    while (m_running) {                                                      // Bis zum Beenden
        Uint32 frameStart = SDL_GetTicks();                                  // Beginn dieses Bildes
        m_input.clicked = false; m_input.rightClicked = false; m_input.wheel = 0; m_input.consumed = false; // Eingaben zurücksetzen
        SDL_Event e;                                                         // Ereignis
        while (SDL_PollEvent(&e)) handleEvent(e);                            // Alle Ereignisse verarbeiten
        Uint32 now = SDL_GetTicks();                                         // Aktuelle Zeit
        float dt = std::min(0.05f, static_cast<float>(now - last) / 1000.0f); // Vergangene Zeit (begrenzt)
        last = now;                                                          // Merken
        update(dt);                                                          // Spiel fortschreiben
        render();                                                            // Bild zeichnen
        present();                                                           // Anzeigen
        Uint32 used = SDL_GetTicks() - frameStart;                           // Benötigte Zeit
        Uint32 target = 1000u / static_cast<Uint32>(m_maxFps);               // Zeit je Bild
        if (used < target) SDL_Delay(target - used);                         // Rest abwarten (schont die CPU)
    }                                                                        // Ende der Schleife
} // Ende von run

// Verarbeitet ein SDL-Ereignis
void Game::handleEvent(const SDL_Event& e) {                                 // Beginn von handleEvent
    switch (e.type) {                                                        // Je nach Art
    case SDL_QUIT:                                                           // Fenster geschlossen
        if (m_screen == Screen::Playing || m_screen == Screen::LoadShip) { saveGame(); recordScore(); } // Spiel sichern
        m_running = false;                                                   // Beenden
        break;                                                               // Fertig
    case SDL_MOUSEMOTION:                                                    // Maus bewegt
        m_input.mouseX = e.motion.x; m_input.mouseY = e.motion.y;            // Position merken
        break;                                                               // Fertig
    case SDL_MOUSEBUTTONDOWN:                                                // Maustaste gedrückt
        m_input.mouseX = e.button.x; m_input.mouseY = e.button.y;            // Position merken
        if (e.button.button == SDL_BUTTON_LEFT) m_input.clicked = true;      // Linke Taste
        if (e.button.button == SDL_BUTTON_RIGHT) m_input.rightClicked = true; // Rechte Taste
        if (m_ui.mouseOverUi(e.button.x, e.button.y)) sound("klick", 60);    // Klickgeräusch auf der Oberfläche
        break;                                                               // Fertig
    case SDL_MOUSEWHEEL:                                                     // Mausrad
        m_input.wheel += e.wheel.y;                                          // Drehung merken
        break;                                                               // Fertig
    case SDL_KEYDOWN:                                                        // Taste gedrückt
        if (e.key.repeat) break;                                             // Wiederholungen ignorieren
        if (m_screen == Screen::Playing && m_panel == Panel::None) {         // Zoom nur im freien Spiel
            SDL_Keycode k = e.key.keysym.sym;                                // Tastencode
            if (k == SDLK_PLUS || k == SDLK_KP_PLUS || k == SDLK_EQUALS) m_input.wheel += 1; // Hineinzoomen
            if (k == SDLK_MINUS || k == SDLK_KP_MINUS) m_input.wheel -= 1;   // Herauszoomen
        }                                                                    // Ende Zoom
        handleKey(e.key.keysym.scancode);                                    // Taste auswerten
        break;                                                               // Fertig
    default: break;                                                          // Andere Ereignisse ignorieren
    }                                                                        // Ende der Fallunterscheidung
} // Ende von handleEvent

// Wertet einen Tastendruck aus
void Game::handleKey(SDL_Scancode sc) {                                      // Beginn von handleKey
    if (m_screen == Screen::Scores) { if (sc == keyOf("pause")) m_screen = Screen::Menu; return; } // Punkteliste schließen
    if (m_screen == Screen::Menu) return;                                    // Im Menü nur Maus
    if (m_screen == Screen::LoadShip) {                                      // Minispiel
        if (sc == keyOf("drehen")) m_lsRot = (m_lsRot + 1) % 4;              // Gehaltene Einheit drehen
        if (sc == keyOf("pause") && m_lsCapsize < 0.0f) {                    // Minispiel verlassen
            if (m_lsHolding) { m_lsCargo.x = m_lsCargo.y = -1; m_state.pier(m_state.docked).push_back(m_lsCargo); m_lsHolding = false; } // Gehaltenes zurück an den Steg
            m_screen = Screen::Playing;                                      // Zurück ins Spiel
            openPanel(Panel::Pier);                                          // Stegfenster
        }                                                                    // Ende Verlassen
        return;                                                              // Fertig
    }                                                                        // Ende Minispiel
    if (sc == keyOf("pause")) {                                              // Pause / Fenster schließen
        if (m_panel == Panel::Sunk) return;                                  // Muss bestätigt werden
        m_panel = m_panel == Panel::None ? Panel::Pause : Panel::None;       // Umschalten
        return;                                                              // Fertig
    }                                                                        // Ende Pause
    if (sc == keyOf("seekarte")) { m_panel = m_panel == Panel::Chart ? Panel::None : (m_panel == Panel::None ? Panel::Chart : m_panel); return; } // Seekarte
    if (sc == keyOf("schiffsinfo")) { if (m_panel == Panel::ShipInfo) m_panel = Panel::None; else if (m_panel == Panel::None) openPanel(Panel::ShipInfo); return; } // Schiffsinfo
    if (sc == keyOf("zeitung")) { if (m_panel == Panel::Newspaper) m_panel = Panel::None; else if (m_panel == Panel::None) openPanel(Panel::Newspaper); return; } // Zeitung
    if (sc == keyOf("hilfe")) { if (m_panel == Panel::Help) m_panel = Panel::None; else if (m_panel == Panel::None) openPanel(Panel::Help); return; } // Hilfe
    if (m_panel != Panel::None) return;                                      // Andere Tasten nur ohne Fenster
    if (sc == keyOf("aktion")) { interact(); return; }                       // Aktion
    if (m_state.onFoot) return;                                              // Die folgenden Tasten nur auf dem Schiff
    if (m_dockAnim > 0.0f) return;                                           // Während des Anlegens keine Steuerung
    if (sc == keyOf("hoch")) m_throttle = std::min(3, m_throttle + 1);       // Mehr Fahrt
    if (sc == keyOf("runter")) m_throttle = std::max(-1, m_throttle - 1);    // Weniger Fahrt / rückwärts
    if (sc == keyOf("autopilot")) {                                          // Autopilot
        if (!m_state.hasTarget) notify("Kein Ziel gesetzt. Öffne die Seekarte (" + keyName("seekarte") + ") und klicke auf ein Ziel."); // Ohne Ziel nicht möglich
        else if (m_autopilot) { m_autopilot = false; notify("Autopilot aus"); } // Ausschalten
        else if (planRoute()) { m_autopilot = true; if (m_throttle <= 0) m_throttle = 2; notify("Autopilot: Kurs auf " + m_state.targetName); } // Einschalten mit Seeweg
    }                                                                        // Ende Autopilot
    if (sc == keyOf("kanone")) fireCannons();                                // Kanonen
    if (sc == keyOf("maschine")) {                                           // Hilfsmaschine
        if (m_state.shipClass().drive == "hybrid") { m_engineOn = !m_engineOn; notify(m_engineOn ? "Hilfsmaschine an" : "Hilfsmaschine aus"); } // Nur beim Hybrid schaltbar
        else if (m_state.shipClass().drive == "motor") notify("Das Dampfschiff fährt immer mit Maschine."); // Hinweis
        else notify("Dieses Schiff hat keine Maschine.");                    // Hinweis
    }                                                                        // Ende Maschine
} // Ende von handleKey

// Schreibt das Spiel fort
void Game::update(float dt) {                                                // Beginn von update
    m_time += dt;                                                            // Laufzeit
    for (Notice& n : m_notices) n.time -= dt;                                // Meldungen altern
    m_notices.erase(std::remove_if(m_notices.begin(), m_notices.end(), [](const Notice& n) { return n.time <= 0.0f; }), m_notices.end()); // Abgelaufene entfernen
    if (m_screen == Screen::Menu || m_screen == Screen::Scores) {            // Menü
        m_menuTime += dt;                                                    // Laufzeit
        int start = std::max(0, m_world.islandIndex(m_world.startIsland));   // Startinsel
        const Island& isl = m_world.islands[static_cast<std::size_t>(start)]; // Insel
        m_cam.x = isl.plazaX + 4.0f + std::sin(m_menuTime * 0.05f) * 6.0f;   // Kamera schwebt langsam
        m_cam.y = isl.plazaY + 2.0f + std::cos(m_menuTime * 0.05f) * 6.0f;   // Kamera y
    } else if (m_screen == Screen::Playing) {                                // Im Spiel
        if (!panelBlocksTime()) updatePlaying(dt);                           // Welt läuft
        else updateParticles(dt);                                            // Nur Effekte laufen weiter
        for (const std::string& m : m_state.messages) notify(m);             // Neue Meldungen des Spielstands anzeigen
        m_state.messages.clear();                                            // Abgeholt
    } else if (m_screen == Screen::LoadShip) {                               // Minispiel
        updateLoadShip(dt);                                                  // Minispiel fortschreiben
        for (const std::string& m : m_state.messages) notify(m);             // Meldungen
        m_state.messages.clear();                                            // Abgeholt
    }                                                                        // Ende der Bildschirme
    updateMusic();                                                           // Musik anpassen
} // Ende von update

// Zeichnet das aktuelle Bild
void Game::render() {                                                        // Beginn von render
    Canvas c(m_frame);                                                       // Zeichenfläche
    m_ui.beginFrame(c, m_input);                                             // Oberfläche beginnen
    if (m_screen == Screen::Menu || m_screen == Screen::Scores) {            // Menü
        renderWorld(c);                                                      // Welt als Hintergrund
        c.fillRect(0, 0, m_w, m_h, rgba(10, 30, 60, 90));                    // Leicht abdunkeln
        if (m_screen == Screen::Menu) renderMenu(); else renderScores();     // Menü oder Punkteliste
    } else if (m_screen == Screen::Playing) {                                // Im Spiel
        renderWorld(c);                                                      // Welt
        renderLighting(c);                                                   // Tageszeit
        renderWeather(c);                                                    // Wetter
        renderHud();                                                         // HUD
        renderPanel();                                                       // Fenster
        if (m_panel == Panel::None && m_input.clicked && !m_input.consumed && !m_ui.mouseOverUi(m_input.mouseX, m_input.mouseY)) { // Klick in die Welt
            float wx = 0.0f, wy = 0.0f;                                      // Weltposition
            m_cam.toWorld(static_cast<float>(m_input.mouseX), static_cast<float>(m_input.mouseY) + (m_state.onFoot ? LAND_HEIGHT * m_cam.tileWidth * 0.61f : 0.0f), wx, wy); // Bildschirm -> Boden
            m_input.consumed = true;                                         // Klick verbraucht
            clickWorld(wx, wy, m_input.mouseX, m_input.mouseY);              // Auswerten
        }                                                                    // Ende Klick
        if (m_panel == Panel::None && m_input.wheel != 0) {                  // Zoom mit dem Mausrad
            m_zoomIndex = clampValue(m_zoomIndex + (m_input.wheel > 0 ? 1 : -1), 0, static_cast<int>(m_zoomLevels.size()) - 1); // Neue Stufe
            m_cam.tileWidth = m_zoomLevels[static_cast<std::size_t>(m_zoomIndex)]; // Übernehmen
        }                                                                    // Ende Zoom
    } else if (m_screen == Screen::LoadShip) {                               // Minispiel
        renderLoadShip();                                                    // Minispiel zeichnen
    }                                                                        // Ende der Bildschirme
    m_ui.endFrame();                                                         // Tooltip zeichnen
} // Ende von render

// Bringt den Bildspeicher auf den Bildschirm
void Game::present() {                                                       // Beginn von present
    SDL_UpdateTexture(m_texture, nullptr, m_frame.pixels.data(), m_w * static_cast<int>(sizeof(Color))); // Pixel in die Textur kopieren
    SDL_RenderClear(m_renderer);                                             // Hintergrund löschen
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);                 // Textur zeichnen
    SDL_RenderPresent(m_renderer);                                           // Anzeigen
} // Ende von present

// Zeichnet einen einfachen Ladebildschirm und zeigt ihn sofort an
void Game::loadingScreen(const std::string& title, float progress) {         // Beginn von loadingScreen
    if (!m_renderer) return;                                                 // Noch kein Fenster
    Canvas c(m_frame);                                                       // Zeichenfläche
    c.clear(rgba(18, 52, 92));                                               // Meeresblau
    Font::drawTextShadow(c, m_w / 2 - Font::textWidth("Mini Ship Delivery", 5) / 2, m_h / 2 - 110, "Mini Ship Delivery", rgba(255, 220, 120), rgba(0, 0, 0, 160), 5); // Titel
    Font::drawText(c, m_w / 2 - Font::textWidth(title, 2) / 2, m_h / 2 - 10, title, rgba(230, 240, 250), 2); // Text
    c.fillRect(m_w / 2 - 250, m_h / 2 + 30, 500, 22, rgba(10, 25, 45));      // Balkenhintergrund
    c.fillRect(m_w / 2 - 247, m_h / 2 + 33, static_cast<int>(494.0f * clampValue(progress, 0.0f, 1.0f)), 16, rgba(240, 190, 80)); // Fortschritt
    present();                                                               // Sofort anzeigen
    SDL_PumpEvents();                                                        // Fenster reagiert weiter
} // Ende von loadingScreen

// Rendert Sprites vorab, damit das Spiel später nicht stockt
void Game::prewarm(const std::string& title) {                               // Beginn von prewarm
    float tw = m_cam.tileWidth;                                              // Aktueller Maßstab
    std::vector<const WorldObject*> near;                                    // Objekte in der Nähe der Kamera
    for (const WorldObject& o : m_world.objects) if (std::fabs(o.x - m_cam.x) < 16.0f && std::fabs(o.y - m_cam.y) < 16.0f) near.push_back(&o); // Umkreis
    int total = static_cast<int>(near.size()) + (m_screen == Screen::Menu ? 0 : 8 * 16 + m_shipSteps); // Anzahl der Arbeitsschritte
    int done = 0;                                                            // Erledigt
    for (const WorldObject* o : near) {                                      // Objekte
        m_lib.sprite(o->model, o->angle, o->steps, tw);                      // Rendern
        if (++done % 10 == 0) loadingScreen(title, static_cast<float>(done) / static_cast<float>(std::max(1, total))); // Fortschritt
    }                                                                        // Ende Objekte
    if (m_screen == Screen::Menu) return;                                    // Im Menü genügt die Insel
    for (const char* anim : {"stehen", "laufen"}) {                          // Animationen der Spielfigur
        float dur = m_lib.figureDuration("spieler", anim);                   // Länge
        for (int d = 0; d < 8; ++d) for (int f = 0; f < 8; ++f) {            // Richtungen und Bilder
            m_lib.figure("spieler", anim, d, (static_cast<float>(f) + 0.5f) / 8.0f * dur, tw); // Rendern
            if (++done % 16 == 0) loadingScreen(title, static_cast<float>(done) / static_cast<float>(std::max(1, total))); // Fortschritt
        }                                                                    // Ende Richtungen
    }                                                                        // Ende Animationen
    for (int r = 0; r < m_shipSteps; ++r) {                                  // Alle Drehungen des eigenen Schiffs
        m_lib.sprite(m_state.shipClass().model, static_cast<float>(r) * 2.0f * PI / static_cast<float>(m_shipSteps), m_shipSteps, tw); // Rendern
        if (++done % 8 == 0) loadingScreen(title, static_cast<float>(done) / static_cast<float>(std::max(1, total))); // Fortschritt
    }                                                                        // Ende Drehungen
} // Ende von prewarm

// Leert alle Dinge, die nicht gespeichert werden
void Game::resetTransient() {                                                // Beginn von resetTransient
    m_pirates.clear(); m_monsters.clear(); m_shots.clear(); m_particles.clear(); // Gegner und Effekte
    m_notices.clear(); m_log.clear(); m_path.clear(); m_zoneCooldown.clear(); // Meldungen und Weg
    m_pendingBuilding = -1; m_pendingPier = false; m_figMoving = false;      // Figur
    m_shipSpeed = 0.0f; m_throttle = 0; m_engineOn = false; m_autopilot = false; m_seaPath.clear(); // Schiff
    m_cannonReload = 0.0f; m_dockAnim = 0.0f; m_hitFlash = 0.0f;             // Zeitgeber
    m_panel = Panel::None; m_lsHolding = false; m_lsCapsize = -1.0f;          // Fenster und Minispiel
} // Ende von resetTransient

// Beginnt ein neues Spiel
void Game::startNewGame() {                                                  // Beginn von startNewGame
    m_state.newGame(static_cast<unsigned>(std::time(nullptr)) ^ SDL_GetTicks()); // Neuer Spielstand mit neuem Zufall
    resetTransient();                                                        // Effekte leeren
    m_screen = Screen::Playing;                                              // Spielbildschirm
    m_cam.x = m_state.figX; m_cam.y = m_state.figY;                          // Kamera auf die Figur
    prewarm("Die Segel werden gesetzt ...");                                 // Vorbereiten
    saveGame();                                                              // Ersten Spielstand anlegen
    openPanel(Panel::Help);                                                  // Steuerung erklären
} // Ende von startNewGame

// Lädt den Spielstand
bool Game::continueGame() {                                                  // Beginn von continueGame
    if (!m_state.load(ImageIO::joinPath(m_root, "speicher/spielstand.txt"))) { notify("Kein Spielstand gefunden."); return false; } // Laden
    resetTransient();                                                        // Effekte leeren
    m_screen = Screen::Playing;                                              // Spielbildschirm
    m_cam.x = m_state.onFoot ? m_state.figX : m_state.shipX;                 // Kamera x
    m_cam.y = m_state.onFoot ? m_state.figY : m_state.shipY;                 // Kamera y
    prewarm("Spielstand wird geladen ...");                                  // Vorbereiten
    notify("Willkommen zurück, Kapitän! " + m_state.clockText());            // Begrüßung
    return true;                                                             // Erfolgreich
} // Ende von continueGame

// Speichert den Spielstand
void Game::saveGame() {                                                      // Beginn von saveGame
    ImageIO::makeDirectories(ImageIO::joinPath(m_root, "speicher"));        // Ordner anlegen
    if (m_state.save(ImageIO::joinPath(m_root, "speicher/spielstand.txt"))) m_hasSave = true; // Speichern
    else notify("Speichern fehlgeschlagen!");                                // Fehler
} // Ende von saveGame

// Zurück ins Hauptmenü
void Game::leaveToMenu() {                                                   // Beginn von leaveToMenu
    saveGame();                                                              // Sichern
    recordScore();                                                           // Werten
    m_screen = Screen::Menu;                                                 // Menü
    m_panel = Panel::None;                                                   // Kein Fenster
    m_menuTime = 0.0f;                                                       // Menüzeit
} // Ende von leaveToMenu

// Liest die Punkteliste
void Game::loadScores() {                                                    // Beginn von loadScores
    m_scores.clear();                                                        // Leeren
    PropertyFile f;                                                          // Datei
    if (!f.load(ImageIO::joinPath(m_root, "speicher/score.txt"))) return;    // Noch keine Liste
    for (const std::string& s : f.sectionNames()) {                          // Alle Einträge
        ScoreEntry e;                                                        // Neuer Eintrag
        e.score = f.getInt(s, "punkte", 0); e.days = f.getInt(s, "tage", 0); // Punkte und Tage
        e.credits = f.getInt(s, "credits", 0); e.explorer = f.getInt(s, "entdecker", 0); // Geld und Entdecker
        e.ship = f.getString(s, "schiff", "");                               // Schiff
        m_scores.push_back(e);                                               // Speichern
    }                                                                        // Ende der Einträge
    std::sort(m_scores.begin(), m_scores.end(), [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; }); // Beste zuerst
} // Ende von loadScores

// Trägt das aktuelle Spiel in die Punkteliste ein (ein Eintrag je Spiel, der beste Stand zählt)
void Game::recordScore() {                                                   // Beginn von recordScore
    PropertyFile f;                                                          // Datei
    std::string path = ImageIO::joinPath(m_root, "speicher/score.txt");      // Pfad
    f.load(path);                                                            // Vorhandene Liste (falls es sie gibt)
    std::string sec = "Spiel_" + std::to_string(m_state.seed);               // Abschnitt dieses Spiels
    if (f.getInt(sec, "punkte", -1) > m_state.score()) return;               // Früher schon besser gewesen
    f.setInt(sec, "punkte", m_state.score());                                // Punkte
    f.setInt(sec, "tage", m_state.day());                                    // Tage
    f.setInt(sec, "credits", m_state.credits);                               // Geld
    f.setInt(sec, "entdecker", m_state.explorerPoints);                      // Entdeckerpunkte
    f.set(sec, "schiff", m_state.shipClass().name);                          // Schiff
    ImageIO::makeDirectories(ImageIO::joinPath(m_root, "speicher"));         // Ordner anlegen
    f.save(path, "Mini Ship Delivery - Punkteliste");                        // Schreiben
    loadScores();                                                            // Neu einlesen
} // Ende von recordScore

// Zeigt eine Meldung oben an und merkt sie für das Dialogfenster
void Game::notify(const std::string& text) {                                 // Beginn von notify
    if (text.empty()) return;                                                // Leere Meldung
    m_notices.push_back({text, 5.0f + static_cast<float>(text.size()) * 0.03f}); // Neue Meldung (lange Texte länger)
    if (m_notices.size() > 4) m_notices.erase(m_notices.begin());            // Höchstens vier
    m_log.push_back(text);                                                   // Für das Dialogfenster
    if (m_log.size() > 6) m_log.erase(m_log.begin());                        // Höchstens sechs
} // Ende von notify

void Game::sound(const std::string& effect, int volume) { m_audio.play(effect, volume); } // Geräusch abspielen

// Wählt Musik und Hintergrundgeräusch passend zur Lage
void Game::updateMusic() {                                                   // Beginn von updateMusic
    if (!m_audio.enabled()) return;                                          // Kein Ton
    if (m_screen == Screen::Menu || m_screen == Screen::Scores) { m_audio.music("menue"); m_audio.ambience("wellen"); return; } // Menü
    WeatherSlot w = m_state.weatherAt(m_state.hours);                        // Aktuelles Wetter
    if (m_state.onFoot) m_audio.music("insel");                              // An Land
    else m_audio.music(m_state.isNight() ? "meer_nacht" : "meer_tag");       // Auf See
    m_audio.ambience(w.type == WeatherType::Storm ? "sturm" : (w.type == WeatherType::Rain ? "regen" : "wellen")); // Wetterklang
} // Ende von updateMusic

// Wird die Taste einer Aktion gehalten?
bool Game::keyHeld(const std::string& action) const {                        // Beginn von keyHeld
    const Uint8* keys = SDL_GetKeyboardState(nullptr);                       // Tastaturzustand
    SDL_Scancode sc = keyOf(action);                                         // Taste
    return sc != SDL_SCANCODE_UNKNOWN && keys[sc];                           // Gedrückt?
} // Ende von keyHeld

// Taste einer Aktion
SDL_Scancode Game::keyOf(const std::string& action) const {                  // Beginn von keyOf
    auto it = m_keys.find(action);                                           // Suchen
    return it == m_keys.end() ? SDL_SCANCODE_UNKNOWN : it->second;           // Ergebnis
} // Ende von keyOf

std::string Game::keyName(const std::string& action) const { return SDL_GetScancodeName(keyOf(action)); } // Tastenname

std::string Game::goodName(const std::string& id) const { const GoodDef* g = m_data.good(id); return g ? g->name : id; } // Warenname

std::string Game::islandName(int island) const { return island >= 0 && island < static_cast<int>(m_world.islands.size()) ? m_world.islands[static_cast<std::size_t>(island)].name : std::string("?"); } // Inselname

// Hält das offene Fenster die Spielzeit an?
bool Game::panelBlocksTime() const {                                         // Beginn von panelBlocksTime
    if (m_panel == Panel::None) return false;                                // Kein Fenster
    if (m_panel == Panel::Pause || m_panel == Panel::Help || m_panel == Panel::Sunk) return true; // Diese halten immer an
    return !m_set.timeInWindows;                                             // Je nach Einstellung
} // Ende von panelBlocksTime

// Öffnet ein Fenster
void Game::openPanel(Panel p, int building) {                                // Beginn von openPanel
    m_panel = p;                                                             // Fenster
    m_panelBuilding = building;                                              // Gebäude
    m_tab = 0;                                                               // Erster Reiter
    m_scroll = 0;                                                            // Oben beginnen
    m_path.clear();                                                          // Figur bleibt stehen
    m_figMoving = false;                                                     // Keine Laufanimation
} // Ende von openPanel
