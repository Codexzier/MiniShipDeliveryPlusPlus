// Player.cpp - Bewegung, Kollision, Enterhaken-Physik (Pendel) und Animationen der Spielfigur
#include "Player.h" // Eigene Deklarationen

#include <algorithm> // std::min, std::max
#include <cmath>     // std::sin, std::cos, std::atan2, std::sqrt, std::fabs

// Liest alle Eigenschaften aus data/spieler.txt (fehlende Werte behalten den Standard)
void Player::loadStats(const PropertyFile& f) {                                // Beginn von loadStats
    const std::string s = "Spieler";                                           // Objektname der Figur
    m_stats.name = f.getString(s, "name", m_stats.name);                       // Name
    m_stats.width = f.getFloat(s, "breite", m_stats.width);                    // Breite der Kollisionsbox
    m_stats.height = f.getFloat(s, "hoehe", m_stats.height);                   // Höhe der Kollisionsbox
    m_stats.handHeight = f.getFloat(s, "hand_hoehe", m_stats.handHeight);      // Höhe der Hände
    m_stats.runSpeed = f.getFloat(s, "lauf_geschwindigkeit", m_stats.runSpeed); // Laufgeschwindigkeit
    m_stats.accelGround = f.getFloat(s, "beschleunigung_boden", m_stats.accelGround); // Beschleunigung am Boden
    m_stats.accelAir = f.getFloat(s, "beschleunigung_luft", m_stats.accelAir); // Beschleunigung in der Luft
    m_stats.jumpSpeed = f.getFloat(s, "sprung_geschwindigkeit", m_stats.jumpSpeed); // Sprungkraft
    m_stats.gravity = f.getFloat(s, "schwerkraft", m_stats.gravity);           // Schwerkraft
    m_stats.maxFall = f.getFloat(s, "max_fallgeschwindigkeit", m_stats.maxFall); // Maximale Fallgeschwindigkeit
    m_stats.maxHealth = f.getFloat(s, "leben_max", m_stats.maxHealth);         // Maximales Leben
    m_stats.maxStamina = f.getFloat(s, "ausdauer_max", m_stats.maxStamina);    // Maximale Ausdauer
    m_stats.staminaRegen = f.getFloat(s, "ausdauer_erholung", m_stats.staminaRegen); // Erholung pro Sekunde
    m_stats.jumpCost = f.getFloat(s, "sprung_ausdauer", m_stats.jumpCost);     // Ausdauer pro Sprung
    m_stats.startCoins = f.getInt(s, "start_coins", m_stats.startCoins);       // Start-Coins
    m_stats.invulnerableTime = f.getFloat(s, "unverwundbar_nach_schaden", m_stats.invulnerableTime); // Schutzzeit
    m_stats.respawnDelay = f.getFloat(s, "wiedererscheinen_nach", m_stats.respawnDelay); // Wartezeit nach Sturz
    const std::string h = "Enterhaken_Physik";                                 // Objektname der Hakenwerte
    m_stats.hookRange = f.getFloat(h, "reichweite", m_stats.hookRange);        // Reichweite
    m_stats.ropeLength = f.getFloat(h, "seil_laenge", m_stats.ropeLength);     // Seillänge
    m_stats.reelSpeed = f.getFloat(h, "einzug_geschwindigkeit", m_stats.reelSpeed); // Einzugsgeschwindigkeit
    m_stats.hookCost = f.getFloat(h, "ausdauer_kosten", m_stats.hookCost);     // Ausdauerkosten
    m_stats.swingDamping = f.getFloat(h, "pendel_daempfung", m_stats.swingDamping); // Dämpfung
    m_stats.swingPump = f.getFloat(h, "schwung_kraft", m_stats.swingPump);     // Schwungkraft
    m_stats.releaseBoost = f.getFloat(h, "abwurf_verstaerkung", m_stats.releaseBoost); // Verstärkung beim Loslassen
    m_stats.releaseUp = f.getFloat(h, "abwurf_hoch", m_stats.releaseUp);       // Schwung nach oben
} // Ende von loadStats

