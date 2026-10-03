// Effects.cpp - Umsetzung der Partikel und schwebenden Texte
#include "Effects.h" // Eigene Deklarationen

#include <algorithm> // std::remove_if

#include "Common.h" // SimpleRandom
#include "Font.h"   // Schrift für die Texte

namespace { SimpleRandom g_effectRandom(12345); } // Zufallsgenerator für die Effekte

// Erzeugt einen Wasserspritzer aus vielen blauen Tropfen
void Effects::splash(float x, float y) {                                    // Beginn von splash
    for (int i = 0; i < 26; ++i) {                                          // 26 Tropfen
        Particle p;                                                         // Neuer Tropfen
        p.x = x + g_effectRandom.range(-0.2f, 0.2f);                        // Start x leicht gestreut
        p.y = y;                                                            // Start auf der Wasseroberfläche
        p.vx = g_effectRandom.range(-1.8f, 1.8f);                           // Seitliche Geschwindigkeit
        p.vy = g_effectRandom.range(-6.0f, -2.5f);                          // Nach oben spritzen
        p.maxLife = g_effectRandom.range(0.5f, 0.9f);                       // Lebensdauer
        p.life = p.maxLife;                                                 // Volle Restzeit
        p.color = g_effectRandom.nextFloat() < 0.5f ? rgba(120, 190, 240) : rgba(220, 240, 255); // Blau oder Weiß
        p.size = g_effectRandom.range(0.03f, 0.06f);                        // Tropfengröße
        m_particles.push_back(p);                                           // Speichern
    }                                                                       // Ende der Schleife
} // Ende von splash

// Erzeugt ein kleines Funkeln
void Effects::sparkle(float x, float y, Color color) {                      // Beginn von sparkle
    for (int i = 0; i < 10; ++i) {                                          // 10 Funken
        Particle p;                                                         // Neuer Funke
        p.x = x;                                                            // Start x
        p.y = y;                                                            // Start y
        float angle = static_cast<float>(i) / 10.0f * 2.0f * PI;            // Gleichmäßig im Kreis verteilt
        p.vx = std::cos(angle) * 1.5f;                                      // Geschwindigkeit x
        p.vy = std::sin(angle) * 1.5f;                                      // Geschwindigkeit y
        p.maxLife = 0.4f;                                                   // Kurze Lebensdauer
        p.life = p.maxLife;                                                 // Volle Restzeit
        p.color = color;                                                    // Farbe übernehmen
        p.size = 0.035f;                                                    // Größe
        m_particles.push_back(p);                                           // Speichern
    }                                                                       // Ende der Schleife
} // Ende von sparkle

// Fügt einen aufsteigenden Text hinzu
void Effects::floatingText(float x, float y, const std::string& text, Color color) { // Beginn von floatingText
    m_texts.push_back(Text{x, y, 1.2f, text, color});                       // Text mit 1,2 Sekunden Lebensdauer speichern
} // Ende von floatingText

// Bewegt alle Effekte und entfernt abgelaufene
void Effects::update(float dt) {                                            // Beginn von update
    for (Particle& p : m_particles) {                                       // Alle Partikel
        p.vy += 14.0f * dt;                                                 // Schwerkraft
        p.x += p.vx * dt;                                                   // Bewegen x
        p.y += p.vy * dt;                                                   // Bewegen y
        p.life -= dt;                                                       // Altern
    }                                                                       // Ende der Partikelschleife
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p) { return p.life <= 0.0f; }), m_particles.end()); // Abgelaufene entfernen
    for (Text& t : m_texts) {                                               // Alle Texte
        t.y -= 0.8f * dt;                                                   // Nach oben schweben
        t.life -= dt;                                                       // Altern
    }                                                                       // Ende der Textschleife
    m_texts.erase(std::remove_if(m_texts.begin(), m_texts.end(), [](const Text& t) { return t.life <= 0.0f; }), m_texts.end()); // Abgelaufene entfernen
} // Ende von update

// Zeichnet alle Effekte
void Effects::draw(Canvas& canvas, float camX, int tile, int uiScale) const { // Beginn von draw
    const float T = static_cast<float>(tile);                               // Kachelgröße
    for (const Particle& p : m_particles) {                                 // Alle Partikel
        int r = std::max(1, static_cast<int>(p.size * T * (p.life / p.maxLife))); // Partikel werden kleiner
        canvas.fillCircle(static_cast<int>((p.x - camX) * T), static_cast<int>(p.y * T), r, p.color); // Partikel zeichnen
    }                                                                       // Ende der Partikelschleife
    for (const Text& t : m_texts) {                                         // Alle Texte
        int w = Font::textWidth(t.text, uiScale);                           // Textbreite
        Font::drawTextShadow(canvas, static_cast<int>((t.x - camX) * T) - w / 2, static_cast<int>(t.y * T), t.text, t.color, rgba(20, 20, 30), uiScale); // Text mit Schatten
    }                                                                       // Ende der Textschleife
} // Ende von draw

// Entfernt alle Effekte
void Effects::clear() {                                                     // Beginn von clear
    m_particles.clear();                                                    // Partikel löschen
    m_texts.clear();                                                        // Texte löschen
} // Ende von clear
