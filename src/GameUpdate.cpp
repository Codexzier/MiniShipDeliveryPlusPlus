// GameUpdate.cpp - Bewegung von Figur und Schiff, Anlegen und Ablegen, Piraten, Seeungeheuer, Kanonen und Artefakte
#include "Game.h" // Eigene Deklarationen

#include <algorithm> // std::min, std::max, std::remove_if
#include <cmath>     // std::sin, std::cos, std::atan2, std::sqrt
#include <cstdio>    // std::snprintf

namespace { // Interne Hilfen

// Kürzester Winkelabstand von a nach b (-PI..PI)
float angleDiff(float a, float b) {                                          // Beginn von angleDiff
    float d = std::fmod(b - a + PI * 3.0f, PI * 2.0f);                        // Verschieben und begrenzen
    if (d < 0.0f) d += PI * 2.0f;                                            // Negativen Rest korrigieren
    return d - PI;                                                           // In den Bereich -PI..PI
} // Ende von angleDiff

// Abstand zweier Punkte
float dist(float ax, float ay, float bx, float by) { return std::sqrt((ax - bx) * (ax - bx) + (ay - by) * (ay - by)); } // Pythagoras

// Wirkung der Segel je nach Winkel zwischen Kurs und Windrichtung (0 = Wind von hinten)
float sailEfficiency(float a) {                                              // Beginn von sailEfficiency
    a = std::fabs(a);                                                        // Nur der Betrag zählt
    if (a <= PI * 0.5f) return lerp(0.75f, 1.0f, a / (PI * 0.5f));           // Vorm Wind bis halber Wind
    if (a <= PI * 0.75f) return lerp(1.0f, 0.55f, (a - PI * 0.5f) / (PI * 0.25f)); // Am Wind
    return lerp(0.55f, 0.1f, (a - PI * 0.75f) / (PI * 0.25f));               // Gegen den Wind kaum Fahrt
} // Ende von sailEfficiency

// Zahl mit einer Nachkommastelle als Text
std::string oneDecimal(float v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(v)); return buf; } // Formatieren

} // Ende des internen Namensraums

// Anzeigename eines Gebäudes
std::string Game::buildingTitle(int building) const {                        // Beginn von buildingTitle
    if (building < 0 || building >= static_cast<int>(m_world.buildings.size())) return "?"; // Ungültig
    const Building& b = m_world.buildings[static_cast<std::size_t>(building)]; // Gebäude
    if (b.type == "kontor") return "Kontor";                                 // Kontor
    if (b.type == "museum") return "Museum";                                 // Museum
    if (b.type == "werft") return "Werft";                                   // Werft
    if (b.type == "taverne") return "Taverne";                               // Taverne
    if (b.type == "haendler") { const TraderDef* d = m_data.trader(b.ref); return d ? d->name : "Händler"; } // Händler
    if (b.type == "hersteller") { const ProducerDef* d = m_data.producer(b.ref); return d ? d->name : "Werkstatt"; } // Hersteller
    return b.type;                                                           // Sonst die Art
} // Ende von buildingTitle

// Schreibt die Spielwelt fort
void Game::updatePlaying(float dt) {                                         // Beginn von updatePlaying
    m_state.advance(dt / m_set.secondsPerHour);                              // Spielzeit läuft
    m_autosave -= dt / m_set.secondsPerHour;                                 // Automatisch speichern ...
    if (m_autosave <= 0.0f) { m_autosave = 3.0f; saveGame(); }               // ... alle drei Spielstunden
    if (m_state.onFoot) updateFigure(dt); else updateShip(dt);               // Figur oder Schiff
    updatePirates(dt);                                                       // Piraten
    updateMonsters(dt);                                                      // Seeungeheuer
    updateShots(dt);                                                         // Kanonenkugeln
    updateParticles(dt);                                                     // Partikel
    updateArtifacts();                                                       // Artefakte
    updateCamera(dt);                                                        // Kamera
    m_hitFlash = std::max(0.0f, m_hitFlash - dt);                            // Trefferblitz verblasst
    m_prompt.clear();                                                        // Hinweis neu bestimmen
    if (m_state.onFoot) {                                                    // Zu Fuß
        int b = nearDoor();                                                  // Gebäude in der Nähe?
        if (b >= 0) {                                                        // Ja
            const Building& bd = m_world.buildings[static_cast<std::size_t>(b)]; // Gebäude
            std::string type = bd.type;                                      // Art (für die Öffnungszeiten)
            m_prompt = keyName("aktion") + ": " + buildingTitle(b) + " betreten" + (m_state.isOpen(type) ? "" : " (geschlossen)"); // Hinweis
        } else if (nearPier()) m_prompt = keyName("aktion") + ": Zum Schiff (beladen / ablegen)"; // Am Steg
    } else if (m_dockAnim <= 0.0f) {                                         // Auf dem Schiff
        int isl = nearDock();                                                // Hafen in der Nähe?
        if (isl >= 0) m_prompt = std::fabs(m_shipSpeed) > m_set.dockSpeed ? "Langsamer werden zum Anlegen (" + keyName("runter") + ")" : keyName("aktion") + ": Anlegen in " + islandName(isl); // Hinweis
        else {                                                               // Kein Hafen
            for (float a = 0.0f; a < 6.28f; a += 0.8f) if (m_world.isLand(static_cast<int>(m_state.shipX + std::cos(a) * 2.2f), static_cast<int>(m_state.shipY + std::sin(a) * 2.2f))) { m_prompt = "Anlegen nur am Hafensteg möglich"; break; } // Küste nah
        }                                                                    // Ende kein Hafen
    }                                                                        // Ende Schiff
} // Ende von updatePlaying

// Bewegt die Figur (Tasten oder Klickweg)
void Game::updateFigure(float dt) {                                          // Beginn von updateFigure
    float speed = m_set.walkSpeed * (m_state.health < m_set.injuredBelow ? 0.6f : 1.0f); // Lauftempo (verletzt langsamer)
    float mx = 0.0f, my = 0.0f;                                              // Richtung aus den Tasten
    if (keyHeld("hoch")) { mx -= 1.0f; my -= 1.0f; }                         // Bildschirm hoch = Nordwest in der Welt
    if (keyHeld("runter")) { mx += 1.0f; my += 1.0f; }                       // Bildschirm runter
    if (keyHeld("links")) { mx -= 1.0f; my += 1.0f; }                        // Bildschirm links
    if (keyHeld("rechts")) { mx += 1.0f; my -= 1.0f; }                       // Bildschirm rechts
    bool arrived = false;                                                    // Ziel des Klickwegs erreicht?
    if (mx != 0.0f || my != 0.0f) {                                          // Tastensteuerung hat Vorrang
        m_path.clear(); m_pendingBuilding = -1; m_pendingPier = false;       // Klickweg abbrechen
        float len = std::sqrt(mx * mx + my * my);                            // Länge
        mx /= len; my /= len;                                                // Normieren
    } else if (!m_path.empty()) {                                            // Klickweg folgen
        float tx = m_path.front().first, ty = m_path.front().second;         // Nächster Wegpunkt
        float d = dist(m_state.figX, m_state.figY, tx, ty);                  // Abstand
        if (d < 0.08f) { m_path.erase(m_path.begin()); arrived = m_path.empty(); } // Wegpunkt erreicht
        else { mx = (tx - m_state.figX) / d; my = (ty - m_state.figY) / d; if (d < speed * dt) { mx *= d / (speed * dt); my *= d / (speed * dt); } } // Richtung (am Ende nicht überschießen)
    }                                                                        // Ende Wegfolge
    m_figMoving = mx != 0.0f || my != 0.0f;                                  // Läuft die Figur?
    if (m_figMoving) {                                                       // Bewegen
        m_figAngle = std::atan2(my, mx);                                     // Blickrichtung
        float nx = m_state.figX + mx * speed * dt;                           // Neue Position x
        float ny = m_state.figY + my * speed * dt;                           // Neue Position y
        if (m_world.walkable(static_cast<int>(nx), static_cast<int>(m_state.figY))) m_state.figX = nx; // x getrennt prüfen (gleiten an Hindernissen)
        if (m_world.walkable(static_cast<int>(m_state.figX), static_cast<int>(ny))) m_state.figY = ny; // y getrennt prüfen
    }                                                                        // Ende Bewegen
    if (arrived) {                                                           // Ziel erreicht
        if (m_pendingBuilding >= 0) { int b = m_pendingBuilding; m_pendingBuilding = -1; enterBuilding(b); } // Gebäude betreten
        else if (m_pendingPier) { m_pendingPier = false; openPanel(Panel::Pier); } // Steg öffnen
    }                                                                        // Ende Ankunft
} // Ende von updateFigure

