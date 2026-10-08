// Mesh.h - 3D-Modell aus Dreiecken mit Materialfarben (und optional Texturen) sowie OBJ/MTL-Lader
#pragma once // Header nur einmal einbinden

#include <memory> // std::shared_ptr für gemeinsam genutzte Texturen
#include <string> // std::string
#include <vector> // std::vector

#include "Common.h"   // Color
#include "Graphics.h" // Image (Texturen)
#include "Math3D.h"   // Vec3, Mat4

// Material eines Dreiecks
struct Material {                                    // Beginn der Struktur
    std::string name;                                // Name aus der MTL-Datei
    Color color = rgba(200, 200, 200);               // Grundfarbe (Kd)
    std::shared_ptr<Image> texture;                  // Optionale Textur (map_Kd oder Figuren-Skin)
}; // Ende der Struktur Material

// Ein Dreieck: drei Eckpunkte, drei Texturkoordinaten und ein Material
struct Triangle {                                    // Beginn der Struktur
    int v[3] = {0, 0, 0};                            // Indizes in Mesh::positions
    int uv[3] = {-1, -1, -1};                        // Indizes in Mesh::uvs (-1 = keine)
    int material = 0;                                // Index in Mesh::materials
}; // Ende der Struktur Triangle

// Ein komplettes 3D-Modell
struct Mesh {                                                            // Beginn der Struktur
    std::vector<Vec3> positions;                                         // Eckpunkte (Modellraum, y zeigt nach oben)
    std::vector<float> uvs;                                              // Texturkoordinaten paarweise (u, v)
    std::vector<Triangle> triangles;                                     // Dreiecke
    std::vector<Material> materials;                                     // Materialien

    bool empty() const { return triangles.empty(); }                     // Hat das Modell keine Dreiecke?
    void bounds(Vec3& minOut, Vec3& maxOut) const;                       // Umgebender Quader
    void append(const Mesh& other, const Mat4& transform, Color tint = rgba(255, 255, 255)); // Anderes Modell transformiert anhängen
}; // Ende der Struktur Mesh

bool loadObj(const std::string& path, Mesh& out, std::string& error);    // OBJ-Datei samt MTL laden
Color multiplyColor(Color a, Color b);                                   // Farben kanalweise multiplizieren
