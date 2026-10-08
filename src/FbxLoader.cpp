// FbxLoader.cpp - Binäres FBX lesen (Version 7.x), Skin und Animationen auswerten
#include "FbxLoader.h" // Eigene Deklarationen

#include <cstring>  // std::memcpy, std::strlen
#include <fstream>  // std::ifstream
#include <functional> // std::function für die rekursive Posenberechnung
#include <zlib.h>   // uncompress() für komprimierte Listen

namespace { // Interne Hilfen

constexpr double FBX_TICKS_PER_SECOND = 46186158000.0; // FBX-Zeiteinheiten pro Sekunde

// Liest Zahlen beliebigen Typs aus dem Bytepuffer (Little Endian)
template <typename T>                                                     // Zieltyp
T readValue(const std::vector<unsigned char>& d, std::size_t& pos) {      // Beginn von readValue
    T value{};                                                            // Ergebnis
    if (pos + sizeof(T) > d.size()) { pos = d.size(); return value; }     // Dateiende -> 0 zurückgeben
    std::memcpy(&value, &d[pos], sizeof(T));                              // Bytes kopieren
    pos += sizeof(T);                                                     // Lesezeiger weiterschieben
    return value;                                                         // Wert zurückgeben
} // Ende von readValue

// Liest eine Liste (z.B. Eckpunkte), die optional mit zlib komprimiert ist
template <typename T>                                                     // Elementtyp in der Datei
bool readArray(const std::vector<unsigned char>& d, std::size_t& pos, std::vector<T>& out) { // Beginn von readArray
    std::uint32_t count = readValue<std::uint32_t>(d, pos);               // Anzahl der Elemente
    std::uint32_t encoding = readValue<std::uint32_t>(d, pos);            // 0 = roh, 1 = zlib
    std::uint32_t compressedLen = readValue<std::uint32_t>(d, pos);       // Länge in der Datei
    if (pos + compressedLen > d.size()) return false;                     // Datei zu kurz
    out.resize(count);                                                    // Platz schaffen
    uLongf rawLen = static_cast<uLongf>(count * sizeof(T));               // Erwartete Rohlänge
    if (encoding == 1) {                                                  // Komprimiert
        if (count > 0 && uncompress(reinterpret_cast<Bytef*>(out.data()), &rawLen, &d[pos], compressedLen) != Z_OK) return false; // Entpacken
    } else if (count > 0) {                                               // Unkomprimiert
        std::memcpy(out.data(), &d[pos], std::min<std::size_t>(compressedLen, count * sizeof(T))); // Direkt kopieren
    }                                                                     // Ende der Unterscheidung
    pos += compressedLen;                                                 // Hinter die Daten springen
    return true;                                                          // Erfolgreich
} // Ende von readArray

// Liest einen Knoten rekursiv. Gibt false zurück, wenn ein Null-Knoten (Listenende) gelesen wurde.
bool readNode(const std::vector<unsigned char>& d, std::size_t& pos, bool wide, FbxNode& node, bool& ok) { // Beginn von readNode
    std::uint64_t end = wide ? readValue<std::uint64_t>(d, pos) : readValue<std::uint32_t>(d, pos); // Endposition des Knotens
    std::uint64_t numProps = wide ? readValue<std::uint64_t>(d, pos) : readValue<std::uint32_t>(d, pos); // Anzahl der Eigenschaften
    std::uint64_t propLen = wide ? readValue<std::uint64_t>(d, pos) : readValue<std::uint32_t>(d, pos); // Länge der Eigenschaftsliste
    std::uint8_t nameLen = readValue<std::uint8_t>(d, pos);               // Länge des Namens
    if (end == 0) return false;                                           // Null-Knoten: Ende der Liste
    if (pos + nameLen > d.size() || end > d.size()) { ok = false; return false; } // Beschädigte Datei
    node.name.assign(reinterpret_cast<const char*>(&d[pos]), nameLen);    // Namen lesen
    pos += nameLen;                                                       // Hinter den Namen springen
    std::size_t propStart = pos;                                          // Beginn der Eigenschaften
    for (std::uint64_t i = 0; i < numProps && ok; ++i) {                  // Alle Eigenschaften
        FbxProperty p;                                                    // Neue Eigenschaft
        p.type = static_cast<char>(readValue<std::uint8_t>(d, pos));      // Typkennung
        switch (p.type) {                                                 // Je nach Typ
        case 'Y': p.integer = readValue<std::int16_t>(d, pos); break;     // 16-Bit-Ganzzahl
        case 'C': p.integer = readValue<std::uint8_t>(d, pos); break;     // Wahrheitswert
        case 'I': p.integer = readValue<std::int32_t>(d, pos); break;     // 32-Bit-Ganzzahl
        case 'L': p.integer = readValue<std::int64_t>(d, pos); break;     // 64-Bit-Ganzzahl
        case 'F': p.number = readValue<float>(d, pos); break;             // Kommazahl einfach
        case 'D': p.number = readValue<double>(d, pos); break;            // Kommazahl doppelt
        case 'f': { std::vector<float> a; ok = readArray(d, pos, a); p.numbers.assign(a.begin(), a.end()); break; } // Liste float
        case 'd': { ok = readArray(d, pos, p.numbers); break; }           // Liste double
        case 'i': { std::vector<std::int32_t> a; ok = readArray(d, pos, a); p.integers.assign(a.begin(), a.end()); break; } // Liste int32
        case 'l': { ok = readArray(d, pos, p.integers); break; }          // Liste int64
        case 'b': { std::vector<std::uint8_t> a; ok = readArray(d, pos, a); p.integers.assign(a.begin(), a.end()); break; } // Liste bool
        case 'S': case 'R': {                                             // Text oder Rohdaten
            std::uint32_t len = readValue<std::uint32_t>(d, pos);         // Länge
            if (pos + len > d.size()) { ok = false; break; }              // Datei zu kurz
            if (p.type == 'S') {                                          // Nur Texte übernehmen
                p.text.assign(reinterpret_cast<const char*>(&d[pos]), len); // Text kopieren
                std::size_t zero = p.text.find('\0');                     // Trennzeichen "\0\1" zwischen Name und Klasse
                if (zero != std::string::npos) p.text = p.text.substr(0, zero); // Nur den Namen behalten
            }                                                             // Ende Text
            pos += len;                                                   // Hinter die Daten springen
            break;                                                        // Ende
        }                                                                 // Ende case S/R
        default: ok = false; break;                                       // Unbekannter Typ -> Fehler
        }                                                                 // Ende der Fallunterscheidung
        node.props.push_back(p);                                          // Eigenschaft speichern
    }                                                                     // Ende der Eigenschaftsschleife
    pos = propStart + propLen;                                            // Sicherheitshalber exakt hinter die Eigenschaften springen
    while (ok && pos < end) {                                             // Unterknoten bis zum Knotenende
        FbxNode childNode;                                                // Neuer Unterknoten
        if (!readNode(d, pos, wide, childNode, ok)) break;                // Null-Knoten beendet die Liste
        node.children.push_back(std::move(childNode));                    // Unterknoten speichern
    }                                                                     // Ende der Unterknotenschleife
    pos = end;                                                            // Hinter den Knoten springen
    return true;                                                          // Knoten gelesen
} // Ende von readNode

// Liest die Eigenschaften eines "Properties70"-Blocks als Vektoren (z.B. "Lcl Rotation")
std::map<std::string, Vec3> readProperties70(const FbxNode& node) {       // Beginn von readProperties70
    std::map<std::string, Vec3> result;                                   // Ergebnis
    const FbxNode* p70 = node.child("Properties70");                      // Block suchen
    if (!p70) return result;                                              // Nicht vorhanden
    for (const FbxNode& p : p70->children) {                              // Alle Einträge "P"
        if (p.props.size() < 5 || p.props[0].text.empty()) continue;      // Zu kurz oder ohne Namen
        Vec3 v;                                                           // Wert
        v.x = static_cast<float>(p.props[4].type == 'L' || p.props[4].type == 'I' ? static_cast<double>(p.props[4].integer) : p.props[4].number); // Erster Wert
        if (p.props.size() > 5) v.y = static_cast<float>(p.props[5].number); // Zweiter Wert
        if (p.props.size() > 6) v.z = static_cast<float>(p.props[6].number); // Dritter Wert
        result[p.props[0].text] = v;                                      // Unter dem Namen speichern
    }                                                                     // Ende der Schleife
    return result;                                                        // Ergebnis zurückgeben
} // Ende von readProperties70

// Liest den "LocalStop"-Wert (Länge der Animation) als ganze Zahl
std::int64_t readLocalStop(const FbxNode& stack) {                        // Beginn von readLocalStop
    const FbxNode* p70 = stack.child("Properties70");                     // Block suchen
    if (!p70) return 0;                                                   // Nicht vorhanden
    for (const FbxNode& p : p70->children) {                              // Alle Einträge
        if (p.props.size() >= 5 && p.props[0].text == "LocalStop") return p.props[4].integer; // Gefunden
    }                                                                     // Ende der Schleife
    return 0;                                                             // Nicht gefunden
} // Ende von readLocalStop

// Eine Verbindung zwischen zwei Objekten
struct Connection {                 // Beginn der Struktur
    std::string kind;               // "OO" (Objekt-Objekt) oder "OP" (Objekt-Eigenschaft)
    std::int64_t child = 0;         // Kindobjekt
    std::int64_t parent = 0;        // Elternobjekt
    std::string property;           // Eigenschaftsname bei "OP"
}; // Ende von Connection

// Liest alle Verbindungen
std::vector<Connection> readConnections(const FbxNode& root) {            // Beginn von readConnections
    std::vector<Connection> result;                                       // Ergebnis
    const FbxNode* conns = root.child("Connections");                     // Block suchen
    if (!conns) return result;                                            // Nicht vorhanden
    for (const FbxNode& c : conns->children) {                            // Alle Einträge "C"
        if (c.props.size() < 3) continue;                                 // Zu kurz
        Connection con;                                                   // Neue Verbindung
        con.kind = c.props[0].text;                                       // Art
        con.child = c.props[1].integer;                                   // Kind
        con.parent = c.props[2].integer;                                  // Eltern
        if (c.props.size() > 3) con.property = c.props[3].text;           // Eigenschaft
        result.push_back(con);                                            // Speichern
    }                                                                     // Ende der Schleife
    return result;                                                        // Ergebnis
} // Ende von readConnections

// Lokale Matrix eines Knochens: T * R * S
Mat4 localMatrix(const Vec3& t, const Vec3& r, const Vec3& s) {           // Beginn von localMatrix
    return Mat4::translation(t) * eulerXYZ(r) * Mat4::scale(s);           // Verschieben * Drehen * Skalieren
} // Ende von localMatrix

} // Ende des internen Namensraums

