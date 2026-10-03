// SpriteFactory.cpp - Zeichnet alle Platzhalter-Grafiken per Programmcode
// Die Spielfigur wird aus Gelenkwinkeln (Posen) zusammengesetzt, damit die Bewegungsabläufe erkennbar sind.
#include "SpriteFactory.h" // Eigene Deklarationen

#include <algorithm> // std::max, std::min
#include <cmath>     // std::sin, std::cos

#include "Font.h" // Schrift für Schilder (z.B. "SHOP" und "ZIEL")

namespace { // Interne Hilfsfunktionen, nur in dieser Datei sichtbar

// Einfacher 2D-Vektor für Gelenkpositionen
struct Vec2 {                                                  // Beginn der Struktur
    float x = 0.0f;                                            // x-Koordinate
    float y = 0.0f;                                            // y-Koordinate
};                                                             // Ende der Struktur Vec2

Vec2 add(Vec2 a, Vec2 b) { return Vec2{a.x + b.x, a.y + b.y}; }   // Zwei Vektoren addieren
Vec2 mul(Vec2 a, float s) { return Vec2{a.x * s, a.y * s}; }       // Vektor mit einer Zahl multiplizieren
Vec2 dirFromDown(float angle) { return Vec2{std::sin(angle), std::cos(angle)}; } // Richtung: 0 = nach unten, positiv = nach vorne

// Pose: Gelenkwinkel der Figur in Radiant (Blickrichtung rechts, 0 = gerade nach unten)
struct Pose {                                                  // Beginn der Struktur
    float lean = 0.08f;                                        // Neigung des Oberkörpers (positiv = nach vorne)
    float hipF = 0.0f;                                         // Hüftwinkel des vorderen Beins
    float kneeF = 0.1f;                                        // Kniebeugung des vorderen Beins
    float hipB = 0.0f;                                         // Hüftwinkel des hinteren Beins
    float kneeB = 0.1f;                                        // Kniebeugung des hinteren Beins
    float shF = 0.1f;                                          // Schulterwinkel des vorderen Arms
    float elF = 0.3f;                                          // Ellenbogenbeugung des vorderen Arms
    float shB = -0.1f;                                         // Schulterwinkel des hinteren Arms
    float elB = 0.3f;                                          // Ellenbogenbeugung des hinteren Arms
};                                                             // Ende der Struktur Pose

// Erzeugt eine Pose aus einer Werteliste (kürzere Schreibweise für die Schlüsselposen)
Pose makePose(float lean, float hipF, float kneeF, float hipB, float kneeB, float shF, float elF, float shB, float elB) { // Beginn von makePose
    Pose p;                                                    // Neue Pose
    p.lean = lean;                                             // Oberkörperneigung
    p.hipF = hipF;                                             // Vordere Hüfte
    p.kneeF = kneeF;                                           // Vorderes Knie
    p.hipB = hipB;                                             // Hintere Hüfte
    p.kneeB = kneeB;                                           // Hinteres Knie
    p.shF = shF;                                               // Vordere Schulter
    p.elF = elF;                                               // Vorderer Ellenbogen
    p.shB = shB;                                               // Hintere Schulter
    p.elB = elB;                                               // Hinterer Ellenbogen
    return p;                                                  // Pose zurückgeben
} // Ende von makePose

// Pose für die Laufanimation (t = 0..1 über einen ganzen Doppelschritt)
Pose walkPose(float t) {                                       // Beginn von walkPose
    float phase = t * 2.0f * PI;                               // Phase des Schrittzyklus in Radiant
    float s = std::sin(phase);                                 // Sinus bestimmt die Beinstellung
    float c = std::cos(phase);                                 // Kosinus bestimmt, welches Bein schwingt
    Pose p;                                                    // Neue Pose
    p.lean = 0.12f;                                            // Beim Laufen leicht nach vorne geneigt
    p.hipF = 0.5f * s;                                         // Vorderes Bein schwingt vor und zurück
    p.hipB = -0.5f * s;                                        // Hinteres Bein genau gegenläufig
    p.kneeF = 0.12f + 0.85f * std::max(0.0f, c);               // Vorderes Knie beugt sich beim Vorschwingen
    p.kneeB = 0.12f + 0.85f * std::max(0.0f, -c);              // Hinteres Knie beugt sich beim Vorschwingen
    p.shF = -0.5f * s;                                         // Vorderer Arm schwingt entgegen dem vorderen Bein
    p.shB = 0.5f * s;                                          // Hinterer Arm schwingt entgegen dem hinteren Bein
    p.elF = 0.45f + 0.25f * std::max(0.0f, -s);                // Vorderer Ellenbogen leicht gebeugt
    p.elB = 0.45f + 0.25f * std::max(0.0f, s);                 // Hinterer Ellenbogen leicht gebeugt
    return p;                                                  // Pose zurückgeben
} // Ende von walkPose

// Pose für das Stehen (leichtes Atmen)
Pose idlePose(float t) {                                       // Beginn von idlePose
    float breath = 0.5f - 0.5f * std::cos(t * 2.0f * PI);      // Atemkurve 0..1
    Pose p;                                                    // Neue Pose
    p.lean = 0.04f;                                            // Fast aufrecht
    p.hipF = 0.06f;                                            // Vorderes Bein minimal vorne
    p.hipB = -0.06f;                                           // Hinteres Bein minimal hinten
    p.kneeF = 0.08f + 0.1f * breath;                           // Knie federn beim Atmen leicht
    p.kneeB = 0.08f + 0.1f * breath;                           // Knie federn beim Atmen leicht
    p.shF = 0.12f + 0.05f * breath;                            // Arme bewegen sich minimal
    p.shB = -0.08f - 0.05f * breath;                           // Arme bewegen sich minimal
    p.elF = 0.25f;                                             // Ellenbogen leicht gebeugt
    p.elB = 0.25f;                                             // Ellenbogen leicht gebeugt
    return p;                                                  // Pose zurückgeben
} // Ende von idlePose

// Schlüsselposen der übrigen Animationen (key = Nummer der Schlüsselpose)
Pose keyPose(PlayerAnim anim, int key) {                       // Beginn von keyPose
    const float UP = PI;                                       // Winkel "Arm gerade nach oben"
    switch (anim) {                                            // Je nach Animation
    case PlayerAnim::Jump: {                                   // Springen: 2 Bilder aufwärts, 2 Bilder abwärts
        const Pose keys[4] = {                                 // Vier Schlüsselposen
            makePose(0.12f, 0.55f, 1.0f, -0.25f, 0.15f, 2.3f, 0.3f, 1.9f, 0.4f), // Absprung, Arme nach oben
            makePose(0.10f, 0.9f, 1.5f, 0.5f, 1.3f, 1.8f, 0.5f, 1.5f, 0.5f),     // Aufwärts, Knie angezogen
            makePose(0.05f, 0.45f, 0.7f, 0.15f, 0.5f, 1.5f, 0.3f, 2.0f, 0.3f),   // Abwärts, Arme ausbalancieren
            makePose(0.05f, 0.3f, 0.25f, 0.05f, 0.2f, 1.0f, 0.3f, 1.3f, 0.3f)};  // Abwärts, bereit zur Landung
        return keys[key % 4];                                  // Passende Pose zurückgeben
    }                                                          // Ende Springen
    case PlayerAnim::Land: {                                   // Landen: tiefe Hocke, dann aufrichten
        const Pose keys[4] = {                                 // Vier Schlüsselposen
            makePose(0.45f, 1.1f, 2.0f, 0.8f, 1.8f, 0.9f, 0.4f, 0.6f, 0.4f),     // Aufprall, tiefe Hocke
            makePose(0.35f, 0.8f, 1.5f, 0.6f, 1.3f, 0.6f, 0.4f, 0.4f, 0.4f),     // Abfedern
            makePose(0.20f, 0.45f, 0.85f, 0.3f, 0.65f, 0.35f, 0.35f, 0.1f, 0.3f), // Aufrichten
            makePose(0.10f, 0.15f, 0.3f, 0.05f, 0.2f, 0.15f, 0.3f, -0.05f, 0.3f)}; // Fast stehend
        return keys[key % 4];                                  // Passende Pose zurückgeben
    }                                                          // Ende Landen
    case PlayerAnim::HookThrow: {                              // Enterhaken werfen
        const Pose keys[4] = {                                 // Vier Schlüsselposen
            makePose(-0.05f, 0.3f, 0.25f, -0.25f, 0.2f, -0.6f, 0.5f, 0.5f, 0.4f), // Ausholen: Arm nach hinten unten
            makePose(-0.12f, 0.3f, 0.25f, -0.25f, 0.2f, -2.2f, 0.3f, 0.7f, 0.4f), // Arm hinter dem Kopf
            makePose(0.08f, 0.3f, 0.25f, -0.25f, 0.2f, 2.8f, 0.1f, 0.3f, 0.5f),   // Arm über dem Kopf nach vorne
            makePose(0.12f, 0.3f, 0.25f, -0.25f, 0.2f, 2.2f, 0.0f, 0.1f, 0.4f)};  // Arm zeigt zum Ziel
        return keys[key % 4];                                  // Passende Pose zurückgeben
    }                                                          // Ende Werfen
    case PlayerAnim::HookPull: {                               // Hochschwingen am Seil
        const Pose keys[4] = {                                 // Vier Schlüsselposen
            makePose(-0.05f, 0.1f, 0.15f, -0.05f, 0.1f, UP - 0.1f, 0.05f, UP + 0.05f, 0.05f), // Gestreckt hängen
            makePose(-0.10f, 0.7f, 1.2f, 0.55f, 1.1f, UP - 0.25f, 0.7f, UP - 0.1f, 0.7f),     // Hochziehen, Knie anziehen
            makePose(-0.20f, 1.1f, 0.5f, 0.95f, 0.4f, UP - 0.3f, 0.9f, UP - 0.2f, 0.9f),      // Beine nach vorne treten
            makePose(-0.15f, 0.8f, 0.2f, 0.6f, 0.2f, UP - 0.2f, 0.3f, UP - 0.1f, 0.3f)};      // Schwung aufbauen
        return keys[key % 4];                                  // Passende Pose zurückgeben
    }                                                          // Ende Hochschwingen
    case PlayerAnim::SwingUp: {                                // Aufschwung: Beine schwingen nach vorne oben
        const Pose keys[2] = {                                 // Zwei Schlüsselposen
            makePose(-0.25f, 0.8f, 0.35f, 0.65f, 0.3f, UP - 0.35f, 0.1f, UP - 0.25f, 0.1f),   // Beine vorne
            makePose(-0.35f, 1.2f, 0.15f, 1.05f, 0.1f, UP - 0.45f, 0.1f, UP - 0.35f, 0.1f)};  // Beine ganz vorne oben
        return keys[key % 2];                                  // Passende Pose zurückgeben
    }                                                          // Ende Aufschwung
    case PlayerAnim::SwingDown: {                              // Abschwung: Beine hängen nach hinten
        const Pose keys[2] = {                                 // Zwei Schlüsselposen
            makePose(0.2f, -0.25f, 0.45f, -0.4f, 0.6f, UP + 0.2f, 0.1f, UP + 0.3f, 0.1f),     // Beine leicht hinten
            makePose(0.3f, -0.5f, 0.8f, -0.65f, 1.0f, UP + 0.3f, 0.1f, UP + 0.4f, 0.1f)};     // Beine weit hinten
        return keys[key % 2];                                  // Passende Pose zurückgeben
    }                                                          // Ende Abschwung
    case PlayerAnim::HookLand: {                               // Landen nach dem Schwingen
        const Pose keys[4] = {                                 // Vier Schlüsselposen
            makePose(0.4f, 1.0f, 1.9f, 0.8f, 1.7f, 2.6f, 0.2f, 2.3f, 0.2f),     // Aufprall, Arme noch oben
            makePose(0.3f, 0.75f, 1.4f, 0.55f, 1.2f, 1.5f, 0.3f, 1.2f, 0.3f),   // Abfedern, Arme kommen runter
            makePose(0.18f, 0.4f, 0.8f, 0.3f, 0.6f, 0.6f, 0.3f, 0.4f, 0.3f),    // Aufrichten
            makePose(0.08f, 0.12f, 0.25f, 0.04f, 0.2f, 0.2f, 0.3f, -0.05f, 0.3f)}; // Fast stehend
        return keys[key % 4];                                  // Passende Pose zurückgeben
    }                                                          // Ende Haken-Landung
    default:                                                   // Alle anderen Animationen
        return idlePose(0.0f);                                 // Normale Standpose
    }                                                          // Ende der Fallunterscheidung
} // Ende von keyPose

// Anzahl der Schlüsselposen einer Animation
int keyCount(PlayerAnim anim) {                                // Beginn von keyCount
    if (anim == PlayerAnim::SwingUp || anim == PlayerAnim::SwingDown) return 2; // Auf-/Abschwung haben 2 Posen
    return 4;                                                  // Alle anderen haben 4 Posen
} // Ende von keyCount

// Liefert die Pose für Bild "frame" von "frames" einer Animation
Pose poseFor(PlayerAnim anim, int frame, int frames) {         // Beginn von poseFor
    float t = static_cast<float>(frame) / static_cast<float>(std::max(1, frames)); // Relativer Zeitpunkt 0..1
    if (anim == PlayerAnim::WalkRight || anim == PlayerAnim::WalkLeft) return walkPose(t); // Laufzyklus
    if (anim == PlayerAnim::Idle) return idlePose(t);          // Stehen
    int key = frame * keyCount(anim) / std::max(1, frames);    // Bildnummer auf Schlüsselpose abbilden
    return keyPose(anim, key);                                 // Schlüsselpose zurückgeben
} // Ende von poseFor

// Zeichnet die Figur (Paketbote mit Mütze und Paket auf dem Rücken) in eine Zelle der Größe S
// hangFromHands = true: Hände werden oben fixiert (für die Animationen am Seil)
void drawFigure(Canvas& c, float S, const Pose& p, bool hangFromHands) { // Beginn von drawFigure
    const float thigh = 0.19f * S;                             // Länge Oberschenkel
    const float shin = 0.19f * S;                              // Länge Unterschenkel
    const float torso = 0.25f * S;                             // Länge Oberkörper
    const float upper = 0.14f * S;                             // Länge Oberarm
    const float fore = 0.13f * S;                              // Länge Unterarm
    const float headR = 0.095f * S;                            // Radius des Kopfes
    const float legT = std::max(2.0f, 0.085f * S);             // Dicke der Beine
    const float armT = std::max(2.0f, 0.065f * S);             // Dicke der Arme
    const float torsoT = std::max(3.0f, 0.15f * S);            // Dicke des Oberkörpers

    const Color jacket = rgba(240, 140, 30);                   // Orange Lieferjacke
    const Color jacketDark = shade(jacket, 0.72f);             // Dunklere Jacke für den hinteren Arm
    const Color pants = rgba(45, 60, 115);                     // Dunkelblaue Hose
    const Color pantsDark = shade(pants, 0.72f);               // Dunklere Hose für das hintere Bein
    const Color skin = rgba(245, 200, 160);                    // Hautfarbe
    const Color skinDark = shade(skin, 0.85f);                 // Dunklere Hand hinten
    const Color cap = rgba(30, 95, 210);                       // Blaue Mütze
    const Color shoe = rgba(70, 45, 30);                       // Braune Schuhe
    const Color box = rgba(195, 145, 85);                      // Paket auf dem Rücken
    const Color tape = rgba(235, 215, 160);                    // Klebeband auf dem Paket
    const Color eye = rgba(30, 25, 35);                        // Augenfarbe

    // Gelenkpositionen relativ zur Hüfte berechnen
    Vec2 hip{0.0f, 0.0f};                                      // Hüfte ist der Ursprung
    Vec2 kneeF = mul(dirFromDown(p.hipF), thigh);              // Vorderes Knie
    Vec2 footF = add(kneeF, mul(dirFromDown(p.hipF - p.kneeF), shin)); // Vorderer Fuß
    Vec2 kneeB = mul(dirFromDown(p.hipB), thigh);              // Hinteres Knie
    Vec2 footB = add(kneeB, mul(dirFromDown(p.hipB - p.kneeB), shin)); // Hinterer Fuß
    Vec2 up{std::sin(p.lean), -std::cos(p.lean)};              // Richtung des Oberkörpers (nach oben)
    Vec2 shoulder = mul(up, torso);                            // Schulterposition
    Vec2 head = add(add(shoulder, mul(up, 0.13f * S)), Vec2{0.01f * S, 0.0f}); // Kopfmitte
    Vec2 elbowF = add(shoulder, mul(dirFromDown(p.shF), upper)); // Vorderer Ellenbogen
    Vec2 handF = add(elbowF, mul(dirFromDown(p.shF + p.elF), fore)); // Vordere Hand
    Vec2 elbowB = add(shoulder, mul(dirFromDown(p.shB), upper)); // Hinterer Ellenbogen
    Vec2 handB = add(elbowB, mul(dirFromDown(p.shB + p.elB), fore)); // Hintere Hand

    // Lage der Figur in der Zelle bestimmen
    Vec2 origin;                                               // Bildposition der Hüfte
    if (hangFromHands) {                                       // Am Seil: Hände oben in der Mitte festhalten
        Vec2 handsMid = mul(add(handF, handB), 0.5f);          // Mittelpunkt beider Hände
        origin.x = 0.5f * S - handsMid.x;                      // Hände waagerecht in die Mitte
        origin.y = 0.07f * S - handsMid.y;                     // Hände nahe der Oberkante
    } else {                                                   // Am Boden: tiefsten Fuß auf die Unterkante stellen
        float lowest = std::max(footF.y, footB.y) + legT * 0.5f; // Unterkante des tieferen Schuhs
        origin.x = 0.48f * S;                                  // Hüfte etwa in der Mitte
        origin.y = S - 1.0f - lowest;                          // Füße auf die unterste Pixelzeile stellen
    }                                                          // Ende der Lagebestimmung
    auto P = [&](Vec2 v) { return add(origin, v); };           // Relativposition in Bildkoordinaten umrechnen
    auto limb = [&](Vec2 a, Vec2 b, Color col, float thick) {  // Hilfsfunktion: Gliedmaße als dicke Linie
        Vec2 pa = P(a);                                        // Startpunkt in Bildkoordinaten
        Vec2 pb = P(b);                                        // Endpunkt in Bildkoordinaten
        c.lineF(pa.x, pa.y, pb.x, pb.y, col, thick);           // Dicke Linie zeichnen
    };                                                         // Ende der Hilfsfunktion
    auto dot = [&](Vec2 v, float r, Color col) {               // Hilfsfunktion: gefüllter Kreis
        Vec2 pv = P(v);                                        // Position in Bildkoordinaten
        c.fillCircle(static_cast<int>(pv.x + 0.5f), static_cast<int>(pv.y + 0.5f), static_cast<int>(r), col); // Kreis zeichnen
    };                                                         // Ende der Hilfsfunktion
    Vec2 shoeOffset{0.07f * S, 0.0f};                          // Schuhe zeigen nach vorne

    // 1. Hinterer Arm (hinter dem Körper, dunkler)
    limb(shoulder, elbowB, jacketDark, armT);                  // Oberarm hinten
    limb(elbowB, handB, jacketDark, armT);                     // Unterarm hinten
    dot(handB, armT * 0.6f, skinDark);                         // Hand hinten

    // 2. Hinteres Bein (dunkler)
    limb(hip, kneeB, pantsDark, legT);                         // Oberschenkel hinten
    limb(kneeB, footB, pantsDark, legT);                       // Unterschenkel hinten
    limb(footB, add(footB, shoeOffset), shade(shoe, 0.8f), legT * 0.8f); // Schuh hinten

    // 3. Paket auf dem Rücken (gedrehtes Rechteck)
    Vec2 forward{std::cos(p.lean), std::sin(p.lean)};          // Richtung "nach vorne" senkrecht zum Oberkörper
    Vec2 boxCenter = add(mul(shoulder, 0.55f), mul(forward, -(torsoT * 0.5f + 0.06f * S))); // Paketmitte hinter dem Rücken
    float hw = 0.085f * S;                                     // Halbe Paketbreite
    float hh = 0.11f * S;                                      // Halbe Pakethöhe
    std::vector<std::pair<float, float>> boxPoly;              // Eckpunkte des Pakets
    const float signs[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}; // Vorzeichen der vier Ecken
    for (const auto& s : signs) {                              // Alle vier Ecken berechnen
        Vec2 corner = add(boxCenter, add(mul(forward, s[0] * hw), mul(up, s[1] * hh))); // Ecke relativ zur Hüfte
        Vec2 pc = P(corner);                                   // In Bildkoordinaten umrechnen
        boxPoly.push_back({pc.x, pc.y});                       // Ecke speichern
    }                                                          // Ende der Eckenschleife
    c.fillPolygon(boxPoly, box);                               // Paket füllen
    limb(add(boxCenter, mul(forward, -hw)), add(boxCenter, mul(forward, hw)), tape, std::max(1.0f, 0.025f * S)); // Klebeband quer

    // 4. Oberkörper mit Hosenbund
    dot(hip, torsoT * 0.5f, pants);                            // Hüfte/Hosenbund
    limb(mul(up, 0.04f * S), shoulder, jacket, torsoT);        // Jacke vom Bund bis zur Schulter
    limb(mul(up, 0.06f * S), mul(up, 0.08f * S), shade(jacket, 0.85f), torsoT); // Dunklerer Gürtelstreifen

    // 5. Vorderes Bein
    limb(hip, kneeF, pants, legT);                             // Oberschenkel vorne
    limb(kneeF, footF, pants, legT);                           // Unterschenkel vorne
    limb(footF, add(footF, shoeOffset), shoe, legT * 0.8f);    // Schuh vorne

    // 6. Kopf mit Mütze
    limb(shoulder, head, skin, armT);                          // Hals
    dot(head, headR, skin);                                    // Kopf
    Vec2 headPx = P(head);                                     // Kopfmitte in Bildkoordinaten
    int hx = static_cast<int>(headPx.x + 0.5f);                // Kopfmitte x (gerundet)
    int hy = static_cast<int>(headPx.y + 0.5f);                // Kopfmitte y (gerundet)
    int r = static_cast<int>(headR);                           // Kopfradius als Ganzzahl
    c.setClip(RectI{0, 0, c.width(), hy - r / 4});             // Nur den oberen Teil des Kopfes bemalen
    c.fillCircle(hx, hy, r + 1, cap);                          // Mützenkuppel
    c.resetClip();                                             // Zeichenbereich wieder freigeben
    c.fillRect(hx, hy - r / 4 - std::max(1, r / 4), r + r / 2, std::max(1, r / 4), shade(cap, 0.75f)); // Mützenschirm nach vorne
    c.fillRect(hx + r / 2, hy, std::max(1, static_cast<int>(S / 40)), std::max(1, static_cast<int>(S / 28)), eye); // Auge

    // 7. Vorderer Arm (vor dem Körper)
    limb(shoulder, elbowF, jacket, armT);                      // Oberarm vorne
    limb(elbowF, handF, jacket, armT);                         // Unterarm vorne
    dot(handF, armT * 0.6f, skin);                             // Hand vorne
} // Ende von drawFigure

// Zeichnet ein Objekt mehrfach, damit es nahtlos an den Bildrändern weiterläuft (für kachelbare Hintergründe)
template <typename DrawFn>                                     // Beliebige Zeichenfunktion
void drawWrapped(int width, DrawFn fn) {                       // Beginn von drawWrapped
    fn(0);                                                     // Normal zeichnen
    fn(-width);                                                // Um eine Bildbreite nach links versetzt
    fn(width);                                                 // Um eine Bildbreite nach rechts versetzt
} // Ende von drawWrapped

// Zeichnet ein kleines Ampelmännchen (stehend oder gehend)
void drawPedestrian(Canvas& c, int cx, int cy, int size, bool walking, Color col) { // Beginn von drawPedestrian
    int u = std::max(1, size / 8);                             // Grundeinheit des Männchens
    c.fillCircle(cx, cy - 3 * u, u, col);                      // Kopf
    c.fillRect(cx - u, cy - 2 * u, 2 * u, 3 * u, col);         // Körper
    if (walking) {                                             // Gehendes (grünes) Männchen
        c.line(cx, cy + u, cx - 2 * u, cy + 3 * u, col, u);    // Bein nach hinten
        c.line(cx, cy + u, cx + 2 * u, cy + 3 * u, col, u);    // Bein nach vorne
        c.line(cx, cy - 2 * u, cx + 2 * u, cy, col, u);        // Arm nach vorne
        c.line(cx, cy - 2 * u, cx - 2 * u, cy, col, u);        // Arm nach hinten
    } else {                                                   // Stehendes (rotes) Männchen
        c.fillRect(cx - u, cy + u, u, 2 * u, col);             // Linkes Bein
        c.fillRect(cx + u / 2, cy + u, u, 2 * u, col);         // Rechtes Bein
        c.fillRect(cx - 2 * u, cy - 2 * u, u, 3 * u, col);     // Linker Arm
        c.fillRect(cx + u, cy - 2 * u, u, 3 * u, col);         // Rechter Arm
    }                                                          // Ende der Unterscheidung
} // Ende von drawPedestrian

} // Ende des internen Namensraums