// Setzt die Figur für ein neues Spiel zurück
void Player::reset(float startX, float groundY) {                              // Beginn von reset
    x = startX;                                                                // Startposition x
    y = groundY;                                                               // Füße auf dem Boden
    vx = 0.0f;                                                                 // Keine Bewegung
    vy = 0.0f;                                                                 // Keine Bewegung
    onGround = true;                                                           // Steht auf dem Boden
    facing = 1;                                                                // Blick nach rechts
    health = m_stats.maxHealth;                                                // Volles Leben
    stamina = m_stats.maxStamina;                                              // Volle Ausdauer
    coins = m_stats.startCoins;                                                // Start-Coins
    invulnerable = 0.0f;                                                       // Nicht unverwundbar
    lastSafeX = startX;                                                        // Startposition ist sicher
    state = PlayerState::Normal;                                               // Normaler Zustand
    m_landing = false;                                                         // Keine Landung
    m_hookFlight = false;                                                      // Kein Flug nach dem Haken
    m_pullPhase = false;                                                       // Kein Hochschwingen
    m_retract = 0.0f;                                                          // Kein Seileinzug
    m_coyote = 0.0f;                                                           // Keine Kantenzeit
    m_jumpBuffer = 0.0f;                                                       // Kein gespeicherter Sprung
} // Ende von reset

// Kollisionsbox: Füße unten in der Mitte
RectF Player::bounds() const { return RectF{x - m_stats.width * 0.5f, y - m_stats.height, m_stats.width, m_stats.height}; } // Box berechnen

// Prüft, ob eine Box eine feste Fläche der Karte berührt
bool Player::collides(const RectF& box, const Level& level) const {           // Beginn von collides
    for (const RectF& s : level.solids()) {                                    // Alle festen Flächen
        if (box.intersects(s)) return true;                                    // Überschneidung gefunden
    }                                                                          // Ende der Schleife
    return false;                                                              // Keine Überschneidung
} // Ende von collides

// Dauer der Wurfanimation (Bilder / Bilder pro Sekunde)
float Player::throwDuration() const {                                          // Beginn von throwDuration
    if (!m_anims) return 0.25f;                                                // Ohne Animationsdaten: Standardwert
    const AnimationDef& def = m_anims->get(PlayerAnim::HookThrow);             // Definition der Wurfanimation
    return static_cast<float>(def.frames) / def.fps;                           // Dauer in Sekunden
} // Ende von throwDuration

// Position der Hände (dort beginnt das Seil)
void Player::handPosition(float& hx, float& hy) const {                        // Beginn von handPosition
    if (state == PlayerState::HookSwing) {                                     // Beim Hängen: Hände oben in der Mitte
        hx = x;                                                                // Mitte
        hy = y - m_stats.handHeight;                                           // Über dem Kopf
    } else {                                                                   // Beim Werfen: Hand vorne oben
        hx = x + static_cast<float>(facing) * 0.15f;                           // Etwas vor der Figur
        hy = y - m_stats.height * 0.85f;                                       // Auf Schulterhöhe
    }                                                                          // Ende der Unterscheidung
} // Ende von handPosition

// Bewegt die Figur erst waagerecht, dann senkrecht und löst Kollisionen auf
void Player::moveAndCollide(float dt, const Level& level) {                    // Beginn von moveAndCollide
    const float eps = 0.0005f;                                                 // Kleiner Abstand, damit die Box nicht klebt
    x += vx * dt;                                                              // Waagerecht bewegen
    for (const RectF& s : level.solids()) {                                    // Alle festen Flächen prüfen
        RectF box = bounds();                                                  // Aktuelle Box
        if (!box.intersects(s)) continue;                                      // Keine Überschneidung
        if (vx > 0.0f) x = s.x - m_stats.width * 0.5f - eps;                   // Nach rechts gelaufen -> vor die Wand setzen
        else if (vx < 0.0f) x = s.right() + m_stats.width * 0.5f + eps;        // Nach links gelaufen -> hinter die Wand setzen
        vx = 0.0f;                                                             // Bewegung stoppen
    }                                                                          // Ende der waagerechten Prüfung
    x = clampValue(x, 0.25f, level.length - 0.25f);                            // Nicht aus der Karte laufen
    bool wasOnGround = onGround;                                               // Vorherigen Bodenkontakt merken
    onGround = false;                                                          // Wird gleich neu bestimmt
    y += vy * dt;                                                              // Senkrecht bewegen
    for (const RectF& s : level.solids()) {                                    // Alle festen Flächen prüfen
        RectF box = bounds();                                                  // Aktuelle Box
        if (!box.intersects(s)) continue;                                      // Keine Überschneidung
        if (vy >= 0.0f) {                                                      // Beim Fallen
            y = s.y - eps;                                                     // Auf die Fläche stellen
            onGround = true;                                                   // Bodenkontakt
        } else {                                                               // Beim Steigen
            y = s.bottom() + m_stats.height + eps;                             // Unter die Fläche setzen (Kopfstoß)
        }                                                                      // Ende der Unterscheidung
        vy = 0.0f;                                                             // Senkrechte Bewegung stoppen
    }                                                                          // Ende der senkrechten Prüfung
    if (wasOnGround && !onGround && vy >= 0.0f) m_coyote = 0.1f;               // Gerade über eine Kante gelaufen
} // Ende von moveAndCollide

