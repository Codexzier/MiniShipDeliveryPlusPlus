// SpriteFactory.h - Erzeugt Platzhalter-Grafiken (Dummies), solange keine eigenen Sprites vorliegen
#pragma once // Header nur einmal einbinden

#include <string> // std::string

#include "Animation.h" // AnimationSet (Aufbau des Sprite-Sheets der Spielfigur)
#include "Graphics.h"  // Image und Canvas

namespace SpriteFactory {                                                 // Alle Erzeuger liegen in diesem Namensraum
void drawPlayerSheet(Image& sheet, int tile, const AnimationSet& anims);  // Sprite-Sheet der Spielfigur mit allen Animationen
void drawCoinStrip(Image& image, int frames);                             // Sich drehende Münze (Bilder nebeneinander)
void drawCarFront(Image& image, Color body);                              // Auto von vorne (fährt auf den Betrachter zu)
void drawTrafficLight(Image& image, int tile, bool green);                // Fußgängerampel (rot oder grün)
void drawShop(Image& image, int tile, const std::string& sign);           // Ladengebäude mit Schild
void drawGoalFlag(Image& image, int tile, int frames);                    // Wehende Zielflagge (Bilder nebeneinander)
void drawTrash(Image& image, int type);                                   // Schwimmender Müll (verschiedene Arten)
void drawGroundTile(Image& image, int tile, bool street);                 // Bodenkachel (Gehweg oder Straße)
void drawAnchorRing(Image& image);                                        // Ankerring für den Enterhaken
void drawItemIcon(Image& image, const std::string& icon);                 // Symbol eines Gegenstands
void drawHudIcon(Image& image, const std::string& kind);                  // Kleine Symbole der Menüleiste
void drawCloudLayer(Image& image, int tile);                              // Hintergrundebene: Wolken
void drawMountainLayer(Image& image, int tile, int groundPx);             // Hintergrundebene: Berge und Hügel
void drawCityLayer(Image& image, int tile, int groundPx);                 // Hintergrundebene: ferne Stadt
void drawHouseLayer(Image& image, int tile, int groundPx);                // Hintergrundebene: nahe Häuser und Bäume
void addOutline(Image& image, Color outline, int thickness);              // Dunklen Umriss um sichtbare Pixel legen
} // Ende des Namensraums SpriteFactory
