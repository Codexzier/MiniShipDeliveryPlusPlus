// GameState.h - Der veränderliche Spielstand: Zeit, Wetter, Wirtschaft, Schiff, Crew, Ladung, Aufträge und Artefakte
//
// Hier steckt die gesamte Spiellogik, die nichts mit Darstellung oder Eingabe zu tun hat.
// Alles, was gespeichert werden muss, liegt in dieser Klasse.
#pragma once // Header nur einmal einbinden

#include <map>    // std::map
#include <string> // std::string
#include <vector> // std::vector

#include "GameData.h"     // Feste Spieldaten
#include "PropertyFile.h" // Einstellungen und Spielstand als Textdatei
#include "World.h"        // Inseln und Gebäude

// Einstellbare Spielwerte aus data/spiel.txt
struct Settings {                                   // Beginn der Struktur
    float secondsPerHour = 15.0f;                   // Echte Sekunden je Spielstunde
    float startHour = 7.0f;                         // Uhrzeit beim Start
    float nightStart = 21.0f;                       // Beginn der Nacht
    float nightEnd = 5.0f;                          // Ende der Nacht
    bool timeInWindows = false;                     // Läuft die Zeit in Fenstern weiter?
    int startCredits = 600;                         // Startgeld
    std::string startShip = "Kutter";               // Startschiff
    std::vector<std::string> startCrew;             // Startcrew (Rollen)
    float startHealth = 100.0f;                     // Gesundheit des Kapitäns
    float walkSpeed = 2.8f;                         // Laufgeschwindigkeit
    float injuredBelow = 30.0f;                     // Ab hier läuft der Kapitän langsamer
    float healthRegen = 2.0f;                       // Erholung je Spielstunde
    float acceleration = 0.9f;                      // Beschleunigung des Schiffs
    float reverseFactor = 0.25f;                    // Rückwärtsfahrt
    float dockDistance = 3.0f;                      // Anlegeabstand
    float dockSpeed = 1.6f;                         // Höchsttempo zum Anlegen
    float groundDamage = 6.0f;                      // Schaden beim Auflaufen je Geschwindigkeit
    float understaffedMalus = 18.0f;                // Abzug je fehlendem Besatzungsmitglied (Prozent)
    float noEngineerPower = 0.5f;                   // Maschinenleistung ohne Maschinist
    float noEngineerFuel = 1.6f;                    // Mehrverbrauch ohne Maschinist
    float stormFrom = 0.8f;                         // Ab dieser Windstärke gibt es Sturmschaden
    float stormDamage = 1.5f;                       // Sturmschaden je Sekunde bei vollen Segeln
    float seaRepairMax = 0.7f;                      // Höchster Anteil, den die Crew auf See repariert
    float kontorBuy = 0.85f;                        // Kontor zahlt diesen Anteil
    float kontorSell = 1.1f;                        // Kontor verlangt diesen Anteil
    float ownGood = 0.7f;                           // Preisfaktor eigene Ware
    float wantedGood = 1.45f;                       // Preisfaktor gesuchte Ware
    float priceSwing = 0.12f;                       // Tägliche Schwankung
    float saturation = 0.03f;                       // Preisverfall je verkaufter Einheit
    float saturationRecovery = 0.3f;                // Erholung pro Tag
    int fuelPrice = 2;                              // Preis je Treibstoffeinheit
    int contractOffers = 3;                         // Auftragsangebote je Kontor
    int maxContracts = 3;                           // Höchstzahl aktiver Aufträge
    float contractReward = 1.6f;                    // Belohnungsfaktor
    float contractDistanceBonus = 2.0f;             // Bonus je Kachel Entfernung
    float contractPenalty = 0.25f;                  // Strafe bei verpasster Frist
    float contractBuffer = 18.0f;                   // Puffer der Frist in Stunden
    int repairPrice = 3;                            // Reparaturkosten je Rumpfpunkt
    float tradeIn = 0.5f;                           // Inzahlungnahme beim Schiffstausch
    int restPrice = 20;                             // Übernachten
    float wakeHour = 7.0f;                          // Aufwachzeit
    int applicants = 3;                             // Bewerber je Taverne und Tag
    std::map<std::string, std::pair<float, float>> openHours; // Öffnungszeiten je Gebäudeart
    float pirateSpeed = 3.5f;                       // Piratentempo
    float pirateSight = 8.0f;                       // Piratensicht am Tag
    float pirateSightNight = 5.0f;                  // Piratensicht bei Nacht
    float pirateHp = 60.0f;                         // Lebenspunkte eines Piratenschiffs
    float pirateDamage = 9.0f;                      // Kanonenschaden der Piraten
    float pirateRange = 5.5f;                       // Kanonenreichweite der Piraten
    float pirateReload = 3.0f;                      // Nachladezeit der Piraten
    float pirateRouteChance = 0.15f;                // Angriffswahrscheinlichkeit auf Routen
    float pirateSteal = 0.5f;                       // Gestohlener Anteil beim Entern
    int lootMin = 120;                              // Beute mindestens
    int lootMax = 320;                              // Beute höchstens
    float monsterSpeed = 3.9f;                      // Tempo der Seeungeheuer
    float monsterDamage = 22.0f;                    // Bissschaden
    float monsterBitePause = 2.0f;                  // Zeit zwischen Bissen
    float monsterHp = 220.0f;                       // Lebenspunkte eines Ungeheuers
    float monsterSight = 9.0f;                      // Ab dieser Entfernung wird gejagt
    float cannonRange = 6.0f;                       // Reichweite der eigenen Kanonen
    float cannonReload = 2.5f;                      // Nachladezeit der eigenen Kanonen
    float weatherSlot = 3.0f;                       // Stunden je Wetterabschnitt
    float windMean = 0.45f;                         // Mittlere Windstärke
    float windSwing = 0.2f;                         // Schwankung der Windstärke
    float windTurn = 35.0f;                         // Drehung der Windrichtung je Abschnitt (Grad)
    float rainChance = 0.15f;                       // Regenwahrscheinlichkeit
    float fogChance = 0.1f;                         // Nebelwahrscheinlichkeit
    float forecastError = 0.18f;                    // Fehler der Vorhersage
    float forecastErrorNavigator = 0.06f;           // Fehler mit Navigator
    float stability = 12.0f;                        // Erlaubte Schlagseite in Grad
    int loaderFee = 2;                              // Gebühr je Einheit für automatisches Beladen
    float capsizeDamage = 0.3f;                     // Rumpfschaden beim Kentern
    void load(const PropertyFile& f);               // Werte aus der Datei lesen
}; // Ende der Struktur Settings

