// Effects.h - Kleine Effekte: Wasserspritzer, Funkeln und schwebende Texte (z.B. "+5")
#pragma once // Header nur einmal einbinden

#include <string> // std::string
#include <vector> // std::vector

#include "Graphics.h" // Canvas

class Effects {                                                               // Beginn der Klasse Effects
public:                                                                       // Öffentliche Schnittstelle
    void splash(float x, float y);                                            // Wasserspritzer an einer Weltposition
    void sparkle(float x, float y, Color color);                              // Funkeln (z.B. beim Münzen sammeln)
    void floatingText(float x, float y, const std::string& text, Color color); // Aufsteigender Text
    void update(float dt);                                                    // Effekte bewegen und altern lassen
    void draw(Canvas& canvas, float camX, int tile, int uiScale) const;       // Effekte zeichnen
    void clear();                                                             // Alle Effekte entfernen
private:                                                                      // Interne Daten
    struct Particle {                                                         // Ein Partikel
        float x, y, vx, vy;                                                   // Position und Geschwindigkeit (Kacheln)
        float life, maxLife;                                                  // Restlebenszeit und Gesamtlebenszeit
        Color color;                                                          // Farbe
        float size;                                                           // Größe (Anteil einer Kachel)
    };                                                                        // Ende von Particle
    struct Text {                                                             // Ein schwebender Text
        float x, y, life;                                                     // Position und Restlebenszeit
        std::string text;                                                     // Inhalt
        Color color;                                                          // Farbe
    };                                                                        // Ende von Text
    std::vector<Particle> m_particles;                                        // Alle Partikel
    std::vector<Text> m_texts;                                                // Alle Texte
}; // Ende der Klasse Effects
