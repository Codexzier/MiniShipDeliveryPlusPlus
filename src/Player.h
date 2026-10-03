// Player.h - Die Spielfigur: Laufen, Springen, Enterhaken, Ausdauer, Leben und Animationen
#pragma once // Header nur einmal einbinden

#include "Animation.h"    // Animationen der Figur
#include "Assets.h"       // Sprite-Sheet
#include "Graphics.h"     // Canvas
#include "Level.h"        // Kollisionen und Ankerpunkte
#include "PropertyFile.h" // Eigenschaften aus data/spieler.txt

// Zustand der Bewegung
enum class PlayerState { // Beginn der Aufzählung
    Normal,              // Laufen, Springen, Fallen
    HookThrow,           // Enterhaken wird geworfen
    HookSwing,           // Figur schwingt am Seil
    Respawning           // Figur ist hineingefallen und erscheint gleich wieder
}; // Ende der Aufzählung PlayerState

// Alle Eigenschaften der Figur (werden aus der Textdatei gelesen)
struct PlayerStats {                    // Beginn der Struktur
    std::string name = "Paketbote";     // Name der Figur
    float width = 0.4f;                 // Breite der Kollisionsbox (Kacheln)
    float height = 0.86f;               // Höhe der Kollisionsbox (Kacheln)
    float handHeight = 0.93f;           // Höhe der Hände über den Füßen beim Hängen am Seil
    float runSpeed = 3.2f;              // Laufgeschwindigkeit (Kacheln pro Sekunde)
    float accelGround = 30.0f;          // Beschleunigung am Boden
    float accelAir = 12.0f;             // Beschleunigung in der Luft
    float jumpSpeed = 8.6f;             // Absprunggeschwindigkeit
    float gravity = 22.0f;              // Schwerkraft
    float maxFall = 14.0f;              // Maximale Fallgeschwindigkeit
    float maxHealth = 100.0f;           // Maximales Leben
    float maxStamina = 100.0f;          // Maximale Ausdauer (ohne Gegenstände)
    float staminaRegen = 14.0f;         // Ausdauer-Erholung pro Sekunde
    float jumpCost = 10.0f;             // Ausdauerkosten eines Sprungs
    int startCoins = 40;                // Coins zu Spielbeginn
    float invulnerableTime = 1.5f;      // Unverwundbare Zeit nach Schaden
    float respawnDelay = 0.9f;          // Wartezeit nach einem Sturz
    float hookRange = 4.6f;             // Reichweite des Enterhakens
    float ropeLength = 2.5f;            // Seillänge nach dem Hochziehen
    float reelSpeed = 4.0f;             // Geschwindigkeit des Seileinzugs
    float hookCost = 15.0f;             // Ausdauerkosten des Enterhakens
    float swingDamping = 0.15f;         // Dämpfung der Pendelbewegung
    float swingPump = 4.0f;             // Schwungkraft durch A/D am Seil
    float releaseBoost = 1.25f;         // Verstärkung der Geschwindigkeit beim Loslassen
    float releaseUp = 2.0f;             // Zusätzlicher Schwung nach oben beim Loslassen
}; // Ende der Struktur PlayerStats

// Tastatureingaben für einen Spielschritt
struct PlayerInput {           // Beginn der Struktur
    int moveDir = 0;           // -1 = links (A), 0 = keine Richtung, 1 = rechts (D)
    bool jump = false;         // Leertaste wurde gedrückt
    bool hook = false;         // Q wurde gedrückt
}; // Ende der Struktur PlayerInput

// Ereignisse eines Spielschritts (für Meldungen und Effekte)
struct PlayerEvents {          // Beginn der Struktur
    bool jumped = false;       // Figur ist gesprungen
    bool noStamina = false;    // Zu wenig Ausdauer
    bool noHook = false;       // Kein Enterhaken im Inventar
    bool hookMissed = false;   // Kein Ankerpunkt in Reichweite
    bool hookAttached = false; // Haken hat gegriffen
    bool released = false;     // Seil losgelassen
    bool landed = false;       // Figur ist gelandet
}; // Ende der Struktur PlayerEvents

