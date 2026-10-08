// GameHud.cpp - HUD, Hauptmenü, Punkteliste und alle Fenster (Kontor, Händler, Hersteller, Museum, Werft, Taverne, Steg, Seekarte ...)
#include "Game.h" // Eigene Deklarationen

#include <algorithm> // std::min, std::max, std::sort
#include <cmath>     // std::atan2, std::sqrt
#include <cstdio>    // std::snprintf

#include "Font.h"       // Textbreite und Zeilenumbruch
#include "Rasterizer.h" // Iso-Projektion für die Seekarte

namespace { // Interne Hilfen

// Ganzzahl mit Tausenderpunkten ("12.345")
std::string thousands(int v) {                                                 // Beginn von thousands
    std::string s = std::to_string(v < 0 ? -v : v);                            // Ziffern ohne Vorzeichen
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(static_cast<std::size_t>(i), "."); // Punkte einfügen
    return (v < 0 ? "-" : "") + s;                                             // Vorzeichen wieder anfügen
} // Ende von thousands

std::string cr(int v) { return thousands(v) + " Cr"; }                        // Geldbetrag als Text

// Kommazahl mit einer Nachkommastelle
std::string f1(float v) { char b[32]; std::snprintf(b, sizeof(b), "%.1f", static_cast<double>(v)); return b; } // Formatieren

// Kürzt einen Text auf eine Pixelbreite
std::string fit(const std::string& t, int width, int scale) {                  // Beginn von fit
    if (Font::textWidth(t, scale) <= width) return t;                          // Passt
    std::string s = t;                                                         // Kopie
    while (!s.empty() && Font::textWidth(s + "..", scale) > width) {           // Zu lang
        s.pop_back();                                                          // Letztes Byte entfernen
        while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80) s.pop_back(); // Halbe UTF-8-Zeichen entfernen
        if (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0xC0) s.pop_back(); // Startbyte eines Umlauts entfernen
    }                                                                          // Ende der Schleife
    return s + "..";                                                           // Mit Auslassung
} // Ende von fit

// Bildschirmrichtung eines Weltvektors (0 = rechts, im Uhrzeigersinn)
float screenAngle(float dx, float dy) { return std::atan2((dx + dy) * 0.5f, dx - dy); } // Iso-Projektion

// Anzeigename eines Schiffsattributs
std::string attributeName(const std::string& a) {                              // Beginn von attributeName
    if (a == "geschwindigkeit") return "Geschwindigkeit";                      // Tempo
    if (a == "wendigkeit") return "Wendigkeit";                                // Drehen
    if (a == "reparatur") return "Reparatur";                                  // Reparatur
    if (a == "panzerung") return "Panzerung";                                  // Panzerung
    if (a == "abwehr") return "Abwehr";                                        // Kanonen
    if (a == "sichtweite") return "Sichtweite";                                // Scanner
    if (a == "handel") return "Handel";                                        // Handel
    if (a == "verbrauch") return "Verbrauch";                                  // Treibstoff
    if (a == "motor") return "Maschinenleistung";                              // Maschine
    if (a == "verderb") return "Verderb";                                      // Haltbarkeit
    if (a == "laderaum_breite") return "Laderaum";                             // Laderaum
    if (a == "rumpf") return "Rumpf";                                          // Rumpf
    if (a == "treibstoff_max") return "Tank";                                  // Tank
    if (a == "stabilitaet") return "Stabilität";                               // Stabilität
    return a;                                                                  // Unbekannt
} // Ende von attributeName

// Bonusliste einer Rolle als Text ("Geschwindigkeit +10 %, ...")
std::string bonusText(const CrewRole& r) {                                     // Beginn von bonusText
    std::string t;                                                             // Ergebnis
    for (const auto& kv : r.bonus) { if (!t.empty()) t += ", "; t += attributeName(kv.first) + (kv.second >= 0 ? " +" : " ") + std::to_string(static_cast<int>(kv.second)) + " %"; } // Alle Boni
    return t.empty() ? "Packt mit an" : t;                                     // Ergebnis
} // Ende von bonusText

} // Ende des internen Namensraums

// ============================================================ HUD

// Zeichnet das HUD passend zum Modus (Schiff oder Figur)
void Game::renderHud() {                                                       // Beginn von renderHud
    hudClock();                                                                // Oben links
    hudHealth();                                                               // Oben rechts
    if (m_state.onFoot) hudDialog(); else hudWindDepth();                      // Unten links
    hudTarget();                                                               // Unten rechts
    hudNotices();                                                              // Meldungen
    Canvas& c = m_ui.canvas();                                                 // Zeichenfläche
    if (!m_prompt.empty() && m_panel == Panel::None) {                         // Hinweis zur Aktionstaste
        int w = Font::textWidth(m_prompt, 2) + 40;                             // Breite
        RectI r{m_w / 2 - w / 2, m_h - 74, w, 40};                             // Rechteck
        c.fillRect(r.x, r.y, r.w, r.h, rgba(30, 20, 10, 170));                 // Dunkler Hintergrund
        c.drawRect(r.x, r.y, r.w, r.h, rgba(240, 200, 120, 200), 2);           // Rahmen
        m_ui.textShadow(r.x + 20, r.y + 12, m_prompt, rgba(255, 235, 180), 2); // Text
    }                                                                          // Ende Hinweis
    if (m_state.onFoot && m_panel == Panel::None && !m_ui.mouseOverUi(m_input.mouseX, m_input.mouseY)) { // Gebäudename unter der Maus
        int b = pickBuilding(m_input.mouseX, m_input.mouseY);                  // Gebäude suchen
        if (b >= 0) {                                                          // Gefunden
            std::string name = buildingTitle(b);                               // Name
            const Building& bd = m_world.buildings[static_cast<std::size_t>(b)]; // Gebäude
            if (!m_state.isOpen(bd.type)) name += " (geschlossen)";            // Geschlossen
            int w = Font::textWidth(name, 2) + 20;                             // Breite
            c.fillRect(m_input.mouseX + 14, m_input.mouseY + 10, w, 30, rgba(30, 20, 10, 190)); // Hintergrund
            m_ui.textShadow(m_input.mouseX + 24, m_input.mouseY + 18, name, rgba(255, 235, 180), 2); // Name
        }                                                                      // Ende gefunden
    }                                                                          // Ende Gebäudename
    std::string keys = keyName("seekarte") + " Seekarte   " + keyName("schiffsinfo") + " Schiff   " + keyName("zeitung") + " Zeitung   " + keyName("hilfe") + " Hilfe   " + keyName("pause") + " Menü"; // Tastenhilfe
    m_ui.textShadow(m_w / 2 - Font::textWidth(keys, 1) / 2, m_h - 20, keys, rgba(230, 230, 230), 1); // Klein am unteren Rand
} // Ende von renderHud

// Oben links: Uhrzeit, Tag/Nacht, Wetter und nächster Termin
void Game::hudClock() {                                                        // Beginn von hudClock
    RectI r{12, 12, 400, 118};                                                 // Fenster
    m_ui.panel(r, PanelStyle::Paper);                                          // Hintergrund
    Canvas& c = m_ui.canvas();                                                 // Zeichenfläche
    float day = m_state.daylight();                                            // Helligkeit
    int ix = r.x + 40, iy = r.y + 40;                                          // Position des Symbols
    if (day > 0.0f) {                                                          // Sonne (auch in der Dämmerung)
        c.fillCircle(ix, iy, 15, mixColor(rgba(240, 120, 60), rgba(255, 210, 60), day)); // Sonnenscheibe
        for (int k = 0; k < 8; ++k) { float a = static_cast<float>(k) * PI * 0.25f + m_time * 0.2f; c.line(ix + static_cast<int>(std::cos(a) * 18), iy + static_cast<int>(std::sin(a) * 18), ix + static_cast<int>(std::cos(a) * 23), iy + static_cast<int>(std::sin(a) * 23), rgba(240, 170, 40), 2); } // Strahlen
    } else {                                                                   // Mond
        c.fillCircle(ix, iy, 17, rgba(40, 50, 90));                            // Nachthimmel
        c.fillCircle(ix - 2, iy + 1, 11, rgba(245, 240, 200));                 // Mond
        c.fillCircle(ix + 4, iy - 3, 10, rgba(40, 50, 90));                    // Schatten (Mondsichel)
        c.putPixel(ix + 9, iy + 8, rgba(255, 255, 255)); c.putPixel(ix + 11, iy - 9, rgba(255, 255, 255)); // Sterne
    }                                                                          // Ende Symbol
    m_ui.text(r.x + 72, r.y + 20, m_state.clockText(), UiColor::INK, 2);       // Tag und Uhrzeit
    WeatherSlot w = m_state.weatherAt(m_state.hours);                          // Wetter
    std::string phase = m_state.isNight() ? "Nacht" : (day < 1.0f ? "Dämmerung" : "Tag"); // Tageszeit
    m_ui.text(r.x + 72, r.y + 44, phase + " - " + GameState::weatherName(w.type), UiColor::INK_LIGHT, 2); // Tageszeit und Wetter
    const Contract* t = m_state.nextDeadline();                                // Nächster Termin
    if (t) {                                                                   // Termin vorhanden
        float left = t->deadline - m_state.hours;                              // Verbleibende Stunden
        Color col = left < 4.0f ? UiColor::RED : UiColor::INK;                 // Knapp = rot
        m_ui.text(r.x + 20, r.y + 70, fit("Termin: " + std::to_string(t->amount) + " " + goodName(t->good) + " > " + islandName(t->to), r.w - 40, 2), col, 2); // Was und wohin
        m_ui.text(r.x + 20, r.y + 92, "bis " + GameState::timeText(t->deadline) + " (" + std::to_string(static_cast<int>(left)) + " Std.)", col, 1); // Frist
    } else {                                                                   // Kein Termin
        m_ui.text(r.x + 20, r.y + 74, "Keine Termine (Kontor)", UiColor::INK_LIGHT, 2); // Hinweis
    }                                                                          // Ende Termin
} // Ende von hudClock

// Oben rechts: Gesundheit (Figur) bzw. Rumpf (Schiff) und Geld
void Game::hudHealth() {                                                       // Beginn von hudHealth
    RectI r{m_w - 312, 12, 300, 118};                                          // Fenster
    m_ui.panel(r, PanelStyle::Paper);                                          // Hintergrund
    ShipStats st = m_state.stats();                                            // Schiffswerte
    bool ship = !m_state.onFoot;                                               // Modus
    float value = ship ? m_state.hull / st.hullMax : m_state.health / 100.0f;  // Anteil
    m_ui.text(r.x + 20, r.y + 18, ship ? "Rumpf" : "Gesundheit", UiColor::INK, 2); // Beschriftung
    std::string num = ship ? std::to_string(static_cast<int>(m_state.hull + 0.5f)) + "/" + std::to_string(static_cast<int>(st.hullMax)) : std::to_string(static_cast<int>(m_state.health + 0.5f)) + "/100"; // Zahlen
    m_ui.text(r.x + r.w - 20 - Font::textWidth(num, 2), r.y + 18, num, UiColor::INK, 2); // Rechtsbündig
    m_ui.progress(RectI{r.x + 18, r.y + 42, r.w - 36, 22}, value, value < 0.3f ? BarColor::Red : BarColor::Green); // Balken
    m_ui.text(r.x + 20, r.y + 76, cr(m_state.credits), rgba(160, 100, 10), 2); // Geld
    std::string ep = "Entdecker " + std::to_string(m_state.explorerPoints);    // Entdeckerpunkte
    m_ui.text(r.x + r.w - 20 - Font::textWidth(ep, 1), r.y + 82, ep, UiColor::INK_LIGHT, 1); // Klein rechts
} // Ende von hudHealth

