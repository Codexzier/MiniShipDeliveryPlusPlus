// GameData.cpp - Lädt die festen Spieldaten aus den Textdateien
#include "GameData.h" // Eigene Deklarationen

#include <algorithm> // std::max

#include <cstdlib>   // std::strtof, std::atoi

#include "Common.h"       // clampValue
#include "ImageIO.h"      // joinPath
#include "PropertyFile.h" // Textdateien

namespace { // Interne Hilfen

// Liest "attribut:wert, attribut:wert" in eine Tabelle
std::map<std::string, float> parseAttributeList(const std::vector<std::string>& items) { // Beginn von parseAttributeList
    std::map<std::string, float> result;                                     // Ergebnis
    for (const std::string& item : items) {                                  // Alle Einträge
        std::size_t colon = item.find(':');                                  // Doppelpunkt suchen
        if (colon == std::string::npos) continue;                            // Ohne Wert überspringen
        result[PropertyFile::trim(item.substr(0, colon))] = std::strtof(item.c_str() + colon + 1, nullptr); // Speichern
    }                                                                        // Ende der Schleife
    return result;                                                           // Ergebnis
} // Ende von parseAttributeList

// Wandelt den Text einer Kategorie in den Aufzählungswert um
GoodCategory parseCategory(const std::string& text) {                        // Beginn von parseCategory
    std::string t = PropertyFile::toLower(text);                             // Klein geschrieben
    if (t == "fertigware") return GoodCategory::Finished;                    // Fertigware
    if (t == "gemuese") return GoodCategory::Vegetable;                      // Gemüse
    if (t == "obst") return GoodCategory::Fruit;                             // Obst
    if (t == "lebensmittel") return GoodCategory::Food;                      // Lebensmittel
    if (t == "luxus") return GoodCategory::Luxury;                           // Luxus
    return GoodCategory::Raw;                                                // Standard: Rohware
} // Ende von parseCategory

// Sucht in einer Liste nach der ID (Groß-/Kleinschreibung egal)
template <typename T>                                                        // Beliebiger Datentyp mit "id"
const T* findById(const std::vector<T>& list, const std::string& id) {       // Beginn von findById
    std::string low = PropertyFile::toLower(id);                             // Klein geschrieben
    for (const T& item : list) if (PropertyFile::toLower(item.id) == low) return &item; // Gefunden
    return nullptr;                                                          // Nicht gefunden
} // Ende von findById

} // Ende des internen Namensraums

// Dreht eine Form um 90 Grad im Uhrzeigersinn
CargoShape CargoShape::rotated() const {                                     // Beginn von rotated
    CargoShape r;                                                            // Ergebnis
    r.width = height;                                                        // Breite und Höhe tauschen
    r.height = width;                                                        // Breite und Höhe tauschen
    for (const auto& c : cells) r.cells.push_back({height - 1 - c.second, c.first}); // (x, y) -> (h-1-y, x)
    return r;                                                                // Gedrehte Form
} // Ende von rotated

// Erzeugt eine Form aus einem Kürzel
CargoShape parseShape(const std::string& text) {                             // Beginn von parseShape
    CargoShape s;                                                            // Ergebnis
    std::string t = PropertyFile::toLower(PropertyFile::trim(text));         // Kürzel
    if (t == "2x1") s.cells = {{0, 0}, {1, 0}};                              // Zwei nebeneinander
    else if (t == "3x1") s.cells = {{0, 0}, {1, 0}, {2, 0}};                 // Drei nebeneinander
    else if (t == "2x2") s.cells = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};         // Quadrat
    else if (t == "l") s.cells = {{0, 0}, {0, 1}, {1, 1}};                   // L-Form
    else if (t == "t") s.cells = {{0, 0}, {1, 0}, {2, 0}, {1, 1}};           // T-Form
    else if (t == "3x2") s.cells = {{0, 0}, {1, 0}, {2, 0}, {0, 1}, {1, 1}, {2, 1}}; // Großer Block
    else s.cells = {{0, 0}};                                                 // Standard: ein Feld
    for (const auto& c : s.cells) { s.width = std::max(s.width, c.first + 1); s.height = std::max(s.height, c.second + 1); } // Größe bestimmen
    return s;                                                                // Fertige Form
} // Ende von parseShape

