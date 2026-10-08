// LoadShip.cpp - Minispiel "Load the Ship": Waren vom Steg in den Laderaum packen
//
// Jede Ware hat eine Form (1x1, 2x1, L ...) und ein Gewicht. Ziel: möglichst viel laden und das Gewicht
// gleichmäßig verteilen. Zu viel Gewicht auf einer Seite gibt Schlagseite - wird die Grenze überschritten,
// kentert das Schiff beim Ablegen und die Ladung geht über Bord.
#include "Game.h" // Eigene Deklarationen

#include <algorithm> // std::min, std::max
#include <cmath>     // std::floor, std::sin, std::cos
#include <map>       // std::map

#include "Font.h" // Textbreite

namespace { // Interne Hilfen

// Farbe einer Ladeeinheit nach Kategorie
Color categoryColor(GoodCategory c) {                                        // Beginn von categoryColor
    switch (c) {                                                             // Je nach Kategorie
    case GoodCategory::Raw: return rgba(170, 125, 80);                       // Rohware: Holzbraun
    case GoodCategory::Finished: return rgba(185, 95, 70);                   // Fertigware: Rotbraun
    case GoodCategory::Vegetable: return rgba(110, 165, 80);                 // Gemüse: Grün
    case GoodCategory::Fruit: return rgba(225, 150, 60);                     // Obst: Orange
    case GoodCategory::Food: return rgba(90, 140, 190);                      // Lebensmittel: Blau
    case GoodCategory::Luxury: return rgba(150, 100, 170);                   // Luxus: Violett
    }                                                                        // Ende der Fallunterscheidung
    return rgba(150, 150, 150);                                              // Ersatz
} // Ende von categoryColor

} // Ende des internen Namensraums

// Pixel je Laderaumfeld
int Game::holdCellSize() const {                                             // Beginn von holdCellSize
    ShipStats st = m_state.stats();                                          // Laderaumgröße
    int byW = (m_w - 330 - 360 - 120) / std::max(1, st.holdW);               // Platz in der Breite
    int byH = (m_h - 300) / std::max(1, st.holdH);                           // Platz in der Höhe
    return clampValue(std::min(byW, byH), 28, 90);                           // Begrenzen
} // Ende von holdCellSize

// Bildschirmbereich des Laderaums
RectI Game::holdRect() const {                                               // Beginn von holdRect
    ShipStats st = m_state.stats();                                          // Laderaumgröße
    int cs = holdCellSize();                                                 // Feldgröße
    int cx = 330 + (m_w - 330 - 360) / 2;                                    // Mitte zwischen den Seitenleisten
    return RectI{cx - st.holdW * cs / 2 - 20, 170 + (m_h - 330 - st.holdH * cs) / 2, st.holdW * cs, st.holdH * cs}; // Rechteck (etwas nach links wegen des Bugs)
} // Ende von holdRect

// Öffnet das Minispiel
void Game::openLoadShip(bool departAfter) {                                  // Beginn von openLoadShip
    m_screen = Screen::LoadShip;                                             // Bildschirm wechseln
    m_panel = Panel::None;                                                   // Kein Fenster
    m_lsDepart = departAfter;                                                // Danach ablegen?
    m_lsHolding = false;                                                     // Nichts in der Hand
    m_lsRot = 0;                                                             // Keine Drehung
    m_lsCapsize = -1.0f;                                                     // Kein Kentern
    m_lsHeelShown = m_state.holdHeel();                                      // Anzeige startet beim echten Wert
    m_scroll = 0;                                                            // Liste oben
    m_lsMessage = "Klicke eine Ware am Steg an und lege sie in den Laderaum. R oder Rechtsklick dreht."; // Hinweis
} // Ende von openLoadShip

