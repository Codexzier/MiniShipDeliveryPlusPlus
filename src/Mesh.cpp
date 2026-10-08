// Mesh.cpp - Laden von OBJ/MTL-Dateien (Wavefront-Format der Kenney-Pakete) und Zusammensetzen von Modellen
#include "Mesh.h" // Eigene Deklarationen

#include <cstdlib> // std::strtof, std::strtol
#include <fstream> // std::ifstream
#include <map>     // std::map für Materialnamen
#include <sstream> // std::istringstream

#include "ImageIO.h" // Texturen laden, Pfade

// Multipliziert zwei Farben kanalweise (z.B. Materialfarbe mit Tönung)
Color multiplyColor(Color a, Color b) {                                       // Beginn von multiplyColor
    return rgba(redOf(a) * redOf(b) / 255, greenOf(a) * greenOf(b) / 255,      // Rot und Grün multiplizieren
                blueOf(a) * blueOf(b) / 255, alphaOf(a) * alphaOf(b) / 255);  // Blau und Alpha multiplizieren
} // Ende von multiplyColor

// Berechnet den umgebenden Quader aller Eckpunkte
void Mesh::bounds(Vec3& minOut, Vec3& maxOut) const {                         // Beginn von bounds
    if (positions.empty()) { minOut = Vec3(); maxOut = Vec3(); return; }      // Leeres Modell
    minOut = maxOut = positions[0];                                           // Mit dem ersten Punkt beginnen
    for (const Vec3& p : positions) {                                         // Alle Punkte
        minOut.x = std::min(minOut.x, p.x); maxOut.x = std::max(maxOut.x, p.x); // x-Grenzen
        minOut.y = std::min(minOut.y, p.y); maxOut.y = std::max(maxOut.y, p.y); // y-Grenzen
        minOut.z = std::min(minOut.z, p.z); maxOut.z = std::max(maxOut.z, p.z); // z-Grenzen
    }                                                                         // Ende der Schleife
} // Ende von bounds

// Hängt ein anderes Modell an (transformiert und optional eingefärbt) - für zusammengesetzte Modelle
void Mesh::append(const Mesh& other, const Mat4& transform, Color tint) {     // Beginn von append
    int vOffset = static_cast<int>(positions.size());                         // Verschiebung der Punktindizes
    int uvOffset = static_cast<int>(uvs.size() / 2);                          // Verschiebung der UV-Indizes
    int mOffset = static_cast<int>(materials.size());                         // Verschiebung der Materialindizes
    for (const Vec3& p : other.positions) positions.push_back(transform.transformPoint(p)); // Punkte transformiert übernehmen
    uvs.insert(uvs.end(), other.uvs.begin(), other.uvs.end());                // Texturkoordinaten übernehmen
    for (Material m : other.materials) {                                      // Materialien kopieren
        m.color = multiplyColor(m.color, tint);                               // Tönung anwenden
        materials.push_back(m);                                               // Speichern
    }                                                                         // Ende der Schleife
    for (Triangle t : other.triangles) {                                      // Dreiecke kopieren
        for (int i = 0; i < 3; ++i) {                                         // Alle drei Ecken
            t.v[i] += vOffset;                                                // Punktindex verschieben
            if (t.uv[i] >= 0) t.uv[i] += uvOffset;                            // UV-Index verschieben
        }                                                                     // Ende der Eckenschleife
        t.material += mOffset;                                                // Materialindex verschieben
        triangles.push_back(t);                                               // Speichern
    }                                                                         // Ende der Schleife
} // Ende von append

