// ModelLibrary.cpp - Modelle laden, zusammensetzen, Häuser bauen und Sprites zwischenspeichern
#include "ModelLibrary.h" // Eigene Deklarationen

#include <algorithm> // std::find
#include <cmath>    // std::fmod, std::round
#include <cstdio>   // std::snprintf
#include <iostream> // std::cout für Hinweise
#include <sstream>  // std::stringstream

#include "ImageIO.h" // Pfade und Bilder

namespace { // Interne Hilfen

constexpr float DEG = 3.14159265358979f / 180.0f;                        // Grad -> Radiant
const std::string HOUSE_PARTS = "Fantasy Town Kit/Models/OBJ format/";   // Ordner der Hausbauteile

// Liest "r, g, b" als Farbe
Color parseColor(const std::string& text) {                              // Beginn von parseColor
    int r = 255, g = 255, b = 255;                                        // Standard: weiß
    std::sscanf(text.c_str(), "%d , %d , %d", &r, &g, &b);                // Drei Zahlen lesen
    return rgba(r, g, b);                                                 // Farbe zurückgeben
} // Ende von parseColor

// Liest "x, y, z" als Vektor
Vec3 parseVec(const std::string& text) {                                  // Beginn von parseVec
    Vec3 v;                                                               // Ergebnis
    std::string t = text;                                                 // Kopie
    for (char& c : t) if (c == ',') c = ' ';                              // Kommas durch Leerzeichen ersetzen
    std::stringstream in(t);                                              // Zum Lesen
    in >> v.x >> v.y >> v.z;                                              // Drei Zahlen lesen
    return v;                                                             // Ergebnis
} // Ende von parseVec

// Zerlegt einen Text an einem Trennzeichen und entfernt Leerzeichen
std::vector<std::string> split(const std::string& text, char sep) {       // Beginn von split
    std::vector<std::string> out;                                         // Ergebnis
    std::stringstream in(text);                                           // Zum Lesen
    std::string item;                                                     // Ein Teil
    while (std::getline(in, item, sep)) out.push_back(PropertyFile::trim(item)); // Teil speichern
    return out;                                                           // Ergebnis
} // Ende von split

} // Ende des internen Namensraums