namespace SpriteFactory { // Öffentliche Erzeuger

// Legt einen dunklen Umriss um alle sichtbaren Pixel (typischer Pixelart-Look)
void addOutline(Image& image, Color outline, int thickness) {  // Beginn von addOutline
    for (int pass = 0; pass < thickness; ++pass) {             // Pro Durchgang wächst der Umriss um 1 Pixel
        Image copy = image;                                    // Kopie, damit neue Umrisspixel nicht sofort mitzählen
        for (int y = 0; y < image.height; ++y) {               // Alle Zeilen
            for (int x = 0; x < image.width; ++x) {            // Alle Spalten
                if (alphaOf(copy.get(x, y)) != 0) continue;    // Nur durchsichtige Pixel können Umriss werden
                bool nearSolid = alphaOf(copy.get(x - 1, y)) != 0 || alphaOf(copy.get(x + 1, y)) != 0 || // Nachbar links/rechts sichtbar?
                                 alphaOf(copy.get(x, y - 1)) != 0 || alphaOf(copy.get(x, y + 1)) != 0;   // Nachbar oben/unten sichtbar?
                if (nearSolid) image.set(x, y, outline);       // Dann wird dieses Pixel zum Umriss
            }                                                  // Ende der Spaltenschleife
        }                                                      // Ende der Zeilenschleife
    }                                                          // Ende der Durchgänge
} // Ende von addOutline

// Zeichnet das komplette Sprite-Sheet der Spielfigur (eine Zeile pro Animation)
void drawPlayerSheet(Image& sheet, int tile, const AnimationSet& anims) { // Beginn von drawPlayerSheet
    const float S = static_cast<float>(tile);                  // Zellengröße als Kommazahl
    const int outlineThickness = std::max(1, tile / 64);       // Umrissdicke passend zur Größe
    for (int a = 0; a < PLAYER_ANIM_COUNT; ++a) {              // Alle Animationen
        PlayerAnim anim = static_cast<PlayerAnim>(a);          // Aktuelle Animation
        const AnimationDef& def = anims.get(anim);             // Definition (Zeile und Bildanzahl)
        bool hanging = anim == PlayerAnim::HookPull || anim == PlayerAnim::SwingUp || anim == PlayerAnim::SwingDown; // Hängt die Figur am Seil?
        for (int f = 0; f < def.frames; ++f) {                 // Alle Bilder der Animation
            if ((f + 1) * tile > sheet.width || (def.row + 1) * tile > sheet.height) continue; // Passt nicht ins Sheet
            Image frame(tile, tile);                           // Leere Zelle
            Canvas fc(frame);                                  // Zeichenfläche für die Zelle
            drawFigure(fc, S, poseFor(anim, f, def.frames), hanging); // Figur in der passenden Pose zeichnen
            addOutline(frame, rgba(25, 20, 30), outlineThickness); // Umriss hinzufügen
            if (anim == PlayerAnim::WalkLeft) frame = mirrorImage(frame); // Nach links laufen = gespiegelt
            Canvas sc(sheet);                                  // Zeichenfläche für das ganze Sheet
            sc.blit(frame, f * tile, def.row * tile);          // Zelle an ihre Position kopieren
        }                                                      // Ende der Bildschleife
    }                                                          // Ende der Animationsschleife
} // Ende von drawPlayerSheet

// Zeichnet eine sich drehende Münze, alle Bilder nebeneinander
void drawCoinStrip(Image& image, int frames) {                 // Beginn von drawCoinStrip
    Canvas c(image);                                           // Zeichenfläche
    int size = image.height;                                   // Größe eines Bildes (quadratisch)
    int r = size / 2 - 1;                                      // Radius der Münze
    for (int f = 0; f < frames; ++f) {                         // Alle Bilder
        float squeeze = std::fabs(std::cos(static_cast<float>(f) * PI / static_cast<float>(frames))); // Wie breit die Münze erscheint
        int rx = std::max(1, static_cast<int>(static_cast<float>(r) * squeeze)); // Halbe Breite der Ellipse
        int cx = f * size + size / 2;                          // Mittelpunkt x in diesem Bild
        int cy = size / 2;                                     // Mittelpunkt y
        c.fillEllipse(cx, cy, rx, r, rgba(190, 130, 20));      // Dunkler Rand
        c.fillEllipse(cx, cy, std::max(0, rx - std::max(1, size / 12)), r - std::max(1, size / 12), rgba(250, 200, 40)); // Goldene Fläche
        if (rx > size / 6) c.fillRect(cx - std::max(1, rx / 3), cy - r / 2, std::max(1, rx / 4), r, rgba(255, 240, 150)); // Glanzstreifen
    }                                                          // Ende der Bildschleife
} // Ende von drawCoinStrip

// Zeichnet ein Auto von vorne
void drawCarFront(Image& image, Color body) {                  // Beginn von drawCarFront
    Canvas c(image);                                           // Zeichenfläche
    int w = image.width;                                       // Bildbreite
    int h = image.height;                                      // Bildhöhe
    int wheelW = w / 7;                                        // Reifenbreite
    c.fillRect(w / 12, h * 3 / 4, wheelW, h / 4, rgba(25, 25, 28)); // Linker Reifen
    c.fillRect(w - w / 12 - wheelW, h * 3 / 4, wheelW, h / 4, rgba(25, 25, 28)); // Rechter Reifen
    std::vector<std::pair<float, float>> cabin = {             // Fahrerkabine als Trapez
        {w * 0.2f, h * 0.38f}, {w * 0.28f, h * 0.06f}, {w * 0.72f, h * 0.06f}, {w * 0.8f, h * 0.38f}}; // Vier Eckpunkte
    c.fillPolygon(cabin, shade(body, 0.85f));                  // Kabine etwas dunkler
    std::vector<std::pair<float, float>> glass = {             // Windschutzscheibe
        {w * 0.25f, h * 0.36f}, {w * 0.31f, h * 0.11f}, {w * 0.69f, h * 0.11f}, {w * 0.75f, h * 0.36f}}; // Vier Eckpunkte
    c.fillPolygon(glass, rgba(150, 200, 235));                 // Scheibe hellblau
    c.line(static_cast<int>(w * 0.4f), static_cast<int>(h * 0.14f), static_cast<int>(w * 0.33f), static_cast<int>(h * 0.32f), rgba(225, 240, 250), std::max(1, w / 60)); // Spiegelung
    c.fillRect(w / 20, static_cast<int>(h * 0.36f), w - w / 10, static_cast<int>(h * 0.44f), body); // Karosserie
    c.fillRect(w / 20, static_cast<int>(h * 0.36f), w - w / 10, std::max(1, h / 30), shade(body, 1.25f)); // Glanzkante oben
    c.fillRect(w * 3 / 10, static_cast<int>(h * 0.52f), w * 4 / 10, static_cast<int>(h * 0.14f), rgba(40, 40, 45)); // Kühlergrill
    c.fillCircle(w / 6, static_cast<int>(h * 0.56f), std::max(2, h / 10), rgba(255, 250, 200)); // Linker Scheinwerfer
    c.fillCircle(w - w / 6, static_cast<int>(h * 0.56f), std::max(2, h / 10), rgba(255, 250, 200)); // Rechter Scheinwerfer
    c.fillRect(w / 20, static_cast<int>(h * 0.74f), w - w / 10, std::max(2, h / 12), rgba(150, 150, 155)); // Stoßstange
    c.fillRect(w * 2 / 5, static_cast<int>(h * 0.68f), w / 5, std::max(2, h / 10), rgba(240, 240, 240)); // Nummernschild
    c.fillRect(0, static_cast<int>(h * 0.33f), w / 14, h / 10, shade(body, 0.8f)); // Linker Außenspiegel
    c.fillRect(w - w / 14, static_cast<int>(h * 0.33f), w / 14, h / 10, shade(body, 0.8f)); // Rechter Außenspiegel
    addOutline(image, rgba(20, 20, 25), std::max(1, w / 120)); // Umriss
} // Ende von drawCarFront

// Zeichnet die Fußgängerampel mit Mast und Taster
void drawTrafficLight(Image& image, int tile, bool green) {    // Beginn von drawTrafficLight
    Canvas c(image);                                           // Zeichenfläche
    int w = image.width;                                       // Bildbreite
    int h = image.height;                                      // Bildhöhe
    int poleW = std::max(2, tile / 16);                        // Breite des Mastes
    int boxW = std::min(w - 2, tile * 36 / 100);               // Breite des Signalkastens
    int boxH = tile * 64 / 100;                                // Höhe des Signalkastens
    int boxX = (w - boxW) / 2;                                 // Linke Kante des Kastens
    int boxY = 1;                                              // Obere Kante des Kastens
    c.fillRect(w / 2 - poleW / 2, boxY + boxH, poleW, h - boxH - 1, rgba(120, 125, 130)); // Mast
    c.fillRect(w / 2 - poleW / 2, boxY + boxH, std::max(1, poleW / 3), h - boxH - 1, rgba(170, 175, 180)); // Glanz am Mast
    c.fillRect(boxX, boxY, boxW, boxH, rgba(30, 30, 35));      // Signalkasten
    c.drawRect(boxX, boxY, boxW, boxH, rgba(70, 70, 75), std::max(1, tile / 64)); // Rahmen des Kastens
    int lampR = boxW * 38 / 100;                               // Radius der Lampen
    int cx = w / 2;                                            // Mittelpunkt x der Lampen
    int redY = boxY + boxH / 4 + 1;                            // Mittelpunkt y der roten Lampe
    int greenY = boxY + boxH * 3 / 4 - 1;                      // Mittelpunkt y der grünen Lampe
    c.fillCircle(cx, redY, lampR, green ? rgba(70, 20, 20) : rgba(240, 40, 30)); // Rote Lampe (an oder aus)
    c.fillCircle(cx, greenY, lampR, green ? rgba(40, 230, 90) : rgba(20, 60, 30)); // Grüne Lampe (an oder aus)
    drawPedestrian(c, cx, redY, lampR * 2, false, green ? rgba(110, 40, 40) : rgba(255, 220, 210)); // Stehendes Männchen
    drawPedestrian(c, cx, greenY, lampR * 2, true, green ? rgba(220, 255, 230) : rgba(40, 90, 50)); // Gehendes Männchen
    int btnW = std::max(4, tile * 16 / 100);                   // Breite des Tasters
    int btnH = std::max(5, tile * 22 / 100);                   // Höhe des Tasters
    int btnY = h - tile * 105 / 100;                           // Höhe des Tasters über dem Boden
    c.fillRect(cx - btnW / 2, btnY, btnW, btnH, rgba(250, 200, 30)); // Gelber Tasterkasten
    c.fillRect(cx - btnW / 4, btnY + btnH / 2, btnW / 2, std::max(1, btnH / 4), rgba(40, 40, 40)); // Drückknopf
    c.fillRect(cx - btnW / 3, btnY + std::max(1, btnH / 8), btnW * 2 / 3, std::max(1, btnH / 6), rgba(30, 30, 30)); // Signalfeld "Signal kommt"
    addOutline(image, rgba(20, 20, 25), std::max(1, tile / 64)); // Umriss
} // Ende von drawTrafficLight

// Zeichnet ein Ladengebäude mit Markise, Schaufenster, Tür und Schild
void drawShop(Image& image, int tile, const std::string& sign) { // Beginn von drawShop
    Canvas c(image);                                           // Zeichenfläche
    int w = image.width;                                       // Bildbreite
    int h = image.height;                                      // Bildhöhe
    int u = std::max(1, tile / 32);                            // Grundeinheit für Details
    int roofH = tile / 5;                                      // Höhe des Dachrands
    c.fillRect(0, roofH, w, h - roofH, rgba(200, 110, 70));    // Ziegelwand
    SimpleRandom rnd(77);                                      // Zufall für Ziegelfarben
    int brickH = std::max(2, tile / 10);                       // Höhe einer Ziegelreihe
    int brickW = std::max(4, tile / 5);                        // Breite eines Ziegels
    for (int y = roofH; y < h; y += brickH) {                  // Alle Ziegelreihen
        int offset = ((y - roofH) / brickH) % 2 ? brickW / 2 : 0; // Jede zweite Reihe versetzt
        for (int x = -offset; x < w; x += brickW) {            // Alle Ziegel einer Reihe
            c.fillRect(x + u, y + u, brickW - u, brickH - u, shade(rgba(200, 110, 70), rnd.range(0.85f, 1.1f))); // Ziegel mit Farbvariation
        }                                                      // Ende der Ziegelschleife
    }                                                          // Ende der Reihenschleife
    c.fillRect(0, 0, w, roofH, rgba(90, 60, 50));              // Dachrand
    c.fillRect(0, roofH - 2 * u, w, 2 * u, rgba(60, 40, 35));  // Schattenkante unter dem Dach
    int signW = w * 6 / 10;                                    // Breite des Schildes
    int signH = std::max(10, tile * 30 / 100);                 // Höhe des Schildes
    int signX = (w - signW) / 2;                               // Linke Kante des Schildes
    int signY = roofH + 2 * u;                                 // Obere Kante des Schildes
    c.fillRect(signX, signY, signW, signH, rgba(250, 235, 180)); // Schildfläche
    c.drawRect(signX, signY, signW, signH, rgba(120, 70, 30), std::max(1, u)); // Schildrahmen
    int textScale = std::max(1, std::min(signH / 11, signW / std::max(1, Font::textWidth(sign, 1) + 4))); // Textgröße passend zum Schild
    int tw = Font::textWidth(sign, textScale);                 // Textbreite
    Font::drawText(c, signX + (signW - tw) / 2, signY + (signH - 7 * textScale) / 2, sign, rgba(160, 40, 30), textScale); // Text mittig
    int awningY = signY + signH + 3 * u;                       // Obere Kante der Markise
    int awningH = tile * 30 / 100;                             // Höhe der Markise
    int stripe = std::max(2, tile / 8);                        // Breite eines Markisenstreifens
    for (int x = 0; x < w; x += stripe) {                      // Alle Streifen
        Color col = (x / stripe) % 2 ? rgba(245, 245, 240) : rgba(220, 50, 50); // Rot-weiß im Wechsel
        c.fillRect(x, awningY, stripe, awningH, col);          // Streifen zeichnen
        c.fillCircle(x + stripe / 2, awningY + awningH, stripe / 2, col); // Bogenkante unten
    }                                                          // Ende der Streifenschleife
    int floorY = h - std::max(2, tile / 12);                   // Oberkante des Sockels
    c.fillRect(0, floorY, w, h - floorY, rgba(120, 120, 125)); // Steinsockel
    int winTop = awningY + awningH + stripe;                   // Oberkante von Fenster und Tür
    int doorW = std::max(6, tile * 45 / 100);                  // Türbreite
    int doorX = w - doorW - w / 10;                            // Tür rechts im Gebäude
    c.fillRect(doorX, winTop, doorW, floorY - winTop, rgba(110, 70, 40)); // Türblatt
    c.drawRect(doorX, winTop, doorW, floorY - winTop, rgba(70, 45, 25), std::max(1, u)); // Türrahmen
    c.fillRect(doorX + doorW / 5, winTop + (floorY - winTop) / 8, doorW * 3 / 5, (floorY - winTop) / 3, rgba(170, 210, 230)); // Türfenster
    c.fillCircle(doorX + doorW * 4 / 5, winTop + (floorY - winTop) * 6 / 10, std::max(1, u), rgba(240, 200, 60)); // Türgriff
    int winX = w / 10;                                         // Linke Kante des Schaufensters
    int winW = doorX - winX - w / 12;                          // Breite des Schaufensters
    int winH = (floorY - winTop) * 7 / 10;                     // Höhe des Schaufensters
    c.fillRect(winX, winTop, winW, winH, rgba(160, 205, 230)); // Glasfläche
    c.drawRect(winX, winTop, winW, winH, rgba(240, 240, 240), std::max(1, u * 2)); // Weißer Fensterrahmen
    int shelfY = winTop + winH * 6 / 10;                       // Höhe des Regalbretts
    c.fillRect(winX + 2 * u, shelfY, winW - 4 * u, std::max(1, u), rgba(120, 80, 50)); // Regalbrett
    SimpleRandom goods(5);                                     // Zufall für die Waren im Fenster
    for (int x = winX + 4 * u; x < winX + winW - 6 * u; x += 5 * u) { // Waren nebeneinander
        int gh = goods.rangeInt(3, 6) * u;                     // Höhe der Ware
        Color gc = rgba(goods.rangeInt(80, 250), goods.rangeInt(80, 250), goods.rangeInt(60, 200)); // Zufällige Farbe
        c.fillRect(x, shelfY - gh, 3 * u, gh, gc);             // Ware auf dem Regal
    }                                                          // Ende der Warenschleife
    c.line(winX + winW / 5, winTop + winH / 6, winX + winW / 3, winTop + winH / 3, rgba(230, 245, 255), std::max(1, u)); // Spiegelung im Glas
    addOutline(image, rgba(30, 20, 20), std::max(1, tile / 64)); // Umriss
} // Ende von drawShop

// Zeichnet eine wehende Zielflagge mit Schild "ZIEL" (Bilder nebeneinander)
void drawGoalFlag(Image& image, int tile, int frames) {        // Beginn von drawGoalFlag
    Canvas c(image);                                           // Zeichenfläche
    int fw = image.width / std::max(1, frames);                // Breite eines Einzelbildes
    int h = image.height;                                      // Bildhöhe
    int poleW = std::max(2, tile / 18);                        // Breite des Fahnenmastes
    int sq = std::max(2, tile / 9);                            // Größe eines Schachbrettfeldes
    int flagW = sq * 5;                                        // Flaggenbreite
    int flagH = sq * 4;                                        // Flaggenhöhe
    for (int f = 0; f < frames; ++f) {                         // Alle Einzelbilder
        int ox = f * fw;                                       // Linke Kante dieses Bildes
        int poleX = ox + fw / 6;                               // Position des Mastes
        c.fillRect(poleX, tile / 8, poleW, h - tile / 8, rgba(200, 200, 205)); // Fahnenmast
        c.fillCircle(poleX + poleW / 2, tile / 8, poleW, rgba(250, 210, 50)); // Goldene Kugel oben
        float phase = static_cast<float>(f) / static_cast<float>(frames) * 2.0f * PI; // Wellenphase des Bildes
        for (int x = 0; x < flagW; ++x) {                      // Alle Spalten der Flagge
            float wave = std::sin(static_cast<float>(x) / static_cast<float>(flagW) * 2.0f * PI - phase); // Wellenform
            int dy = static_cast<int>(wave * static_cast<float>(sq) * 0.35f * (static_cast<float>(x) / static_cast<float>(flagW))); // Verschiebung (am Mast 0)
            for (int y = 0; y < flagH; ++y) {                  // Alle Zeilen der Flagge
                bool dark = ((x / sq) + (y / sq)) % 2 == 0;    // Schachbrettmuster
                c.putPixel(poleX + poleW + x, tile / 5 + y + dy, dark ? rgba(25, 25, 25) : rgba(245, 245, 245)); // Pixel setzen
            }                                                  // Ende der Zeilenschleife
        }                                                      // Ende der Spaltenschleife
        int boardW = std::min(fw - 2, tile * 7 / 10);          // Breite des Schildes
        int boardH = std::max(9, tile / 4);                    // Höhe des Schildes
        int boardX = poleX - boardW / 2 + poleW / 2;           // Schild mittig am Mast
        int boardY = h - tile * 9 / 10;                        // Höhe des Schildes
        if (boardX < ox) boardX = ox;                          // Nicht über den linken Bildrand hinaus
        c.fillRect(boardX, boardY, boardW, boardH, rgba(40, 140, 60)); // Grünes Schild
        c.drawRect(boardX, boardY, boardW, boardH, rgba(240, 240, 240), std::max(1, tile / 64)); // Weißer Rand
        int ts = std::max(1, boardH / 12);                     // Textgröße
        Font::drawText(c, boardX + (boardW - Font::textWidth("ZIEL", ts)) / 2, boardY + (boardH - 7 * ts) / 2, "ZIEL", rgba(255, 255, 255), ts); // Text "ZIEL"
    }                                                          // Ende der Bildschleife
    addOutline(image, rgba(20, 20, 25), std::max(1, tile / 64)); // Umriss
} // Ende von drawGoalFlag

// Zeichnet ein Stück Müll, das im Wasser schwimmt
void drawTrash(Image& image, int type) {                       // Beginn von drawTrash
    Canvas c(image);                                           // Zeichenfläche
    int s = image.width;                                       // Bildgröße (quadratisch)
    int u = std::max(1, s / 16);                               // Grundeinheit
    switch (type % 5) {                                        // Je nach Müllart
    case 0: {                                                  // Glasflasche (liegend)
        c.fillRect(s / 8, s * 4 / 10, s * 6 / 10, s / 4, rgba(40, 140, 70)); // Flaschenkörper
        c.fillRect(s * 7 / 10, s * 45 / 100, s / 6, s / 7, rgba(40, 140, 70)); // Flaschenhals
        c.fillRect(s * 85 / 100, s * 45 / 100, s / 10, s / 7, rgba(200, 180, 60)); // Verschluss
        c.fillRect(s / 5, s * 43 / 100, s / 3, u, rgba(150, 230, 170)); // Lichtreflex
        break;                                                 // Ende Flasche
    }                                                          // Ende case 0
    case 1: {                                                  // Getränkedose (liegend)
        c.fillRect(s / 5, s * 38 / 100, s * 6 / 10, s * 3 / 10, rgba(210, 40, 40)); // Dosenkörper rot
        c.fillRect(s / 5, s * 38 / 100, 2 * u, s * 3 / 10, rgba(200, 200, 205)); // Silberner Rand links
        c.fillRect(s * 8 / 10 - 2 * u, s * 38 / 100, 2 * u, s * 3 / 10, rgba(200, 200, 205)); // Silberner Rand rechts
        c.fillRect(s * 4 / 10, s * 45 / 100, s / 5, s / 8, rgba(250, 250, 250)); // Etikett
        break;                                                 // Ende Dose
    }                                                          // Ende case 1
    case 2: {                                                  // Alter Autoreifen
        c.ring(s / 2, s / 2, s * 4 / 10, std::max(2, s / 6), rgba(30, 30, 32)); // Reifen
        c.ring(s / 2, s / 2, s * 4 / 10 - std::max(2, s / 6), std::max(1, u), rgba(90, 90, 95)); // Felgenrand
        break;                                                 // Ende Reifen
    }                                                          // Ende case 2
    case 3: {                                                  // Plastiktüte
        c.fillEllipse(s / 2, s * 6 / 10, s * 35 / 100, s / 4, rgba(230, 230, 235)); // Tütenkörper
        c.ring(s * 35 / 100, s * 35 / 100, s / 8, std::max(1, u), rgba(230, 230, 235)); // Linker Henkel
        c.ring(s * 65 / 100, s * 35 / 100, s / 8, std::max(1, u), rgba(230, 230, 235)); // Rechter Henkel
        c.fillRect(s * 3 / 10, s * 55 / 100, s * 4 / 10, std::max(1, u), rgba(60, 120, 220)); // Aufdruck
        break;                                                 // Ende Tüte
    }                                                          // Ende case 3
    default: {                                                 // Holzkiste
        c.fillRect(s / 6, s / 4, s * 2 / 3, s / 2, rgba(160, 110, 60)); // Kiste
        c.drawRect(s / 6, s / 4, s * 2 / 3, s / 2, rgba(110, 70, 35), std::max(1, u)); // Kistenrand
        c.line(s / 6, s / 4, s * 5 / 6, s * 3 / 4, rgba(110, 70, 35), std::max(1, u)); // Querlatte
        break;                                                 // Ende Kiste
    }                                                          // Ende default
    }                                                          // Ende der Fallunterscheidung
    addOutline(image, rgba(20, 30, 40), 1);                    // Dünner Umriss
} // Ende von drawTrash

// Zeichnet eine Bodenkachel: Gehweg oder Straße oben, Erde darunter
void drawGroundTile(Image& image, int tile, bool street) {     // Beginn von drawGroundTile
    Canvas c(image);                                           // Zeichenfläche
    int top = tile * 16 / 100;                                 // Höhe der Oberflächenschicht
    int u = std::max(1, tile / 32);                            // Grundeinheit
    c.fillRect(0, 0, tile, tile, rgba(120, 85, 55));           // Erde füllt die ganze Kachel
    SimpleRandom rnd(street ? 31 : 17);                        // Zufall für Steine (je Kachelart fest)
    for (int i = 0; i < 9; ++i) {                              // Neun Steine in der Erde
        int rx = std::max(1, rnd.rangeInt(2, 5) * u);          // Halbe Breite des Steins
        int ry = std::max(1, rx * 2 / 3);                      // Halbe Höhe des Steins
        int sx = rnd.rangeInt(rx, tile - rx - 1);              // Position x (nicht über den Rand)
        int sy = rnd.rangeInt(top + ry + 2 * u, tile - ry - 1); // Position y unterhalb der Oberfläche
        c.fillEllipse(sx, sy, rx, ry, shade(rgba(120, 85, 55), rnd.range(0.6f, 1.3f))); // Stein zeichnen
    }                                                          // Ende der Steinschleife
    if (street) {                                              // Straßenbelag
        c.fillRect(0, 0, tile, top, rgba(60, 60, 66));         // Asphalt
        int stripeW = tile / 6;                                // Breite eines Zebrastreifens
        for (int x = 0; x < tile; x += stripeW * 2) {          // Zebrastreifen im Wechsel
            std::vector<std::pair<float, float>> stripe = {    // Schräg gestellter Streifen (Perspektive)
                {static_cast<float>(x + u), static_cast<float>(top)}, // Unten links
                {static_cast<float>(x + 3 * u), 0.0f},         // Oben links
                {static_cast<float>(x + 3 * u + stripeW), 0.0f}, // Oben rechts
                {static_cast<float>(x + u + stripeW), static_cast<float>(top)}}; // Unten rechts
            c.fillPolygon(stripe, rgba(235, 235, 235));        // Weißer Streifen
        }                                                      // Ende der Streifenschleife
        c.fillRect(0, top, tile, 2 * u, rgba(45, 45, 50));     // Dunkle Kante unter dem Asphalt
    } else {                                                   // Gehweg
        c.fillRect(0, 0, tile, top, rgba(175, 175, 180));      // Gehwegplatten
        c.fillRect(0, 0, tile, u, rgba(215, 215, 220));        // Helle Oberkante
        c.fillRect(0, top / 2, tile, std::max(1, u / 2), rgba(150, 150, 155)); // Fuge längs
        c.fillRect(tile / 2, 0, u, top, rgba(140, 140, 145));  // Fuge quer in der Mitte
        c.fillRect(0, 0, u, top, rgba(140, 140, 145));         // Fuge quer am Rand
        c.fillRect(0, top, tile, 2 * u, rgba(110, 110, 115));  // Bordstein-Kante
    }                                                          // Ende der Unterscheidung
} // Ende von drawGroundTile

// Zeichnet den Ankerring, an dem der Enterhaken greifen kann
void drawAnchorRing(Image& image) {                            // Beginn von drawAnchorRing
    Canvas c(image);                                           // Zeichenfläche
    int s = image.width;                                       // Bildgröße
    int t = std::max(2, s / 7);                                // Dicke des Rings
    c.fillRect(s / 2 - t / 2, 0, t, s / 3, rgba(110, 110, 120)); // Halterung oben
    c.ring(s / 2, s * 6 / 10, s * 35 / 100, t, rgba(170, 170, 180)); // Stahlring
    c.ring(s / 2, s * 6 / 10, s * 35 / 100, std::max(1, t / 3), rgba(220, 220, 230)); // Glanz außen
    addOutline(image, rgba(30, 30, 40), 1);                    // Umriss
} // Ende von drawAnchorRing

// Zeichnet das Symbol eines Gegenstands (für Inventar und Shop)
void drawItemIcon(Image& image, const std::string& icon) {     // Beginn von drawItemIcon
    Canvas c(image);                                           // Zeichenfläche
    int s = image.width;                                       // Bildgröße
    int u = std::max(1, s / 16);                               // Grundeinheit
    if (icon == "beutel") {                                    // Einkaufsbeutel
        std::vector<std::pair<float, float>> bag = {           // Beutelform (Trapez)
            {s * 0.18f, s * 0.38f}, {s * 0.82f, s * 0.38f}, {s * 0.88f, s * 0.92f}, {s * 0.12f, s * 0.92f}}; // Vier Eckpunkte
        c.ring(s * 38 / 100, s * 36 / 100, s / 7, std::max(1, u), rgba(90, 140, 60)); // Linker Henkel
        c.ring(s * 62 / 100, s * 36 / 100, s / 7, std::max(1, u), rgba(90, 140, 60)); // Rechter Henkel
        c.fillPolygon(bag, rgba(120, 180, 80));                // Beutel grün
        c.fillRect(s / 5, s * 55 / 100, s * 6 / 10, std::max(1, s / 10), rgba(250, 240, 200)); // Heller Streifen
        c.fillRect(s * 45 / 100, s * 7 / 10, s / 10, s / 10, rgba(240, 140, 30)); // Logo-Punkt
    } else if (icon == "haken") {                              // Enterhaken
        c.ring(s / 2, s * 2 / 10, s / 8, std::max(1, u), rgba(160, 110, 60)); // Seilöse
        c.fillRect(s / 2 - u, s * 3 / 10, 2 * u, s * 45 / 100, rgba(170, 170, 180)); // Schaft
        c.line(s / 2, s * 75 / 100, s / 5, s / 2, rgba(170, 170, 180), std::max(1, 2 * u)); // Linke Kralle
        c.line(s / 2, s * 75 / 100, s * 8 / 10, s / 2, rgba(170, 170, 180), std::max(1, 2 * u)); // Rechte Kralle
        c.line(s / 2, s * 75 / 100, s / 2, s * 9 / 10, rgba(170, 170, 180), std::max(1, 2 * u)); // Mittlere Kralle
        c.line(s / 5, s / 2, s / 5 + u, s * 4 / 10, rgba(220, 220, 230), std::max(1, u)); // Spitze links
        c.line(s * 8 / 10, s / 2, s * 8 / 10 - u, s * 4 / 10, rgba(220, 220, 230), std::max(1, u)); // Spitze rechts
        c.line(s / 2, s * 2 / 10, s / 8, s / 10, rgba(160, 110, 60), std::max(1, u)); // Seilende
    } else if (icon == "energy") {                             // Dose Energy
        c.fillRect(s * 3 / 10, s / 6, s * 4 / 10, s * 7 / 10, rgba(40, 40, 50)); // Dosenkörper
        c.fillRect(s * 3 / 10, s / 6, s * 4 / 10, std::max(1, s / 12), rgba(200, 200, 210)); // Deckel
        c.fillRect(s * 3 / 10, s * 8 / 10, s * 4 / 10, std::max(1, s / 16), rgba(200, 200, 210)); // Boden
        std::vector<std::pair<float, float>> bolt = {          // Blitz-Symbol
            {s * 0.55f, s * 0.28f}, {s * 0.38f, s * 0.52f}, {s * 0.5f, s * 0.52f}, // Obere Hälfte
            {s * 0.44f, s * 0.74f}, {s * 0.62f, s * 0.46f}, {s * 0.5f, s * 0.46f}}; // Untere Hälfte
        c.fillPolygon(bolt, rgba(250, 220, 40));               // Blitz gelb
        c.fillRect(s * 33 / 100, s / 4, std::max(1, u), s / 2, rgba(110, 110, 130)); // Lichtreflex
    } else {                                                   // Unbekanntes Symbol
        c.fillRect(s / 5, s / 5, s * 3 / 5, s * 3 / 5, rgba(180, 140, 90)); // Kiste
        int ts = std::max(1, s / 16);                          // Textgröße
        Font::drawText(c, (s - Font::textWidth("?", ts)) / 2, (s - 7 * ts) / 2, "?", rgba(40, 30, 20), ts); // Fragezeichen
    }                                                          // Ende der Unterscheidung
    addOutline(image, rgba(25, 20, 30), 1);                    // Umriss
} // Ende von drawItemIcon

// Zeichnet die kleinen Symbole der Menüleiste
void drawHudIcon(Image& image, const std::string& kind) {      // Beginn von drawHudIcon
    Canvas c(image);                                           // Zeichenfläche
    int s = image.width;                                       // Bildgröße
    if (kind == "herz") {                                      // Herz für die Lebensleiste
        c.fillCircle(s * 3 / 10, s * 35 / 100, s / 4, rgba(230, 40, 60)); // Linke Rundung
        c.fillCircle(s * 7 / 10, s * 35 / 100, s / 4, rgba(230, 40, 60)); // Rechte Rundung
        std::vector<std::pair<float, float>> tip = {{s * 0.06f, s * 0.42f}, {s * 0.94f, s * 0.42f}, {s * 0.5f, s * 0.95f}}; // Spitze
        c.fillPolygon(tip, rgba(230, 40, 60));                 // Spitze füllen
        c.fillRect(s / 5, s / 4, std::max(1, s / 8), std::max(1, s / 8), rgba(255, 170, 180)); // Glanzpunkt
    } else if (kind == "blitz") {                              // Blitz für die Ausdauer
        std::vector<std::pair<float, float>> bolt = {          // Blitzform
            {s * 0.6f, s * 0.02f}, {s * 0.15f, s * 0.55f}, {s * 0.45f, s * 0.55f}, // Obere Hälfte
            {s * 0.35f, s * 0.98f}, {s * 0.85f, s * 0.42f}, {s * 0.55f, s * 0.42f}}; // Untere Hälfte
        c.fillPolygon(bolt, rgba(250, 210, 40));               // Gelb füllen
    } else {                                                   // Münze für die Coins
        c.fillCircle(s / 2, s / 2, s / 2 - 1, rgba(190, 130, 20)); // Dunkler Rand
        c.fillCircle(s / 2, s / 2, s / 2 - 1 - std::max(1, s / 8), rgba(250, 200, 40)); // Goldene Fläche
        c.fillRect(s * 4 / 10, s / 4, std::max(1, s / 8), s / 2, rgba(255, 240, 150)); // Glanz
    }                                                          // Ende der Unterscheidung
    addOutline(image, rgba(25, 20, 30), 1);                    // Umriss
} // Ende von drawHudIcon

// Hintergrundebene 1: Wolken (kachelbar)
void drawCloudLayer(Image& image, int tile) {                  // Beginn von drawCloudLayer
    Canvas c(image);                                           // Zeichenfläche
    int w = image.width;                                       // Bildbreite
    int h = image.height;                                      // Bildhöhe
    SimpleRandom rnd(2024);                                    // Fester Zufall für immer gleiche Wolken
    int count = std::max(4, w / (tile * 2));                   // Anzahl der Wolken
    for (int i = 0; i < count; ++i) {                          // Alle Wolken
        int cx = rnd.rangeInt(0, w - 1);                       // Mittelpunkt x
        int cy = static_cast<int>(rnd.range(0.14f, 0.36f) * static_cast<float>(h)); // Mittelpunkt y im oberen Himmel
        float size = rnd.range(0.35f, 0.7f) * static_cast<float>(tile); // Grundgröße
        int puffs = rnd.rangeInt(4, 7);                        // Anzahl der Wattebäusche
        for (int p = 0; p < puffs; ++p) {                      // Alle Bäusche
            int px = cx + static_cast<int>((static_cast<float>(p) - static_cast<float>(puffs) / 2.0f) * size * 0.45f); // Position nebeneinander
            int py = cy - static_cast<int>(rnd.range(0.0f, 0.35f) * size); // Leicht unterschiedliche Höhe
            int r = static_cast<int>(size * rnd.range(0.35f, 0.6f)); // Radius des Bausches
            drawWrapped(w, [&](int off) { c.fillCircle(px + off, py + r / 5, r, rgba(205, 220, 240)); }); // Schatten unten
            drawWrapped(w, [&](int off) { c.fillCircle(px + off, py, r, rgba(250, 252, 255)); }); // Weißer Bausch
        }                                                      // Ende der Bauschschleife
    }                                                          // Ende der Wolkenschleife
} // Ende von drawCloudLayer

// Hintergrundebene 2: ferne Berge und Hügel (kachelbar über Sinuswellen)
void drawMountainLayer(Image& image, int tile, int groundPx) { // Beginn von drawMountainLayer
    Canvas c(image);                                           // Zeichenfläche
    int w = image.width;                                       // Bildbreite
    int h = image.height;                                      // Bildhöhe
    float T = static_cast<float>(tile);                        // Kachelgröße als Kommazahl
    for (int x = 0; x < w; ++x) {                              // Jede Bildspalte
        float u = static_cast<float>(x) / static_cast<float>(w) * 2.0f * PI; // Position als Winkel (kachelbar)
        float mountain = 1.6f + 0.6f * std::sin(2.0f * u + 0.7f) + 0.35f * std::sin(5.0f * u + 1.9f) + 0.15f * std::sin(11.0f * u); // Berghöhe in Kacheln
        int my = groundPx - static_cast<int>(mountain * T);    // Obere Kante des Berges
        c.vLine(x, my, h - 1, rgba(130, 150, 200));            // Bergspalte füllen
        if (mountain > 2.0f) c.vLine(x, my, my + static_cast<int>((mountain - 2.0f) * T * 0.4f), rgba(240, 245, 255)); // Schneekappe
        float hill = 0.7f + 0.25f * std::sin(3.0f * u + 2.5f) + 0.12f * std::sin(8.0f * u + 0.4f); // Hügelhöhe in Kacheln
        int hy = groundPx - static_cast<int>(hill * T);        // Obere Kante des Hügels
        c.vLine(x, hy, h - 1, rgba(120, 170, 150));            // Hügelspalte füllen
    }                                                          // Ende der Spaltenschleife
} // Ende von drawMountainLayer

// Hintergrundebene 3: ferne Stadtsilhouette mit Fenstern
void drawCityLayer(Image& image, int tile, int groundPx) {     // Beginn von drawCityLayer
    Canvas c(image);                                           // Zeichenfläche
    int w = image.width;                                       // Bildbreite
    int h = image.height;                                      // Bildhöhe
    float T = static_cast<float>(tile);                        // Kachelgröße
    SimpleRandom rnd(99);                                      // Fester Zufall
    int x = 0;                                                 // Aktuelle Position
    int win = std::max(1, tile / 16);                          // Fenstergröße
    while (x < w) {                                            // Gebäude nebeneinander bis zum Rand
        int bw = static_cast<int>(rnd.range(0.5f, 1.2f) * T);  // Gebäudebreite
        int bh = static_cast<int>(rnd.range(1.3f, 3.0f) * T);  // Gebäudehöhe
        Color col = shade(rgba(95, 115, 160), rnd.range(0.85f, 1.1f)); // Gebäudefarbe
        int bx = x;                                            // Linke Kante dieses Gebäudes
        int by = groundPx - bh;                                // Obere Kante dieses Gebäudes
        drawWrapped(w, [&](int off) {                          // Gebäude kachelbar zeichnen
            c.fillRect(bx + off, by, bw, h - by, col);         // Gebäudekörper
            for (int wy = by + 2 * win; wy < groundPx - 2 * win; wy += 3 * win) { // Fensterreihen
                for (int wx = bx + win; wx < bx + bw - win; wx += 3 * win) { // Fensterspalten
                    bool lit = ((wx / win) * 7 + (wy / win) * 13) % 5 == 0; // Manche Fenster sind beleuchtet
                    c.fillRect(wx + off, wy, win, win, lit ? rgba(250, 230, 150) : shade(col, 1.25f)); // Fenster zeichnen
                }                                              // Ende der Fensterspalten
            }                                                  // Ende der Fensterreihen
        });                                                    // Ende des kachelbaren Zeichnens
        x += bw + static_cast<int>(rnd.range(0.0f, 0.25f) * T); // Zum nächsten Gebäude
    }                                                          // Ende der Gebäudeschleife
} // Ende von drawCityLayer

// Hintergrundebene 4: nahe, bunte Häuser und Bäume
void drawHouseLayer(Image& image, int tile, int groundPx) {    // Beginn von drawHouseLayer
    Canvas c(image);                                           // Zeichenfläche
    int w = image.width;                                       // Bildbreite
    int h = image.height;                                      // Bildhöhe
    float T = static_cast<float>(tile);                        // Kachelgröße
    SimpleRandom rnd(4711);                                    // Fester Zufall
    const Color walls[5] = {rgba(240, 200, 150), rgba(200, 225, 240), rgba(240, 180, 170), rgba(210, 235, 190), rgba(250, 235, 170)}; // Pastellfarben
    int x = 0;                                                 // Aktuelle Position
    int u = std::max(1, tile / 32);                            // Grundeinheit
    int index = 0;                                             // Zähler für Haus/Baum-Wechsel
    while (x < w) {                                            // Bis zum rechten Rand
        if (index % 3 == 2) {                                  // Jedes dritte Objekt ist ein Baum
            int tx = x + static_cast<int>(0.4f * T);           // Stammposition
            int crown = static_cast<int>(rnd.range(0.35f, 0.5f) * T); // Kronenradius
            int trunkH = static_cast<int>(0.6f * T);           // Stammhöhe
            drawWrapped(w, [&](int off) {                      // Baum kachelbar zeichnen
                c.fillRect(tx + off - 2 * u, groundPx - trunkH, 4 * u, trunkH + h, rgba(110, 75, 45)); // Stamm
                c.fillCircle(tx + off, groundPx - trunkH - crown / 2, crown, rgba(60, 140, 70)); // Krone
                c.fillCircle(tx + off - crown / 2, groundPx - trunkH, crown * 2 / 3, rgba(70, 155, 75)); // Krone links
                c.fillCircle(tx + off + crown / 2, groundPx - trunkH, crown * 2 / 3, rgba(50, 125, 65)); // Krone rechts
            });                                                // Ende des Baumes
            x += static_cast<int>(0.8f * T);                   // Platz für den Baum
        } else {                                               // Sonst ein Haus
            int hw = static_cast<int>(rnd.range(1.4f, 2.1f) * T); // Hausbreite
            int hh = static_cast<int>(rnd.range(1.2f, 1.8f) * T); // Haushöhe (ohne Dach)
            int roofH = static_cast<int>(0.55f * T);           // Dachhöhe
            Color wall = walls[rnd.rangeInt(0, 4)];            // Wandfarbe auswählen
            Color roof = shade(rgba(170, 70, 50), rnd.range(0.8f, 1.1f)); // Dachfarbe
            int hx = x;                                        // Linke Kante des Hauses
            int hy = groundPx - hh;                            // Obere Kante der Hauswand
            drawWrapped(w, [&](int off) {                      // Haus kachelbar zeichnen
                c.fillRect(hx + off, hy, hw, h - hy, wall);    // Hauswand
                std::vector<std::pair<float, float>> roofPoly = { // Dach als Dreieck
                    {static_cast<float>(hx + off - 3 * u), static_cast<float>(hy)}, // Linke Traufe
                    {static_cast<float>(hx + off + hw / 2), static_cast<float>(hy - roofH)}, // First
                    {static_cast<float>(hx + off + hw + 3 * u), static_cast<float>(hy)}}; // Rechte Traufe
                c.fillPolygon(roofPoly, roof);                 // Dach füllen
                int winW = static_cast<int>(0.32f * T);        // Fensterbreite
                int winH = static_cast<int>(0.38f * T);        // Fensterhöhe
                for (int wy = hy + static_cast<int>(0.2f * T); wy + winH < groundPx - static_cast<int>(0.15f * T); wy += static_cast<int>(0.6f * T)) { // Stockwerke
                    for (int wx = hx + static_cast<int>(0.2f * T); wx + winW < hx + hw - static_cast<int>(0.1f * T); wx += static_cast<int>(0.55f * T)) { // Fenster je Stockwerk
                        c.fillRect(wx + off, wy, winW, winH, rgba(250, 250, 245)); // Fensterrahmen
                        c.fillRect(wx + off + u, wy + u, winW - 2 * u, winH - 2 * u, rgba(140, 190, 225)); // Glas
                        c.fillRect(wx + off + winW / 2, wy, u, winH, rgba(250, 250, 245)); // Fensterkreuz senkrecht
                    }                                          // Ende der Fensterschleife
                }                                              // Ende der Stockwerksschleife
            });                                                // Ende des Hauses
            x += hw + static_cast<int>(rnd.range(0.05f, 0.25f) * T); // Zum nächsten Objekt
        }                                                      // Ende der Unterscheidung
        ++index;                                               // Zähler erhöhen
    }                                                          // Ende der Schleife
    addOutline(image, rgba(60, 50, 60), 1);                    // Dünner Umriss um alles
} // Ende von drawHouseLayer

} // Ende des Namensraums SpriteFactory
