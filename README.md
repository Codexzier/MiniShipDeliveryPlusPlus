# Mini Ship Delivery

Ein isometrisches Handels- und Seefahrtspiel in C++17 (Ansicht ähnlich wie bei Diablo).
Als Kapitän läufst du zu Fuß über Inseln und segelst mit deinem Schiff zwischen ihnen hin und her:
Waren günstig einkaufen, das Schiff im Minispiel geschickt beladen, gegen Wind, Wetter, Piraten und
nächtliche Seeungeheuer bestehen und die Ware mit Gewinn verkaufen. Mit dem Geld kaufst du Verbesserungen,
größere Schiffe und heuerst Crew an. Treibgut (Artefakte) bringt im Museum Geld und Entdeckerpunkte.

Alles wird von der CPU gezeichnet, ein Grafikbeschleuniger wird nicht benötigt: Die 3D-Modelle aus den
Kenney-Paketen (OBJ und animierte FBX-Figuren) werden mit einem eigenen Software-Rasterizer einmal in
isometrische Sprites gerendert und zwischengespeichert. SDL2 dient nur für Fenster, Eingaben, Bilder laden,
Ton und das Kopieren des fertigen Bildes ins Fenster (Software-Renderer von SDL).
Jede Codezeile ist auf Deutsch kommentiert. Alle Spielwerte stehen in Textdateien im Ordner `data/`.

## Voraussetzungen

- C++17-Compiler (GCC oder Clang), CMake ab 3.16
- SDL2, SDL2_image, zlib (Pflicht), SDL2_mixer (optional, ohne bleibt das Spiel stumm)

| System | Installation |
|---|---|
| Ubuntu, Debian, Raspberry Pi OS | `sudo apt install build-essential cmake libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev zlib1g-dev` |
| Fedora | `sudo dnf install gcc-c++ cmake SDL2-devel SDL2_image-devel SDL2_mixer-devel zlib-devel` |
| Arch Linux | `sudo pacman -S base-devel cmake sdl2 sdl2_image sdl2_mixer zlib` |

Läuft auf PCs (x86/x64) und auf ARM-Systemen wie dem Raspberry Pi 4 oder neuer.

## Bauen und Starten

**CLion:** *File → Open*, den Projektordner wählen, Konfiguration `MiniShipDelivery` starten.