// Eine Ladeeinheit (im Laderaum oder am Steg)
struct Cargo {                                      // Beginn der Struktur
    std::string good;                               // Ware (ID)
    float born = 0.0f;                              // Spielstunde des Einkaufs (für die Frische)
    int x = -1;                                     // Spalte im Laderaum (-1 = nicht im Laderaum)
    int y = -1;                                     // Reihe im Laderaum
    int rot = 0;                                    // Drehung in 90-Grad-Schritten
}; // Ende der Struktur Cargo

// Ein Besatzungsmitglied
struct CrewMember {                                 // Beginn der Struktur
    std::string role;                               // Rolle (ID aus crew.txt)
    std::string name;                               // Vorname
    int wage = 10;                                  // Tageslohn
}; // Ende der Struktur CrewMember

// Ein Liefer-Auftrag
struct Contract {                                   // Beginn der Struktur
    std::string good;                               // Ware
    int amount = 1;                                 // Menge
    int from = 0;                                   // Insel, an der der Auftrag angeboten wird
    int to = 0;                                     // Zielinsel
    float deadline = 0.0f;                          // Frist (Spielstunde)
    int reward = 100;                               // Belohnung
}; // Ende der Struktur Contract

// Ein treibendes Artefakt auf See
struct FloatingArtifact {                           // Beginn der Struktur
    std::string id;                                 // Artefakt (ID)
    float x = 0.0f;                                 // Position x
    float y = 0.0f;                                 // Position y
    bool seen = false;                              // Schon einmal im Scanner gewesen?
}; // Ende der Struktur FloatingArtifact

// Zustand eines Händlers (Marktstand oder Laden)
struct TraderState {                                // Beginn der Struktur
    int building = -1;                              // Gebäude in der Welt
    std::string def;                                // Händlerart (ID)
    std::vector<std::string> buys;                  // Angekaufte Waren
    std::vector<std::string> sells;                 // Verkaufte Waren
    std::map<std::string, float> demand;            // Aktueller Bedarf je Ware
    std::map<std::string, float> stock;             // Aktueller Bestand je Ware
}; // Ende der Struktur TraderState