// Bewegt das Schiff (Wind, Segel, Maschine, Ruder, Untiefen)
void Game::updateShip(float dt) {                                            // Beginn von updateShip
    ShipStats st = m_state.stats();                                          // Schiffswerte
    if (m_dockAnim > 0.0f) {                                                 // Anlegeanimation läuft
        m_dockAnim = std::max(0.0f, m_dockAnim - dt);                        // Fortschritt
        const Island& isl = m_world.islands[static_cast<std::size_t>(m_state.docked)]; // Zielhafen
        float t = 1.0f - m_dockAnim;                                         // 0..1
        t = t * t * (3.0f - 2.0f * t);                                       // Weich beginnen und enden
        m_state.shipX = lerp(m_dockFromX, isl.dockX, t);                     // Position x
        m_state.shipY = lerp(m_dockFromY, isl.dockY, t);                     // Position y
        m_state.shipAngle = m_dockFromA + angleDiff(m_dockFromA, isl.dockAngle) * t; // Ausrichtung
        if (m_dockAnim <= 0.0f) {                                            // Fertig angelegt
            m_state.onFoot = true;                                           // Kapitän geht an Land
            m_state.figX = isl.pierEndX; m_state.figY = isl.pierEndY;        // Am Stegende
            m_figAngle = std::atan2(isl.pierBaseY - isl.pierEndY, isl.pierBaseX - isl.pierEndX); // Blick zum Land
            notify("Angelegt in " + isl.name + ".");                         // Meldung
            for (const Contract& c : m_state.active) if (c.to == m_state.docked) { notify("Auftrag: " + std::to_string(c.amount) + " " + goodName(c.good) + " hier im Kontor abliefern."); break; } // Hinweis auf Auftrag
            saveGame();                                                      // Automatisch speichern
        }                                                                    // Ende fertig
        return;                                                              // Keine Steuerung während des Anlegens
    }                                                                        // Ende Anlegeanimation
    WeatherSlot w = m_state.weatherAt(m_state.hours);                        // Aktuelles Wetter
    const ShipClass& cls = m_state.shipClass();                              // Schiffsklasse
    bool motorUsed = st.motorPower > 0.0f && (cls.drive == "motor" || m_engineOn) && m_state.fuel > 0.0f; // Läuft die Maschine?
    float level = static_cast<float>(std::max(0, m_throttle)) / 3.0f;        // Fahrstufe 0..1
    float sailPart = 0.0f, motorPart = 0.0f;                                 // Anteile am Vortrieb
    if (st.sails) {                                                          // Segel
        float windFactor = 0.25f + 0.75f * clampValue(w.windStrength / 0.55f, 0.0f, 1.2f); // Mehr Wind = mehr Fahrt
        sailPart = level * sailEfficiency(angleDiff(m_state.shipAngle, w.windAngle)) * windFactor; // Segelkraft
    }                                                                        // Ende Segel
    if (motorUsed) motorPart = level * st.motorPower;                        // Maschinenkraft
    float propulsion = cls.drive == "motor" ? motorPart : std::min(1.15f, sailPart + motorPart * 0.8f); // Gesamter Vortrieb
    float heelPenalty = 1.0f - 0.25f * clampValue((m_state.capsizeRisk() - 0.5f) * 2.0f, 0.0f, 1.0f); // Schlagseite bremst
    float target = st.speed * propulsion * heelPenalty;                      // Zielgeschwindigkeit
    if (m_throttle < 0) target = -st.speed * m_set.reverseFactor;            // Rückwärts (Riemen oder Maschine)
    float accel = m_set.acceleration * (std::fabs(target) < std::fabs(m_shipSpeed) ? 1.6f : 1.0f); // Bremsen geht schneller
    m_shipSpeed = approach(m_shipSpeed, target, accel * dt);                 // Geschwindigkeit anpassen
    if (motorUsed && m_throttle > 0) {                                       // Treibstoff verbrauchen
        m_state.fuel = std::max(0.0f, m_state.fuel - st.fuelUse * level * dt); // Verbrauch
        if (m_state.fuel <= 0.0f) notify("Der Treibstoff ist aufgebraucht! Kohle gibt es im Kontor oder in der Werft."); // Leer
    }                                                                        // Ende Treibstoff
    float rudder = 0.0f;                                                     // Ruderausschlag
    if (keyHeld("links")) rudder -= 1.0f;                                    // Nach links (Bildschirm)
    if (keyHeld("rechts")) rudder += 1.0f;                                   // Nach rechts
    if (rudder != 0.0f) m_autopilot = false;                                 // Handsteuerung schaltet den Autopiloten aus
    if (m_autopilot && m_state.hasTarget) {                                  // Autopilot
        float d = dist(m_state.shipX, m_state.shipY, m_state.targetX, m_state.targetY); // Abstand zum Ziel
        m_repath -= dt;                                                      // Zeitgeber der Wegprüfung
        if (m_seaPath.empty() || (m_repath <= 0.0f && !m_world.clearLine(m_state.shipX, m_state.shipY, m_seaPath.front().first, m_seaPath.front().second, st.draft))) planRoute(); // Weg (neu) planen
        if (m_repath <= 0.0f) m_repath = 3.0f;                               // Nächste Prüfung
        if (d < 2.5f) { m_autopilot = false; m_seaPath.clear(); notify("Ziel erreicht: " + m_state.targetName); } // Angekommen
        else if (!m_seaPath.empty()) {                                       // Wegpunkt ansteuern
            if (m_seaPath.size() > 1 && dist(m_state.shipX, m_state.shipY, m_seaPath.front().first, m_seaPath.front().second) < 1.8f) m_seaPath.erase(m_seaPath.begin()); // Wegpunkt erreicht
            float wx = m_seaPath.front().first, wy = m_seaPath.front().second; // Nächster Wegpunkt
            rudder = clampValue(angleDiff(m_state.shipAngle, std::atan2(wy - m_state.shipY, wx - m_state.shipX)) * 2.0f, -1.0f, 1.0f); // Dorthin drehen
            if (d < 7.0f && m_throttle > 1) m_throttle = 1;                  // Vor dem Ziel langsamer (zum Anlegen)
        }                                                                    // Ende Wegpunkt
    }                                                                        // Ende Autopilot
    float turnRate = st.turn * PI / 180.0f * (0.35f + 0.65f * clampValue(std::fabs(m_shipSpeed) / 1.5f, 0.0f, 1.0f)); // Wendigkeit (im Stand schwächer)
    m_state.shipAngle += rudder * turnRate * dt;                             // Drehen
    float c = std::cos(m_state.shipAngle), s = std::sin(m_state.shipAngle);  // Kursvektor
    float nx = m_state.shipX + c * m_shipSpeed * dt;                         // Neue Position x
    float ny = m_state.shipY + s * m_shipSpeed * dt;                         // Neue Position y
    float dir = m_shipSpeed >= 0.0f ? 1.0f : -1.0f;                          // Fahrtrichtung
    float bowX = nx + c * 0.8f * dir, bowY = ny + s * 0.8f * dir;            // Bug (bzw. Heck bei Rückwärtsfahrt)
    bool free = m_world.sailable(nx, ny, st.draft) && m_world.sailable(bowX, bowY, st.draft); // Ist der Weg frei?
    m_groundWarn = std::max(0.0f, m_groundWarn - dt);                        // Zeitgeber der Warnung
    if (free) { m_state.shipX = nx; m_state.shipY = ny; }                    // Fahren
    else if (std::fabs(m_shipSpeed) > 0.05f) {                               // Hindernis
        const Tile& t = m_world.tile(static_cast<int>(bowX), static_cast<int>(bowY)); // Kachel am Bug
        bool land = m_world.isLand(static_cast<int>(bowX), static_cast<int>(bowY)) || t.blocked; // Land, Steg oder Felsen?
        if (std::fabs(m_shipSpeed) > 0.8f) damageShip(std::fabs(m_shipSpeed) * m_set.groundDamage, land ? "Auflaufen" : "Grundberührung"); // Schaden bei Tempo
        if (m_groundWarn <= 0.0f) { notify(land ? "Rumms! Das Schiff ist aufgelaufen. Rückwärts (" + keyName("runter") + ") und wenden." : "Zu flach für den Tiefgang (" + oneDecimal(st.draft) + " m)! Rückwärts (" + keyName("runter") + ")."); m_groundWarn = 3.0f; } // Meldung
        splash(bowX, bowY, 10, rgba(230, 245, 255));                         // Gischt
        m_shipSpeed = 0.0f;                                                  // Stehen bleiben
        if (m_throttle > 0) m_throttle = 0;                                  // Fahrt wegnehmen (sonst rammt man immer wieder)
        m_autopilot = false;                                                 // Autopilot aus
    }                                                                        // Ende Hindernis
    m_stormWarn = std::max(0.0f, m_stormWarn - dt);                          // Zeitgeber der Sturmwarnung
    if (st.sails && w.windStrength >= m_set.stormFrom && m_throttle >= 3) {  // Volle Segel im Sturm
        m_state.hull -= m_set.stormDamage * dt;                              // Takelage und Rumpf leiden
        if (m_stormWarn <= 0.0f) { notify("Sturm! Segel reffen (" + keyName("runter") + "), sonst nimmt das Schiff Schaden!"); m_stormWarn = 8.0f; } // Warnung
        if (m_state.hull <= 0.0f) shipSunk("einen Sturm");                   // Gesunken
    }                                                                        // Ende Sturm
    m_wakeTimer -= dt;                                                       // Kielwasser
    if (m_wakeTimer <= 0.0f && std::fabs(m_shipSpeed) > 0.5f) {              // Neue Gischt hinter dem Schiff
        m_wakeTimer = 0.06f;                                                 // Abstand
        Particle p;                                                          // Neuer Partikel
        p.x = m_state.shipX - c * 0.9f + (randf() - 0.5f) * 0.4f;            // Hinter dem Heck
        p.y = m_state.shipY - s * 0.9f + (randf() - 0.5f) * 0.4f;            // Hinter dem Heck y
        p.vx = -c * 0.3f + (randf() - 0.5f) * 0.3f; p.vy = -s * 0.3f + (randf() - 0.5f) * 0.3f; // Treibt langsam weg
        p.life = p.maxLife = 1.6f; p.size = 2.0f + randf() * 2.0f;           // Lebensdauer und Größe
        p.color = rgba(235, 248, 255, 200);                                  // Weiß
        m_particles.push_back(p);                                            // Speichern
    }                                                                        // Ende Kielwasser
    m_cannonReload = std::max(0.0f, m_cannonReload - dt);                    // Kanonen laden nach
} // Ende von updateShip