// Unten links (Schiff): Windstärke und Wassertiefe
void Game::hudWindDepth() {                                                    // Beginn von hudWindDepth
    RectI r{12, m_h - 188, 400, 160};                                          // Fenster
    m_ui.panel(r, PanelStyle::Paper);                                          // Hintergrund
    Canvas& c = m_ui.canvas();                                                 // Zeichenfläche
    WeatherSlot w = m_state.weatherAt(m_state.hours);                          // Wetter
    int cx = r.x + 56, cy = r.y + 62;                                          // Windrose
    c.fillCircle(cx, cy, 34, rgba(225, 210, 175));                             // Hintergrund
    c.ring(cx, cy, 34, 3, rgba(120, 85, 50));                                  // Rand
    m_ui.text(cx + static_cast<int>(std::cos(screenAngle(0, -1)) * 26) - 3, cy + static_cast<int>(std::sin(screenAngle(0, -1)) * 26) - 4, "N", UiColor::RED, 1); // Norden markieren
    drawArrow(cx, cy, screenAngle(std::cos(w.windAngle), std::sin(w.windAngle)), static_cast<int>(18 + w.windStrength * 12), w.windStrength >= m_set.stormFrom ? UiColor::RED : rgba(40, 90, 160)); // Windpfeil (wohin der Wind weht)
    m_ui.text(r.x + 104, r.y + 20, "Wind", UiColor::INK_LIGHT, 2);             // Überschrift
    m_ui.text(r.x + 104, r.y + 42, fit(GameState::windName(w.windStrength), r.w - 120, 2), w.windStrength >= m_set.stormFrom ? UiColor::RED : UiColor::INK, 2); // Stärke
    m_ui.text(r.x + 104, r.y + 64, "aus " + GameState::directionName(w.windAngle), UiColor::INK, 2); // Richtung
    ShipStats st = m_state.stats();                                            // Schiffswerte
    float depth = m_world.depthAt(m_state.shipX, m_state.shipY);               // Wassertiefe
    Color dc = depth < st.draft + 0.8f ? UiColor::RED : (depth < st.draft + 2.5f ? rgba(200, 130, 40) : UiColor::INK); // Warnfarbe
    m_ui.text(r.x + 20, r.y + 104, "Tiefe " + f1(depth) + " m", dc, 2);        // Wassertiefe
    m_ui.text(r.x + 210, r.y + 108, "Tiefgang " + f1(st.draft) + " m", UiColor::INK_LIGHT, 1); // Tiefgang
    std::string drive;                                                         // Fahrstufe als Text
    if (m_throttle < 0) drive = "Rückwärts";                                   // Rückwärts
    else drive = (m_state.shipClass().drive == "motor" ? "Maschine " : "Segel ") + std::string(static_cast<std::size_t>(m_throttle), '|') + std::string(static_cast<std::size_t>(3 - m_throttle), '.'); // Stufe als Striche
    if (m_engineOn && m_state.shipClass().drive == "hybrid") drive += " +Masch.";      // Hilfsmaschine
    if (m_autopilot) drive += "  Autopilot";                                   // Autopilot
    m_ui.text(r.x + 20, r.y + 128, drive, UiColor::INK, 2);                    // Fahrstufe
    std::string spd = f1(std::fabs(m_shipSpeed) * 4.0f) + " kn";               // Tempo in Knoten (Spielmaßstab)
    m_ui.text(r.x + r.w - 24 - Font::textWidth(spd, 2), r.y + 128, spd, UiColor::INK, 2); // Rechtsbündig
    if (st.fuelMax > 0.0f) m_ui.progress(RectI{r.x + 210, r.y + 122, 100, 14}, m_state.fuel / st.fuelMax, BarColor::Blue); // Treibstoff
} // Ende von hudWindDepth

// Unten links (Figur): Dialogfenster
void Game::hudDialog() {                                                       // Beginn von hudDialog
    RectI r{12, m_h - 188, 520, 160};                                          // Fenster
    m_ui.panel(r, PanelStyle::Paper);                                          // Hintergrund
    int island = m_world.islandAt(m_state.figX, m_state.figY);                 // Insel unter der Figur
    std::string speaker = "Kapitän";                                           // Sprecher
    std::string text;                                                          // Text
    int b = nearDoor();                                                        // Gebäude in der Nähe
    if (b >= 0) {                                                              // Vor einer Tür
        const Building& bd = m_world.buildings[static_cast<std::size_t>(b)];   // Gebäude
        speaker = buildingTitle(b);                                            // Gebäude spricht
        if (!m_state.isOpen(bd.type)) text = "Geschlossen! " + m_state.openText(bd.type) + "."; // Geschlossen
        else if (bd.type == "kontor") text = "Willkommen im Kontor! Wir kaufen alles und verkaufen, was auf der Insel wächst. Aufträge gibt es hier auch."; // Begrüßung
        else if (bd.type == "museum") text = "Habt Ihr Artefakte gefunden? Wir zahlen gut - und Euer Ruf als Entdecker wächst."; // Begrüßung
        else if (bd.type == "werft") text = "Reparaturen, Verbesserungen und neue Schiffe - alles aus einer Hand!"; // Begrüßung
        else if (bd.type == "taverne") text = "Ahoi! Hier findet Ihr neue Leute für Eure Mannschaft, ein Bett und die neuesten Gerüchte."; // Begrüßung
        else if (bd.type == "haendler") text = "Frische Ware! Ich kaufe nur begrenzte Mengen, zahle aber besser als das Kontor."; // Begrüßung
        else text = "Ich verarbeite Rohwaren zu feinen Produkten. Bringt mir Nachschub!"; // Hersteller
    } else if (nearPier()) {                                                   // Am Steg
        speaker = "Bootsmann";                                                 // Sprecher
        text = "Die gekaufte Ware liegt hier am Steg. Wir können das Schiff beladen und ablegen, Käpt'n!"; // Text
    } else if (!m_log.empty()) {                                               // Letzte Meldung
        text = m_log.back();                                                   // Text
    } else {                                                                   // Nichts los
        text = "Klicke auf ein Gebäude oder laufe mit " + keyName("hoch") + keyName("links") + keyName("runter") + keyName("rechts") + ". Am Stegende geht es zurück an Bord."; // Hilfe
    }                                                                          // Ende der Auswahl
    m_ui.text(r.x + 22, r.y + 18, speaker + (island >= 0 ? "  (" + islandName(island) + ")" : ""), rgba(150, 70, 30), 2); // Sprecher
    m_ui.textWrapped(r.x + 22, r.y + 46, r.w - 44, text, UiColor::INK, 2);     // Text
} // Ende von hudDialog

// Unten rechts: Pfeil zum gesetzten Ziel
void Game::hudTarget() {                                                       // Beginn von hudTarget
    int cx = m_w - 86, cy = m_h - 124;                                         // Mitte des Rings
    Canvas& c = m_ui.canvas();                                                 // Zeichenfläche
    c.fillCircle(cx, cy, 56, rgba(225, 210, 175));                             // Papierscheibe
    m_ui.iconScaled("minimapMap_woodRing", cx, cy, 144);                       // Holzring
    m_ui.addArea(RectI{cx - 72, cy - 72, 144, 144});                           // Bereich gehört zur Oberfläche
    float px = m_state.onFoot ? m_state.figX : m_state.shipX;                  // Eigene Position x
    float py = m_state.onFoot ? m_state.figY : m_state.shipY;                  // Eigene Position y
    std::string label = "Kein Ziel (" + keyName("seekarte") + ")";             // Beschriftung
    if (m_state.hasTarget) {                                                   // Ziel vorhanden
        float dx = m_state.targetX - px, dy = m_state.targetY - py;            // Richtung
        float d = std::sqrt(dx * dx + dy * dy);                                // Entfernung
        float a = screenAngle(dx, dy);                                         // Bildschirmwinkel
        drawArrow(cx, cy, a, 34, rgba(190, 50, 30));                           // Großer Pfeil zum Ziel
        label = fit(m_state.targetName, 220, 2) + " " + std::to_string(static_cast<int>(d)) + " sm"; // Name und Entfernung
    } else m_ui.icon("minimapIcon_exclamationYellow", cx, cy);                 // Ausrufezeichen
    int w = Font::textWidth(label, 2) + 24;                                    // Breite des Schildes
    RectI lr{std::min(m_w - w - 8, cx - w / 2), cy + 70, w, 30};               // Schild unter dem Ring
    c.fillRect(lr.x, lr.y, lr.w, lr.h, rgba(30, 20, 10, 170));                 // Hintergrund
    m_ui.textShadow(lr.x + 12, lr.y + 8, label, rgba(255, 235, 180), 2);       // Text
} // Ende von hudTarget

// Meldungen oben in der Bildmitte
void Game::hudNotices() {                                                      // Beginn von hudNotices
    int y = 14;                                                                // Startzeile
    for (const Notice& n : m_notices) {                                        // Alle Meldungen
        std::vector<std::string> lines = Font::wrap(n.text, 420, 2);           // Umbrechen
        int h = static_cast<int>(lines.size()) * Font::lineHeight(2) + 14;     // Höhe
        int alpha = static_cast<int>(clampValue(n.time, 0.0f, 1.0f) * 180.0f); // Ausblenden am Ende
        m_ui.canvas().fillRect(m_w / 2 - 225, y, 450, h, rgba(20, 15, 10, alpha)); // Hintergrund
        for (std::size_t i = 0; i < lines.size(); ++i) m_ui.textShadow(m_w / 2 - Font::textWidth(lines[i], 2) / 2, y + 8 + static_cast<int>(i) * Font::lineHeight(2), lines[i], withAlpha(rgba(255, 245, 220), std::min(255, alpha + 75)), 2); // Zeilen
        y += h + 6;                                                            // Nächste Meldung
    }                                                                          // Ende der Meldungen
} // Ende von hudNotices

// ============================================================ Menü und Punkteliste

// Hauptmenü
void Game::renderMenu() {                                                      // Beginn von renderMenu
    const std::string title = "Mini Ship Delivery";                            // Titel
    m_ui.textShadow(m_w / 2 - Font::textWidth(title, 7) / 2, 70, title, rgba(255, 220, 120), 7); // Großer Titel
    const std::string sub = "Handel, Abenteuer und Seefahrt zwischen den Inseln"; // Untertitel
    m_ui.textShadow(m_w / 2 - Font::textWidth(sub, 2) / 2, 150, sub, rgba(235, 240, 250), 2); // Untertitel
    RectI r{m_w / 2 - 190, 210, 380, 360};                                     // Menüfenster
    m_ui.panel(r, PanelStyle::Wood);                                           // Holzfenster
    int y = r.y + 36;                                                          // Erste Knopfzeile
    if (m_ui.button(RectI{r.x + 50, y, r.w - 100, 56}, "Neues Spiel")) startNewGame(); // Neues Spiel
    y += 74;                                                                   // Nächste Zeile
    if (m_ui.button(RectI{r.x + 50, y, r.w - 100, 56}, "Fortsetzen", m_hasSave)) continueGame(); // Spielstand laden
    y += 74;                                                                   // Nächste Zeile
    if (m_ui.button(RectI{r.x + 50, y, r.w - 100, 56}, "Score")) { loadScores(); m_screen = Screen::Scores; } // Punkteliste
    y += 74;                                                                   // Nächste Zeile
    if (m_ui.button(RectI{r.x + 50, y, r.w - 100, 56}, "Beenden")) m_running = false; // Beenden
    std::string foot = "3D-Modelle und Oberfläche: Kenney.nl (CC0)   -   " + m_audio.status(); // Fußzeile
    m_ui.textShadow(m_w / 2 - Font::textWidth(foot, 1) / 2, m_h - 24, foot, rgba(220, 230, 240), 1); // Fußzeile
} // Ende von renderMenu

// Punkteliste
void Game::renderScores() {                                                    // Beginn von renderScores
    RectI r{m_w / 2 - 380, 90, 760, 540};                                      // Fenster
    m_ui.panel(r, PanelStyle::WoodPaper);                                      // Hintergrund
    m_ui.text(r.x + 40, r.y + 30, "Score - die besten Kapitäne", UiColor::INK, 3); // Überschrift
    int y = r.y + 84;                                                          // Erste Zeile
    const int cols[6] = {40, 110, 260, 360, 510, 610};                         // Spalten
    const char* heads[6] = {"Rang", "Punkte", "Tage", "Credits", "Entdecker", "Schiff"}; // Überschriften
    for (int i = 0; i < 6; ++i) m_ui.text(r.x + cols[i], y, heads[i], UiColor::INK_LIGHT, 2); // Kopfzeile
    y += 32;                                                                   // Nächste Zeile
    if (m_scores.empty()) m_ui.text(r.x + 40, y + 20, "Noch keine Einträge. Spiel eine Runde!", UiColor::INK, 2); // Leere Liste
    for (std::size_t i = 0; i < m_scores.size() && i < 10; ++i) {              // Höchstens zehn Einträge
        const ScoreEntry& e = m_scores[i];                                     // Eintrag
        Color col = i == 0 ? rgba(170, 110, 20) : UiColor::INK;                // Erster Platz in Gold
        m_ui.text(r.x + cols[0], y, std::to_string(i + 1) + ".", col, 2);      // Rang
        m_ui.text(r.x + cols[1], y, thousands(e.score), col, 2);               // Punkte
        m_ui.text(r.x + cols[2], y, std::to_string(e.days), col, 2);           // Tage
        m_ui.text(r.x + cols[3], y, thousands(e.credits), col, 2);             // Geld
        m_ui.text(r.x + cols[4], y, std::to_string(e.explorer), col, 2);       // Entdeckerpunkte
        m_ui.text(r.x + cols[5], y, fit(e.ship, 120, 2), col, 2);              // Schiff
        y += 34;                                                               // Nächste Zeile
    }                                                                          // Ende der Einträge
    m_ui.text(r.x + 40, r.y + r.h - 62, "Punkte = Credits + Schiffswert + Ladung + 10 x Entdeckerpunkte", UiColor::INK_LIGHT, 1); // Erklärung
    if (m_ui.button(RectI{r.x + r.w - 220, r.y + r.h - 76, 180, 50}, "Zurück")) m_screen = Screen::Menu; // Zurück
} // Ende von renderScores

// ============================================================ Fenster-Hilfen

// Zeichnet einen Fensterrahmen mit Titel und Schließen-Knopf, liefert den Innenbereich
RectI Game::panelFrame(int w, int h, const std::string& title, PanelStyle style) { // Beginn von panelFrame
    w = std::min(w, m_w - 16); h = std::min(h, m_h - 16);                      // Nicht größer als der Bildschirm
    RectI r{(m_w - w) / 2, (m_h - h) / 2, w, h};                               // Zentriert
    m_ui.canvas().fillRect(0, 0, m_w, m_h, rgba(0, 0, 0, 80));                 // Hintergrund abdunkeln
    m_ui.panel(r, style);                                                      // Fenster
    m_ui.text(r.x + 34, r.y + 26, fit(title, w - 140, 3), UiColor::INK, 3);    // Titel
    if (m_panel != Panel::Sunk && m_ui.smallButton(RectI{r.x + r.w - 66, r.y + 18, 44, 44}, "X")) m_panel = Panel::None; // Schließen
    return RectI{r.x + 34, r.y + 74, r.w - 68, r.h - 100};                     // Innenbereich
} // Ende von panelFrame