// Zustand eines Herstellers
struct ProducerState {                              // Beginn der Struktur
    int building = -1;                              // Gebäude in der Welt
    std::string def;                                // Hersteller (ID)
    std::map<std::string, float> stock;             // Lagerbestand (Rohwaren und Produkte)
    float progress = 0.0f;                          // Fortschritt des laufenden Produkts in Stunden
    int nextRecipe = 0;                             // Nächstes Rezept (abwechselnd)
}; // Ende der Struktur ProducerState

// Ein Bewerber in der Taverne
struct Applicant {                                  // Beginn der Struktur
    std::string role;                               // Rolle
    std::string name;                               // Name
    int wage = 10;                                  // Lohnforderung
}; // Ende der Struktur Applicant

// Wetterarten
enum class WeatherType { Clear = 0, Cloudy = 1, Rain = 2, Storm = 3, Fog = 4 }; // Klar, bewölkt, Regen, Sturm, Nebel

// Wetter in einem Zeitabschnitt
struct WeatherSlot {                                // Beginn der Struktur
    float windAngle = 0.0f;                         // Richtung, in die der Wind weht (Radiant, Weltkoordinaten)
    float windStrength = 0.4f;                      // Stärke 0..1 (x10 = Beaufort)
    WeatherType type = WeatherType::Clear;          // Wetterart
}; // Ende der Struktur WeatherSlot

// Berechnete Werte des Schiffs (Klasse + Verbesserungen + Crew)
struct ShipStats {                                  // Beginn der Struktur
    float speed = 3.0f;                             // Höchstgeschwindigkeit
    float turn = 90.0f;                             // Wendigkeit (Grad pro Sekunde)
    float repair = 2.0f;                            // Reparatur je Spielstunde
    float armor = 5.0f;                             // Panzerung (Prozent)
    float defense = 0.0f;                           // Abwehr (Kanonenschaden)
    float scan = 9.0f;                              // Sichtweite in Kacheln
    float trade = 100.0f;                           // Handelsfaktor in Prozent
    float hullMax = 80.0f;                          // Rumpfpunkte
    float fuelMax = 0.0f;                           // Tankgröße
    float fuelUse = 0.0f;                           // Verbrauch je Sekunde bei voller Maschinenfahrt
    float motorPower = 0.0f;                        // Maschinenleistung 0..1 (0 = keine Maschine)
    bool sails = true;                              // Hat Segel?
    float draft = 1.2f;                             // Tiefgang
    int holdW = 4;                                  // Laderaumbreite
    int holdH = 3;                                  // Laderaumhöhe
    float stability = 12.0f;                        // Erlaubte Schlagseite
    float perishFactor = 1.0f;                      // Haltbarkeitsfaktor verderblicher Ware
    int missingCrew = 0;                            // Fehlende Besatzung
    bool engineer = false;                          // Maschinist an Bord?
    bool navigator = false;                         // Navigator an Bord?
}; // Ende der Struktur ShipStats

// Der komplette veränderliche Spielstand
class GameState {                                                                       // Beginn der Klasse
public:                                                                                 // Öffentliche Schnittstelle
    void attach(const GameData* data, const World* world, const Settings* settings);    // Feste Daten verbinden
    void newGame(unsigned seed);                                                        // Neues Spiel anlegen
    bool save(const std::string& path) const;                                           // Spielstand speichern
    bool load(const std::string& path);                                                 // Spielstand laden

    // ---------- Zeit ----------
    int day() const;                                                                    // Spieltag (ab 1)
    float hourOfDay() const;                                                            // Uhrzeit 0..24
    bool isNight() const;                                                               // Ist es Nacht?
    float daylight() const;                                                             // Helligkeit 0 (Nacht) .. 1 (Tag)
    std::string clockText() const;                                                      // "Tag 3, 14:05"
    static std::string timeText(float hours);                                           // Zeitpunkt als Text
    bool isOpen(const std::string& buildingType) const;                                 // Hat ein Gebäude geöffnet?
    std::string openText(const std::string& buildingType) const;                        // Öffnungszeiten als Text
    void advance(float gameHours);                                                      // Zeit vorspulen (stündliche und tägliche Ereignisse)

