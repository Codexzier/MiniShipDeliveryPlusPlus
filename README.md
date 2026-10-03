# Mini Ship Delivery

Ein Pixelart-Sidescroller in C++17. Ein Paketbote läuft durch eine kurze Teststrecke,
kauft im Shop einen Enterhaken, wartet an der Fußgängerampel auf Grün und schwingt
mit dem Enterhaken über einen Wassergraben, in dem Müll schwimmt.

Das Spiel rendert komplett in Software (CPU), ein Grafikbeschleuniger wird nicht benötigt.
SDL2 wird nur für Fenster, Tastatur/Maus und das Kopieren des fertigen Bildes ins Fenster benutzt
(mit dem Software-Renderer von SDL). Jede Zeile des Quellcodes ist auf Deutsch kommentiert.

## Voraussetzungen

- C++17-Compiler (GCC oder Clang)
- CMake ab Version 3.16
- SDL2-Entwicklungspaket

| System | Installation |
|---|---|
| Ubuntu, Debian, Raspberry Pi OS | `sudo apt install build-essential cmake libsdl2-dev` |
| Fedora | `sudo dnf install gcc-c++ cmake SDL2-devel` |
| Arch Linux | `sudo pacman -S base-devel cmake sdl2` |

Läuft auf PCs (x86/x64) und auf ARM-Systemen wie dem Raspberry Pi 4 oder neuer.

## Bauen und Starten

### CLion

1. *File → Open* und den Projektordner (mit `CMakeLists.txt`) öffnen.
2. Oben rechts die Konfiguration `MiniShipDelivery` wählen und auf *Run* klicken.

Das Programm findet den Ordner `data/` automatisch (neben der Programmdatei, eine oder zwei
Ebenen darüber, im Arbeitsverzeichnis oder im Projektordner). Änderungen an den Textdateien
in `data/` wirken beim nächsten Start, ohne neu zu kompilieren.

### Terminal

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/MiniShipDelivery
```

### Raspberry Pi

Funktioniert genauso. Für flüssiges Spielen auf dem Pi in `data/spiel.txt` eine kleinere
Spritegröße einstellen, z.B. `kachel_groesse = 64` (ein Viertel der Pixel) oder `32`.
Bei 1280×720 und Kachelgröße 128 braucht das Spiel auf einem PC ungefähr 30 % eines CPU-Kerns,
bei 64 etwa 15 % und bei 32 etwa 8 %.

## Steuerung

| Taste | Aktion |
|---|---|
| A / D | nach links / nach rechts laufen |
| Leertaste | springen (am Seil: loslassen) |
| E | interagieren (Shop betreten, Ampel-Taster drücken, Shop schließen) |
| Q | Enterhaken werfen (am Seil: loslassen) |
| A / D am Seil | Schwung holen |
| 1 bis 8 | Gegenstand im Inventarfeld benutzen (oder anklicken) |
| Esc | Frage „Spiel beenden?“ öffnen |
| F12 | Bildschirmfoto nach `screenshots/` speichern |

Mit der Maus lassen sich alle Knöpfe bedienen. Wenn du mit der Maus über Gegenstände im Shop
oder im Inventar fährst, erscheint ein Tooltip mit Name, Beschreibung und Preis.

Die Tasten lassen sich in `data/spiel.txt` im Abschnitt `[Steuerung]` ändern.

## Ablauf der Teststrecke

1. **Start:** zwei Münzen, danach ein schmaler, trockener **Graben** zum Drüberspringen.
2. **Shop („Krämerladen“):** vor der Tür `E` drücken. Die Karte wechselt nicht, es öffnet sich das
   Shop-Menü mit 4×4 Feldern: Einkaufsbeutel, Enterhaken und Dose Energy. Beim Schließen wird
   automatisch gespeichert.
3. **Straße mit Fußgängerampel:** Bei Rot fahren Autos über die Furt (sie kommen aus der Tiefe
   auf den Betrachter zu). Wer bei Rot losläuft, wird angefahren und verliert Leben. Mit `E` am
   Ampelmast drückst du den Taster und es wird schneller grün.
4. **Wassergraben mit Müll:** zu breit zum Springen. Mit gekauftem Enterhaken an den Ankerring
   am Stahlgestell werfen (`Q`), Schwung holen (`A`/`D`) und mit `Leertaste` loslassen.
5. Ein paar Schritte weiter wartet das **Ziel**. Dann erscheint die Meldung, dass das Level-Ende
   erreicht wurde, und die Frage, ob du von vorne beginnen möchtest.

Fällt die Figur in einen Graben, verliert sie Leben und erscheint am Grabenrand wieder.
Bei 0 Leben fragt das Spiel, ob du neu beginnen möchtest.

## Bildschirmaufbau

- **Hauptmenü:** Titel „Mini Ship Delivery“ in der oberen Hälfte, darunter die Knöpfe
  *Neues Spiel*, *Letzter Spielstand* und *Beenden*.
- **Menüleiste oben** über die ganze Breite: ganz links der Beenden-Knopf (mit Sicherheitsabfrage),
  links die Ausdauer, in der Mitte die Lebensleiste, rechts die Coins, ganz rechts die Uhrzeit.
- **Inventar unten links:** 4 Felder nebeneinander, 2 Reihen.
- **Hintergrund:** vier Parallax-Ebenen (Wolken, Berge, Stadt, Häuser), die sich langsamer als
  der Vordergrund bewegen und so Tiefe erzeugen.

## Eigenschaften in Textdateien

Alle Eigenschaften von Objekten, Gegenständen und der Spielfigur stehen in Textdateien im Ordner
`data/`. In eckigen Klammern steht der Name des Objekts, darunter folgen Eigenschaft und Wert:

```ini
[Spieler]
lauf_geschwindigkeit = 3.2
sprung_geschwindigkeit = 8.6
```

| Datei | Inhalt |
|---|---|
| `data/spiel.txt` | Fenster, Spritegröße (128/64/32), sichtbare Breite, Steuerung, erste Karte |
| `data/spieler.txt` | Spielfigur (Laufen, Springen, Leben, Ausdauer, Coins) und Physik des Enterhakens |
| `data/animationen.txt` | Zeile, Bildanzahl und Geschwindigkeit jeder Animation im Sprite-Sheet |
| `data/gegenstaende.txt` | Gegenstände mit Name, Beschreibung, Preis, Symbol und Wirkung |
| `data/level1.txt` | Karte: Gräben, Shop, Ampel, Ankerpunkt, Münzen, Ziel |

Zeilen mit `#` sind Kommentare, Kommazahlen dürfen Punkt oder Komma enthalten. In der Karte
werden Objekte am Anfang ihres Namens erkannt (`Graben…`, `Shop…`, `Ampel…`, `Anker…`, `Muenze…`).
Neue Objekte lassen sich einfach ergänzen, z.B. `[Graben_3]` oder `[Muenze_9]`.