// Reiterleiste
bool Game::tabs(const RectI& r, const std::vector<std::string>& names, int& current) { // Beginn von tabs
    int w = std::min(230, r.w / std::max<int>(1, static_cast<int>(names.size()))); // Breite je Reiter
    bool changed = false;                                                      // Gewechselt?
    for (std::size_t i = 0; i < names.size(); ++i) {                           // Alle Reiter
        if (m_ui.button(RectI{r.x + static_cast<int>(i) * w, r.y, w - 8, 40}, names[i], true, static_cast<int>(i) == current)) { current = static_cast<int>(i); m_scroll = 0; changed = true; } // Umschalten
    }                                                                          // Ende der Reiter
    return changed;                                                            // Ergebnis
} // Ende von tabs

// Begrenzt die Rollposition und wertet das Mausrad aus
void Game::ensureScroll(int count, int visible) {                              // Beginn von ensureScroll
    m_scroll -= m_input.wheel;                                                 // Mausrad
    m_scroll = clampValue(m_scroll, 0, std::max(0, count - visible));          // Begrenzen
} // Ende von ensureScroll

// Zeichnet ein Modell als kleines Symbol (wird einmal gerendert und zwischengespeichert)
void Game::drawModelIcon(const std::string& model, int cx, int cy, int size) { // Beginn von drawModelIcon
    std::string key = model + "|" + std::to_string(size);                      // Schlüssel
    auto it = m_icons.find(key);                                               // Schon vorhanden?
    if (it == m_icons.end()) {                                                 // Nein -> erzeugen
        const Sprite& probe = m_lib.sprite(model, 0.0f, 8, 64.0f);             // Probe in fester Größe
        Image img;                                                             // Ergebnis
        if (probe.valid()) {                                                   // Modell vorhanden
            float big = static_cast<float>(std::max(probe.image.width, probe.image.height)); // Größte Seite
            float tw = clampValue(64.0f * static_cast<float>(size) / std::max(1.0f, big), 4.0f, 900.0f); // Passender Maßstab
            img = m_lib.sprite(model, 0.0f, 8, std::floor(tw)).image;          // Endgültig rendern
        }                                                                      // Ende vorhanden
        it = m_icons.emplace(key, img).first;                                  // Speichern
    }                                                                          // Ende erzeugen
    if (!it->second.empty()) m_ui.canvas().blit(it->second, cx - it->second.width / 2, cy - it->second.height / 2); // Zentriert zeichnen
} // Ende von drawModelIcon

// Ware als Symbol
void Game::drawGoodIcon(const std::string& good, int cx, int cy, int size) {   // Beginn von drawGoodIcon
    const GoodDef* g = m_data.good(good);                                      // Ware
    if (g && !g->model.empty()) drawModelIcon(g->model, cx, cy, size);         // Modell zeichnen
    else m_ui.canvas().fillRect(cx - size / 3, cy - size / 3, size * 2 / 3, size * 2 / 3, rgba(160, 120, 70)); // Ersatz: Kiste
} // Ende von drawGoodIcon

// Zeichnet einen Pfeil (Winkel im Bildschirm, 0 = rechts)
void Game::drawArrow(int cx, int cy, float a, int len, Color col) {            // Beginn von drawArrow
    float dx = std::cos(a), dy = std::sin(a);                                  // Richtung
    float tx = static_cast<float>(cx) + dx * static_cast<float>(len), ty = static_cast<float>(cy) + dy * static_cast<float>(len); // Spitze
    float bx = static_cast<float>(cx) - dx * static_cast<float>(len) * 0.7f, by = static_cast<float>(cy) - dy * static_cast<float>(len) * 0.7f; // Ende
    float hl = std::max(10.0f, static_cast<float>(len) * 0.4f), hw = hl * 0.8f; // Größe der Spitze
    m_ui.canvas().lineF(bx, by, tx - dx * hl * 0.5f, ty - dy * hl * 0.5f, col, std::max(3.0f, static_cast<float>(len) / 7.0f)); // Schaft
    std::vector<std::pair<float, float>> head = {{tx + dx * 4.0f, ty + dy * 4.0f}, {tx - dx * hl - dy * hw, ty - dy * hl + dx * hw}, {tx - dx * hl + dy * hw, ty - dy * hl - dx * hw}}; // Spitze
    m_ui.canvas().fillPolygon(head, col);                                      // Spitze füllen
} // Ende von drawArrow

// Kurzinfo zu einer Ware
std::string Game::goodTooltip(const std::string& good) const {                 // Beginn von goodTooltip
    const GoodDef* g = m_data.good(good);                                      // Ware
    if (!g) return good;                                                       // Unbekannt
    std::string t = g->name + " (" + GameData::categoryName(g->category) + "): " + g->description + " Grundpreis " + cr(g->basePrice) + ", Gewicht " + std::to_string(g->weight); // Grunddaten
    if (g->perishDays > 0) t += ", verdirbt nach " + std::to_string(g->perishDays) + " Tagen"; // Haltbarkeit
    return t;                                                                  // Ergebnis
} // Ende von goodTooltip

// ============================================================ Fenster

// Zeichnet das offene Fenster
void Game::renderPanel() {                                                     // Beginn von renderPanel
    switch (m_panel) {                                                         // Je nach Fenster
    case Panel::None: break;                                                   // Keins
    case Panel::Kontor: panelKontor(); break;                                  // Kontor
    case Panel::Trader: panelTrader(); break;                                  // Händler
    case Panel::Producer: panelProducer(); break;                              // Hersteller
    case Panel::Museum: panelMuseum(); break;                                  // Museum
    case Panel::Shipyard: panelShipyard(); break;                              // Werft
    case Panel::Tavern: panelTavern(); break;                                  // Taverne
    case Panel::Pier: panelPier(); break;                                      // Steg
    case Panel::Chart: panelChart(); break;                                    // Seekarte
    case Panel::ShipInfo: panelShipInfo(); break;                              // Schiff
    case Panel::Pause: panelPause(); break;                                    // Pause
    case Panel::Newspaper: panelNewspaper(); break;                            // Zeitung
    case Panel::Help: panelHelp(); break;                                      // Hilfe
    case Panel::Sunk: panelSunk(); break;                                      // Schiff verloren
    }                                                                          // Ende der Fallunterscheidung
    if (m_panel != Panel::None) m_input.consumed = true;                       // Klicks gehen nicht in die Welt
} // Ende von renderPanel

// Kontor: Handel, Aufträge, Treibstoff und Lager
void Game::panelKontor() {                                                     // Beginn von panelKontor
    const Building& b = m_world.buildings[static_cast<std::size_t>(m_panelBuilding)]; // Gebäude
    int isl = b.island;                                                        // Insel
    RectI in = panelFrame(900, 600, "Kontor von " + islandName(isl));          // Rahmen
    tabs(in, {"Handel", "Aufträge", "Treibstoff & Lager"}, m_tab);             // Reiter
    int y = in.y + 54;                                                         // Inhalt beginnt unter den Reitern
    if (m_tab == 0) {                                                          // Handel
        std::vector<std::string> rows;                                         // Zeilen: eigene Waren der Insel + Vorrat des Spielers
        for (const GoodDef& g : m_data.goods) if (m_state.kontorSells(isl, g.id) || m_state.available(isl, g.id) > 0) rows.push_back(g.id); // Auswahl
        const int rowH = 44, visible = 9;                                      // Zeilenhöhe und Anzahl
        ensureScroll(static_cast<int>(rows.size()), visible);                  // Rollen
        m_ui.text(in.x + 50, y, "Ware", UiColor::INK_LIGHT, 2); m_ui.text(in.x + 226, y, "Kaufen", UiColor::INK_LIGHT, 2); // Kopfzeile
        m_ui.text(in.x + 470, y, "Vorrat", UiColor::INK_LIGHT, 2); m_ui.text(in.x + 568, y, "Verkaufen", UiColor::INK_LIGHT, 2); // Kopfzeile
        y += 26;                                                               // Erste Zeile
        for (int i = m_scroll; i < static_cast<int>(rows.size()) && i < m_scroll + visible; ++i) { // Sichtbare Zeilen
            const std::string& g = rows[static_cast<std::size_t>(i)];          // Ware
            const GoodDef* gd = m_data.good(g);                                // Definition
            if ((i - m_scroll) % 2 == 0) m_ui.canvas().fillRect(in.x, y, in.w, rowH - 2, rgba(120, 80, 40, 22)); // Zebrastreifen
            drawGoodIcon(g, in.x + 22, y + rowH / 2, 36);                      // Symbol
            RectI nameR{in.x + 44, y, 170, rowH};                              // Namensbereich
            m_ui.text(in.x + 50, y + 13, fit(gd->name, 160, 2), UiColor::INK, 2); // Name
            if (m_ui.hovered(nameR)) m_ui.tooltip(Font::wrap(goodTooltip(g), 420, 2)); // Kurzinfo
            if (m_state.kontorSells(isl, g)) {                                 // Kontor verkauft
                int price = m_state.kontorBuyPrice(isl, g);                    // Preis
                m_ui.text(in.x + 226, y + 13, cr(price), UiColor::INK, 2);     // Preis
                if (m_ui.button(RectI{in.x + 330, y + 4, 70, 36}, "1x", m_state.credits >= price)) { m_state.buy(isl, g, price); sound("kaufen"); } // Eine Einheit
                if (m_ui.button(RectI{in.x + 404, y + 4, 56, 36}, "5x", m_state.credits >= price * 5)) { for (int k = 0; k < 5; ++k) m_state.buy(isl, g, price); sound("kaufen"); } // Fünf Einheiten
            } else m_ui.text(in.x + 226, y + 13, "-", UiColor::INK_LIGHT, 2);  // Wird hier nicht verkauft
            int have = m_state.available(isl, g);                              // Vorrat des Spielers
            m_ui.text(in.x + 480, y + 13, std::to_string(have), UiColor::INK, 2); // Vorrat
            int sp = m_state.kontorSellPrice(isl, g);                          // Ankaufspreis des Kontors
            Color pc = static_cast<float>(sp) > static_cast<float>(gd->basePrice) * 1.15f ? UiColor::GREEN : (static_cast<float>(sp) < static_cast<float>(gd->basePrice) * 0.8f ? UiColor::RED : UiColor::INK); // Gut oder schlecht
            m_ui.text(in.x + 568, y + 13, cr(sp), pc, 2);                      // Preis
            if (m_ui.button(RectI{in.x + 668, y + 4, 70, 36}, "1x", have > 0)) { m_state.sellOne(isl, g, sp, false, 0.0f); m_state.saturate(isl, g); sound("verkaufen"); } // Eine verkaufen
            if (m_ui.button(RectI{in.x + 742, y + 4, 80, 36}, "Alle", have > 0)) { for (int k = 0; k < have; ++k) { m_state.sellOne(isl, g, m_state.kontorSellPrice(isl, g), false, 0.0f); m_state.saturate(isl, g); } sound("verkaufen"); } // Alle verkaufen
            y += rowH;                                                         // Nächste Zeile
        }                                                                      // Ende der Zeilen
        m_ui.text(in.x, in.y + in.h - 22, "Gekaufte Ware liegt am Steg. Verladen am Stegende. Viele Verkäufe drücken den Preis.", UiColor::INK_LIGHT, 1); // Hinweis
    } else if (m_tab == 1) {                                                   // Aufträge
        m_ui.text(in.x, y, "Angebote (Frist und Lohn):", UiColor::INK_LIGHT, 2); // Überschrift
        y += 28;                                                               // Nächste Zeile
        int shown = 0;                                                         // Angezeigte Angebote
        for (std::size_t i = 0; i < m_state.offers.size(); ++i) {              // Alle Angebote
            const Contract& c = m_state.offers[i];                             // Angebot
            if (c.from != isl) continue;                                       // Nur von dieser Insel
            drawGoodIcon(c.good, in.x + 22, y + 22, 34);                       // Symbol
            m_ui.text(in.x + 50, y + 4, std::to_string(c.amount) + " " + goodName(c.good) + " nach " + islandName(c.to), UiColor::INK, 2); // Was und wohin
            m_ui.text(in.x + 50, y + 26, "bis " + GameState::timeText(c.deadline) + "   Lohn " + cr(c.reward), UiColor::INK_LIGHT, 1); // Frist und Lohn
            if (m_ui.button(RectI{in.x + in.w - 170, y + 4, 160, 38}, "Annehmen")) { Contract copy = c; std::string err = m_state.acceptContract(i); if (err.empty()) { notify("Auftrag angenommen: " + std::to_string(copy.amount) + " " + goodName(copy.good) + " nach " + islandName(copy.to)); sound("glocke"); } else notify(err); break; } // Annehmen (Kopie, weil das Angebot entfernt wird)
            y += 50; ++shown;                                                  // Nächste Zeile
        }                                                                      // Ende der Angebote
        if (shown == 0) { m_ui.text(in.x + 20, y, "Heute keine weiteren Angebote. Morgen früh gibt es neue.", UiColor::INK, 2); y += 30; } // Keine Angebote
        y += 16;                                                               // Abstand
        m_ui.text(in.x, y, "Deine Aufträge:", UiColor::INK_LIGHT, 2);          // Überschrift
        y += 28;                                                               // Nächste Zeile
        if (m_state.active.empty()) m_ui.text(in.x + 20, y, "Keine.", UiColor::INK, 2); // Keine aktiven Aufträge
        for (std::size_t i = 0; i < m_state.active.size(); ++i) {              // Aktive Aufträge
            const Contract& c = m_state.active[i];                             // Auftrag
            drawGoodIcon(c.good, in.x + 22, y + 22, 34);                       // Symbol
            m_ui.text(in.x + 50, y + 4, std::to_string(c.amount) + " " + goodName(c.good) + " nach " + islandName(c.to) + " (" + cr(c.reward) + ")", UiColor::INK, 2); // Auftrag
            m_ui.text(in.x + 50, y + 26, "Frist " + GameState::timeText(c.deadline) + "   vorhanden: " + std::to_string(m_state.available(isl, c.good)), c.deadline - m_state.hours < 4.0f ? UiColor::RED : UiColor::INK_LIGHT, 1); // Frist
            if (c.to == isl && m_ui.button(RectI{in.x + in.w - 170, y + 4, 160, 38}, "Abliefern", m_state.available(isl, c.good) >= c.amount)) { // Abliefern möglich
                int reward = c.reward;                                         // Lohn merken
                std::string err = m_state.deliverContract(i, isl);             // Abliefern
                if (err.empty()) { notify("Auftrag erfüllt! +" + cr(reward)); sound("verkaufen"); } else notify(err); // Ergebnis
                break;                                                         // Liste hat sich geändert
            }                                                                  // Ende Abliefern
            y += 50;                                                           // Nächste Zeile
        }                                                                      // Ende der Aufträge
    } else {                                                                   // Treibstoff und Lager
        ShipStats st = m_state.stats();                                        // Schiffswerte
        if (st.fuelMax > 0.0f) {                                               // Schiff mit Maschine
            m_ui.text(in.x, y, "Kohle: " + std::to_string(static_cast<int>(m_state.fuel)) + " / " + std::to_string(static_cast<int>(st.fuelMax)), UiColor::INK, 2); // Füllstand
            m_ui.progress(RectI{in.x + 260, y - 2, 300, 22}, m_state.fuel / st.fuelMax, BarColor::Blue); // Balken
            int need = static_cast<int>(st.fuelMax - m_state.fuel);            // Fehlende Menge
            int cost = need * m_set.fuelPrice;                                 // Kosten
            if (m_ui.button(RectI{in.x + 580, y - 10, 240, 40}, "Auffüllen " + cr(cost), need > 0 && m_state.credits > 0)) { // Auffüllen
                int buyable = std::min(need, m_state.credits / std::max(1, m_set.fuelPrice)); // Bezahlbare Menge
                m_state.credits -= buyable * m_set.fuelPrice; m_state.fuel += static_cast<float>(buyable); sound("kaufen"); // Kaufen
            }                                                                  // Ende Auffüllen
        } else m_ui.text(in.x, y, "Dein Schiff fährt nur unter Segeln und braucht keine Kohle.", UiColor::INK, 2); // Kein Treibstoff nötig
        y += 50;                                                               // Abstand
        m_ui.text(in.x, y, "Am Steg gelagert:", UiColor::INK_LIGHT, 2);        // Überschrift
        y += 28;                                                               // Nächste Zeile
        std::map<std::string, std::pair<int, int>> pierCount;                  // Ware -> (gut, verdorben)
        for (const Cargo& c : m_state.pier(isl)) { auto& p = pierCount[c.good]; if (m_state.rotten(c)) ++p.second; else ++p.first; } // Zählen
        int col = 0;                                                           // Spalte
        for (const auto& kv : pierCount) {                                     // Alle Waren
            int x = in.x + (col % 3) * 280, yy = y + (col / 3) * 44;           // Position
            drawGoodIcon(kv.first, x + 20, yy + 18, 32);                       // Symbol
            m_ui.text(x + 44, yy + 8, fit(goodName(kv.first), 150, 2) + " " + std::to_string(kv.second.first), UiColor::INK, 2); // Name und Menge
            if (kv.second.second > 0) m_ui.text(x + 44, yy + 28, std::to_string(kv.second.second) + " verdorben", UiColor::RED, 1); // Verdorben
            ++col;                                                             // Nächste Spalte
        }                                                                      // Ende der Waren
        if (pierCount.empty()) m_ui.text(in.x + 20, y, "Nichts.", UiColor::INK, 2); // Leer
        if (m_ui.button(RectI{in.x, in.y + in.h - 46, 300, 42}, "Verdorbenes wegwerfen")) { int n = m_state.discardRotten(isl); notify(std::to_string(n) + " verdorbene Einheiten entsorgt."); } // Wegwerfen
    }                                                                          // Ende der Reiter
} // Ende von panelKontor