    // ---------- Wetter ----------
    WeatherSlot weatherAt(float hours) const;                                           // Wetter zu einem Zeitpunkt (Wind interpoliert)
    WeatherSlot forecast(float hours) const;                                            // Vorhersage (mit Ungenauigkeit)
    static std::string weatherName(WeatherType t);                                      // Name einer Wetterart
    static std::string windName(float strength);                                        // Name einer Windstärke
    static std::string directionName(float angle);                                      // Himmelsrichtung, aus der der Wind kommt

    // ---------- Schiff ----------
    ShipStats stats() const;                                                            // Aktuelle Schiffswerte
    const ShipClass& shipClass() const;                                                 // Aktuelle Schiffsklasse
    int upgradeLevel(const std::string& id) const;                                      // Stufe eines Upgrades
    int upgradePrice(const std::string& id) const;                                      // Preis der nächsten Stufe
    int shipValue() const;                                                              // Wert des Schiffs (Inzahlungnahme)
    bool hasRole(const std::string& role) const;                                        // Ist eine Rolle an Bord?

    // ---------- Ladung ----------
    bool fits(const Cargo& c, int x, int y, int rot, int ignore) const;                 // Passt eine Einheit an diese Stelle?
    std::vector<std::pair<int, int>> cells(const Cargo& c, int x, int y, int rot) const; // Belegte Felder einer Einheit
    int holdUsedCells() const;                                                          // Belegte Felder im Laderaum
    float freshness(const Cargo& c) const;                                              // Frische 1 (frisch) .. 0 (verdorben)
    bool rotten(const Cargo& c) const;                                                  // Verdorben?
    int available(int island, const std::string& good) const;                           // Verkaufbare Einheiten (Steg + Laderaum)
    std::vector<Cargo> take(int island, const std::string& good, int amount);           // Einheiten entnehmen (älteste zuerst)
    int discardRotten(int island);                                                      // Verdorbene Ware wegwerfen
    std::vector<Cargo>& pier(int island);                                               // Steglager einer Insel
    float holdHeel() const;                                                             // Schlagseite in Grad (seitlich)
    float holdTrim() const;                                                             // Trimm in Grad (Bug/Heck)
    float capsizeRisk() const;                                                          // 0 = ruhig, 1 = kentert
    void autoLoad(int island);                                                          // Laderaum automatisch packen
    void unloadAll(int island);                                                         // Alles aus dem Laderaum an den Steg

    // ---------- Preise und Handel ----------
    float islandPrice(int island, const std::string& good) const;                       // Mittlerer Inselpreis
    bool kontorSells(int island, const std::string& good) const;                        // Verkauft das Kontor diese Ware?
    int kontorBuyPrice(int island, const std::string& good) const;                      // Spieler kauft beim Kontor
    int kontorSellPrice(int island, const std::string& good) const;                     // Spieler verkauft ans Kontor
    int traderBuyPrice(const TraderState& t, const std::string& good) const;            // Spieler kauft beim Händler
    int traderSellPrice(const TraderState& t, const std::string& good, float fresh) const; // Spieler verkauft an den Händler
    int producerBuyPrice(const ProducerState& p, const std::string& good) const;        // Spieler kauft Produkte
    int producerSellPrice(const ProducerState& p, const std::string& good) const;       // Spieler verkauft Rohwaren
    int producerSpace(const ProducerState& p) const;                                    // Freier Lagerplatz
    std::string buy(int island, const std::string& good, int unitPrice);               // Eine Einheit kaufen (leer = ok, sonst Fehlertext)
    int sellOne(int island, const std::string& good, int unitPrice, bool freshMatters, float freshBonus); // Eine Einheit verkaufen (liefert Erlös, -1 = nichts da)
    void saturate(int island, const std::string& good);                                 // Preisverfall nach Verkauf
    TraderState* trader(int building);                                                  // Händlerzustand eines Gebäudes
    ProducerState* producer(int building);                                              // Herstellerzustand eines Gebäudes

    // ---------- Aufträge ----------
    std::string acceptContract(std::size_t offerIndex);                                 // Angebot annehmen
    std::string deliverContract(std::size_t activeIndex, int island);                   // Auftrag abliefern
    const Contract* nextDeadline() const;                                               // Dringendster Auftrag

    // ---------- Artefakte ----------
    int unlockedTier() const;                                                           // Höchste freigeschaltete Stufe
    void spawnArtifact(unsigned seed);                                                  // Neues Artefakt auf See setzen
    std::string sellArtifact(std::size_t index);                                        // Artefakt im Museum verkaufen

