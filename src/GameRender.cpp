// GameRender.cpp - Zeichnet die Welt: Boden, Modelle (nach Tiefe sortiert), Figuren, Schiffe, Effekte, Licht und Wetter
#include "Game.h" // Eigene Deklarationen

#include <algorithm> // std::sort
#include <cmath>     // std::sin, std::cos

#include "Rasterizer.h" // Iso-Projektion

namespace { // Interne Hilfen

// Ein zu zeichnendes Bild mit Tiefenwert
struct Drawable {                     // Beginn der Struktur
    float depth = 0.0f;               // Sortierschlüssel (weiter hinten = kleiner)
    const Sprite* sprite = nullptr;   // Bild
    int x = 0, y = 0;                 // Bildschirmposition des Ankers
    int alpha = 255;                  // Deckkraft
    int clipBottom = -1;              // Untere Schnittkante (für versinkende Dinge, -1 = keine)
}; // Ende der Struktur Drawable

// Weicher Lichtschein: mehrere Kreise, innen heller
void softGlow(Canvas& c, int x, int y, int r, int red, int green, int blue, int alpha) { // Beginn von softGlow
    const int rings = 6;                                                    // Anzahl der Ringe
    for (int k = rings; k >= 1; --k) c.fillEllipse(x, y, r * k / rings, r * k / rings * 3 / 4, rgba(red, green, blue, alpha / rings)); // Von außen nach innen
} // Ende von softGlow

// Richtung (0..7) einer Figur aus einem Weltwinkel
int figureDir(float angle) {                                                // Beginn von figureDir
    int d = static_cast<int>(std::lround(angle / (PI * 0.25f)));            // Auf 45 Grad runden
    return ((d % 8) + 8) % 8;                                               // In den Bereich 0..7
} // Ende von figureDir

} // Ende des internen Namensraums