// Kamera folgt Figur oder Schiff
void Game::updateCamera(float dt) {                                          // Beginn von updateCamera
    float tx = m_state.onFoot ? m_state.figX : m_state.shipX + std::cos(m_state.shipAngle) * m_shipSpeed * 0.7f; // Ziel x (beim Schiff etwas voraus)
    float ty = m_state.onFoot ? m_state.figY : m_state.shipY + std::sin(m_state.shipAngle) * m_shipSpeed * 0.7f; // Ziel y
    float k = std::min(1.0f, dt * 4.0f);                                     // Nachführrate
    m_cam.x += (tx - m_cam.x) * k;                                           // Weich nachführen
    m_cam.y += (ty - m_cam.y) * k;                                           // Weich nachführen
} // Ende von updateCamera

// Steuert alle Piratenschiffe
void Game::updatePirates(float dt) {                                         // Beginn von updatePirates
    bool atSea = !m_state.onFoot && m_dockAnim <= 0.0f;                      // Ist der Spieler auf See?
    ShipStats st = m_state.stats();                                          // Eigene Werte
    WeatherSlot w = m_state.weatherAt(m_state.hours);                        // Wetter (Nebel verkürzt die Sicht)
    float sight = (m_state.isNight() ? m_set.pirateSightNight : m_set.pirateSight) * (w.type == WeatherType::Fog ? 0.6f : 1.0f); // Sichtweite der Piraten
    for (std::size_t zi = 0; zi < m_world.zones.size(); ++zi) {              // Neue Piraten
        const Zone& z = m_world.zones[zi];                                   // Zone
        if (z.kind != "piraten") continue;                                   // Nur Piratenzonen
        float& cd = m_zoneCooldown[static_cast<int>(zi)];                    // Wartezeit
        cd = std::max(0.0f, cd - dt);                                        // Herunterzählen
        int count = 0;                                                       // Piraten dieser Zone
        for (const Pirate& p : m_pirates) if (p.zone == static_cast<int>(zi)) ++count; // Zählen
        if (!atSea || count >= static_cast<int>(z.strength + 0.5f) || cd > 0.0f) continue; // Kein neuer Pirat
        if (dist(m_state.shipX, m_state.shipY, z.x, z.y) > z.radius + 30.0f) continue; // Spieler zu weit weg
        for (int attempt = 0; attempt < 20; ++attempt) {                     // Startpunkt suchen
            float a = randf() * 6.283f, r = std::sqrt(randf()) * z.radius;   // Zufällig in der Zone
            float px = z.x + std::cos(a) * r, py = z.y + std::sin(a) * r;    // Position
            if (!m_world.sailable(px, py, 1.5f) || dist(px, py, m_state.shipX, m_state.shipY) < st.scan * 0.9f) continue; // Ungeeignet oder in Sicht
            Pirate p;                                                        // Neuer Pirat
            p.x = px; p.y = py; p.angle = randf() * 6.283f; p.hp = m_set.pirateHp; p.zone = static_cast<int>(zi); // Werte
            p.tx = z.x; p.ty = z.y;                                          // Erster Wegpunkt
            m_pirates.push_back(p);                                          // Speichern
            cd = 45.0f;                                                      // Wartezeit bis zum nächsten
            break;                                                           // Fertig
        }                                                                    // Ende der Suche
    }                                                                        // Ende der Zonen
    for (Pirate& p : m_pirates) {                                            // Alle Piraten
        if (p.state == 3) { p.timer += dt; continue; }                       // Sinkt
        const Zone& z = m_world.zones[static_cast<std::size_t>(p.zone)];     // Heimatzone
        float d = dist(p.x, p.y, m_state.shipX, m_state.shipY);              // Abstand zum Spieler
        float desired = p.angle;                                             // Gewünschter Kurs
        float speed = m_set.pirateSpeed * 0.45f;                             // Gewünschtes Tempo
        if (!atSea && p.state == 1) { p.state = 0; p.decided = false; }      // Spieler im Hafen: Jagd beenden
        if (p.state == 0) {                                                  // Kreuzen
            if (dist(p.x, p.y, p.tx, p.ty) < 2.0f) { float a = randf() * 6.283f, r = randf() * z.radius; p.tx = z.x + std::cos(a) * r; p.ty = z.y + std::sin(a) * r; } // Neuer Wegpunkt
            desired = std::atan2(p.ty - p.y, p.tx - p.x);                    // Zum Wegpunkt
            if (atSea && d < sight) {                                        // Spieler entdeckt
                if (!p.decided) {                                            // Noch nicht entschieden
                    p.decided = true;                                        // Jetzt entscheiden
                    bool onRoute = m_world.routeDistance(m_state.shipX, m_state.shipY) < 3.0f; // Auf einer Handelsroute?
                    p.attack = !onRoute || randf() < m_set.pirateRouteChance; // Auf Routen selten
                    if (p.attack) { p.state = 1; notify("Piraten! Ein Piratenschiff nimmt Kurs auf dich!"); sound("glocke"); } // Angriff
                }                                                            // Ende Entscheidung
            } else if (d > sight * 1.5f) p.decided = false;                  // Außer Sicht: später neu entscheiden
        } else if (p.state == 1) {                                           // Jagd
            float px = m_state.shipX + std::cos(m_state.shipAngle) * m_shipSpeed * 0.8f; // Vorausberechnete Position
            float py = m_state.shipY + std::sin(m_state.shipAngle) * m_shipSpeed * 0.8f; // Vorausberechnete Position y
            desired = std::atan2(py - p.y, px - p.x);                        // Abfangkurs
            speed = m_set.pirateSpeed;                                       // Volle Fahrt
            p.reload -= dt;                                                  // Nachladen
            if (d < m_set.pirateRange && p.reload <= 0.0f) {                 // Feuern
                Shot sh;                                                     // Neue Kugel
                sh.x = p.x; sh.y = p.y;                                      // Start
                sh.total = sh.life = std::max(0.3f, d / 8.0f);               // Flugzeit
                float ex = px + (randf() - 0.5f) * 1.6f, ey = py + (randf() - 0.5f) * 1.6f; // Ziel mit Streuung
                sh.vx = (ex - p.x) / sh.total; sh.vy = (ey - p.y) / sh.total; // Geschwindigkeit
                sh.damage = m_set.pirateDamage;                              // Schaden
                m_shots.push_back(sh);                                       // Speichern
                p.reload = m_set.pirateReload;                               // Nachladen
                sound("kanone", 70);                                         // Knall
                splash(p.x, p.y, 4, rgba(200, 200, 200, 200));               // Pulverdampf
            }                                                                // Ende Feuern
            if (d < 1.3f) { stealCargo(); p.state = 2; p.timer = 25.0f; }    // Entern
            else if (d > sight * 2.2f) { p.state = 0; p.decided = false; notify("Die Piraten haben die Verfolgung aufgegeben."); } // Abgehängt
        } else if (p.state == 2) {                                           // Flucht mit der Beute
            desired = std::atan2(p.y - m_state.shipY, p.x - m_state.shipX);  // Weg vom Spieler
            speed = m_set.pirateSpeed;                                       // Volle Fahrt
            p.timer -= dt;                                                   // Zeit läuft
        }                                                                    // Ende der Zustände
        p.angle += clampValue(angleDiff(p.angle, desired), -1.2f * dt, 1.2f * dt); // Langsam drehen
        p.speed = approach(p.speed, speed, 0.8f * dt);                       // Beschleunigen
        float nx = p.x + std::cos(p.angle) * p.speed * dt, ny = p.y + std::sin(p.angle) * p.speed * dt; // Neue Position
        if (m_world.sailable(nx + std::cos(p.angle) * 0.8f, ny + std::sin(p.angle) * 0.8f, 1.2f)) { p.x = nx; p.y = ny; } // Freie Fahrt
        else { p.angle += 1.6f * dt * 3.0f; p.speed *= 0.5f; }               // Hindernis: abdrehen
    }                                                                        // Ende der Piraten
    m_pirates.erase(std::remove_if(m_pirates.begin(), m_pirates.end(), [&](const Pirate& p) { // Piraten entfernen
        if (p.state == 3) return p.timer > 3.0f;                             // Fertig gesunken
        if (p.state == 2) return p.timer <= 0.0f;                            // Mit der Beute verschwunden
        const Zone& z = m_world.zones[static_cast<std::size_t>(p.zone)];     // Zone
        return dist(m_state.shipX, m_state.shipY, z.x, z.y) > z.radius + 45.0f; // Spieler weit weg
    }), m_pirates.end());                                                    // Ende des Entfernens
} // Ende von updatePirates