// Liest die Definitionen aller Modelle und Figuren
bool ModelLibrary::load(const std::string& modelFile, const std::string& figureFile, const std::string& assetDir) { // Beginn von load
    m_assetDir = assetDir;                                                // Asset-Ordner merken
    PropertyFile file;                                                    // Datei-Objekt
    if (!file.load(modelFile)) return false;                              // Datei fehlt
    for (const std::string& name : file.sectionNames()) {                 // Alle Modelle
        Def d;                                                            // Neue Definition
        d.file = file.getString(name, "datei", "");                       // OBJ-Datei
        d.scale = file.getFloat(name, "skalierung", 1.0f);                // Skalierung
        d.forward = file.getFloat(name, "vorne", 0.0f);                   // Grundausrichtung
        d.offset = parseVec(file.getString(name, "versatz", "0,0,0"));    // Verschiebung
        d.tint = parseColor(file.getString(name, "toenung", "255,255,255")); // Einfärbung
        d.type = file.getString(name, "typ", "");                         // Sonderart
        d.onlyGroups = file.getList(name, "gruppen");                     // Nur bestimmte Gruppen
        d.skipGroups = file.getList(name, "ohne_gruppen");                // Gruppen weglassen
        d.houseSize = clampValue(file.getInt(name, "haus_groesse", 2), 1, 2); // Hausgröße
        d.floors = clampValue(file.getInt(name, "stockwerke", 1), 1, 3);  // Stockwerke
        d.wallStyle = file.getString(name, "wand", "stein");              // Wandart
        d.roofStyle = file.getString(name, "dach", "walm");               // Dachart
        for (int i = 1; i <= 40; ++i) {                                   // Teile teil_1 .. teil_40
            std::string part = file.getString(name, "teil_" + std::to_string(i), ""); // Teil lesen
            if (!part.empty()) d.parts.push_back(part);                   // Speichern
        }                                                                 // Ende der Teileschleife
        for (const std::string& key : file.keys(name)) {                  // Alle Eigenschaften des Modells
            if (key.rfind("farbe_", 0) != 0) continue;                    // Nur "farbe_<material>"
            std::string material = key.substr(6);                         // Materialname
            for (const std::string& orig : {std::string("roofLight"), std::string("woodDark"), std::string("_defaultMat")}) { // Gemischte Schreibweisen
                if (PropertyFile::toLower(orig) == material) material = orig; // Originalschreibweise wiederherstellen
            }                                                             // Ende der Schleife
            d.recolor[material] = parseColor(file.getString(name, key, "255,255,255")); // Ersatzfarbe merken
        }                                                                 // Ende der Materialschleife
        m_defs[name] = d;                                                 // Speichern
    }                                                                     // Ende der Modellschleife
    PropertyFile figs;                                                    // Figurendatei
    if (figs.load(figureFile)) {                                          // Laden (optional)
        for (const std::string& name : figs.sectionNames()) {             // Alle Figuren
            Figure f;                                                     // Neue Figur
            f.modelFile = figs.getString(name, "modell", "");             // FBX mit Netz
            f.skinFile = figs.getString(name, "skin", "");                // Textur
            f.height = figs.getFloat(name, "hoehe", 0.95f);               // Größe
            f.frames = clampValue(figs.getInt(name, "bilder", 8), 1, 32); // Bilder je Animation
            f.forward = figs.getFloat(name, "vorne", 90.0f);              // Blickrichtung
            for (const char* anim : {"stehen", "laufen", "springen"}) { // Bekannte Animationen
                std::vector<std::string> parts = split(figs.getString(name, std::string("anim_") + anim, ""), '|'); // "datei | name"
                if (parts.empty() || parts[0].empty()) continue;          // Nicht angegeben
                f.animFiles[anim] = parts[0];                             // Datei
                f.animStacks[anim] = parts.size() > 1 ? parts[1] : "";    // Name der Animation in der Datei
            }                                                             // Ende der Animationsschleife
            m_figures[name] = f;                                          // Speichern
        }                                                                 // Ende der Figurenschleife
    }                                                                     // Ende Figuren
    m_empty.image.resize(1, 1);                                           // Leeres 1x1-Sprite
    return true;                                                          // Erfolgreich
} // Ende von load

bool ModelLibrary::has(const std::string& name) const { return m_defs.count(name) > 0; } // Modell bekannt?

// Liefert alle Modellnamen
std::vector<std::string> ModelLibrary::names() const {                    // Beginn von names
    std::vector<std::string> result;                                      // Ergebnis
    for (const auto& entry : m_defs) result.push_back(entry.first);       // Namen sammeln
    return result;                                                        // Ergebnis
} // Ende von names

