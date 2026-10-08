// FbxLoader.h - Lädt animierte Figuren aus binären FBX-Dateien (Kenney "Animated Characters")
//
// Unterstützt: Dreiecksnetz mit Texturkoordinaten, Skin-Cluster (Knochengewichte),
// Knochenhierarchie mit Lcl Translation/Rotation/Scaling und Animationskurven.
#pragma once // Header nur einmal einbinden

#include <cstdint> // std::int64_t
#include <map>     // std::map
#include <string>  // std::string
#include <vector>  // std::vector

#include "Math3D.h" // Vec3, Mat4
#include "Mesh.h"   // Mesh

// Eine Eigenschaft eines FBX-Knotens (Zahl, Text oder Liste)
struct FbxProperty {                       // Beginn der Struktur
    char type = 0;                         // Typkennung aus der Datei (I, L, D, S, f, d, i, l ...)
    std::int64_t integer = 0;              // Ganzzahlwert
    double number = 0.0;                   // Kommazahl
    std::string text;                      // Text
    std::vector<double> numbers;           // Liste von Kommazahlen
    std::vector<std::int64_t> integers;    // Liste von Ganzzahlen
}; // Ende der Struktur FbxProperty

// Ein Knoten im FBX-Baum
struct FbxNode {                                                   // Beginn der Struktur
    std::string name;                                              // Knotenname (z.B. "Objects", "Model")
    std::vector<FbxProperty> props;                                // Eigenschaften
    std::vector<FbxNode> children;                                 // Unterknoten
    const FbxNode* child(const std::string& n) const;              // Ersten Unterknoten mit diesem Namen suchen
}; // Ende der Struktur FbxNode

bool parseFbxFile(const std::string& path, FbxNode& root, std::string& error); // Binäre FBX-Datei einlesen

// Eine Animation (z.B. "Run") samt Knochenhierarchie
struct FbxAnimation {                                              // Beginn der Struktur
    struct Curve {                                                 // Zeitverlauf eines Wertes
        std::vector<double> times;                                 // Zeitpunkte in Sekunden
        std::vector<float> values;                                 // Werte
        float sample(double t) const;                              // Wert zu einer Zeit (linear interpoliert)
    };                                                             // Ende von Curve
    struct Bone {                                                  // Ein Knochen
        std::string name;                                          // Name (verbindet Animation und Modell)
        int parent = -1;                                           // Index des Elternknochens (-1 = Wurzel)
        Vec3 translation;                                          // Ruhewert Verschiebung
        Vec3 rotation;                                             // Ruhewert Drehung (Grad)
        Vec3 scaling = Vec3(1, 1, 1);                              // Ruhewert Skalierung
        int curve[9] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};       // Kurven für T(x,y,z), R(x,y,z), S(x,y,z)
    };                                                             // Ende von Bone
    std::vector<Bone> bones;                                       // Alle Knochen
    std::vector<Curve> curves;                                     // Alle Kurven
    double duration = 0.0;                                         // Länge in Sekunden
    std::map<std::string, Mat4> pose(double time) const;           // Globale Knochenmatrizen zu einem Zeitpunkt
}; // Ende der Struktur FbxAnimation

// Eine Figur mit Skin (Netz + Knochengewichte)
struct FbxCharacter {                                              // Beginn der Struktur
    struct Cluster {                                               // Einfluss eines Knochens auf Eckpunkte
        std::string bone;                                          // Name des Knochens
        std::vector<int> indices;                                  // Betroffene Eckpunkte
        std::vector<float> weights;                                // Gewichte
        Mat4 offset;                                               // Bindungsmatrix (Netz -> Knochen)
    };                                                             // Ende von Cluster
    Mesh mesh;                                                     // Netz mit UVs (Positionen im Netzraum)
    std::vector<Cluster> clusters;                                 // Alle Cluster
    std::vector<Vec3> skin(const std::map<std::string, Mat4>& pose) const; // Eckpunkte in einer Pose berechnen
}; // Ende der Struktur FbxCharacter

bool loadFbxCharacter(const std::string& path, FbxCharacter& out, std::string& error); // Figur laden
bool loadFbxAnimation(const std::string& path, const std::string& stackName, FbxAnimation& out, std::string& error); // Animation laden
