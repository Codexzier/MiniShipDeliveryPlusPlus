// Math3D.h - Kleine 3D-Mathematik: Vektoren und 4x4-Matrizen für den Software-3D-Renderer
#pragma once // Header nur einmal einbinden

#include <cmath> // std::sin, std::cos, std::sqrt

// Dreidimensionaler Vektor
struct Vec3 {                                                                  // Beginn der Struktur
    float x = 0.0f;                                                            // x-Anteil
    float y = 0.0f;                                                            // y-Anteil
    float z = 0.0f;                                                            // z-Anteil
    Vec3() = default;                                                          // Nullvektor
    Vec3(float ax, float ay, float az) : x(ax), y(ay), z(az) {}                // Vektor aus drei Werten
    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); } // Addition
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); } // Subtraktion
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }         // Mit einer Zahl multiplizieren
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; } // Aufaddieren
}; // Ende der Struktur Vec3

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; } // Skalarprodukt
inline Vec3 cross(const Vec3& a, const Vec3& b) { return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); } // Kreuzprodukt
inline float length(const Vec3& v) { return std::sqrt(dot(v, v)); }            // Länge eines Vektors
inline Vec3 normalize(const Vec3& v) { float l = length(v); return l > 1e-12f ? v * (1.0f / l) : Vec3(0, 0, 1); } // Auf Länge 1 bringen