// Sucht den nächstgelegenen erreichbaren Ankerpunkt
const Anchor* Player::findAnchor(const Level& level) const {                   // Beginn von findAnchor
    float hx = x;                                                              // Handposition x (Mitte)
    float hy = y - m_stats.handHeight;                                         // Handposition y (über dem Kopf)
    const Anchor* best = nullptr;                                              // Bester Kandidat
    float bestDist = m_stats.hookRange;                                        // Größte erlaubte Entfernung
    for (const Anchor& a : level.anchors) {                                    // Alle Ankerpunkte
        float dx = a.x - hx;                                                   // Abstand x
        float dy = a.y - hy;                                                   // Abstand y
        float dist = std::sqrt(dx * dx + dy * dy);                             // Entfernung
        if (dist > bestDist) continue;                                         // Zu weit weg
        if (a.y > hy - 0.3f) continue;                                         // Muss über der Figur hängen
        if (dx * static_cast<float>(facing) < -0.6f) continue;                 // Darf nicht hinter der Figur liegen
        best = &a;                                                             // Neuer bester Kandidat
        bestDist = dist;                                                       // Seine Entfernung merken
    }                                                                          // Ende der Schleife
    return best;                                                               // Ergebnis (oder nullptr)
} // Ende von findAnchor

// Löst den Enterhaken aus (Q)
void Player::tryHook(const Level& level, bool hasHook, PlayerEvents& events) { // Beginn von tryHook
    if (!hasHook) { events.noHook = true; return; }                            // Ohne Enterhaken geht es nicht
    if (stamina < m_stats.hookCost) { events.noStamina = true; return; }       // Zu wenig Ausdauer
    stamina -= m_stats.hookCost;                                               // Ausdauer abziehen
    const Anchor* anchor = findAnchor(level);                                  // Ankerpunkt suchen
    state = PlayerState::HookThrow;                                            // Wurf beginnt
    m_hookTimer = 0.0f;                                                        // Wurfzeit zurücksetzen
    m_hookHasTarget = anchor != nullptr;                                       // Gibt es ein Ziel?
    float hx = 0.0f;                                                           // Handposition x
    float hy = 0.0f;                                                           // Handposition y
    handPosition(hx, hy);                                                      // Handposition berechnen
    if (anchor) {                                                              // Ziel gefunden
        m_hookTargetX = anchor->x;                                             // Haken fliegt zum Anker (x)
        m_hookTargetY = anchor->y;                                             // Haken fliegt zum Anker (y)
    } else {                                                                   // Kein Ziel: schräg nach oben werfen
        m_hookTargetX = hx + static_cast<float>(facing) * m_stats.hookRange * 0.7f; // Ins Leere nach vorne
        m_hookTargetY = hy - m_stats.hookRange * 0.7f;                         // Ins Leere nach oben
    }                                                                          // Ende der Unterscheidung
} // Ende von tryHook

