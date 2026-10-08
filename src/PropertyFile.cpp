// PropertyFile.cpp - Umsetzung des Lesens und Schreibens der Eigenschafts-Textdateien
#include "PropertyFile.h" // Eigene Klassendeklaration

#include <cstdlib>  // std::strtof und std::strtol zum Umwandeln von Text in Zahlen
#include <fstream>  // std::ifstream / std::ofstream für Dateizugriffe
#include <iomanip>  // std::setprecision für das Formatieren von Kommazahlen
#include <locale>   // std::locale::classic, damit immer ein Punkt als Dezimaltrenner geschrieben wird
#include <sstream>  // std::ostringstream zum Zusammenbauen von Texten

// Entfernt Leerzeichen, Tabulatoren und Zeilenenden am Anfang und Ende eines Textes
std::string PropertyFile::trim(const std::string& text) {      // Beginn von trim
    const char* spaces = " \t\r\n";                            // Zeichen, die entfernt werden sollen
    std::size_t start = text.find_first_not_of(spaces);        // Erstes Nicht-Leerzeichen suchen
    if (start == std::string::npos) return "";                 // Nur Leerzeichen -> leerer Text
    std::size_t end = text.find_last_not_of(spaces);           // Letztes Nicht-Leerzeichen suchen
    return text.substr(start, end - start + 1);                // Teil dazwischen zurückgeben
} // Ende von trim

// Wandelt alle ASCII-Großbuchstaben in Kleinbuchstaben um
std::string PropertyFile::toLower(const std::string& text) {   // Beginn von toLower
    std::string result = text;                                 // Kopie des Textes anlegen
    for (char& c : result) {                                   // Jedes Zeichen durchgehen
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a'); // Großbuchstaben umwandeln
    }                                                          // Ende der Schleife
    return result;                                             // Umgewandelten Text zurückgeben
} // Ende von toLower

// Liest eine Datei ein und zerlegt sie in Objekte und Eigenschaften
bool PropertyFile::load(const std::string& path) {             // Beginn von load
    std::ifstream file(path);                                  // Datei zum Lesen öffnen
    if (!file) return false;                                   // Datei nicht gefunden -> Fehler melden
    m_sections.clear();                                        // Alte Inhalte verwerfen
    m_path = path;                                             // Pfad merken
    Section* current = nullptr;                                // Aktuelles Objekt (noch keins)
    std::string line;                                          // Puffer für eine Zeile
    bool firstLine = true;                                     // Merker für die erste Zeile (wegen UTF-8-BOM)
    while (std::getline(file, line)) {                         // Zeile für Zeile lesen
        if (firstLine && line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) { // Beginnt die Datei mit einem UTF-8-BOM?
            line = line.substr(3);                             // BOM (3 Bytes) entfernen
        }                                                      // Ende der BOM-Prüfung
        firstLine = false;                                     // Ab jetzt keine erste Zeile mehr
        std::string trimmed = trim(line);                      // Leerzeichen entfernen
        if (trimmed.empty()) continue;                         // Leere Zeilen überspringen
        if (trimmed[0] == '#' || trimmed[0] == ';') continue;  // Kommentarzeilen überspringen
        if (trimmed.front() == '[' && trimmed.back() == ']') { // Zeile der Form [Objektname]?
            Section section;                                   // Neues Objekt anlegen
            section.name = trim(trimmed.substr(1, trimmed.size() - 2)); // Namen zwischen den Klammern übernehmen
            m_sections.push_back(section);                     // Objekt zur Liste hinzufügen
            current = &m_sections.back();                      // Neues Objekt ist jetzt das aktuelle
            continue;                                          // Weiter mit der nächsten Zeile
        }                                                      // Ende der Objekt-Erkennung
        std::size_t eq = trimmed.find('=');                    // Gleichheitszeichen suchen
        if (eq == std::string::npos) continue;                 // Keine gültige Zeile -> ignorieren
        if (!current) {                                        // Eigenschaft vor dem ersten Objekt?
            Section section;                                   // Ein namenloses Objekt anlegen
            m_sections.push_back(section);                     // ... und zur Liste hinzufügen
            current = &m_sections.back();                      // ... und als aktuelles Objekt nutzen
        }                                                      // Ende der Prüfung
        Entry entry;                                           // Neue Eigenschaft
        entry.key = toLower(trim(trimmed.substr(0, eq)));      // Name links vom '=' (klein geschrieben)
        entry.value = trim(trimmed.substr(eq + 1));            // Wert rechts vom '='
        current->entries.push_back(entry);                     // Eigenschaft im Objekt speichern
    }                                                          // Ende der Zeilenschleife
    return true;                                               // Erfolgreich gelesen
} // Ende von load

