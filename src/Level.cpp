// Level.cpp - Laden, Aktualisieren und Zeichnen der Karte
#include "Level.h" // Eigene Deklarationen

#include <algorithm> // std::sort, std::max, std::min, std::remove_if
#include <cmath>     // std::sin, std::floor

#include "PropertyFile.h" // Textdatei mit den Kartenobjekten

// Darf jetzt noch ein Auto losfahren? (Autos dürfen nie bei Grün über die Kreuzung fahren)
bool TrafficLight::canSpawnCar() const {                                    // Beginn von canSpawnCar
    if (green) return false;                                                // Bei Grün fahren keine Autos
    float timeToLane = carDuration * (1.0f - ROAD_DEPTH_FAR) / (ROAD_DEPTH_NEAR - ROAD_DEPTH_FAR); // Zeit bis zur Fußgängerfurt
    return remaining() > timeToLane + 0.4f;                                 // Nur wenn das Auto vor Grün durch ist
} // Ende von canSpawnCar

// Startet ein neues Auto bei einem bestimmten Fortschritt
void TrafficLight::spawnCar(float startProgress) {                          // Beginn von spawnCar
    Car car;                                                                // Neues Auto
    car.progress = startProgress;                                           // Startposition
    car.variant = carCounter++ % 3;                                         // Farbe reihum wechseln
    cars.push_back(car);                                                    // Zur Liste hinzufügen
} // Ende von spawnCar

// Fußgängertaster: verkürzt die restliche Rotzeit
void TrafficLight::pressButton() {                                          // Beginn von pressButton
    if (green || buttonPressed) return;                                     // Nur einmal pro Rotphase wirksam
    buttonPressed = true;                                                   // Taster als gedrückt merken
    phaseTimer = std::max(phaseTimer, redTime - buttonWait);                // Restliche Rotzeit höchstens buttonWait
} // Ende von pressButton

// Ampelphasen umschalten und Autos bewegen
void TrafficLight::update(float dt) {                                       // Beginn von update
    phaseTimer += dt;                                                       // Zeit in der Phase erhöhen
    if (green && phaseTimer >= greenTime) {                                 // Grünphase ist vorbei
        green = false;                                                      // Auf Rot schalten
        phaseTimer = 0.0f;                                                  // Phasenzeit zurücksetzen
        carTimer = 0.0f;                                                    // Erstes Auto direkt nach der Räumzeit
        buttonPressed = false;                                              // Taster wieder bereit
    } else if (!green && phaseTimer >= redTime) {                           // Rotphase ist vorbei
        green = true;                                                       // Auf Grün schalten
        phaseTimer = 0.0f;                                                  // Phasenzeit zurücksetzen
    }                                                                       // Ende der Phasenumschaltung
    if (!green && phaseTimer >= clearance) {                                // Rot und Räumzeit vorbei
        carTimer -= dt;                                                     // Countdown bis zum nächsten Auto
        if (carTimer <= 0.0f && canSpawnCar()) {                            // Zeit für ein neues Auto
            spawnCar(0.0f);                                                 // Auto am Horizont starten
            carTimer = carInterval;                                         // Nächstes Auto planen
        }                                                                   // Ende der Prüfung
    }                                                                       // Ende der Autoerzeugung
    for (Car& car : cars) car.progress += dt / std::max(0.2f, carDuration); // Alle Autos weiterfahren lassen
    cars.erase(std::remove_if(cars.begin(), cars.end(), [](const Car& c) { return c.progress > 1.0f; }), cars.end()); // Vorbeigefahrene Autos entfernen
} // Ende von update

