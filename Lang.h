#pragma once
#include <juce_core/juce_core.h>
#include "Announcer.h"

// Texte dans la langue de l'utilisateur : français si l'application hôte tourne en français, anglais sinon.
// Les deux textes sont écrits en UTF-8.
inline juce::String jprText (const char* frenchUtf8, const char* englishUtf8)
{
    return juce::String::fromUTF8 (jprSystemIsFrench() ? frenchUtf8 : englishUtf8);
}
