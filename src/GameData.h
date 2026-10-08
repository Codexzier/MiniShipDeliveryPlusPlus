// GameData.h - Feste Spieldaten aus den Textdateien in data/ (Waren, Schiffe, Crew, Upgrades, Artefakte, Händler, Hersteller)
#pragma once // Header nur einmal einbinden

#include <map>     // std::map
#include <string>  // std::string
#include <utility> // std::pair
#include <vector>  // std::vector

// Kategorie einer Ware
enum class GoodCategory { Raw, Finished, Vegetable, Fruit, Food, Luxury }; // Rohware, Fertigware, Gemüse, Obst, Lebensmittel, Luxus

// Form einer Ladeeinheit im Laderaum (Minispiel)
struct CargoShape {                                     // Beginn der Struktur
    std::vector<std::pair<int, int>> cells;             // Belegte Felder (x, y)
    int width = 1;                                      // Breite
    int height = 1;                                     // Höhe
    CargoShape rotated() const;                         // Um 90 Grad gedreht
}; // Ende der Struktur CargoShape

// Eine Ware
struct GoodDef {                                        // Beginn der Struktur
    std::string id;                                     // Objektname in der Datei
    std::string name;                                   // Anzeigename
    std::string model;                                  // Modell für das Symbol
    std::string description;                            // Kurzbeschreibung
    GoodCategory category = GoodCategory::Raw;          // Kategorie
    int basePrice = 10;                                 // Grundpreis in Credits
    int weight = 1;                                     // Gewicht je Einheit (Schiffsbalance)
    int perishDays = 0;                                 // Tage bis zum Verderben (0 = nie)
    CargoShape shape;                                   // Form im Laderaum
}; // Ende der Struktur GoodDef

// Eine Schiffsklasse
struct ShipClass {                                      // Beginn der Struktur
    std::string id;                                     // Objektname
    std::string name;                                   // Anzeigename
    std::string model;                                  // Modellname
    std::string description;                            // Beschreibung
    int price = 0;                                      // Kaufpreis
    int holdWidth = 5;                                  // Laderaum: Felder nebeneinander
    int holdHeight = 3;                                 // Laderaum: Reihen
    float speed = 4.0f;                                 // Höchstgeschwindigkeit (Kacheln pro Sekunde)
    float repair = 1.0f;                                // Reparaturrate (Rumpfpunkte pro Spielstunde)
    float armor = 10.0f;                                // Panzerung (Schadensminderung in Prozent)
    float defense = 5.0f;                               // Abwehr (Kanonenschaden)
    float turn = 80.0f;                                 // Wendigkeit (Grad pro Sekunde)
    float scan = 8.0f;                                  // Sichtweite / Scanner (Kacheln)
    float trade = 100.0f;                               // Handel in Prozent (bei allen Schiffen gleich)
    float hull = 100.0f;                                // Rumpfpunkte (Gesundheit)
    std::string drive = "wind";                         // Antrieb: wind, motor oder hybrid
    float fuelMax = 0.0f;                               // Tankgröße
    float fuelUse = 0.0f;                               // Verbrauch pro Sekunde bei voller Fahrt
    float draft = 1.5f;                                 // Tiefgang in Metern
    int minCrew = 1;                                    // Mindestbesatzung
    int maxCrew = 2;                                    // Höchstbesatzung
}; // Ende der Struktur ShipClass

// Eine Crew-Rolle mit Bonus und Malus auf Schiffsattribute
struct CrewRole {                                       // Beginn der Struktur
    std::string id;                                     // Objektname
    std::string name;                                   // Anzeigename (z.B. "Navigator")
    std::string description;                            // Beschreibung
    std::map<std::string, float> bonus;                 // Attribut -> Verbesserung in Prozent
    std::map<std::string, float> malus;                 // Attribut -> Verschlechterung in Prozent (nur bei Unterbesetzung)
    int wage = 10;                                      // Tageslohn (Heuer)
    std::string figure;                                 // Figur für die Darstellung
}; // Ende der Struktur CrewRole

// Eine Schiffsverbesserung in der Werft
struct UpgradeDef {                                     // Beginn der Struktur
    std::string id;                                     // Objektname
    std::string name;                                   // Anzeigename
    std::string description;                            // Beschreibung
    std::string attribute;                              // Betroffenes Attribut
    float value = 0.0f;                                 // Wert je Stufe
    int price = 100;                                    // Preis der ersten Stufe (steigt je Stufe)
    int maxLevel = 3;                                   // Höchste Stufe
}; // Ende der Struktur UpgradeDef