// Steuert die Seeungeheuer (nur nachts in ihren Gebieten)
void Game::updateMonsters(float dt) {                                        // Beginn von updateMonsters
    bool atSea = !m_state.onFoot && m_dockAnim <= 0.0f;                      // Spieler auf See?
    bool night = m_state.isNight();                                          // Nacht?
    for (std::size_t zi = 0; zi < m_world.zones.size(); ++zi) {              // Neue Ungeheuer
        const Zone& z = m_world.zones[zi];                                   // Zone
        if (z.kind != "monster") continue;                                   // Nur Monsterzonen
        float& cd = m_zoneCooldown[1000 + static_cast<int>(zi)];             // Wartezeit (eigener Schlüsselbereich)
        cd = std::max(0.0f, cd - dt);                                        // Herunterzählen
        bool exists = false;                                                 // Gibt es schon eins?
        for (const Monster& m : m_monsters) if (m.zone == static_cast<int>(zi)) exists = true; // Suchen
        if (exists || !atSea || !night || cd > 0.0f) continue;               // Kein neues
        if (dist(m_state.shipX, m_state.shipY, z.x, z.y) > z.radius) continue; // Spieler nicht im Gebiet
        Monster m;                                                           // Neues Ungeheuer
        float a = m_state.shipAngle + PI + (randf() - 0.5f) * 1.2f;          // Hinter dem Schiff
        m.x = m_state.shipX + std::cos(a) * m_set.monsterSight; m.y = m_state.shipY + std::sin(a) * m_set.monsterSight; // Position
        m.angle = a + PI; m.hp = m_set.monsterHp; m.zone = static_cast<int>(zi); m.bite = 1.0f; // Werte
        m_monsters.push_back(m);                                             // Speichern
        notify("Ein Seeungeheuer taucht aus der Tiefe auf! Nur schnelle Schiffe entkommen.");    // Warnung
        sound("monster");                                                    // Brüllen
    }                                                                        // Ende der Zonen
    for (Monster& m : m_monsters) {                                          // Alle Ungeheuer
        float d = dist(m.x, m.y, m_state.shipX, m_state.shipY);              // Abstand
        if (m.state == 0) {                                                  // Jagt
            m.emerge = std::min(1.0f, m.emerge + dt * 0.8f);                 // Auftauchen
            if (!night || !atSea || d > m_set.monsterSight + 7.0f) {         // Tag, Hafen oder abgehängt
                m.state = 1;                                                 // Abtauchen
                m_zoneCooldown[1000 + m.zone] = 40.0f;                       // Wartezeit
                if (atSea) notify(d > m_set.monsterSight + 7.0f ? "Du bist dem Seeungeheuer entkommen!" : "Das Seeungeheuer verschwindet in der Tiefe."); // Meldung
            }                                                                // Ende Abbruch
            float desired = std::atan2(m_state.shipY - m.y, m_state.shipX - m.x); // Zum Schiff
            m.angle += clampValue(angleDiff(m.angle, desired), -2.0f * dt, 2.0f * dt); // Drehen
            float sp = m.emerge > 0.5f ? m_set.monsterSpeed : 0.5f;          // Erst nach dem Auftauchen schnell
            float nx = m.x + std::cos(m.angle) * sp * dt, ny = m.y + std::sin(m.angle) * sp * dt; // Neue Position
            if (!m_world.isLand(static_cast<int>(nx), static_cast<int>(ny))) { m.x = nx; m.y = ny; } // Nicht an Land
            m.bite -= dt;                                                    // Bisspause
            if (d < 1.5f && m.bite <= 0.0f && atSea) {                       // Zubeißen
                damageShip(m_set.monsterDamage, "ein Seeungeheuer");         // Schaden
                m.bite = m_set.monsterBitePause;                             // Pause
                splash(m_state.shipX, m_state.shipY, 16, rgba(220, 240, 255)); // Gischt
                sound("monster", 80);                                        // Brüllen
            }                                                                // Ende Biss
            if (m.hp <= 0.0f) {                                              // Vertrieben
                m.state = 1;                                                 // Abtauchen
                m_zoneCooldown[1000 + m.zone] = 120.0f;                      // Lange Ruhe
                m_state.explorerPoints += 15;                                // Belohnung
                notify("Das Seeungeheuer flieht verwundet in die Tiefe! +15 Entdeckerpunkte");     // Meldung
            }                                                                // Ende vertrieben
        } else {                                                             // Taucht ab
            m.emerge -= dt * 0.7f;                                           // Versinken
        }                                                                    // Ende der Zustände
    }                                                                        // Ende der Ungeheuer
    m_monsters.erase(std::remove_if(m_monsters.begin(), m_monsters.end(), [](const Monster& m) { return m.state == 1 && m.emerge <= 0.0f; }), m_monsters.end()); // Abgetauchte entfernen
} // Ende von updateMonsters