// Ein Physik-Schritt (wird mehrmals pro Bild mit kleinem dt aufgerufen)
void Player::update(float dt, const PlayerInput& input, const Level& level, bool hasHook, float maxStamina, PlayerEvents& events) { // Beginn von update
    invulnerable = std::max(0.0f, invulnerable - dt);                          // Schutzzeit herunterzählen
    m_retract = std::max(0.0f, m_retract - dt);                                // Seileinzug herunterzählen
    if (state == PlayerState::Respawning) {                                    // Figur ist gerade hineingefallen
        m_respawnTimer -= dt;                                                  // Wartezeit herunterzählen
        if (m_respawnTimer <= 0.0f) {                                          // Wartezeit vorbei
            x = m_respawnX;                                                    // An die sichere Stelle setzen (x)
            y = m_respawnY;                                                    // An die sichere Stelle setzen (y)
            vx = 0.0f;                                                         // Keine Bewegung
            vy = 0.0f;                                                         // Keine Bewegung
            onGround = true;                                                   // Auf dem Boden
            state = PlayerState::Normal;                                       // Wieder normal spielbar
            invulnerable = m_stats.invulnerableTime;                           // Kurz unverwundbar (blinkt)
        }                                                                      // Ende der Prüfung
        return;                                                                // Sonst passiert nichts
    }                                                                          // Ende Respawn
    switch (state) {                                                           // Je nach Zustand
    case PlayerState::Normal: updateNormal(dt, input, level, hasHook, events); break; // Laufen/Springen
    case PlayerState::HookThrow: updateThrow(dt, level, events); break;        // Haken fliegt
    case PlayerState::HookSwing: updateSwing(dt, input, level, events); break; // Schwingen
    default: break;                                                            // Sonst nichts
    }                                                                          // Ende der Fallunterscheidung
    if (onGround && state == PlayerState::Normal) {                            // Am Boden erholt sich die Ausdauer
        stamina = std::min(maxStamina, stamina + m_stats.staminaRegen * dt);   // Ausdauer auffüllen
        if (!level.ditchAt(x - m_stats.width * 0.5f) && !level.ditchAt(x + m_stats.width * 0.5f)) lastSafeX = x; // Sichere Position merken
    }                                                                          // Ende der Erholung
    stamina = std::min(stamina, maxStamina);                                   // Nie über dem Maximum
} // Ende von update

// Normaler Zustand: Laufen, Springen, Fallen
void Player::updateNormal(float dt, const PlayerInput& input, const Level& level, bool hasHook, PlayerEvents& events) { // Beginn von updateNormal
    float target = static_cast<float>(input.moveDir) * m_stats.runSpeed;       // Gewünschte Geschwindigkeit
    if (onGround) {                                                            // Am Boden
        vx = approach(vx, target, m_stats.accelGround * dt);                   // Schnell auf Zielgeschwindigkeit
    } else if (input.moveDir != 0 && static_cast<float>(input.moveDir) * vx > m_stats.runSpeed) { // In der Luft schneller als Laufen in gleicher Richtung
        vx = approach(vx, target, m_stats.accelAir * 0.1f * dt);               // Schwung vom Seil fast erhalten
    } else {                                                                   // Sonstige Luftsteuerung
        vx = approach(vx, target, m_stats.accelAir * dt);                      // Sanft lenken
    }                                                                          // Ende der Beschleunigung
    if (input.moveDir != 0) facing = input.moveDir;                            // Blickrichtung anpassen
    m_coyote = std::max(0.0f, m_coyote - dt);                                  // Kantenzeit herunterzählen
    m_jumpBuffer = std::max(0.0f, m_jumpBuffer - dt);                          // Sprungwunsch herunterzählen
    if (input.jump) m_jumpBuffer = 0.12f;                                      // Sprungwunsch kurz merken
    if (m_jumpBuffer > 0.0f && (onGround || m_coyote > 0.0f)) {                // Springen möglich?
        m_jumpBuffer = 0.0f;                                                   // Wunsch verbraucht
        if (stamina >= m_stats.jumpCost) {                                     // Genug Ausdauer?
            vy = -m_stats.jumpSpeed;                                           // Nach oben abspringen
            stamina -= m_stats.jumpCost;                                       // Ausdauer abziehen
            onGround = false;                                                  // In der Luft
            m_coyote = 0.0f;                                                   // Kantenzeit verbraucht
            m_landing = false;                                                 // Eventuelle Landung abbrechen
            events.jumped = true;                                              // Ereignis melden
        } else {                                                               // Nicht genug Ausdauer
            events.noStamina = true;                                           // Ereignis melden
        }                                                                      // Ende der Ausdauerprüfung
    }                                                                          // Ende Springen
    if (input.hook) {                                                          // Q gedrückt
        tryHook(level, hasHook, events);                                       // Enterhaken versuchen
        if (state != PlayerState::Normal) return;                              // Wurf hat begonnen
    }                                                                          // Ende Enterhaken
    bool wasOnGround = onGround;                                               // Bodenkontakt vor der Bewegung
    vy = std::min(vy + m_stats.gravity * dt, m_stats.maxFall);                 // Schwerkraft anwenden
    moveAndCollide(dt, level);                                                 // Bewegen und Kollisionen lösen
    if (!wasOnGround && onGround) {                                            // Gerade gelandet
        m_landing = true;                                                      // Landeanimation starten
        m_landingRestart = true;                                               // Neu beginnen
        m_landingAnim = m_hookFlight ? PlayerAnim::HookLand : PlayerAnim::Land; // Normale oder Haken-Landung
        m_hookFlight = false;                                                  // Flug beendet
        events.landed = true;                                                  // Ereignis melden
    }                                                                          // Ende Landung
} // Ende von updateNormal

