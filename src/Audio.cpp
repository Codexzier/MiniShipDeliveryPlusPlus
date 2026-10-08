// Audio.cpp - Abspielen von Musik und Geräuschen; erzeugt einfache Platzhalter-Klänge per Synthese
#include "Audio.h" // Eigene Deklarationen

#include <cmath>    // std::sin, std::exp
#include <iostream> // std::cout

#include "Common.h"       // SimpleRandom, PI
#include "ImageIO.h"      // Pfade und Dateiprüfung
#include "PropertyFile.h" // data/audio.txt

#include <SDL.h>          // SDL_InitSubSystem

#ifdef MSD_AUDIO          // Nur wenn SDL2_mixer vorhanden ist
#include <SDL_mixer.h>    // Mix_OpenAudio, Mix_PlayChannel ...
#endif                    // Ende der Bedingung

namespace {                                     // Interne Konstanten
constexpr int RATE = 44100;                     // Abtastrate in Hertz
constexpr int CHANNEL_MUSIC = 0;                // Kanal für Platzhalter-Musik
constexpr int CHANNEL_AMBIENCE = 1;             // Kanal für Hintergrundgeräusche
} // Ende des internen Namensraums

// Gibt beim Beenden alles frei
Audio::~Audio() { shutdown(); }                  // Aufräumen

// Öffnet das Tongerät und lädt alle Klänge
bool Audio::init(const std::string& configFile, const std::string& assetDir) { // Beginn von init
#ifdef MSD_AUDIO                                                              // Mit SDL2_mixer
    PropertyFile cfg;                                                         // Einstellungen
    cfg.load(configFile);                                                     // Laden (fehlt die Datei, gelten Standardwerte)
    m_musicVolume = clampValue(cfg.getInt("Einstellungen", "musik_lautstaerke", 60), 0, 100); // Musiklautstärke
    m_effectVolume = clampValue(cfg.getInt("Einstellungen", "effekt_lautstaerke", 80), 0, 100); // Effektlautstärke
    m_placeholders = cfg.getBool("Einstellungen", "platzhalter_klaenge", true); // Platzhalter erzeugen?
    if (!cfg.getBool("Einstellungen", "ton_an", true)) { m_status = "Ton in data/audio.txt ausgeschaltet"; return false; } // Abgeschaltet
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) { m_status = std::string("Kein Tongerät: ") + SDL_GetError(); return false; } // Audio starten
    if (Mix_OpenAudio(RATE, AUDIO_S16SYS, 2, 1024) != 0) { m_status = std::string("Mix_OpenAudio: ") + Mix_GetError(); return false; } // Gerät öffnen
    Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3);                                    // Decoder für OGG und MP3 vorbereiten
    Mix_AllocateChannels(16);                                                 // 16 gleichzeitige Kanäle
    Mix_ReserveChannels(2);                                                   // Kanäle 0 und 1 für Musik/Hintergrund reservieren
    m_enabled = true;                                                         // Ton ist aktiv
    int loaded = 0, synthetic = 0;                                            // Zähler
    const char* effects[] = {"klick", "kaufen", "verkaufen", "kanone", "treffer", "platsch", "anlegen", "glocke", "monster", "fund", "kentern", "fehler"}; // Bekannte Effekte
    for (const char* e : effects) {                                           // Alle Effekte
        std::string file = cfg.getString("Effekte", e, "");                   // Dateiname aus der Konfiguration
        void* chunk = nullptr;                                                // Klang
        if (!file.empty() && ImageIO::fileExists(ImageIO::joinPath(assetDir, file))) { chunk = Mix_LoadWAV(ImageIO::joinPath(assetDir, file).c_str()); if (chunk) ++loaded; } // Datei laden
        if (!chunk && m_placeholders) { chunk = synthesize(e); if (chunk) ++synthetic; } // Sonst Platzhalter erzeugen
        if (chunk) m_effects[e] = chunk;                                      // Speichern
    }                                                                         // Ende der Effektschleife
    const char* tracks[] = {"menue", "meer_tag", "meer_nacht", "insel"};      // Bekannte Musikstücke
    for (const char* t : tracks) {                                            // Alle Musikstücke
        std::string file = cfg.getString("Musik", t, "");                     // Dateiname
        if (!file.empty() && ImageIO::fileExists(ImageIO::joinPath(assetDir, file))) { // Datei vorhanden
            Mix_Music* mus = Mix_LoadMUS(ImageIO::joinPath(assetDir, file).c_str()); // Laden
            if (mus) { m_musicFiles[t] = mus; ++loaded; continue; }           // Speichern
        }                                                                     // Ende Datei
        if (m_placeholders) { void* c = synthesize(std::string("musik_") + t); if (c) { m_musicChunks[t] = c; ++synthetic; } } // Platzhalter-Melodie
    }                                                                         // Ende der Musikschleife
    const char* loops[] = {"wellen", "sturm", "regen"};                       // Bekannte Hintergrundgeräusche
    for (const char* l : loops) {                                             // Alle Hintergrundgeräusche
        std::string file = cfg.getString("Hintergrund", l, "");               // Dateiname
        void* chunk = nullptr;                                                // Klang
        if (!file.empty() && ImageIO::fileExists(ImageIO::joinPath(assetDir, file))) { chunk = Mix_LoadWAV(ImageIO::joinPath(assetDir, file).c_str()); if (chunk) ++loaded; } // Datei laden
        if (!chunk && m_placeholders) { chunk = synthesize(std::string("loop_") + l); if (chunk) ++synthetic; } // Platzhalter
        if (chunk) m_effects[std::string("loop_") + l] = chunk;               // Speichern
    }                                                                         // Ende der Schleife
    m_status = "Ton aktiv: " + std::to_string(loaded) + " Dateien, " + std::to_string(synthetic) + " Platzhalter"; // Statusmeldung
    return true;                                                              // Erfolgreich