// Ein Artefakt, das auf See gefunden werden kann
struct ArtifactDef {                                    // Beginn der Struktur
    std::string id;                                     // Objektname
    std::string name;                                   // Anzeigename
    std::string description;                            // Beschreibung
    std::string model;                                  // Modell (schwimmt im Wasser)
    int tier = 1;                                       // Stufe (höhere Stufen brauchen mehr Entdeckerpunkte)
    int value = 50;                                     // Verkaufswert im Museum
    int points = 5;                                     // Entdeckerpunkte beim Verkauf
}; // Ende der Struktur ArtifactDef

// Eine Händlerart (Gemüse, Obst, Fertigwaren ...)
struct TraderDef {                                      // Beginn der Struktur
    std::string id;                                     // Objektname
    std::string name;                                   // Anzeigename
    std::string kind;                                   // Art (Attribut zur Unterscheidung)
    std::string model;                                  // Marktstand oder Laden
    std::string figure;                                 // Figur des Händlers
    std::vector<std::string> buys;                      // Waren, die er kauft (Bedarf)
    std::vector<std::string> sells;                     // Waren, die er verkauft
    int demandMax = 8;                                  // Höchster Bedarf je Ware
    std::string demandMode = "taeglich";                // "taeglich" (einmal am Tag voll) oder "steigend" (wächst im Intervall)
    float demandRate = 1.0f;                            // Anstieg je Spielstunde bei "steigend"
    int stockMax = 6;                                   // Höchster Bestand je Ware
    float stockRate = 0.5f;                             // Wiederherstellung je Spielstunde
    float buyFactor = 1.35f;                            // Er zahlt so viel mal den Grundpreis
    float sellFactor = 1.2f;                            // Er verlangt so viel mal den Grundpreis
    float freshBonus = 0.3f;                            // Zuschlag für frische verderbliche Ware
}; // Ende der Struktur TraderDef

// Ein Hersteller (Tischler, Uhrenmacher, Kartenhersteller ...)
struct ProducerDef {                                    // Beginn der Struktur
    struct Recipe {                                     // Ein Rezept
        std::string product;                            // Erzeugtes Produkt
        std::vector<std::pair<std::string, int>> inputs; // Rohwaren und Mengen
    };                                                  // Ende von Recipe
    std::string id;                                     // Objektname
    std::string name;                                   // Anzeigename
    std::string figure;                                 // Figur des Herstellers
    std::vector<std::string> rawGoods;                  // Angekaufte Rohwaren
    std::vector<Recipe> recipes;                        // Rezepte
    float productionHours = 3.0f;                       // Spielstunden je Produkt
    int storage = 30;                                   // Lagerplatz (Rohwaren + Produkte)
    float buyFactor = 1.4f;                             // Zahlt so viel mal den Grundpreis für Rohwaren
    float sellFactor = 0.9f;                            // Verlangt so viel mal den Grundpreis für Produkte
}; // Ende der Struktur ProducerDef

// Alle festen Spieldaten
class GameData {                                                              // Beginn der Klasse
public:                                                                       // Öffentliche Schnittstelle
    bool load(const std::string& dataDir, std::string& error);                // Alle Dateien laden
    const GoodDef* good(const std::string& id) const;                         // Ware suchen
    const ShipClass* ship(const std::string& id) const;                       // Schiffsklasse suchen
    const CrewRole* role(const std::string& id) const;                        // Crew-Rolle suchen
    const UpgradeDef* upgrade(const std::string& id) const;                   // Upgrade suchen
    const ArtifactDef* artifact(const std::string& id) const;                 // Artefakt suchen
    const TraderDef* trader(const std::string& id) const;                     // Händlerart suchen
    const ProducerDef* producer(const std::string& id) const;                 // Hersteller suchen
    static std::string categoryName(GoodCategory c);                          // Anzeigename einer Kategorie

    std::vector<GoodDef> goods;                                               // Alle Waren
    std::vector<ShipClass> ships;                                             // Alle Schiffsklassen
    std::vector<CrewRole> roles;                                              // Alle Crew-Rollen
    std::vector<UpgradeDef> upgrades;                                         // Alle Upgrades
    std::vector<ArtifactDef> artifacts;                                       // Alle Artefakte
    std::vector<TraderDef> traders;                                           // Alle Händlerarten
    std::vector<ProducerDef> producers;                                       // Alle Hersteller
    std::vector<int> explorerThresholds;                                      // Entdeckerpunkte je Artefaktstufe
    std::vector<std::string> crewNames;                                       // Namen für Bewerber in der Taverne
}; // Ende der Klasse GameData

CargoShape parseShape(const std::string& text);                               // Form aus Text ("1x1", "2x1", "L" ...) erzeugen