// Der Haken fliegt zum Ziel
void Player::updateThrow(float dt, const Level& level, PlayerEvents& events) { // Beginn von updateThrow
    m_hookTimer += dt;                                                         // Wurfzeit erhöhen
    if (onGround) vx = approach(vx, 0.0f, m_stats.accelGround * dt);           // Am Boden abbremsen
    vy = std::min(vy + m_stats.gravity * dt, m_stats.maxFall);                 // Schwerkraft wirkt weiter
    moveAndCollide(dt, level);                                                 // Bewegen
    if (m_hookTimer < throwDuration()) return;                                 // Haken fliegt noch
    if (!m_hookHasTarget) {                                                    // Kein Ziel getroffen
        state = PlayerState::Normal;                                           // Zurück zum normalen Zustand
        m_retract = 0.2f;                                                      // Seil einziehen
        events.hookMissed = true;                                              // Ereignis melden
        return;                                                                // Fertig
    }                                                                          // Ende Fehlwurf
    state = PlayerState::HookSwing;                                            // Haken greift -> schwingen
    float hx = x;                                                              // Handposition beim Hängen (x)
    float hy = y - m_stats.handHeight;                                         // Handposition beim Hängen (y)
    float rx = hx - m_hookTargetX;                                             // Abstand zum Anker (x)
    float ry = hy - m_hookTargetY;                                             // Abstand zum Anker (y)
    m_ropeLength = std::max(0.3f, std::sqrt(rx * rx + ry * ry));               // Anfangslänge des Seils
    m_angle = std::atan2(rx, ry);                                              // Pendelwinkel (0 = senkrecht darunter)
    float tangentX = std::cos(m_angle);                                        // Bewegungsrichtung des Pendels (x)
    float tangentY = -std::sin(m_angle);                                       // Bewegungsrichtung des Pendels (y)
    m_pullSpeed = clampValue(vx * tangentX + vy * tangentY, -5.0f, 5.0f);      // Laufgeschwindigkeit entlang der Pendelbahn merken
    m_angularVel = m_pullSpeed / m_ropeLength;                                 // In Winkelgeschwindigkeit umrechnen
    onGround = false;                                                          // Figur hängt in der Luft
    m_pullPhase = true;                                                        // Mit Hochschwingen beginnen
    m_landing = false;                                                         // Keine Landung
    events.hookAttached = true;                                                // Ereignis melden
} // Ende von updateThrow