    // ---------- Crew ----------
    std::string hire(int island, std::size_t applicantIndex);                           // Bewerber anheuern
    void fire(std::size_t crewIndex);                                                   // Crewmitglied entlassen
    int dailyWages() const;                                                             // Summe der Tageslöhne

    // ---------- Punkte ----------
    int score() const;                                                                  // Gesamtpunkte (Vermögen + Entdeckerpunkte)
    int cargoValue() const;                                                             // Wert der Ladung (Kontorpreis Startinsel)
    void addMessage(const std::string& m) { messages.push_back(m); }                    // Meldung für die Oberfläche

    // ---------- Daten (öffentlich, damit Spiel und Oberfläche einfach zugreifen können) ----------
    float hours = 7.0f;                                  // Vergangene Spielstunden seit Tag 1, 0:00
    unsigned seed = 1;                                   // Startwert für Zufall (Wetter, Preise ...)
    int credits = 0;                                     // Geld
    int explorerPoints = 0;                              // Entdeckerpunkte
    float health = 100.0f;                               // Gesundheit des Kapitäns
    std::string shipId;                                  // Schiffsklasse
    std::map<std::string, int> upgrades;                 // Upgrade-Stufen
    int upgradeSpent = 0;                                // Für Upgrades ausgegebenes Geld (Schiffswert)
    float hull = 80.0f;                                  // Rumpfpunkte
    float fuel = 0.0f;                                   // Treibstoff
    std::vector<CrewMember> crew;                        // Besatzung
    std::vector<Cargo> hold;                             // Laderaum
    std::map<int, std::vector<Cargo>> piers;             // Steglager je Insel
    std::vector<std::string> artifacts;                  // Artefakte in der Kajüte
    std::vector<FloatingArtifact> floating;              // Artefakte auf See
    std::vector<Contract> offers;                        // Angebotene Aufträge (alle Kontore)
    std::vector<Contract> active;                        // Angenommene Aufträge
    std::vector<TraderState> traders;                    // Alle Händler
    std::vector<ProducerState> producers;                // Alle Hersteller
    std::map<int, std::vector<Applicant>> applicants;    // Bewerber je Insel
    std::map<std::string, float> saturation;             // Preisverfall je "insel|ware"
    bool onFoot = true;                                  // Ist der Kapitän zu Fuß unterwegs?
    float figX = 0.0f, figY = 0.0f;                      // Position der Figur
    float shipX = 0.0f, shipY = 0.0f;                    // Position des Schiffs
    float shipAngle = 0.0f;                              // Kurs des Schiffs (Radiant)
    int docked = -1;                                     // Insel, an der das Schiff liegt (-1 = auf See)
    int lastHarbor = 0;                                  // Zuletzt besuchter Hafen
    bool hasTarget = false;                              // Ist ein Ziel gesetzt?
    float targetX = 0.0f, targetY = 0.0f;                // Ziel auf der Karte
    std::string targetName;                              // Name des Ziels
    std::vector<std::string> messages;                   // Neue Meldungen (holt die Oberfläche ab)
    int totalEarned = 0;                                 // Statistik: Einnahmen
    int deliveries = 0;                                  // Statistik: erfüllte Aufträge
    int artifactsSold = 0;                               // Statistik: verkaufte Artefakte
    int piratesSunk = 0;                                 // Statistik: versenkte Piraten
    int shipsLost = 0;                                   // Statistik: verlorene Schiffe

private:                                                                                // Interne Funktionen
    void hourTick(int absoluteHour);                                                    // Stündliche Ereignisse
    void dayTick();                                                                     // Tägliche Ereignisse (6 Uhr)
    void setupTraders();                                                                // Händler und Hersteller anlegen
    void makeOffers();                                                                  // Neue Aufträge erzeugen
    void makeApplicants();                                                              // Neue Bewerber erzeugen
    WeatherSlot slot(int index) const;                                                  // Wetter eines Abschnitts
    float hash01(unsigned a, unsigned b, unsigned c) const;                             // Zufallszahl 0..1 aus drei Werten
    const GameData* m_data = nullptr;                                                   // Feste Spieldaten
    const World* m_world = nullptr;                                                     // Welt
    const Settings* m_set = nullptr;                                                    // Einstellungen
    mutable std::vector<WeatherSlot> m_weather;                                         // Zwischenspeicher der Wetterabschnitte
}; // Ende der Klasse GameState