// Bewegt die Kanonenkugeln und wertet Treffer aus
void Game::updateShots(float dt) {                                           // Beginn von updateShots
    for (Shot& s : m_shots) {                                                // Alle Kugeln
        s.x += s.vx * dt; s.y += s.vy * dt;                                  // Fliegen
        s.life -= dt;                                                        // Flugzeit
        if (s.life > 0.0f) continue;                                         // Noch in der Luft
        bool hit = false;                                                    // Getroffen?
        if (s.fromPlayer) {                                                  // Eigene Kugel
            for (Pirate& p : m_pirates) {                                    // Piraten
                if (p.state == 3 || dist(s.x, s.y, p.x, p.y) > 1.2f) continue; // Daneben
                p.hp -= s.damage; hit = true;                                // Treffer
                if (p.state == 0) { p.state = 1; p.decided = true; p.attack = true; } // Angegriffene Piraten wehren sich
                if (p.hp <= 0.0f) {                                          // Versenkt
                    p.state = 3; p.timer = 0.0f;                             // Sinkt
                    int loot = m_set.lootMin + static_cast<int>(randf() * static_cast<float>(m_set.lootMax - m_set.lootMin)); // Beute
                    m_state.credits += loot; m_state.totalEarned += loot; ++m_state.piratesSunk; // Gutschreiben
                    notify("Piratenschiff versenkt! Beute: " + std::to_string(loot) + " Cr"); // Meldung
                    sound("platsch");                                        // Geräusch
                }                                                            // Ende versenkt
                break;                                                       // Eine Kugel trifft nur einmal
            }                                                                // Ende der Piraten
            if (!hit) for (Monster& m : m_monsters) if (m.state == 0 && dist(s.x, s.y, m.x, m.y) < 1.4f) { m.hp -= s.damage; hit = true; break; } // Ungeheuer
        } else if (!m_state.onFoot && dist(s.x, s.y, m_state.shipX, m_state.shipY) < 1.2f) { // Feindliche Kugel trifft das eigene Schiff
            damageShip(s.damage, "Piraten");                                 // Schaden
            hit = true;                                                      // Treffer
        }                                                                    // Ende der Trefferprüfung
        splash(s.x, s.y, hit ? 10 : 8, hit ? rgba(255, 170, 60) : rgba(220, 240, 255)); // Feuer oder Wasserfontäne
        if (!hit) sound("platsch", 40);                                      // Platschen
    }                                                                        // Ende der Kugeln
    m_shots.erase(std::remove_if(m_shots.begin(), m_shots.end(), [](const Shot& s) { return s.life <= 0.0f; }), m_shots.end()); // Gelandete entfernen
} // Ende von updateShots

// Bewegt alle Partikel
void Game::updateParticles(float dt) {                                       // Beginn von updateParticles
    for (Particle& p : m_particles) {                                        // Alle Partikel
        p.x += p.vx * dt; p.y += p.vy * dt; p.z += p.vz * dt;                // Bewegen
        p.vz -= 6.0f * dt;                                                   // Schwerkraft
        if (p.z < 0.0f) { p.z = 0.0f; p.vz = 0.0f; }                         // Auf der Wasseroberfläche bleiben
        p.life -= dt;                                                        // Altern
    }                                                                        // Ende der Schleife
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p) { return p.life <= 0.0f; }), m_particles.end()); // Alte entfernen
    if (m_particles.size() > 1500) m_particles.erase(m_particles.begin(), m_particles.begin() + static_cast<long>(m_particles.size() - 1500)); // Obergrenze
} // Ende von updateParticles

// Entdeckt Artefakte im Scanner und sammelt sie ein
void Game::updateArtifacts() {                                               // Beginn von updateArtifacts
    if (m_state.onFoot) return;                                              // Nur auf See
    float scan = m_state.stats().scan;                                       // Sichtweite
    WeatherSlot w = m_state.weatherAt(m_state.hours);                        // Wetter
    if (w.type == WeatherType::Fog) scan *= 0.5f;                            // Nebel halbiert die Sicht
    if (m_state.isNight()) scan *= 0.7f;                                     // Nachts sieht man weniger
    for (std::size_t i = 0; i < m_state.floating.size();) {                  // Alle Artefakte
        FloatingArtifact& a = m_state.floating[i];                           // Artefakt
        float d = dist(a.x, a.y, m_state.shipX, m_state.shipY);              // Abstand
        if (d < scan && !a.seen) { a.seen = true; notify("Der Ausguck meldet Treibgut! Es ist auf der Seekarte markiert."); } // Entdeckt
        if (d < 1.4f) {                                                      // Einsammeln
            const ArtifactDef* def = m_data.artifact(a.id);                  // Definition
            m_state.artifacts.push_back(a.id);                               // In die Kajüte
            notify("Artefakt geborgen: " + (def ? def->name : a.id) + "! Verkaufe es im Museum einer Hafenstadt."); // Meldung
            sound("fund");                                                   // Fundgeräusch
            splash(a.x, a.y, 12, rgba(255, 230, 120));                       // Glitzern
            m_state.floating.erase(m_state.floating.begin() + static_cast<long>(i)); // Vom Meer entfernen
        } else ++i;                                                          // Nächstes
    }                                                                        // Ende der Artefakte
} // Ende von updateArtifacts