// Erzeugt ein Haus aus Fantasy-Town-Bauteilen (Wände je Zellkante, Dach oben)
void ModelLibrary::buildHouse(const Def& def, Mesh& out) {               // Beginn von buildHouse
    auto piece = [&](const std::string& file) -> const Mesh* {            // Bauteil laden (mit Zwischenspeicher)
        auto it = m_rawObj.find(file);                                    // Schon geladen?
        if (it != m_rawObj.end()) return &it->second;                     // Ja -> zurückgeben
        Mesh m; std::string err;                                          // Neues Modell
        if (!loadObj(ImageIO::joinPath(m_assetDir, HOUSE_PARTS + file + ".obj"), m, err)) { std::cout << err << "\n"; return nullptr; } // Laden
        return &(m_rawObj[file] = m);                                     // Speichern und zurückgeben
    };                                                                    // Ende der Hilfsfunktion
    bool wood = def.wallStyle == "holz";                                  // Holzwände?
    std::string plain = wood ? "wallWood" : "wall";                       // Glatte Wand
    std::string window = wood ? "wallWoodWindowShutters" : "wallWindowShutters"; // Wand mit Fenster
    std::string door = wood ? "wallWoodDoor" : "wallDoor";                // Wand mit Tür
    int n = def.houseSize;                                                // Zellen je Seite
    float half = static_cast<float>(n - 1) * 0.5f;                        // Abstand der Zellmitten zur Hausmitte
    const float edgeAngle[4] = {0.0f, 90.0f, 180.0f, 270.0f};             // Drehung für Kante +x, -z, -x, +z
    const int edgeDx[4] = {1, 0, -1, 0};                                  // Richtung der Kante in x
    const int edgeDz[4] = {0, -1, 0, 1};                                  // Richtung der Kante in z
    int wallCounter = 0;                                                  // Zähler für abwechselnde Fenster
    for (int floor = 0; floor < def.floors; ++floor) {                    // Alle Stockwerke
        for (int i = 0; i < n; ++i) {                                     // Zellen in x
            for (int k = 0; k < n; ++k) {                                 // Zellen in z
                for (int e = 0; e < 4; ++e) {                             // Alle vier Kanten
                    int ni = i + edgeDx[e];                               // Nachbarzelle x
                    int nk = k + edgeDz[e];                               // Nachbarzelle z
                    if (ni >= 0 && ni < n && nk >= 0 && nk < n) continue; // Innenkante -> keine Wand
                    bool isDoor = floor == 0 && e == 3 && i == 0;         // Tür vorne (+z) in der ersten Zelle
                    std::string file = isDoor ? door : ((wallCounter++ % 2 == 0) ? window : plain); // Bauteil wählen
                    const Mesh* m = piece(file);                          // Bauteil laden
                    if (!m) continue;                                     // Fehlt -> überspringen
                    Mat4 t = Mat4::translation(Vec3(static_cast<float>(i) - half, static_cast<float>(floor), static_cast<float>(k) - half)) * Mat4::rotationY(edgeAngle[e] * DEG); // Position und Drehung
                    out.append(*m, t);                                    // Anhängen
                }                                                         // Ende der Kantenschleife
            }                                                             // Ende z
        }                                                                 // Ende x
    }                                                                     // Ende der Stockwerke
    float roofY = static_cast<float>(def.floors);                         // Höhe des Dachs
    if (n == 1) {                                                         // Kleines Haus
        const Mesh* m = piece(def.roofStyle == "giebel" ? "roofGable" : "roofPoint"); // Spitzdach oder Giebeldach
        if (m) out.append(*m, Mat4::translation(Vec3(0, roofY, 0)));      // Dach aufsetzen
    } else {                                                              // Großes Haus (2x2)
        const Mesh* corner = piece("roofCorner");                         // Eckteil des Walmdachs
        const float cornerAngle[2][2] = {{0.0f, 90.0f}, {270.0f, 180.0f}}; // Drehung je Zelle [i][k], damit die Ecken nach außen zeigen
        for (int i = 0; i < 2 && corner; ++i) {                           // Zellen in x
            for (int k = 0; k < 2; ++k) {                                 // Zellen in z
                Mat4 t = Mat4::translation(Vec3(static_cast<float>(i) - half, roofY, static_cast<float>(k) - half)) * Mat4::rotationY(cornerAngle[i][k] * DEG); // Position und Drehung
                out.append(*corner, t);                                   // Anhängen
            }                                                             // Ende z
        }                                                                 // Ende x
    }                                                                     // Ende der Unterscheidung
} // Ende von buildHouse

