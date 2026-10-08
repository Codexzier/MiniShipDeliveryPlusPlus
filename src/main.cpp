// main.cpp - Einstiegspunkt von "Mini Ship Delivery"
#define SDL_MAIN_HANDLED // Eigene main-Funktion (SDL soll sie nicht ersetzen)
#include <SDL.h>         // SDL2

#include <iostream> // Fehlerausgabe
#include <string>   // std::string

#include "Game.h" // Das Spiel

int main(int argc, char** argv) {                                          // Programmstart
    (void)argc; (void)argv;                                                 // Keine Kommandozeilenparameter nötig
    SDL_SetMainReady();                                                     // SDL mitteilen, dass main bereit ist
    Game game;                                                              // Das Spiel anlegen
    std::string error;                                                      // Fehlertext
    if (!game.init(error)) {                                                // Laden und Fenster öffnen
        std::cerr << "Fehler beim Start: " << error << "\n";                // Fehler ausgeben
        game.shutdown();                                                    // Aufräumen
        return 1;                                                           // Mit Fehlercode beenden
    }                                                                       // Ende Fehlerfall
    game.run();                                                             // Hauptschleife
    game.shutdown();                                                        // Aufräumen
    return 0;                                                               // Erfolgreich beendet
}                                                                           // Ende von main
