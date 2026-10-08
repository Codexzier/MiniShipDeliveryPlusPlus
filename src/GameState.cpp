// GameState.cpp - Spiellogik ohne Darstellung: Zeit, Wetter, Preise, Handel, Crew, Laderaum, Aufträge, Artefakte, Speichern
#include "GameState.h" // Eigene Deklarationen

#include <algorithm> // std::sort, std::min, std::max, std::find
#include <cmath>     // std::floor, std::sqrt, std::fmod
#include <cstdint>   // std::uint32_t
#include <cstdio>    // std::snprintf
#include <cstdlib>   // std::atoi, std::strtof

#include "Common.h" // clampValue, lerp

namespace { // Interne Hilfen

// PI kommt aus Common.h

// Vergleicht zwei Texte ohne Groß-/Kleinschreibung
bool sameId(const std::string& a, const std::string& b) { return PropertyFile::toLower(a) == PropertyFile::toLower(b); } // Gleich?

// Ist ein Text in einer Liste (ohne Groß-/Kleinschreibung)?
bool contains(const std::vector<std::string>& list, const std::string& id) {   // Beginn von contains
    for (const std::string& s : list) if (sameId(s, id)) return true;          // Gefunden
    return false;                                                              // Nicht gefunden
} // Ende von contains

// Wandelt ein Kategoriewort in die Kategorie um
GoodCategory categoryOf(const std::string& kind) {                             // Beginn von categoryOf
    std::string k = PropertyFile::toLower(kind);                               // Klein geschrieben
    if (k == "gemuese") return GoodCategory::Vegetable;                        // Gemüse
    if (k == "obst") return GoodCategory::Fruit;                               // Obst
    if (k == "fertigware") return GoodCategory::Finished;                      // Fertigware
    if (k == "lebensmittel") return GoodCategory::Food;                        // Lebensmittel
    if (k == "luxus") return GoodCategory::Luxury;                             // Luxus
    return GoodCategory::Raw;                                                  // Rohware
} // Ende von categoryOf

// Teilt einen Text an einem Zeichen
std::vector<std::string> splitText(const std::string& text, char sep) {        // Beginn von splitText
    std::vector<std::string> out;                                              // Ergebnis
    std::string cur;                                                           // Aktueller Teil
    for (char ch : text) {                                                     // Alle Zeichen
        if (ch == sep) { out.push_back(PropertyFile::trim(cur)); cur.clear(); } // Trennzeichen -> Teil fertig
        else cur += ch;                                                        // Zeichen anhängen
    }                                                                          // Ende der Schleife
    out.push_back(PropertyFile::trim(cur));                                    // Letzter Teil
    return out;                                                                // Ergebnis
} // Ende von splitText

// Fügt Texte mit einem Trennzeichen zusammen
std::string joinText(const std::vector<std::string>& parts, const std::string& sep) { // Beginn von joinText
    std::string out;                                                           // Ergebnis
    for (std::size_t i = 0; i < parts.size(); ++i) { if (i) out += sep; out += parts[i]; } // Teile verbinden
    return out;                                                                // Ergebnis
} // Ende von joinText

// Kommazahl als Text (Punkt als Dezimaltrenner)
std::string num(float v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.3f", static_cast<double>(v)); return buf; } // Formatieren

// Kürzester Winkelabstand von a nach b
float angleDelta(float a, float b) {                                           // Beginn von angleDelta
    float d = std::fmod(b - a + PI * 3.0f, PI * 2.0f) - PI;                    // In den Bereich -PI..PI bringen
    return d;                                                                  // Ergebnis
} // Ende von angleDelta

} // Ende des internen Namensraums

// ============================================================ Einstellungen

// Liest alle Spielwerte aus data/spiel.txt
void Settings::load(const PropertyFile& f) {                                   // Beginn von load
    secondsPerHour = std::max(1.0f, f.getFloat("Zeit", "sekunden_pro_stunde", secondsPerHour)); // Zeitmaßstab
    startHour = f.getFloat("Zeit", "start_stunde", startHour);                 // Startzeit
    nightStart = f.getFloat("Zeit", "nacht_beginn", nightStart);               // Nachtbeginn
    nightEnd = f.getFloat("Zeit", "nacht_ende", nightEnd);                     // Nachtende
    timeInWindows = f.getBool("Zeit", "zeit_in_fenstern", timeInWindows);      // Zeit in Fenstern
    startCredits = f.getInt("Start", "credits", startCredits);                 // Startgeld
    startShip = f.getString("Start", "schiff", startShip);                     // Startschiff
    startCrew = f.getList("Start", "crew");                                    // Startcrew
    startHealth = f.getFloat("Start", "kapitaen_gesundheit", startHealth);     // Gesundheit
    walkSpeed = f.getFloat("Figur", "geschwindigkeit", walkSpeed);             // Lauftempo
    injuredBelow = f.getFloat("Figur", "verletzt_unter", injuredBelow);        // Verletzungsgrenze
    healthRegen = f.getFloat("Figur", "erholung", healthRegen);                // Erholung
    acceleration = f.getFloat("Schiff", "beschleunigung", acceleration);       // Beschleunigung
    reverseFactor = f.getFloat("Schiff", "rueckwaerts", reverseFactor);        // Rückwärts
    dockDistance = f.getFloat("Schiff", "anlege_abstand", dockDistance);       // Anlegeabstand
    dockSpeed = f.getFloat("Schiff", "anlege_tempo", dockSpeed);               // Anlegetempo
    groundDamage = f.getFloat("Schiff", "auflauf_schaden", groundDamage);      // Auflaufschaden
    understaffedMalus = f.getFloat("Schiff", "unterbesetzung_malus", understaffedMalus); // Unterbesetzung
    noEngineerPower = f.getFloat("Schiff", "ohne_maschinist_leistung", noEngineerPower); // Ohne Maschinist
    noEngineerFuel = f.getFloat("Schiff", "ohne_maschinist_verbrauch", noEngineerFuel);  // Mehrverbrauch
    stormFrom = f.getFloat("Schiff", "sturm_ab", stormFrom);                   // Sturmgrenze
    stormDamage = f.getFloat("Schiff", "sturm_schaden", stormDamage);          // Sturmschaden
    seaRepairMax = f.getFloat("Schiff", "see_reparatur_max", seaRepairMax);    // Reparatur auf See
    kontorBuy = f.getFloat("Kontor", "ankauf", kontorBuy);                     // Kontor-Ankauf
    kontorSell = f.getFloat("Kontor", "verkauf", kontorSell);                  // Kontor-Verkauf
    ownGood = f.getFloat("Kontor", "eigene_ware", ownGood);                    // Eigene Ware
    wantedGood = f.getFloat("Kontor", "gesuchte_ware", wantedGood);            // Gesuchte Ware
    priceSwing = f.getFloat("Kontor", "schwankung", priceSwing);               // Schwankung
    saturation = f.getFloat("Kontor", "saettigung", saturation);               // Sättigung
    saturationRecovery = f.getFloat("Kontor", "saettigung_erholung", saturationRecovery); // Erholung
    fuelPrice = f.getInt("Kontor", "treibstoff_preis", fuelPrice);             // Treibstoffpreis
    contractOffers = f.getInt("Auftraege", "angebote", contractOffers);        // Angebote
    maxContracts = f.getInt("Auftraege", "max_aktiv", maxContracts);           // Aktive Aufträge
    contractReward = f.getFloat("Auftraege", "belohnung_faktor", contractReward); // Belohnung
    contractDistanceBonus = f.getFloat("Auftraege", "entfernung_bonus", contractDistanceBonus); // Entfernungsbonus
    contractPenalty = f.getFloat("Auftraege", "strafe", contractPenalty);      // Strafe
    contractBuffer = f.getFloat("Auftraege", "frist_puffer", contractBuffer);  // Puffer
    repairPrice = f.getInt("Werft", "reparatur_preis", repairPrice);           // Reparaturpreis
    tradeIn = f.getFloat("Werft", "inzahlungnahme", tradeIn);                  // Inzahlungnahme
    restPrice = f.getInt("Taverne", "uebernachten_preis", restPrice);          // Übernachten
    wakeHour = f.getFloat("Taverne", "aufwach_stunde", wakeHour);              // Aufwachen
    applicants = f.getInt("Taverne", "bewerber", applicants);                  // Bewerber
    for (const char* type : {"kontor", "haendler", "hersteller", "museum", "werft"}) { // Öffnungszeiten
        std::vector<std::string> v = f.getList("Oeffnungszeiten", type);       // "von, bis"
        if (v.size() == 2) openHours[type] = {std::strtof(v[0].c_str(), nullptr), std::strtof(v[1].c_str(), nullptr)}; // Speichern
    }                                                                          // Ende der Öffnungszeiten
    pirateSpeed = f.getFloat("Piraten", "geschwindigkeit", pirateSpeed);       // Piratentempo
    pirateSight = f.getFloat("Piraten", "sichtweite", pirateSight);            // Sicht
    pirateSightNight = f.getFloat("Piraten", "sichtweite_nacht", pirateSightNight); // Sicht nachts
    pirateHp = f.getFloat("Piraten", "lebenspunkte", pirateHp);                // Lebenspunkte
    pirateDamage = f.getFloat("Piraten", "kanonen_schaden", pirateDamage);     // Schaden
    pirateRange = f.getFloat("Piraten", "kanonen_reichweite", pirateRange);    // Reichweite
    pirateReload = f.getFloat("Piraten", "nachladen", pirateReload);           // Nachladen
    pirateRouteChance = f.getFloat("Piraten", "angriff_auf_route", pirateRouteChance); // Angriff auf Routen
    pirateSteal = f.getFloat("Piraten", "raub_anteil", pirateSteal);           // Raubanteil
    std::vector<std::string> loot = f.getList("Piraten", "beute");             // Beute "von, bis"
    if (loot.size() == 2) { lootMin = std::atoi(loot[0].c_str()); lootMax = std::max(lootMin, std::atoi(loot[1].c_str())); } // Speichern
    monsterSpeed = f.getFloat("Monster", "geschwindigkeit", monsterSpeed);     // Monstertempo
    monsterDamage = f.getFloat("Monster", "schaden", monsterDamage);           // Bissschaden
    monsterBitePause = f.getFloat("Monster", "biss_pause", monsterBitePause);  // Bisspause
    monsterHp = f.getFloat("Monster", "lebenspunkte", monsterHp);              // Lebenspunkte
    monsterSight = f.getFloat("Monster", "sichtweite", monsterSight);          // Sicht
    cannonRange = f.getFloat("Spieler_Kanonen", "reichweite", cannonRange);    // Eigene Reichweite
    cannonReload = f.getFloat("Spieler_Kanonen", "nachladen", cannonReload);   // Eigenes Nachladen
    weatherSlot = std::max(1.0f, f.getFloat("Wetter", "abschnitt", weatherSlot)); // Wetterabschnitt
    windMean = f.getFloat("Wetter", "wind_mittel", windMean);                  // Mittlerer Wind
    windSwing = f.getFloat("Wetter", "wind_schwankung", windSwing);            // Schwankung
    windTurn = f.getFloat("Wetter", "wind_drehung", windTurn);                 // Drehung
    rainChance = f.getFloat("Wetter", "regen", rainChance);                    // Regen
    fogChance = f.getFloat("Wetter", "nebel", fogChance);                      // Nebel
    forecastError = f.getFloat("Wetter", "vorhersage_fehler", forecastError);  // Vorhersagefehler
    forecastErrorNavigator = f.getFloat("Wetter", "vorhersage_fehler_navigator", forecastErrorNavigator); // Mit Navigator
    stability = f.getFloat("Minispiel", "stabilitaet", stability);             // Stabilität
    loaderFee = f.getInt("Minispiel", "arbeiter_gebuehr", loaderFee);          // Arbeitergebühr
    capsizeDamage = f.getFloat("Minispiel", "kentern_schaden", capsizeDamage); // Kenterschaden
} // Ende von load

// ============================================================ Grundlagen

// Verbindet die festen Daten
void GameState::attach(const GameData* data, const World* world, const Settings* settings) { // Beginn von attach
    m_data = data;                                                             // Spieldaten
    m_world = world;                                                           // Welt
    m_set = settings;                                                          // Einstellungen
} // Ende von attach

// Zufallszahl 0..1 aus drei Werten (immer gleich für gleiche Eingaben)
float GameState::hash01(unsigned a, unsigned b, unsigned c) const {            // Beginn von hash01
    std::uint32_t h = seed * 374761393u + a * 668265263u + b * 2246822519u + c * 3266489917u; // Mischen
    h = (h ^ (h >> 15)) * 2246822519u;                                         // Bits verwirbeln
    h = (h ^ (h >> 13)) * 3266489917u;                                         // Weiter verwirbeln
    h ^= h >> 16;                                                              // Abschluss
    return static_cast<float>(h & 0xFFFFFF) / static_cast<float>(0x1000000);   // Auf 0..1 abbilden
} // Ende von hash01

// Legt ein neues Spiel an
void GameState::newGame(unsigned newSeed) {                                    // Beginn von newGame
    seed = newSeed;                                                            // Zufallsstart merken
    hours = m_set->startHour;                                                  // Startzeit (Tag 1)
    credits = m_set->startCredits;                                             // Startgeld
    explorerPoints = 0;                                                        // Keine Entdeckerpunkte
    health = m_set->startHealth;                                               // Gesund
    shipId = m_data->ship(m_set->startShip) ? m_data->ship(m_set->startShip)->id : m_data->ships.front().id; // Startschiff
    upgrades.clear();                                                          // Keine Upgrades
    upgradeSpent = 0;                                                          // Nichts ausgegeben
    crew.clear(); hold.clear(); piers.clear(); artifacts.clear(); floating.clear(); // Listen leeren
    offers.clear(); active.clear(); saturation.clear(); applicants.clear(); messages.clear(); m_weather.clear(); // Weitere Listen leeren
    totalEarned = deliveries = artifactsSold = piratesSunk = shipsLost = 0;    // Statistik zurücksetzen
    std::size_t nameIndex = 0;                                                 // Index für Crewnamen
    for (const std::string& role : m_set->startCrew) {                         // Startcrew anheuern
        const CrewRole* r = m_data->role(role);                                // Rolle suchen
        if (!r) continue;                                                      // Unbekannt
        CrewMember m;                                                          // Neues Mitglied
        m.role = r->id;                                                        // Rolle
        m.name = m_data->crewNames.empty() ? "Hein" : m_data->crewNames[nameIndex++ % m_data->crewNames.size()]; // Name
        m.wage = r->wage;                                                      // Lohn
        crew.push_back(m);                                                     // Speichern
    }                                                                          // Ende der Startcrew
    ShipStats s = stats();                                                     // Werte des Startschiffs
    hull = s.hullMax;                                                          // Voller Rumpf
    fuel = s.fuelMax;                                                          // Voller Tank
    setupTraders();                                                            // Händler und Hersteller
    int start = std::max(0, m_world->islandIndex(m_world->startIsland));       // Startinsel
    const Island& isl = m_world->islands[static_cast<std::size_t>(start)];     // Insel
    onFoot = true;                                                             // Kapitän steht an Land
    figX = isl.pierBaseX; figY = isl.pierBaseY;                                // Am Beginn des Stegs
    shipX = isl.dockX; shipY = isl.dockY; shipAngle = isl.dockAngle;           // Schiff am Liegeplatz
    docked = start;                                                            // Angelegt
    lastHarbor = start;                                                        // Letzter Hafen
    hasTarget = false;                                                         // Kein Ziel
    makeOffers();                                                              // Aufträge
    makeApplicants();                                                          // Bewerber
    for (int i = 0; i < m_data->artifactsOnMap; ++i) spawnArtifact(seed + static_cast<unsigned>(i) * 31u); // Artefakte verteilen
    addMessage("Willkommen in " + isl.name + "! Kaufe Waren günstig ein und verkaufe sie teurer auf anderen Inseln."); // Begrüßung
} // Ende von newGame

// ============================================================ Zeit

int GameState::day() const { return static_cast<int>(std::floor(hours / 24.0f)) + 1; } // Spieltag ab 1
float GameState::hourOfDay() const { return std::fmod(hours, 24.0f); }                 // Uhrzeit
bool GameState::isNight() const { float h = hourOfDay(); return h >= m_set->nightStart || h < m_set->nightEnd; } // Nacht?

// Helligkeit mit Dämmerung
float GameState::daylight() const {                                            // Beginn von daylight
    float h = hourOfDay();                                                     // Uhrzeit
    const float fade = 1.5f;                                                   // Dauer der Dämmerung in Stunden
    if (h >= m_set->nightEnd && h < m_set->nightEnd + fade) return (h - m_set->nightEnd) / fade; // Morgendämmerung
    if (h >= m_set->nightStart - fade && h < m_set->nightStart) return (m_set->nightStart - h) / fade; // Abenddämmerung
    return isNight() ? 0.0f : 1.0f;                                            // Tag oder Nacht
} // Ende von daylight

// Zeitpunkt als Text
std::string GameState::timeText(float t) {                                     // Beginn von timeText
    int d = static_cast<int>(std::floor(t / 24.0f)) + 1;                       // Tag
    float h = std::fmod(t, 24.0f);                                             // Stunde
    int hh = static_cast<int>(h);                                              // Volle Stunde
    int mm = static_cast<int>((h - static_cast<float>(hh)) * 60.0f);           // Minuten
    char buf[48];                                                              // Puffer
    std::snprintf(buf, sizeof(buf), "Tag %d, %02d:%02d", d, hh, mm);           // Formatieren
    return buf;                                                                // Ergebnis
} // Ende von timeText

std::string GameState::clockText() const { return timeText(hours); }           // Aktuelle Zeit als Text

// Hat ein Gebäude gerade geöffnet?
bool GameState::isOpen(const std::string& type) const {                        // Beginn von isOpen
    auto it = m_set->openHours.find(type);                                     // Öffnungszeiten suchen
    if (it == m_set->openHours.end()) return true;                             // Keine Angabe: immer offen
    float h = hourOfDay();                                                     // Uhrzeit
    return h >= it->second.first && h < it->second.second;                     // Innerhalb?
} // Ende von isOpen

// Öffnungszeiten als Text
std::string GameState::openText(const std::string& type) const {               // Beginn von openText
    auto it = m_set->openHours.find(type);                                     // Öffnungszeiten suchen
    if (it == m_set->openHours.end()) return "Immer geöffnet";                 // Keine Angabe
    return "Geöffnet " + std::to_string(static_cast<int>(it->second.first)) + " - " + std::to_string(static_cast<int>(it->second.second)) + " Uhr"; // Text
} // Ende von openText

// Spult die Zeit vor und löst stündliche Ereignisse aus
void GameState::advance(float gameHours) {                                     // Beginn von advance
    if (gameHours <= 0.0f) return;                                             // Nichts zu tun
    float before = hours;                                                      // Alte Zeit
    hours += gameHours;                                                        // Neue Zeit
    int first = static_cast<int>(std::floor(before)) + 1;                      // Erste neue volle Stunde
    int last = static_cast<int>(std::floor(hours));                            // Letzte volle Stunde
    for (int h = first; h <= last; ++h) hourTick(h);                           // Jede Stunde abarbeiten
} // Ende von advance

// Stündliche Ereignisse
void GameState::hourTick(int absoluteHour) {                                   // Beginn von hourTick
    for (TraderState& t : traders) {                                           // Alle Händler
        const TraderDef* d = m_data->trader(t.def);                            // Definition
        if (!d) continue;                                                      // Unbekannt
        if (d->demandMode == "steigend") for (auto& kv : t.demand) kv.second = std::min(static_cast<float>(d->demandMax), kv.second + d->demandRate); // Bedarf wächst
        for (auto& kv : t.stock) kv.second = std::min(static_cast<float>(d->stockMax), kv.second + d->stockRate); // Bestand erholt sich
    }                                                                          // Ende der Händler
    for (ProducerState& p : producers) {                                       // Alle Hersteller
        const ProducerDef* d = m_data->producer(p.def);                        // Definition
        if (!d || d->recipes.empty()) continue;                                // Unbekannt oder ohne Rezept
        p.progress += 1.0f;                                                    // Eine Stunde Arbeit
        while (p.progress >= d->productionHours) {                             // Ein Produkt fertig?
            bool made = false;                                                 // Wurde etwas hergestellt?
            for (std::size_t k = 0; k < d->recipes.size() && !made; ++k) {     // Rezepte der Reihe nach probieren
                const ProducerDef::Recipe& r = d->recipes[(static_cast<std::size_t>(p.nextRecipe) + k) % d->recipes.size()]; // Rezept
                bool ok = true;                                                // Alle Zutaten da?
                for (const auto& in : r.inputs) if (p.stock[in.first] < static_cast<float>(in.second)) ok = false; // Zutat fehlt
                if (!ok) continue;                                             // Nächstes Rezept
                for (const auto& in : r.inputs) p.stock[in.first] -= static_cast<float>(in.second); // Zutaten verbrauchen
                p.stock[r.product] += 1.0f;                                    // Produkt einlagern
                p.nextRecipe = static_cast<int>((static_cast<std::size_t>(p.nextRecipe) + k + 1) % d->recipes.size()); // Nächstes Mal anderes Rezept
                made = true;                                                   // Erfolgreich
            }                                                                  // Ende der Rezepte
            if (!made) { p.progress = d->productionHours; break; }             // Warten auf Rohwaren
            p.progress -= d->productionHours;                                  // Zeit verbrauchen
        }                                                                      // Ende der Produktion
    }                                                                          // Ende der Hersteller
    for (std::size_t i = 0; i < active.size();) {                              // Fristen prüfen
        if (hours > active[i].deadline) {                                      // Frist verpasst
            int penalty = static_cast<int>(static_cast<float>(active[i].reward) * m_set->contractPenalty); // Strafe
            credits = std::max(0, credits - penalty);                          // Abziehen
            const GoodDef* g = m_data->good(active[i].good);                   // Ware
            addMessage("Frist verpasst: " + std::to_string(active[i].amount) + " " + (g ? g->name : active[i].good) + " nach " + m_world->islands[static_cast<std::size_t>(active[i].to)].name + ". Strafe: " + std::to_string(penalty) + " Cr"); // Meldung
            active.erase(active.begin() + static_cast<long>(i));               // Auftrag entfernen
        } else ++i;                                                            // Nächster Auftrag
    }                                                                          // Ende der Fristen
    health = std::min(100.0f, health + m_set->healthRegen);                    // Kapitän erholt sich
    ShipStats s = stats();                                                     // Schiffswerte
    if (hull < s.hullMax * m_set->seaRepairMax) hull = std::min(s.hullMax * m_set->seaRepairMax, hull + s.repair); // Crew flickt das Schiff
    if (absoluteHour % 24 == 6) dayTick();                                     // Jeden Morgen um 6 Uhr
} // Ende von hourTick

// Tägliche Ereignisse um 6 Uhr
void GameState::dayTick() {                                                    // Beginn von dayTick
    int wages = dailyWages();                                                  // Löhne
    while (!crew.empty() && credits < wages) {                                 // Nicht genug Geld?
        addMessage(crew.back().name + " (" + (m_data->role(crew.back().role) ? m_data->role(crew.back().role)->name : crew.back().role) + ") hat abgemustert: keine Heuer."); // Meldung
        crew.pop_back();                                                       // Mitglied geht
        wages = dailyWages();                                                  // Neu berechnen
    }                                                                          // Ende der Schleife
    credits -= wages;                                                          // Löhne zahlen
    if (wages > 0) addMessage("Heuer bezahlt: " + std::to_string(wages) + " Cr"); // Meldung
    for (auto it = saturation.begin(); it != saturation.end();) {              // Preise erholen sich
        it->second -= m_set->saturationRecovery;                               // Weniger Sättigung
        if (it->second <= 0.0f) it = saturation.erase(it); else ++it;          // Ganz erholt -> entfernen
    }                                                                          // Ende der Schleife
    for (TraderState& t : traders) {                                           // Täglicher Bedarf
        const TraderDef* d = m_data->trader(t.def);                            // Definition
        if (d && d->demandMode != "steigend") for (auto& kv : t.demand) kv.second = static_cast<float>(d->demandMax); // Bedarf voll
    }                                                                          // Ende der Händler
    makeOffers();                                                              // Neue Aufträge
    makeApplicants();                                                          // Neue Bewerber
    for (int k = 0; k < m_data->artifactsPerDay; ++k) {                        // Neue Artefakte
        if (static_cast<int>(floating.size()) >= m_data->artifactsOnMap) break; // Genug auf der Karte
        spawnArtifact(seed * 13u + static_cast<unsigned>(day()) * 101u + static_cast<unsigned>(k)); // Neues Artefakt
    }                                                                          // Ende der Artefakte
    WeatherSlot w = forecast(hours + 6.0f);                                    // Vorhersage für den Tag
    addMessage("Hafenzeitung Tag " + std::to_string(day()) + ": " + weatherName(w.type) + ", " + windName(w.windStrength) + " aus " + directionName(w.windAngle) + ". Neue Aufträge im Kontor."); // Zeitung
} // Ende von dayTick

// ============================================================ Wetter

// Wetter eines Abschnitts (wird der Reihe nach erzeugt und zwischengespeichert)
WeatherSlot GameState::slot(int index) const {                                 // Beginn von slot
    if (index < 0) index = 0;                                                  // Nicht vor Spielbeginn
    while (static_cast<int>(m_weather.size()) <= index) {                      // Fehlende Abschnitte erzeugen
        unsigned i = static_cast<unsigned>(m_weather.size());                  // Nummer des neuen Abschnitts
        WeatherSlot w;                                                         // Neuer Abschnitt
        if (m_weather.empty()) {                                               // Erster Abschnitt
            w.windAngle = hash01(i, 1, 7) * 2.0f * PI;                         // Zufällige Richtung
            w.windStrength = m_set->windMean;                                  // Mittlerer Wind
        } else {                                                               // Folgeabschnitt
            const WeatherSlot& p = m_weather.back();                           // Vorheriger Abschnitt
            w.windAngle = p.windAngle + (hash01(i, 2, 7) - 0.5f) * 2.0f * m_set->windTurn * PI / 180.0f; // Richtung dreht langsam
            w.windStrength = clampValue(p.windStrength + (hash01(i, 3, 7) - 0.5f) * 2.0f * m_set->windSwing + (m_set->windMean - p.windStrength) * 0.3f, 0.03f, 1.0f); // Stärke schwankt um den Mittelwert
        }                                                                      // Ende der Unterscheidung
        float r = hash01(i, 4, 7);                                             // Würfel für die Wetterart
        if (w.windStrength > 0.82f) w.type = WeatherType::Storm;               // Starker Wind = Sturm
        else if (r < m_set->fogChance && w.windStrength < 0.35f) w.type = WeatherType::Fog; // Nebel bei wenig Wind
        else if (r < m_set->fogChance + m_set->rainChance) w.type = WeatherType::Rain; // Regen
        else if (r < 0.55f) w.type = WeatherType::Cloudy;                      // Bewölkt
        else w.type = WeatherType::Clear;                                      // Klar
        m_weather.push_back(w);                                                // Speichern
    }                                                                          // Ende der Erzeugung
    return m_weather[static_cast<std::size_t>(index)];                         // Abschnitt zurückgeben
} // Ende von slot

// Wetter zu einem Zeitpunkt (Wind weich überblendet)
WeatherSlot GameState::weatherAt(float t) const {                              // Beginn von weatherAt
    float pos = t / m_set->weatherSlot;                                        // Position in Abschnitten
    int i = static_cast<int>(std::floor(pos));                                 // Aktueller Abschnitt
    float f = pos - static_cast<float>(i);                                     // Anteil im Abschnitt
    WeatherSlot a = slot(i), b = slot(i + 1);                                  // Dieser und nächster Abschnitt
    WeatherSlot w = a;                                                         // Ergebnis
    float blend = clampValue((f - 0.6f) / 0.4f, 0.0f, 1.0f);                   // Erst am Ende überblenden
    w.windAngle = a.windAngle + angleDelta(a.windAngle, b.windAngle) * blend;  // Richtung überblenden
    w.windStrength = lerp(a.windStrength, b.windStrength, blend);              // Stärke überblenden
    return w;                                                                  // Ergebnis
} // Ende von weatherAt

// Vorhersage mit Ungenauigkeit (je weiter in der Zukunft, desto ungenauer)
WeatherSlot GameState::forecast(float t) const {                               // Beginn von forecast
    WeatherSlot w = weatherAt(t);                                              // Echtes Wetter
    float err = hasRole("Navigator") ? m_set->forecastErrorNavigator : m_set->forecastError; // Grundfehler
    err *= clampValue((t - hours) / 24.0f + 0.3f, 0.0f, 1.5f);                 // Weiter in der Zukunft = ungenauer
    unsigned i = static_cast<unsigned>(std::max(0.0f, t / m_set->weatherSlot)); // Abschnittsnummer für festen Zufall
    w.windStrength = clampValue(w.windStrength + (hash01(i, 5, 9) - 0.5f) * 2.0f * err, 0.0f, 1.0f); // Stärke verfälschen
    w.windAngle += (hash01(i, 6, 9) - 0.5f) * 2.0f * err * PI;                 // Richtung verfälschen
    if (hash01(i, 7, 9) < err) w.type = w.type == WeatherType::Clear ? WeatherType::Cloudy : (w.type == WeatherType::Cloudy ? WeatherType::Clear : w.type); // Manchmal falsche Wetterart
    return w;                                                                  // Ergebnis
} // Ende von forecast

// Name einer Wetterart
std::string GameState::weatherName(WeatherType t) {                            // Beginn von weatherName
    switch (t) {                                                               // Je nach Art
    case WeatherType::Clear: return "Klar";                                    // Klar
    case WeatherType::Cloudy: return "Bewölkt";                                // Bewölkt
    case WeatherType::Rain: return "Regen";                                    // Regen
    case WeatherType::Storm: return "Sturm";                                   // Sturm
    case WeatherType::Fog: return "Nebel";                                     // Nebel
    }                                                                          // Ende der Fallunterscheidung
    return "?";                                                                // Ersatz
} // Ende von weatherName

// Name einer Windstärke (Beaufort-ähnlich)
std::string GameState::windName(float s) {                                     // Beginn von windName
    int bft = static_cast<int>(s * 10.0f + 0.5f);                              // Beaufort 0..10
    std::string name = bft <= 1 ? "Flaute" : (bft <= 2 ? "leichte Brise" : (bft <= 4 ? "mäßiger Wind" : (bft <= 6 ? "frischer Wind" : (bft <= 7 ? "starker Wind" : "Sturm")))); // Name
    return name + " (" + std::to_string(bft) + " Bft)";                       // Mit Zahl
} // Ende von windName

// Himmelsrichtung, aus der der Wind kommt
std::string GameState::directionName(float angle) {                            // Beginn von directionName
    float from = angle + PI;                                                   // Herkunft = Gegenrichtung
    float deg = std::fmod(from * 180.0f / PI + 720.0f, 360.0f);                // In Grad 0..360
    const char* names[8] = {"Ost", "Südost", "Süd", "Südwest", "West", "Nordwest", "Nord", "Nordost"}; // x = Osten, y = Süden
    return names[static_cast<int>((deg + 22.5f) / 45.0f) % 8];                 // Nächste Richtung
} // Ende von directionName

// ============================================================ Schiff

// Aktuelle Schiffsklasse
const ShipClass& GameState::shipClass() const {                                // Beginn von shipClass
    const ShipClass* c = m_data->ship(shipId);                                 // Suchen
    return c ? *c : m_data->ships.front();                                     // Ersatz: erste Klasse
} // Ende von shipClass

int GameState::upgradeLevel(const std::string& id) const { auto it = upgrades.find(id); return it == upgrades.end() ? 0 : it->second; } // Stufe

// Preis der nächsten Stufe (verdoppelt sich je Stufe)
int GameState::upgradePrice(const std::string& id) const {                     // Beginn von upgradePrice
    const UpgradeDef* u = m_data->upgrade(id);                                 // Definition
    if (!u) return 0;                                                          // Unbekannt
    return u->price * (1 << upgradeLevel(id));                                 // Preis * 2^Stufe
} // Ende von upgradePrice

int GameState::shipValue() const { return static_cast<int>(static_cast<float>(shipClass().price) * m_set->tradeIn + static_cast<float>(upgradeSpent) * 0.5f); } // Schiffswert

bool GameState::hasRole(const std::string& role) const { for (const CrewMember& m : crew) if (sameId(m.role, role)) return true; return false; } // Rolle an Bord?

// Berechnet die aktuellen Schiffswerte aus Klasse, Verbesserungen und Crew
ShipStats GameState::stats() const {                                           // Beginn von stats
    const ShipClass& c = shipClass();                                          // Schiffsklasse
    ShipStats s;                                                               // Ergebnis
    s.speed = c.speed; s.turn = c.turn; s.repair = c.repair; s.armor = c.armor; s.defense = c.defense; // Grundwerte übernehmen
    s.scan = c.scan; s.trade = c.trade; s.hullMax = c.hull; s.fuelMax = c.fuelMax; s.fuelUse = c.fuelUse; // Weitere Grundwerte
    s.draft = c.draft; s.holdW = c.holdWidth; s.holdH = c.holdHeight; s.stability = m_set->stability; // Laderaum und Stabilität
    s.sails = c.drive != "motor";                                              // Segel vorhanden?
    bool motor = c.drive == "motor" || c.drive == "hybrid";                    // Maschine vorhanden?
    for (const auto& kv : upgrades) {                                          // Alle Verbesserungen
        const UpgradeDef* u = m_data->upgrade(kv.first);                       // Definition
        if (!u) continue;                                                      // Unbekannt
        float v = u->value * static_cast<float>(kv.second);                    // Zuwachs
        const std::string& a = u->attribute;                                   // Attribut
        if (a == "laderaum_breite") s.holdW += static_cast<int>(v);            // Laderaum breiter
        else if (a == "laderaum_hoehe") s.holdH += static_cast<int>(v);        // Laderaum höher
        else if (a == "geschwindigkeit") s.speed += v;                         // Schneller
        else if (a == "reparatur") s.repair += v;                              // Reparatur
        else if (a == "panzerung") s.armor += v;                               // Panzerung
        else if (a == "abwehr") s.defense += v;                                // Abwehr
        else if (a == "wendigkeit") s.turn += v;                               // Wendigkeit
        else if (a == "sichtweite") s.scan += v;                               // Sichtweite
        else if (a == "rumpf") s.hullMax += v;                                 // Rumpf
        else if (a == "treibstoff_max" && motor) s.fuelMax += v;               // Tank (nur mit Maschine)
        else if (a == "stabilitaet") s.stability += v;                         // Stabilität
    }                                                                          // Ende der Verbesserungen
    s.missingCrew = std::max(0, c.minCrew - static_cast<int>(crew.size()));    // Fehlende Besatzung
    std::map<std::string, float> pct;                                          // Summe der Prozentwerte je Attribut
    for (const CrewMember& m : crew) {                                         // Alle Crewmitglieder
        const CrewRole* r = m_data->role(m.role);                              // Rolle
        if (!r) continue;                                                      // Unbekannt
        for (const auto& kv : r->bonus) pct[kv.first] += kv.second;            // Bonus addieren
        if (s.missingCrew > 0) for (const auto& kv : r->malus) pct[kv.first] -= kv.second; // Malus nur bei Unterbesetzung
    }                                                                          // Ende der Crew
    float under = static_cast<float>(s.missingCrew) * m_set->understaffedMalus; // Abzug durch Unterbesetzung
    s.speed *= std::max(0.3f, 1.0f + (pct["geschwindigkeit"] - under) / 100.0f); // Geschwindigkeit
    s.turn *= std::max(0.3f, 1.0f + (pct["wendigkeit"] - under) / 100.0f);    // Wendigkeit
    s.repair *= std::max(0.0f, 1.0f + pct["reparatur"] / 100.0f);              // Reparatur
    s.armor = std::min(80.0f, s.armor * (1.0f + pct["panzerung"] / 100.0f));   // Panzerung (höchstens 80 %)
    s.defense *= std::max(0.0f, 1.0f + pct["abwehr"] / 100.0f);                // Abwehr
    s.scan *= std::max(0.3f, 1.0f + pct["sichtweite"] / 100.0f);               // Sichtweite
    s.trade += pct["handel"];                                                  // Handel (Prozentpunkte)
    s.engineer = hasRole("Maschinist");                                        // Maschinist an Bord?
    s.navigator = hasRole("Navigator");                                        // Navigator an Bord?
    s.motorPower = motor ? (s.engineer ? 1.0f : m_set->noEngineerPower) : 0.0f; // Maschinenleistung
    s.fuelUse *= (s.engineer ? 1.0f : m_set->noEngineerFuel) * std::max(0.2f, 1.0f + pct["verbrauch"] / 100.0f); // Verbrauch
    s.perishFactor = std::max(0.2f, 1.0f - pct["verderb"] / 100.0f);          // Haltbarkeit (negativer Wert = länger)
    return s;                                                                  // Ergebnis
} // Ende von stats

// ============================================================ Laderaum

// Felder, die eine Einheit an einer Stelle belegt
std::vector<std::pair<int, int>> GameState::cells(const Cargo& c, int x, int y, int rot) const { // Beginn von cells
    std::vector<std::pair<int, int>> out;                                      // Ergebnis
    const GoodDef* g = m_data->good(c.good);                                   // Ware
    CargoShape shape = g ? g->shape : parseShape("1x1");                       // Form
    for (int r = 0; r < ((rot % 4) + 4) % 4; ++r) shape = shape.rotated();     // Drehen
    for (const auto& cell : shape.cells) out.push_back({x + cell.first, y + cell.second}); // Verschieben
    return out;                                                                // Ergebnis
} // Ende von cells

// Passt eine Einheit an diese Stelle? (ignore = Index im Laderaum, der nicht zählt)
bool GameState::fits(const Cargo& c, int x, int y, int rot, int ignore) const { // Beginn von fits
    ShipStats s = stats();                                                     // Laderaumgröße
    std::vector<std::pair<int, int>> mine = cells(c, x, y, rot);               // Gewünschte Felder
    for (const auto& p : mine) if (p.first < 0 || p.second < 0 || p.first >= s.holdW || p.second >= s.holdH) return false; // Außerhalb
    for (std::size_t i = 0; i < hold.size(); ++i) {                            // Alle geladenen Einheiten
        if (static_cast<int>(i) == ignore || hold[i].x < 0) continue;          // Ignorieren
        for (const auto& q : cells(hold[i], hold[i].x, hold[i].y, hold[i].rot)) for (const auto& p : mine) if (p == q) return false; // Überschneidung
    }                                                                          // Ende der Einheiten
    return true;                                                               // Passt
} // Ende von fits

// Belegte Felder im Laderaum
int GameState::holdUsedCells() const {                                         // Beginn von holdUsedCells
    int n = 0;                                                                 // Zähler
    for (const Cargo& c : hold) if (c.x >= 0) n += static_cast<int>(cells(c, c.x, c.y, c.rot).size()); // Felder zählen
    return n;                                                                  // Ergebnis
} // Ende von holdUsedCells

// Frische einer Einheit
float GameState::freshness(const Cargo& c) const {                             // Beginn von freshness
    const GoodDef* g = m_data->good(c.good);                                   // Ware
    if (!g || g->perishDays <= 0) return 1.0f;                                 // Hält ewig
    float life = static_cast<float>(g->perishDays) * 24.0f * stats().perishFactor; // Haltbarkeit in Stunden
    return clampValue(1.0f - (hours - c.born) / life, 0.0f, 1.0f);             // 1 = frisch, 0 = verdorben
} // Ende von freshness

bool GameState::rotten(const Cargo& c) const { const GoodDef* g = m_data->good(c.good); return g && g->perishDays > 0 && freshness(c) <= 0.0f; } // Verdorben?

std::vector<Cargo>& GameState::pier(int island) { return piers[island]; }     // Steglager (wird bei Bedarf angelegt)

// Verkaufbare Einheiten einer Ware (Steg + Laderaum, wenn das Schiff hier liegt)
int GameState::available(int island, const std::string& good) const {          // Beginn von available
    int n = 0;                                                                 // Zähler
    auto it = piers.find(island);                                              // Steglager
    if (it != piers.end()) for (const Cargo& c : it->second) if (sameId(c.good, good) && !rotten(c)) ++n; // Am Steg
    if (docked == island) for (const Cargo& c : hold) if (sameId(c.good, good) && !rotten(c)) ++n; // Im Laderaum
    return n;                                                                  // Ergebnis
} // Ende von available

// Entnimmt Einheiten (zuerst vom Steg, dann aus dem Laderaum, jeweils die ältesten)
std::vector<Cargo> GameState::take(int island, const std::string& good, int amount) { // Beginn von take
    std::vector<Cargo> out;                                                    // Entnommene Einheiten
    auto takeFrom = [&](std::vector<Cargo>& list) {                            // Hilfsfunktion für eine Liste
        while (static_cast<int>(out.size()) < amount) {                        // Noch mehr gebraucht?
            int best = -1;                                                     // Älteste passende Einheit
            for (std::size_t i = 0; i < list.size(); ++i) {                    // Alle Einheiten
                if (!sameId(list[i].good, good) || rotten(list[i])) continue;  // Falsche Ware oder verdorben
                if (best < 0 || list[i].born < list[static_cast<std::size_t>(best)].born) best = static_cast<int>(i); // Älter
            }                                                                  // Ende der Suche
            if (best < 0) break;                                               // Keine mehr
            out.push_back(list[static_cast<std::size_t>(best)]);               // Merken
            list.erase(list.begin() + best);                                   // Entfernen
        }                                                                      // Ende der Schleife
    };                                                                         // Ende der Hilfsfunktion
    takeFrom(pier(island));                                                    // Erst vom Steg
    if (docked == island) takeFrom(hold);                                      // Dann aus dem Laderaum
    return out;                                                                // Ergebnis
} // Ende von take

// Wirft verdorbene Ware weg (Steg und Laderaum)
int GameState::discardRotten(int island) {                                     // Beginn von discardRotten
    int n = 0;                                                                 // Zähler
    auto clean = [&](std::vector<Cargo>& list) {                               // Hilfsfunktion
        for (std::size_t i = 0; i < list.size();) { if (rotten(list[i])) { list.erase(list.begin() + static_cast<long>(i)); ++n; } else ++i; } // Verdorbenes entfernen
    };                                                                         // Ende der Hilfsfunktion
    clean(pier(island));                                                       // Steg
    if (docked == island || island < 0) clean(hold);                           // Laderaum
    return n;                                                                  // Anzahl
} // Ende von discardRotten

// Schlagseite in Grad (Gewicht links/rechts der Mittellinie)
float GameState::holdHeel() const {                                            // Beginn von holdHeel
    ShipStats s = stats();                                                     // Laderaumgröße
    float m = 0.0f;                                                            // Drehmoment
    for (const Cargo& c : hold) {                                              // Alle Einheiten
        if (c.x < 0) continue;                                                 // Nicht geladen
        const GoodDef* g = m_data->good(c.good);                               // Ware
        std::vector<std::pair<int, int>> cs = cells(c, c.x, c.y, c.rot);       // Felder
        float w = static_cast<float>(g ? g->weight : 1) / static_cast<float>(cs.size()); // Gewicht je Feld
        for (const auto& p : cs) m += w * (static_cast<float>(p.second) + 0.5f - static_cast<float>(s.holdH) * 0.5f); // Abstand zur Mittellinie
    }                                                                          // Ende der Einheiten
    return m * 6.0f / std::sqrt(static_cast<float>(s.holdW * s.holdH));        // Größere Schiffe liegen ruhiger
} // Ende von holdHeel

// Trimm in Grad (Gewicht vorne/hinten)
float GameState::holdTrim() const {                                            // Beginn von holdTrim
    ShipStats s = stats();                                                     // Laderaumgröße
    float m = 0.0f;                                                            // Drehmoment
    for (const Cargo& c : hold) {                                              // Alle Einheiten
        if (c.x < 0) continue;                                                 // Nicht geladen
        const GoodDef* g = m_data->good(c.good);                               // Ware
        std::vector<std::pair<int, int>> cs = cells(c, c.x, c.y, c.rot);       // Felder
        float w = static_cast<float>(g ? g->weight : 1) / static_cast<float>(cs.size()); // Gewicht je Feld
        for (const auto& p : cs) m += w * (static_cast<float>(p.first) + 0.5f - static_cast<float>(s.holdW) * 0.5f); // Abstand zur Schiffsmitte
    }                                                                          // Ende der Einheiten
    return m * 3.0f / std::sqrt(static_cast<float>(s.holdW * s.holdH));        // Trimm wirkt schwächer
} // Ende von holdTrim

// Kenterrisiko: 1 oder mehr = das Schiff kentert
float GameState::capsizeRisk() const {                                         // Beginn von capsizeRisk
    float st = stats().stability;                                              // Erlaubte Schlagseite
    return std::max(std::fabs(holdHeel()) / st, std::fabs(holdTrim()) / (st * 2.0f)); // Größerer Anteil zählt
} // Ende von capsizeRisk

// Hafenarbeiter packen den Laderaum möglichst voll und ausgeglichen
void GameState::autoLoad(int island) {                                         // Beginn von autoLoad
    ShipStats s = stats();                                                     // Laderaumgröße
    std::vector<Cargo>& p = pier(island);                                      // Steglager
    std::vector<std::size_t> order;                                            // Reihenfolge (große zuerst)
    for (std::size_t i = 0; i < p.size(); ++i) if (!rotten(p[i])) order.push_back(i); // Nur gute Ware
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {  // Sortieren
        std::size_t ca = cells(p[a], 0, 0, 0).size(), cb = cells(p[b], 0, 0, 0).size(); // Größe
        if (ca != cb) return ca > cb;                                          // Große zuerst
        const GoodDef* ga = m_data->good(p[a].good); const GoodDef* gb = m_data->good(p[b].good); // Waren
        return (ga ? ga->weight : 1) > (gb ? gb->weight : 1);                  // Schwere zuerst
    });                                                                        // Ende der Sortierung
    std::vector<bool> moved(p.size(), false);                                  // Welche Einheiten wurden geladen?
    int placed = 0;                                                            // Zähler
    for (std::size_t idx : order) {                                            // Alle Kandidaten
        if (credits < m_set->loaderFee) { addMessage("Kein Geld mehr für die Hafenarbeiter."); break; } // Gebühr nicht bezahlbar
        Cargo c = p[idx];                                                      // Kopie der Einheit
        float bestScore = 1e9f;                                                // Beste Bewertung
        int bx = -1, by = -1, br = 0;                                          // Beste Stelle
        for (int rot = 0; rot < 4; ++rot) for (int y = 0; y < s.holdH; ++y) for (int x = 0; x < s.holdW; ++x) { // Alle Stellen probieren
            if (!fits(c, x, y, rot, -1)) continue;                             // Passt nicht
            c.x = x; c.y = y; c.rot = rot;                                     // Probeweise setzen
            hold.push_back(c);                                                 // Einfügen
            float score = std::fabs(holdHeel()) + std::fabs(holdTrim()) * 0.5f + static_cast<float>(x) * 0.01f; // Ausgeglichen und kompakt
            hold.pop_back();                                                   // Wieder entfernen
            if (score < bestScore) { bestScore = score; bx = x; by = y; br = rot; } // Beste Stelle merken
        }                                                                      // Ende der Stellen
        if (bx < 0) continue;                                                  // Kein Platz
        c.x = bx; c.y = by; c.rot = br;                                        // Beste Stelle
        hold.push_back(c);                                                     // Einladen
        moved[idx] = true;                                                     // Markieren
        credits -= m_set->loaderFee;                                           // Gebühr zahlen
        ++placed;                                                              // Zählen
    }                                                                          // Ende der Kandidaten
    for (std::size_t i = p.size(); i-- > 0;) if (moved[i]) p.erase(p.begin() + static_cast<long>(i)); // Geladene vom Steg entfernen
    addMessage("Hafenarbeiter haben " + std::to_string(placed) + " Einheiten verladen (" + std::to_string(placed * m_set->loaderFee) + " Cr)."); // Meldung
} // Ende von autoLoad