// Zeichnet die Welt mit allen Objekten
void Game::renderWorld(Canvas& c) {                                         // Beginn von renderWorld
    m_world.drawTerrain(c, m_cam, m_time);                                  // Boden und Wasser
    const float tw = m_cam.tileWidth;                                       // Kachelbreite
    bool playing = m_screen == Screen::Playing;                             // Im Spiel?
    float margin = 3.0f * tw;                                               // Rand für große Modelle
    auto visible = [&](float sx, float sy) { return sx > -margin && sx < static_cast<float>(m_w) + margin && sy > -margin && sy < static_cast<float>(m_h) + margin * 1.5f; }; // Im Bild?
    std::vector<Drawable> list;                                             // Alle Bilder dieses Frames
    list.reserve(512);                                                      // Platz reservieren
    auto add = [&](const Sprite& s, float wx, float wy, float z, float depth, int alpha) { // Bild einreihen
        if (!s.valid()) return;                                             // Leeres Sprite
        float sx = m_cam.toScreenX(wx, wy), sy = m_cam.toScreenY(wx, wy, z); // Bildschirmposition
        if (!visible(sx, sy)) return;                                       // Außerhalb
        Drawable d;                                                         // Neuer Eintrag
        d.depth = depth; d.sprite = &s; d.x = static_cast<int>(sx); d.y = static_cast<int>(sy); d.alpha = alpha; // Werte
        list.push_back(d);                                                  // Speichern
    };                                                                      // Ende von add
    for (const WorldObject& o : m_world.objects) {                          // Feste Objekte
        float sx = m_cam.toScreenX(o.x, o.y), sy = m_cam.toScreenY(o.x, o.y, o.z); // Position
        if (!visible(sx, sy)) continue;                                     // Unsichtbar
        float z = o.z;                                                      // Höhe
        if (o.bob) z += std::sin(m_time * 2.0f + o.x) * 0.03f;              // Schaukeln im Wasser
        add(m_lib.sprite(o.model, o.angle, o.steps, tw), o.x, o.y, z, o.x + o.y, 255); // Einreihen
    }                                                                       // Ende der Objekte
    for (const Building& b : m_world.buildings) {                           // Leute vor den Türen
        std::string fig;                                                    // Figur
        if (b.type == "haendler") { const TraderDef* d = m_data.trader(b.ref); fig = d ? d->figure : "haendler"; } // Händler
        else if (b.type == "hersteller") { const ProducerDef* d = m_data.producer(b.ref); fig = d ? d->figure : "handwerker"; } // Handwerker
        else if (b.type == "taverne") fig = "matrose";                      // Wirt
        else if (b.type == "werft") fig = "handwerker";                     // Schiffsbauer
        else fig = (m_world.islands[static_cast<std::size_t>(b.island)].landscape == "grusel") ? "inselbewohner" : "beamter"; // Beamter oder Inselbewohner
        bool open = !playing || m_state.isOpen(b.type);                     // Nur bei geöffnetem Geschäft
        if (!open) continue;                                                // Geschlossen: niemand da
        float fx = b.doorX + (b.doorX - b.x) * 0.12f, fy = b.doorY + (b.doorY - b.y) * 0.12f; // Etwas vor der Tür
        if (b.type == "haendler") { fx = b.x - (b.doorX - b.x) * 0.25f; fy = b.y - (b.doorY - b.y) * 0.25f; } // Händler steht hinter seinem Stand
        float face = std::atan2(fy - b.y, fx - b.x);                        // Blick vom Haus weg
        if (b.type == "haendler") face = std::atan2(b.doorY - b.y, b.doorX - b.x); // Händler schaut zum Kunden
        add(m_lib.figure(fig, "stehen", figureDir(face), m_time + b.x, tw), fx, fy, LAND_HEIGHT, fx + fy, 255); // Einreihen
    }                                                                       // Ende der Leute
    if (playing) {                                                          // Spielobjekte
        for (const FloatingArtifact& a : m_state.floating) {                // Treibgut
            float d = std::sqrt((a.x - m_state.shipX) * (a.x - m_state.shipX) + (a.y - m_state.shipY) * (a.y - m_state.shipY)); // Abstand zum Schiff
            if (!a.seen && d > m_state.stats().scan) continue;              // Nur im Scanner sichtbar
            const ArtifactDef* def = m_data.artifact(a.id);                 // Definition
            if (!def) continue;                                             // Unbekannt
            add(m_lib.sprite(def->model, m_time * 0.5f, 8, tw), a.x, a.y, -0.05f + std::sin(m_time * 2.2f + a.x) * 0.04f, a.x + a.y, 255); // Schaukelndes Artefakt
        }                                                                   // Ende Treibgut
        {                                                                   // Eigenes Schiff (immer sichtbar)
            float bob = std::sin(m_time * 1.7f) * 0.025f;                   // Schaukeln
            add(m_lib.sprite(m_state.shipClass().model, m_state.shipAngle, m_shipSteps, tw), m_state.shipX, m_state.shipY, bob, m_state.shipX + m_state.shipY, 255); // Einreihen
        }                                                                   // Ende Schiff
        for (const Pirate& p : m_pirates) {                                 // Piraten
            float sink = p.state == 3 ? p.timer * 0.25f : 0.0f;             // Versinken
            add(m_lib.sprite("schiff_piraten", p.angle, m_shipSteps, tw), p.x, p.y, std::sin(m_time * 1.9f + p.x) * 0.025f - sink, p.x + p.y, p.state == 3 ? std::max(0, 255 - static_cast<int>(p.timer * 80.0f)) : 255); // Einreihen
        }                                                                   // Ende Piraten
        for (const Monster& m : m_monsters) {                               // Seeungeheuer
            add(m_lib.sprite("seeschlange", m.angle, 16, tw), m.x, m.y, -0.9f + m.emerge * 0.9f, m.x + m.y, static_cast<int>(clampValue(m.emerge, 0.0f, 1.0f) * 255.0f)); // Taucht auf und ab
        }                                                                   // Ende Ungeheuer
        if (m_state.onFoot) {                                               // Spielfigur
            add(m_lib.figure("spieler", m_figMoving ? "laufen" : "stehen", figureDir(m_figAngle), m_time, tw), m_state.figX, m_state.figY, LAND_HEIGHT, m_state.figX + m_state.figY + 0.05f, 255); // Einreihen
        }                                                                   // Ende Figur
        if (m_state.docked >= 0) {                                          // Kisten am Steg
            const Island& isl = m_world.islands[static_cast<std::size_t>(m_state.docked)]; // Hafen
            auto it = m_state.piers.find(m_state.docked);                   // Steglager
            if (it != m_state.piers.end() && !it->second.empty()) {         // Ware vorhanden
                int n = std::min<int>(6, static_cast<int>(it->second.size())); // Höchstens sechs Kisten zeigen
                float dx = isl.pierEndX - isl.pierBaseX, dy = isl.pierEndY - isl.pierBaseY; // Stegrichtung
                float len = std::max(0.1f, std::sqrt(dx * dx + dy * dy));   // Länge
                for (int k = 0; k < n; ++k) {                               // Alle Kisten
                    float t = 0.35f + 0.1f * static_cast<float>(k % 3);     // Position entlang des Stegs
                    float side = (k < 3 ? 0.28f : -0.28f);                  // Links oder rechts
                    float px = isl.pierBaseX + dx * t - dy / len * side, py = isl.pierBaseY + dy * t + dx / len * side; // Position
                    add(m_lib.sprite("kiste", 0.0f, 1, tw), px, py, LAND_HEIGHT, px + py, 255); // Kiste
                }                                                           // Ende der Kisten
            }                                                               // Ende Ware vorhanden
        }                                                                   // Ende Steg
    }                                                                       // Ende Spielobjekte
    std::sort(list.begin(), list.end(), [](const Drawable& a, const Drawable& b) { return a.depth < b.depth; }); // Von hinten nach vorne
    for (const Drawable& d : list) {                                        // Alle Bilder
        int x = d.x - d.sprite->anchorX, y = d.y - d.sprite->anchorY;       // Linke obere Ecke
        if (d.alpha >= 255) c.blit(d.sprite->image, x, y);                  // Deckend
        else if (d.alpha > 0) c.blitAlpha(d.sprite->image, x, y, d.alpha);  // Durchscheinend
    }                                                                       // Ende der Bilder
    if (!playing) return;                                                   // Im Menü keine Effekte
    if (m_state.onFoot) {                                                   // Figur hinter Gebäuden durchscheinen lassen
        const Sprite& f = m_lib.figure("spieler", m_figMoving ? "laufen" : "stehen", figureDir(m_figAngle), m_time, tw); // Gleiches Bild wie oben
        if (f.valid()) c.blitAlpha(f.image, static_cast<int>(m_cam.toScreenX(m_state.figX, m_state.figY)) - f.anchorX, static_cast<int>(m_cam.toScreenY(m_state.figX, m_state.figY, LAND_HEIGHT)) - f.anchorY, 70); // Schwach darüber zeichnen
    }                                                                       // Ende Durchscheinen
    for (const Shot& s : m_shots) {                                         // Kanonenkugeln
        float t = 1.0f - s.life / s.total;                                  // Flugfortschritt
        float z = 4.0f * t * (1.0f - t) * 0.9f + 0.3f;                      // Flugbogen
        int sx = static_cast<int>(m_cam.toScreenX(s.x, s.y)), sy = static_cast<int>(m_cam.toScreenY(s.x, s.y, z)); // Position
        c.fillEllipse(static_cast<int>(m_cam.toScreenX(s.x, s.y)), static_cast<int>(m_cam.toScreenY(s.x, s.y, 0.0f)), 4, 2, rgba(0, 0, 0, 70)); // Schatten
        c.fillCircle(sx, sy, std::max(2, static_cast<int>(tw / 30.0f)), rgba(30, 30, 35)); // Kugel
    }                                                                       // Ende der Kugeln
    for (const Particle& p : m_particles) {                                 // Partikel
        int sx = static_cast<int>(m_cam.toScreenX(p.x, p.y)), sy = static_cast<int>(m_cam.toScreenY(p.x, p.y, p.z * 0.2f)); // Position
        if (sx < -10 || sy < -10 || sx > m_w + 10 || sy > m_h + 10) continue; // Unsichtbar
        int a = static_cast<int>(static_cast<float>(alphaOf(p.color)) * clampValue(p.life / p.maxLife, 0.0f, 1.0f)); // Verblassen
        int r = std::max(1, static_cast<int>(p.size * tw / 96.0f));         // Größe mit dem Zoom
        c.fillCircle(sx, sy, r, withAlpha(p.color, a));                     // Zeichnen
    }                                                                       // Ende der Partikel
    if (m_state.hasTarget) {                                                // Zielmarkierung
        int sx = static_cast<int>(m_cam.toScreenX(m_state.targetX, m_state.targetY)), sy = static_cast<int>(m_cam.toScreenY(m_state.targetX, m_state.targetY, 0.0f)); // Position
        int r = static_cast<int>(tw * 0.3f + std::sin(m_time * 4.0f) * 3.0f); // Pulsierender Ring
        c.fillEllipse(sx, sy, r, r / 2, rgba(255, 220, 80, 60));           // Leuchtender Fleck
    }                                                                       // Ende Zielmarkierung
} // Ende von renderWorld