// Pendelbewegung am Seil
void Player::updateSwing(float dt, const PlayerInput& input, const Level& level, PlayerEvents& events) { // Beginn von updateSwing
    bool reeling = m_ropeLength > m_stats.ropeLength;                          // Wird das Seil noch eingezogen (Hochschwingen)?
    if (reeling) {                                                             // Figur zieht sich am Seil hoch
        m_ropeLength = std::max(m_stats.ropeLength, m_ropeLength - m_stats.reelSpeed * dt); // Seil einziehen, Winkel bleibt gleich
        m_angularVel = 0.0f;                                                   // Während des Hochziehens kein Pendeln
        if (m_ropeLength <= m_stats.ropeLength) m_angularVel = m_pullSpeed / m_ropeLength; // Oben angekommen: Schwung aus dem Anlauf übernehmen
    } else {                                                                   // Normales Pendeln
        m_angularVel = clampValue(m_angularVel, -6.0f, 6.0f);                  // Zu schnelles Pendeln verhindern
        float alpha = -(m_stats.gravity / m_ropeLength) * std::sin(m_angle);   // Rückstellbeschleunigung durch die Schwerkraft
        alpha -= m_stats.swingDamping * m_angularVel;                          // Dämpfung (Luftwiderstand)
        if (input.moveDir != 0 && static_cast<float>(input.moveDir) * m_angularVel >= 0.0f) { // A/D in Bewegungsrichtung gedrückt?
            alpha += static_cast<float>(input.moveDir) * m_stats.swingPump / m_ropeLength; // Dann Schwung holen (Energie zufügen)
        }                                                                      // Ende Schwung holen
        m_angularVel += alpha * dt;                                            // Winkelgeschwindigkeit ändern
        m_angle += m_angularVel * dt;                                          // Winkel ändern
    }                                                                          // Ende der Unterscheidung
    const float limit = 1.5f;                                                  // Größter erlaubter Winkel (ca. 86 Grad)
    if (m_angle > limit) { m_angle = limit; m_angularVel = std::min(m_angularVel, 0.0f); }   // Rechte Grenze
    if (m_angle < -limit) { m_angle = -limit; m_angularVel = std::max(m_angularVel, 0.0f); } // Linke Grenze
    float newX = m_hookTargetX + m_ropeLength * std::sin(m_angle);             // Neue Handposition x
    float newY = m_hookTargetY + m_ropeLength * std::cos(m_angle) + m_stats.handHeight; // Neue Fußposition y
    RectF box{newX - m_stats.width * 0.5f, newY - m_stats.height, m_stats.width, m_stats.height}; // Box an der neuen Position
    if (collides(box, level)) {                                                // Pendel stößt gegen den Boden oder eine Wand
        state = PlayerState::Normal;                                           // Seil lösen
        vx = 0.0f;                                                             // Bewegung stoppen
        vy = 0.0f;                                                             // Bewegung stoppen
        m_retract = 0.2f;                                                      // Seil einziehen
        m_hookFlight = true;                                                   // Landung nach dem Haken
        events.released = true;                                                // Ereignis melden
        return;                                                                // Fertig
    }                                                                          // Ende Kollision
    x = clampValue(newX, 0.25f, level.length - 0.25f);                         // Position übernehmen (x)
    y = newY;                                                                  // Position übernehmen (y)
    vx = m_ropeLength * m_angularVel * std::cos(m_angle);                      // Bahngeschwindigkeit x (für das Loslassen)
    vy = -m_ropeLength * m_angularVel * std::sin(m_angle);                     // Bahngeschwindigkeit y (für das Loslassen)
    if (std::fabs(m_angularVel) > 0.3f) facing = m_angularVel > 0.0f ? 1 : -1; // Blickrichtung in Schwungrichtung
    else if (reeling && std::fabs(m_angle) > 0.1f) facing = m_angle < 0.0f ? 1 : -1; // Beim Hochziehen zum Anker schauen
    if (input.jump || input.hook) {                                            // Leertaste oder Q: loslassen
        state = PlayerState::Normal;                                           // Wieder frei
        vx *= m_stats.releaseBoost;                                            // Schwung verstärken (x)
        vy = vy * m_stats.releaseBoost - m_stats.releaseUp;                    // Schwung verstärken und etwas nach oben
        m_hookFlight = true;                                                   // Landung nach dem Haken
        m_retract = 0.2f;                                                      // Seil einziehen
        events.released = true;                                                // Ereignis melden
    }                                                                          // Ende Loslassen
} // Ende von updateSwing

// Lässt die Figur nach einem Sturz kurz verschwinden und an sicherer Stelle wieder erscheinen
void Player::startRespawn(float respawnX, float groundY) {                     // Beginn von startRespawn
    state = PlayerState::Respawning;                                           // Zustand wechseln
    m_respawnTimer = m_stats.respawnDelay;                                     // Wartezeit setzen
    m_respawnX = respawnX;                                                     // Zielposition x
    m_respawnY = groundY;                                                      // Zielposition y
    m_hookFlight = false;                                                      // Kein Flug mehr
    m_landing = false;                                                         // Keine Landung mehr
} // Ende von startRespawn