Der Spielstand wird im gleichen Format in `speicher/spielstand.txt` geschrieben.

## Eigene Sprites statt Platzhalter

Da noch keine Grafiken vorliegen, zeichnet das Spiel beim Start Platzhalter per Programmcode.
Die Spielfigur wird dabei aus Gelenkwinkeln zusammengesetzt, damit die Bewegungsabläufe
erkennbar sind. Auch Shop, Ampel, Autos, Müll, Münzen, Zielflagge und Hintergründe sind Platzhalter.

- Beim Start werden alle Platzhalter als BMP-Vorlagen nach `assets_dummies/<Größe>/` exportiert
  (abschaltbar mit `dummies_exportieren = 0`).
- Eine Vorlage bearbeiten oder neu zeichnen und unter gleichem Namen nach `assets/<Größe>/` legen,
  z.B. `assets/128/spieler.bmp`. Eigene Dateien haben immer Vorrang vor den Platzhaltern.
- Magenta (R 255, G 0, B 255) gilt als durchsichtig. 32-Bit-BMPs mit Alphakanal gehen ebenfalls.
- Hat ein eigenes Bild eine andere Größe, wird es passend skaliert (mit Hinweis in der Konsole).

Sprite-Sheet der Spielfigur (`spieler.bmp`, Standard 1280×1280 Pixel bei 128er Sprites):

| Zeile | Animation | Bilder |
|---|---|---|
| 0 | Laufen nach rechts | 10 |
| 1 | Laufen nach links | 10 |
| 2 | Springen (2 aufwärts, 2 abwärts) | 4 |
| 3 | Landen | 4 |
| 4 | Enterhaken werfen | 4 |
| 5 | Hochschwingen | 4 |
| 6 | Aufschwung | 2 |
| 7 | Abschwung | 2 |
| 8 | Landen nach dem Enterhaken | 4 |
| 9 | Stehen (zusätzlich) | 2 |

Zeilen und Bildanzahlen lassen sich in `data/animationen.txt` ändern.

## Quellcode

| Datei | Aufgabe |
|---|---|
| `src/main.cpp` | Einstiegspunkt |
| `src/Game.*` | Spielschleife, Hauptmenü, Popups, Spiellogik, Speichern |
| `src/Graphics.*` | Bildspeicher und Zeichenfunktionen (Software-Rendering) |
| `src/Font.*` | Eingebaute Pixelschrift mit Umlauten |
| `src/PropertyFile.*` | Lesen/Schreiben der Eigenschafts-Textdateien |
| `src/ImageIO.*`, `src/Assets.*` | BMP-Dateien und Bildverwaltung |
| `src/SpriteFactory.*` | Platzhalter-Grafiken |
| `src/Animation.*` | Animationen der Figur |
| `src/Player.*` | Spielfigur: Bewegung, Kollision, Enterhaken-Pendel |
| `src/Level.*` | Karte mit Gräben, Shop, Ampel und Autos, Ankern, Münzen, Ziel |
| `src/Background.*` | Parallax-Hintergrund |
| `src/Hud.*`, `src/Shop.*`, `src/Ui.*` | Menüleiste, Inventar, Shop-Menü, Knöpfe, Popups, Tooltips |
| `src/Items.*` | Gegenstände und Inventar |
| `src/Effects.*` | Wasserspritzer, Funkeln, schwebende Texte |
| `src/SaveGame.*` | Spielstand |

## Hinweise

- „Einkaufbau“ aus der Spezifikation wurde als **Einkaufsbeutel** umgesetzt (erhöht die maximale
  Ausdauer um 25). Name, Beschreibung und Wirkung lassen sich in `data/gegenstaende.txt` ändern.
- „Beenden“ in der Menüleiste speichert den Spielstand und kehrt ins Hauptmenü zurück.
  Dort beendet „Beenden“ das Programm.
- Die Ausdauer wird beim Springen und beim Enterhaken verbraucht und erholt sich am Boden.
  Die Dose Energy füllt sie sofort auf.