namespace { // Interne Hilfsfunktionen

// Liest eine MTL-Datei und trägt die Materialien in eine Tabelle ein
void loadMtl(const std::string& path, std::map<std::string, Material>& table) { // Beginn von loadMtl
    std::ifstream file(path);                                                 // Datei öffnen
    if (!file) return;                                                        // Fehlt -> keine Materialien
    Material* current = nullptr;                                              // Aktuelles Material
    std::string line;                                                         // Zeilenpuffer
    while (std::getline(file, line)) {                                        // Zeile für Zeile
        std::istringstream in(line);                                          // Zeile zerlegen
        std::string key;                                                      // Schlüsselwort
        in >> key;                                                            // Erstes Wort lesen
        if (key == "newmtl") {                                                // Neues Material
            std::string name;                                                 // Materialname
            in >> name;                                                       // Namen lesen
            table[name].name = name;                                          // Material anlegen
            current = &table[name];                                           // Als aktuelles merken
        } else if (key == "Kd" && current) {                                  // Grundfarbe
            float r = 0.8f, g = 0.8f, b = 0.8f;                               // Standardwerte
            in >> r >> g >> b;                                                // Werte lesen (0..1)
            current->color = rgba(static_cast<int>(r * 255.0f + 0.5f), static_cast<int>(g * 255.0f + 0.5f), static_cast<int>(b * 255.0f + 0.5f)); // In 0..255 umrechnen
        } else if (key == "map_Kd" && current) {                              // Textur
            std::string texName;                                              // Dateiname der Textur
            std::getline(in, texName);                                        // Rest der Zeile lesen (kann Leerzeichen enthalten)
            while (!texName.empty() && (texName.front() == ' ' || texName.front() == '\t')) texName.erase(texName.begin()); // Führende Leerzeichen entfernen
            while (!texName.empty() && (texName.back() == ' ' || texName.back() == '\r')) texName.pop_back(); // Abschließende Leerzeichen entfernen
            auto tex = std::make_shared<Image>();                             // Neues Bild
            if (ImageIO::loadImage(ImageIO::joinPath(ImageIO::directoryOf(path), texName), *tex)) current->texture = tex; // Laden und merken
        }                                                                     // Ende der Unterscheidung
    }                                                                         // Ende der Zeilenschleife
} // Ende von loadMtl

// Wandelt einen OBJ-Index (1-basiert oder negativ = von hinten) in einen 0-basierten Index um
int resolveIndex(long index, int count) {                                     // Beginn von resolveIndex
    if (index > 0) return static_cast<int>(index - 1);                        // Positiv: 1-basiert
    if (index < 0) return count + static_cast<int>(index);                    // Negativ: relativ zum Ende
    return -1;                                                                // 0 ist ungültig
} // Ende von resolveIndex

} // Ende des internen Namensraums