// Zieht Leben ab, wenn die Figur nicht gerade geschützt ist
bool Player::damage(float amount) {                                            // Beginn von damage
    if (invulnerable > 0.0f) return false;                                     // Geschützt -> kein Schaden
    health = std::max(0.0f, health - amount);                                  // Leben abziehen (nie unter 0)
    invulnerable = m_stats.invulnerableTime;                                   // Kurz unverwundbar
    return true;                                                               // Schaden wurde genommen
} // Ende von damage

// Wählt die passende Animation zum aktuellen Zustand
void Player::updateAnimation(float dt) {                                       // Beginn von updateAnimation
    if (!m_anims) return;                                                      // Ohne Definitionen keine Animation
    auto play = [&](PlayerAnim a, bool restart) { m_anim.play(a, m_anims->get(a), restart); }; // Hilfsfunktion zum Starten
    switch (state) {                                                           // Je nach Zustand
    case PlayerState::Respawning:                                              // Unsichtbar -> nichts ändern
        break;                                                                 // Ende Respawn
    case PlayerState::HookThrow: {                                             // Haken werfen
        play(PlayerAnim::HookThrow, false);                                    // Wurfanimation
        const AnimationDef& def = m_anims->get(PlayerAnim::HookThrow);         // Definition
        int frame = static_cast<int>(m_hookTimer / throwDuration() * static_cast<float>(def.frames)); // Bild passend zum Wurffortschritt
        m_anim.setFrame(frame);                                                // Bild setzen
        break;                                                                 // Ende Wurf
    }                                                                          // Ende case HookThrow
    case PlayerState::HookSwing:                                               // Am Seil
        if (m_pullPhase) {                                                     // Zuerst hochschwingen
            play(PlayerAnim::HookPull, false);                                 // Hochschwing-Animation
            m_anim.update(dt);                                                 // Weiterlaufen lassen
            if (m_anim.finished()) m_pullPhase = false;                        // Danach normal schwingen
        } else {                                                               // Normales Schwingen
            bool away = m_angularVel * m_angle > 0.0f;                         // Entfernt sich die Figur vom tiefsten Punkt?
            play(away ? PlayerAnim::SwingUp : PlayerAnim::SwingDown, false);   // Aufschwung oder Abschwung
            m_anim.update(dt);                                                 // Weiterlaufen lassen
        }                                                                      // Ende der Unterscheidung
        break;                                                                 // Ende Schwingen
    case PlayerState::Normal:                                                  // Normaler Zustand
        if (!onGround) {                                                       // In der Luft
            play(PlayerAnim::Jump, false);                                     // Sprunganimation
            float j = m_stats.jumpSpeed;                                       // Absprunggeschwindigkeit als Maßstab
            int frame = vy < -j * 0.45f ? 0 : (vy < 0.0f ? 1 : (vy < j * 0.45f ? 2 : 3)); // 2 Bilder aufwärts, 2 abwärts
            m_anim.setFrame(frame);                                            // Bild setzen
        } else if (m_landing) {                                                // Landung läuft
            play(m_landingAnim, m_landingRestart);                             // Landeanimation
            m_landingRestart = false;                                          // Nur einmal neu starten
            m_anim.update(dt);                                                 // Weiterlaufen lassen
            if (m_anim.finished()) m_landing = false;                          // Fertig gelandet
        } else if (std::fabs(vx) > 0.15f) {                                    // Figur läuft
            play(facing > 0 ? PlayerAnim::WalkRight : PlayerAnim::WalkLeft, false); // Laufanimation in Blickrichtung
            m_anim.update(dt, std::fabs(vx) / m_stats.runSpeed);               // Je schneller, desto schneller die Beine
        } else {                                                               // Figur steht
            play(PlayerAnim::Idle, false);                                     // Standanimation
            m_anim.update(dt);                                                 // Weiterlaufen lassen
        }                                                                      // Ende der Unterscheidung
        break;                                                                 // Ende Normal
    }                                                                          // Ende der Fallunterscheidung
} // Ende von updateAnimation