// Händler: begrenzter Ankauf und Verkauf
void Game::panelTrader() {                                                     // Beginn von panelTrader
    TraderState* t = m_state.trader(m_panelBuilding);                          // Zustand
    if (!t) { m_panel = Panel::None; return; }                                 // Unbekannt
    const TraderDef* d = m_data.trader(t->def);                                // Definition
    int isl = m_world.buildings[static_cast<std::size_t>(m_panelBuilding)].island; // Insel
    RectI in = panelFrame(980, 560, (d ? d->name : "Händler") + " - " + islandName(isl)); // Rahmen
    int half = in.w / 2 - 10;                                                  // Breite einer Spalte
    int y0 = in.y;                                                             // Oberkante
    m_ui.text(in.x, y0, "Verkauft (Vorrat):", UiColor::INK_LIGHT, 2);          // Linke Spalte
    m_ui.text(in.x + half + 20, y0, "Kauft (Bedarf):", UiColor::INK_LIGHT, 2); // Rechte Spalte
    int y = y0 + 30;                                                           // Erste Zeile links
    for (const std::string& g : t->sells) {                                    // Angebot
        int stock = static_cast<int>(t->stock[g]);                             // Vorrat
        int price = m_state.traderBuyPrice(*t, g);                             // Preis
        drawGoodIcon(g, in.x + 20, y + 22, 34);                                // Symbol
        m_ui.text(in.x + 46, y + 4, fit(goodName(g), 170, 2) + "  x" + std::to_string(stock), UiColor::INK, 2); // Name und Vorrat
        m_ui.text(in.x + 46, y + 26, cr(price) + " (Kontor: " + (m_state.kontorSells(isl, g) ? cr(m_state.kontorBuyPrice(isl, g)) : std::string("-")) + ")", UiColor::INK_LIGHT, 1); // Preisvergleich
        if (m_ui.hovered(RectI{in.x, y, 260, 44})) m_ui.tooltip(Font::wrap(goodTooltip(g), 420, 2)); // Kurzinfo
        if (m_ui.button(RectI{in.x + half - 120, y + 4, 110, 38}, "Kaufen", stock >= 1 && m_state.credits >= price)) { if (m_state.buy(isl, g, price).empty()) { t->stock[g] -= 1.0f; sound("kaufen"); } } // Kaufen
        y += 50;                                                               // Nächste Zeile
    }                                                                          // Ende Angebot
    if (t->sells.empty()) m_ui.text(in.x + 20, y, "Heute nichts im Angebot.", UiColor::INK, 2); // Kein Angebot
    y = y0 + 30;                                                               // Erste Zeile rechts
    int x = in.x + half + 20;                                                  // Rechte Spalte
    for (const std::string& g : t->buys) {                                     // Bedarf
        int demand = static_cast<int>(t->demand[g]);                           // Bedarf
        int have = m_state.available(isl, g);                                  // Vorrat des Spielers
        int base = m_state.traderSellPrice(*t, g, 0.0f);                       // Preis für alte Ware
        int fresh = m_state.traderSellPrice(*t, g, 1.0f);                      // Preis für frische Ware
        drawGoodIcon(g, x + 20, y + 22, 34);                                   // Symbol
        m_ui.text(x + 46, y + 4, fit(goodName(g), 170, 2) + "  Bedarf " + std::to_string(demand), UiColor::INK, 2); // Name und Bedarf
        m_ui.text(x + 46, y + 26, (fresh != base ? cr(base) + " - " + cr(fresh) + " frisch" : cr(base)) + "  (du: " + std::to_string(have) + ")", UiColor::INK_LIGHT, 1); // Preis
        if (m_ui.hovered(RectI{x, y, 260, 44})) m_ui.tooltip(Font::wrap(goodTooltip(g), 420, 2)); // Kurzinfo
        if (m_ui.button(RectI{x + half - 120, y + 4, 110, 38}, "Verkaufen", have > 0 && demand >= 1)) { // Verkaufen
            int earned = m_state.sellOne(isl, g, base, true, d ? d->freshBonus : 0.0f); // Verkaufen mit Frischezuschlag
            if (earned > 0) {                                                  // Erfolgreich
                t->demand[g] -= 1.0f;                                          // Bedarf sinkt
                for (const std::string& s : t->sells) t->stock[s] = std::min(static_cast<float>(d ? d->stockMax : 6) * 1.5f, t->stock[s] + (d ? d->deliveryBonus : 0.3f)); // Lieferung belebt das Angebot
                sound("verkaufen");                                            // Geräusch
                notify("Verkauft: " + goodName(g) + " für " + cr(earned));     // Meldung
            }                                                                  // Ende erfolgreich
        }                                                                      // Ende Verkaufen
        y += 50;                                                               // Nächste Zeile
    }                                                                          // Ende Bedarf
    std::string info = d && d->demandMode == "steigend" ? "Der Bedarf wächst langsam nach (+" + f1(d->demandRate) + " je Stunde)." : "Der Bedarf wird jeden Morgen um 6 Uhr wieder voll."; // Bedarfsart
    info += " Frische, verderbliche Ware bringt mehr. Jede Lieferung füllt das Angebot auf.";  // Hinweis
    m_ui.textWrapped(in.x, in.y + in.h - 42, in.w, info, UiColor::INK_LIGHT, 1); // Unten
} // Ende von panelTrader

// Hersteller: kauft Rohwaren, verkauft Produkte
void Game::panelProducer() {                                                   // Beginn von panelProducer
    ProducerState* p = m_state.producer(m_panelBuilding);                      // Zustand
    if (!p) { m_panel = Panel::None; return; }                                 // Unbekannt
    const ProducerDef* d = m_data.producer(p->def);                            // Definition
    if (!d) { m_panel = Panel::None; return; }                                 // Unbekannt
    int isl = m_world.buildings[static_cast<std::size_t>(m_panelBuilding)].island; // Insel
    RectI in = panelFrame(980, 580, d->name + " - " + islandName(isl));        // Rahmen
    int half = in.w / 2 - 10;                                                  // Spaltenbreite
    int space = m_state.producerSpace(*p);                                     // Freier Lagerplatz
    m_ui.text(in.x, in.y, "Produkte:", UiColor::INK_LIGHT, 2);                 // Linke Spalte
    int y = in.y + 30;                                                         // Erste Zeile
    for (const auto& r : d->recipes) {                                         // Alle Produkte
        int stock = static_cast<int>(p->stock[r.product]);                     // Vorrat
        int price = m_state.producerBuyPrice(*p, r.product);                   // Preis
        drawGoodIcon(r.product, in.x + 20, y + 22, 36);                        // Symbol
        m_ui.text(in.x + 46, y + 4, fit(goodName(r.product), 170, 2) + "  x" + std::to_string(stock), UiColor::INK, 2); // Name und Vorrat
        std::string recipe;                                                    // Rezept als Text
        for (const auto& inp : r.inputs) { if (!recipe.empty()) recipe += " + "; recipe += std::to_string(inp.second) + " " + goodName(inp.first); } // Zutaten
        m_ui.text(in.x + 46, y + 26, cr(price) + "   aus " + recipe, UiColor::INK_LIGHT, 1); // Preis und Rezept
        if (m_ui.hovered(RectI{in.x, y, 260, 44})) m_ui.tooltip(Font::wrap(goodTooltip(r.product), 420, 2)); // Kurzinfo
        if (m_ui.button(RectI{in.x + half - 110, y + 4, 100, 38}, "Kaufen", stock >= 1 && m_state.credits >= price)) { if (m_state.buy(isl, r.product, price).empty()) { p->stock[r.product] -= 1.0f; sound("kaufen"); } } // Kaufen
        y += 54;                                                               // Nächste Zeile
    }                                                                          // Ende der Produkte
    y += 10;                                                                   // Abstand
    m_ui.text(in.x, y, "Werkstatt: " + f1(d->productionHours) + " Std. je Stück", UiColor::INK_LIGHT, 2); // Produktionszeit
    m_ui.progress(RectI{in.x, y + 28, half - 20, 20}, p->progress / d->productionHours, BarColor::Blue); // Fortschritt
    m_ui.text(in.x, y + 60, "Lager: " + std::to_string(d->storage - space) + " / " + std::to_string(d->storage), space <= 0 ? UiColor::RED : UiColor::INK, 2); // Lagerbelegung
    int x = in.x + half + 20;                                                  // Rechte Spalte
    m_ui.text(x, in.y, "Kauft Rohwaren:", UiColor::INK_LIGHT, 2);              // Überschrift
    y = in.y + 30;                                                             // Erste Zeile
    for (const std::string& g : d->rawGoods) {                                 // Rohwaren
        int stock = static_cast<int>(p->stock[g]);                             // Lagerbestand
        int have = m_state.available(isl, g);                                  // Vorrat des Spielers
        int price = m_state.producerSellPrice(*p, g);                          // Preis
        drawGoodIcon(g, x + 20, y + 22, 34);                                   // Symbol
        m_ui.text(x + 46, y + 4, fit(goodName(g), 150, 2) + "  " + std::to_string(stock) + "/" + std::to_string(d->rawMax), UiColor::INK, 2); // Name und Bestand
        m_ui.text(x + 46, y + 26, cr(price) + " (Kontor " + cr(m_state.kontorSellPrice(isl, g)) + ")  du: " + std::to_string(have), UiColor::INK_LIGHT, 1); // Preisvergleich
        bool can = have > 0 && stock < d->rawMax && space > 0;                 // Ankauf möglich?
        if (m_ui.button(RectI{x + half - 120, y + 4, 110, 38}, "Verkaufen", can)) { if (m_state.sellOne(isl, g, price, false, 0.0f) > 0) { p->stock[g] += 1.0f; sound("verkaufen"); } } // Verkaufen
        y += 54;                                                               // Nächste Zeile
    }                                                                          // Ende der Rohwaren
    m_ui.textWrapped(in.x, in.y + in.h - 42, in.w, "Hersteller zahlen für Rohwaren mehr als das Kontor, nehmen aber nur so viel, wie ins Lager passt. Je mehr Rohwaren, desto mehr Produkte.", UiColor::INK_LIGHT, 1); // Hinweis
} // Ende von panelProducer

