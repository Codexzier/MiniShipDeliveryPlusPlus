// ModelLibrary.h - Verwaltet alle 3D-Modelle (aus data/modelle.txt) und ihre gerenderten Sprites
//
// Modelle können sein:
//   - eine OBJ-Datei aus den Kenney-Paketen (datei = ...)
//   - zusammengesetzt aus anderen Modellen (teil_1 = name | x, y, z | drehung | skalierung)
//   - ein generiertes Haus aus Fantasy-Town-Bauteilen (typ = haus)
// Sprites werden erst bei Bedarf gerendert und dann zwischengespeichert.
#pragma once // Header nur einmal einbinden

#include <map>    // std::map
#include <memory> // std::unique_ptr
#include <string> // std::string
#include <vector> // std::vector

#include "FbxLoader.h"    // Animierte Figuren
#include "Mesh.h"         // Mesh
#include "PropertyFile.h" // Textdateien
#include "Rasterizer.h"   // Sprite, RenderParams

class ModelLibrary {                                                               // Beginn der Klasse
public:                                                                            // Öffentliche Schnittstelle
    bool load(const std::string& modelFile, const std::string& figureFile, const std::string& assetDir); // Definitionen lesen
    bool has(const std::string& name) const;                                       // Gibt es ein Modell mit diesem Namen?
    const Mesh* mesh(const std::string& name);                                     // Fertiges Modell (bei Bedarf geladen)
    const Sprite& sprite(const std::string& name, float angle, int steps, float tileWidth); // Sprite in einer Drehung (Winkel wird auf steps gerundet)
    const Sprite& figure(const std::string& name, const std::string& anim, int direction, float time, float tileWidth); // Figur-Sprite (8 Richtungen)
    float figureDuration(const std::string& name, const std::string& anim);        // Länge einer Figurenanimation
    void setSupersample(int ss) { m_supersample = ss; }                            // Kantenglättung einstellen
    std::vector<std::string> names() const;                                        // Alle Modellnamen
    int renderedCount() const { return m_renderCount; }                            // Anzahl gerenderter Sprites (Statistik)

private:                                                                           // Interne Funktionen und Daten
    struct Def {                                                                   // Definition eines Modells
        std::string file;                                                          // OBJ-Datei (relativ zum Asset-Ordner)
        float scale = 1.0f;                                                        // Skalierung
        float forward = 0.0f;                                                      // Grundausrichtung in Grad
        Vec3 offset;                                                               // Verschiebung nach dem Skalieren
        std::map<std::string, Color> recolor;                                      // Materialfarben ersetzen
        Color tint = rgba(255, 255, 255);                                          // Einfärbung
        std::vector<std::string> parts;                                            // Teile (zusammengesetztes Modell)
        std::string type;                                                          // Sonderarten: "haus"
        int houseSize = 2;                                                         // Haus: Zellen je Seite (1 oder 2)
        int floors = 1;                                                            // Haus: Stockwerke
        std::string wallStyle = "stein";                                           // Haus: stein oder holz
        std::string roofStyle = "walm";                                            // Haus: walm, spitz oder giebel
    };                                                                             // Ende von Def
    struct Figure {                                                                // Eine animierte Figur
        std::string modelFile;                                                     // FBX mit Netz und Skin
        std::string skinFile;                                                      // Textur (PNG)
        std::map<std::string, std::string> animFiles;                              // Animationsname -> FBX-Datei
        std::map<std::string, std::string> animStacks;                             // Animationsname -> Name im FBX
        float height = 0.95f;                                                      // Gewünschte Größe in Kacheln
        int frames = 8;                                                            // Bilder pro Animation
        float forward = 90.0f;                                                     // Blickrichtung des Modells in Grad
        bool loaded = false;                                                       // Schon geladen?
        bool failed = false;                                                       // Laden fehlgeschlagen?
        FbxCharacter character;                                                    // Netz + Skin
        std::map<std::string, FbxAnimation> anims;                                 // Geladene Animationen
        float scale = 1.0f;                                                        // Berechnete Skalierung
    };                                                                             // Ende von Figure

    bool buildMesh(const std::string& name, Mesh& out, int depth);                // Modell zusammenbauen
    void buildHouse(const Def& def, Mesh& out);                                    // Haus aus Bauteilen erzeugen
    bool loadFigure(Figure& f);                                                    // Figur laden

    std::string m_assetDir;                                                        // Asset-Ordner
    std::map<std::string, Def> m_defs;                                             // Definitionen nach Namen
    std::map<std::string, std::unique_ptr<Mesh>> m_meshes;                         // Geladene Modelle
    std::map<std::string, Sprite> m_sprites;                                       // Gerenderte Sprites
    std::map<std::string, Figure> m_figures;                                       // Animierte Figuren
    std::map<std::string, Mesh> m_rawObj;                                          // Zwischenspeicher für OBJ-Dateien
    Sprite m_empty;                                                                // Leeres Sprite für Fehlerfälle
    int m_supersample = 2;                                                         // Kantenglättung
    int m_renderCount = 0;                                                         // Statistik
}; // Ende der Klasse ModelLibrary
