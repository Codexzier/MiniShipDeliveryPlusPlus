// ImageIO.cpp - Bilddateien über die BMP-Funktionen von SDL2 lesen und schreiben
#include "ImageIO.h" // Eigene Deklarationen

#include <SDL.h>        // SDL_LoadBMP, SDL_SaveBMP und Oberflächen (Surfaces)
#include <filesystem>   // std::filesystem für Ordner und Dateiprüfungen
#include <system_error> // std::error_code, damit Dateifehler keine Ausnahmen werfen

namespace ImageIO { // Beginn des Namensraums

constexpr Color MAGENTA = rgba(255, 0, 255); // Magenta steht in BMP-Dateien für "durchsichtig"

// Prüft, ob unter dem Pfad eine Datei liegt
bool fileExists(const std::string& path) {                         // Beginn von fileExists
    std::error_code error;                                         // Fehlercode statt Ausnahme
    return std::filesystem::is_regular_file(path, error);          // true, wenn es eine normale Datei ist
} // Ende von fileExists

// Legt einen Ordner inklusive aller fehlenden Elternordner an
bool makeDirectories(const std::string& path) {                    // Beginn von makeDirectories
    std::error_code error;                                         // Fehlercode statt Ausnahme
    std::filesystem::create_directories(path, error);              // Ordner anlegen (existierende sind kein Fehler)
    return std::filesystem::is_directory(path, error);             // Erfolgreich, wenn der Ordner jetzt existiert
} // Ende von makeDirectories

// Verbindet zwei Pfadteile mit genau einem Schrägstrich
std::string joinPath(const std::string& a, const std::string& b) { // Beginn von joinPath
    if (a.empty()) return b;                                       // Kein erster Teil -> nur zweiten zurückgeben
    if (a.back() == '/' || a.back() == '\\') return a + b;         // Schrägstrich schon vorhanden
    return a + "/" + b;                                            // Schrägstrich einfügen
} // Ende von joinPath

// Lädt eine BMP-Datei und wandelt sie in unser Image-Format (ARGB) um
bool loadBMP(const std::string& path, Image& out) {                // Beginn von loadBMP
    if (!fileExists(path)) return false;                           // Datei fehlt -> nichts laden
    SDL_Surface* loaded = SDL_LoadBMP(path.c_str());               // Datei mit SDL lesen
    if (!loaded) return false;                                     // Lesen fehlgeschlagen
    SDL_Surface* converted = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_ARGB8888, 0); // In 32-Bit ARGB umwandeln
    SDL_FreeSurface(loaded);                                       // Original wird nicht mehr gebraucht
    if (!converted) return false;                                  // Umwandlung fehlgeschlagen
    out.resize(converted->w, converted->h);                        // Zielbild auf die passende Größe bringen
    SDL_LockSurface(converted);                                    // Pixelzugriff erlauben
    bool hasAlpha = false;                                         // Merker: enthält die Datei echte Alphawerte?
    for (int y = 0; y < converted->h; ++y) {                       // Alle Zeilen
        const Uint32* row = reinterpret_cast<const Uint32*>(static_cast<const Uint8*>(converted->pixels) + y * converted->pitch); // Zeilenanfang
        for (int x = 0; x < converted->w; ++x) {                   // Alle Spalten
            Color c = row[x];                                      // Pixel lesen
            if (alphaOf(c) != 255) hasAlpha = true;                // Nicht voll deckendes Pixel gefunden
            out.set(x, y, c);                                      // In das Zielbild kopieren
        }                                                          // Ende der Spaltenschleife
    }                                                              // Ende der Zeilenschleife
    SDL_UnlockSurface(converted);                                  // Pixelzugriff beenden
    SDL_FreeSurface(converted);                                    // Speicher der umgewandelten Oberfläche freigeben
    bool allTransparent = hasAlpha;                                // Prüfen, ob alles durchsichtig ist (32-Bit-BMP ohne Alpha)
    for (Color c : out.pixels) if (alphaOf(c) != 0) { allTransparent = false; break; } // Ein sichtbares Pixel reicht
    for (Color& c : out.pixels) {                                  // Alle Pixel nachbearbeiten
        if (allTransparent) c = withAlpha(c, 255);                 // Fehlenden Alphakanal als "deckend" deuten
        if ((c & 0x00FFFFFFu) == (MAGENTA & 0x00FFFFFFu)) c = TRANSPARENT; // Magenta wird durchsichtig
    }                                                              // Ende der Nachbearbeitung
    out.computeBounds();                                           // Sichtbaren Bereich bestimmen
    return true;                                                   // Erfolgreich geladen
} // Ende von loadBMP

// Speichert ein Bild als 24-Bit-BMP; durchsichtige Stellen werden Magenta (kompatibel mit jedem Malprogramm)
bool saveBMP(const std::string& path, const Image& image) {        // Beginn von saveBMP
    if (image.empty()) return false;                               // Leeres Bild kann nicht gespeichert werden
    std::vector<Color> copy = image.pixels;                        // Kopie der Pixel anlegen
    for (Color& c : copy) {                                        // Alle Pixel durchgehen
        if (alphaOf(c) < 128) c = MAGENTA;                         // Durchsichtige Pixel als Magenta speichern
        else c = withAlpha(c, 255);                                // Sichtbare Pixel voll deckend speichern
    }                                                              // Ende der Schleife
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(copy.data(), image.width, image.height, 32, image.width * 4, SDL_PIXELFORMAT_ARGB8888); // Oberfläche um die Pixel legen
    if (!surface) return false;                                    // Anlegen fehlgeschlagen
    SDL_Surface* rgb = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB24, 0); // In 24 Bit ohne Alpha umwandeln
    SDL_FreeSurface(surface);                                      // Hilfsoberfläche freigeben
    if (!rgb) return false;                                        // Umwandlung fehlgeschlagen
    bool ok = SDL_SaveBMP(rgb, path.c_str()) == 0;                 // Datei schreiben (0 bedeutet Erfolg)
    SDL_FreeSurface(rgb);                                          // Speicher freigeben
    return ok;                                                     // Ergebnis melden
} // Ende von saveBMP

} // Ende des Namensraums ImageIO