// Aktionstaste
void Game::interact() {                                                      // Beginn von interact
    if (m_state.onFoot) {                                                    // Zu Fuß
        int b = nearDoor();                                                  // Gebäude in der Nähe?
        if (b >= 0) { enterBuilding(b); return; }                            // Betreten
        if (nearPier()) { openPanel(Panel::Pier); return; }                  // Am Steg
        notify("Hier gibt es nichts zu tun. Gehe zu einer Tür oder zum Stegende.");  // Hinweis
        return;                                                              // Fertig
    }                                                                        // Ende zu Fuß
    if (m_dockAnim > 0.0f) return;                                           // Legt gerade an
    int isl = nearDock();                                                    // Hafen in der Nähe?
    if (isl < 0) { notify("Anlegen ist nur am Hafensteg einer Insel möglich. Setze auf der Seekarte ein Ziel."); return; } // Kein Hafen
    if (std::fabs(m_shipSpeed) > m_set.dockSpeed) { notify("Zu schnell zum Anlegen! Fahrt verringern (" + keyName("runter") + ")."); return; } // Zu schnell
    dock(isl);                                                               // Anlegen
} // Ende von interact

// Klick in die Welt: Figur läuft dorthin, Schiff nimmt Kurs
// Sucht das vorderste Gebäude, dessen Bild an einem Bildschirmpunkt sichtbar ist
int Game::pickBuilding(int sx, int sy) {                                     // Beginn von pickBuilding
    int best = -1;                                                           // Bester Treffer
    float bestDepth = -1e9f;                                                 // Tiefe des besten Treffers (vorne = größer)
    for (std::size_t i = 0; i < m_world.buildings.size(); ++i) {             // Alle Gebäude
        const Building& b = m_world.buildings[i];                            // Gebäude
        if (b.model.empty()) continue;                                       // Ohne Modell
        const Sprite& s = m_lib.sprite(b.model, b.facing, 4, m_cam.tileWidth); // Bild wie in der Welt
        if (!s.valid()) continue;                                            // Leer
        int x0 = static_cast<int>(m_cam.toScreenX(b.x, b.y)) - s.anchorX;    // Linke Kante
        int y0 = static_cast<int>(m_cam.toScreenY(b.x, b.y, LAND_HEIGHT)) - s.anchorY; // Obere Kante
        if (alphaOf(s.image.get(sx - x0, sy - y0)) < 40) continue;           // Durchsichtiger Pixel: kein Treffer
        if (b.x + b.y > bestDepth) { bestDepth = b.x + b.y; best = static_cast<int>(i); } // Vorderstes merken
    }                                                                        // Ende der Gebäude
    return best;                                                             // Ergebnis
} // Ende von pickBuilding

void Game::clickWorld(float wx, float wy, int sx, int sy) {                  // Beginn von clickWorld
    if (!m_state.onFoot) {                                                   // Auf dem Schiff
        if (m_dockAnim > 0.0f) return;                                       // Legt gerade an
        int isl = -1;                                                        // Geklickte Insel
        for (std::size_t i = 0; i < m_world.islands.size(); ++i) if (dist(wx, wy, m_world.islands[i].cx, m_world.islands[i].cy) < m_world.islands[i].radius * 1.1f) isl = static_cast<int>(i); // Insel getroffen?
        if (isl >= 0) setTarget(m_world.islands[static_cast<std::size_t>(isl)].dockX, m_world.islands[static_cast<std::size_t>(isl)].dockY, "Hafen " + islandName(isl)); // Hafen als Ziel
        else setTarget(wx, wy, "Wegpunkt");                                  // Wegpunkt auf See
        m_autopilot = planRoute();                                           // Autopilot steuert dorthin (wenn es einen Seeweg gibt)
        if (!m_autopilot) return;                                            // Kein Weg
        if (m_throttle <= 0) m_throttle = 2;                                 // Fahrt aufnehmen
        notify("Kurs auf " + m_state.targetName + ". Ruder (" + keyName("links") + "/" + keyName("rechts") + ") übernimmt wieder von Hand."); // Meldung
        return;                                                              // Fertig
    }                                                                        // Ende Schiff
    m_pendingBuilding = -1; m_pendingPier = false;                           // Alte Absichten verwerfen
    float tx = wx, ty = wy;                                                  // Ziel
    int picked = pickBuilding(sx, sy);                                       // Gebäude unter der Maus (pixelgenau)
    if (picked < 0) for (std::size_t i = 0; i < m_world.buildings.size(); ++i) if (dist(wx, wy, m_world.buildings[i].doorX, m_world.buildings[i].doorY) < 0.8f) picked = static_cast<int>(i); // Sonst: Klick vor eine Tür
    if (picked >= 0) { m_pendingBuilding = picked; tx = m_world.buildings[static_cast<std::size_t>(picked)].doorX; ty = m_world.buildings[static_cast<std::size_t>(picked)].doorY; } // Tür als Ziel
    if (m_pendingBuilding < 0 && m_state.docked >= 0) {                      // Steg oder Schiff angeklickt?
        const Island& isl = m_world.islands[static_cast<std::size_t>(m_state.docked)]; // Hafeninsel
        const Sprite& ship = m_lib.sprite(m_state.shipClass().model, m_state.shipAngle, m_shipSteps, m_cam.tileWidth); // Bild des Schiffs
        int x0 = static_cast<int>(m_cam.toScreenX(m_state.shipX, m_state.shipY)) - ship.anchorX; // Linke Kante
        int y0 = static_cast<int>(m_cam.toScreenY(m_state.shipX, m_state.shipY, 0.0f)) - ship.anchorY; // Obere Kante
        bool onShip = ship.valid() && alphaOf(ship.image.get(sx - x0, sy - y0)) >= 40; // Schiff angeklickt (pixelgenau)?
        if (onShip || dist(wx, wy, isl.pierEndX, isl.pierEndY) < 1.2f) { m_pendingPier = true; tx = isl.pierEndX; ty = isl.pierEndY; } // Stegende als Ziel
    }                                                                        // Ende Steg
    if ((m_pendingBuilding >= 0 || m_pendingPier) && dist(m_state.figX, m_state.figY, tx, ty) < 0.5f) { // Schon da
        if (m_pendingBuilding >= 0) { int b = m_pendingBuilding; m_pendingBuilding = -1; enterBuilding(b); } // Betreten
        else { m_pendingPier = false; openPanel(Panel::Pier); }             // Steg öffnen
        return;                                                              // Fertig
    }                                                                        // Ende schon da
    m_path = m_world.findPath(m_state.figX, m_state.figY, tx, ty);           // Weg suchen
    if (m_path.empty()) { m_pendingBuilding = -1; m_pendingPier = false; notify("Dorthin führt kein Weg."); } // Kein Weg
} // Ende von clickWorld