// Lädt alle Datendateien
bool GameData::load(const std::string& dataDir, std::string& error) {        // Beginn von load
    PropertyFile f;                                                          // Wiederverwendbares Datei-Objekt
    if (!f.load(ImageIO::joinPath(dataDir, "waren.txt"))) { error = "data/waren.txt fehlt"; return false; } // Waren
    for (const std::string& id : f.sectionNames()) {                         // Alle Waren
        GoodDef g;                                                           // Neue Ware
        g.id = id;                                                           // ID
        g.name = f.getString(id, "name", id);                                // Name
        g.model = f.getString(id, "modell", "");                             // Modell
        g.description = f.getString(id, "beschreibung", "");                 // Beschreibung
        g.category = parseCategory(f.getString(id, "kategorie", "rohware")); // Kategorie
        g.basePrice = std::max(1, f.getInt(id, "basispreis", 10));           // Grundpreis
        g.weight = std::max(1, f.getInt(id, "gewicht", 1));                  // Gewicht
        g.perishDays = std::max(0, f.getInt(id, "verderblich", 0));          // Haltbarkeit
        g.shape = parseShape(f.getString(id, "form", "1x1"));                // Form im Laderaum
        goods.push_back(g);                                                  // Speichern
    }                                                                        // Ende der Warenschleife
    if (!f.load(ImageIO::joinPath(dataDir, "schiffe.txt"))) { error = "data/schiffe.txt fehlt"; return false; } // Schiffe
    for (const std::string& id : f.sectionNames()) {                         // Alle Schiffsklassen
        ShipClass s;                                                         // Neue Klasse
        s.id = id;                                                           // ID
        s.name = f.getString(id, "name", id);                                // Name
        s.model = f.getString(id, "modell", "");                             // Modell
        s.description = f.getString(id, "beschreibung", "");                 // Beschreibung
        s.price = f.getInt(id, "preis", 0);                                  // Preis
        s.holdWidth = clampValue(f.getInt(id, "laderaum_breite", 5), 2, 12); // Laderaumbreite
        s.holdHeight = clampValue(f.getInt(id, "laderaum_hoehe", 3), 2, 8);  // Laderaumhöhe
        s.speed = f.getFloat(id, "geschwindigkeit", 4.0f);                   // Geschwindigkeit
        s.repair = f.getFloat(id, "reparatur", 1.0f);                        // Reparaturrate
        s.armor = f.getFloat(id, "panzerung", 10.0f);                        // Panzerung
        s.defense = f.getFloat(id, "abwehr", 5.0f);                          // Abwehr
        s.turn = f.getFloat(id, "wendigkeit", 80.0f);                        // Wendigkeit
        s.scan = f.getFloat(id, "sichtweite", 8.0f);                         // Sichtweite
        s.trade = f.getFloat(id, "handel", 100.0f);                          // Handel
        s.hull = f.getFloat(id, "rumpf", 100.0f);                            // Rumpf
        s.drive = PropertyFile::toLower(f.getString(id, "antrieb", "wind")); // Antrieb
        s.fuelMax = f.getFloat(id, "treibstoff_max", 0.0f);                  // Tank
        s.fuelUse = f.getFloat(id, "verbrauch", 0.0f);                       // Verbrauch
        s.draft = f.getFloat(id, "tiefgang", 1.5f);                          // Tiefgang
        s.minCrew = std::max(1, f.getInt(id, "mindest_crew", 1));            // Mindestbesatzung
        s.maxCrew = std::max(s.minCrew, f.getInt(id, "max_crew", 2));        // Höchstbesatzung
        ships.push_back(s);                                                  // Speichern
    }                                                                        // Ende der Schiffsschleife
    if (!f.load(ImageIO::joinPath(dataDir, "crew.txt"))) { error = "data/crew.txt fehlt"; return false; } // Crew
    for (const std::string& id : f.sectionNames()) {                         // Alle Abschnitte
        if (PropertyFile::toLower(id) == "namen") { crewNames = f.getList(id, "liste"); continue; } // Namensliste für Bewerber
        CrewRole r;                                                          // Neue Rolle
        r.id = id;                                                           // ID
        r.name = f.getString(id, "name", id);                                // Name
        r.description = f.getString(id, "beschreibung", "");                 // Beschreibung
        r.bonus = parseAttributeList(f.getList(id, "bonus"));                // Bonus
        r.malus = parseAttributeList(f.getList(id, "malus"));                // Malus
        r.wage = std::max(0, f.getInt(id, "heuer", 10));                     // Lohn
        r.figure = f.getString(id, "figur", "matrose");                      // Figur
        roles.push_back(r);                                                  // Speichern
    }                                                                        // Ende der Rollenschleife
    if (!f.load(ImageIO::joinPath(dataDir, "upgrades.txt"))) { error = "data/upgrades.txt fehlt"; return false; } // Upgrades
    for (const std::string& id : f.sectionNames()) {                         // Alle Upgrades
        UpgradeDef u;                                                        // Neues Upgrade
        u.id = id;                                                           // ID
        u.name = f.getString(id, "name", id);                                // Name
        u.description = f.getString(id, "beschreibung", "");                 // Beschreibung
        u.attribute = f.getString(id, "attribut", "");                       // Attribut
        u.value = f.getFloat(id, "wert", 0.0f);                              // Wert je Stufe
        u.price = std::max(1, f.getInt(id, "preis", 100));                   // Preis
        u.maxLevel = clampValue(f.getInt(id, "max_stufe", 3), 1, 9);         // Höchste Stufe
        upgrades.push_back(u);                                               // Speichern
    }                                                                        // Ende der Upgradeschleife
    if (!f.load(ImageIO::joinPath(dataDir, "artefakte.txt"))) { error = "data/artefakte.txt fehlt"; return false; } // Artefakte
    for (const std::string& id : f.sectionNames()) {                         // Alle Abschnitte
        if (PropertyFile::toLower(id) == "entdecker") {                      // Schwellen der Entdeckerpunkte
            for (int tier = 1; tier <= 9; ++tier) explorerThresholds.push_back(f.getInt(id, "stufe_" + std::to_string(tier), tier == 1 ? 0 : 999999)); // Je Stufe
            artifactsOnMap = clampValue(f.getInt(id, "anzahl_auf_karte", 6), 1, 40); // Höchstzahl auf der Karte
            artifactsPerDay = clampValue(f.getInt(id, "neue_pro_tag", 2), 0, 40);    // Neue je Tag
            continue;                                                        // Weiter
        }                                                                    // Ende Entdecker
        ArtifactDef a;                                                       // Neues Artefakt
        a.id = id;                                                           // ID
        a.name = f.getString(id, "name", id);                                // Name
        a.description = f.getString(id, "beschreibung", "");                 // Beschreibung
        a.model = f.getString(id, "modell", "");                             // Modell
        a.tier = clampValue(f.getInt(id, "stufe", 1), 1, 9);                 // Stufe
        a.value = std::max(1, f.getInt(id, "wert", 50));                     // Wert
        a.points = std::max(0, f.getInt(id, "punkte", 5));                   // Entdeckerpunkte
        artifacts.push_back(a);                                              // Speichern
    }                                                                        // Ende der Artefaktschleife
    if (explorerThresholds.empty()) explorerThresholds = {0, 30, 100, 250};  // Standardschwellen
    if (!f.load(ImageIO::joinPath(dataDir, "haendler.txt"))) { error = "data/haendler.txt fehlt"; return false; } // Händler
    for (const std::string& id : f.sectionNames()) {                         // Alle Händlerarten
        TraderDef t;                                                         // Neue Händlerart
        t.id = id;                                                           // ID
        t.name = f.getString(id, "name", id);                                // Name
        t.kind = f.getString(id, "art", "");                                 // Art
        t.model = f.getString(id, "modell", "marktstand");                   // Stand oder Laden
        t.figure = f.getString(id, "figur", "haendler");                     // Figur
        t.buys = f.getList(id, "kauft");                                     // Bedarf
        t.sells = f.getList(id, "verkauft");                                 // Angebot
        t.demandMax = std::max(1, f.getInt(id, "bedarf_max", 8));            // Höchster Bedarf
        t.demandMode = PropertyFile::toLower(f.getString(id, "bedarf_modus", "taeglich")); // Modus
        t.demandRate = f.getFloat(id, "bedarf_anstieg", 1.0f);               // Anstieg
        t.stockMax = std::max(0, f.getInt(id, "bestand_max", 6));            // Höchster Bestand
        t.stockRate = f.getFloat(id, "bestand_erholung", 0.5f);              // Erholung
        t.buyFactor = f.getFloat(id, "preisfaktor_ankauf", 1.35f);           // Ankaufspreis
        t.sellFactor = f.getFloat(id, "preisfaktor_verkauf", 1.2f);          // Verkaufspreis
        t.freshBonus = f.getFloat(id, "frische_bonus", 0.3f);                // Frischezuschlag
        t.deliveryBonus = f.getFloat(id, "lieferbonus", 0.3f);               // Lieferbonus
        traders.push_back(t);                                                // Speichern
    }                                                                        // Ende der Händlerschleife
    if (!f.load(ImageIO::joinPath(dataDir, "hersteller.txt"))) { error = "data/hersteller.txt fehlt"; return false; } // Hersteller
    for (const std::string& id : f.sectionNames()) {                         // Alle Hersteller
        ProducerDef p;                                                       // Neuer Hersteller
        p.id = id;                                                           // ID
        p.name = f.getString(id, "name", id);                                // Name
        p.figure = f.getString(id, "figur", "haendler");                     // Figur
        p.rawGoods = f.getList(id, "rohwaren");                              // Rohwaren
        p.productionHours = std::max(0.1f, f.getFloat(id, "produktionszeit", 3.0f)); // Produktionszeit
        p.storage = std::max(4, f.getInt(id, "lager", 30));                  // Lager
        p.rawMax = std::max(1, f.getInt(id, "rohwaren_max", 12));            // Höchstmenge je Rohware
        p.startStock = std::max(0, f.getInt(id, "start_bestand", 2));        // Startbestand
        p.buyFactor = f.getFloat(id, "preisfaktor_ankauf", 1.4f);            // Ankaufspreis
        p.sellFactor = f.getFloat(id, "preisfaktor_verkauf", 0.9f);          // Verkaufspreis
        for (const std::string& product : f.getList(id, "produkte")) {       // Alle Produkte
            ProducerDef::Recipe r;                                           // Neues Rezept
            r.product = product;                                             // Produkt
            for (const std::string& part : f.getList(id, "rezept_" + product)) { // "Ware:Menge"
                std::size_t colon = part.find(':');                          // Doppelpunkt
                if (colon == std::string::npos) continue;                    // Ungültig
                r.inputs.push_back({PropertyFile::trim(part.substr(0, colon)), std::max(1, std::atoi(part.c_str() + colon + 1))}); // Zutat speichern
            }                                                                // Ende der Zutaten
            p.recipes.push_back(r);                                          // Rezept speichern
        }                                                                    // Ende der Produkte
        producers.push_back(p);                                              // Speichern
    }                                                                        // Ende der Herstellerschleife
    if (goods.empty() || ships.empty()) { error = "Keine Waren oder Schiffe definiert"; return false; } // Mindestdaten prüfen
    return true;                                                             // Erfolgreich
} // Ende von load

