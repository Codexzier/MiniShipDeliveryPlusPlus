// PropertyFile.h - Liest und schreibt Eigenschafts-Textdateien
//
// Aufbau einer solchen Datei (Beispiel):
//   # Kommentarzeile
//   [Spieler]                    <- Name des Objekts
//   lauf_geschwindigkeit = 3.2   <- Name der Eigenschaft = Wert
//
#pragma once // Header nur einmal einbinden

#include <string> // std::string für Texte
#include <vector> // std::vector für Listen

class PropertyFile {                                         // Beginn der Klasse PropertyFile
public:                                                      // Öffentliche Schnittstelle
    bool load(const std::string& path);                      // Datei einlesen, true bei Erfolg
    bool save(const std::string& path, const std::string& headerComment = "") const; // Datei schreiben

    bool hasSection(const std::string& section) const;       // Gibt es ein Objekt mit diesem Namen?
    bool has(const std::string& section, const std::string& key) const; // Gibt es die Eigenschaft im Objekt?

    std::string getString(const std::string& section, const std::string& key, const std::string& fallback = "") const; // Text lesen
    float getFloat(const std::string& section, const std::string& key, float fallback) const; // Kommazahl lesen
    int getInt(const std::string& section, const std::string& key, int fallback) const;       // Ganzzahl lesen
    bool getBool(const std::string& section, const std::string& key, bool fallback) const;    // Ja/Nein-Wert lesen
    std::vector<std::string> getList(const std::string& section, const std::string& key) const; // Kommagetrennte Liste lesen

    void set(const std::string& section, const std::string& key, const std::string& value); // Text setzen
    void setFloat(const std::string& section, const std::string& key, float value);         // Kommazahl setzen
    void setInt(const std::string& section, const std::string& key, int value);             // Ganzzahl setzen

    std::vector<std::string> sectionNames() const;                                   // Alle Objektnamen in Dateireihenfolge
    std::vector<std::string> sectionsWithPrefix(const std::string& prefix) const;    // Objektnamen, die mit prefix beginnen
    const std::string& path() const { return m_path; }                               // Pfad der zuletzt geladenen Datei

    static std::string trim(const std::string& text);        // Leerzeichen am Anfang/Ende entfernen
    static std::string toLower(const std::string& text);     // Text in Kleinbuchstaben umwandeln (nur ASCII)

private:                                                     // Interne Daten
    struct Entry {                                           // Eine Eigenschaft
        std::string key;                                     // Name der Eigenschaft
        std::string value;                                   // Wert der Eigenschaft als Text
    };                                                       // Ende von Entry
    struct Section {                                         // Ein Objekt mit seinen Eigenschaften
        std::string name;                                    // Name des Objekts
        std::vector<Entry> entries;                          // Eigenschaften in Dateireihenfolge
    };                                                       // Ende von Section

    const Section* findSection(const std::string& name) const; // Objekt suchen (nur lesen)
    Section* findSection(const std::string& name);             // Objekt suchen (zum Ändern)
    const std::string* findValue(const std::string& section, const std::string& key) const; // Wert suchen

    std::vector<Section> m_sections;                         // Alle Objekte der Datei
    std::string m_path;                                      // Dateipfad
}; // Ende der Klasse PropertyFile