// Schreibt alle Objekte und Eigenschaften in eine Datei
bool PropertyFile::save(const std::string& path, const std::string& headerComment) const { // Beginn von save
    std::ofstream file(path);                                  // Datei zum Schreiben öffnen (wird überschrieben)
    if (!file) return false;                                   // Datei konnte nicht angelegt werden
    if (!headerComment.empty()) file << "# " << headerComment << "\n\n"; // Optionalen Kopfkommentar schreiben
    for (const Section& section : m_sections) {                // Alle Objekte durchgehen
        file << "[" << section.name << "]\n";                  // Objektnamen in eckigen Klammern schreiben
        for (const Entry& entry : section.entries) {           // Alle Eigenschaften des Objekts
            file << entry.key << " = " << entry.value << "\n"; // Zeile "name = wert" schreiben
        }                                                      // Ende der Eigenschaftsschleife
        file << "\n";                                          // Leerzeile zwischen den Objekten
    }                                                          // Ende der Objektschleife
    return static_cast<bool>(file);                            // true, wenn beim Schreiben kein Fehler auftrat
} // Ende von save

// Sucht ein Objekt (Groß-/Kleinschreibung egal) und gibt einen Zeiger zum Lesen zurück
const PropertyFile::Section* PropertyFile::findSection(const std::string& name) const { // Beginn von findSection (const)
    std::string wanted = toLower(name);                        // Gesuchten Namen klein schreiben
    for (const Section& section : m_sections) {                // Alle Objekte durchsuchen
        if (toLower(section.name) == wanted) return &section;  // Treffer gefunden
    }                                                          // Ende der Suche
    return nullptr;                                            // Nicht gefunden
} // Ende von findSection (const)

// Sucht ein Objekt und gibt einen veränderbaren Zeiger zurück
PropertyFile::Section* PropertyFile::findSection(const std::string& name) { // Beginn von findSection
    std::string wanted = toLower(name);                        // Gesuchten Namen klein schreiben
    for (Section& section : m_sections) {                      // Alle Objekte durchsuchen
        if (toLower(section.name) == wanted) return &section;  // Treffer gefunden
    }                                                          // Ende der Suche
    return nullptr;                                            // Nicht gefunden
} // Ende von findSection

// Sucht den Wert einer Eigenschaft in einem Objekt
const std::string* PropertyFile::findValue(const std::string& section, const std::string& key) const { // Beginn von findValue
    const Section* s = findSection(section);                   // Zuerst das Objekt suchen
    if (!s) return nullptr;                                    // Objekt existiert nicht
    std::string wanted = toLower(key);                         // Eigenschaftsnamen klein schreiben
    for (const Entry& entry : s->entries) {                    // Alle Eigenschaften durchsuchen
        if (entry.key == wanted) return &entry.value;          // Treffer -> Zeiger auf den Wert
    }                                                          // Ende der Suche
    return nullptr;                                            // Eigenschaft existiert nicht
} // Ende von findValue

bool PropertyFile::hasSection(const std::string& section) const { return findSection(section) != nullptr; } // Objekt vorhanden?
bool PropertyFile::has(const std::string& section, const std::string& key) const { return findValue(section, key) != nullptr; } // Eigenschaft vorhanden?

// Liest eine Eigenschaft als Text
std::string PropertyFile::getString(const std::string& section, const std::string& key, const std::string& fallback) const { // Beginn von getString
    const std::string* value = findValue(section, key);        // Wert suchen
    return value ? *value : fallback;                          // Gefundenen Wert oder Ersatzwert zurückgeben
} // Ende von getString

// Liest eine Eigenschaft als Kommazahl (Punkt oder Komma als Dezimaltrenner erlaubt)
float PropertyFile::getFloat(const std::string& section, const std::string& key, float fallback) const { // Beginn von getFloat
    const std::string* value = findValue(section, key);        // Wert suchen
    if (!value || value->empty()) return fallback;             // Nicht vorhanden -> Ersatzwert
    std::string text = *value;                                 // Kopie zum Bearbeiten
    for (char& c : text) if (c == ',') c = '.';                // Deutsches Komma in Punkt umwandeln
    char* end = nullptr;                                       // Zeiger auf das Ende der gelesenen Zahl
    float result = std::strtof(text.c_str(), &end);            // Text in Zahl umwandeln
    if (end == text.c_str()) return fallback;                  // Keine Zahl erkannt -> Ersatzwert
    return result;                                             // Gelesene Zahl zurückgeben
} // Ende von getFloat

// Liest eine Eigenschaft als Ganzzahl
int PropertyFile::getInt(const std::string& section, const std::string& key, int fallback) const { // Beginn von getInt
    const std::string* value = findValue(section, key);        // Wert suchen
    if (!value || value->empty()) return fallback;             // Nicht vorhanden -> Ersatzwert
    char* end = nullptr;                                       // Zeiger auf das Ende der gelesenen Zahl
    long result = std::strtol(value->c_str(), &end, 10);       // Text als Dezimalzahl lesen
    if (end == value->c_str()) return fallback;                // Keine Zahl erkannt -> Ersatzwert
    return static_cast<int>(result);                           // Gelesene Zahl zurückgeben
} // Ende von getInt