// Erzeugt ein Seeungeheuer: mehrere Rückenbögen (Röhren) und ein Kopf mit leuchtenden Augen
void ModelLibrary::buildSerpent(Mesh& out) {                             // Beginn von buildSerpent
    out.materials.push_back(Material{"koerper", rgba(46, 112, 78), nullptr}); // Material 0: grüner Körper
    out.materials.push_back(Material{"bauch", rgba(150, 190, 120), nullptr}); // Material 1: heller Bauch
    out.materials.push_back(Material{"auge", rgba(255, 225, 60), nullptr});   // Material 2: gelbe Augen
    auto tube = [&](const std::vector<Vec3>& centers, float radius, int material) { // Hilfsfunktion: Röhre entlang einer Linie
        const int around = 8;                                             // Punkte je Ring
        int base = static_cast<int>(out.positions.size());                // Erster neuer Punktindex
        for (std::size_t i = 0; i < centers.size(); ++i) {                // Alle Ringe
            Vec3 t = centers[std::min(i + 1, centers.size() - 1)] - centers[i > 0 ? i - 1 : 0]; // Richtung der Linie
            t = normalize(t);                                             // Normieren
            Vec3 n1(0, 0, 1);                                             // Erste Querachse (seitlich)
            Vec3 n2 = normalize(cross(t, n1));                            // Zweite Querachse
            float taper = 1.0f - 0.25f * std::fabs(static_cast<float>(i) / static_cast<float>(centers.size() - 1) * 2.0f - 1.0f); // Enden etwas dünner
            for (int k = 0; k < around; ++k) {                            // Punkte im Ring
                float a = static_cast<float>(k) / around * 6.2831853f;    // Winkel
                out.positions.push_back(centers[i] + (n2 * std::cos(a) + n1 * std::sin(a)) * (radius * taper)); // Ringpunkt
            }                                                             // Ende des Rings
        }                                                                 // Ende der Ringe
        for (std::size_t i = 0; i + 1 < centers.size(); ++i) {            // Flächen zwischen den Ringen
            for (int k = 0; k < around; ++k) {                            // Alle Segmente
                int a0 = base + static_cast<int>(i) * around + k;         // Punkt Ring i
                int a1 = base + static_cast<int>(i) * around + (k + 1) % around; // Nachbar Ring i
                int b0 = a0 + around, b1 = a1 + around;                   // Punkte Ring i+1
                int mat = (k == around / 2 || k == around / 2 - 1) ? 1 : material; // Unterseite heller
                Triangle t1; t1.v[0] = a0; t1.v[1] = b0; t1.v[2] = b1; t1.material = mat; out.triangles.push_back(t1); // Dreieck 1
                Triangle t2; t2.v[0] = a0; t2.v[1] = b1; t2.v[2] = a1; t2.material = mat; out.triangles.push_back(t2); // Dreieck 2
            }                                                             // Ende der Segmente
        }                                                                 // Ende der Flächen
    };                                                                    // Ende der Hilfsfunktion
    for (int h = 0; h < 3; ++h) {                                         // Drei Rückenbögen
        std::vector<Vec3> arc;                                            // Mittellinie des Bogens
        float cx = 0.6f + static_cast<float>(h) * 1.15f;                  // Mitte des Bogens
        float r = 0.5f - 0.08f * static_cast<float>(h);                   // Bogen wird nach hinten kleiner
        for (int i = 0; i <= 10; ++i) {                                   // Punkte des Halbkreises
            float a = 3.14159265f * static_cast<float>(i) / 10.0f;        // Winkel 0..Pi
            arc.push_back(Vec3(cx - r * std::cos(a), r * std::sin(a) - 0.1f, 0)); // Punkt
        }                                                                 // Ende des Halbkreises
        tube(arc, 0.2f - 0.03f * static_cast<float>(h), 0);               // Röhre erzeugen
    }                                                                     // Ende der Bögen
    std::vector<Vec3> neck;                                               // Hals mit Kopf
    for (int i = 0; i <= 10; ++i) {                                       // Punkte des Halses
        float t = static_cast<float>(i) / 10.0f;                          // Anteil
        neck.push_back(Vec3(-0.1f - 0.5f * t, -0.1f + 1.3f * std::sin(t * 1.6f), 0)); // Nach vorne und oben gebogen
    }                                                                     // Ende des Halses
    tube(neck, 0.24f, 0);                                                 // Halsröhre
    std::vector<Vec3> head = {Vec3(-0.55f, 1.15f, 0), Vec3(-0.8f, 1.2f, 0), Vec3(-1.05f, 1.15f, 0), Vec3(-1.2f, 1.08f, 0)}; // Kopf nach vorne
    tube(head, 0.3f, 0);                                                  // Kopfröhre
    for (int side = -1; side <= 1; side += 2) {                           // Zwei Augen
        Vec3 c(-0.85f, 1.32f, 0.2f * static_cast<float>(side));           // Augenmitte
        int b = static_cast<int>(out.positions.size());                   // Erster Punkt
        const float s = 0.07f;                                            // Halbe Kantenlänge
        for (int k = 0; k < 8; ++k) out.positions.push_back(c + Vec3((k & 1) ? s : -s, (k & 2) ? s : -s, (k & 4) ? s : -s)); // Würfelecken
        const int f[12][3] = {{0, 1, 3}, {0, 3, 2}, {4, 6, 7}, {4, 7, 5}, {0, 4, 5}, {0, 5, 1}, {2, 3, 7}, {2, 7, 6}, {0, 2, 6}, {0, 6, 4}, {1, 5, 7}, {1, 7, 3}}; // Würfelflächen
        for (const auto& tri : f) { Triangle t; t.v[0] = b + tri[0]; t.v[1] = b + tri[1]; t.v[2] = b + tri[2]; t.material = 2; out.triangles.push_back(t); } // Dreiecke speichern
    }                                                                     // Ende der Augen
} // Ende von buildSerpent