#else                                                                         // Ohne SDL2_mixer
    (void)configFile;                                                         // Unbenutzt
    (void)assetDir;                                                           // Unbenutzt
    m_status = "Ohne SDL2_mixer gebaut - kein Ton";                           // Statusmeldung
    return false;                                                             // Kein Ton
#endif                                                                        // Ende der Bedingung
} // Ende von init

// Gibt alle Klänge frei und schließt das Gerät
void Audio::shutdown() {                                                      // Beginn von shutdown
#ifdef MSD_AUDIO                                                              // Mit SDL2_mixer
    if (!m_enabled) return;                                                   // Nichts geöffnet
    Mix_HaltChannel(-1);                                                      // Alle Kanäle stoppen
    Mix_HaltMusic();                                                          // Musik stoppen
    for (auto& e : m_effects) Mix_FreeChunk(static_cast<Mix_Chunk*>(e.second)); // Geräusche freigeben
    for (auto& e : m_musicChunks) Mix_FreeChunk(static_cast<Mix_Chunk*>(e.second)); // Platzhalter-Musik freigeben
    for (auto& e : m_musicFiles) Mix_FreeMusic(static_cast<Mix_Music*>(e.second)); // Musik freigeben
    m_effects.clear();                                                        // Listen leeren
    m_musicChunks.clear();                                                    // Listen leeren
    m_musicFiles.clear();                                                     // Listen leeren
    Mix_CloseAudio();                                                         // Gerät schließen
    Mix_Quit();                                                               // Decoder beenden
    m_enabled = false;                                                        // Ton aus
#endif                                                                        // Ende der Bedingung
} // Ende von shutdown