// Betritt ein Gebäude (öffnet das passende Fenster)
void Game::enterBuilding(int building) {                                     // Beginn von enterBuilding
    const Building& b = m_world.buildings[static_cast<std::size_t>(building)]; // Gebäude
    if (!m_state.isOpen(b.type)) { notify(buildingTitle(building) + " hat geschlossen. " + m_state.openText(b.type) + "."); return; } // Geschlossen
    m_figAngle = std::atan2(b.y - m_state.figY, b.x - m_state.figX);         // Zum Gebäude schauen
    if (b.type == "kontor") openPanel(Panel::Kontor, building);              // Kontor
    else if (b.type == "haendler") openPanel(Panel::Trader, building);       // Händler
    else if (b.type == "hersteller") openPanel(Panel::Producer, building);   // Hersteller
    else if (b.type == "museum") openPanel(Panel::Museum, building);         // Museum
    else if (b.type == "werft") openPanel(Panel::Shipyard, building);        // Werft
    else if (b.type == "taverne") openPanel(Panel::Tavern, building);        // Taverne
} // Ende von enterBuilding

// Beginnt das Anlegen
void Game::dock(int island) {                                                // Beginn von dock
    m_dockAnim = 1.2f;                                                       // Dauer der Animation
    m_dockFromX = m_state.shipX; m_dockFromY = m_state.shipY; m_dockFromA = m_state.shipAngle; // Startpunkt merken
    m_shipSpeed = 0.0f; m_throttle = 0; m_autopilot = false;                 // Anhalten
    m_state.docked = island;                                                 // Liegeplatz
    m_state.lastHarbor = island;                                             // Letzter Hafen
    if (m_state.hasTarget && dist(m_state.targetX, m_state.targetY, m_world.islands[static_cast<std::size_t>(island)].dockX, m_world.islands[static_cast<std::size_t>(island)].dockY) < 3.0f) m_state.hasTarget = false; // Ziel erreicht
    for (Pirate& p : m_pirates) if (p.state == 1) { p.state = 0; p.decided = false; } // Piraten geben auf
    sound("anlegen");                                                        // Geräusch
} // Ende von dock

// Geht an Bord und legt ab
void Game::board() {                                                         // Beginn von board
    if (m_state.docked < 0) return;                                          // Kein Schiff hier
    if (m_state.capsizeRisk() >= 1.0f) { capsize(); return; }                // Zu schief beladen -> kentert
    const Island& isl = m_world.islands[static_cast<std::size_t>(m_state.docked)]; // Hafen
    m_state.onFoot = false;                                                  // An Bord
    m_state.shipX = isl.dockX; m_state.shipY = isl.dockY; m_state.shipAngle = isl.dockAngle; // Am Liegeplatz
    m_state.docked = -1;                                                     // Abgelegt
    m_shipSpeed = 0.0f; m_throttle = 0; m_autopilot = false; m_engineOn = false; // Ruhig starten
    m_panel = Panel::None;                                                   // Fenster schließen
    m_path.clear();                                                          // Kein Fußweg
    sound("glocke");                                                         // Schiffsglocke
    ShipStats st = m_state.stats();                                          // Werte
    notify("Leinen los! " + keyName("hoch") + "/" + keyName("runter") + ": Fahrt, " + keyName("links") + "/" + keyName("rechts") + ": Ruder, Klick: Kurs setzen."); // Hilfe
    if (st.missingCrew > 0) notify("Unterbesetzt: Es fehlen " + std::to_string(st.missingCrew) + " Leute. Das Schiff ist langsamer und schwerfälliger."); // Warnung
    if (m_state.capsizeRisk() > 0.6f) notify("Das Schiff hat Schlagseite und fährt langsamer.");          // Warnung
} // Ende von board

// Das Schiff kentert beim Ablegen (Animation im Minispiel)
void Game::capsize() {                                                       // Beginn von capsize
    m_screen = Screen::LoadShip;                                             // Minispiel-Bildschirm zeigt das Kentern
    m_panel = Panel::None;                                                   // Fenster schließen
    m_lsCapsize = 0.0f;                                                      // Animation starten
    m_lsHolding = false;                                                     // Nichts in der Hand
    m_lsMessage = "Das Schiff ist zu schief beladen und kentert!";           // Hinweis
    sound("kentern");                                                        // Geräusch
} // Ende von capsize

// Feuert die eigenen Kanonen auf das nächste Ziel
void Game::fireCannons() {                                                   // Beginn von fireCannons
    ShipStats st = m_state.stats();                                          // Werte
    if (st.defense <= 0.0f) { notify("Dieses Schiff hat keine Kanonen. Upgrade in der Werft oder ein größeres Schiff kaufen."); return; } // Keine Kanonen
    if (m_cannonReload > 0.0f) return;                                       // Lädt noch
    float bestD = m_set.cannonRange;                                         // Höchste Reichweite
    float tx = 0.0f, ty = 0.0f, tvx = 0.0f, tvy = 0.0f;                      // Ziel und seine Bewegung
    bool found = false;                                                      // Ziel gefunden?
    for (const Pirate& p : m_pirates) {                                      // Piraten
        float d = dist(p.x, p.y, m_state.shipX, m_state.shipY);              // Abstand
        if (p.state != 3 && d < bestD) { bestD = d; tx = p.x; ty = p.y; tvx = std::cos(p.angle) * p.speed; tvy = std::sin(p.angle) * p.speed; found = true; } // Näher
    }                                                                        // Ende der Piraten
    for (const Monster& m : m_monsters) {                                    // Ungeheuer
        float d = dist(m.x, m.y, m_state.shipX, m_state.shipY);              // Abstand
        if (m.state == 0 && d < bestD) { bestD = d; tx = m.x; ty = m.y; tvx = std::cos(m.angle) * m_set.monsterSpeed; tvy = std::sin(m.angle) * m_set.monsterSpeed; found = true; } // Näher
    }                                                                        // Ende der Ungeheuer
    if (!found) { notify("Kein Ziel in Reichweite der Kanonen."); return; }  // Nichts zu treffen
    int count = st.defense >= 30.0f ? 3 : (st.defense >= 15.0f ? 2 : 1);     // Anzahl der Kugeln
    for (int k = 0; k < count; ++k) {                                        // Alle Kugeln
        Shot s;                                                              // Neue Kugel
        s.x = m_state.shipX; s.y = m_state.shipY;                            // Start
        s.total = s.life = std::max(0.3f, bestD / 9.0f);                     // Flugzeit
        float ex = tx + tvx * s.total + (randf() - 0.5f) * 0.9f, ey = ty + tvy * s.total + (randf() - 0.5f) * 0.9f; // Vorhalt mit Streuung
        s.vx = (ex - s.x) / s.total; s.vy = (ey - s.y) / s.total;            // Geschwindigkeit
        s.damage = st.defense / static_cast<float>(count);                   // Schaden je Kugel
        s.fromPlayer = true;                                                 // Eigene Kugel
        m_shots.push_back(s);                                                // Speichern
    }                                                                        // Ende der Kugeln
    m_cannonReload = m_set.cannonReload;                                     // Nachladen
    sound("kanone");                                                         // Knall
    splash(m_state.shipX, m_state.shipY, 6, rgba(210, 210, 210, 200));       // Pulverdampf
} // Ende von fireCannons