// Museum: Artefakte verkaufen
void Game::panelMuseum() {                                                     // Beginn von panelMuseum
    int isl = m_world.buildings[static_cast<std::size_t>(m_panelBuilding)].island; // Insel
    RectI in = panelFrame(860, 560, "Museum von " + islandName(isl));          // Rahmen
    int tier = m_state.unlockedTier();                                         // Rang
    m_ui.text(in.x, in.y, "Entdeckerpunkte: " + std::to_string(m_state.explorerPoints) + "    Rang " + std::to_string(tier), UiColor::INK, 2); // Punkte
    if (tier < static_cast<int>(m_data.explorerThresholds.size()) && m_data.explorerThresholds[static_cast<std::size_t>(tier)] < 999999) m_ui.text(in.x, in.y + 26, "Nächster Rang ab " + std::to_string(m_data.explorerThresholds[static_cast<std::size_t>(tier)]) + " Punkten: wertvollere Artefakte treiben auf See.", UiColor::INK_LIGHT, 1); // Nächste Schwelle
    int y = in.y + 56;                                                         // Liste
    if (m_state.artifacts.empty()) m_ui.textWrapped(in.x + 10, y, in.w - 20, "Ihr habt keine Artefakte an Bord. Haltet auf See Ausschau nach Treibgut - der Ausguck markiert es auf der Seekarte. Seltene Stücke treiben oft in gefährlichen Gewässern.", UiColor::INK, 2); // Leer
    for (std::size_t i = 0; i < m_state.artifacts.size(); ++i) {               // Alle Artefakte
        const ArtifactDef* a = m_data.artifact(m_state.artifacts[i]);          // Definition
        if (!a) continue;                                                      // Unbekannt
        if (y > in.y + in.h - 60) break;                                       // Kein Platz mehr
        drawModelIcon(a->model, in.x + 26, y + 26, 44);                        // Symbol
        m_ui.text(in.x + 60, y + 4, a->name + "  (Stufe " + std::to_string(a->tier) + ")", UiColor::INK, 2); // Name
        m_ui.text(in.x + 60, y + 28, fit(a->description, in.w - 300, 1), UiColor::INK_LIGHT, 1); // Beschreibung
        m_ui.text(in.x + in.w - 330, y + 14, cr(a->value) + " + " + std::to_string(a->points) + " P", rgba(150, 100, 20), 2); // Wert
        if (m_ui.button(RectI{in.x + in.w - 140, y + 6, 130, 40}, "Verkaufen")) { std::string name = a->name; int v = a->value; if (m_state.sellArtifact(i).empty()) { notify(name + " verkauft: +" + cr(v)); sound("verkaufen"); } break; } // Verkaufen
        y += 56;                                                               // Nächste Zeile
    }                                                                          // Ende der Artefakte
} // Ende von panelMuseum

// Werft: Reparatur, Verbesserungen, neue Schiffe
void Game::panelShipyard() {                                                   // Beginn von panelShipyard
    int isl = m_world.buildings[static_cast<std::size_t>(m_panelBuilding)].island; // Insel
    RectI in = panelFrame(1000, 620, "Werft von " + islandName(isl));          // Rahmen
    tabs(in, {"Reparatur", "Verbesserungen", "Schiffe"}, m_tab);               // Reiter
    int y = in.y + 56;                                                         // Inhalt
    ShipStats st = m_state.stats();                                            // Schiffswerte
    if (m_tab == 0) {                                                          // Reparatur
        m_ui.text(in.x, y, m_state.shipClass().name + " - Rumpf " + std::to_string(static_cast<int>(m_state.hull)) + " / " + std::to_string(static_cast<int>(st.hullMax)), UiColor::INK, 2); // Zustand
        m_ui.progress(RectI{in.x, y + 30, 500, 24}, m_state.hull / st.hullMax, m_state.hull / st.hullMax < 0.3f ? BarColor::Red : BarColor::Green); // Balken
        int missing = static_cast<int>(st.hullMax - m_state.hull + 0.99f);     // Fehlende Punkte
        int cost = missing * m_set.repairPrice;                                // Kosten
        if (m_ui.button(RectI{in.x, y + 70, 320, 44}, "Ganz reparieren " + cr(cost), missing > 0 && m_state.credits >= cost)) { m_state.credits -= cost; m_state.hull = st.hullMax; sound("kaufen"); notify("Das Schiff ist wieder wie neu."); } // Komplett
        int part = std::min(missing, 10);                                      // Teilreparatur
        if (m_ui.button(RectI{in.x + 340, y + 70, 280, 44}, "+10 Punkte " + cr(part * m_set.repairPrice), part > 0 && m_state.credits >= part * m_set.repairPrice)) { m_state.credits -= part * m_set.repairPrice; m_state.hull = std::min(st.hullMax, m_state.hull + static_cast<float>(part)); sound("kaufen"); } // Teilweise
        if (st.fuelMax > 0.0f) {                                               // Treibstoff
            int need = static_cast<int>(st.fuelMax - m_state.fuel);            // Fehlend
            m_ui.text(in.x, y + 150, "Kohle " + std::to_string(static_cast<int>(m_state.fuel)) + " / " + std::to_string(static_cast<int>(st.fuelMax)), UiColor::INK, 2); // Füllstand
            if (m_ui.button(RectI{in.x, y + 180, 320, 44}, "Auffüllen " + cr(need * m_set.fuelPrice), need > 0 && m_state.credits >= need * m_set.fuelPrice)) { m_state.credits -= need * m_set.fuelPrice; m_state.fuel = st.fuelMax; sound("kaufen"); } // Auffüllen
        }                                                                      // Ende Treibstoff
        m_ui.textWrapped(in.x, y + 260, in.w, "Auf See flickt die Crew das Schiff langsam (bis " + std::to_string(static_cast<int>(m_set.seaRepairMax * 100.0f)) + " %). Ein Schiffszimmermann hilft dabei. Volle Reparatur gibt es nur in der Werft.", UiColor::INK_LIGHT, 2); // Hinweis
    } else if (m_tab == 1) {                                                   // Verbesserungen
        const int rowH = 52, visible = 8;                                      // Zeilen
        ensureScroll(static_cast<int>(m_data.upgrades.size()), visible);       // Rollen
        for (int i = m_scroll; i < static_cast<int>(m_data.upgrades.size()) && i < m_scroll + visible; ++i) { // Sichtbare Upgrades
            const UpgradeDef& u = m_data.upgrades[static_cast<std::size_t>(i)]; // Upgrade
            int lvl = m_state.upgradeLevel(u.id);                              // Stufe
            int price = m_state.upgradePrice(u.id);                            // Preis
            bool usable = u.attribute != "treibstoff_max" || st.motorPower > 0.0f; // Tank nur mit Maschine sinnvoll
            if ((i - m_scroll) % 2 == 0) m_ui.canvas().fillRect(in.x, y, in.w, rowH - 4, rgba(120, 80, 40, 22)); // Zebrastreifen
            m_ui.text(in.x + 10, y + 4, u.name + "  [" + std::to_string(lvl) + "/" + std::to_string(u.maxLevel) + "]", UiColor::INK, 2); // Name und Stufe
            m_ui.text(in.x + 10, y + 28, fit(u.description + "  (" + attributeName(u.attribute) + " +" + f1(u.value) + " je Stufe)", in.w - 330, 1), UiColor::INK_LIGHT, 1); // Beschreibung
            bool can = usable && lvl < u.maxLevel && m_state.credits >= price; // Kaufbar?
            if (m_ui.button(RectI{in.x + in.w - 300, y + 4, 290, 40}, lvl >= u.maxLevel ? "Maximum" : "Einbauen " + cr(price), can)) { // Kaufen
                m_state.credits -= price; m_state.upgrades[u.id] = lvl + 1; m_state.upgradeSpent += price; // Bezahlen und einbauen
                if (u.attribute == "rumpf") m_state.hull += u.value;           // Neuer Rumpf ist heil
                sound("kaufen"); notify(u.name + " eingebaut.");               // Meldung
            }                                                                  // Ende Kaufen
            y += rowH;                                                         // Nächste Zeile
        }                                                                      // Ende der Upgrades
        m_ui.text(in.x, in.y + in.h - 20, "Verbesserungen gehen beim Kauf eines neuen Schiffs verloren (sie werden zur Hälfte angerechnet).", UiColor::INK_LIGHT, 1); // Hinweis
    } else {                                                                   // Schiffe
        const int rowH = 74, visible = 6;                                      // Zeilen
        ensureScroll(static_cast<int>(m_data.ships.size()), visible);          // Rollen
        int value = m_state.shipValue();                                       // Inzahlungnahme
        m_ui.text(in.x, y - 6, "Dein Schiff wird mit " + cr(value) + " angerechnet.", UiColor::INK_LIGHT, 1); // Hinweis
        y += 12;                                                               // Abstand
        for (int i = m_scroll; i < static_cast<int>(m_data.ships.size()) && i < m_scroll + visible; ++i) { // Sichtbare Schiffe
            const ShipClass& s = m_data.ships[static_cast<std::size_t>(i)];    // Schiffsklasse
            bool mine = s.id == m_state.shipId;                                // Eigenes Schiff?
            if ((i - m_scroll) % 2 == 0) m_ui.canvas().fillRect(in.x, y, in.w, rowH - 4, rgba(120, 80, 40, 22)); // Zebrastreifen
            drawModelIcon(s.model, in.x + 40, y + 34, 66);                     // Schiffsmodell
            m_ui.text(in.x + 86, y + 4, s.name + (mine ? "  (dein Schiff)" : ""), mine ? rgba(150, 100, 20) : UiColor::INK, 2); // Name
            m_ui.text(in.x + 86, y + 28, "Laderaum " + std::to_string(s.holdWidth) + "x" + std::to_string(s.holdHeight) + "  Tempo " + f1(s.speed) + "  Rumpf " + std::to_string(static_cast<int>(s.hull)) + "  Abwehr " + std::to_string(static_cast<int>(s.defense)) + "  Crew " + std::to_string(s.minCrew) + "-" + std::to_string(s.maxCrew) + "  Tiefgang " + f1(s.draft) + " m  " + (s.drive == "wind" ? "Segel" : (s.drive == "motor" ? "Dampf" : "Hybrid")), UiColor::INK_LIGHT, 1); // Werte
            m_ui.text(in.x + 86, y + 46, fit(s.description, in.w - 380, 1), UiColor::INK_LIGHT, 1); // Beschreibung
            int net = s.price - value;                                         // Zuzahlung
            if (!mine && m_ui.button(RectI{in.x + in.w - 280, y + 12, 270, 44}, "Kaufen " + cr(std::max(0, net)), m_state.credits >= net)) { // Kaufen
                if (!m_state.hold.empty()) { m_state.unloadAll(isl); notify("Die Ladung wurde an den Steg gebracht."); } // Laderaum leeren
                m_state.credits -= std::max(0, net);                           // Bezahlen
                if (net < 0) m_state.credits -= net;                           // Überschuss auszahlen
                m_state.shipId = s.id; m_state.upgrades.clear(); m_state.upgradeSpent = 0; // Neues Schiff
                ShipStats ns = m_state.stats();                                // Neue Werte
                m_state.hull = ns.hullMax; m_state.fuel = ns.fuelMax;          // Heil und vollgetankt
                while (static_cast<int>(m_state.crew.size()) > s.maxCrew) { notify(m_state.crew.back().name + " findet keine Koje und geht von Bord."); m_state.crew.pop_back(); } // Zu viele Leute
                sound("glocke"); notify("Glückwunsch zur neuen " + s.name + "!"); // Meldung
                Panel keep = m_panel;                                          // Fenster merken
                prewarm("Die " + s.name + " wird zu Wasser gelassen ...");     // Neues Schiff vorbereiten
                m_panel = keep;                                                // Fenster bleibt offen
            }                                                                  // Ende Kaufen
            y += rowH;                                                         // Nächste Zeile
        }                                                                      // Ende der Schiffe
    }                                                                          // Ende der Reiter
} // Ende von panelShipyard