// Lädt alles aus dem Laderaum an den Steg
void GameState::unloadAll(int island) {                                        // Beginn von unloadAll
    for (Cargo c : hold) { c.x = c.y = -1; c.rot = 0; pier(island).push_back(c); } // Alle Einheiten an den Steg
    hold.clear();                                                              // Laderaum leer
} // Ende von unloadAll

// ============================================================ Preise

// Mittlerer Inselpreis einer Ware (Grundpreis x Inselfaktor x Tagesschwankung x Sättigung)
float GameState::islandPrice(int island, const std::string& good) const {      // Beginn von islandPrice
    const GoodDef* g = m_data->good(good);                                     // Ware
    if (!g || island < 0 || island >= static_cast<int>(m_world->islands.size())) return 1.0f; // Ungültig
    const Island& isl = m_world->islands[static_cast<std::size_t>(island)];    // Insel
    float f = 1.0f;                                                            // Inselfaktor
    if (contains(isl.produces, g->id)) f = m_set->ownGood;                     // Eigene Ware: billig
    else if (contains(isl.demands, g->id)) f = m_set->wantedGood;              // Gesuchte Ware: teuer
    unsigned gi = static_cast<unsigned>(g - &m_data->goods[0]);                // Nummer der Ware
    float swing = 1.0f + (hash01(static_cast<unsigned>(day()), static_cast<unsigned>(island) * 131u + gi, 3) * 2.0f - 1.0f) * m_set->priceSwing; // Tagesschwankung
    auto it = saturation.find(std::to_string(island) + "|" + g->id);           // Sättigung
    float sat = it == saturation.end() ? 0.0f : it->second;                    // Wert
    return static_cast<float>(g->basePrice) * f * swing * std::max(0.4f, 1.0f - sat); // Preis
} // Ende von islandPrice

