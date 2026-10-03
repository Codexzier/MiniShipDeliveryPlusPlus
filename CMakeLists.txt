# Mindestens benötigte CMake-Version (CLion bringt eine neuere Version mit)
cmake_minimum_required(VERSION 3.16)
# Name des Projekts, Versionsnummer und verwendete Programmiersprache
project(MiniShipDelivery VERSION 0.1.0 LANGUAGES CXX)

# Wir verwenden den Sprachstandard C++17
set(CMAKE_CXX_STANDARD 17)
# C++17 ist zwingend erforderlich (kein stilles Zurückfallen auf ältere Standards)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Keine compilerspezifischen Spracherweiterungen (z.B. gnu++17) benutzen
set(CMAKE_CXX_EXTENSIONS OFF)

# Wenn kein Build-Typ angegeben wurde ...
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    # ... wird optimiert gebaut, weil das Spiel ohne Grafikkarte rendert
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build-Typ (Debug/Release)" FORCE)
# Ende der Bedingung für den Build-Typ
endif()

# SDL2 suchen: liefert Fenster, Tastatur/Maus und das reine Software-Rendering
find_package(SDL2 REQUIRED)

# Liste aller Quelldateien des Spiels
set(MSD_SOURCES
    # Einstiegspunkt des Programms
    src/main.cpp
    # Hauptklasse mit Spielschleife und Zustandsverwaltung
    src/Game.cpp
    # Lesen und Schreiben der Eigenschafts-Textdateien
    src/PropertyFile.cpp
    # Bildspeicher und Zeichenfunktionen (Software-Rendering)
    src/Graphics.cpp
    # Eingebaute Pixel-Schriftart
    src/Font.cpp
    # Laden und Speichern von BMP-Bildern
    src/ImageIO.cpp
    # Verwaltung aller Bilder (eigene Sprites oder Platzhalter)
    src/Assets.cpp
    # Erzeugung der Platzhalter-Sprites (Dummies)
    src/SpriteFactory.cpp
    # Animationsdefinitionen und Abspielen von Animationen
    src/Animation.cpp
    # Gegenstände und Inventar
    src/Items.cpp
    # Spielfigur mit Bewegung, Sprung und Enterhaken
    src/Player.cpp
    # Karte mit Gräben, Shop, Ampel, Ankerpunkten, Münzen und Ziel
    src/Level.cpp
    # Parallax-Hintergrund
    src/Background.cpp
    # Partikel und schwebende Texte
    src/Effects.cpp
    # Bedienelemente wie Knöpfe, Popups und Tooltips
    src/Ui.cpp
    # Menüleiste oben und Inventar unten links
    src/Hud.cpp
    # Shop-Menü mit 4x4 Feldern
    src/Shop.cpp
    # Speichern und Laden des Spielstands
    src/SaveGame.cpp
# Ende der Quelldateiliste
)

# Ausführbare Datei aus allen Quelldateien erzeugen
add_executable(MiniShipDelivery ${MSD_SOURCES})

# Projektordner als Rückfallpfad für data/ und assets/ in das Programm einbauen
target_compile_definitions(MiniShipDelivery PRIVATE MSD_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# Neuere SDL2-Pakete stellen ein fertiges Ziel SDL2::SDL2 bereit ...
if(TARGET SDL2::SDL2)
    # ... das wir direkt verlinken
    target_link_libraries(MiniShipDelivery PRIVATE SDL2::SDL2)
# Ältere SDL2-Pakete liefern nur Variablen
else()
    # Include-Verzeichnisse von SDL2 bekannt machen
    target_include_directories(MiniShipDelivery PRIVATE ${SDL2_INCLUDE_DIRS})
    # SDL2-Bibliotheken verlinken
    target_link_libraries(MiniShipDelivery PRIVATE ${SDL2_LIBRARIES})
# Ende der Unterscheidung
endif()

# Sehr alte GCC-Versionen (vor 9) brauchen eine Extra-Bibliothek für std::filesystem
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    # stdc++fs zusätzlich verlinken
    target_link_libraries(MiniShipDelivery PRIVATE stdc++fs)
# Ende der Bedingung
endif()

# Bei GCC und Clang zusätzliche Warnungen einschalten
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Viele hilfreiche Warnungen aktivieren
    target_compile_options(MiniShipDelivery PRIVATE -Wall -Wextra)
# Ende der Bedingung
endif()