// Taverne: Crew anheuern, übernachten, Gerüchte
void Game::panelTavern() {                                                     // Beginn von panelTavern
    int isl = m_world.buildings[static_cast<std::size_t>(m_panelBuilding)].island; // Insel
    RectI in = panelFrame(960, 600, "Taverne von " + islandName(isl));         // Rahmen
    tabs(in, {"Anheuern", "Deine Crew", "Übernachten & Gerüchte"}, m_tab);     // Reiter
    int y = in.y + 56;                                                         // Inhalt
    const ShipClass& cls = m_state.shipClass();                                // Schiffsklasse
    std::string crewInfo = "Crew " + std::to_string(m_state.crew.size()) + " / " + std::to_string(cls.maxCrew) + " Kojen, Mindestbesatzung " + std::to_string(cls.minCrew) + ", Heuer " + cr(m_state.dailyWages()) + " pro Tag"; // Übersicht
    m_ui.text(in.x, y, crewInfo, static_cast<int>(m_state.crew.size()) < cls.minCrew ? UiColor::RED : UiColor::INK_LIGHT, 2); // Anzeigen
    y += 36;                                                                   // Abstand
    if (m_tab == 0) {                                                          // Anheuern
        auto it = m_state.applicants.find(isl);                                // Bewerber
        if (it == m_state.applicants.end() || it->second.empty()) m_ui.text(in.x + 10, y, "Heute sucht niemand mehr Arbeit. Morgen früh kommen neue Bewerber.", UiColor::INK, 2); // Keine
        else for (std::size_t i = 0; i < it->second.size(); ++i) {             // Alle Bewerber
            const Applicant& a = it->second[i];                                // Bewerber
            const CrewRole* r = m_data.role(a.role);                           // Rolle
            if (!r) continue;                                                  // Unbekannt
            if (i % 2 == 0) m_ui.canvas().fillRect(in.x, y, in.w, 82, rgba(120, 80, 40, 22)); // Zebrastreifen
            m_ui.text(in.x + 10, y + 6, a.name + " - " + r->name + "  (" + cr(a.wage) + " pro Tag)", UiColor::INK, 2); // Name und Rolle
            m_ui.text(in.x + 10, y + 32, fit(r->description, in.w - 300, 1), UiColor::INK_LIGHT, 1); // Beschreibung
            m_ui.text(in.x + 10, y + 50, fit("Bonus: " + bonusText(*r), in.w - 300, 1), UiColor::GREEN, 1); // Bonus
            if (m_ui.button(RectI{in.x + in.w - 270, y + 16, 260, 44}, "Anheuern " + cr(a.wage), m_state.credits >= a.wage && static_cast<int>(m_state.crew.size()) < cls.maxCrew)) { // Anheuern
                std::string name = a.name;                                     // Name merken
                std::string err = m_state.hire(isl, i);                        // Anheuern
                if (err.empty()) { notify(name + " heuert an!"); sound("glocke"); } else notify(err); // Ergebnis
                break;                                                         // Liste hat sich geändert
            }                                                                  // Ende Anheuern
            y += 90;                                                           // Nächste Zeile
        }                                                                      // Ende der Bewerber
        m_ui.text(in.x, in.y + in.h - 20, "Das Handgeld beträgt einen Tageslohn. Die Heuer wird jeden Morgen um 6 Uhr bezahlt.", UiColor::INK_LIGHT, 1); // Hinweis
    } else if (m_tab == 1) {                                                   // Eigene Crew
        if (m_state.crew.empty()) m_ui.text(in.x + 10, y, "Niemand an Bord. Ohne Mindestbesatzung ist das Schiff langsam!", UiColor::RED, 2); // Leer
        for (std::size_t i = 0; i < m_state.crew.size(); ++i) {                // Alle Mitglieder
            const CrewMember& m = m_state.crew[i];                             // Mitglied
            const CrewRole* r = m_data.role(m.role);                           // Rolle
            m_ui.text(in.x + 10, y + 6, m.name + " - " + (r ? r->name : m.role) + "  (" + cr(m.wage) + ")", UiColor::INK, 2); // Name
            if (r) m_ui.text(in.x + 10, y + 30, fit(bonusText(*r), in.w - 260, 1), UiColor::GREEN, 1); // Bonus
            if (m_ui.button(RectI{in.x + in.w - 200, y + 6, 190, 40}, "Entlassen")) { notify(m.name + " geht von Bord."); m_state.fire(i); break; } // Entlassen
            y += 54;                                                           // Nächste Zeile
        }                                                                      // Ende der Crew
    } else {                                                                   // Übernachten und Gerüchte
        if (m_ui.button(RectI{in.x, y, 420, 46}, "Übernachten bis " + std::to_string(static_cast<int>(m_set.wakeHour)) + " Uhr (" + cr(m_set.restPrice) + ")", m_state.credits >= m_set.restPrice)) { // Schlafen
            m_state.credits -= m_set.restPrice;                                // Bezahlen
            float h = m_state.hourOfDay();                                     // Uhrzeit
            float sleep = std::fmod(m_set.wakeHour - h + 48.0f, 24.0f);        // Stunden bis zum Aufwachen
            if (sleep < 1.0f) sleep += 24.0f;                                  // Mindestens eine Nacht
            m_state.advance(sleep);                                            // Zeit vergeht
            m_state.health = 100.0f;                                           // Ausgeschlafen
            notify("Gut geschlafen! Es ist " + m_state.clockText() + ".");     // Meldung
        }                                                                      // Ende Schlafen
        if (m_ui.button(RectI{in.x + 440, y, 260, 46}, "Hafenzeitung lesen")) { openPanel(Panel::Newspaper); return; } // Zeitung
        y += 70;                                                               // Abstand
        m_ui.text(in.x, y, "Am Tresen erzählt man sich:", UiColor::INK_LIGHT, 2); // Überschrift
        y += 30;                                                               // Nächste Zeile
        std::vector<std::string> rumors;                                       // Gerüchte
        unsigned seedDay = static_cast<unsigned>(m_state.day()) * 7u + static_cast<unsigned>(isl); // Wechselt täglich
        for (int k = 0; k < 3 && !m_data.goods.empty(); ++k) {                 // Drei Handelstipps
            const GoodDef& g = m_data.goods[(seedDay * 13u + static_cast<unsigned>(k) * 5u) % m_data.goods.size()]; // Ware
            int bestBuy = -1, bestSell = -1;                                   // Beste Inseln
            for (std::size_t i = 0; i < m_world.islands.size(); ++i) {         // Alle Inseln
                if (m_state.kontorSells(static_cast<int>(i), g.id) && (bestBuy < 0 || m_state.kontorBuyPrice(static_cast<int>(i), g.id) < m_state.kontorBuyPrice(bestBuy, g.id))) bestBuy = static_cast<int>(i); // Günstigster Einkauf
                if (bestSell < 0 || m_state.kontorSellPrice(static_cast<int>(i), g.id) > m_state.kontorSellPrice(bestSell, g.id)) bestSell = static_cast<int>(i); // Bester Verkauf
            }                                                                  // Ende der Inseln
            if (bestBuy >= 0 && bestSell >= 0 && bestBuy != bestSell) rumors.push_back(g.name + " gibt es billig in " + islandName(bestBuy) + " (" + cr(m_state.kontorBuyPrice(bestBuy, g.id)) + "), in " + islandName(bestSell) + " zahlt das Kontor " + cr(m_state.kontorSellPrice(bestSell, g.id)) + "."); // Tipp
        }                                                                      // Ende der Tipps
        for (const Zone& z : m_world.zones) {                                  // Gefahren
            int nearest = 0; float best = 1e9f;                                // Nächste Insel
            for (std::size_t i = 0; i < m_world.islands.size(); ++i) { float d = std::sqrt((m_world.islands[i].cx - z.x) * (m_world.islands[i].cx - z.x) + (m_world.islands[i].cy - z.y) * (m_world.islands[i].cy - z.y)); if (d < best) { best = d; nearest = static_cast<int>(i); } } // Suchen
            if (z.kind == "piraten") rumors.push_back("Bei " + islandName(nearest) + " kreuzen Piraten. Auf den Bojen-Routen ist man sicherer."); // Piraten
            if (z.kind == "monster") rumors.push_back("Nachts soll in den Gewässern bei " + islandName(nearest) + " ein Seeungeheuer jagen. Nur schnelle Schiffe entkommen."); // Ungeheuer
        }                                                                      // Ende der Gefahren
        for (std::size_t k = 0; k < rumors.size() && k < 6; ++k) {            // Höchstens sechs Gerüchte
            y += m_ui.textWrapped(in.x + 10, y, in.w - 20, "- " + rumors[(k + seedDay) % rumors.size()], UiColor::INK, 2) + 6; // Gerücht
        }                                                                      // Ende der Gerüchte
    }                                                                          // Ende der Reiter
} // Ende von panelTavern

// Steg: Schiff beladen und ablegen
void Game::panelPier() {                                                       // Beginn von panelPier
    int isl = m_state.docked;                                                  // Hafen
    if (isl < 0) { m_panel = Panel::None; return; }                            // Kein Schiff
    RectI in = panelFrame(820, 560, "Steg von " + islandName(isl) + " - " + m_state.shipClass().name); // Rahmen
    ShipStats st = m_state.stats();                                            // Schiffswerte
    int y = in.y;                                                              // Zeile
    int pierUnits = static_cast<int>(m_state.pier(isl).size());                // Einheiten am Steg
    m_ui.text(in.x, y, "Laderaum: " + std::to_string(m_state.holdUsedCells()) + " / " + std::to_string(st.holdW * st.holdH) + " Felder belegt (" + std::to_string(m_state.hold.size()) + " Einheiten)", UiColor::INK, 2); y += 28; // Laderaum
    m_ui.text(in.x, y, "Am Steg: " + std::to_string(pierUnits) + " Einheiten", UiColor::INK, 2); y += 28; // Steglager
    int missing = st.missingCrew;                                              // Fehlende Crew
    m_ui.text(in.x, y, "Besatzung: " + std::to_string(m_state.crew.size()) + " (mindestens " + std::to_string(m_state.shipClass().minCrew) + ")", missing > 0 ? UiColor::RED : UiColor::INK, 2); y += 28; // Crew
    m_ui.text(in.x, y, "Rumpf: " + std::to_string(static_cast<int>(m_state.hull)) + " / " + std::to_string(static_cast<int>(st.hullMax)) + (st.fuelMax > 0.0f ? "    Kohle: " + std::to_string(static_cast<int>(m_state.fuel)) + " / " + std::to_string(static_cast<int>(st.fuelMax)) : ""), UiColor::INK, 2); y += 28; // Zustand
    float risk = m_state.capsizeRisk();                                        // Kenterrisiko
    m_ui.text(in.x, y, "Schlagseite " + f1(m_state.holdHeel()) + " Grad, Trimm " + f1(m_state.holdTrim()) + " Grad", risk >= 1.0f ? UiColor::RED : UiColor::INK, 2); // Balance
    m_ui.progress(RectI{in.x + 500, y - 2, 240, 22}, risk, risk >= 0.7f ? BarColor::Red : BarColor::Green); y += 34; // Risikobalken
    if (risk >= 1.0f) { m_ui.text(in.x, y, "Achtung: So beladen kentert das Schiff beim Ablegen!", UiColor::RED, 2); y += 28; } // Warnung
    if (missing > 0) { m_ui.text(in.x, y, "Unterbesetzt: langsamer und schwerfälliger. Crew gibt es in der Taverne.", UiColor::RED, 1); y += 20; } // Warnung
    y = in.y + 220;                                                            // Knöpfe
    if (m_ui.button(RectI{in.x, y, 360, 50}, "Schiff beladen (Minispiel)")) { openLoadShip(false); return; } // Minispiel
    int autoCost = pierUnits * m_set.loaderFee;                                // Kosten der Hafenarbeiter
    if (m_ui.button(RectI{in.x + 380, y, 370, 50}, "Hafenarbeiter laden (" + cr(autoCost) + ")", pierUnits > 0)) { m_state.autoLoad(isl); sound("kaufen"); } // Automatisch
    y += 64;                                                                   // Nächste Zeile
    if (m_ui.button(RectI{in.x, y, 360, 50}, "Alles ausladen", !m_state.hold.empty())) { m_state.unloadAll(isl); notify("Die Ladung liegt jetzt am Steg."); } // Ausladen
    if (m_ui.button(RectI{in.x + 380, y, 370, 50}, "Verdorbenes wegwerfen")) { int n = m_state.discardRotten(isl); notify(std::to_string(n) + " verdorbene Einheiten über Bord."); } // Wegwerfen
    y += 80;                                                                   // Nächste Zeile
    if (m_ui.button(RectI{in.x + in.w / 2 - 200, y, 400, 60}, "An Bord gehen und ablegen", true, risk < 1.0f)) { board(); return; } // Ablegen
    m_ui.text(in.x, in.y + in.h - 20, "Nur Ware im Laderaum fährt mit. Verkaufen kann man direkt vom Steg und aus dem Laderaum.", UiColor::INK_LIGHT, 1); // Hinweis
} // Ende von panelPier