// Tageszeit: Abenddämmerung, Nacht und Lichter
void Game::renderLighting(Canvas& c) {                                      // Beginn von renderLighting
    float day = m_state.daylight();                                         // Helligkeit 0..1
    WeatherSlot w = m_state.weatherAt(m_state.hours);                       // Wetter
    float cloud = w.type == WeatherType::Storm ? 0.7f : (w.type == WeatherType::Rain ? 0.82f : (w.type == WeatherType::Cloudy ? 0.93f : 1.0f)); // Wolken dunkeln ab
    Color dayCol = rgba(static_cast<int>(255 * cloud), static_cast<int>(255 * cloud), static_cast<int>(255 * cloud)); // Tageslicht
    Color dusk = rgba(255, 170, 130);                                       // Abendrot
    Color night = rgba(70, 85, 140);                                        // Mondlicht
    Color tint = day >= 1.0f ? dayCol : (day > 0.5f ? mixColor(dusk, dayCol, (day - 0.5f) * 2.0f) : mixColor(night, dusk, day * 2.0f)); // Lichtfarbe
    if (tint != rgba(255, 255, 255)) c.multiply(tint);                      // Bild einfärben
    if (day < 0.6f) {                                                       // Lichter in der Dämmerung und nachts
        int glow = static_cast<int>((0.6f - day) / 0.6f * 120.0f);          // Stärke des Leuchtens
        const float tw = m_cam.tileWidth;                                   // Kachelbreite
        for (const WorldObject& o : m_world.objects) {                      // Laternen
            if (o.model != "laterne") continue;                             // Nur Laternen
            int sx = static_cast<int>(m_cam.toScreenX(o.x, o.y)), sy = static_cast<int>(m_cam.toScreenY(o.x, o.y, o.z + 1.0f)); // Lampenkopf
            if (sx < -100 || sy < -100 || sx > m_w + 100 || sy > m_h + 100) continue; // Unsichtbar
            int gy = static_cast<int>(m_cam.toScreenY(o.x, o.y, o.z));      // Boden unter der Laterne
            softGlow(c, sx, gy, static_cast<int>(tw * 1.1f), 255, 200, 120, glow);     // Lichtkreis am Boden
            softGlow(c, sx, sy, static_cast<int>(tw * 0.3f), 255, 230, 150, glow * 2); // Heller Kern an der Lampe
        }                                                                   // Ende Laternen
        for (const Building& b : m_world.buildings) {                       // Erleuchtete Fenster
            if (b.type == "haendler") continue;                             // Marktstände haben keine Fenster
            int sx = static_cast<int>(m_cam.toScreenX(b.x, b.y)), sy = static_cast<int>(m_cam.toScreenY(b.x, b.y, 0.9f)); // Hausmitte
            if (sx < -200 || sy < -200 || sx > m_w + 200 || sy > m_h + 200) continue; // Unsichtbar
            softGlow(c, sx, sy, static_cast<int>(tw * 1.0f), 255, 190, 100, glow / 2); // Warmer Schein aus den Fenstern
        }                                                                   // Ende Fenster
        if (m_screen == Screen::Playing) {                                  // Schiffslaterne
            int sx = static_cast<int>(m_cam.toScreenX(m_state.shipX, m_state.shipY)), sy = static_cast<int>(m_cam.toScreenY(m_state.shipX, m_state.shipY, 0.8f)); // Position
            softGlow(c, sx, sy, static_cast<int>(m_cam.tileWidth * 0.9f), 255, 210, 130, glow * 3 / 4); // Lichtschein der Schiffslaterne
            for (const Monster& m : m_monsters) {                           // Leuchtende Augen der Ungeheuer
                int mx = static_cast<int>(m_cam.toScreenX(m.x, m.y)), my = static_cast<int>(m_cam.toScreenY(m.x, m.y, m.emerge * 1.2f)); // Kopf
                c.fillCircle(mx, my, 4, rgba(255, 60, 40, static_cast<int>(200.0f * m.emerge))); // Rotes Glühen
            }                                                               // Ende Augen
        }                                                                   // Ende Schiffslaterne
    }                                                                       // Ende Lichter
} // Ende von renderLighting

