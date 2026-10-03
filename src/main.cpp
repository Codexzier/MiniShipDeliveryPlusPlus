// main.cpp - Einstiegspunkt des Programms "Mini Ship Delivery"
#define SDL_MAIN_HANDLED // Wir schreiben unsere eigene main-Funktion (SDL soll sie nicht ersetzen)
#include <SDL.h>         // SDL2-Grundfunktionen

#include "Game.h" // Hauptklasse des Spiels

// Startpunkt: erzeugt das Spiel und startet die Spielschleife
int main(int argc, char** argv) { // Beginn von main
    SDL_SetMainReady();           // SDL mitteilen, dass main bereit ist (wegen SDL_MAIN_HANDLED)
    Game game;                    // Spielobjekt anlegen
    return game.run(argc, argv);  // Spiel laufen lassen und dessen Rückgabewert weitergeben
} // Ende von main