// Schreibt das Minispiel fort (Anzeige der Schlagseite, Kenteranimation)
void Game::updateLoadShip(float dt) {                                        // Beginn von updateLoadShip
    float target = m_state.holdHeel();                                       // Echte Schlagseite
    if (m_lsCapsize >= 0.0f) {                                               // Kentert gerade
        m_lsCapsize += dt;                                                   // Animation läuft
        target = (m_state.holdHeel() >= 0.0f ? 1.0f : -1.0f) * std::min(100.0f, 20.0f + m_lsCapsize * 45.0f); // Kippt immer weiter
        if (m_lsCapsize > 3.2f) {                                            // Animation fertig
            int lost = static_cast<int>(m_state.hold.size());                // Verlorene Ladung
            m_state.hold.clear();                                            // Alles über Bord
            ShipStats st = m_state.stats();                                  // Schiffswerte
            m_state.hull = std::max(1.0f, m_state.hull - st.hullMax * m_set.capsizeDamage); // Schaden am Rumpf
            notify("Gekentert! " + std::to_string(lost) + " Ladungseinheiten sind über Bord gegangen. Hafenarbeiter haben das Schiff wieder aufgerichtet."); // Meldung
            m_lsCapsize = -1.0f;                                             // Animation aus
            m_screen = Screen::Playing;                                      // Zurück ins Spiel
            openPanel(Panel::Pier);                                          // Stegfenster
            return;                                                          // Fertig
        }                                                                    // Ende fertig
    }                                                                        // Ende Kentern
    m_lsHeelShown += (target - m_lsHeelShown) * std::min(1.0f, dt * 4.0f);   // Anzeige folgt weich
} // Ende von updateLoadShip

// Wertet einen Klick im Minispiel aus (nach den Knöpfen)
void Game::loadShipClick(int mx, int my, bool right) {                       // Beginn von loadShipClick
    if (m_lsCapsize >= 0.0f) return;                                         // Während des Kenterns nichts
    if (right) { if (m_lsHolding) m_lsRot = (m_lsRot + 1) % 4; return; }     // Rechtsklick dreht
    RectI hr = holdRect();                                                   // Laderaum
    int cs = holdCellSize();                                                 // Feldgröße
    RectI pierList{20, 80, 320, m_h - 110};                                  // Stegliste
    std::vector<Cargo>& pier = m_state.pier(m_state.docked);                 // Steglager
    if (m_lsHolding) {                                                       // Etwas in der Hand
        if (hr.contains(mx, my) || (mx > hr.x - cs && mx < hr.x + hr.w + cs && my > hr.y - cs && my < hr.y + hr.h + cs)) { // Über dem Laderaum
            std::vector<std::pair<int, int>> cells = m_state.cells(m_lsCargo, 0, 0, m_lsRot); // Form
            int sw = 1, sh = 1;                                              // Größe der Form
            for (const auto& c : cells) { sw = std::max(sw, c.first + 1); sh = std::max(sh, c.second + 1); } // Bestimmen
            int gx = static_cast<int>(std::floor(static_cast<float>(mx - hr.x) / static_cast<float>(cs) - static_cast<float>(sw) * 0.5f + 0.5f)); // Linke Spalte
            int gy = static_cast<int>(std::floor(static_cast<float>(my - hr.y) / static_cast<float>(cs) - static_cast<float>(sh) * 0.5f + 0.5f)); // Obere Reihe
            if (m_state.fits(m_lsCargo, gx, gy, m_lsRot, -1)) {              // Passt
                m_lsCargo.x = gx; m_lsCargo.y = gy; m_lsCargo.rot = m_lsRot; // Platzieren
                m_state.hold.push_back(m_lsCargo);                           // In den Laderaum
                m_lsHolding = false;                                         // Hand frei
                sound("klick");                                              // Geräusch
                m_lsMessage = m_state.capsizeRisk() >= 1.0f ? "Vorsicht! So würde das Schiff kentern." : (m_state.capsizeRisk() > 0.6f ? "Das Schiff bekommt Schlagseite - gegenüber Gewicht ausgleichen." : "Gut verstaut."); // Rückmeldung
            } else { m_lsMessage = "Passt dort nicht."; sound("fehler", 60); } // Passt nicht
        } else if (pierList.contains(mx, my)) {                              // Zurück an den Steg
            m_lsCargo.x = m_lsCargo.y = -1; m_lsCargo.rot = 0;               // Position löschen
            pier.push_back(m_lsCargo);                                       // An den Steg
            m_lsHolding = false;                                             // Hand frei
        }                                                                    // Ende der Ziele
        return;                                                              // Fertig
    }                                                                        // Ende etwas in der Hand
    if (hr.contains(mx, my)) {                                               // Klick in den Laderaum: Einheit aufnehmen
        int gx = (mx - hr.x) / cs, gy = (my - hr.y) / cs;                    // Feld
        for (std::size_t i = 0; i < m_state.hold.size(); ++i) {             // Alle Einheiten
            const Cargo& c = m_state.hold[i];                                // Einheit
            for (const auto& p : m_state.cells(c, c.x, c.y, c.rot)) {        // Ihre Felder
                if (p.first != gx || p.second != gy) continue;               // Nicht dieses Feld
                m_lsCargo = c; m_lsRot = c.rot; m_lsHolding = true;          // Aufnehmen
                m_state.hold.erase(m_state.hold.begin() + static_cast<long>(i)); // Aus dem Laderaum
                return;                                                      // Fertig
            }                                                                // Ende der Felder
        }                                                                    // Ende der Einheiten
        return;                                                              // Leeres Feld
    }                                                                        // Ende Laderaum
    if (pierList.contains(mx, my)) {                                         // Klick in die Stegliste
        std::vector<std::string> goods;                                      // Waren in Listenreihenfolge
        for (const Cargo& c : pier) if (std::find(goods.begin(), goods.end(), c.good) == goods.end()) goods.push_back(c.good); // Eindeutige Waren
        int row = (my - (pierList.y + 50)) / 54 + m_scroll;                  // Angeklickte Zeile
        if (row < 0 || row >= static_cast<int>(goods.size())) return;        // Keine Zeile
        int best = -1;                                                       // Beste Einheit (frisch zuerst nicht nötig: unverdorben zuerst)
        for (std::size_t i = 0; i < pier.size(); ++i) {                      // Alle Einheiten
            if (pier[i].good != goods[static_cast<std::size_t>(row)]) continue; // Andere Ware
            if (best < 0 || (m_state.rotten(pier[static_cast<std::size_t>(best)]) && !m_state.rotten(pier[i]))) best = static_cast<int>(i); // Unverdorbene bevorzugen
        }                                                                    // Ende der Einheiten
        if (best < 0) return;                                                // Nichts gefunden
        m_lsCargo = pier[static_cast<std::size_t>(best)];                    // Aufnehmen
        m_lsRot = 0; m_lsHolding = true;                                     // In der Hand
        pier.erase(pier.begin() + best);                                     // Vom Steg entfernen
    }                                                                        // Ende Stegliste
} // Ende von loadShipClick