// Regen, Nebel und Sturm
void Game::renderWeather(Canvas& c) {                                       // Beginn von renderWeather
    WeatherSlot w = m_state.weatherAt(m_state.hours);                       // Wetter
    if (w.type == WeatherType::Fog) c.fillRect(0, 0, m_w, m_h, rgba(220, 225, 230, 110)); // Nebelschleier
    if (w.type == WeatherType::Rain || w.type == WeatherType::Storm) {      // Regen
        int drops = w.type == WeatherType::Storm ? 420 : 220;               // Anzahl der Tropfen
        float slant = std::cos(w.windAngle) * 6.0f * w.windStrength;        // Schräglage durch den Wind
        for (int i = 0; i < drops; ++i) {                                   // Alle Tropfen
            unsigned h = static_cast<unsigned>(i) * 2654435761u;            // Fester Zufall je Tropfen
            float speed = 700.0f + static_cast<float>(h % 300);             // Fallgeschwindigkeit
            float x = static_cast<float>((h >> 8) % static_cast<unsigned>(m_w)) + m_time * slant * 40.0f; // Spalte
            float y = std::fmod(static_cast<float>((h >> 4) % 1000) + m_time * speed, static_cast<float>(m_h + 40)) - 20.0f; // Zeile (fällt nach unten)
            int xi = static_cast<int>(std::fmod(x, static_cast<float>(m_w)) + static_cast<float>(m_w)) % m_w; // Am Rand umklappen
            c.line(xi, static_cast<int>(y), xi + static_cast<int>(slant), static_cast<int>(y) + 12, rgba(200, 215, 235, 120)); // Strich
        }                                                                   // Ende der Tropfen
    }                                                                       // Ende Regen
    if (w.type == WeatherType::Storm && std::fmod(m_time, 9.0f) < 0.08f) c.fillRect(0, 0, m_w, m_h, rgba(255, 255, 255, 90)); // Blitz
    if (m_hitFlash > 0.0f) c.fillRect(0, 0, m_w, m_h, rgba(200, 30, 20, static_cast<int>(m_hitFlash * 200.0f))); // Roter Trefferblitz
} // Ende von renderWeather