bool GameState::kontorSells(int island, const std::string& good) const { return island >= 0 && contains(m_world->islands[static_cast<std::size_t>(island)].produces, good); } // Eigene Ware?

int GameState::kontorBuyPrice(int island, const std::string& good) const { return std::max(1, static_cast<int>(islandPrice(island, good) * m_set->kontorSell * 100.0f / stats().trade + 0.5f)); } // Spieler kauft
int GameState::kontorSellPrice(int island, const std::string& good) const { return std::max(1, static_cast<int>(islandPrice(island, good) * m_set->kontorBuy * stats().trade / 100.0f + 0.5f)); } // Spieler verkauft

// Spieler kauft beim Händler
int GameState::traderBuyPrice(const TraderState& t, const std::string& good) const { // Beginn von traderBuyPrice
    const TraderDef* d = m_data->trader(t.def);                                // Definition
    int island = m_world->buildings[static_cast<std::size_t>(t.building)].island; // Insel
    return std::max(1, static_cast<int>(islandPrice(island, good) * (d ? d->sellFactor : 1.0f) * 100.0f / stats().trade + 0.5f)); // Preis
} // Ende von traderBuyPrice

// Spieler verkauft an den Händler (frische Ware bringt einen Zuschlag)
int GameState::traderSellPrice(const TraderState& t, const std::string& good, float fresh) const { // Beginn von traderSellPrice
    const TraderDef* d = m_data->trader(t.def);                                // Definition
    const GoodDef* g = m_data->good(good);                                     // Ware
    int island = m_world->buildings[static_cast<std::size_t>(t.building)].island; // Insel
    float bonus = (d && g && g->perishDays > 0) ? 1.0f + d->freshBonus * fresh : 1.0f; // Frischezuschlag
    return std::max(1, static_cast<int>(islandPrice(island, good) * (d ? d->buyFactor : 1.0f) * bonus * stats().trade / 100.0f + 0.5f)); // Preis
} // Ende von traderSellPrice