// Lädt die Karte aus der Textdatei
bool Level::load(const std::string& path) {                                 // Beginn von load
    PropertyFile file;                                                      // Datei-Objekt
    if (!file.load(path)) return false;                                     // Datei fehlt -> Fehler
    std::size_t slash = path.find_last_of("/\\");                           // Letzten Schrägstrich suchen
    fileName = slash == std::string::npos ? path : path.substr(slash + 1);  // Dateiname ohne Ordner
    name = file.getString("Level", "name", "Level");                        // Name der Karte
    length = file.getFloat("Level", "laenge", 50.0f);                       // Länge der Karte
    groundY = file.getFloat("Level", "boden_hoehe", 4.4f);                  // Höhe der Bodenkante
    startX = file.getFloat("Level", "start_x", 1.5f);                       // Startposition
    goalX = file.getFloat("Ziel", "x", length - 4.0f);                      // Zielposition

    ditches.clear();                                                        // Alte Gräben löschen
    for (const std::string& id : file.sectionsWithPrefix("Graben")) {       // Alle Objekte "Graben..."
        Ditch d;                                                            // Neuer Graben
        d.id = id;                                                          // Objektname
        d.x = file.getFloat(id, "x", 10.0f);                                // Linker Rand
        d.width = std::max(0.2f, file.getFloat(id, "breite", 1.5f));        // Breite
        d.depth = std::max(0.5f, file.getFloat(id, "tiefe", 1.5f));         // Tiefe
        d.water = file.getBool(id, "wasser", false);                        // Wasser ja/nein
        d.waterLevel = clampValue(file.getFloat(id, "wasser_tiefe", 0.55f), 0.1f, d.depth - 0.1f); // Wasseroberfläche
        d.damage = file.getFloat(id, "schaden", 10.0f);                     // Schaden
        int trashCount = std::max(0, file.getInt(id, "muell", 0));          // Anzahl Müllteile
        SimpleRandom rnd(static_cast<std::uint32_t>(d.x * 100.0f) + 7u);    // Fester Zufall pro Graben
        for (int i = 0; i < trashCount; ++i) {                              // Müllteile verteilen
            TrashItem t;                                                    // Neues Müllteil
            t.type = (i * 3 + rnd.rangeInt(0, 4)) % 5;                      // Art (gemischt)
            t.offset = (static_cast<float>(i) + rnd.range(0.25f, 0.75f)) / static_cast<float>(trashCount) * d.width; // Gleichmäßig verteilt
            t.phase = rnd.range(0.0f, 6.28f);                               // Zufällige Phase
            t.speed = rnd.range(1.2f, 2.2f);                                // Zufällige Schaukelgeschwindigkeit
            d.trash.push_back(t);                                           // Speichern
        }                                                                   // Ende der Müllschleife
        ditches.push_back(d);                                               // Graben speichern
    }                                                                       // Ende der Grabenschleife
    std::sort(ditches.begin(), ditches.end(), [](const Ditch& a, const Ditch& b) { return a.x < b.x; }); // Von links nach rechts sortieren

    shops.clear();                                                          // Alte Shops löschen
    for (const std::string& id : file.sectionsWithPrefix("Shop")) {         // Alle Objekte "Shop..."
        ShopSpot s;                                                         // Neuer Shop
        s.id = id;                                                          // Objektname
        s.name = file.getString(id, "name", "Shop");                        // Anzeigename
        s.sign = file.getString(id, "schild", "SHOP");                      // Schildtext
        s.x = file.getFloat(id, "x", 10.0f);                                // Linke Kante
        s.width = std::max(1.5f, file.getFloat(id, "breite", 3.0f));        // Breite
        s.height = std::max(1.5f, file.getFloat(id, "hoehe", 2.6f));        // Höhe
        s.wares = file.getList(id, "waren");                                // Angebotene Waren
        shops.push_back(s);                                                 // Speichern
    }                                                                       // Ende der Shopschleife

    lights.clear();                                                         // Alte Ampeln löschen
    for (const std::string& id : file.sectionsWithPrefix("Ampel")) {        // Alle Objekte "Ampel..."
        TrafficLight l;                                                     // Neue Ampel
        l.id = id;                                                          // Objektname
        l.x = file.getFloat(id, "x", 20.0f);                                // Linker Straßenrand
        l.streetWidth = std::max(1.0f, file.getFloat(id, "strassen_breite", 2.5f)); // Straßenbreite
        l.redTime = std::max(1.0f, file.getFloat(id, "rot_dauer", 7.0f));   // Rotphase
        l.greenTime = std::max(1.0f, file.getFloat(id, "gruen_dauer", 5.0f)); // Grünphase
        l.clearance = std::max(0.0f, file.getFloat(id, "raeumzeit", 1.0f)); // Räumzeit
        l.carInterval = std::max(0.5f, file.getFloat(id, "auto_intervall", 1.6f)); // Autoabstand
        l.carDuration = std::max(0.4f, file.getFloat(id, "auto_fahrzeit", 1.4f)); // Fahrzeit eines Autos
        l.damage = file.getFloat(id, "schaden", 25.0f);                     // Schaden
        l.buttonWait = std::max(0.5f, file.getFloat(id, "taster_wartezeit", 2.0f)); // Wartezeit nach Taster
        l.green = file.getBool(id, "start_gruen", false);                   // Anfangsphase
        lights.push_back(l);                                                // Speichern
    }                                                                       // Ende der Ampelschleife
    m_initialLights = lights;                                               // Ausgangszustand merken

    anchors.clear();                                                        // Alte Ankerpunkte löschen
    for (const std::string& id : file.sectionsWithPrefix("Anker")) {        // Alle Objekte "Anker..."
        Anchor a;                                                           // Neuer Ankerpunkt
        a.id = id;                                                          // Objektname
        a.x = file.getFloat(id, "x", 30.0f);                                // Position x
        a.y = groundY - file.getFloat(id, "hoehe", 3.5f);                   // Höhe über dem Boden in y umrechnen
        a.frame = file.getBool(id, "gestell", true);                        // Gestell zeichnen?
        a.frameWidth = file.getFloat(id, "gestell_breite", 4.8f);           // Pfostenabstand
        anchors.push_back(a);                                               // Speichern
    }                                                                       // Ende der Ankerschleife

    coins.clear();                                                          // Alte Münzen löschen
    for (const std::string& id : file.sectionsWithPrefix("Muenze")) {       // Alle Objekte "Muenze..."
        Coin c;                                                             // Neue Münze
        c.id = id;                                                          // Objektname
        c.x = file.getFloat(id, "x", 5.0f);                                 // Position x
        c.y = groundY - file.getFloat(id, "hoehe", 0.5f);                   // Höhe über dem Boden in y umrechnen
        c.value = std::max(1, file.getInt(id, "wert", 5));                  // Wert
        coins.push_back(c);                                                 // Speichern
    }                                                                       // Ende der Münzschleife

    buildSolids();                                                          // Kollisionsflächen berechnen
    return true;                                                            // Erfolgreich geladen
} // Ende von load