// Sucht den ersten Unterknoten mit einem Namen
const FbxNode* FbxNode::child(const std::string& n) const {               // Beginn von child
    for (const FbxNode& c : children) if (c.name == n) return &c;         // Durchsuchen
    return nullptr;                                                       // Nicht gefunden
} // Ende von child

// Liest eine binäre FBX-Datei vollständig ein
bool parseFbxFile(const std::string& path, FbxNode& root, std::string& error) { // Beginn von parseFbxFile
    std::ifstream file(path, std::ios::binary);                           // Datei binär öffnen
    if (!file) { error = "FBX nicht gefunden: " + path; return false; }    // Fehler melden
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); // Ganze Datei lesen
    if (data.size() < 27 || std::memcmp(data.data(), "Kaydara FBX Binary", 18) != 0) { error = "Keine binäre FBX-Datei: " + path; return false; } // Kennung prüfen
    std::size_t pos = 23;                                                 // Position der Versionsnummer
    std::uint32_t version = readValue<std::uint32_t>(data, pos);          // Version lesen
    bool wide = version >= 7500;                                          // Ab 7.5 sind die Längenangaben 64 Bit breit
    root = FbxNode();                                                     // Wurzel leeren
    bool ok = true;                                                       // Fehlermerker
    while (ok && pos + 13 < data.size()) {                                // Alle Knoten der obersten Ebene
        FbxNode node;                                                     // Neuer Knoten
        if (!readNode(data, pos, wide, node, ok)) break;                  // Null-Knoten -> Ende
        root.children.push_back(std::move(node));                         // Speichern
    }                                                                     // Ende der Schleife
    if (!ok) { error = "FBX-Datei beschädigt: " + path; return false; }   // Fehler melden
    return true;                                                          // Erfolgreich
} // Ende von parseFbxFile

