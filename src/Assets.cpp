// Assets.cpp - Umsetzung der Bildverwaltung
#include "Assets.h" // Eigene Deklarationen

#include <iostream> // std::cout für Hinweise in der Konsole

#include "ImageIO.h" // Laden/Speichern von BMP-Dateien

// Legt Ordner und Kachelgröße fest
void Assets::configure(const std::string& assetDir, const std::string& dummyDir, int tileSize, bool exportDummies) { // Beginn von configure
    m_assetDir = ImageIO::joinPath(assetDir, std::to_string(tileSize));  // Eigene Sprites liegen in assets/<Größe>/
    m_dummyDir = ImageIO::joinPath(dummyDir, std::to_string(tileSize));  // Vorlagen landen in assets_dummies/<Größe>/
    m_tileSize = tileSize;                                               // Kachelgröße merken
    m_exportDummies = exportDummies;                                     // Export-Einstellung merken
    m_images.clear();                                                    // Alte Bilder verwerfen
    m_custom.clear();                                                    // Alte Merker verwerfen
    m_customCount = 0;                                                   // Zähler zurücksetzen
    m_exportedCount = 0;                                                 // Zähler zurücksetzen
    m_missing.resize(8, 8, rgba(255, 0, 255));                           // Ersatzbild: kleines magentafarbenes Quadrat
    if (m_exportDummies) ImageIO::makeDirectories(m_dummyDir);           // Vorlagenordner bei Bedarf anlegen
} // Ende von configure

// Lädt ein eigenes Bild oder erzeugt einen Platzhalter
const Image& Assets::loadOrGenerate(const std::string& name, int width, int height, const Generator& generator) { // Beginn von loadOrGenerate
    Image image;                                                         // Ergebnisbild
    std::string customPath = ImageIO::joinPath(m_assetDir, name + ".bmp"); // Pfad zu einem eigenen Sprite
    bool custom = ImageIO::loadBMP(customPath, image);                   // Versuchen, das eigene Sprite zu laden
    if (custom) {                                                        // Eigenes Sprite gefunden
        if (image.width != width || image.height != height) {            // Passt die Größe nicht?
            std::cout << "Hinweis: " << customPath << " hat " << image.width << "x" << image.height // Hinweis ausgeben
                      << " statt " << width << "x" << height << " Pixel und wird skaliert.\n"; // ... mit erwarteter Größe
            image = scaleImage(image, width, height);                    // Auf die erwartete Größe skalieren
        }                                                                // Ende der Größenprüfung
        ++m_customCount;                                                 // Eigene Bilder zählen
    } else {                                                             // Kein eigenes Sprite vorhanden
        image.resize(width, height);                                     // Leeres, durchsichtiges Bild anlegen
        if (generator) generator(image);                                 // Platzhalter hineinzeichnen
        image.computeBounds();                                           // Sichtbaren Bereich bestimmen
        if (m_exportDummies) {                                           // Vorlagen sollen exportiert werden
            std::string dummyPath = ImageIO::joinPath(m_dummyDir, name + ".bmp"); // Pfad der Vorlage
            if (!ImageIO::fileExists(dummyPath) && ImageIO::saveBMP(dummyPath, image)) ++m_exportedCount; // Nur neue Vorlagen schreiben
        }                                                                // Ende des Exports
    }                                                                    // Ende der Unterscheidung
    m_custom[name] = custom;                                             // Herkunft merken
    m_images[name] = std::move(image);                                   // Bild unter seinem Namen speichern
    return m_images[name];                                               // Gespeichertes Bild zurückgeben
} // Ende von loadOrGenerate

// Holt ein Bild über seinen Namen (unbekannte Namen liefern das Ersatzbild)
const Image& Assets::get(const std::string& name) const {               // Beginn von get
    auto it = m_images.find(name);                                       // In der Tabelle suchen
    if (it == m_images.end()) return m_missing;                          // Nicht gefunden -> Ersatzbild
    return it->second;                                                   // Gefundenes Bild zurückgeben
} // Ende von get

bool Assets::has(const std::string& name) const { return m_images.count(name) > 0; } // Bild vorhanden?

// Gibt zurück, ob ein Bild aus einer eigenen Datei stammt
bool Assets::isCustom(const std::string& name) const {                  // Beginn von isCustom
    auto it = m_custom.find(name);                                       // Merker suchen
    return it != m_custom.end() && it->second;                           // true, wenn eigenes Bild
} // Ende von isCustom
