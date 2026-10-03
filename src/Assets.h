// Assets.h - Verwaltung aller Bilder: eigene BMP-Dateien oder automatisch erzeugte Platzhalter
#pragma once // Header nur einmal einbinden

#include <functional> // std::function für die Platzhalter-Erzeuger
#include <map>        // std::map als Nachschlagetabelle Name -> Bild
#include <string>     // std::string

#include "Graphics.h" // Image

class Assets {                                                         // Beginn der Klasse Assets
public:                                                                // Öffentliche Schnittstelle
    using Generator = std::function<void(Image&)>;                     // Funktion, die einen Platzhalter in ein Bild zeichnet

    void configure(const std::string& assetDir, const std::string& dummyDir, int tileSize, bool exportDummies); // Pfade und Größe festlegen
    const Image& loadOrGenerate(const std::string& name, int width, int height, const Generator& generator); // Bild holen/erzeugen
    const Image& get(const std::string& name) const;                   // Bereits geladenes Bild holen
    bool has(const std::string& name) const;                           // Ist ein Bild mit diesem Namen geladen?
    bool isCustom(const std::string& name) const;                      // Stammt das Bild aus einer eigenen Datei?
    int tileSize() const { return m_tileSize; }                        // Aktuelle Kachel-/Spritegröße
    int customCount() const { return m_customCount; }                  // Anzahl geladener eigener Bilder
    int exportedCount() const { return m_exportedCount; }              // Anzahl exportierter Platzhalter

private:                                                               // Interne Daten
    std::map<std::string, Image> m_images;                             // Alle Bilder nach Namen
    std::map<std::string, bool> m_custom;                              // Merker, ob ein Bild aus einer Datei stammt
    std::string m_assetDir;                                            // Ordner für eigene Sprites
    std::string m_dummyDir;                                            // Ordner für exportierte Platzhalter-Vorlagen
    int m_tileSize = 128;                                              // Kachelgröße in Pixel
    bool m_exportDummies = false;                                      // Sollen Platzhalter als Vorlage gespeichert werden?
    int m_customCount = 0;                                             // Zähler für eigene Bilder
    int m_exportedCount = 0;                                           // Zähler für exportierte Vorlagen
    Image m_missing;                                                   // Ersatzbild, falls ein Name unbekannt ist
}; // Ende der Klasse Assets