// Wert einer Kurve zu einem Zeitpunkt (lineare Interpolation)
float FbxAnimation::Curve::sample(double t) const {                       // Beginn von sample
    if (times.empty()) return 0.0f;                                       // Keine Werte
    if (t <= times.front()) return values.front();                        // Vor dem ersten Schlüssel
    if (t >= times.back()) return values.back();                          // Nach dem letzten Schlüssel
    std::size_t hi = 1;                                                   // Index des nächsten Schlüssels
    while (hi < times.size() && times[hi] < t) ++hi;                      // Passenden Abschnitt suchen
    double span = times[hi] - times[hi - 1];                              // Länge des Abschnitts
    float f = span > 0.0 ? static_cast<float>((t - times[hi - 1]) / span) : 0.0f; // Anteil im Abschnitt
    return values[hi - 1] + (values[hi] - values[hi - 1]) * f;            // Linear interpolieren
} // Ende von sample

// Berechnet die globalen Matrizen aller Knochen zu einem Zeitpunkt
std::map<std::string, Mat4> FbxAnimation::pose(double time) const {       // Beginn von pose
    std::vector<Mat4> global(bones.size());                               // Globale Matrizen
    std::vector<bool> done(bones.size(), false);                          // Schon berechnet?
    std::function<void(int)> compute = [&](int i) {                       // Rekursive Berechnung (Eltern zuerst)
        if (done[static_cast<std::size_t>(i)]) return;                    // Schon fertig
        const Bone& b = bones[static_cast<std::size_t>(i)];               // Knochen
        float v[9] = {b.translation.x, b.translation.y, b.translation.z, b.rotation.x, b.rotation.y, b.rotation.z, b.scaling.x, b.scaling.y, b.scaling.z}; // Ruhewerte
        for (int k = 0; k < 9; ++k) if (b.curve[k] >= 0) v[k] = curves[static_cast<std::size_t>(b.curve[k])].sample(time); // Animierte Werte einsetzen
        Mat4 local = localMatrix(Vec3(v[0], v[1], v[2]), Vec3(v[3], v[4], v[5]), Vec3(v[6], v[7], v[8])); // Lokale Matrix
        if (b.parent >= 0) {                                              // Hat einen Elternknochen
            compute(b.parent);                                            // Eltern zuerst berechnen
            global[static_cast<std::size_t>(i)] = global[static_cast<std::size_t>(b.parent)] * local; // Eltern * lokal
        } else {                                                          // Wurzel
            global[static_cast<std::size_t>(i)] = local;                  // Lokale Matrix ist global
        }                                                                 // Ende der Unterscheidung
        done[static_cast<std::size_t>(i)] = true;                         // Als fertig markieren
    };                                                                    // Ende der Lambda-Funktion
    std::map<std::string, Mat4> result;                                   // Ergebnis nach Namen
    for (std::size_t i = 0; i < bones.size(); ++i) {                      // Alle Knochen
        compute(static_cast<int>(i));                                     // Berechnen
        result[bones[i].name] = global[i];                                // Unter dem Namen speichern
    }                                                                     // Ende der Schleife
    return result;                                                        // Ergebnis zurückgeben
} // Ende von pose