// Setzt den veränderlichen Zustand zurück (für ein neues Spiel)
void Level::resetRuntime() {                                                // Beginn von resetRuntime
    for (Coin& c : coins) c.collected = false;                              // Alle Münzen wieder sichtbar
    lights = m_initialLights;                                               // Ampeln in den Anfangszustand
} // Ende von resetRuntime

// Aktualisiert alle beweglichen Teile der Karte
void Level::update(float dt) {                                              // Beginn von update
    for (TrafficLight& l : lights) l.update(dt);                            // Alle Ampeln weiterschalten
} // Ende von update

// Berechnet die festen Flächen: Boden überall außer in den Gräben, dazu die Grabensohlen
void Level::buildSolids() {                                                 // Beginn von buildSolids
    m_solids.clear();                                                       // Alte Flächen löschen
    float cursor = -10.0f;                                                  // Start weit links vom Kartenanfang
    for (const Ditch& d : ditches) {                                        // Alle Gräben von links nach rechts
        if (d.x > cursor) m_solids.push_back(RectF{cursor, groundY, d.x - cursor, 20.0f}); // Boden bis zum Graben
        m_solids.push_back(RectF{d.x, groundY + d.depth, d.width, 20.0f});  // Grabensohle
        cursor = std::max(cursor, d.x + d.width);                           // Hinter dem Graben weitermachen
    }                                                                       // Ende der Schleife
    m_solids.push_back(RectF{cursor, groundY, length + 10.0f - cursor, 20.0f}); // Boden bis weit hinter das Kartenende
} // Ende von buildSolids

