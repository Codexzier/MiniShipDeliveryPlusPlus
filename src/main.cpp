// main.cpp - vorläufiger Test der Modellbibliothek
#define SDL_MAIN_HANDLED // Eigene main-Funktion
#include <SDL.h>         // SDL2
#include <iostream>      // Ausgabe
#include "ImageIO.h"     // Bilder
#include "ModelLibrary.h" // Modelle
int main(int argc, char** argv) {                                       // Testprogramm
    SDL_SetMainReady();                                                  // SDL vorbereiten
    ModelLibrary lib;                                                    // Bibliothek
    lib.load(MSD_SOURCE_DIR "/data/modelle.txt", "", MSD_SOURCE_DIR "/assets"); // Laden
    Image sheet(1400, 800, rgba(60, 110, 160));                          // Testbild
    Canvas c(sheet);                                                     // Zeichenfläche
    auto put = [&](const Sprite& s, int x, int y) { c.blit(s.image, x - s.anchorX, y - s.anchorY); c.fillCircle(x, y, 3, rgba(255, 0, 0)); }; // Zeichnen
    put(lib.sprite("haus_test", 0, 4, 128), 250, 350);                   // Haus
    put(lib.sprite("haus_holz", 0, 4, 128), 550, 300);                   // Kleines Haus
    for (int d = 0; d < 8; ++d) put(lib.sprite("boot", d * 0.785398f, 8, 80), 100 + d * 150, 600); // Boot in 8 Richtungen
    put(lib.sprite("brigg", 0, 16, 80), 1000, 300);                      // Brigg Richtung 0 (+x)
    c.line(1000, 300, 1000 + 120, 300 + 60, rgba(255, 255, 0), 3);       // +x-Richtung anzeigen
    ImageIO::savePNG(argc > 1 ? argv[1] : "lib.png", sheet);             // Speichern
    return 0;                                                            // Ende
}                                                                        // Ende von main