// Lädt eine OBJ-Datei mit ihren Materialien
bool loadObj(const std::string& path, Mesh& out, std::string& error) {        // Beginn von loadObj
    std::ifstream file(path);                                                 // Datei öffnen
    if (!file) { error = "Datei nicht gefunden: " + path; return false; }     // Fehler melden
    out = Mesh();                                                             // Ergebnis leeren
    std::map<std::string, Material> mtlTable;                                 // Materialien aus der MTL-Datei
    std::map<std::string, int> materialIndex;                                 // Materialname -> Index im Mesh
    int currentMaterial = -1;                                                 // Aktuelles Material (noch keins)
    std::string line;                                                         // Zeilenpuffer
    std::vector<int> faceV;                                                   // Punktindizes einer Fläche
    std::vector<int> faceT;                                                   // UV-Indizes einer Fläche
    while (std::getline(file, line)) {                                        // Zeile für Zeile
        if (line.size() < 2) continue;                                        // Zu kurze Zeilen überspringen
        if (line[0] == 'v' && line[1] == ' ') {                               // Eckpunkt "v x y z"
            const char* p = line.c_str() + 2;                                 // Hinter "v " beginnen
            char* end = nullptr;                                              // Zeiger für strtof
            float x = std::strtof(p, &end); p = end;                          // x lesen
            float y = std::strtof(p, &end); p = end;                          // y lesen
            float z = std::strtof(p, &end);                                   // z lesen
            out.positions.push_back(Vec3(x, y, z));                           // Speichern
        } else if (line[0] == 'v' && line[1] == 't') {                        // Texturkoordinate "vt u v"
            const char* p = line.c_str() + 3;                                 // Hinter "vt " beginnen
            char* end = nullptr;                                              // Zeiger für strtof
            float u = std::strtof(p, &end); p = end;                          // u lesen
            float v = std::strtof(p, &end);                                   // v lesen
            out.uvs.push_back(u);                                             // u speichern
            out.uvs.push_back(v);                                             // v speichern
        } else if (line[0] == 'f' && line[1] == ' ') {                        // Fläche "f a/b/c ..."
            if (currentMaterial < 0) {                                        // Noch kein Material gesetzt
                materialIndex["_standard"] = static_cast<int>(out.materials.size()); // Standardmaterial anlegen
                out.materials.push_back(Material{"_standard", rgba(200, 200, 200), nullptr}); // Hellgrau
                currentMaterial = materialIndex["_standard"];                 // Benutzen
            }                                                                 // Ende der Prüfung
            faceV.clear();                                                    // Puffer leeren
            faceT.clear();                                                    // Puffer leeren
            std::istringstream in(line.substr(2));                            // Rest der Zeile zerlegen
            std::string token;                                                // Ein Eckpunkt der Fläche
            while (in >> token) {                                             // Alle Eckpunkte
                long vi = std::strtol(token.c_str(), nullptr, 10);            // Punktindex vor dem ersten '/'
                long ti = 0;                                                  // UV-Index (falls vorhanden)
                std::size_t slash = token.find('/');                          // Erster Schrägstrich
                if (slash != std::string::npos && slash + 1 < token.size() && token[slash + 1] != '/') ti = std::strtol(token.c_str() + slash + 1, nullptr, 10); // UV-Index lesen
                faceV.push_back(resolveIndex(vi, static_cast<int>(out.positions.size()))); // Punktindex umrechnen
                faceT.push_back(ti != 0 ? resolveIndex(ti, static_cast<int>(out.uvs.size() / 2)) : -1); // UV-Index umrechnen
            }                                                                 // Ende der Eckpunktschleife
            for (std::size_t k = 1; k + 1 < faceV.size(); ++k) {              // Vieleck als Dreiecksfächer zerlegen
                Triangle t;                                                   // Neues Dreieck
                t.v[0] = faceV[0]; t.v[1] = faceV[k]; t.v[2] = faceV[k + 1];  // Eckpunkte
                t.uv[0] = faceT[0]; t.uv[1] = faceT[k]; t.uv[2] = faceT[k + 1]; // Texturkoordinaten
                t.material = currentMaterial;                                 // Material
                if (t.v[0] >= 0 && t.v[1] >= 0 && t.v[2] >= 0) out.triangles.push_back(t); // Nur gültige Dreiecke speichern
            }                                                                 // Ende der Zerlegung
        } else if (line.compare(0, 6, "mtllib") == 0) {                       // Materialbibliothek
            std::string name = line.substr(7);                                // Dateiname
            while (!name.empty() && (name.back() == ' ' || name.back() == '\r')) name.pop_back(); // Leerzeichen am Ende entfernen
            loadMtl(ImageIO::joinPath(ImageIO::directoryOf(path), name), mtlTable); // MTL-Datei im gleichen Ordner laden
        } else if (line.compare(0, 6, "usemtl") == 0) {                       // Material wechseln
            std::string name = line.substr(7);                                // Materialname
            while (!name.empty() && (name.back() == ' ' || name.back() == '\r')) name.pop_back(); // Leerzeichen am Ende entfernen
            auto it = materialIndex.find(name);                               // Schon im Mesh?
            if (it == materialIndex.end()) {                                  // Noch nicht
                Material m = mtlTable.count(name) ? mtlTable[name] : Material{name, rgba(200, 200, 200), nullptr}; // Aus der Tabelle oder Standard
                materialIndex[name] = static_cast<int>(out.materials.size()); // Index merken
                out.materials.push_back(m);                                   // Material speichern
                it = materialIndex.find(name);                                // Neu suchen
            }                                                                 // Ende der Prüfung
            currentMaterial = it->second;                                     // Als aktuelles Material verwenden
        }                                                                     // Ende der Unterscheidung
    }                                                                         // Ende der Zeilenschleife
    if (out.triangles.empty()) { error = "Keine Flächen in " + path; return false; } // Leere Datei melden
    return true;                                                              // Erfolgreich geladen
} // Ende von loadObj