// Liefert den Graben, in dessen Bereich x liegt
const Ditch* Level::ditchAt(float x) const {                                // Beginn von ditchAt
    for (const Ditch& d : ditches) {                                        // Alle Gräben prüfen
        if (x >= d.x && x <= d.x + d.width) return &d;                      // x liegt im Graben
    }                                                                       // Ende der Schleife
    return nullptr;                                                         // Kein Graben an dieser Stelle
} // Ende von ditchAt

// Liefert den Index der Straße, auf der x liegt
int Level::streetAt(float x) const {                                        // Beginn von streetAt
    for (std::size_t i = 0; i < lights.size(); ++i) {                       // Alle Straßen prüfen
        if (x >= lights[i].x && x <= lights[i].x + lights[i].streetWidth) return static_cast<int>(i); // x liegt auf der Straße
    }                                                                       // Ende der Schleife
    return -1;                                                              // Keine Straße
} // Ende von streetAt

// Tiefenfaktor eines Autos: 0.3 am Horizont, 1.0 auf Höhe der Spielfigur, 1.7 vorne
float Level::carDepth(const Car& car) { return lerp(ROAD_DEPTH_FAR, ROAD_DEPTH_NEAR, car.progress); } // Linear über den Fortschritt

// Zeichnet die perspektivische Straße, die vom Horizont bis zur Bodenkante führt
void Level::drawRoad(Canvas& canvas, const TrafficLight& light, float camX, int tile) const { // Beginn von drawRoad
    const float T = static_cast<float>(tile);                               // Kachelgröße als Kommazahl
    const float W = static_cast<float>(canvas.width());                     // Bildschirmbreite
    float laneY = groundY * T;                                              // Bodenkante in Pixel
    float horizonY = (groundY - ROAD_HORIZON) * T;                          // Horizont der Straße in Pixel
    float centerPx = (light.centerX() - camX) * T;                          // Straßenmitte auf Höhe der Spielfigur
    float halfW = light.streetWidth * 0.5f * T;                             // Halbe Straßenbreite auf Höhe der Spielfigur
    for (int y = static_cast<int>(horizonY); y < static_cast<int>(laneY); ++y) { // Jede Pixelzeile der Straße
        float t = (static_cast<float>(y) + 0.5f - horizonY) / (laneY - horizonY); // 0 am Horizont, 1 an der Bodenkante
        float f = lerp(ROAD_DEPTH_FAR, 1.0f, t);                            // Tiefenfaktor dieser Zeile
        float cx = W * 0.5f + (centerPx - W * 0.5f) * f;                    // Straßenmitte in dieser Tiefe
        float hw = halfW * f;                                               // Halbe Breite in dieser Tiefe
        int x0 = static_cast<int>(cx - hw);                                 // Linker Straßenrand
        int x1 = static_cast<int>(cx + hw);                                 // Rechter Straßenrand
        canvas.fillRect(x0, y, x1 - x0, 1, rgba(70, 70, 78));               // Asphalt
        int edge = std::max(1, static_cast<int>(hw * 0.08f));               // Breite des Bordsteins
        canvas.fillRect(x0, y, edge, 1, rgba(160, 160, 165));               // Linker Bordstein
        canvas.fillRect(x1 - edge, y, edge, 1, rgba(160, 160, 165));        // Rechter Bordstein
        if (static_cast<int>(f * 14.0f) % 2 == 0) {                         // Gestrichelte Mittellinie
            int lw = std::max(1, static_cast<int>(hw * 0.05f));             // Linienbreite
            canvas.fillRect(static_cast<int>(cx) - lw / 2, y, lw, 1, rgba(240, 240, 230)); // Linie zeichnen
        }                                                                   // Ende der Mittellinie
    }                                                                       // Ende der Zeilenschleife
} // Ende von drawRoad