**Konsole:**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/MiniShipDelivery
```

Das Programm sucht `data/` und `assets/` neben der ausführbaren Datei, eine Ebene darüber, im aktuellen Ordner
und zuletzt im Quellordner. Spielstand und Punkteliste landen in `speicher/`.

## Steuerung

| Taste | Zu Fuß | Auf dem Schiff |
|---|---|---|
| W A S D | laufen | W/S: Fahrstufe (Segel bzw. Maschine, ganz unten rückwärts), A/D: Ruder |
| Linksklick | hinlaufen; Gebäude, Händler oder Schiff anklicken = hingehen und betreten | Kurs setzen (Autopilot fährt um Inseln herum) |
| E | Tür öffnen / am Stegende zum Schiff | Anlegen (nur am Hafensteg) |
| T | – | Autopilot zum gesetzten Ziel an/aus |
| Leertaste | – | Kanonen auf das nächste Ziel |
| F | – | Hilfsmaschine an/aus (Hybrid-Schiff) |
| M / I / Z | Seekarte / Schiff & Crew / Hafenzeitung | ebenso |
| Mausrad, + / - | Zoom | Zoom |
| F1, Esc, F3 | Hilfe, Pausemenü, Bildzeit anzeigen | ebenso |

Im Minispiel „Schiff beladen“: Linksklick nimmt eine Ware vom Steg bzw. legt sie ab, R oder Rechtsklick dreht,
Esc kehrt zum Steg zurück. Alle Tasten lassen sich in `data/spiel.txt` (Abschnitt `[Steuerung]`) ändern.

## Spielablauf

- **Inseln und Häfen:** Jede Insel hat genau einen Hafensteg. Nur dort legt das Schiff an, und nur über den Steg
  geht der Kapitän an Land oder an Bord.
- **Kontor:** kauft alles, verkauft unbegrenzt die Waren, die auf der Insel entstehen. Eigene Waren sind billig,
  gesuchte teuer; viele Verkäufe drücken den Preis (erholt sich täglich). Hier gibt es auch Lieferaufträge mit Frist
  (der nächste Termin steht oben links im HUD) und Kohle.
- **Händler** (Marktstand oder Laden; Gemüse, Obst, Fertigwaren): kaufen und verkaufen nur begrenzte Mengen, zahlen
  aber mehr als das Kontor. Bedarf: täglich voll oder langsam steigend. Frische verderbliche Ware bringt einen Zuschlag,
  jede Lieferung füllt das Angebot des Händlers auf. Der Bestand erholt sich mit der Zeit.
- **Hersteller** (Tischler, Uhrmacher, Kartenzeichner): kaufen nur Rohwaren (mehr als das Kontor, begrenzt durch
  das Lager) und verkaufen nur ihre Produkte; wie viel sie herstellen, hängt von den gelieferten Rohwaren ab.
- **Steg und Laderaum:** Gekaufte Ware liegt am Steg. Im Minispiel wird sie in den Laderaum gepackt – Formen und
  Gewichte unterscheiden sich. Zu viel Gewicht auf einer Seite gibt Schlagseite; wird die Grenze überschritten,
  kentert das Schiff beim Ablegen und die Ladung geht über Bord. Hafenarbeiter packen gegen Gebühr automatisch.
- **Schiff:** Frachtgröße, Geschwindigkeit, Reparatur, Panzerung, Abwehr, Wendigkeit, Sichtweite (Scanner),
  Handel (100 %), Rumpf, Tiefgang. Antrieb Wind, Dampf (Kohle, Maschinist nötig) oder Hybrid. Klassen: Kutter,
  Schaluppe, Brigg, Dampfer, Klipper, Fregatte. Verbesserungen und neue Schiffe gibt es in der Werft.
- **Crew** (Taverne): Matrose, Navigator, Maschinist, Kanonier, Schiffszimmermann, Ausguck, Zahlmeister, Smutje.
  Jede Rolle gibt Boni; Nachteile gibt es nur, wenn die Mindestbesatzung unterschritten ist. Heuer täglich um 6 Uhr.
- **Wetter und Zeit:** Windrichtung und -stärke wechseln; Segler sind vor dem Wind schnell und kommen gegen den Wind
  kaum voran, im Sturm schaden volle Segel. Regen und Nebel (weniger Sicht), Tag und Nacht. Die Hafenzeitung (Z)
  hat eine Vorhersage (mit Navigator genauer). Geschäfte haben Öffnungszeiten, die Taverne bietet Übernachtung.
- **Gefahren:** Riffe und flaches Wasser (Tiefgang!), Piraten abseits der Bojen-Routen (Kanonenfeuer, Entern mit
  Ladungsraub, Untergang), nachts Seeungeheuer in bestimmten Gewässern – nur schnelle Schiffe entkommen.
  Ein gesunkenes Schiff wird im letzten Hafen durch einen Kutter ersetzt, Ladung und Artefakte sind verloren.
- **Artefakte:** treiben auf See und erscheinen im Scanner. Verkauf im Museum gegen Credits und Entdeckerpunkte;
  mehr Punkte schalten wertvollere Artefakte frei (seltene treiben oft in gefährlichen Gewässern).
- **Punkte:** Credits + Schiffswert + Ladung + 10 × Entdeckerpunkte (Score-Liste im Hauptmenü).

## HUD

| Ecke | Mit dem Schiff | Als Figur |
|---|---|---|
| oben links | Uhrzeit, Tag/Nacht, Wetter, nächster Termin | ebenso |
| oben rechts | Rumpf, Credits | Gesundheit des Kapitäns, Credits |
| unten links | Windstärke und -richtung, Wassertiefe, Fahrstufe | Dialogfenster |
| unten rechts | Richtungspfeil zum gesetzten Ziel | ebenso |

## Dateien in `data/`

| Datei | Inhalt |
|---|---|
| `spiel.txt` | Fenster, Grafik (Kachelgröße, Zoom, Kantenglättung), Zeit, Startwerte, Steuerung, Preise, Gefahren, Wetter, Minispiel |
| `welt.txt` | Karte: Inseln (Lage, Größe, Hafenrichtung, Gebäude, Markt, eigene und gesuchte Waren), Riffe, Piraten- und Monstergebiete, Handelsrouten |
| `waren.txt` | Waren mit Kategorie, Grundpreis, Gewicht, Haltbarkeit und Form im Laderaum |
| `schiffe.txt` | Schiffsklassen mit allen Attributen |
| `crew.txt` | Crew-Rollen mit Boni, Nachteilen und Heuer, Namen für Bewerber |
| `upgrades.txt` | Werft-Verbesserungen |
| `artefakte.txt` | Artefakte und Entdecker-Schwellen |
| `haendler.txt` / `hersteller.txt` | Händlerarten bzw. Hersteller mit Rezepten |
| `modelle.txt` / `figuren.txt` | 3D-Modelle (OBJ, Skalierung, Farben, zusammengesetzte Modelle, Hausgenerator) und animierte Figuren (FBX) |
| `audio.txt` | Pfade zu Musik und Geräuschen (fehlende Dateien werden durch synthetische Platzhalter ersetzt) |

## Leistung

Auf einem aktuellen x86-PC braucht ein Bild bei 1280 × 720 rund 10–15 ms (F3 zeigt den Wert).
Für schwächere Rechner wie den Raspberry Pi in `data/spiel.txt` z. B. `kachel_groesse = 64`,
`zoomstufen = 48, 64`, `kantenglaettung = 1` und ein kleineres Fenster einstellen.

## Quellcode (`src/`)

| Datei | Aufgabe |
|---|---|
| `main.cpp`, `Game.*` | Start, Hauptschleife, Eingaben, Menüablauf, Speichern, Punkteliste |
| `GameUpdate.cpp` | Figur, Schiffsphysik (Wind, Segel, Maschine, Tiefgang), Anlegen, Autopilot, Piraten, Seeungeheuer, Kanonen |
| `GameRender.cpp`, `GameHud.cpp`, `LoadShip.cpp` | Weltdarstellung mit Licht und Wetter, HUD und Fenster, Minispiel |
| `GameState.*`, `GameData.*` | Spiellogik (Zeit, Wetter, Wirtschaft, Crew, Laderaum, Aufträge, Artefakte, Spielstand) und feste Daten |
| `World.*` | Inselgenerator, Häfen, Gebäude, Bewuchs, Wegsuche zu Fuß und auf See |
| `Rasterizer.*`, `Mesh.*`, `FbxLoader.*`, `Math3D.h`, `ModelLibrary.*` | Software-3D: OBJ/MTL, FBX mit Skelettanimation, isometrisches Rendern, Sprite-Zwischenspeicher |
| `Graphics.*`, `Font.*`, `Ui.*`, `ImageIO.*`, `Audio.*`, `PropertyFile.*` | Zeichnen, Pixelschrift, Oberfläche (Interface Pack), Bilder, Ton, Textdateien |

## Lizenzen der Grafiken

Alle 3D-Modelle, Figuren und Oberflächengrafiken in `assets/` stammen von Kenney (www.kenney.nl, CC0).
