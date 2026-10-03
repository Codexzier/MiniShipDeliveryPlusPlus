// SaveGame.cpp - Spielstand im Format der Eigenschaftsdateien schreiben und lesen
#include "SaveGame.h" // Eigene Deklarationen

#include <cstdio> // std::snprintf
#include <ctime>  // std::time, std::localtime

#include "PropertyFile.h" // Textformat [Objekt] eigenschaft = wert

namespace SaveGame { // Beginn des Namensraums

// Schreibt den Spielstand in eine Textdatei
bool write(const std::string& path, const SaveData& data) {                // Beginn von write
    PropertyFile file;                                                      // Neue, leere Datei
    std::time_t now = std::time(nullptr);                                   // Aktuelle Zeit
    std::tm* local = std::localtime(&now);                                  // Lokale Zeit
    char stamp[64];                                                         // Puffer für Datum/Uhrzeit
    std::snprintf(stamp, sizeof(stamp), "%04d-%02d-%02d %02d:%02d:%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec); // Formatieren
    file.set("Spielstand", "level", data.level);                            // Karte
    file.set("Spielstand", "gespeichert_am", stamp);                        // Zeitpunkt
    file.setFloat("Spieler", "x", data.x);                                  // Position
    file.setFloat("Spieler", "leben", data.health);                         // Leben
    file.setFloat("Spieler", "ausdauer", data.stamina);                     // Ausdauer
    file.setInt("Spieler", "coins", data.coins);                            // Coins
    for (int i = 0; i < Inventory::SIZE; ++i) {                             // Alle Inventarfelder
        const InventorySlot& slot = data.slots[static_cast<std::size_t>(i)]; // Feld
        std::string key = "feld_" + std::to_string(i + 1);                  // Name der Eigenschaft (feld_1 .. feld_8)
        file.set("Inventar", key, slot.empty() ? "" : slot.itemId);         // Gegenstand (leer = frei)
        file.setInt("Inventar", key + "_anzahl", slot.empty() ? 0 : slot.count); // Anzahl
    }                                                                       // Ende der Schleife
    std::string collected;                                                  // Kommagetrennte Liste der Münzen
    for (const std::string& id : data.collectedCoins) collected += (collected.empty() ? "" : ", ") + id; // Liste zusammensetzen
    file.set("Muenzen", "eingesammelt", collected);                         // Eingesammelte Münzen
    return file.save(path, "Spielstand von Mini Ship Delivery (wird automatisch geschrieben)"); // Datei speichern
} // Ende von write

// Liest einen Spielstand aus einer Textdatei
bool read(const std::string& path, SaveData& data) {                       // Beginn von read
    PropertyFile file;                                                      // Datei-Objekt
    if (!file.load(path)) return false;                                     // Datei fehlt
    if (!file.hasSection("Spielstand")) return false;                       // Kein gültiger Spielstand
    data.level = file.getString("Spielstand", "level", "level1.txt");       // Karte
    data.savedAt = file.getString("Spielstand", "gespeichert_am", "");      // Zeitpunkt
    data.x = file.getFloat("Spieler", "x", 1.5f);                           // Position
    data.health = file.getFloat("Spieler", "leben", 100.0f);                // Leben
    data.stamina = file.getFloat("Spieler", "ausdauer", 100.0f);            // Ausdauer
    data.coins = file.getInt("Spieler", "coins", 0);                        // Coins
    for (int i = 0; i < Inventory::SIZE; ++i) {                             // Alle Inventarfelder
        std::string key = "feld_" + std::to_string(i + 1);                  // Name der Eigenschaft
        InventorySlot slot;                                                 // Neues Feld
        slot.itemId = file.getString("Inventar", key, "");                  // Gegenstand
        slot.count = slot.itemId.empty() ? 0 : file.getInt("Inventar", key + "_anzahl", 1); // Anzahl
        data.slots[static_cast<std::size_t>(i)] = slot;                     // Speichern
    }                                                                       // Ende der Schleife
    data.collectedCoins = file.getList("Muenzen", "eingesammelt");          // Eingesammelte Münzen
    return true;                                                            // Erfolgreich gelesen
} // Ende von read

} // Ende des Namensraums SaveGame