// 4x4-Matrix im Spaltenformat (m[spalte*4 + zeile]), wie in OpenGL und FBX üblich
struct Mat4 {                                                                  // Beginn der Struktur
    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};            // Startwert: Einheitsmatrix
    float& at(int row, int col) { return m[col * 4 + row]; }                   // Element (Zeile, Spalte) zum Schreiben
    float at(int row, int col) const { return m[col * 4 + row]; }              // Element (Zeile, Spalte) zum Lesen

    static Mat4 identity() { return Mat4(); }                                  // Einheitsmatrix
    static Mat4 translation(const Vec3& t) {                                   // Verschiebungsmatrix
        Mat4 r;                                                                // Einheitsmatrix als Basis
        r.at(0, 3) = t.x;                                                      // Verschiebung x
        r.at(1, 3) = t.y;                                                      // Verschiebung y
        r.at(2, 3) = t.z;                                                      // Verschiebung z
        return r;                                                              // Ergebnis
    } // Ende von translation
    static Mat4 scale(const Vec3& s) {                                         // Skalierungsmatrix
        Mat4 r;                                                                // Einheitsmatrix als Basis
        r.at(0, 0) = s.x;                                                      // Faktor x
        r.at(1, 1) = s.y;                                                      // Faktor y
        r.at(2, 2) = s.z;                                                      // Faktor z
        return r;                                                              // Ergebnis
    } // Ende von scale
    static Mat4 rotationX(float a) {                                           // Drehung um die x-Achse (Radiant)
        Mat4 r;                                                                // Einheitsmatrix als Basis
        float c = std::cos(a), s = std::sin(a);                                // Kosinus und Sinus
        r.at(1, 1) = c; r.at(1, 2) = -s; r.at(2, 1) = s; r.at(2, 2) = c;        // Drehanteile eintragen
        return r;                                                              // Ergebnis
    } // Ende von rotationX
    static Mat4 rotationY(float a) {                                           // Drehung um die y-Achse (Radiant)
        Mat4 r;                                                                // Einheitsmatrix als Basis
        float c = std::cos(a), s = std::sin(a);                                // Kosinus und Sinus
        r.at(0, 0) = c; r.at(0, 2) = s; r.at(2, 0) = -s; r.at(2, 2) = c;        // Drehanteile eintragen
        return r;                                                              // Ergebnis
    } // Ende von rotationY
    static Mat4 rotationZ(float a) {                                           // Drehung um die z-Achse (Radiant)
        Mat4 r;                                                                // Einheitsmatrix als Basis
        float c = std::cos(a), s = std::sin(a);                                // Kosinus und Sinus
        r.at(0, 0) = c; r.at(0, 1) = -s; r.at(1, 0) = s; r.at(1, 1) = c;        // Drehanteile eintragen
        return r;                                                              // Ergebnis
    } // Ende von rotationZ
    static Mat4 fromArray(const double* values) {                              // Matrix aus 16 Zahlen (Spaltenformat, z.B. aus FBX)
        Mat4 r;                                                                // Neue Matrix
        for (int i = 0; i < 16; ++i) r.m[i] = static_cast<float>(values[i]);   // Werte übernehmen
        return r;                                                              // Ergebnis
    } // Ende von fromArray

    Mat4 operator*(const Mat4& o) const {                                      // Matrixprodukt (this * o)
        Mat4 r;                                                                // Ergebnis
        for (int row = 0; row < 4; ++row) {                                    // Alle Zeilen
            for (int col = 0; col < 4; ++col) {                                // Alle Spalten
                float sum = 0.0f;                                              // Summe des Skalarprodukts
                for (int k = 0; k < 4; ++k) sum += at(row, k) * o.at(k, col);  // Zeile mal Spalte
                r.at(row, col) = sum;                                          // Eintragen
            }                                                                  // Ende der Spaltenschleife
        }                                                                      // Ende der Zeilenschleife
        return r;                                                              // Ergebnis
    } // Ende von operator*

    Vec3 transformPoint(const Vec3& p) const {                                 // Punkt transformieren (mit Verschiebung)
        return Vec3(at(0, 0) * p.x + at(0, 1) * p.y + at(0, 2) * p.z + at(0, 3), // Neues x
                    at(1, 0) * p.x + at(1, 1) * p.y + at(1, 2) * p.z + at(1, 3), // Neues y
                    at(2, 0) * p.x + at(2, 1) * p.y + at(2, 2) * p.z + at(2, 3)); // Neues z
    } // Ende von transformPoint

    Vec3 transformVector(const Vec3& v) const {                                // Richtung transformieren (ohne Verschiebung)
        return Vec3(at(0, 0) * v.x + at(0, 1) * v.y + at(0, 2) * v.z,           // Neues x
                    at(1, 0) * v.x + at(1, 1) * v.y + at(1, 2) * v.z,           // Neues y
                    at(2, 0) * v.x + at(2, 1) * v.y + at(2, 2) * v.z);          // Neues z
    } // Ende von transformVector

    Mat4 inverse() const {                                                     // Inverse Matrix (Gauß-Jordan-Verfahren)
        float a[4][8];                                                         // Erweiterte Matrix [this | Einheit]
        for (int r = 0; r < 4; ++r) {                                          // Alle Zeilen
            for (int c = 0; c < 4; ++c) {                                      // Alle Spalten
                a[r][c] = at(r, c);                                            // Linke Hälfte: diese Matrix
                a[r][c + 4] = (r == c) ? 1.0f : 0.0f;                          // Rechte Hälfte: Einheitsmatrix
            }                                                                  // Ende der Spaltenschleife
        }                                                                      // Ende der Zeilenschleife
        for (int c = 0; c < 4; ++c) {                                          // Jede Spalte als Pivotspalte
            int pivot = c;                                                     // Zeile mit dem größten Wert suchen
            for (int r = c + 1; r < 4; ++r) if (std::fabs(a[r][c]) > std::fabs(a[pivot][c])) pivot = r; // Bessere Pivotzeile
            if (std::fabs(a[pivot][c]) < 1e-12f) return Mat4();                // Nicht invertierbar -> Einheitsmatrix
            if (pivot != c) for (int k = 0; k < 8; ++k) { float t = a[c][k]; a[c][k] = a[pivot][k]; a[pivot][k] = t; } // Zeilen tauschen
            float inv = 1.0f / a[c][c];                                        // Kehrwert des Pivotelements
            for (int k = 0; k < 8; ++k) a[c][k] *= inv;                        // Pivotzeile normieren
            for (int r = 0; r < 4; ++r) {                                      // Alle anderen Zeilen
                if (r == c) continue;                                          // Pivotzeile überspringen
                float f = a[r][c];                                             // Faktor zum Eliminieren
                for (int k = 0; k < 8; ++k) a[r][k] -= f * a[c][k];            // Spalte c in Zeile r auf 0 bringen
            }                                                                  // Ende der Eliminationsschleife
        }                                                                      // Ende der Pivotschleife
        Mat4 r;                                                                // Ergebnis
        for (int row = 0; row < 4; ++row) for (int col = 0; col < 4; ++col) r.at(row, col) = a[row][col + 4]; // Rechte Hälfte übernehmen
        return r;                                                              // Inverse zurückgeben
    } // Ende von inverse
}; // Ende der Struktur Mat4

// Drehmatrix aus Eulerwinkeln in Grad (Reihenfolge XYZ wie in FBX: erst X, dann Y, dann Z)
inline Mat4 eulerXYZ(const Vec3& degrees) {                                    // Beginn von eulerXYZ
    const float d2r = 3.14159265358979f / 180.0f;                              // Umrechnung Grad -> Radiant
    return Mat4::rotationZ(degrees.z * d2r) * Mat4::rotationY(degrees.y * d2r) * Mat4::rotationX(degrees.x * d2r); // Rz * Ry * Rx
} // Ende von eulerXYZ