// Baut ein Modell aus Datei, Teilen und Sondertypen zusammen
bool ModelLibrary::buildMesh(const std::string& name, Mesh& out, int depth) { // Beginn von buildMesh
    auto it = m_defs.find(name);                                          // Definition suchen
    if (it == m_defs.end() || depth > 8) return false;                    // Unbekannt oder zu tief verschachtelt
    const Def& d = it->second;                                            // Definition
    Mesh base;                                                            // Unskaliertes Modell
    if (!d.file.empty()) {                                                // Aus Datei
        auto raw = m_rawObj.find(d.file);                                 // Schon geladen?
        if (raw == m_rawObj.end()) {                                      // Nein
            Mesh m; std::string err;                                      // Neues Modell
            if (!loadObj(ImageIO::joinPath(m_assetDir, d.file), m, err)) { std::cout << "Modell '" << name << "': " << err << "\n"; return false; } // Laden
            raw = m_rawObj.emplace(d.file, std::move(m)).first;           // Speichern
        }                                                                 // Ende Laden
        if (d.onlyGroups.empty() && d.skipGroups.empty()) {               // Ganze Datei verwenden
            base.append(raw->second, Mat4());                             // Übernehmen
        } else {                                                          // Nur ausgewählte Gruppen
            Mesh filtered = raw->second;                                  // Kopie
            filtered.triangles.clear();                                   // Dreiecke neu auswählen
            for (const Triangle& t : raw->second.triangles) {             // Alle Dreiecke
                const std::string& g = raw->second.groups[static_cast<std::size_t>(t.group)]; // Gruppenname
                bool inOnly = d.onlyGroups.empty() || std::find(d.onlyGroups.begin(), d.onlyGroups.end(), g) != d.onlyGroups.end(); // Gewünscht?
                bool inSkip = std::find(d.skipGroups.begin(), d.skipGroups.end(), g) != d.skipGroups.end(); // Ausgeschlossen?
                if (inOnly && !inSkip) filtered.triangles.push_back(t);   // Übernehmen
            }                                                             // Ende der Schleife
            base.append(filtered, Mat4());                                // Gefiltertes Modell übernehmen
        }                                                                 // Ende der Gruppenauswahl
    }                                                                     // Ende Datei
    if (d.type == "haus") buildHouse(d, base);                            // Haus generieren
    if (d.type == "seeschlange") buildSerpent(base);                      // Seeungeheuer generieren
    for (const std::string& partText : d.parts) {                         // Alle Teile
        std::vector<std::string> p = split(partText, '|');                // "name | x,y,z | drehung | skalierung"
        Mesh part;                                                        // Teilmodell
        if (p.empty() || !buildMesh(p[0], part, depth + 1)) continue;     // Teil bauen
        Vec3 pos = p.size() > 1 ? parseVec(p[1]) : Vec3();                // Position
        Vec3 rot = p.size() > 2 ? (p[2].find(',') != std::string::npos ? parseVec(p[2]) : Vec3(0.0f, std::strtof(p[2].c_str(), nullptr), 0.0f)) : Vec3(); // Drehung (nur y oder x, y, z)
        float sc = p.size() > 3 ? std::strtof(p[3].c_str(), nullptr) : 1.0f; // Skalierung
        if (sc <= 0.0f) sc = 1.0f;                                        // Ungültige Skalierung abfangen
        base.append(part, Mat4::translation(pos) * eulerXYZ(rot) * Mat4::scale(Vec3(sc, sc, sc))); // Teil anhängen
    }                                                                     // Ende der Teileschleife
    for (Material& m : base.materials) {                                  // Farben anpassen
        auto rc = d.recolor.find(m.name);                                 // Ersatzfarbe für dieses Material?
        if (rc != d.recolor.end()) m.color = rc->second;                  // Ersetzen
        m.color = multiplyColor(m.color, d.tint);                         // Einfärben
    }                                                                     // Ende der Materialschleife
    out.append(base, Mat4::translation(d.offset) * Mat4::scale(Vec3(d.scale, d.scale, d.scale))); // Skalieren und verschieben
    return !out.empty();                                                  // Erfolgreich, wenn etwas entstanden ist
} // Ende von buildMesh

