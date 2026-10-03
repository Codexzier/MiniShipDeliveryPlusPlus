// Animation.h - Animationsdefinitionen der Spielfigur und Abspielen von Animationen
#pragma once // Header nur einmal einbinden

#include <array>  // std::array für die Liste aller Animationen
#include <string> // std::string

#include "PropertyFile.h" // Laden der Werte aus data/animationen.txt

// Alle Animationen der Spielfigur (jede belegt eine Zeile im Sprite-Sheet)
enum class PlayerAnim {   // Beginn der Aufzählung
    WalkRight = 0,        // 10 Bilder: Laufen nach rechts
    WalkLeft,             // 10 Bilder: Laufen nach links
    Jump,                 // 4 Bilder: Springen (2 aufwärts, 2 abwärts)
    Land,                 // 4 Bilder: Landen nach einem Sprung
    HookThrow,            // 4 Bilder: Enterhaken werfen
    HookPull,             // 4 Bilder: Hochschwingen am Seil
    SwingUp,              // 2 Bilder: Aufschwung
    SwingDown,            // 2 Bilder: Abschwung
    HookLand,             // 4 Bilder: Landen nach dem Enterhaken
    Idle,                 // 2 Bilder: Stehen (zusätzlich, damit die Figur im Stand nicht läuft)
    Count                 // Anzahl der Animationen (kein echter Eintrag)
}; // Ende der Aufzählung PlayerAnim

constexpr int PLAYER_ANIM_COUNT = static_cast<int>(PlayerAnim::Count); // Anzahl als Ganzzahl

// Beschreibung einer Animation
struct AnimationDef {    // Beginn der Struktur
    int row = 0;         // Zeile im Sprite-Sheet
    int frames = 1;      // Anzahl der Einzelbilder
    float fps = 10.0f;   // Bilder pro Sekunde
    bool loop = true;    // Soll die Animation wiederholt werden?
}; // Ende der Struktur AnimationDef

// Alle Animationsdefinitionen der Spielfigur
class AnimationSet {                                       // Beginn der Klasse
public:                                                    // Öffentliche Schnittstelle
    AnimationSet();                                        // Setzt die Standardwerte
    void load(const PropertyFile& file);                   // Werte aus der Textdatei übernehmen
    const AnimationDef& get(PlayerAnim anim) const;        // Definition einer Animation holen
    int columns() const;                                   // Benötigte Spalten im Sprite-Sheet
    int rows() const;                                      // Benötigte Zeilen im Sprite-Sheet
    static const char* sectionName(PlayerAnim anim);       // Objektname in der Textdatei
private:                                                   // Interne Daten
    std::array<AnimationDef, PLAYER_ANIM_COUNT> m_defs;    // Eine Definition pro Animation
}; // Ende der Klasse AnimationSet

// Spielt eine Animation ab und liefert das aktuelle Einzelbild
class AnimationPlayer {                                    // Beginn der Klasse
public:                                                    // Öffentliche Schnittstelle
    void play(PlayerAnim anim, const AnimationDef& def, bool restart = false); // Animation starten/wechseln
    void update(float dt, float speed = 1.0f);             // Zeit weiterlaufen lassen
    void setFrame(int frame);                              // Einzelbild direkt setzen (z.B. beim Sprung)
    PlayerAnim current() const { return m_anim; }          // Aktuelle Animation
    int frame() const { return m_frame; }                  // Aktuelles Einzelbild
    bool finished() const { return m_finished; }           // Ist eine einmalige Animation fertig?
    const AnimationDef& def() const { return m_def; }      // Definition der aktuellen Animation
private:                                                   // Interne Daten
    PlayerAnim m_anim = PlayerAnim::Count;                 // Aktuelle Animation (Count = noch keine gestartet)
    AnimationDef m_def;                                    // Kopie der Definition
    float m_time = 0.0f;                                   // Verstrichene Zeit in der Animation
    int m_frame = 0;                                       // Aktuelles Einzelbild
    bool m_finished = false;                               // Fertig-Merker
}; // Ende der Klasse AnimationPlayer