// Berechnet die Eckpunkte der Figur in einer Pose (lineares Skinning, Gewichte normiert)
std::vector<Vec3> FbxCharacter::skin(const std::map<std::string, Mat4>& pose) const { // Beginn von skin
    std::vector<Vec3> sum(mesh.positions.size());                         // Gewichtete Summen
    std::vector<float> weight(mesh.positions.size(), 0.0f);               // Gewichtssummen
    for (const Cluster& c : clusters) {                                   // Alle Cluster
        auto it = pose.find(c.bone);                                      // Knochenmatrix suchen
        if (it == pose.end()) continue;                                   // Knochen fehlt in der Animation
        Mat4 m = it->second * c.offset;                                   // Gesamtmatrix: Knochen(t) * Bindung
        for (std::size_t k = 0; k < c.indices.size(); ++k) {              // Alle betroffenen Punkte
            int idx = c.indices[k];                                       // Punktindex
            if (idx < 0 || idx >= static_cast<int>(sum.size())) continue; // Ungültig
            sum[static_cast<std::size_t>(idx)] += m.transformPoint(mesh.positions[static_cast<std::size_t>(idx)]) * c.weights[k]; // Gewichtet aufaddieren
            weight[static_cast<std::size_t>(idx)] += c.weights[k];        // Gewicht aufaddieren
        }                                                                 // Ende der Punktschleife
    }                                                                     // Ende der Clusterschleife
    for (std::size_t i = 0; i < sum.size(); ++i) {                        // Alle Punkte
        if (weight[i] > 1e-6f) sum[i] = sum[i] * (1.0f / weight[i]);      // Gewichte normieren (Kenney-Gewichte ergeben oft mehr als 1)
        else sum[i] = mesh.positions[i];                                  // Ohne Gewicht: Ruheposition
    }                                                                     // Ende der Schleife
    return sum;                                                           // Fertige Eckpunkte
} // Ende von skin

