// Items.cpp - Umsetzung von Gegenstandsliste und Inventar
#include "Items.h" // Eigene Deklarationen

#include <algorithm> // std::max

namespace { // Interne Hilfsfunktion

// Wandelt den Text aus der Datei in eine Wirkung um
ItemEffect parseEffect(const std::string& text) {                       // Beginn von parseEffect
    std::string t = PropertyFile::toLower(text);                        // Klein geschrieben vergleichen
    if (t == "ausdauer") return ItemEffect::Stamina;                    // Ausdauer auffüllen
    if (t == "leben") return ItemEffect::Health;                        // Leben auffüllen
    if (t == "enterhaken") return ItemEffect::Hook;                     // Enterhaken freischalten
    if (t == "max_ausdauer") return ItemEffect::MaxStamina;             // Maximale Ausdauer erhöhen
    return ItemEffect::None;                                            // Alles andere: keine Wirkung
} // Ende von parseEffect

} // Ende des internen Namensraums

// Liest alle Gegenstände aus der Textdatei
bool ItemDatabase::load(const std::string& path) {                      // Beginn von load
    PropertyFile file;                                                  // Datei-Objekt
    if (!file.load(path)) return false;                                 // Datei fehlt -> Fehler
    m_items.clear();                                                    // Alte Einträge löschen
    m_order.clear();                                                    // Alte Reihenfolge löschen
    for (const std::string& section : file.sectionNames()) {            // Jedes Objekt ist ein Gegenstand
        ItemDef item;                                                   // Neuer Gegenstand
        item.id = section;                                              // ID = Objektname
        item.name = file.getString(section, "name", section);           // Anzeigename
        item.description = file.getString(section, "beschreibung", ""); // Beschreibung
        item.icon = file.getString(section, "symbol", "unbekannt");     // Symbolname
        item.price = std::max(0, file.getInt(section, "preis", 0));     // Preis
        item.effect = parseEffect(file.getString(section, "effekt", "keiner")); // Wirkung
        item.value = file.getFloat(section, "wert", 0.0f);              // Stärke der Wirkung
        item.consumable = file.getBool(section, "verbrauchbar", false); // Verbrauchbar?
        item.unique = file.getBool(section, "einzigartig", false);      // Nur einmal besitzbar?
        item.maxStack = std::max(1, file.getInt(section, "stapel_max", 1)); // Stapelgröße
        m_items[PropertyFile::toLower(section)] = item;                 // Unter klein geschriebener ID speichern
        m_order.push_back(section);                                     // Reihenfolge merken
    }                                                                   // Ende der Schleife
    return true;                                                        // Erfolgreich geladen
} // Ende von load

// Sucht einen Gegenstand (Groß-/Kleinschreibung egal)
const ItemDef* ItemDatabase::find(const std::string& id) const {        // Beginn von find
    auto it = m_items.find(PropertyFile::toLower(id));                  // In der Tabelle suchen
    return it == m_items.end() ? nullptr : &it->second;                 // Zeiger oder nullptr zurückgeben
} // Ende von find

// Leert alle Inventarfelder
void Inventory::clear() { for (InventorySlot& s : m_slots) s = InventorySlot{}; } // Jedes Feld zurücksetzen

// Fügt einen Gegenstand hinzu: zuerst auf passende Stapel, sonst in ein freies Feld
bool Inventory::add(const ItemDef& item) {                              // Beginn von add
    for (InventorySlot& s : m_slots) {                                  // Vorhandene Stapel suchen
        if (!s.empty() && PropertyFile::toLower(s.itemId) == PropertyFile::toLower(item.id) && s.count < item.maxStack) { // Gleicher Gegenstand mit Platz
            ++s.count;                                                  // Stapel erhöhen
            return true;                                                // Erfolgreich
        }                                                               // Ende der Prüfung
    }                                                                   // Ende der Stapelsuche
    for (InventorySlot& s : m_slots) {                                  // Freies Feld suchen
        if (s.empty()) {                                                // Feld ist frei
            s.itemId = item.id;                                         // Gegenstand eintragen
            s.count = 1;                                                // Anzahl 1
            return true;                                                // Erfolgreich
        }                                                               // Ende der Prüfung
    }                                                                   // Ende der Suche
    return false;                                                       // Kein Platz mehr
} // Ende von add

// Prüft, ob ein Gegenstand im Inventar liegt
bool Inventory::has(const std::string& id) const {                      // Beginn von has
    for (const InventorySlot& s : m_slots) {                            // Alle Felder prüfen
        if (!s.empty() && PropertyFile::toLower(s.itemId) == PropertyFile::toLower(id)) return true; // Gefunden
    }                                                                   // Ende der Schleife
    return false;                                                       // Nicht gefunden
} // Ende von has

// Entfernt einen Gegenstand aus einem Feld
void Inventory::removeOne(int slot) {                                   // Beginn von removeOne
    if (slot < 0 || slot >= SIZE) return;                               // Ungültiges Feld ignorieren
    InventorySlot& s = m_slots[static_cast<std::size_t>(slot)];         // Referenz auf das Feld
    if (s.empty()) return;                                              // Leeres Feld ignorieren
    --s.count;                                                          // Anzahl verringern
    if (s.count <= 0) s = InventorySlot{};                              // Leer geworden -> Feld freigeben
} // Ende von removeOne

// Setzt ein Feld direkt (wird beim Laden eines Spielstands benutzt)
void Inventory::setSlot(int index, const InventorySlot& value) {        // Beginn von setSlot
    if (index < 0 || index >= SIZE) return;                             // Ungültiges Feld ignorieren
    m_slots[static_cast<std::size_t>(index)] = value;                   // Feld überschreiben
} // Ende von setSlot

// Addiert die Werte aller Gegenstände mit einer bestimmten (passiven) Wirkung
float Inventory::sumEffect(ItemEffect effect, const ItemDatabase& db) const { // Beginn von sumEffect
    float sum = 0.0f;                                                   // Startwert
    for (const InventorySlot& s : m_slots) {                            // Alle Felder
        if (s.empty()) continue;                                        // Leere Felder überspringen
        const ItemDef* def = db.find(s.itemId);                         // Gegenstand nachschlagen
        if (def && def->effect == effect) sum += def->value * static_cast<float>(s.count); // Wert addieren
    }                                                                   // Ende der Schleife
    return sum;                                                         // Summe zurückgeben
} // Ende von sumEffect

// Prüft, ob irgendein Gegenstand die gewünschte Wirkung hat
bool Inventory::hasEffect(ItemEffect effect, const ItemDatabase& db) const { // Beginn von hasEffect
    for (const InventorySlot& s : m_slots) {                            // Alle Felder
        if (s.empty()) continue;                                        // Leere Felder überspringen
        const ItemDef* def = db.find(s.itemId);                         // Gegenstand nachschlagen
        if (def && def->effect == effect) return true;                  // Wirkung gefunden
    }                                                                   // Ende der Schleife
    return false;                                                       // Nicht gefunden
} // Ende von hasEffect