// Zeichnet ein Auto in seiner aktuellen Tiefe (je näher, desto größer)
void Level::drawCar(Canvas& canvas, const Assets& assets, const TrafficLight& light, const Car& car, float camX) const { // Beginn von drawCar
    const float T = static_cast<float>(assets.tileSize());                  // Kachelgröße
    const float W = static_cast<float>(canvas.width());                     // Bildschirmbreite
    float f = carDepth(car);                                                // Tiefenfaktor des Autos
    float laneY = groundY * T;                                              // Bodenkante in Pixel
    float horizonY = (groundY - ROAD_HORIZON) * T;                          // Horizont in Pixel
    float bottom = horizonY + (laneY - horizonY) * (f - ROAD_DEPTH_FAR) / (1.0f - ROAD_DEPTH_FAR); // Unterkante des Autos
    float centerPx = (light.centerX() - camX) * T;                          // Straßenmitte auf Höhe der Spielfigur
    float cx = W * 0.5f + (centerPx - W * 0.5f) * f - light.streetWidth * 0.18f * T * f; // Rechtsverkehr: Autos kommen auf der linken Bildseite
    const Image& img = assets.get("auto_" + std::to_string(car.variant));   // Bild der passenden Farbe
    int w = static_cast<int>(static_cast<float>(img.width) * f);            // Skalierte Breite
    int h = static_cast<int>(static_cast<float>(img.height) * f);           // Skalierte Höhe
    canvas.blitScaled(img, RectI{0, 0, img.width, img.height}, RectI{static_cast<int>(cx) - w / 2, static_cast<int>(bottom) - h, w, h}); // Skaliert zeichnen
} // Ende von drawCar