// Liefert ein fertiges Modell (lädt es beim ersten Zugriff)
const Mesh* ModelLibrary::mesh(const std::string& name) {                 // Beginn von mesh
    auto it = m_meshes.find(name);                                        // Schon gebaut?
    if (it != m_meshes.end()) return it->second.get();                    // Ja -> zurückgeben (kann nullptr sein)
    auto m = std::make_unique<Mesh>();                                    // Neues Modell
    if (!buildMesh(name, *m, 0)) {                                        // Bauen
        std::cout << "Hinweis: Modell '" << name << "' konnte nicht erzeugt werden.\n"; // Hinweis
        m_meshes[name] = nullptr;                                         // Fehlschlag merken
        return nullptr;                                                   // Kein Modell
    }                                                                     // Ende Fehlerfall
    const Mesh* result = m.get();                                         // Zeiger merken
    m_meshes[name] = std::move(m);                                        // Speichern
    return result;                                                        // Zurückgeben
} // Ende von mesh

// Liefert ein Sprite eines Modells in einer Drehung (Weltwinkel in Radiant, 0 = Richtung +x)
const Sprite& ModelLibrary::sprite(const std::string& name, float angle, int steps, float tileWidth) { // Beginn von sprite
    steps = std::max(1, steps);                                           // Mindestens eine Drehung
    float step = 2.0f * 3.14159265f / static_cast<float>(steps);          // Winkel je Stufe
    int index = static_cast<int>(std::lround(angle / step)) % steps;      // Nächste Stufe
    if (index < 0) index += steps;                                        // Negativ -> umklappen
    char key[256];                                                        // Schlüssel für den Zwischenspeicher
    std::snprintf(key, sizeof(key), "%s|%d|%d|%d", name.c_str(), index, steps, static_cast<int>(tileWidth)); // Name|Stufe|Stufen|Kachelbreite
    auto it = m_sprites.find(key);                                        // Schon gerendert?
    if (it != m_sprites.end()) return it->second;                         // Ja -> zurückgeben
    const Mesh* m = mesh(name);                                           // Modell holen
    if (!m) return m_empty;                                               // Fehlt -> leeres Sprite
    RenderParams rp;                                                      // Renderparameter
    rp.tileWidth = tileWidth;                                             // Maßstab
    rp.supersample = m_supersample;                                       // Kantenglättung
    rp.rotation = static_cast<float>(index) * step - m_defs[name].forward * DEG; // Drehung: Zielwinkel minus Grundausrichtung
    ++m_renderCount;                                                      // Statistik
    return m_sprites[key] = renderMesh(*m, nullptr, Mat4(), rp);          // Rendern und speichern
} // Ende von sprite

