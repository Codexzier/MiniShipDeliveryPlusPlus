# Mindestens benötigte CMake-Version (CLion bringt eine neuere Version mit)
cmake_minimum_required(VERSION 3.16)
# Name des Projekts, Versionsnummer und verwendete Programmiersprache
project(MiniShipDelivery VERSION 0.2.0 LANGUAGES CXX)

# Wir verwenden den Sprachstandard C++17
set(CMAKE_CXX_STANDARD 17)
# C++17 ist zwingend erforderlich
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Keine compilerspezifischen Spracherweiterungen benutzen
set(CMAKE_CXX_EXTENSIONS OFF)

# Wenn kein Build-Typ angegeben wurde ...
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    # ... wird optimiert gebaut, weil das Spiel komplett auf der CPU rendert
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build-Typ (Debug/Release)" FORCE)
# Ende der Bedingung
endif()

# SDL2: Fenster, Eingaben und Software-Renderer (Pflicht)
find_package(SDL2 REQUIRED)
# zlib: entpackt die komprimierten Daten in FBX-Dateien (Pflicht)
find_package(ZLIB REQUIRED)
# pkg-config hilft, SDL2_image und SDL2_mixer auch auf älteren Systemen zu finden
find_package(PkgConfig QUIET)

# SDL2_image lädt die PNG-Bilder des Interface Packs und die Figuren-Skins
find_package(SDL2_image CONFIG QUIET)
# Falls kein CMake-Paket existiert, über pkg-config suchen
if(NOT TARGET SDL2_image::SDL2_image AND PKG_CONFIG_FOUND)
    # SDL2_image per pkg-config als importiertes Ziel anlegen
    pkg_check_modules(SDL2IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
# Ende der Bedingung
endif()

# SDL2_mixer spielt Musik und Geräusche ab (optional: ohne bleibt das Spiel stumm)
find_package(SDL2_mixer CONFIG QUIET)
# Falls kein CMake-Paket existiert, über pkg-config suchen
if(NOT TARGET SDL2_mixer::SDL2_mixer AND PKG_CONFIG_FOUND)
    # SDL2_mixer per pkg-config suchen (nicht zwingend)
    pkg_check_modules(SDL2MIXER IMPORTED_TARGET SDL2_mixer)
# Ende der Bedingung
endif()

# Alle Quelldateien im Ordner src/ einsammeln (neue Dateien werden automatisch erkannt)
file(GLOB MSD_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
# Alle Header ebenfalls einsammeln (für die Anzeige in CLion)
file(GLOB MSD_HEADERS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.h)

# Ausführbare Datei aus allen Quelldateien erzeugen
add_executable(MiniShipDelivery ${MSD_SOURCES} ${MSD_HEADERS})

# Projektordner als Rückfallpfad für data/ und assets/ in das Programm einbauen
target_compile_definitions(MiniShipDelivery PRIVATE MSD_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# SDL2 verlinken (neue Pakete liefern ein Ziel, alte nur Variablen)
if(TARGET SDL2::SDL2)
    # Fertiges Ziel verwenden
    target_link_libraries(MiniShipDelivery PRIVATE SDL2::SDL2)
# Ältere SDL2-Pakete
else()
    # Include-Verzeichnisse von SDL2 bekannt machen
    target_include_directories(MiniShipDelivery PRIVATE ${SDL2_INCLUDE_DIRS})
    # SDL2-Bibliotheken verlinken
    target_link_libraries(MiniShipDelivery PRIVATE ${SDL2_LIBRARIES})
# Ende der Unterscheidung
endif()

# SDL2_image verlinken (CMake-Ziel oder pkg-config-Ziel)
if(TARGET SDL2_image::SDL2_image)
    # Ziel aus dem CMake-Paket
    target_link_libraries(MiniShipDelivery PRIVATE SDL2_image::SDL2_image)
# Sonst das pkg-config-Ziel
else()
    # Ziel aus pkg-config
    target_link_libraries(MiniShipDelivery PRIVATE PkgConfig::SDL2IMAGE)
# Ende der Unterscheidung
endif()

# SDL2_mixer verlinken, falls vorhanden, und Audio im Code einschalten
if(TARGET SDL2_mixer::SDL2_mixer)
    # Ziel aus dem CMake-Paket verlinken
    target_link_libraries(MiniShipDelivery PRIVATE SDL2_mixer::SDL2_mixer)
    # Audio-Code aktivieren
    target_compile_definitions(MiniShipDelivery PRIVATE MSD_AUDIO=1)
# Sonst über pkg-config
elseif(TARGET PkgConfig::SDL2MIXER)
    # Ziel aus pkg-config verlinken
    target_link_libraries(MiniShipDelivery PRIVATE PkgConfig::SDL2MIXER)
    # Audio-Code aktivieren
    target_compile_definitions(MiniShipDelivery PRIVATE MSD_AUDIO=1)
# Ohne SDL2_mixer
else()
    # Hinweis beim Konfigurieren ausgeben
    message(STATUS "SDL2_mixer nicht gefunden - das Spiel wird ohne Ton gebaut.")
# Ende der Unterscheidung
endif()

# zlib verlinken
target_link_libraries(MiniShipDelivery PRIVATE ZLIB::ZLIB)

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