// Spieler kauft Produkte beim Hersteller
int GameState::producerBuyPrice(const ProducerState& p, const std::string& good) const { // Beginn von producerBuyPrice
    const ProducerDef* d = m_data->producer(p.def);                            // Definition
    int island = m_world->buildings[static_cast<std::size_t>(p.building)].island; // Insel
    return std::max(1, static_cast<int>(islandPrice(island, good) * (d ? d->sellFactor : 1.0f) * 100.0f / stats().trade + 0.5f)); // Preis
} // Ende von producerBuyPrice

// Spieler verkauft Rohwaren an den Hersteller
int GameState::producerSellPrice(const ProducerState& p, const std::string& good) const { // Beginn von producerSellPrice
    const ProducerDef* d = m_data->producer(p.def);                            // Definition
    int island = m_world->buildings[static_cast<std::size_t>(p.building)].island; // Insel
    return std::max(1, static_cast<int>(islandPrice(island, good) * (d ? d->buyFactor : 1.0f) * stats().trade / 100.0f + 0.5f)); // Preis
} // Ende von producerSellPrice

// Freier Lagerplatz eines Herstellers
int GameState::producerSpace(const ProducerState& p) const {                   // Beginn von producerSpace
    const ProducerDef* d = m_data->producer(p.def);                            // Definition
    float used = 0.0f;                                                         // Belegt
    for (const auto& kv : p.stock) used += kv.second;                          // Summe
    return std::max(0, (d ? d->storage : 0) - static_cast<int>(used + 0.5f));  // Frei
} // Ende von producerSpace