// Zeichnet das Minispiel
void Game::renderLoadShip() {                                                // Beginn von renderLoadShip
    Canvas& c = m_ui.canvas();                                               // Zeichenfläche
    c.clear(rgba(40, 92, 128));                                              // Hafenwasser
    for (int y = 0; y < m_h; y += 24) c.fillRect(0, y + static_cast<int>(std::sin(m_time + static_cast<float>(y) * 0.05f) * 3.0f), m_w, 2, rgba(70, 130, 165)); // Wellenlinien
    ShipStats st = m_state.stats();                                          // Schiffswerte
    std::string title = "Schiff beladen - " + m_state.shipClass().name;      // Titel
    m_ui.textShadow(m_w / 2 - Font::textWidth(title, 3) / 2, 22, title, UiColor::GOLD, 3); // Titel
    RectI hr = holdRect();                                                   // Laderaum
    int cs = holdCellSize();                                                 // Feldgröße
    float capsizeT = m_lsCapsize >= 0.0f ? m_lsCapsize : 0.0f;               // Fortschritt des Kenterns
    int sinkOffset = static_cast<int>(capsizeT * capsizeT * 40.0f);          // Ladung rutscht beim Kentern
    std::vector<std::pair<float, float>> hull = {                            // Rumpf von oben (Bug rechts)
        {static_cast<float>(hr.x - 34), static_cast<float>(hr.y - 26)}, {static_cast<float>(hr.x + hr.w + 24), static_cast<float>(hr.y - 26)}, // Oben
        {static_cast<float>(hr.x + hr.w + 24 + hr.h / 2 + 40), static_cast<float>(hr.y + hr.h / 2)}, // Bugspitze
        {static_cast<float>(hr.x + hr.w + 24), static_cast<float>(hr.y + hr.h + 26)}, {static_cast<float>(hr.x - 34), static_cast<float>(hr.y + hr.h + 26)}}; // Unten
    c.fillPolygon(hull, rgba(120, 78, 45));                                  // Rumpf
    for (int y = hr.y - 20; y < hr.y + hr.h + 24; y += 12) c.hLine(hr.x - 30, hr.x + hr.w + 20, y, rgba(100, 64, 36)); // Planken
    c.fillRect(hr.x - 6, hr.y - 6, hr.w + 12, hr.h + 12, rgba(70, 45, 25));  // Lukenrand
    for (int gy = 0; gy < st.holdH; ++gy) for (int gx = 0; gx < st.holdW; ++gx) c.fillRect(hr.x + gx * cs + 1, hr.y + gy * cs + 1, cs - 2, cs - 2, rgba(52, 36, 22)); // Leere Felder
    m_ui.text(hr.x + hr.w / 2 - Font::textWidth("Backbord", 1) / 2, hr.y - 22, "Backbord", rgba(240, 220, 180), 1); // Links
    m_ui.text(hr.x + hr.w / 2 - Font::textWidth("Steuerbord", 1) / 2, hr.y + hr.h + 14, "Steuerbord", rgba(240, 220, 180), 1); // Rechts
    m_ui.text(hr.x + hr.w + 30, hr.y + hr.h / 2 - 4, "Bug", rgba(240, 220, 180), 1); // Vorne
    for (const Cargo& cg : m_state.hold) {                                   // Geladene Einheiten
        const GoodDef* g = m_data.good(cg.good);                             // Ware
        Color col = g ? categoryColor(g->category) : rgba(150, 150, 150);     // Farbe
        if (m_state.rotten(cg)) col = rgba(90, 90, 70);                      // Verdorben: grau
        int minX = 99, minY = 99, maxX = -1, maxY = -1;                      // Umriss
        for (const auto& p : m_state.cells(cg, cg.x, cg.y, cg.rot)) {        // Alle Felder
            c.fillRect(hr.x + p.first * cs + 3, hr.y + p.second * cs + 3 + sinkOffset, cs - 6, cs - 6, col); // Feld füllen
            c.drawRect(hr.x + p.first * cs + 3, hr.y + p.second * cs + 3 + sinkOffset, cs - 6, cs - 6, shade(col, 0.6f), 2); // Rand
            minX = std::min(minX, p.first); minY = std::min(minY, p.second); maxX = std::max(maxX, p.first); maxY = std::max(maxY, p.second); // Umriss erweitern
        }                                                                    // Ende der Felder
        int icx = hr.x + (minX + maxX + 1) * cs / 2, icy = hr.y + (minY + maxY + 1) * cs / 2 + sinkOffset; // Mitte
        drawGoodIcon(cg.good, icx, icy, cs * 3 / 4);                         // Symbol
        if (g) m_ui.textShadow(hr.x + minX * cs + 6, hr.y + minY * cs + 6 + sinkOffset, std::to_string(g->weight), UiColor::WHITE, 1); // Gewicht
    }                                                                        // Ende der Einheiten
    RectI pierList{20, 80, 320, m_h - 110};                                  // Stegliste
    m_ui.panel(pierList, PanelStyle::Paper);                                 // Hintergrund
    m_ui.text(pierList.x + 22, pierList.y + 18, "Am Steg", UiColor::INK, 2); // Überschrift
    std::vector<Cargo>& pier = m_state.pier(m_state.docked);                 // Steglager
    std::vector<std::string> goods;                                          // Eindeutige Waren
    std::map<std::string, std::pair<int, int>> counts;                       // Ware -> (gut, verdorben)
    for (const Cargo& cg : pier) { if (std::find(goods.begin(), goods.end(), cg.good) == goods.end()) goods.push_back(cg.good); auto& e = counts[cg.good]; if (m_state.rotten(cg)) ++e.second; else ++e.first; } // Zählen
    int visible = (pierList.h - 70) / 54;                                    // Sichtbare Zeilen
    if (pierList.contains(m_input.mouseX, m_input.mouseY)) ensureScroll(static_cast<int>(goods.size()), visible); // Rollen
    int y = pierList.y + 50;                                                 // Erste Zeile
    for (int i = m_scroll; i < static_cast<int>(goods.size()) && i < m_scroll + visible; ++i) { // Sichtbare Waren
        const std::string& g = goods[static_cast<std::size_t>(i)];           // Ware
        const GoodDef* gd = m_data.good(g);                                  // Definition
        RectI row{pierList.x + 14, y, pierList.w - 28, 50};                  // Zeile
        if (m_ui.hovered(row) && !m_lsHolding) c.fillRect(row.x, row.y, row.w, row.h, rgba(255, 230, 160, 90)); // Hervorheben
        drawGoodIcon(g, row.x + 24, row.y + 25, 40);                         // Symbol
        m_ui.text(row.x + 52, row.y + 6, gd ? gd->name : g, UiColor::INK, 2); // Name
        std::string info = "x" + std::to_string(counts[g].first);            // Anzahl
        if (counts[g].second > 0) info += "  (" + std::to_string(counts[g].second) + " verdorben)"; // Verdorbene
        if (gd) info += "  Gewicht " + std::to_string(gd->weight);           // Gewicht
        m_ui.text(row.x + 52, row.y + 30, info, UiColor::INK_LIGHT, 1);      // Infozeile
        y += 54;                                                             // Nächste Zeile
    }                                                                        // Ende der Waren
    if (goods.empty()) m_ui.textWrapped(pierList.x + 22, pierList.y + 56, pierList.w - 44, "Der Steg ist leer. Kaufe Waren im Kontor, bei Händlern oder Herstellern.", UiColor::INK, 2); // Leer
    RectI side{m_w - 350, 80, 330, m_h - 110};                               // Rechte Leiste
    m_ui.panel(side, PanelStyle::Paper);                                     // Hintergrund
    int sx = side.x + 22, sy = side.y + 18;                                  // Startpunkt
    int used = m_state.holdUsedCells(), total = st.holdW * st.holdH;         // Belegung
    int weight = 0;                                                          // Gesamtgewicht
    for (const Cargo& cg : m_state.hold) { const GoodDef* g = m_data.good(cg.good); weight += g ? g->weight : 1; } // Summieren
    m_ui.text(sx, sy, "Belegt: " + std::to_string(used) + " / " + std::to_string(total), UiColor::INK, 2); // Belegung
    m_ui.progress(RectI{sx, sy + 26, side.w - 44, 20}, static_cast<float>(used) / static_cast<float>(std::max(1, total)), BarColor::Blue); // Balken
    m_ui.text(sx, sy + 56, "Gewicht: " + std::to_string(weight), UiColor::INK, 2); // Gewicht
    int gx = side.x + side.w / 2, gy = sy + 160;                             // Mitte der Schiffsansicht von hinten
    float a = m_lsHeelShown * PI / 180.0f;                                   // Schlagseite in Radiant
    c.fillRect(side.x + 14, gy + 8, side.w - 28, 46, rgba(70, 130, 170));    // Wasser
    auto rot = [&](float x, float y) { return std::pair<float, float>{static_cast<float>(gx) + x * std::cos(a) - y * std::sin(a), static_cast<float>(gy) + x * std::sin(a) + y * std::cos(a)}; }; // Punkt drehen
    std::vector<std::pair<float, float>> section = {rot(-60, -10), rot(60, -10), rot(44, 30), rot(-44, 30)}; // Rumpfquerschnitt
    c.fillPolygon(section, rgba(120, 78, 45));                               // Rumpf
    std::pair<float, float> m0 = rot(0, -10), m1 = rot(0, -80);              // Mast
    c.lineF(m0.first, m0.second, m1.first, m1.second, rgba(80, 50, 30), 4.0f); // Mast zeichnen
    std::vector<std::pair<float, float>> sail = {rot(3, -76), rot(40, -30), rot(3, -24)}; // Segel
    c.fillPolygon(sail, rgba(240, 235, 220));                                // Segel zeichnen
    float risk = m_state.capsizeRisk();                                      // Kenterrisiko
    m_ui.text(sx, gy + 70, "Schlagseite " + std::to_string(static_cast<int>(std::fabs(m_state.holdHeel()) + 0.5f)) + " / " + std::to_string(static_cast<int>(st.stability)) + " Grad", risk >= 1.0f ? UiColor::RED : UiColor::INK, 2); // Seitliche Neigung
    m_ui.text(sx, gy + 96, "Trimm " + std::to_string(static_cast<int>(std::fabs(m_state.holdTrim()) + 0.5f)) + " / " + std::to_string(static_cast<int>(st.stability * 2.0f)) + " Grad", UiColor::INK, 2); // Längsneigung
    m_ui.text(sx, gy + 124, "Kenterrisiko", UiColor::INK_LIGHT, 2);          // Überschrift
    m_ui.progress(RectI{sx, gy + 148, side.w - 44, 22}, risk, risk >= 0.7f ? BarColor::Red : BarColor::Green); // Balken
    m_ui.textWrapped(sx, gy + 176, side.w - 44, m_lsMessage, risk >= 1.0f ? UiColor::RED : UiColor::INK, 1); // Rückmeldung
    int by = side.y + side.h - 222;                                          // Knöpfe
    int pierUnits = static_cast<int>(pier.size());                           // Einheiten am Steg
    bool idle = m_lsCapsize < 0.0f;                                          // Keine Animation
    if (m_ui.button(RectI{sx, by, side.w - 44, 42}, "Hafenarbeiter (" + std::to_string(pierUnits * m_set.loaderFee) + " Cr)", idle && pierUnits > 0 && !m_lsHolding)) { m_state.autoLoad(m_state.docked); sound("kaufen"); } // Automatisch beladen
    if (m_ui.button(RectI{sx, by + 50, side.w - 44, 42}, "Alles ausladen", idle && !m_state.hold.empty() && !m_lsHolding)) m_state.unloadAll(m_state.docked); // Ausladen
    if (m_ui.button(RectI{sx, by + 100, side.w - 44, 42}, "Fertig (zum Steg)", idle && !m_lsHolding)) { m_screen = Screen::Playing; openPanel(Panel::Pier); return; } // Zurück
    if (m_ui.button(RectI{sx, by + 150, side.w - 44, 52}, "Ablegen!", idle && !m_lsHolding, risk < 1.0f)) { // Ablegen
        if (risk >= 1.0f) { m_lsCapsize = 0.0f; m_lsMessage = "Das Schiff ist zu schief beladen und kentert!"; sound("kentern"); } // Kentert
        else { m_screen = Screen::Playing; board(); return; }                // Losfahren
    }                                                                        // Ende Ablegen
    if (m_lsHolding) {                                                       // Gehaltene Einheit an der Maus
        std::vector<std::pair<int, int>> cells = m_state.cells(m_lsCargo, 0, 0, m_lsRot); // Form
        int sw = 1, sh = 1;                                                  // Größe
        for (const auto& p : cells) { sw = std::max(sw, p.first + 1); sh = std::max(sh, p.second + 1); } // Bestimmen
        int mx = m_input.mouseX, my = m_input.mouseY;                        // Maus
        int gxCell = static_cast<int>(std::floor(static_cast<float>(mx - hr.x) / static_cast<float>(cs) - static_cast<float>(sw) * 0.5f + 0.5f)); // Spalte
        int gyCell = static_cast<int>(std::floor(static_cast<float>(my - hr.y) / static_cast<float>(cs) - static_cast<float>(sh) * 0.5f + 0.5f)); // Reihe
        bool over = mx > hr.x - cs && mx < hr.x + hr.w + cs && my > hr.y - cs && my < hr.y + hr.h + cs; // Über dem Laderaum?
        const GoodDef* g = m_data.good(m_lsCargo.good);                      // Ware
        Color col = g ? categoryColor(g->category) : rgba(150, 150, 150);     // Farbe
        if (over) {                                                          // Vorschau im Raster
            bool ok = m_state.fits(m_lsCargo, gxCell, gyCell, m_lsRot, -1);  // Passt?
            for (const auto& p : cells) c.fillRect(hr.x + (gxCell + p.first) * cs + 3, hr.y + (gyCell + p.second) * cs + 3, cs - 6, cs - 6, ok ? rgba(120, 220, 120, 150) : rgba(230, 80, 60, 150)); // Felder
            drawGoodIcon(m_lsCargo.good, hr.x + gxCell * cs + sw * cs / 2, hr.y + gyCell * cs + sh * cs / 2, cs * 3 / 4); // Symbol
        } else {                                                             // Frei an der Maus
            for (const auto& p : cells) c.fillRect(mx - sw * 20 + p.first * 40, my - sh * 20 + p.second * 40, 38, 38, withAlpha(col, 200)); // Kleine Felder
            drawGoodIcon(m_lsCargo.good, mx, my, 36);                        // Symbol
        }                                                                    // Ende Vorschau
    }                                                                        // Ende gehaltene Einheit
    m_ui.textShadow(m_w / 2 - Font::textWidth("Linksklick: nehmen / ablegen   R oder Rechtsklick: drehen   Klick auf den Steg: zurücklegen   Esc: zurück", 1) / 2, m_h - 18, "Linksklick: nehmen / ablegen   R oder Rechtsklick: drehen   Klick auf den Steg: zurücklegen   Esc: zurück", UiColor::WHITE, 1); // Hilfe
    if (m_lsCapsize >= 0.0f) {                                               // Kentern
        c.fillRect(0, m_h / 2 - 40, m_w, 80, rgba(120, 20, 10, 170));        // Banner
        m_ui.textShadow(m_w / 2 - Font::textWidth("Das Schiff kentert!", 4) / 2, m_h / 2 - 18, "Das Schiff kentert!", UiColor::WHITE, 4); // Text
    }                                                                        // Ende Kentern
    if ((m_input.clicked || m_input.rightClicked) && !m_input.consumed) { m_input.consumed = true; loadShipClick(m_input.mouseX, m_input.mouseY, m_input.rightClicked && !m_input.clicked); } // Klick auswerten
} // Ende von renderLoadShip