// Zeichnet alles, was hinter der Spielfigur liegt
void Level::drawBack(Canvas& canvas, const Assets& assets, float camX, float time) const { // Beginn von drawBack
    const int tile = assets.tileSize();                                     // Kachelgröße in Pixel
    const float T = static_cast<float>(tile);                               // Kachelgröße als Kommazahl
    const int W = canvas.width();                                           // Bildschirmbreite
    const int H = canvas.height();                                          // Bildschirmhöhe
    const float viewLeft = camX - 3.0f;                                     // Linker Rand des sichtbaren Bereichs (mit Reserve)
    const float viewRight = camX + static_cast<float>(W) / T + 3.0f;        // Rechter Rand des sichtbaren Bereichs (mit Reserve)
    const int groundPx = static_cast<int>(groundY * T);                     // Bodenkante in Pixel
    auto toPx = [&](float worldX) { return static_cast<int>(std::floor((worldX - camX) * T + 0.5f)); }; // Weltposition -> Bildschirmspalte

    // 1. Perspektivische Straßen und Autos hinter der Fußgängerfurt
    for (const TrafficLight& light : lights) {                              // Alle Straßen
        if (light.x + light.streetWidth < viewLeft - 4.0f || light.x > viewRight + 4.0f) continue; // Weit außerhalb -> überspringen
        drawRoad(canvas, light, camX, tile);                                // Straße bis zum Horizont
        if (light.green) {                                                  // Bei Grün wartet ein Auto vor der Furt
            Car waiting;                                                    // Wartendes Auto
            waiting.progress = 0.2f;                                        // Steht weiter hinten
            waiting.variant = (light.carCounter + 1) % 3;                   // Farbe
            drawCar(canvas, assets, light, waiting, camX);                  // Zeichnen
        }                                                                   // Ende wartendes Auto
        for (const Car& car : light.cars) {                                 // Fahrende Autos
            if (carDepth(car) < 1.0f) drawCar(canvas, assets, light, car, camX); // Nur Autos hinter der Spielfigur
        }                                                                   // Ende der Autoschleife
    }                                                                       // Ende der Straßenschleife

    // 2. Shops
    for (const ShopSpot& shop : shops) {                                    // Alle Shops
        if (shop.x + shop.width < viewLeft || shop.x > viewRight) continue; // Außerhalb -> überspringen
        const Image& img = assets.get("shop_" + shop.id);                   // Bild des Shops
        canvas.blit(img, toPx(shop.x), groundPx - img.height);              // Unterkante auf den Boden stellen
    }                                                                       // Ende der Shopschleife

    // 3. Ampelmasten
    for (const TrafficLight& light : lights) {                              // Alle Ampeln
        if (light.poleX() < viewLeft || light.poleX() > viewRight) continue; // Außerhalb -> überspringen
        bool showGreen = light.green;                                       // Grün anzeigen?
        if (light.green && light.remaining() < 1.5f) showGreen = static_cast<int>(time * 6.0f) % 2 == 0; // Am Ende der Grünphase blinken
        const Image& img = assets.get(showGreen ? "ampel_gruen" : "ampel_rot"); // Passendes Bild
        canvas.blit(img, toPx(light.poleX()) - img.width / 2, groundPx - img.height); // Mast auf den Bordstein stellen
    }                                                                       // Ende der Ampelschleife

    // 4. Innenseite der Gräben
    for (const Ditch& d : ditches) {                                        // Alle Gräben
        if (d.x + d.width < viewLeft || d.x > viewRight) continue;          // Außerhalb -> überspringen
        int px0 = toPx(d.x);                                                // Linker Rand in Pixel
        int px1 = toPx(d.x + d.width);                                      // Rechter Rand in Pixel
        int bottom = static_cast<int>((groundY + d.depth) * T);             // Grabensohle in Pixel
        int wall = std::max(2, tile * 6 / 100);                             // Dicke der Grabenwände
        canvas.fillRect(px0, groundPx, px1 - px0, bottom - groundPx, rgba(70, 48, 32)); // Dunkle Grabenerde
        canvas.fillRect(px0, groundPx, wall, bottom - groundPx, rgba(98, 68, 44)); // Linke Wand
        canvas.fillRect(px1 - wall, groundPx, wall, bottom - groundPx, rgba(98, 68, 44)); // Rechte Wand
        int floorH = std::max(2, tile * 12 / 100);                          // Höhe der Sohle
        canvas.fillRect(px0, bottom - floorH, px1 - px0, floorH, d.water ? rgba(60, 70, 50) : rgba(95, 80, 55)); // Grabensohle
        canvas.fillRect(px0, bottom, px1 - px0, H - bottom, rgba(100, 70, 45)); // Erde unter der Sohle
        if (!d.water) {                                                     // Trockener Graben: Geröll auf der Sohle
            SimpleRandom rnd(static_cast<std::uint32_t>(d.x * 10.0f) + 3u); // Fester Zufall
            for (int i = 0; i < 6; ++i) {                                   // Sechs Steine
                int sx = px0 + wall + rnd.rangeInt(0, std::max(1, px1 - px0 - 2 * wall)); // Position x
                int r = std::max(1, rnd.rangeInt(tile / 20, tile / 10));    // Größe
                canvas.fillEllipse(sx, bottom - floorH, r, r * 2 / 3, rgba(130, 120, 110)); // Stein zeichnen
            }                                                               // Ende der Steinschleife
        }                                                                   // Ende Geröll
    }                                                                       // Ende der Grabenschleife

    // 5. Boden (Gehweg) und Straßenbelag
    const Image& ground = assets.get("boden");                              // Gehweg-Kachel
    const Image& street = assets.get("strasse");                            // Straßen-Kachel
    auto drawTiles = [&](float x0, float x1, const Image& img) {            // Hilfsfunktion: Kacheln zwischen x0 und x1
        x0 = std::max(x0, viewLeft);                                        // Auf den sichtbaren Bereich begrenzen
        x1 = std::min(x1, viewRight);                                       // Auf den sichtbaren Bereich begrenzen
        if (x0 >= x1) return;                                               // Nichts sichtbar
        int px0 = toPx(x0);                                                 // Linker Rand in Pixel
        int px1 = toPx(x1);                                                 // Rechter Rand in Pixel
        canvas.setClip(RectI{px0, 0, px1 - px0, H});                        // Nur innerhalb des Abschnitts zeichnen
        for (int k = static_cast<int>(std::floor(x0)); k < static_cast<int>(std::ceil(x1)); ++k) { // Alle Kachelspalten
            canvas.blit(img, toPx(static_cast<float>(k)), groundPx);        // Kachel zeichnen
        }                                                                   // Ende der Kachelschleife
        canvas.fillRect(px0, groundPx + tile, px1 - px0, H - groundPx - tile, rgba(100, 70, 45)); // Tiefere Erde
        canvas.resetClip();                                                 // Zeichenbereich freigeben
    };                                                                      // Ende der Hilfsfunktion
    for (const RectF& s : m_solids) {                                       // Alle festen Flächen
        if (s.y > groundY + 0.01f) continue;                                // Grabensohlen nicht als Gehweg zeichnen
        drawTiles(s.x, s.right(), ground);                                  // Gehweg-Kacheln
    }                                                                       // Ende der Bodenschleife
    for (const TrafficLight& light : lights) drawTiles(light.x, light.x + light.streetWidth, street); // Straßenbelag darüber

    // 6. Stahlgestelle mit Ankerringen
    for (const Anchor& a : anchors) {                                       // Alle Ankerpunkte
        if (a.x + a.frameWidth < viewLeft || a.x - a.frameWidth > viewRight) continue; // Außerhalb -> überspringen
        const Image& ring = assets.get("anker");                            // Bild des Rings
        int beamY = static_cast<int>((a.y - 0.3f) * T);                     // Oberkante des Querträgers
        int beamH = std::max(3, tile / 10);                                 // Höhe des Querträgers
        if (a.frame) {                                                      // Gestell zeichnen
            int postW = std::max(3, tile * 9 / 100);                        // Breite eines Pfostens
            float posts[2] = {a.x - a.frameWidth * 0.5f, a.x + a.frameWidth * 0.5f}; // Positionen der Pfosten
            for (float post : posts) {                                      // Beide Pfosten
                float bottomY = groundY;                                    // Normalerweise steht der Pfosten auf dem Boden
                if (const Ditch* d = ditchAt(post)) bottomY = groundY + d->depth; // Im Graben bis zur Sohle
                int px = toPx(post);                                        // Position in Pixel
                canvas.fillRect(px - postW / 2, beamY, postW, static_cast<int>(bottomY * T) - beamY, rgba(110, 115, 125)); // Pfosten
                canvas.fillRect(px - postW / 2, beamY, std::max(1, postW / 4), static_cast<int>(bottomY * T) - beamY, rgba(160, 165, 175)); // Glanzkante
                int dir = post < a.x ? 1 : -1;                              // Strebe zeigt zur Mitte
                canvas.line(px, beamY + tile / 2, px + dir * tile / 2, beamY + beamH, rgba(110, 115, 125), std::max(2, postW / 2)); // Schräge Strebe
            }                                                               // Ende der Pfostenschleife
            int bx0 = toPx(posts[0]) - postW / 2;                           // Linke Kante des Trägers
            int bx1 = toPx(posts[1]) + postW / 2;                           // Rechte Kante des Trägers
            canvas.fillRect(bx0, beamY, bx1 - bx0, beamH, rgba(120, 125, 135)); // Querträger
            canvas.fillRect(bx0, beamY, bx1 - bx0, std::max(1, beamH / 4), rgba(170, 175, 185)); // Glanzkante oben
            for (int rx = bx0 + tile / 6; rx < bx1; rx += tile / 3) canvas.fillCircle(rx, beamY + beamH / 2, std::max(1, tile / 40), rgba(70, 70, 80)); // Nieten
        }                                                                   // Ende Gestell
        canvas.blit(ring, toPx(a.x) - ring.width / 2, static_cast<int>(a.y * T) - ring.height * 6 / 10); // Ring am Ankerpunkt
    }                                                                       // Ende der Ankerschleife

    // 7. Zielflagge
    if (goalX > viewLeft && goalX < viewRight) {                            // Ziel sichtbar?
        const Image& flag = assets.get("ziel");                             // Bildstreifen der Flagge
        int frames = 4;                                                     // Anzahl der Einzelbilder
        int fw = flag.width / frames;                                       // Breite eines Einzelbildes
        int frame = static_cast<int>(time * 6.0f) % frames;                 // Aktuelles Einzelbild
        canvas.blitRegion(flag, RectI{frame * fw, 0, fw, flag.height}, toPx(goalX) - fw / 6, groundPx - flag.height); // Flagge zeichnen
    }                                                                       // Ende Zielflagge

    // 8. Münzen
    const Image& coinImg = assets.get("muenze");                            // Bildstreifen der Münze
    int coinSize = coinImg.height;                                          // Größe eines Einzelbildes
    int coinFrames = std::max(1, coinImg.width / std::max(1, coinSize));    // Anzahl der Einzelbilder
    for (std::size_t i = 0; i < coins.size(); ++i) {                        // Alle Münzen
        const Coin& c = coins[i];                                           // Aktuelle Münze
        if (c.collected || c.x < viewLeft || c.x > viewRight) continue;     // Eingesammelt oder unsichtbar
        int frame = static_cast<int>(time * 10.0f + static_cast<float>(i) * 2.0f) % coinFrames; // Drehbild
        float bob = std::sin(time * 3.0f + static_cast<float>(i)) * 0.04f;  // Leichtes Schweben
        canvas.blitRegion(coinImg, RectI{frame * coinSize, 0, coinSize, coinSize}, toPx(c.x) - coinSize / 2, static_cast<int>((c.y + bob) * T) - coinSize / 2); // Münze zeichnen
    }                                                                       // Ende der Münzschleife

    // 9. Schwimmender Müll (wird danach teilweise vom Wasser überdeckt)
    for (const Ditch& d : ditches) {                                        // Alle Gräben
        if (!d.water || d.x + d.width < viewLeft || d.x > viewRight) continue; // Nur sichtbare Wassergräben
        for (const TrashItem& t : d.trash) {                                // Alle Müllteile
            const Image& img = assets.get("muell_" + std::to_string(t.type)); // Bild der Müllart
            float wx = d.x + t.offset + std::sin(time * 0.4f + t.phase) * 0.1f; // Langsames Treiben
            float wy = groundY + d.waterLevel + std::sin(time * t.speed + t.phase) * 0.03f; // Auf und ab schaukeln
            canvas.blit(img, toPx(wx) - img.width / 2, static_cast<int>(wy * T) - img.height / 2); // Müll zeichnen
        }                                                                   // Ende der Müllschleife
    }                                                                       // Ende der Grabenschleife
} // Ende von drawBack