// Seekarte mit Inseln, Routen, Gefahren, Artefakten und Ziel
void Game::panelChart() {                                                      // Beginn von panelChart
    RectI in = panelFrame(m_w - 40, m_h - 30, "Seekarte", PanelStyle::WoodPaper); // Rahmen
    Canvas& c = m_ui.canvas();                                                 // Zeichenfläche
    int listW = 230;                                                           // Breite der Inselliste
    int ox = in.x + (in.w - listW - m_chart.width) / 2;                        // Linke Kante der Karte
    int oy = in.y + std::max(0, (in.h - 30 - m_chart.height) / 2);             // Obere Kante der Karte
    c.blit(m_chart, ox, oy);                                                   // Karte zeichnen
    float tw = m_chartTile;                                                    // Kachelbreite der Karte
    float baseX = static_cast<float>(ox) + static_cast<float>(m_world.height()) * tw * 0.5f + 2.0f; // Kartenursprung x
    float baseY = static_cast<float>(oy) + 2.0f;                               // Kartenursprung y
    auto mapX = [&](float x, float y) { return static_cast<int>(baseX + Iso::screenX(x, y, tw)); }; // Welt -> Karte x
    auto mapY = [&](float x, float y) { return static_cast<int>(baseY + Iso::screenY(x, y, 0.0f, tw) + tw * 0.25f); }; // Welt -> Karte y
    for (const Route& r : m_world.routes) {                                    // Handelsrouten (gestrichelt)
        for (std::size_t i = 0; i + 1 < r.points.size(); ++i) {                // Alle Abschnitte
            float ax = r.points[i].first, ay = r.points[i].second, bx = r.points[i + 1].first, by = r.points[i + 1].second; // Endpunkte
            float len = std::sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay)); // Länge
            for (float s = 0.0f; s < len; s += 3.0f) {                         // Striche
                float t0 = s / len, t1 = std::min(1.0f, (s + 1.8f) / len);     // Strichanfang und -ende
                c.line(mapX(ax + (bx - ax) * t0, ay + (by - ay) * t0), mapY(ax + (bx - ax) * t0, ay + (by - ay) * t0), mapX(ax + (bx - ax) * t1, ay + (by - ay) * t1), mapY(ax + (bx - ax) * t1, ay + (by - ay) * t1), rgba(200, 140, 30), 2); // Strich
            }                                                                  // Ende der Striche
        }                                                                      // Ende der Abschnitte
    }                                                                          // Ende der Routen
    for (const Zone& z : m_world.zones) {                                      // Gefahrenzonen
        Color col = z.kind == "piraten" ? rgba(190, 40, 30) : (z.kind == "monster" ? rgba(120, 40, 150) : rgba(90, 80, 70)); // Farbe
        for (int k = 0; k < 48; k += 2) {                                      // Gestrichelter Kreis
            float a0 = static_cast<float>(k) / 48.0f * 6.283f, a1 = static_cast<float>(k + 1) / 48.0f * 6.283f; // Winkel
            c.line(mapX(z.x + std::cos(a0) * z.radius, z.y + std::sin(a0) * z.radius), mapY(z.x + std::cos(a0) * z.radius, z.y + std::sin(a0) * z.radius), mapX(z.x + std::cos(a1) * z.radius, z.y + std::sin(a1) * z.radius), mapY(z.x + std::cos(a1) * z.radius, z.y + std::sin(a1) * z.radius), col, 2); // Bogenstück
        }                                                                      // Ende des Kreises
        std::string label = z.kind == "piraten" ? "Piraten" : (z.kind == "monster" ? "Ungeheuer (nachts)" : "Riff"); // Beschriftung
        m_ui.text(mapX(z.x, z.y) - Font::textWidth(label, 1) / 2, mapY(z.x, z.y) - 4, label, col, 1); // Beschriftung
    }                                                                          // Ende der Zonen
    for (std::size_t i = 0; i < m_world.islands.size(); ++i) {                 // Inseln
        const Island& isl = m_world.islands[i];                                // Insel
        int hx = mapX(isl.dockX, isl.dockY), hy = mapY(isl.dockX, isl.dockY);  // Hafen auf der Karte
        c.fillCircle(hx, hy, 5, rgba(60, 40, 25)); c.fillCircle(hx, hy, 3, rgba(240, 220, 160)); // Hafensymbol
        int nx = mapX(isl.cx, isl.cy), ny = mapY(isl.cx, isl.cy);              // Inselmitte
        m_ui.textShadow(nx - Font::textWidth(isl.name, 2) / 2, ny - 10, isl.name, rgba(255, 245, 220), 2); // Name
    }                                                                          // Ende der Inseln
    for (const FloatingArtifact& a : m_state.floating) if (a.seen) m_ui.icon("minimapIcon_starYellow", mapX(a.x, a.y), mapY(a.x, a.y)); // Gesichtete Artefakte
    if (m_state.hasTarget) m_ui.icon("minimapIcon_jewelRed", mapX(m_state.targetX, m_state.targetY), mapY(m_state.targetX, m_state.targetY)); // Ziel
    int sx = mapX(m_state.shipX, m_state.shipY), sy = mapY(m_state.shipX, m_state.shipY); // Eigenes Schiff
    drawArrow(sx, sy, screenAngle(std::cos(m_state.shipAngle), std::sin(m_state.shipAngle)), 9, rgba(20, 60, 140)); // Kurs
    c.ring(sx, sy, 7 + static_cast<int>(std::sin(m_time * 5.0f) * 2.0f), 2, rgba(20, 60, 140)); // Pulsierender Ring
    RectI mapRect{ox, oy, m_chart.width, m_chart.height};                      // Kartenbereich
    if (m_ui.hovered(mapRect) && (m_input.clicked || m_input.rightClicked) && !m_input.consumed) { // Klick in die Karte
        if (m_input.rightClicked) { m_state.hasTarget = false; m_autopilot = false; notify("Ziel gelöscht."); } // Rechtsklick löscht
        else {                                                                 // Linksklick setzt das Ziel
            float a = (static_cast<float>(m_input.mouseX) - baseX) / (tw * 0.5f); // x - y
            float b = (static_cast<float>(m_input.mouseY) - baseY - tw * 0.25f) / (tw * 0.25f); // x + y
            float wx = (a + b) * 0.5f, wy = (b - a) * 0.5f;                    // Weltposition
            int hit = -1;                                                      // Angeklickte Insel
            for (std::size_t i = 0; i < m_world.islands.size(); ++i) { float dx = wx - m_world.islands[i].cx, dy = wy - m_world.islands[i].cy; if (dx * dx + dy * dy < m_world.islands[i].radius * m_world.islands[i].radius * 1.3f) hit = static_cast<int>(i); } // Treffer?
            if (hit >= 0) setTarget(m_world.islands[static_cast<std::size_t>(hit)].dockX, m_world.islands[static_cast<std::size_t>(hit)].dockY, "Hafen " + islandName(hit)); // Hafen als Ziel
            else setTarget(wx, wy, "Wegpunkt");                                // Freier Punkt
            notify("Ziel gesetzt: " + m_state.targetName);                     // Meldung
        }                                                                      // Ende Linksklick
        m_input.consumed = true;                                               // Verbraucht
    }                                                                          // Ende Klick
    int lx = in.x + in.w - listW, ly = in.y;                                   // Inselliste
    m_ui.text(lx, ly, "Häfen:", UiColor::INK_LIGHT, 2);                        // Überschrift
    ly += 28;                                                                  // Nächste Zeile
    for (std::size_t i = 0; i < m_world.islands.size(); ++i) {                 // Alle Inseln
        if (m_ui.button(RectI{lx, ly, listW - 10, 40}, m_world.islands[i].name)) { setTarget(m_world.islands[i].dockX, m_world.islands[i].dockY, "Hafen " + m_world.islands[i].name); notify("Ziel gesetzt: " + m_state.targetName); } // Ziel setzen
        ly += 46;                                                              // Nächste Zeile
    }                                                                          // Ende der Inseln
    if (m_ui.button(RectI{lx, ly + 8, listW - 10, 40}, "Ziel löschen", m_state.hasTarget)) { m_state.hasTarget = false; m_autopilot = false; } // Löschen
    for (const Contract& ct : m_state.active) {                                // Auftragsziele als Hinweis
        ly += 56;                                                              // Nächste Zeile
        if (ly > in.y + in.h - 60) break;                                      // Kein Platz
        m_ui.text(lx, ly, fit(std::to_string(ct.amount) + " " + goodName(ct.good), listW, 1), UiColor::INK, 1); // Auftrag
        m_ui.text(lx, ly + 12, fit("> " + islandName(ct.to) + " bis " + GameState::timeText(ct.deadline), listW, 1), UiColor::INK_LIGHT, 1); // Ziel
    }                                                                          // Ende der Aufträge
    m_ui.text(in.x, in.y + in.h - 16, "Linksklick: Ziel setzen (Insel = Hafen)   Rechtsklick: Ziel löschen   Gestrichelt: sichere Handelsrouten mit Bojen   Stern: gesichtetes Treibgut", UiColor::INK_LIGHT, 1); // Legende
} // Ende von panelChart