// Lädt eine Figur (Netz + Skin) aus einer FBX-Datei
bool loadFbxCharacter(const std::string& path, FbxCharacter& out, std::string& error) { // Beginn von loadFbxCharacter
    FbxNode root;                                                         // Dateibaum
    if (!parseFbxFile(path, root, error)) return false;                   // Datei lesen
    const FbxNode* objects = root.child("Objects");                       // Objektliste
    if (!objects) { error = "Keine Objekte in " + path; return false; }   // Fehler
    std::vector<Connection> conns = readConnections(root);                // Verbindungen
    out = FbxCharacter();                                                 // Ergebnis leeren
    std::map<std::int64_t, std::string> modelNames;                       // Modell-ID -> Name
    std::map<std::int64_t, const FbxNode*> clusterNodes;                  // Cluster-ID -> Knoten
    const FbxNode* geometry = nullptr;                                    // Das Dreiecksnetz
    for (const FbxNode& o : objects->children) {                          // Alle Objekte
        if (o.props.size() < 3) continue;                                 // Zu kurz
        if (o.name == "Geometry" && o.props[2].text == "Mesh" && !geometry) geometry = &o; // Erstes Netz merken
        if (o.name == "Model") modelNames[o.props[0].integer] = o.props[1].text; // Modellname merken
        if (o.name == "Deformer" && o.props[2].text == "Cluster") clusterNodes[o.props[0].integer] = &o; // Cluster merken
    }                                                                     // Ende der Schleife
    if (!geometry) { error = "Kein Netz in " + path; return false; }      // Fehler
    const FbxNode* vertices = geometry->child("Vertices");                // Eckpunkte
    const FbxNode* polys = geometry->child("PolygonVertexIndex");         // Flächenindizes
    if (!vertices || !polys || vertices->props.empty() || polys->props.empty()) { error = "Netz unvollständig"; return false; } // Fehler
    const std::vector<double>& v = vertices->props[0].numbers;            // Koordinaten
    for (std::size_t i = 0; i + 2 < v.size(); i += 3) out.mesh.positions.push_back(Vec3(static_cast<float>(v[i]), static_cast<float>(v[i + 1]), static_cast<float>(v[i + 2]))); // Punkte übernehmen
    const std::vector<std::int64_t>& pvi = polys->props[0].integers;      // Flächenindizes (negativ = letzter Punkt einer Fläche)
    std::vector<std::int64_t> uvIndex;                                    // UV-Index je Flächenecke
    bool uvDirect = false;                                                // UVs direkt je Flächenecke?
    if (const FbxNode* uvLayer = geometry->child("LayerElementUV")) {     // UV-Schicht vorhanden
        if (const FbxNode* uv = uvLayer->child("UV")) for (double d : uv->props[0].numbers) out.mesh.uvs.push_back(static_cast<float>(d)); // UV-Werte
        if (const FbxNode* ui = uvLayer->child("UVIndex")) uvIndex = ui->props[0].integers; // UV-Indizes
        else uvDirect = true;                                             // Ohne Indexliste: direkt
    }                                                                     // Ende UV
    out.mesh.materials.push_back(Material{"skin", rgba(255, 255, 255), nullptr}); // Ein Material (Textur kommt später)
    std::vector<std::pair<int, int>> polygon;                             // Aktuelle Fläche: (Punkt, UV)
    for (std::size_t k = 0; k < pvi.size(); ++k) {                        // Alle Flächenecken
        std::int64_t idx = pvi[k];                                        // Index
        bool last = idx < 0;                                              // Letzte Ecke der Fläche?
        int vertex = static_cast<int>(last ? ~idx : idx);                 // Echter Punktindex
        int uv = -1;                                                      // UV-Index
        if (!out.mesh.uvs.empty()) uv = uvDirect ? static_cast<int>(k) : (k < uvIndex.size() ? static_cast<int>(uvIndex[k]) : -1); // UV bestimmen
        polygon.push_back({vertex, uv});                                  // Ecke merken
        if (!last) continue;                                              // Fläche noch nicht fertig
        for (std::size_t j = 1; j + 1 < polygon.size(); ++j) {            // Als Fächer in Dreiecke zerlegen
            Triangle t;                                                   // Neues Dreieck
            t.v[0] = polygon[0].first; t.v[1] = polygon[j].first; t.v[2] = polygon[j + 1].first; // Punkte
            t.uv[0] = polygon[0].second; t.uv[1] = polygon[j].second; t.uv[2] = polygon[j + 1].second; // UVs
            out.mesh.triangles.push_back(t);                              // Speichern
        }                                                                 // Ende der Zerlegung
        polygon.clear();                                                  // Nächste Fläche
    }                                                                     // Ende der Eckenschleife
    std::vector<Mat4> transforms;                                         // Alle "Transform"-Matrizen (für die Konventionserkennung)
    std::vector<Mat4> links;                                              // Alle "TransformLink"-Matrizen
    for (const auto& entry : clusterNodes) {                              // Alle Cluster
        const FbxNode* cn = entry.second;                                 // Clusterknoten
        FbxCharacter::Cluster cluster;                                    // Neuer Cluster
        for (const Connection& c : conns) {                               // Knochen über die Verbindungen finden
            if (c.kind == "OO" && c.parent == entry.first && modelNames.count(c.child)) cluster.bone = modelNames[c.child]; // Knochen -> Cluster
        }                                                                 // Ende der Suche
        const FbxNode* idx = cn->child("Indexes");                        // Punktindizes
        const FbxNode* w = cn->child("Weights");                          // Gewichte
        const FbxNode* tr = cn->child("Transform");                       // Netzmatrix
        const FbxNode* tl = cn->child("TransformLink");                   // Knochenmatrix bei der Bindung
        if (!idx || !w || !tr || !tl || cluster.bone.empty()) continue;   // Unvollständig -> überspringen
        for (std::int64_t i : idx->props[0].integers) cluster.indices.push_back(static_cast<int>(i)); // Indizes übernehmen
        for (double d : w->props[0].numbers) cluster.weights.push_back(static_cast<float>(d)); // Gewichte übernehmen
        Mat4 transform = Mat4::fromArray(tr->props[0].numbers.data());    // Transform
        Mat4 link = Mat4::fromArray(tl->props[0].numbers.data());         // TransformLink
        transforms.push_back(transform);                                  // Für die Erkennung merken
        links.push_back(link);                                            // Für die Erkennung merken
        cluster.weights.resize(cluster.indices.size(), 0.0f);             // Gewichtsliste passend zur Indexliste machen
        cluster.offset = transform;                                       // Annahme Blender-Export: Transform = Knochen^-1 * Netz
        out.clusters.push_back(cluster);                                  // Speichern
    }                                                                     // Ende der Clusterschleife
    bool allSame = transforms.size() > 1;                                 // Sind alle Transform-Matrizen gleich? (dann FBX-Standard)
    for (std::size_t i = 1; i < transforms.size() && allSame; ++i) {      // Mit der ersten vergleichen
        for (int k = 0; k < 16; ++k) if (std::fabs(transforms[i].m[k] - transforms[0].m[k]) > 1e-4f) { allSame = false; break; } // Unterschied gefunden
    }                                                                     // Ende des Vergleichs
    if (allSame) {                                                        // Standardkonvention (z.B. Maya, 3ds Max)
        for (std::size_t i = 0; i < out.clusters.size(); ++i) out.clusters[i].offset = links[i].inverse() * transforms[i]; // Link^-1 * Transform
    }                                                                     // Ende der Standardkonvention
    if (out.mesh.triangles.empty()) { error = "Keine Dreiecke in " + path; return false; } // Fehler
    return true;                                                          // Erfolgreich
} // Ende von loadFbxCharacter