// Kauft eine Einheit und legt sie an den Steg
std::string GameState::buy(int island, const std::string& good, int unitPrice) { // Beginn von buy
    if (credits < unitPrice) return "Nicht genug Credits.";                    // Zu teuer
    const GoodDef* g = m_data->good(good);                                     // Ware
    if (!g) return "Unbekannte Ware.";                                         // Fehler
    credits -= unitPrice;                                                      // Bezahlen
    Cargo c;                                                                   // Neue Einheit
    c.good = g->id;                                                            // Ware
    c.born = hours;                                                            // Frisch gekauft
    pier(island).push_back(c);                                                 // An den Steg
    return "";                                                                 // Erfolgreich
} // Ende von buy

// Verkauft eine Einheit und liefert den Erlös (-1 = keine Ware da)
int GameState::sellOne(int island, const std::string& good, int unitPrice, bool freshMatters, float freshBonus) { // Beginn von sellOne
    std::vector<Cargo> got = take(island, good, 1);                            // Eine Einheit entnehmen
    if (got.empty()) return -1;                                                // Nichts da
    float price = static_cast<float>(unitPrice);                               // Grundpreis
    const GoodDef* g = m_data->good(good);                                     // Ware
    if (freshMatters && g && g->perishDays > 0) price *= 1.0f + freshBonus * freshness(got[0]); // Frischezuschlag
    int earned = std::max(1, static_cast<int>(price + 0.5f));                  // Erlös
    credits += earned;                                                         // Gutschreiben
    totalEarned += earned;                                                     // Statistik
    return earned;                                                             // Ergebnis
} // Ende von sellOne

