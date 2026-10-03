// Animation.cpp - Umsetzung der Animationsverwaltung
#include "Animation.h" // Eigene Deklarationen

#include <algorithm> // std::max, std::min

// Standardwerte, falls data/animationen.txt fehlt oder unvollständig ist
AnimationSet::AnimationSet() {                                       // Beginn des Konstruktors
    m_defs[static_cast<int>(PlayerAnim::WalkRight)] = {0, 10, 14.0f, true};  // Laufen rechts: Zeile 0, 10 Bilder
    m_defs[static_cast<int>(PlayerAnim::WalkLeft)] = {1, 10, 14.0f, true};   // Laufen links: Zeile 1, 10 Bilder
    m_defs[static_cast<int>(PlayerAnim::Jump)] = {2, 4, 8.0f, false};        // Springen: Zeile 2, 4 Bilder
    m_defs[static_cast<int>(PlayerAnim::Land)] = {3, 4, 20.0f, false};       // Landen: Zeile 3, 4 Bilder
    m_defs[static_cast<int>(PlayerAnim::HookThrow)] = {4, 4, 16.0f, false};  // Haken werfen: Zeile 4, 4 Bilder
    m_defs[static_cast<int>(PlayerAnim::HookPull)] = {5, 4, 12.0f, false};   // Hochschwingen: Zeile 5, 4 Bilder
    m_defs[static_cast<int>(PlayerAnim::SwingUp)] = {6, 2, 6.0f, true};      // Aufschwung: Zeile 6, 2 Bilder
    m_defs[static_cast<int>(PlayerAnim::SwingDown)] = {7, 2, 6.0f, true};    // Abschwung: Zeile 7, 2 Bilder
    m_defs[static_cast<int>(PlayerAnim::HookLand)] = {8, 4, 18.0f, false};   // Landen nach Haken: Zeile 8, 4 Bilder
    m_defs[static_cast<int>(PlayerAnim::Idle)] = {9, 2, 2.0f, true};         // Stehen: Zeile 9, 2 Bilder
} // Ende des Konstruktors

// Liefert den Objektnamen einer Animation in der Textdatei
const char* AnimationSet::sectionName(PlayerAnim anim) {             // Beginn von sectionName
    switch (anim) {                                                  // Je nach Animation
    case PlayerAnim::WalkRight: return "Laufen_Rechts";              // Laufen nach rechts
    case PlayerAnim::WalkLeft: return "Laufen_Links";                // Laufen nach links
    case PlayerAnim::Jump: return "Springen";                        // Springen
    case PlayerAnim::Land: return "Landen";                          // Landen
    case PlayerAnim::HookThrow: return "Enterhaken_Werfen";          // Enterhaken werfen
    case PlayerAnim::HookPull: return "Hochschwingen";               // Hochschwingen
    case PlayerAnim::SwingUp: return "Aufschwung";                   // Aufschwung
    case PlayerAnim::SwingDown: return "Abschwung";                  // Abschwung
    case PlayerAnim::HookLand: return "Enterhaken_Landen";           // Landen nach dem Schwingen
    case PlayerAnim::Idle: return "Stehen";                          // Stehen
    default: return "Unbekannt";                                     // Sollte nie vorkommen
    }                                                                // Ende der Fallunterscheidung
} // Ende von sectionName

// Übernimmt die Werte aus der Textdatei (fehlende Werte behalten den Standard)
void AnimationSet::load(const PropertyFile& file) {                  // Beginn von load
    for (int i = 0; i < PLAYER_ANIM_COUNT; ++i) {                    // Alle Animationen durchgehen
        AnimationDef& def = m_defs[static_cast<std::size_t>(i)];     // Referenz auf die Definition
        const std::string section = sectionName(static_cast<PlayerAnim>(i)); // Objektname in der Datei
        def.row = std::max(0, file.getInt(section, "zeile", def.row)); // Zeile im Sprite-Sheet
        def.frames = std::max(1, file.getInt(section, "bilder", def.frames)); // Anzahl der Bilder
        def.fps = std::max(0.1f, file.getFloat(section, "bilder_pro_sekunde", def.fps)); // Geschwindigkeit
        def.loop = file.getBool(section, "schleife", def.loop);      // Wiederholen ja/nein
    }                                                                // Ende der Schleife
} // Ende von load

const AnimationDef& AnimationSet::get(PlayerAnim anim) const { return m_defs[static_cast<std::size_t>(anim)]; } // Definition zurückgeben

// Größte Bildanzahl = Anzahl der Spalten im Sprite-Sheet
int AnimationSet::columns() const {                                  // Beginn von columns
    int cols = 1;                                                    // Mindestens eine Spalte
    for (const AnimationDef& def : m_defs) cols = std::max(cols, def.frames); // Größte Bildanzahl suchen
    return cols;                                                     // Ergebnis zurückgeben
} // Ende von columns

// Größte Zeilennummer + 1 = Anzahl der Zeilen im Sprite-Sheet
int AnimationSet::rows() const {                                     // Beginn von rows
    int count = 1;                                                   // Mindestens eine Zeile
    for (const AnimationDef& def : m_defs) count = std::max(count, def.row + 1); // Größte Zeile suchen
    return count;                                                    // Ergebnis zurückgeben
} // Ende von rows

// Startet eine Animation; läuft sie schon, wird sie nur bei restart neu begonnen
void AnimationPlayer::play(PlayerAnim anim, const AnimationDef& def, bool restart) { // Beginn von play
    if (anim == m_anim && !restart) return;                          // Gleiche Animation läuft bereits
    m_anim = anim;                                                   // Neue Animation merken
    m_def = def;                                                     // Definition kopieren
    m_time = 0.0f;                                                   // Zeit zurücksetzen
    m_frame = 0;                                                     // Beim ersten Bild beginnen
    m_finished = false;                                              // Noch nicht fertig
} // Ende von play

// Lässt die Animationszeit weiterlaufen und berechnet das aktuelle Bild
void AnimationPlayer::update(float dt, float speed) {                // Beginn von update
    if (m_finished) return;                                          // Fertige Animationen bleiben stehen
    m_time += dt * speed;                                            // Zeit (mit Geschwindigkeitsfaktor) addieren
    int frame = static_cast<int>(m_time * m_def.fps);                // Bildnummer aus Zeit berechnen
    if (m_def.loop) {                                                // Wiederholende Animation
        m_frame = frame % std::max(1, m_def.frames);                 // Mit Modulo von vorne beginnen
    } else if (frame >= m_def.frames) {                              // Einmalige Animation ist durch
        m_frame = m_def.frames - 1;                                  // Auf dem letzten Bild stehen bleiben
        m_finished = true;                                           // Als fertig markieren
    } else {                                                         // Einmalige Animation läuft noch
        m_frame = frame;                                             // Bild übernehmen
    }                                                                // Ende der Unterscheidung
} // Ende von update

// Setzt das Einzelbild direkt (wird z.B. beim Sprung abhängig von der Geschwindigkeit benutzt)
void AnimationPlayer::setFrame(int frame) { m_frame = std::min(std::max(0, frame), m_def.frames - 1); } // Begrenzen und setzen