// Zeichnet die Seekarte vorab (isometrisch wie die Spielwelt, Kachelbreite m_chartTile)
void Game::buildChart() {                                                   // Beginn von buildChart
    float tw = m_chartTile;                                                 // Kachelbreite der Karte
    int W = static_cast<int>(static_cast<float>(m_world.width() + m_world.height()) * tw * 0.5f) + 4; // Bildbreite
    int H = static_cast<int>(static_cast<float>(m_world.width() + m_world.height()) * tw * 0.25f) + 4; // Bildhöhe
    m_chart.resize(W, H, TRANSPARENT);                                      // Leeres Bild
    Canvas c(m_chart);                                                      // Zeichenfläche
    float ox = static_cast<float>(m_world.height()) * tw * 0.5f + 2.0f;     // Verschiebung, damit (0, hoehe) am linken Rand liegt
    for (int y = 0; y < m_world.height(); ++y) {                            // Zeilen
        for (int x = 0; x < m_world.width(); ++x) {                         // Spalten
            const Tile& t = m_world.tile(x, y);                             // Kachel
            Color col;                                                      // Farbe auf Papier
            switch (t.terrain) {                                            // Je nach Boden
            case Terrain::DeepWater: col = rgba(150, 190, 205); break;      // Tiefes Wasser
            case Terrain::Water: col = rgba(170, 205, 212); break;          // Wasser
            case Terrain::Shallow: col = rgba(200, 222, 210); break;        // Flachwasser
            case Terrain::Sand: col = rgba(222, 200, 150); break;           // Strand
            case Terrain::Pier: col = rgba(140, 95, 60); break;             // Steg
            case Terrain::Plaza: case Terrain::Path: col = rgba(190, 170, 140); break; // Platz und Weg
            default: col = rgba(160, 170, 110); break;                      // Land
            }                                                               // Ende der Fallunterscheidung
            if (t.blocked && !m_world.isLand(x, y)) col = rgba(110, 100, 90); // Felsen im Wasser
            float sx = ox + Iso::screenX(static_cast<float>(x), static_cast<float>(y), tw); // Obere Ecke x
            float sy = 2.0f + Iso::screenY(static_cast<float>(x), static_cast<float>(y), 0.0f, tw); // Obere Ecke y
            std::vector<std::pair<float, float>> d = {{sx, sy}, {sx + tw * 0.5f, sy + tw * 0.25f}, {sx, sy + tw * 0.5f}, {sx - tw * 0.5f, sy + tw * 0.25f}}; // Raute
            c.fillPolygon(d, col);                                          // Zeichnen
        }                                                                   // Ende der Spalten
    }                                                                       // Ende der Zeilen
} // Ende von buildChart