// Verkäufe drücken den Kontorpreis
void GameState::saturate(int island, const std::string& good) {                // Beginn von saturate
    const GoodDef* g = m_data->good(good);                                     // Ware
    if (!g) return;                                                            // Unbekannt
    float& s = saturation[std::to_string(island) + "|" + g->id];               // Sättigungswert
    s = std::min(0.6f, s + m_set->saturation);                                 // Erhöhen
} // Ende von saturate

TraderState* GameState::trader(int building) { for (TraderState& t : traders) if (t.building == building) return &t; return nullptr; }       // Händler suchen
ProducerState* GameState::producer(int building) { for (ProducerState& p : producers) if (p.building == building) return &p; return nullptr; } // Hersteller suchen

// Legt Händler und Hersteller für alle Gebäude an
void GameState::setupTraders() {                                               // Beginn von setupTraders
    traders.clear();                                                           // Alte Händler löschen
    producers.clear();                                                         // Alte Hersteller löschen
    for (std::size_t bi = 0; bi < m_world->buildings.size(); ++bi) {           // Alle Gebäude
        const Building& b = m_world->buildings[bi];                            // Gebäude
        const Island& isl = m_world->islands[static_cast<std::size_t>(b.island)]; // Insel
        if (b.type == "haendler") {                                            // Händler
            const TraderDef* d = m_data->trader(b.ref);                        // Händlerart
            if (!d) continue;                                                  // Unbekannt
            TraderState t;                                                     // Neuer Zustand
            t.building = static_cast<int>(bi);                                 // Gebäude
            t.def = d->id;                                                     // Art
            GoodCategory cat = categoryOf(d->kind);                            // Kategorie
            t.buys = d->buys;                                                  // Feste Ankaufliste
            if (t.buys.empty()) for (const GoodDef& g : m_data->goods) if (g.category == cat && contains(isl.demands, g.id)) t.buys.push_back(g.id); // Gesuchte Waren der Kategorie
            if (t.buys.empty()) {                                              // Insel sucht nichts aus dieser Kategorie
                int k = 0;                                                     // Zähler
                for (const GoodDef& g : m_data->goods) if (g.category == cat && !contains(isl.produces, g.id) && (k++ % 2) == 0) t.buys.push_back(g.id); // Jede zweite fremde Ware
            }                                                                  // Ende Ersatz-Ankauf
            t.sells = d->sells;                                                // Feste Verkaufsliste
            if (t.sells.empty()) for (const GoodDef& g : m_data->goods) if (g.category == cat && contains(isl.produces, g.id)) t.sells.push_back(g.id); // Eigene Waren der Kategorie
            if (t.sells.empty()) for (const GoodDef& g : m_data->goods) if (g.category == cat && !contains(t.buys, g.id) && !contains(isl.demands, g.id)) t.sells.push_back(g.id); // Sonst übrige Waren der Kategorie (eingeführt)
            for (const std::string& g : t.buys) t.demand[g] = static_cast<float>(d->demandMax) * (d->demandMode == "steigend" ? 0.5f : 1.0f); // Startbedarf
            for (const std::string& g : t.sells) t.stock[g] = static_cast<float>(d->stockMax); // Startbestand
            traders.push_back(t);                                              // Speichern
        } else if (b.type == "hersteller") {                                   // Hersteller
            const ProducerDef* d = m_data->producer(b.ref);                    // Definition
            if (!d) continue;                                                  // Unbekannt
            ProducerState p;                                                   // Neuer Zustand
            p.building = static_cast<int>(bi);                                 // Gebäude
            p.def = d->id;                                                     // Art
            for (const std::string& raw : d->rawGoods) p.stock[raw] = 0.0f;    // Leeres Rohwarenlager
            for (const auto& r : d->recipes) p.stock[r.product] = static_cast<float>(d->startStock); // Startbestand
            producers.push_back(p);                                            // Speichern
        }                                                                      // Ende der Unterscheidung
    }                                                                          // Ende der Gebäude
} // Ende von setupTraders

// ============================================================ Aufträge

// Erzeugt neue Auftragsangebote in allen Kontoren
void GameState::makeOffers() {                                                 // Beginn von makeOffers
    offers.clear();                                                            // Alte Angebote verfallen
    ShipStats s = stats();                                                     // Schiffswerte (Laderaumgröße)
    unsigned d = static_cast<unsigned>(day());                                 // Tag als Zufallsquelle
    for (std::size_t i = 0; i < m_world->islands.size(); ++i) {                // Alle Inseln
        bool hasKontor = false;                                                // Kontor vorhanden?
        for (int b : m_world->islands[i].buildings) if (m_world->buildings[static_cast<std::size_t>(b)].type == "kontor") hasKontor = true; // Suchen
        if (!hasKontor || m_world->islands.size() < 2) continue;               // Kein Kontor
        for (int k = 0; k < m_set->contractOffers; ++k) {                      // Angebote
            unsigned key = static_cast<unsigned>(i) * 17u + static_cast<unsigned>(k); // Schlüssel
            std::size_t to = static_cast<std::size_t>(hash01(d, key, 11) * static_cast<float>(m_world->islands.size() - 1)); // Zielinsel
            if (to >= i) ++to;                                                 // Nicht die eigene Insel
            if (to >= m_world->islands.size()) to = 0;                         // Sicherheit
            const Island& target = m_world->islands[to];                       // Ziel
            std::vector<std::string> cand;                                     // Mögliche Waren
            for (const std::string& g : target.demands) if (m_data->good(g)) cand.push_back(m_data->good(g)->id); // Gesuchte Waren des Ziels
            if (cand.empty()) for (const GoodDef& g : m_data->goods) cand.push_back(g.id); // Ersatz: alle Waren
            const GoodDef* g = m_data->good(cand[static_cast<std::size_t>(hash01(d, key, 12) * static_cast<float>(cand.size())) % cand.size()]); // Ware wählen
            int cellsPerUnit = static_cast<int>(g->shape.cells.size());        // Platzbedarf
            int maxUnits = std::max(1, (s.holdW * s.holdH * 3 / 4) / std::max(1, cellsPerUnit)); // Muss in den Laderaum passen
            Contract c;                                                        // Neuer Auftrag
            c.good = g->id;                                                    // Ware
            c.amount = std::max(1, std::min(maxUnits, 2 + static_cast<int>(hash01(d, key, 13) * 5.0f))); // Menge
            c.from = static_cast<int>(i);                                      // Angebotsort
            c.to = static_cast<int>(to);                                       // Ziel
            const Island& from = m_world->islands[i];                          // Start
            float dist = std::sqrt((from.dockX - target.dockX) * (from.dockX - target.dockX) + (from.dockY - target.dockY) * (from.dockY - target.dockY)); // Entfernung
            float travel = dist / std::max(1.0f, s.speed * 0.7f) / m_set->secondsPerHour; // Geschätzte Fahrzeit in Stunden
            c.deadline = hours + travel * 2.5f + m_set->contractBuffer;        // Frist
            c.reward = static_cast<int>(static_cast<float>(c.amount) * islandPrice(c.to, c.good) * m_set->contractReward + dist * m_set->contractDistanceBonus); // Belohnung
            offers.push_back(c);                                               // Speichern
        }                                                                      // Ende der Angebote
    }                                                                          // Ende der Inseln
} // Ende von makeOffers

// Nimmt ein Angebot an
std::string GameState::acceptContract(std::size_t offerIndex) {                // Beginn von acceptContract
    if (offerIndex >= offers.size()) return "Angebot nicht mehr verfügbar.";   // Ungültig
    if (static_cast<int>(active.size()) >= m_set->maxContracts) return "Du hast schon " + std::to_string(m_set->maxContracts) + " Aufträge."; // Zu viele
    active.push_back(offers[offerIndex]);                                      // Annehmen
    offers.erase(offers.begin() + static_cast<long>(offerIndex));              // Aus den Angeboten entfernen
    return "";                                                                 // Erfolgreich
} // Ende von acceptContract

// Liefert einen Auftrag ab
std::string GameState::deliverContract(std::size_t activeIndex, int island) {  // Beginn von deliverContract
    if (activeIndex >= active.size()) return "Auftrag unbekannt.";             // Ungültig
    Contract c = active[activeIndex];                                          // Kopie
    if (c.to != island) return "Dieser Auftrag geht nach " + m_world->islands[static_cast<std::size_t>(c.to)].name + "."; // Falsche Insel
    if (available(island, c.good) < c.amount) return "Nicht genug Ware am Steg oder an Bord."; // Zu wenig Ware
    take(island, c.good, c.amount);                                            // Ware abgeben
    credits += c.reward;                                                       // Belohnung
    totalEarned += c.reward;                                                   // Statistik
    ++deliveries;                                                              // Statistik
    active.erase(active.begin() + static_cast<long>(activeIndex));             // Auftrag erledigt
    return "";                                                                 // Erfolgreich
} // Ende von deliverContract

// Auftrag mit der nächsten Frist
const Contract* GameState::nextDeadline() const {                              // Beginn von nextDeadline
    const Contract* best = nullptr;                                            // Bester Kandidat
    for (const Contract& c : active) if (!best || c.deadline < best->deadline) best = &c; // Früheste Frist
    return best;                                                               // Ergebnis
} // Ende von nextDeadline

// ============================================================ Artefakte

// Höchste Stufe, für die genug Entdeckerpunkte vorhanden sind
int GameState::unlockedTier() const {                                          // Beginn von unlockedTier
    int tier = 1;                                                              // Mindestens Stufe 1
    for (std::size_t i = 0; i < m_data->explorerThresholds.size(); ++i) if (explorerPoints >= m_data->explorerThresholds[i]) tier = static_cast<int>(i) + 1; // Schwellen prüfen
    return tier;                                                               // Ergebnis
} // Ende von unlockedTier