// Liest eine Eigenschaft als Wahrheitswert (1/0, ja/nein, true/false, an/aus)
bool PropertyFile::getBool(const std::string& section, const std::string& key, bool fallback) const { // Beginn von getBool
    const std::string* value = findValue(section, key);        // Wert suchen
    if (!value) return fallback;                               // Nicht vorhanden -> Ersatzwert
    std::string v = toLower(*value);                           // Klein geschriebenen Wert vergleichen
    if (v == "1" || v == "ja" || v == "true" || v == "an") return true;    // Bedeutet "wahr"
    if (v == "0" || v == "nein" || v == "false" || v == "aus") return false; // Bedeutet "falsch"
    return fallback;                                           // Unbekannter Wert -> Ersatzwert
} // Ende von getBool

// Liest eine kommagetrennte Liste, z.B. "Enterhaken, Energy_Dose"
std::vector<std::string> PropertyFile::getList(const std::string& section, const std::string& key) const { // Beginn von getList
    std::vector<std::string> result;                           // Ergebnisliste
    const std::string* value = findValue(section, key);        // Wert suchen
    if (!value) return result;                                 // Nicht vorhanden -> leere Liste
    std::stringstream stream(*value);                          // Text in einen Datenstrom packen
    std::string item;                                          // Puffer für einen Listeneintrag
    while (std::getline(stream, item, ',')) {                  // Am Komma trennen
        item = trim(item);                                     // Leerzeichen entfernen
        if (!item.empty()) result.push_back(item);             // Nicht-leere Einträge übernehmen
    }                                                          // Ende der Schleife
    return result;                                             // Liste zurückgeben
} // Ende von getList

// Setzt den Text-Wert einer Eigenschaft (legt Objekt und Eigenschaft bei Bedarf an)
void PropertyFile::set(const std::string& section, const std::string& key, const std::string& value) { // Beginn von set
    Section* s = findSection(section);                         // Objekt suchen
    if (!s) {                                                  // Objekt gibt es noch nicht
        Section created;                                       // Neues Objekt anlegen
        created.name = section;                                // Namen setzen
        m_sections.push_back(created);                         // Zur Liste hinzufügen
        s = &m_sections.back();                                // Zeiger auf das neue Objekt
    }                                                          // Ende der Objekt-Prüfung
    std::string lowKey = toLower(key);                         // Eigenschaftsnamen klein schreiben
    for (Entry& entry : s->entries) {                          // Vorhandene Eigenschaften durchsuchen
        if (entry.key == lowKey) {                             // Eigenschaft existiert schon
            entry.value = value;                               // Wert überschreiben
            return;                                            // Fertig
        }                                                      // Ende der Prüfung
    }                                                          // Ende der Suche
    s->entries.push_back(Entry{lowKey, value});                // Neue Eigenschaft anhängen
} // Ende von set

// Setzt eine Kommazahl (immer mit Punkt als Dezimaltrenner)
void PropertyFile::setFloat(const std::string& section, const std::string& key, float value) { // Beginn von setFloat
    std::ostringstream stream;                                 // Datenstrom zum Formatieren
    stream.imbue(std::locale::classic());                      // Neutrale Ländereinstellung (Punkt als Trenner)
    stream << std::fixed << std::setprecision(3) << value;     // Mit drei Nachkommastellen schreiben
    set(section, key, stream.str());                           // Als Text speichern
} // Ende von setFloat

void PropertyFile::setInt(const std::string& section, const std::string& key, int value) { set(section, key, std::to_string(value)); } // Ganzzahl als Text speichern

// Liefert die Namen aller Objekte
std::vector<std::string> PropertyFile::sectionNames() const {  // Beginn von sectionNames
    std::vector<std::string> names;                            // Ergebnisliste
    for (const Section& section : m_sections) names.push_back(section.name); // Jeden Namen übernehmen
    return names;                                              // Liste zurückgeben
} // Ende von sectionNames

// Liefert alle Eigenschaftsnamen eines Objekts
std::vector<std::string> PropertyFile::keys(const std::string& section) const { // Beginn von keys
    std::vector<std::string> result;                           // Ergebnisliste
    const Section* s = findSection(section);                   // Objekt suchen
    if (s) for (const Entry& e : s->entries) result.push_back(e.key); // Alle Namen übernehmen
    return result;                                             // Liste zurückgeben
} // Ende von keys

// Liefert alle Objektnamen, die mit einem bestimmten Präfix beginnen (z.B. "Graben")
std::vector<std::string> PropertyFile::sectionsWithPrefix(const std::string& prefix) const { // Beginn von sectionsWithPrefix
    std::vector<std::string> names;                            // Ergebnisliste
    std::string lowPrefix = toLower(prefix);                   // Präfix klein schreiben
    for (const Section& section : m_sections) {                // Alle Objekte prüfen
        if (toLower(section.name).rfind(lowPrefix, 0) == 0) names.push_back(section.name); // Beginnt der Name mit dem Präfix?
    }                                                          // Ende der Schleife
    return names;                                              // Liste zurückgeben
} // Ende von sectionsWithPrefix