class Player {                                                       // Beginn der Klasse Player
public:                                                              // Öffentliche Schnittstelle
    void loadStats(const PropertyFile& file);                        // Eigenschaften aus der Datei lesen
    void setAnimations(const AnimationSet* anims) { m_anims = anims; } // Animationsdefinitionen setzen
    void reset(float x, float groundY);                              // Für ein neues Spiel zurücksetzen
    void update(float dt, const PlayerInput& input, const Level& level, bool hasHook, float maxStamina, PlayerEvents& events); // Physik-Schritt
    void updateAnimation(float dt);                                  // Animation passend zum Zustand wählen
    void drawRope(Canvas& canvas, float camX, int tile) const;       // Seil des Enterhakens zeichnen
    void draw(Canvas& canvas, const Assets& assets, float camX, float time) const; // Figur zeichnen
    void startRespawn(float respawnX, float groundY);                // Nach einem Sturz neu erscheinen lassen
    bool damage(float amount);                                       // Schaden nehmen (false, wenn gerade unverwundbar)
    const Anchor* findAnchor(const Level& level) const;              // Besten Ankerpunkt in Reichweite suchen
    RectF bounds() const;                                            // Kollisionsbox in Weltkoordinaten
    const PlayerStats& stats() const { return m_stats; }             // Eigenschaften lesen

    float x = 0.0f;              // Mittelpunkt der Füße (x)
    float y = 0.0f;              // Unterkante der Füße (y)
    float vx = 0.0f;             // Geschwindigkeit x
    float vy = 0.0f;             // Geschwindigkeit y
    bool onGround = false;       // Steht die Figur auf dem Boden?
    int facing = 1;              // Blickrichtung: 1 = rechts, -1 = links
    float health = 100.0f;       // Aktuelles Leben
    float stamina = 100.0f;      // Aktuelle Ausdauer
    int coins = 0;               // Coins
    float invulnerable = 0.0f;   // Restzeit der Unverwundbarkeit
    float lastSafeX = 0.0f;      // Letzte sichere Position auf dem Boden
    PlayerState state = PlayerState::Normal; // Bewegungszustand

private:                                                             // Interne Hilfsfunktionen und Daten
    void updateNormal(float dt, const PlayerInput& input, const Level& level, bool hasHook, PlayerEvents& events); // Normaler Zustand
    void updateThrow(float dt, const Level& level, PlayerEvents& events); // Haken fliegt
    void updateSwing(float dt, const PlayerInput& input, const Level& level, PlayerEvents& events); // Am Seil schwingen
    void tryHook(const Level& level, bool hasHook, PlayerEvents& events); // Enterhaken auslösen
    void moveAndCollide(float dt, const Level& level);               // Bewegen und mit dem Boden kollidieren
    bool collides(const RectF& box, const Level& level) const;       // Überschneidet eine Box feste Flächen?
    float throwDuration() const;                                     // Dauer der Wurfanimation
    void handPosition(float& hx, float& hy) const;                   // Position der Hände (Seilende)

    PlayerStats m_stats;                     // Eigenschaften der Figur
    const AnimationSet* m_anims = nullptr;   // Animationsdefinitionen
    AnimationPlayer m_anim;                  // Abspieler der Animationen
    bool m_landing = false;                  // Läuft gerade eine Landeanimation?
    bool m_landingRestart = false;           // Muss die Landeanimation neu starten?
    PlayerAnim m_landingAnim = PlayerAnim::Land; // Welche Landeanimation (normal oder nach dem Haken)
    bool m_hookFlight = false;               // Fliegt die Figur nach dem Loslassen des Seils?
    bool m_pullPhase = false;                // Läuft das Hochschwingen am Anfang des Schwingens?
    float m_coyote = 0.0f;                   // Kurze Zeit nach dem Verlassen einer Kante, in der noch gesprungen werden darf
    float m_jumpBuffer = 0.0f;               // Sprungwunsch kurz vor der Landung merken
    float m_respawnTimer = 0.0f;             // Restzeit bis zum Wiedererscheinen
    float m_respawnX = 0.0f;                 // Position zum Wiedererscheinen
    float m_respawnY = 0.0f;                 // Höhe zum Wiedererscheinen
    float m_hookTimer = 0.0f;                // Zeit seit dem Wurf
    bool m_hookHasTarget = false;            // Hat der Wurf einen Ankerpunkt?
    float m_hookTargetX = 0.0f;              // Ziel des Hakens (x)
    float m_hookTargetY = 0.0f;              // Ziel des Hakens (y)
    float m_ropeLength = 0.0f;               // Aktuelle Seillänge
    float m_angle = 0.0f;                    // Pendelwinkel (0 = senkrecht unter dem Anker)
    float m_angularVel = 0.0f;               // Winkelgeschwindigkeit
    float m_pullSpeed = 0.0f;                // Bahngeschwindigkeit beim Greifen (wird nach dem Hochziehen übernommen)
    float m_retract = 0.0f;                  // Restzeit des Einziehens nach einem Fehlwurf
}; // Ende der Klasse Player