const GoodDef* GameData::good(const std::string& id) const { return findById(goods, id); }              // Ware suchen
const ShipClass* GameData::ship(const std::string& id) const { return findById(ships, id); }            // Schiff suchen
const CrewRole* GameData::role(const std::string& id) const { return findById(roles, id); }             // Rolle suchen
const UpgradeDef* GameData::upgrade(const std::string& id) const { return findById(upgrades, id); }     // Upgrade suchen
const ArtifactDef* GameData::artifact(const std::string& id) const { return findById(artifacts, id); }  // Artefakt suchen
const TraderDef* GameData::trader(const std::string& id) const { return findById(traders, id); }        // Händler suchen
const ProducerDef* GameData::producer(const std::string& id) const { return findById(producers, id); }  // Hersteller suchen

// Anzeigename einer Kategorie
std::string GameData::categoryName(GoodCategory c) {                         // Beginn von categoryName
    switch (c) {                                                             // Je nach Kategorie
    case GoodCategory::Raw: return "Rohware";                                // Rohware
    case GoodCategory::Finished: return "Fertigware";                        // Fertigware
    case GoodCategory::Vegetable: return "Gemüse";                           // Gemüse
    case GoodCategory::Fruit: return "Obst";                                 // Obst
    case GoodCategory::Food: return "Lebensmittel";                          // Lebensmittel
    case GoodCategory::Luxury: return "Luxusware";                           // Luxus
    }                                                                        // Ende der Fallunterscheidung
    return "Ware";                                                           // Ersatz
} // Ende von categoryName