// Zeichnet das Seil des Enterhakens
void Player::drawRope(Canvas& canvas, float camX, int tile) const {            // Beginn von drawRope
    if (state == PlayerState::Respawning) return;                              // Unsichtbar -> kein Seil
    const float T = static_cast<float>(tile);                                  // Kachelgröße
    float hx = 0.0f;                                                           // Handposition x
    float hy = 0.0f;                                                           // Handposition y
    handPosition(hx, hy);                                                      // Handposition berechnen
    float tipX = 0.0f;                                                         // Hakenspitze x
    float tipY = 0.0f;                                                         // Hakenspitze y
    if (state == PlayerState::HookThrow) {                                     // Haken fliegt
        float t = clampValue(m_hookTimer / throwDuration(), 0.0f, 1.0f);       // Wurffortschritt
        tipX = lerp(hx, m_hookTargetX, t);                                     // Spitze zwischen Hand und Ziel (x)
        tipY = lerp(hy, m_hookTargetY, t);                                     // Spitze zwischen Hand und Ziel (y)
    } else if (state == PlayerState::HookSwing) {                              // Haken hängt am Anker
        tipX = m_hookTargetX;                                                  // Spitze am Anker (x)
        tipY = m_hookTargetY;                                                  // Spitze am Anker (y)
    } else if (m_retract > 0.0f) {                                             // Seil wird eingezogen
        float t = m_retract / 0.2f;                                            // 1 = ganz ausgefahren, 0 = eingezogen
        tipX = lerp(hx, m_hookTargetX, t);                                     // Spitze wandert zurück (x)
        tipY = lerp(hy, m_hookTargetY, t);                                     // Spitze wandert zurück (y)
    } else {                                                                   // Kein Seil sichtbar
        return;                                                                // Nichts zeichnen
    }                                                                          // Ende der Unterscheidung
    float ax = (hx - camX) * T;                                                // Hand auf dem Bildschirm (x)
    float ay = hy * T;                                                         // Hand auf dem Bildschirm (y)
    float bx = (tipX - camX) * T;                                              // Spitze auf dem Bildschirm (x)
    float by = tipY * T;                                                       // Spitze auf dem Bildschirm (y)
    float thick = std::max(1.0f, T / 40.0f);                                   // Seildicke
    canvas.lineF(ax, ay, bx, by, rgba(150, 105, 60), thick);                   // Seil zeichnen
    int claw = std::max(2, tile / 14);                                         // Größe der Hakenkrallen
    int cx = static_cast<int>(bx);                                             // Hakenmitte x
    int cy = static_cast<int>(by);                                             // Hakenmitte y
    canvas.line(cx, cy, cx - claw, cy - claw, rgba(190, 190, 200), std::max(1, tile / 48)); // Linke Kralle
    canvas.line(cx, cy, cx + claw, cy - claw, rgba(190, 190, 200), std::max(1, tile / 48)); // Rechte Kralle
    canvas.fillCircle(cx, cy, std::max(1, tile / 40), rgba(140, 140, 150));    // Hakenkopf
} // Ende von drawRope

// Zeichnet die Figur mit dem passenden Bild aus dem Sprite-Sheet
void Player::draw(Canvas& canvas, const Assets& assets, float camX, float time) const { // Beginn von draw
    if (state == PlayerState::Respawning) return;                              // Unsichtbar
    if (invulnerable > 0.0f && static_cast<int>(time * 12.0f) % 2 == 0) return; // Blinken nach Schaden
    const Image& sheet = assets.get("spieler");                                // Sprite-Sheet
    int S = assets.tileSize();                                                 // Größe einer Zelle
    const AnimationDef& def = m_anim.def();                                    // Aktuelle Animation
    RectI src{m_anim.frame() * S, def.row * S, S, S};                          // Ausschnitt im Sprite-Sheet
    PlayerAnim current = m_anim.current();                                     // Art der Animation
    bool flip = facing < 0 && current != PlayerAnim::WalkLeft && current != PlayerAnim::WalkRight; // Andere Animationen werden gespiegelt
    int left = static_cast<int>(std::floor((x - camX) * static_cast<float>(S))) - S / 2; // Linke Kante auf dem Bildschirm
    int top = static_cast<int>(std::floor(y * static_cast<float>(S))) - S;      // Obere Kante (Füße unten)
    canvas.blitRegion(sheet, src, left, top, flip);                            // Figur zeichnen
} // Ende von draw
