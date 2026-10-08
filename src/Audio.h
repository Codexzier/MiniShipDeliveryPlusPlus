// Audio.h - Musik und Geräusche über SDL2_mixer (Dateien aus data/audio.txt, sonst synthetische Platzhalter)
#pragma once // Header nur einmal einbinden

#include <map>    // std::map
#include <string> // std::string
#include <vector> // std::vector

class Audio {                                                                    // Beginn der Klasse
public:                                                                          // Öffentliche Schnittstelle
    ~Audio();                                                                    // Gibt alles frei
    bool init(const std::string& configFile, const std::string& assetDir);       // Gerät öffnen und Klänge laden
    void play(const std::string& effect, int volumePercent = 100);               // Geräusch einmal abspielen
    void music(const std::string& track);                                        // Musikstück (Endlosschleife) wechseln
    void ambience(const std::string& loop);                                      // Hintergrundgeräusch (z.B. Wellen) wechseln ("" = aus)
    bool enabled() const { return m_enabled; }                                   // Ist Ton verfügbar?
    std::string status() const { return m_status; }                              // Kurze Statusmeldung
private:                                                                         // Interne Daten
    void shutdown();                                                             // Alles freigeben
    void* synthesize(const std::string& name);                                   // Platzhalter-Klang erzeugen
    bool m_enabled = false;                                                      // Ton aktiv?
    bool m_placeholders = true;                                                  // Platzhalter erzeugen, wenn Dateien fehlen?
    int m_musicVolume = 70;                                                      // Musiklautstärke in Prozent
    int m_effectVolume = 80;                                                     // Effektlautstärke in Prozent
    std::map<std::string, void*> m_effects;                                      // Geladene Geräusche (Mix_Chunk*)
    std::map<std::string, void*> m_musicChunks;                                  // Musik als Klangschleife (Mix_Chunk*)
    std::map<std::string, void*> m_musicFiles;                                   // Musik aus Dateien (Mix_Music*)
    std::vector<std::vector<short>> m_buffers;                                   // Speicher der erzeugten Klänge
    std::string m_currentMusic;                                                  // Laufendes Musikstück
    std::string m_currentAmbience;                                               // Laufendes Hintergrundgeräusch
    std::string m_status = "Ton nicht initialisiert";                            // Statusmeldung
}; // Ende der Klasse Audio