// Spielt ein Geräusch einmal ab
void Audio::play(const std::string& effect, int volumePercent) {              // Beginn von play
#ifdef MSD_AUDIO                                                              // Mit SDL2_mixer
    if (!m_enabled) return;                                                   // Ton aus
    auto it = m_effects.find(effect);                                         // Geräusch suchen
    if (it == m_effects.end()) return;                                        // Unbekannt
    int channel = Mix_PlayChannel(-1, static_cast<Mix_Chunk*>(it->second), 0); // Auf freiem Kanal abspielen
    if (channel >= 0) Mix_Volume(channel, MIX_MAX_VOLUME * m_effectVolume * clampValue(volumePercent, 0, 100) / 10000); // Lautstärke
#else                                                                         // Ohne SDL2_mixer
    (void)effect;                                                             // Unbenutzt
    (void)volumePercent;                                                      // Unbenutzt
#endif                                                                        // Ende der Bedingung
} // Ende von play

// Wechselt das Musikstück
void Audio::music(const std::string& track) {                                 // Beginn von music
#ifdef MSD_AUDIO                                                              // Mit SDL2_mixer
    if (!m_enabled || track == m_currentMusic) return;                        // Ton aus oder schon aktiv
    m_currentMusic = track;                                                   // Merken
    Mix_HaltMusic();                                                          // Laufende Musik stoppen
    Mix_HaltChannel(CHANNEL_MUSIC);                                           // Platzhalter-Musik stoppen
    auto f = m_musicFiles.find(track);                                        // Musikdatei?
    if (f != m_musicFiles.end()) {                                            // Ja
        Mix_VolumeMusic(MIX_MAX_VOLUME * m_musicVolume / 100);                // Lautstärke
        Mix_FadeInMusic(static_cast<Mix_Music*>(f->second), -1, 800);         // Endlos mit Einblenden abspielen
        return;                                                               // Fertig
    }                                                                         // Ende Datei
    auto c = m_musicChunks.find(track);                                       // Platzhalter-Melodie?
    if (c != m_musicChunks.end()) {                                           // Ja
        Mix_Volume(CHANNEL_MUSIC, MIX_MAX_VOLUME * m_musicVolume / 100 / 2);  // Leiser als Effekte
        Mix_FadeInChannel(CHANNEL_MUSIC, static_cast<Mix_Chunk*>(c->second), -1, 800); // Endlos abspielen
    }                                                                         // Ende Platzhalter
#else                                                                         // Ohne SDL2_mixer
    (void)track;                                                              // Unbenutzt
#endif                                                                        // Ende der Bedingung
} // Ende von music

// Wechselt das Hintergrundgeräusch
void Audio::ambience(const std::string& loop) {                               // Beginn von ambience
#ifdef MSD_AUDIO                                                              // Mit SDL2_mixer
    if (!m_enabled || loop == m_currentAmbience) return;                      // Ton aus oder unverändert
    m_currentAmbience = loop;                                                 // Merken
    Mix_FadeOutChannel(CHANNEL_AMBIENCE, 500);                                // Altes Geräusch ausblenden
    if (loop.empty()) return;                                                 // Nur ausschalten
    auto it = m_effects.find("loop_" + loop);                                 // Geräusch suchen
    if (it == m_effects.end()) return;                                        // Unbekannt
    Mix_Volume(CHANNEL_AMBIENCE, MIX_MAX_VOLUME * m_effectVolume / 100 / 3);  // Dezent leise
    Mix_FadeInChannel(CHANNEL_AMBIENCE, static_cast<Mix_Chunk*>(it->second), -1, 1000); // Endlos abspielen
#else                                                                         // Ohne SDL2_mixer
    (void)loop;                                                               // Unbenutzt
#endif                                                                        // Ende der Bedingung
} // Ende von ambience