// Zeichnet alles, was vor der Spielfigur liegt (Wasser und Autos ganz vorne)
void Level::drawFront(Canvas& canvas, const Assets& assets, float camX, float time) const { // Beginn von drawFront
    const int tile = assets.tileSize();                                     // Kachelgröße in Pixel
    const float T = static_cast<float>(tile);                               // Kachelgröße als Kommazahl
    const int W = canvas.width();                                           // Bildschirmbreite
    for (const Ditch& d : ditches) {                                        // Alle Gräben
        if (!d.water) continue;                                             // Nur Gräben mit Wasser
        int px0 = static_cast<int>(std::floor((d.x - camX) * T + 0.5f));   // Linker Rand in Pixel
        int px1 = static_cast<int>(std::floor((d.x + d.width - camX) * T + 0.5f)); // Rechter Rand in Pixel
        if (px1 < 0 || px0 > W) continue;                                   // Außerhalb -> überspringen
        int wall = std::max(2, tile * 6 / 100);                             // Dicke der Grabenwände
        int bottom = static_cast<int>((groundY + d.depth) * T);             // Grabensohle in Pixel
        float surface = (groundY + d.waterLevel) * T;                       // Wasseroberfläche in Pixel
        for (int x = std::max(0, px0 + wall); x < std::min(W, px1 - wall); ++x) { // Jede Spalte im Graben
            float worldX = camX + static_cast<float>(x) / T;                // Weltposition der Spalte
            int y0 = static_cast<int>(surface + std::sin(worldX * 7.8f + time * 2.5f) * 0.025f * T); // Wellenhöhe
            canvas.fillRect(x, y0, 1, bottom - y0, rgba(40, 110, 170, 165)); // Halbdurchsichtiges Wasser
            canvas.fillRect(x, y0, 1, std::max(1, tile / 40), rgba(175, 225, 245, 230)); // Helle Wellenlinie
        }                                                                   // Ende der Spaltenschleife
    }                                                                       // Ende der Grabenschleife
    for (const TrafficLight& light : lights) {                              // Alle Straßen
        for (const Car& car : light.cars) {                                 // Alle Autos
            if (carDepth(car) >= 1.0f) drawCar(canvas, assets, light, car, camX); // Autos vor der Spielfigur
        }                                                                   // Ende der Autoschleife
    }                                                                       // Ende der Straßenschleife
} // Ende von drawFront
