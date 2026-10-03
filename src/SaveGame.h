// SaveGame.h - Speichern und Laden des Spielstands (ebenfalls als lesbare Textdatei)
#pragma once // Header nur einmal einbinden

#include <array>  // std::array für die Inventarfelder
#include <string> // std::string
#include <vector> // std::vector

#include "Items.h" // InventorySlot und Inventory::SIZE

// Alle Daten eines Spielstands
struct SaveData {                                                   // Beginn der Struktur
    std::string level;                                              // Dateiname der Karte
    float x = 0.0f;                                                 // Position der Figur
    float health = 100.0f;                                          // Leben
    float stamina = 100.0f;                                         // Ausdauer
    int coins = 0;                                                  // Coins
    std::array<InventorySlot, Inventory::SIZE> slots;               // Inventar
    std::vector<std::string> collectedCoins;                        // Bereits eingesammelte Münzen
    std::string savedAt;                                            // Datum und Uhrzeit des Speicherns
}; // Ende der Struktur SaveData

namespace SaveGame {                                                // Namensraum für Spielstand-Funktionen
bool write(const std::string& path, const SaveData& data);          // Spielstand schreiben
bool read(const std::string& path, SaveData& data);                 // Spielstand lesen
} // Ende des Namensraums SaveGame