// Setzt ein neues Artefakt auf See (seltene eher in gefährlichen Gebieten)
void GameState::spawnArtifact(unsigned s) {                                    // Beginn von spawnArtifact
    if (m_data->artifacts.empty()) return;                                     // Keine Artefakte definiert
    int maxTier = unlockedTier();                                              // Höchste Stufe
    int tier = hash01(s, 21, 1) < 0.45f ? maxTier : 1 + static_cast<int>(hash01(s, 22, 1) * static_cast<float>(maxTier)); // Stufe würfeln
    std::vector<const ArtifactDef*> cand;                                      // Kandidaten
    while (cand.empty() && tier >= 1) {                                        // Notfalls niedrigere Stufe
        for (const ArtifactDef& a : m_data->artifacts) if (a.tier == tier) cand.push_back(&a); // Passende Stufe
        --tier;                                                                // Eine Stufe tiefer
    }                                                                          // Ende der Suche
    if (cand.empty()) return;                                                  // Nichts gefunden
    const ArtifactDef* a = cand[static_cast<std::size_t>(hash01(s, 23, 1) * static_cast<float>(cand.size())) % cand.size()]; // Artefakt wählen
    FloatingArtifact f;                                                        // Neues Treibgut
    f.id = a->id;                                                              // Artefakt
    bool placed = false;                                                       // Position gefunden?
    if (a->tier >= 2 && !m_world->zones.empty() && hash01(s, 24, 1) < 0.6f) {  // Seltene Stücke liegen oft in Gefahrenzonen
        const Zone& z = m_world->zones[static_cast<std::size_t>(hash01(s, 25, 1) * static_cast<float>(m_world->zones.size())) % m_world->zones.size()]; // Zone wählen
        for (unsigned k = 0; k < 20 && !placed; ++k) {                         // Mehrere Versuche
            float ang = hash01(s, 26, k) * 2.0f * PI, r = hash01(s, 27, k) * z.radius * 0.8f; // Zufällige Stelle in der Zone
            float x = z.x + std::cos(ang) * r, y = z.y + std::sin(ang) * r;    // Position
            if (m_world->sailable(x, y, 0.8f)) { f.x = x; f.y = y; placed = true; } // Befahrbar?
        }                                                                      // Ende der Versuche
    }                                                                          // Ende Gefahrenzone
    if (!placed) { auto p = m_world->randomSeaPoint(s, 3.0f); f.x = p.first; f.y = p.second; } // Sonst irgendwo auf See
    floating.push_back(f);                                                     // Speichern
} // Ende von spawnArtifact

// Verkauft ein Artefakt aus der Kajüte an das Museum
std::string GameState::sellArtifact(std::size_t index) {                       // Beginn von sellArtifact
    if (index >= artifacts.size()) return "Artefakt unbekannt.";               // Ungültig
    const ArtifactDef* a = m_data->artifact(artifacts[index]);                 // Definition
    if (!a) return "Artefakt unbekannt.";                                      // Ungültig
    int tierBefore = unlockedTier();                                           // Stufe vorher
    credits += a->value;                                                       // Geld
    totalEarned += a->value;                                                   // Statistik
    explorerPoints += a->points;                                               // Entdeckerpunkte
    ++artifactsSold;                                                           // Statistik
    artifacts.erase(artifacts.begin() + static_cast<long>(index));             // Aus der Kajüte
    if (unlockedTier() > tierBefore) addMessage("Neuer Entdeckerrang! Jetzt treiben wertvollere Artefakte (Stufe " + std::to_string(unlockedTier()) + ") auf See."); // Neue Stufe
    return "";                                                                 // Erfolgreich
} // Ende von sellArtifact

// ============================================================ Crew

// Erzeugt neue Bewerber in allen Tavernen
void GameState::makeApplicants() {                                             // Beginn von makeApplicants
    applicants.clear();                                                        // Alte Bewerber gehen
    if (m_data->roles.empty()) return;                                         // Keine Rollen
    unsigned d = static_cast<unsigned>(day());                                 // Tag als Zufallsquelle
    for (std::size_t i = 0; i < m_world->islands.size(); ++i) {                // Alle Inseln
        if (!m_world->islands[i].hasTavern) continue;                          // Keine Taverne
        for (int k = 0; k < m_set->applicants; ++k) {                          // Bewerber
            unsigned key = static_cast<unsigned>(i) * 29u + static_cast<unsigned>(k); // Schlüssel
            const CrewRole& r = m_data->roles[static_cast<std::size_t>(hash01(d, key, 31) * static_cast<float>(m_data->roles.size())) % m_data->roles.size()]; // Rolle
            Applicant a;                                                       // Neuer Bewerber
            a.role = r.id;                                                     // Rolle
            a.name = m_data->crewNames.empty() ? "Matrose" : m_data->crewNames[static_cast<std::size_t>(hash01(d, key, 32) * static_cast<float>(m_data->crewNames.size())) % m_data->crewNames.size()]; // Name
            a.wage = std::max(1, static_cast<int>(static_cast<float>(r.wage) * (0.8f + 0.4f * hash01(d, key, 33)) + 0.5f)); // Lohn
            applicants[static_cast<int>(i)].push_back(a);                      // Speichern
        }                                                                      // Ende der Bewerber
    }                                                                          // Ende der Inseln
} // Ende von makeApplicants

// Heuert einen Bewerber an (Handgeld = ein Tageslohn)
std::string GameState::hire(int island, std::size_t index) {                   // Beginn von hire
    auto it = applicants.find(island);                                         // Bewerber der Insel
    if (it == applicants.end() || index >= it->second.size()) return "Bewerber nicht mehr da."; // Ungültig
    if (static_cast<int>(crew.size()) >= shipClass().maxCrew) return "Keine freie Koje an Bord."; // Kein Platz
    const Applicant& a = it->second[index];                                    // Bewerber
    if (credits < a.wage) return "Nicht genug Credits für das Handgeld.";      // Zu wenig Geld
    credits -= a.wage;                                                         // Handgeld zahlen
    crew.push_back({a.role, a.name, a.wage});                                  // An Bord
    it->second.erase(it->second.begin() + static_cast<long>(index));           // Aus der Liste
    return "";                                                                 // Erfolgreich
} // Ende von hire

void GameState::fire(std::size_t index) { if (index < crew.size()) crew.erase(crew.begin() + static_cast<long>(index)); } // Entlassen

int GameState::dailyWages() const { int w = 0; for (const CrewMember& m : crew) w += m.wage; return w; } // Summe der Löhne

// ============================================================ Punkte

// Wert der Ladung (Grundpreise, nur unverdorbene Ware)
int GameState::cargoValue() const {                                            // Beginn von cargoValue
    float v = 0.0f;                                                            // Summe
    auto add = [&](const std::vector<Cargo>& list) { for (const Cargo& c : list) { const GoodDef* g = m_data->good(c.good); if (g && !rotten(c)) v += static_cast<float>(g->basePrice); } }; // Hilfsfunktion
    add(hold);                                                                 // Laderaum
    for (const auto& kv : piers) add(kv.second);                               // Alle Steglager
    return static_cast<int>(v);                                                // Ergebnis
} // Ende von cargoValue

int GameState::score() const { return credits + shipValue() + cargoValue() + explorerPoints * 10; } // Gesamtpunkte

// ============================================================ Speichern und Laden

// Schreibt den Spielstand als Textdatei
bool GameState::save(const std::string& path) const {                          // Beginn von save
    PropertyFile f;                                                            // Neue Datei
    const std::string S = "Spiel";                                             // Hauptabschnitt
    f.set(S, "stunden", num(hours)); f.setInt(S, "seed", static_cast<int>(seed)); // Zeit und Zufall
    f.setInt(S, "credits", credits); f.setInt(S, "entdecker", explorerPoints); f.set(S, "gesundheit", num(health)); // Spieler
    f.set(S, "schiff", shipId); f.set(S, "rumpf", num(hull)); f.set(S, "treibstoff", num(fuel)); f.setInt(S, "upgrade_ausgaben", upgradeSpent); // Schiff
    f.setInt(S, "zu_fuss", onFoot ? 1 : 0); f.set(S, "figur_x", num(figX)); f.set(S, "figur_y", num(figY)); // Figur
    f.set(S, "schiff_x", num(shipX)); f.set(S, "schiff_y", num(shipY)); f.set(S, "schiff_kurs", num(shipAngle)); // Schiffsposition
    f.setInt(S, "angelegt", docked); f.setInt(S, "letzter_hafen", lastHarbor); // Hafen
    f.setInt(S, "ziel", hasTarget ? 1 : 0); f.set(S, "ziel_x", num(targetX)); f.set(S, "ziel_y", num(targetY)); f.set(S, "ziel_name", targetName); // Ziel
    f.setInt(S, "einnahmen", totalEarned); f.setInt(S, "lieferungen", deliveries); f.setInt(S, "artefakte_verkauft", artifactsSold); // Statistik
    f.setInt(S, "piraten_versenkt", piratesSunk); f.setInt(S, "schiffe_verloren", shipsLost); // Statistik
    for (const auto& kv : upgrades) f.setInt("Upgrades", kv.first, kv.second); // Upgrade-Stufen
    std::vector<std::string> list;                                             // Hilfsliste
    for (const CrewMember& m : crew) list.push_back(m.role + "@" + m.name + "@" + std::to_string(m.wage)); // Crew
    f.set("Crew", "liste", joinText(list, ", ")); list.clear();                // Speichern
    for (const Cargo& c : hold) list.push_back(c.good + "@" + num(c.born) + "@" + std::to_string(c.x) + "@" + std::to_string(c.y) + "@" + std::to_string(c.rot)); // Laderaum
    f.set("Laderaum", "liste", joinText(list, ", ")); list.clear();            // Speichern
    for (const auto& kv : piers) {                                             // Steglager
        for (const Cargo& c : kv.second) list.push_back(c.good + "@" + num(c.born)); // Einheiten
        f.set("Steg", "insel_" + std::to_string(kv.first), joinText(list, ", ")); list.clear(); // Speichern
    }                                                                          // Ende der Steglager
    f.set("Kajuete", "liste", joinText(artifacts, ", "));                      // Artefakte an Bord
    for (const FloatingArtifact& a : floating) list.push_back(a.id + "@" + num(a.x) + "@" + num(a.y) + "@" + (a.seen ? "1" : "0")); // Treibgut
    f.set("Treibgut", "liste", joinText(list, ", ")); list.clear();            // Speichern
    auto contractText = [](const Contract& c) { return c.good + "@" + std::to_string(c.amount) + "@" + std::to_string(c.from) + "@" + std::to_string(c.to) + "@" + num(c.deadline) + "@" + std::to_string(c.reward); }; // Auftrag als Text
    for (const Contract& c : offers) list.push_back(contractText(c));          // Angebote
    f.set("Angebote", "liste", joinText(list, ", ")); list.clear();            // Speichern
    for (const Contract& c : active) list.push_back(contractText(c));          // Aktive Aufträge
    f.set("Auftraege", "liste", joinText(list, ", ")); list.clear();           // Speichern
    for (std::size_t i = 0; i < traders.size(); ++i) {                         // Händler
        std::string sec = "Haendler_" + std::to_string(i);                     // Abschnitt
        for (const auto& kv : traders[i].demand) f.set(sec, "bedarf_" + kv.first, num(kv.second)); // Bedarf
        for (const auto& kv : traders[i].stock) f.set(sec, "bestand_" + kv.first, num(kv.second)); // Bestand
    }                                                                          // Ende der Händler
    for (std::size_t i = 0; i < producers.size(); ++i) {                       // Hersteller
        std::string sec = "Hersteller_" + std::to_string(i);                   // Abschnitt
        for (const auto& kv : producers[i].stock) f.set(sec, "lager_" + kv.first, num(kv.second)); // Lager
        f.set(sec, "fortschritt", num(producers[i].progress)); f.setInt(sec, "naechstes", producers[i].nextRecipe); // Fortschritt
    }                                                                          // Ende der Hersteller
    for (const auto& kv : applicants) {                                        // Bewerber
        for (const Applicant& a : kv.second) list.push_back(a.role + "@" + a.name + "@" + std::to_string(a.wage)); // Bewerber als Text
        f.set("Bewerber", "insel_" + std::to_string(kv.first), joinText(list, ", ")); list.clear(); // Speichern
    }                                                                          // Ende der Bewerber
    for (const auto& kv : saturation) {                                        // Sättigung
        std::size_t bar = kv.first.find('|');                                  // Trennzeichen
        f.set("Saettigung", "s_" + kv.first.substr(0, bar) + "_" + kv.first.substr(bar + 1), num(kv.second)); // Speichern
    }                                                                          // Ende der Sättigung
    return f.save(path, "Mini Ship Delivery - Spielstand (wird automatisch geschrieben)"); // Datei schreiben
} // Ende von save