// Lädt Netz, Skin und Animationen einer Figur
bool ModelLibrary::loadFigure(Figure& f) {                                // Beginn von loadFigure
    if (f.loaded) return !f.failed;                                       // Schon versucht
    f.loaded = true;                                                      // Als versucht markieren
    std::string err;                                                      // Fehlertext
    if (!loadFbxCharacter(ImageIO::joinPath(m_assetDir, f.modelFile), f.character, err)) { std::cout << err << "\n"; f.failed = true; return false; } // Netz laden
    auto tex = std::make_shared<Image>();                                 // Skin-Textur
    if (ImageIO::loadImage(ImageIO::joinPath(m_assetDir, f.skinFile), *tex)) f.character.mesh.materials[0].texture = tex; // Textur zuweisen
    for (const auto& entry : f.animFiles) {                               // Alle Animationen
        FbxAnimation anim;                                                // Neue Animation
        if (loadFbxAnimation(ImageIO::joinPath(m_assetDir, entry.second), f.animStacks[entry.first], anim, err)) f.anims[entry.first] = anim; // Laden
        else std::cout << err << "\n";                                    // Fehler ausgeben
    }                                                                     // Ende der Schleife
    if (f.anims.empty()) { f.failed = true; return false; }               // Ohne Animation unbrauchbar
    const FbxAnimation& first = f.anims.begin()->second;                  // Erste Animation
    std::vector<Vec3> pos = f.character.skin(first.pose(0.0));            // Pose zum Ausmessen
    float minY = 1e9f, maxY = -1e9f;                                      // Höhenbereich
    for (const Vec3& p : pos) { minY = std::min(minY, p.y); maxY = std::max(maxY, p.y); } // Ausmessen
    f.scale = (maxY - minY) > 1e-6f ? f.height / (maxY - minY) : 1.0f;    // Skalierung auf die gewünschte Größe
    return true;                                                          // Erfolgreich
} // Ende von loadFigure

// Länge einer Figurenanimation in Sekunden
float ModelLibrary::figureDuration(const std::string& name, const std::string& anim) { // Beginn von figureDuration
    auto it = m_figures.find(name);                                       // Figur suchen
    if (it == m_figures.end() || !loadFigure(it->second)) return 1.0f;    // Unbekannt -> 1 Sekunde
    auto a = it->second.anims.find(anim);                                 // Animation suchen
    return a == it->second.anims.end() ? 1.0f : static_cast<float>(a->second.duration); // Länge
} // Ende von figureDuration

// Liefert ein Bild einer animierten Figur (Richtung 0..7 = 0, 45, 90 ... Grad in der Welt)
const Sprite& ModelLibrary::figure(const std::string& name, const std::string& anim, int direction, float time, float tileWidth) { // Beginn von figure
    auto it = m_figures.find(name);                                       // Figur suchen
    if (it == m_figures.end() || !loadFigure(it->second)) return m_empty; // Unbekannt oder fehlerhaft
    Figure& f = it->second;                                               // Referenz
    auto a = f.anims.find(anim);                                          // Animation suchen
    if (a == f.anims.end()) a = f.anims.begin();                          // Ersatz: erste Animation
    double duration = std::max(0.01, a->second.duration);                 // Länge
    int frame = static_cast<int>(std::fmod(static_cast<double>(time), duration) / duration * f.frames) % f.frames; // Aktuelles Bild
    direction = ((direction % 8) + 8) % 8;                                // Richtung 0..7
    char key[256];                                                        // Schlüssel
    std::snprintf(key, sizeof(key), "#fig|%s|%s|%d|%d|%d", name.c_str(), a->first.c_str(), direction, frame, static_cast<int>(tileWidth)); // Figur|Anim|Richtung|Bild|Maßstab
    auto s = m_sprites.find(key);                                         // Schon gerendert?
    if (s != m_sprites.end()) return s->second;                           // Ja -> zurückgeben
    std::vector<Vec3> pos = f.character.skin(a->second.pose(duration * frame / f.frames)); // Pose berechnen
    RenderParams rp;                                                      // Renderparameter
    rp.tileWidth = tileWidth;                                             // Maßstab
    rp.supersample = m_supersample;                                       // Kantenglättung
    rp.rotation = (static_cast<float>(direction) * 45.0f - f.forward) * DEG; // Drehung in die gewünschte Richtung
    ++m_renderCount;                                                      // Statistik
    return m_sprites[key] = renderMesh(f.character.mesh, &pos, Mat4::scale(Vec3(f.scale, f.scale, f.scale)), rp); // Rendern und speichern
} // Ende von figure