// Lädt eine Animation (Knochen + Kurven) aus einer FBX-Datei
bool loadFbxAnimation(const std::string& path, const std::string& stackName, FbxAnimation& out, std::string& error) { // Beginn von loadFbxAnimation
    FbxNode root;                                                         // Dateibaum
    if (!parseFbxFile(path, root, error)) return false;                   // Datei lesen
    const FbxNode* objects = root.child("Objects");                       // Objektliste
    if (!objects) { error = "Keine Objekte in " + path; return false; }   // Fehler
    std::vector<Connection> conns = readConnections(root);                // Verbindungen
    out = FbxAnimation();                                                 // Ergebnis leeren
    std::map<std::int64_t, int> boneIndex;                                // Modell-ID -> Knochenindex
    std::map<std::int64_t, const FbxNode*> byId;                          // Objekt-ID -> Knoten
    const FbxNode* stack = nullptr;                                       // Gewählte Animation
    std::int64_t bestStop = -1;                                           // Längste Animation (Ersatz)
    const FbxNode* longest = nullptr;                                     // Längste Animation
    for (const FbxNode& o : objects->children) {                          // Alle Objekte
        if (o.props.empty()) continue;                                    // Ohne ID überspringen
        byId[o.props[0].integer] = &o;                                    // Nach ID merken
        if (o.name == "Model" && o.props.size() >= 2) {                   // Knochen/Modell
            FbxAnimation::Bone b;                                         // Neuer Knochen
            b.name = o.props[1].text;                                     // Name
            std::map<std::string, Vec3> p = readProperties70(o);          // Ruhewerte
            if (p.count("Lcl Translation")) b.translation = p["Lcl Translation"]; // Verschiebung
            if (p.count("Lcl Rotation")) b.rotation = p["Lcl Rotation"];  // Drehung
            if (p.count("Lcl Scaling")) b.scaling = p["Lcl Scaling"];     // Skalierung
            boneIndex[o.props[0].integer] = static_cast<int>(out.bones.size()); // Index merken
            out.bones.push_back(b);                                       // Speichern
        }                                                                 // Ende Model
        if (o.name == "AnimationStack" && o.props.size() >= 2) {          // Animation
            std::int64_t stop = readLocalStop(o);                         // Länge
            if (stop > bestStop) { bestStop = stop; longest = &o; }       // Längste merken
            if (!stackName.empty() && o.props[1].text.find(stackName) != std::string::npos) stack = &o; // Gewünschter Name gefunden
        }                                                                 // Ende AnimationStack
    }                                                                     // Ende der Schleife
    if (!stack) stack = longest;                                          // Ersatz: längste Animation
    if (!stack) { error = "Keine Animation in " + path; return false; }   // Fehler
    out.duration = static_cast<double>(readLocalStop(*stack)) / FBX_TICKS_PER_SECOND; // Länge in Sekunden
    std::int64_t stackId = stack->props[0].integer;                       // ID der Animation
    for (const Connection& c : conns) {                                   // Elternknochen bestimmen
        if (c.kind == "OO" && boneIndex.count(c.child) && boneIndex.count(c.parent)) out.bones[static_cast<std::size_t>(boneIndex[c.child])].parent = boneIndex[c.parent]; // Kind -> Eltern
    }                                                                     // Ende der Schleife
    std::vector<std::int64_t> layers;                                     // Ebenen der Animation
    for (const Connection& c : conns) if (c.kind == "OO" && c.parent == stackId) layers.push_back(c.child); // Ebenen sammeln
    for (const Connection& cn : conns) {                                  // Kurvenknoten der Ebenen
        if (cn.kind != "OO") continue;                                    // Nur Objekt-Objekt
        bool inLayer = false;                                             // Gehört der Knoten zu einer Ebene der Animation?
        for (std::int64_t l : layers) if (cn.parent == l) inLayer = true; // Prüfen
        if (!inLayer) continue;                                           // Nicht zu dieser Animation
        std::int64_t curveNode = cn.child;                                // Kurvenknoten
        int bone = -1;                                                    // Zielknochen
        int base = -1;                                                    // 0 = T, 3 = R, 6 = S
        for (const Connection& t : conns) {                               // Ziel des Kurvenknotens suchen
            if (t.kind == "OP" && t.child == curveNode && boneIndex.count(t.parent)) { // Kurvenknoten -> Knochen-Eigenschaft
                bone = boneIndex[t.parent];                               // Knochen
                if (t.property == "Lcl Translation") base = 0;            // Verschiebung
                else if (t.property == "Lcl Rotation") base = 3;          // Drehung
                else if (t.property == "Lcl Scaling") base = 6;           // Skalierung
            }                                                             // Ende der Prüfung
        }                                                                 // Ende der Suche
        if (bone < 0 || base < 0) continue;                               // Kein passendes Ziel
        for (const Connection& k : conns) {                               // Kurven des Kurvenknotens
            if (k.kind != "OP" || k.parent != curveNode || !byId.count(k.child)) continue; // Nur Kurven dieses Knotens
            int axis = k.property == "d|X" ? 0 : (k.property == "d|Y" ? 1 : (k.property == "d|Z" ? 2 : -1)); // Achse
            if (axis < 0) continue;                                       // Unbekannte Achse
            const FbxNode* curveObj = byId[k.child];                      // Kurvenobjekt
            const FbxNode* kt = curveObj->child("KeyTime");               // Zeitpunkte
            const FbxNode* kv = curveObj->child("KeyValueFloat");         // Werte
            if (!kt || !kv) continue;                                     // Unvollständig
            FbxAnimation::Curve curve;                                    // Neue Kurve
            for (std::int64_t t : kt->props[0].integers) curve.times.push_back(static_cast<double>(t) / FBX_TICKS_PER_SECOND); // Zeiten in Sekunden
            for (double v : kv->props[0].numbers) curve.values.push_back(static_cast<float>(v)); // Werte
            if (curve.times.empty() || curve.times.size() != curve.values.size()) continue; // Ungültig
            out.bones[static_cast<std::size_t>(bone)].curve[base + axis] = static_cast<int>(out.curves.size()); // Kurve zuordnen
            out.curves.push_back(curve);                                  // Speichern
        }                                                                 // Ende der Kurvenschleife
    }                                                                     // Ende der Kurvenknotenschleife
    return true;                                                          // Erfolgreich
} // Ende von loadFbxAnimation