// Liest einen Spielstand
bool GameState::load(const std::string& path) {                                // Beginn von load
    PropertyFile f;                                                            // Datei
    if (!f.load(path) || !f.hasSection("Spiel")) return false;                 // Kein Spielstand
    const std::string S = "Spiel";                                             // Hauptabschnitt
    seed = static_cast<unsigned>(f.getInt(S, "seed", 1));                      // Zufall
    m_weather.clear();                                                         // Wetter neu erzeugen
    hours = f.getFloat(S, "stunden", 7.0f);                                    // Zeit
    credits = f.getInt(S, "credits", 0); explorerPoints = f.getInt(S, "entdecker", 0); health = f.getFloat(S, "gesundheit", 100.0f); // Spieler
    shipId = f.getString(S, "schiff", m_data->ships.front().id);               // Schiff
    if (!m_data->ship(shipId)) shipId = m_data->ships.front().id;              // Unbekannte Klasse ersetzen
    hull = f.getFloat(S, "rumpf", 50.0f); fuel = f.getFloat(S, "treibstoff", 0.0f); upgradeSpent = f.getInt(S, "upgrade_ausgaben", 0); // Zustand
    onFoot = f.getInt(S, "zu_fuss", 1) != 0; figX = f.getFloat(S, "figur_x", 10.0f); figY = f.getFloat(S, "figur_y", 10.0f); // Figur
    shipX = f.getFloat(S, "schiff_x", 10.0f); shipY = f.getFloat(S, "schiff_y", 10.0f); shipAngle = f.getFloat(S, "schiff_kurs", 0.0f); // Schiff
    docked = f.getInt(S, "angelegt", -1); lastHarbor = f.getInt(S, "letzter_hafen", 0); // Hafen
    hasTarget = f.getInt(S, "ziel", 0) != 0; targetX = f.getFloat(S, "ziel_x", 0.0f); targetY = f.getFloat(S, "ziel_y", 0.0f); targetName = f.getString(S, "ziel_name", ""); // Ziel
    totalEarned = f.getInt(S, "einnahmen", 0); deliveries = f.getInt(S, "lieferungen", 0); artifactsSold = f.getInt(S, "artefakte_verkauft", 0); // Statistik
    piratesSunk = f.getInt(S, "piraten_versenkt", 0); shipsLost = f.getInt(S, "schiffe_verloren", 0); // Statistik
    upgrades.clear();                                                          // Upgrades
    for (const std::string& k : f.keys("Upgrades")) { const UpgradeDef* u = m_data->upgrade(k); if (u) upgrades[u->id] = f.getInt("Upgrades", k, 0); } // Stufen lesen
    crew.clear();                                                              // Crew
    for (const std::string& e : f.getList("Crew", "liste")) { std::vector<std::string> p = splitText(e, '@'); if (p.size() == 3 && m_data->role(p[0])) crew.push_back({m_data->role(p[0])->id, p[1], std::atoi(p[2].c_str())}); } // Mitglieder
    auto goodId = [&](const std::string& id) { const GoodDef* g = m_data->good(id); return g ? g->id : std::string(); }; // Richtige Schreibweise einer Ware
    hold.clear();                                                              // Laderaum
    for (const std::string& e : f.getList("Laderaum", "liste")) { std::vector<std::string> p = splitText(e, '@'); if (p.size() == 5 && !goodId(p[0]).empty()) { Cargo c; c.good = goodId(p[0]); c.born = std::strtof(p[1].c_str(), nullptr); c.x = std::atoi(p[2].c_str()); c.y = std::atoi(p[3].c_str()); c.rot = std::atoi(p[4].c_str()); hold.push_back(c); } } // Einheiten
    piers.clear();                                                             // Steglager
    for (const std::string& k : f.keys("Steg")) {                              // Alle Inseln
        int island = std::atoi(k.c_str() + 6);                                 // "insel_N"
        for (const std::string& e : f.getList("Steg", k)) { std::vector<std::string> p = splitText(e, '@'); if (p.size() == 2 && !goodId(p[0]).empty()) { Cargo c; c.good = goodId(p[0]); c.born = std::strtof(p[1].c_str(), nullptr); piers[island].push_back(c); } } // Einheiten
    }                                                                          // Ende der Steglager
    artifacts.clear();                                                         // Kajüte
    for (const std::string& e : f.getList("Kajuete", "liste")) if (m_data->artifact(e)) artifacts.push_back(m_data->artifact(e)->id); // Artefakte
    floating.clear();                                                          // Treibgut
    for (const std::string& e : f.getList("Treibgut", "liste")) { std::vector<std::string> p = splitText(e, '@'); if (p.size() == 4 && m_data->artifact(p[0])) floating.push_back({m_data->artifact(p[0])->id, std::strtof(p[1].c_str(), nullptr), std::strtof(p[2].c_str(), nullptr), p[3] == "1"}); } // Artefakte auf See
    auto parseContract = [&](const std::string& e, Contract& c) {              // Auftrag aus Text
        std::vector<std::string> p = splitText(e, '@');                        // Teile
        if (p.size() != 6 || goodId(p[0]).empty()) return false;               // Ungültig
        c.good = goodId(p[0]); c.amount = std::atoi(p[1].c_str()); c.from = std::atoi(p[2].c_str()); c.to = std::atoi(p[3].c_str()); // Werte
        c.deadline = std::strtof(p[4].c_str(), nullptr); c.reward = std::atoi(p[5].c_str()); // Werte
        return c.from >= 0 && c.to >= 0 && c.from < static_cast<int>(m_world->islands.size()) && c.to < static_cast<int>(m_world->islands.size()); // Gültige Inseln?
    };                                                                         // Ende der Hilfsfunktion
    offers.clear(); active.clear();                                            // Aufträge
    for (const std::string& e : f.getList("Angebote", "liste")) { Contract c; if (parseContract(e, c)) offers.push_back(c); } // Angebote
    for (const std::string& e : f.getList("Auftraege", "liste")) { Contract c; if (parseContract(e, c)) active.push_back(c); } // Aktive Aufträge
    setupTraders();                                                            // Händler und Hersteller neu anlegen
    for (std::size_t i = 0; i < traders.size(); ++i) {                         // Händler
        std::string sec = "Haendler_" + std::to_string(i);                     // Abschnitt
        for (const std::string& k : f.keys(sec)) {                             // Alle Werte
            float v = f.getFloat(sec, k, 0.0f);                                // Wert
            if (k.rfind("bedarf_", 0) == 0) { std::string g = goodId(k.substr(7)); if (traders[i].demand.count(g)) traders[i].demand[g] = v; } // Bedarf
            if (k.rfind("bestand_", 0) == 0) { std::string g = goodId(k.substr(8)); if (traders[i].stock.count(g)) traders[i].stock[g] = v; } // Bestand
        }                                                                      // Ende der Werte
    }                                                                          // Ende der Händler
    for (std::size_t i = 0; i < producers.size(); ++i) {                       // Hersteller
        std::string sec = "Hersteller_" + std::to_string(i);                   // Abschnitt
        for (const std::string& k : f.keys(sec)) if (k.rfind("lager_", 0) == 0) { std::string g = goodId(k.substr(6)); if (!g.empty()) producers[i].stock[g] = f.getFloat(sec, k, 0.0f); } // Lager
        producers[i].progress = f.getFloat(sec, "fortschritt", 0.0f); producers[i].nextRecipe = f.getInt(sec, "naechstes", 0); // Fortschritt
    }                                                                          // Ende der Hersteller
    applicants.clear();                                                        // Bewerber
    for (const std::string& k : f.keys("Bewerber")) {                          // Alle Inseln
        int island = std::atoi(k.c_str() + 6);                                 // "insel_N"
        for (const std::string& e : f.getList("Bewerber", k)) { std::vector<std::string> p = splitText(e, '@'); if (p.size() == 3 && m_data->role(p[0])) applicants[island].push_back({m_data->role(p[0])->id, p[1], std::atoi(p[2].c_str())}); } // Bewerber
    }                                                                          // Ende der Bewerber
    saturation.clear();                                                        // Sättigung
    for (const std::string& k : f.keys("Saettigung")) {                        // Alle Einträge "s_<insel>_<ware>"
        std::size_t us = k.find('_', 2);                                       // Trennzeichen nach der Inselnummer
        if (us == std::string::npos) continue;                                 // Ungültig
        std::string g = goodId(k.substr(us + 1));                              // Ware
        if (!g.empty()) saturation[k.substr(2, us - 2) + "|" + g] = f.getFloat("Saettigung", k, 0.0f); // Speichern
    }                                                                          // Ende der Sättigung
    messages.clear();                                                          // Keine alten Meldungen
    return true;                                                               // Erfolgreich
} // Ende von load