// Schiff, Ladung, Kajüte und Aufträge
void Game::panelShipInfo() {                                                   // Beginn von panelShipInfo
    RectI in = panelFrame(960, 600, "Dein Schiff: " + m_state.shipClass().name); // Rahmen
    tabs(in, {"Schiff & Crew", "Ladung", "Kajüte", "Aufträge"}, m_tab);        // Reiter
    int y = in.y + 56;                                                         // Inhalt
    ShipStats st = m_state.stats();                                            // Schiffswerte
    const ShipClass& cls = m_state.shipClass();                                // Klasse
    if (m_tab == 0) {                                                          // Schiff und Crew
        drawModelIcon(cls.model, in.x + 90, y + 80, 160);                      // Schiffsbild
        int x = in.x + 200;                                                    // Werte rechts daneben
        std::vector<std::pair<std::string, std::string>> rows = {              // Wertetabelle
            {"Tempo", f1(st.speed) + " sm/s"}, {"Wendigkeit", std::to_string(static_cast<int>(st.turn)) + " Grad/s"}, // Tempo und Drehen
            {"Rumpf", std::to_string(static_cast<int>(m_state.hull)) + " / " + std::to_string(static_cast<int>(st.hullMax))}, {"Panzerung", std::to_string(static_cast<int>(st.armor)) + " %"}, // Rumpf und Panzerung
            {"Abwehr", std::to_string(static_cast<int>(st.defense))}, {"Reparatur", f1(st.repair) + " / Std."}, // Abwehr und Reparatur
            {"Sichtweite", f1(st.scan) + " sm"}, {"Handel", std::to_string(static_cast<int>(st.trade)) + " %"}, // Scanner und Handel
            {"Laderaum", std::to_string(st.holdW) + " x " + std::to_string(st.holdH)}, {"Tiefgang", f1(st.draft) + " m"}, // Laderaum und Tiefgang
            {"Antrieb", cls.drive == "wind" ? "Segel" : (cls.drive == "motor" ? "Dampf" : "Hybrid")}, {"Stabilität", f1(st.stability) + " Grad"}}; // Antrieb und Stabilität
        if (st.fuelMax > 0.0f) rows.push_back({"Kohle", std::to_string(static_cast<int>(m_state.fuel)) + " / " + std::to_string(static_cast<int>(st.fuelMax)) + (st.engineer ? "" : " (!)")}); // Treibstoff
        for (std::size_t i = 0; i < rows.size(); ++i) {                        // Alle Werte in zwei Spalten
            int cx = x + static_cast<int>(i % 2) * 350, cy = y + static_cast<int>(i / 2) * 26; // Position
            m_ui.text(cx, cy, rows[i].first + ":", UiColor::INK_LIGHT, 2);     // Name
            m_ui.text(cx + 150, cy, rows[i].second, UiColor::INK, 2);          // Wert
        }                                                                      // Ende der Werte
        y += static_cast<int>((rows.size() + 1) / 2) * 26 + 20;                // Unter die Tabelle
        m_ui.text(in.x, y, "Crew (" + std::to_string(m_state.crew.size()) + ", mindestens " + std::to_string(cls.minCrew) + ", höchstens " + std::to_string(cls.maxCrew) + "):", st.missingCrew > 0 ? UiColor::RED : UiColor::INK_LIGHT, 2); // Crew
        y += 26;                                                               // Nächste Zeile
        for (const CrewMember& m : m_state.crew) {                             // Alle Mitglieder
            const CrewRole* r = m_data.role(m.role);                           // Rolle
            m_ui.text(in.x + 10, y, fit(m.name + " (" + (r ? r->name : m.role) + "): " + (r ? bonusText(*r) : ""), in.w - 20, 1), UiColor::INK, 1); // Eintrag
            y += 14;                                                           // Nächste Zeile
        }                                                                      // Ende der Crew
        if (st.missingCrew > 0) m_ui.text(in.x + 10, y + 4, "Unterbesetzt: -" + std::to_string(static_cast<int>(m_set.understaffedMalus * static_cast<float>(st.missingCrew))) + " % Tempo und Wendigkeit, Spezialisten haben Nachteile.", UiColor::RED, 1); // Warnung
    } else if (m_tab == 1) {                                                   // Ladung
        std::map<std::string, std::pair<int, float>> inHold;                   // Ware -> (Anzahl, geringste Frische)
        for (const Cargo& c : m_state.hold) { auto& e = inHold[c.good]; if (e.first == 0) e.second = 1.0f; ++e.first; e.second = std::min(e.second, m_state.freshness(c)); } // Zählen
        m_ui.text(in.x, y, "Im Laderaum (" + std::to_string(m_state.holdUsedCells()) + "/" + std::to_string(st.holdW * st.holdH) + " Felder):", UiColor::INK_LIGHT, 2); // Überschrift
        y += 30;                                                               // Nächste Zeile
        int col = 0;                                                           // Spalte
        for (const auto& kv : inHold) {                                        // Alle Waren
            int x = in.x + (col % 3) * 300, yy = y + (col / 3) * 48;           // Position
            drawGoodIcon(kv.first, x + 20, yy + 20, 34);                       // Symbol
            m_ui.text(x + 46, yy + 6, fit(goodName(kv.first), 160, 2) + " x" + std::to_string(kv.second.first), UiColor::INK, 2); // Name und Menge
            const GoodDef* g = m_data.good(kv.first);                          // Ware
            if (g && g->perishDays > 0) m_ui.text(x + 46, yy + 28, "Frische " + std::to_string(static_cast<int>(kv.second.second * 100.0f)) + " %", kv.second.second <= 0.0f ? UiColor::RED : UiColor::INK_LIGHT, 1); // Frische
            ++col;                                                             // Nächste Spalte
        }                                                                      // Ende der Waren
        if (inHold.empty()) m_ui.text(in.x + 10, y, "Leer.", UiColor::INK, 2); // Leer
        m_ui.text(in.x, in.y + in.h - 20, "Wert der gesamten Ladung (Grundpreise, inkl. Steglager): " + cr(m_state.cargoValue()), UiColor::INK_LIGHT, 1); // Wert
    } else if (m_tab == 2) {                                                   // Kajüte
        m_ui.text(in.x, y, "Artefakte an Bord (Verkauf im Museum):", UiColor::INK_LIGHT, 2); // Überschrift
        y += 30;                                                               // Nächste Zeile
        for (const std::string& id : m_state.artifacts) {                      // Alle Artefakte
            const ArtifactDef* a = m_data.artifact(id);                        // Definition
            if (!a) continue;                                                  // Unbekannt
            drawModelIcon(a->model, in.x + 24, y + 20, 38);                    // Symbol
            m_ui.text(in.x + 56, y + 8, a->name + " - " + cr(a->value) + ", " + std::to_string(a->points) + " Entdeckerpunkte", UiColor::INK, 2); // Eintrag
            y += 46;                                                           // Nächste Zeile
            if (y > in.y + in.h - 40) break;                                   // Kein Platz
        }                                                                      // Ende der Artefakte
        if (m_state.artifacts.empty()) m_ui.text(in.x + 10, y, "Keine.", UiColor::INK, 2); // Leer
    } else {                                                                   // Aufträge
        if (m_state.active.empty()) m_ui.text(in.x + 10, y, "Keine Aufträge. Angebote gibt es in jedem Kontor.", UiColor::INK, 2); // Leer
        for (const Contract& c : m_state.active) {                             // Alle Aufträge
            drawGoodIcon(c.good, in.x + 22, y + 22, 34);                       // Symbol
            m_ui.text(in.x + 50, y + 4, std::to_string(c.amount) + " " + goodName(c.good) + " nach " + islandName(c.to) + " - Lohn " + cr(c.reward), UiColor::INK, 2); // Auftrag
            m_ui.text(in.x + 50, y + 26, "Frist " + GameState::timeText(c.deadline) + " (noch " + std::to_string(static_cast<int>(c.deadline - m_state.hours)) + " Std.)", c.deadline - m_state.hours < 4.0f ? UiColor::RED : UiColor::INK_LIGHT, 1); // Frist
            const Island& t = m_world.islands[static_cast<std::size_t>(c.to)]; // Zielinsel
            if (m_ui.button(RectI{in.x + in.w - 200, y + 4, 190, 40}, "Als Ziel")) { setTarget(t.dockX, t.dockY, "Hafen " + t.name); notify("Ziel gesetzt: " + t.name); } // Ziel setzen
            y += 54;                                                           // Nächste Zeile
        }                                                                      // Ende der Aufträge
    }                                                                          // Ende der Reiter
} // Ende von panelShipInfo

// Pause
void Game::panelPause() {                                                      // Beginn von panelPause
    RectI in = panelFrame(460, 500, "Pause", PanelStyle::Wood);                // Rahmen
    int y = in.y + 4;                                                          // Erste Zeile
    m_ui.textShadow(in.x, y, "Punkte: " + thousands(m_state.score()) + "   (" + m_state.clockText() + ")", UiColor::GOLD, 2); // Punktestand
    y += 40;                                                                   // Abstand
    if (m_ui.button(RectI{in.x + 20, y, in.w - 40, 52}, "Weiterspielen")) m_panel = Panel::None; // Weiter
    y += 64;                                                                   // Nächste Zeile
    if (m_ui.button(RectI{in.x + 20, y, in.w - 40, 52}, "Speichern")) { saveGame(); notify("Spiel gespeichert."); m_panel = Panel::None; } // Speichern
    y += 64;                                                                   // Nächste Zeile
    if (m_ui.button(RectI{in.x + 20, y, in.w - 40, 52}, "Steuerung")) openPanel(Panel::Help); // Hilfe
    y += 64;                                                                   // Nächste Zeile
    if (m_ui.button(RectI{in.x + 20, y, in.w - 40, 52}, "Hauptmenü")) { leaveToMenu(); return; } // Menü
    y += 64;                                                                   // Nächste Zeile
    if (m_ui.button(RectI{in.x + 20, y, in.w - 40, 52}, "Spiel beenden")) { saveGame(); recordScore(); m_running = false; } // Beenden
} // Ende von panelPause

// Hafenzeitung: Wetterbericht, Marktbericht, Gefahren
void Game::panelNewspaper() {                                                  // Beginn von panelNewspaper
    RectI in = panelFrame(1000, 620, "Hafenzeitung - Tag " + std::to_string(m_state.day())); // Rahmen
    int colW = in.w / 2 - 10;                                                  // Spaltenbreite
    int y = in.y;                                                              // Zeile
    m_ui.text(in.x, y, "Wetterbericht", rgba(150, 70, 30), 2);                 // Überschrift
    m_ui.text(in.x + 190, y + 4, m_state.hasRole("Navigator") ? "(Navigator an Bord: genau)" : "(ohne Navigator: ungenau)", UiColor::INK_LIGHT, 1); // Genauigkeit
    y += 30;                                                                   // Nächste Zeile
    float startH = std::floor(m_state.hours / m_set.weatherSlot) * m_set.weatherSlot; // Beginn des aktuellen Abschnitts
    for (int k = 0; k < 8; ++k) {                                              // 24 Stunden in Abschnitten
        float h = startH + static_cast<float>(k) * m_set.weatherSlot + 0.5f;   // Zeitpunkt
        WeatherSlot w = m_state.forecast(h);                                   // Vorhersage
        int hh = static_cast<int>(std::fmod(h, 24.0f));                        // Stunde
        char buf[16]; std::snprintf(buf, sizeof(buf), "%02d Uhr", hh);         // Uhrzeit
        m_ui.text(in.x, y + 6, buf, UiColor::INK, 2);                          // Uhrzeit
        drawArrow(in.x + 110, y + 14, screenAngle(std::cos(w.windAngle), std::sin(w.windAngle)), 9 + static_cast<int>(w.windStrength * 8.0f), w.windStrength >= m_set.stormFrom ? UiColor::RED : rgba(40, 90, 160)); // Windpfeil
        m_ui.text(in.x + 140, y + 6, fit(GameState::weatherName(w.type) + ", " + GameState::windName(w.windStrength), colW - 140, 2), w.type == WeatherType::Storm ? UiColor::RED : UiColor::INK, 2); // Wetter
        y += 34;                                                               // Nächste Zeile
    }                                                                          // Ende der Vorhersage
    m_ui.textWrapped(in.x, y + 6, colW, "Segelschiffe sind bei Rückenwind und halbem Wind am schnellsten. Gegen den Wind kommen sie kaum voran. Bei Sturm die Segel reffen!", UiColor::INK_LIGHT, 1); // Tipp
    int x = in.x + colW + 20;                                                  // Rechte Spalte
    y = in.y;                                                                  // Oben
    m_ui.text(x, y, "Marktbericht", rgba(150, 70, 30), 2);                     // Überschrift
    y += 30;                                                                   // Nächste Zeile
    for (const Island& isl : m_world.islands) {                                // Alle Inseln
        std::string wanted, cheap;                                             // Gesuchte und billige Waren
        for (const std::string& g : isl.demands) wanted += (wanted.empty() ? "" : ", ") + goodName(g); // Gesucht
        for (const std::string& g : isl.produces) cheap += (cheap.empty() ? "" : ", ") + goodName(g); // Billig
        m_ui.text(x, y, isl.name, UiColor::INK, 2);                            // Inselname
        y += 22;                                                               // Nächste Zeile
        y += m_ui.textWrapped(x + 10, y, colW - 10, "gesucht: " + wanted, UiColor::GREEN, 1); // Gesucht
        y += m_ui.textWrapped(x + 10, y, colW - 10, "billig: " + cheap, UiColor::INK_LIGHT, 1) + 8; // Billig
    }                                                                          // Ende der Inseln
    m_ui.textWrapped(x, y + 4, colW, "Warnung: Piraten meiden die Bojen-Routen meist. Nachts jagen in einigen Gewässern Seeungeheuer.", UiColor::RED, 1); // Gefahren
} // Ende von panelNewspaper

// Steuerung und Spielziel
void Game::panelHelp() {                                                       // Beginn von panelHelp
    RectI in = panelFrame(940, 620, "Willkommen, Kapitän!");                   // Rahmen
    std::vector<std::string> lines = {                                         // Hilfetext
        "Ziel: Kaufe Waren günstig, bringe sie auf andere Inseln und verkaufe sie mit Gewinn. Mit dem Geld verbesserst du dein Schiff, kaufst größere Schiffe und heuerst Crew an.", // Ziel
        "Zu Fuß: " + keyName("hoch") + keyName("links") + keyName("runter") + keyName("rechts") + " laufen oder Linksklick (läuft hin, Gebäude werden betreten). " + keyName("aktion") + " = Tür öffnen / zum Schiff.", // Figur
        "Auf See: " + keyName("hoch") + "/" + keyName("runter") + " = Fahrstufe (Segel/Maschine, ganz unten rückwärts), " + keyName("links") + "/" + keyName("rechts") + " = Ruder, Linksklick = Kurs setzen (Autopilot), " + keyName("autopilot") + " = Autopilot an/aus, " + keyName("kanone") + " = Kanonen, " + keyName("maschine") + " = Hilfsmaschine.", // Schiff
        "Anlegen geht nur am Hafensteg (" + keyName("aktion") + " in der Nähe des Liegeplatzes). Nur über den Steg kommst du an Land und zurück an Bord.", // Hafen
        "Gekaufte Ware liegt am Steg. Am Stegende belädst du das Schiff im Minispiel: Formen und Gewichte geschickt verteilen, sonst kentert das Schiff!", // Laden
        "Händler zahlen mehr als das Kontor, aber nur begrenzt. Hersteller kaufen Rohwaren und verkaufen Produkte. Verderbliche Ware frisch verkaufen!", // Handel
        "Wind, Wetter und Tageszeit zählen: Gegenwind bremst Segler, Sturm beschädigt volle Segel, nachts jagen Seeungeheuer. Piraten lauern abseits der Bojen-Routen.", // Gefahren
        "Treibgut (Artefakte) bringt im Museum Geld und Entdeckerpunkte. " + keyName("seekarte") + " = Seekarte, " + keyName("schiffsinfo") + " = Schiff, " + keyName("zeitung") + " = Zeitung, Mausrad = Zoom, " + keyName("pause") + " = Menü."}; // Sonstiges
    int y = in.y;                                                              // Zeile
    for (const std::string& l : lines) y += m_ui.textWrapped(in.x, y, in.w, l, UiColor::INK, 2) + 10; // Alle Absätze
    if (m_ui.button(RectI{in.x + in.w / 2 - 140, in.y + in.h - 52, 280, 50}, "Los geht's!")) m_panel = Panel::None; // Schließen
} // Ende von panelHelp

// Schiff verloren
void Game::panelSunk() {                                                       // Beginn von panelSunk
    RectI in = panelFrame(700, 360, "Schiff verloren!");                       // Rahmen
    m_ui.textWrapped(in.x, in.y, in.w, m_sunkText, UiColor::INK, 2);           // Text
    if (m_ui.button(RectI{in.x + in.w / 2 - 120, in.y + in.h - 52, 240, 50}, "Weiter")) m_panel = Panel::None; // Schließen
} // Ende von panelSunk
