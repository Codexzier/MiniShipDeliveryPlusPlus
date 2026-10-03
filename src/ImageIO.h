// ImageIO.h - Laden und Speichern von Bildern im BMP-Format sowie Datei-Hilfsfunktionen
#pragma once // Header nur einmal einbinden

#include <string> // std::string für Dateipfade

#include "Graphics.h" // Image

namespace ImageIO {                                                // Namensraum für Datei-Funktionen
bool fileExists(const std::string& path);                          // Prüft, ob eine Datei existiert
bool makeDirectories(const std::string& path);                     // Legt einen Ordner (mit Unterordnern) an
std::string joinPath(const std::string& a, const std::string& b);  // Fügt zwei Pfadteile mit "/" zusammen
bool loadBMP(const std::string& path, Image& out);                 // BMP laden (Magenta wird durchsichtig)
bool saveBMP(const std::string& path, const Image& image);         // BMP speichern (Durchsichtiges wird Magenta)
} // Ende des Namensraums ImageIO