// Erzeugt einen einfachen Klang per Rechnung (Platzhalter, bis echte Audiodateien vorliegen)
void* Audio::synthesize(const std::string& name) {                            // Beginn von synthesize
#ifdef MSD_AUDIO                                                              // Mit SDL2_mixer
    SimpleRandom rnd(static_cast<std::uint32_t>(name.size() * 977 + 13));     // Fester Zufall für Rauschen
    std::vector<float> mono;                                                  // Klang als Kommazahlen (-1..1)
    auto tone = [&](float freq, float seconds, float vol, float decay) {      // Hilfsfunktion: abklingender Sinuston anhängen
        int n = static_cast<int>(seconds * RATE);                             // Anzahl der Abtastwerte
        for (int i = 0; i < n; ++i) {                                         // Alle Werte
            float t = static_cast<float>(i) / RATE;                           // Zeit
            mono.push_back(vol * std::sin(2.0f * PI * freq * t) * std::exp(-decay * t)); // Sinus mit Ausklingen
        }                                                                     // Ende der Schleife
    };                                                                        // Ende der Hilfsfunktion
    auto noise = [&](float seconds, float vol, float decay, float smooth) {   // Hilfsfunktion: gefiltertes Rauschen anhängen
        int n = static_cast<int>(seconds * RATE);                             // Anzahl der Werte
        float last = 0.0f;                                                    // Tiefpassfilter-Zustand
        for (int i = 0; i < n; ++i) {                                         // Alle Werte
            float t = static_cast<float>(i) / RATE;                           // Zeit
            last += (rnd.range(-1.0f, 1.0f) - last) * smooth;                 // Tiefpass: je kleiner smooth, desto dumpfer
            mono.push_back(vol * last * std::exp(-decay * t));                // Mit Ausklingen speichern
        }                                                                     // Ende der Schleife
    };                                                                        // Ende der Hilfsfunktion
    if (name == "klick") tone(1800.0f, 0.04f, 0.5f, 60.0f);                   // Kurzer heller Klick
    else if (name == "kaufen") { tone(1320.0f, 0.08f, 0.4f, 20.0f); tone(1760.0f, 0.18f, 0.4f, 12.0f); } // Zwei helle Münztöne
    else if (name == "verkaufen") { tone(1760.0f, 0.08f, 0.4f, 20.0f); tone(1320.0f, 0.18f, 0.4f, 12.0f); } // Umgekehrte Münztöne
    else if (name == "kanone") noise(0.7f, 1.0f, 6.0f, 0.05f);                // Dumpfer Knall
    else if (name == "treffer") noise(0.3f, 0.9f, 12.0f, 0.4f);               // Krachen
    else if (name == "platsch") noise(0.5f, 0.7f, 7.0f, 0.15f);               // Wasserplatschen
    else if (name == "anlegen") { tone(140.0f, 0.15f, 0.8f, 25.0f); tone(110.0f, 0.25f, 0.8f, 18.0f); } // Hölzernes Pochen
    else if (name == "glocke") { for (int i = 0; i < static_cast<int>(1.4f * RATE); ++i) { float t = static_cast<float>(i) / RATE; mono.push_back(0.35f * (std::sin(2 * PI * 660 * t) + 0.5f * std::sin(2 * PI * 1650 * t)) * std::exp(-2.5f * t)); } } // Schiffsglocke
    else if (name == "monster") { for (int i = 0; i < static_cast<int>(1.6f * RATE); ++i) { float t = static_cast<float>(i) / RATE; float saw = std::fmod(55.0f * t, 1.0f) * 2.0f - 1.0f; mono.push_back(0.5f * saw * std::sin(PI * t / 1.6f) * (0.7f + 0.3f * std::sin(2 * PI * 7 * t))); } } // Tiefes Grollen
    else if (name == "fund") { tone(880.0f, 0.1f, 0.35f, 8.0f); tone(1108.0f, 0.1f, 0.35f, 8.0f); tone(1318.0f, 0.3f, 0.35f, 6.0f); } // Aufsteigende Fanfare
    else if (name == "kentern") { for (int i = 0; i < static_cast<int>(1.0f * RATE); ++i) { float t = static_cast<float>(i) / RATE; mono.push_back(0.5f * std::sin(2 * PI * (300.0f - 220.0f * t) * t) * std::exp(-1.5f * t)); } } // Absteigender Ton
    else if (name == "fehler") { tone(220.0f, 0.12f, 0.4f, 10.0f); tone(180.0f, 0.2f, 0.4f, 8.0f); } // Brummen
    else if (name == "loop_wellen" || name == "loop_regen" || name == "loop_sturm") { // Rauschende Schleifen
        float smooth = name == "loop_wellen" ? 0.02f : (name == "loop_regen" ? 0.5f : 0.08f); // Klangfarbe
        int n = 3 * RATE;                                                     // 3 Sekunden
        float last = 0.0f;                                                    // Filterzustand
        for (int i = 0; i < n; ++i) {                                         // Alle Werte
            float t = static_cast<float>(i) / RATE;                           // Zeit
            last += (rnd.range(-1.0f, 1.0f) - last) * smooth;                 // Tiefpass
            float swell = name == "loop_wellen" ? 0.6f + 0.4f * std::sin(2 * PI * t / 3.0f) : 1.0f; // Wellenbewegung
            mono.push_back((name == "loop_wellen" ? 4.0f : 0.8f) * last * swell); // Speichern
        }                                                                     // Ende der Schleife
    } else if (name.rfind("musik_", 0) == 0) {                                // Platzhalter-Melodien
        const float notes[] = {392, 440, 494, 523, 587, 523, 494, 440, 392, 330, 392, 440, 392, 0, 294, 392}; // Kleine Seemannsmelodie (Hz, 0 = Pause)
        float tempo = name == "musik_menue" ? 0.32f : (name == "musik_meer_nacht" ? 0.5f : 0.38f); // Dauer je Note
        float transpose = name == "musik_meer_nacht" ? 0.75f : (name == "musik_insel" ? 1.12f : 1.0f); // Tonhöhe je Stück
        for (int rep = 0; rep < 2; ++rep) {                                   // Melodie zweimal
            for (float f : notes) {                                           // Alle Noten
                int n = static_cast<int>(tempo * RATE);                       // Länge der Note
                for (int i = 0; i < n; ++i) {                                 // Alle Werte
                    float t = static_cast<float>(i) / RATE;                   // Zeit in der Note
                    float env = std::min(1.0f, t * 40.0f) * std::exp(-3.0f * t); // Hüllkurve (Anschlag, Ausklingen)
                    float v = f > 0 ? std::sin(2 * PI * f * transpose * t) + 0.3f * std::sin(4 * PI * f * transpose * t) : 0.0f; // Ton mit Oberton
                    float bass = 0.4f * std::sin(2 * PI * (rep == 0 ? 98.0f : 110.0f) * transpose * t); // Leiser Bass
                    mono.push_back(0.22f * (v * env + bass * 0.5f));          // Speichern
                }                                                             // Ende der Werte
            }                                                                 // Ende der Noten
        }                                                                     // Ende der Wiederholungen
    } else {                                                                  // Unbekannt
        return nullptr;                                                       // Kein Platzhalter
    }                                                                         // Ende der Unterscheidung
    std::vector<short> pcm(mono.size() * 2);                                  // Stereo 16 Bit
    for (std::size_t i = 0; i < mono.size(); ++i) {                           // Alle Werte
        short v = static_cast<short>(clampValue(mono[i], -1.0f, 1.0f) * 30000.0f); // In 16 Bit umrechnen
        pcm[i * 2] = v;                                                       // Linker Kanal
        pcm[i * 2 + 1] = v;                                                   // Rechter Kanal
    }                                                                         // Ende der Schleife
    m_buffers.push_back(std::move(pcm));                                      // Speicher behalten (SDL_mixer kopiert nicht)
    std::vector<short>& buf = m_buffers.back();                               // Referenz auf den Speicher
    return Mix_QuickLoad_RAW(reinterpret_cast<Uint8*>(buf.data()), static_cast<Uint32>(buf.size() * sizeof(short))); // Als Klang anmelden
#else                                                                         // Ohne SDL2_mixer
    (void)name;                                                               // Unbenutzt
    return nullptr;                                                           // Kein Klang
#endif                                                                        // Ende der Bedingung
} // Ende von synthesize