// Schaden am eigenen Schiff (Panzerung schützt)
void Game::damageShip(float amount, const std::string& cause) {              // Beginn von damageShip
    ShipStats st = m_state.stats();                                          // Werte
    m_state.hull -= amount * (1.0f - st.armor / 100.0f);                     // Schaden abzüglich Panzerung
    m_hitFlash = 0.35f;                                                      // Roter Blitz
    sound("treffer");                                                        // Geräusch
    if (m_state.hull <= 0.0f) shipSunk(cause);                               // Gesunken
} // Ende von damageShip

// Das Schiff ist gesunken
void Game::shipSunk(const std::string& cause) {                              // Beginn von shipSunk
    std::string oldName = m_state.shipClass().name;                          // Name des alten Schiffs
    int lostCargo = static_cast<int>(m_state.hold.size());                   // Verlorene Ladung
    int lostArtifacts = static_cast<int>(m_state.artifacts.size());          // Verlorene Artefakte
    m_state.hold.clear(); m_state.artifacts.clear();                         // Alles versinkt
    ++m_state.shipsLost;                                                     // Statistik
    const ShipClass* start = m_data.ship(m_set.startShip);                   // Ersatzschiff
    m_state.shipId = start ? start->id : m_data.ships.front().id;            // Kutter
    m_state.upgrades.clear(); m_state.upgradeSpent = 0;                      // Verbesserungen verloren
    while (static_cast<int>(m_state.crew.size()) > m_state.shipClass().maxCrew) m_state.crew.pop_back(); // Zu viele Leute für den Kutter
    ShipStats st = m_state.stats();                                          // Neue Werte
    m_state.hull = st.hullMax; m_state.fuel = st.fuelMax;                    // Neues Schiff ist heil
    int isl = m_state.lastHarbor;                                            // Rettungshafen
    const Island& h = m_world.islands[static_cast<std::size_t>(isl)];        // Insel
    m_state.docked = isl; m_state.onFoot = true;                             // Im Hafen
    m_state.shipX = h.dockX; m_state.shipY = h.dockY; m_state.shipAngle = h.dockAngle; // Neues Schiff am Steg
    m_state.figX = h.pierEndX; m_state.figY = h.pierEndY;                    // Kapitän am Steg
    m_state.health = std::max(10.0f, m_state.health - 30.0f);                // Der Kapitän ist erschöpft
    m_sunkText = "Dein Schiff (" + oldName + ") ist durch " + cause + " gesunken. " + std::to_string(lostCargo) + " Ladungseinheiten und " + std::to_string(lostArtifacts) + " Artefakte sind verloren. Fischer haben dich nach " + h.name + " gebracht. Die Hafenversicherung stellt dir einen einfachen Kutter."; // Text
    resetTransient();                                                        // Gegner und Effekte weg
    m_panel = Panel::Sunk;                                                   // Fenster zeigen
    sound("kentern");                                                        // Geräusch
    saveGame();                                                              // Speichern
} // Ende von shipSunk

// Piraten entern das Schiff und rauben Ladung
void Game::stealCargo() {                                                    // Beginn von stealCargo
    sound("treffer");                                                        // Geräusch
    m_state.health = std::max(5.0f, m_state.health - 20.0f);                 // Der Kapitän wird verletzt
    int n = static_cast<int>(m_state.hold.size());                           // Ladung
    if (n == 0) {                                                            // Nichts zu holen
        int coins = std::min(m_state.credits, 40 + static_cast<int>(randf() * 80.0f)); // Dann die Bordkasse
        m_state.credits -= coins;                                            // Abziehen
        damageShip(15.0f, "Piraten");                                        // Sie wüten an Bord
        notify("Piraten haben geentert! Ohne Ladung nahmen sie die Bordkasse: " + std::to_string(coins) + " Cr."); // Meldung
        return;                                                              // Fertig
    }                                                                        // Ende ohne Ladung
    int steal = std::max(1, static_cast<int>(static_cast<float>(n) * m_set.pirateSteal + 0.5f)); // Raubmenge
    for (int k = 0; k < steal && !m_state.hold.empty(); ++k) m_state.hold.erase(m_state.hold.begin() + static_cast<long>(randf() * static_cast<float>(m_state.hold.size())) % static_cast<long>(m_state.hold.size())); // Zufällige Einheiten weg
    notify("Piraten haben geentert und " + std::to_string(steal) + " Ladungseinheiten geraubt!"); // Meldung
} // Ende von stealCargo

// Hafen, dessen Liegeplatz in Anlegeweite ist
int Game::nearDock() const {                                                 // Beginn von nearDock
    for (std::size_t i = 0; i < m_world.islands.size(); ++i) if (dist(m_state.shipX, m_state.shipY, m_world.islands[i].dockX, m_world.islands[i].dockY) < m_set.dockDistance) return static_cast<int>(i); // In Reichweite
    return -1;                                                               // Keiner
} // Ende von nearDock

int Game::nearDoor() const { return m_world.nearestBuilding(m_state.figX, m_state.figY, 1.3f); } // Tür in Reichweite

// Steht die Figur am Ende des Stegs, an dem das Schiff liegt?
bool Game::nearPier() const {                                                // Beginn von nearPier
    if (m_state.docked < 0) return false;                                    // Schiff nicht im Hafen
    const Island& isl = m_world.islands[static_cast<std::size_t>(m_state.docked)]; // Hafen
    return dist(m_state.figX, m_state.figY, isl.pierEndX, isl.pierEndY) < 1.4f; // Nah genug
} // Ende von nearPier

// Erzeugt Spritzer
void Game::splash(float x, float y, int count, Color c) {                    // Beginn von splash
    for (int i = 0; i < count; ++i) {                                        // Alle Partikel
        Particle p;                                                          // Neuer Partikel
        p.x = x + (randf() - 0.5f) * 0.5f; p.y = y + (randf() - 0.5f) * 0.5f; // Position
        float a = randf() * 6.283f, sp = 0.5f + randf() * 1.5f;              // Richtung und Tempo
        p.vx = std::cos(a) * sp; p.vy = std::sin(a) * sp; p.vz = 1.5f + randf() * 2.5f; // Nach oben spritzen
        p.life = p.maxLife = 0.6f + randf() * 0.6f;                          // Lebenszeit
        p.size = 2.0f + randf() * 3.0f;                                      // Größe
        p.color = c;                                                         // Farbe
        m_particles.push_back(p);                                            // Speichern
    }                                                                        // Ende der Schleife
} // Ende von splash

// Setzt das Ziel für den Richtungspfeil
void Game::setTarget(float x, float y, const std::string& name) {            // Beginn von setTarget
    m_state.hasTarget = true;                                                // Ziel aktiv
    m_state.targetX = x; m_state.targetY = y;                                // Position
    m_state.targetName = name;                                               // Name
    m_seaPath.clear();                                                       // Alter Seeweg gilt nicht mehr
} // Ende von setTarget

// Berechnet den Seeweg zum Ziel (false = kein Weg gefunden)
bool Game::planRoute() {                                                     // Beginn von planRoute
    if (!m_state.hasTarget) return false;                                    // Kein Ziel
    m_seaPath = m_world.findSeaPath(m_state.shipX, m_state.shipY, m_state.targetX, m_state.targetY, m_state.stats().draft); // A* über das Wasser
    m_repath = 3.0f;                                                         // Nächste Prüfung
    if (m_seaPath.empty()) { notify("Der Navigator findet keinen Seeweg dorthin (zu flach oder an Land)."); return false; } // Kein Weg
    return true;                                                             // Weg gefunden
} // Ende von planRoute
