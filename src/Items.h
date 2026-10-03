// Items.h - Gegenstände (aus data/gegenstaende.txt) und das Inventar der Spielfigur (4x2 Felder)
#pragma once // Header nur einmal einbinden

#include <array>  // std::array für die festen Inventarfelder
#include <map>    // std::map für die Gegenstandsliste
#include <string> // std::string
#include <vector> // std::vector

#include "PropertyFile.h" // Laden der Eigenschaften

// Wirkung eines Gegenstands
enum class ItemEffect { // Beginn der Aufzählung
    None,               // Keine Wirkung
    Stamina,            // Füllt Ausdauer auf
    Health,             // Füllt Leben auf
    Hook,               // Schaltet den Enterhaken frei
    MaxStamina          // Erhöht die maximale Ausdauer, solange er im Inventar ist
}; // Ende der Aufzählung ItemEffect

// Beschreibung eines Gegenstands
struct ItemDef {                     // Beginn der Struktur
    std::string id;                  // Objektname in der Textdatei (z.B. "Enterhaken")
    std::string name;                // Anzeigename
    std::string description;         // Kurze Beschreibung für den Tooltip
    std::string icon;                // Name des Symbols (beutel, haken, energy)
    int price = 0;                   // Preis in Coins
    ItemEffect effect = ItemEffect::None; // Wirkung
    float value = 0.0f;              // Stärke der Wirkung
    bool consumable = false;         // Wird der Gegenstand beim Benutzen verbraucht?
    bool unique = false;             // Darf man ihn nur einmal besitzen?
    int maxStack = 1;                // Wie viele passen in ein Inventarfeld?
}; // Ende der Struktur ItemDef

// Alle bekannten Gegenstände
class ItemDatabase {                                              // Beginn der Klasse
public:                                                           // Öffentliche Schnittstelle
    bool load(const std::string& path);                           // Gegenstände aus Datei lesen
    const ItemDef* find(const std::string& id) const;             // Gegenstand suchen (nullptr, wenn unbekannt)
    const std::vector<std::string>& ids() const { return m_order; } // Alle IDs in Dateireihenfolge
private:                                                          // Interne Daten
    std::map<std::string, ItemDef> m_items;                       // Gegenstände nach klein geschriebener ID
    std::vector<std::string> m_order;                             // Reihenfolge der IDs
}; // Ende der Klasse ItemDatabase

// Ein Feld im Inventar
struct InventorySlot {                         // Beginn der Struktur
    std::string itemId;                        // ID des Gegenstands (leer = Feld frei)
    int count = 0;                             // Anzahl
    bool empty() const { return count <= 0 || itemId.empty(); } // Ist das Feld frei?
}; // Ende der Struktur InventorySlot

// Inventar mit 4 Feldern nebeneinander und 2 Reihen
class Inventory {                                                 // Beginn der Klasse
public:                                                           // Öffentliche Schnittstelle
    static constexpr int COLUMNS = 4;                             // Felder nebeneinander
    static constexpr int ROWS = 2;                                // Reihen
    static constexpr int SIZE = COLUMNS * ROWS;                   // Felder insgesamt

    void clear();                                                 // Alle Felder leeren
    bool add(const ItemDef& item);                                // Gegenstand hinzufügen (false, wenn voll)
    bool has(const std::string& id) const;                        // Besitzt die Figur den Gegenstand?
    void removeOne(int slot);                                     // Einen Gegenstand aus einem Feld entfernen
    const InventorySlot& slot(int index) const { return m_slots[static_cast<std::size_t>(index)]; } // Feld lesen
    void setSlot(int index, const InventorySlot& value);          // Feld direkt setzen (Spielstand laden)
    float sumEffect(ItemEffect effect, const ItemDatabase& db) const; // Summe passiver Wirkungen
    bool hasEffect(ItemEffect effect, const ItemDatabase& db) const;  // Gibt es einen Gegenstand mit dieser Wirkung?
private:                                                          // Interne Daten
    std::array<InventorySlot, SIZE> m_slots;                      // Die acht Felder
}; // Ende der Klasse Inventory
