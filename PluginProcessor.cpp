#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Announcer.h"
#include "Lang.h"
#include <cmath>

namespace
{
    const juce::StringArray spbNames  { "1", "2", "3", "4", "5", "6", "7", "8" };
    const int               spbValues[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    constexpr int           numSpb      = 8;

    constexpr long long aliveWindow   = 100;    // blocs globaux
    constexpr unsigned  doublePressMs = 1500;   // délai pour confirmer un effacement
    constexpr unsigned  holdEraseMs   = 500;    // maintien d'une touche pour effacer
    constexpr unsigned  newTakeMs     = 2000;   // silence qui sépare deux prises (annulation)
    constexpr unsigned  settleMs      = 500;    // une instance toute neuve attend ce délai que l'hôte lui rende son état

    enum RecMode { recNormal = 0, recToggle = 1, recHoldErase = 2 };

    // Touches blanches de l'octave "pistes" : Do Ré Mi Fa Sol La Si Do
    constexpr int whiteKeys[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };

    int spbIndexFor (int value)
    {
        for (int i = 0; i < numSpb; ++i) if (spbValues[i] == value) return i;
        return 3;
    }
}

// =====================================================================
// Paramètres
// =====================================================================
juce::AudioProcessorValueTreeState::ParameterLayout JPRLooperProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    StringArray trackNames;
    for (int i = 1; i <= SharedState::maxTracks; ++i) trackNames.add (String (i));

    const StringArray trigNames    { jprText ("Contrôleur MIDI", "MIDI controller"), jprText ("Note du clavier", "Keyboard note") };
    const StringArray recModeNames { jprText ("Normal : rejouer pour enlever, notes liées", "Normal: replay to remove, tied notes"),
                                     jprText ("Rejouer pour enlever, sans notes liées", "Replay to remove, no tied notes"),
                                     jprText ("Maintenir pour effacer", "Hold to erase") };
    const StringArray midiOutNames { jprText ("Désactivée", "Off"), jprText ("Boucles seulement", "Loops only"),
                                     jprText ("Boucles et clavier", "Loops and keyboard") };
    const StringArray inputNames   { jprText ("Piste active seulement", "Active track only"), jprText ("Toujours", "Always") };
    const StringArray drumNames    { jprText ("Normale", "Normal"), jprText ("Batterie", "Drums") };
    const StringArray hostNames    { "MainStage", "Logic Pro" };

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "track", 1 },   jprText ("Piste", "Track"), trackNames, 0));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "length", 1 },  jprText ("Longueur (pas)", "Length (steps)"), 1, SharedState::maxSteps, 16));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "spb", 1 },     jprText ("Pas par temps", "Steps per beat"), spbNames, 3));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "swing", 1 },   "Swing %", 0, 100, 0));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "gate", 1 },    "Gate %", 10, 100, 50));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { "running", 1 }, jprText ("Lecture", "Playback"), true));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { "mute", 1 },    jprText ("Muet", "Mute"), false));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { "solo", 1 },    "Solo", false));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { "armed", 1 },   jprText ("Armé", "Armed"), false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "recMode", 1 }, jprText ("Mode d'enregistrement", "Recording mode"), recModeNames, 0));
    // Les hôtes affichent « Off / On » pour un paramètre à deux choix dans leur vue Commandes :
    // le nom de ces paramètres est donc une affirmation à laquelle Off et On répondent sans ambiguïté.
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "logicMode", 1 }, jprText ("Mode Logic Pro", "Logic Pro mode"), hostNames, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "cmdTrigType", 1 },
                     jprText ("Mode commande déclenché par une note du clavier", "Command mode triggered by a keyboard note"), trigNames, 0));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "cmdTrigNumber", 1 }, jprText ("Numéro du déclencheur", "Trigger number"), 0, 127, 65));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "midiOut", 1 }, jprText ("Sortie MIDI virtuelle", "Virtual MIDI output"), midiOutNames, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "input", 1 },
                     jprText ("Clavier toujours transmis à l'instrument", "Keyboard always passed to the instrument"), inputNames, 0));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "cmdBase", 1 }, jprText ("Note de base des commandes", "Command base note"), 12, 96, 48));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "grooveAmount", 1 }, jprText ("Intensité du groove %", "Groove amount %"), 0, 200, 100));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "grooveLength", 1 }, jprText ("Longueur du groove", "Groove length"), 1, SharedState::grooveSteps, 16));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "grooveStep", 1 },   jprText ("Pas de groove", "Groove step"), 1, SharedState::grooveSteps, 1));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "grooveValue", 1 },  jprText ("Décalage du pas %", "Step offset %"), -50, 50, 0));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { "inputMute", 1 },    jprText ("Clavier muet", "Keyboard muted"), false));

    // Transposition en temps réel. « Piste de batterie » est propre à chaque instance ; le reste est global.
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "drum", 1 },
                     jprText ("Piste de batterie, non transposée", "Drum track, not transposed"), drumNames, 0));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "transpose", 1 }, jprText ("Transposition (demi-tons)", "Transpose (semitones)"), -24, 24, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "transTrigType", 1 },
                     jprText ("Mode transposition déclenché par une note du clavier", "Transpose mode triggered by a keyboard note"), trigNames, 0));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "transTrigNumber", 1 },
                     jprText ("Numéro du déclencheur de transposition", "Transpose trigger number"), 0, 127, 66));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "transBase", 1 }, jprText ("Note de base de la transposition", "Transpose base note"), 0, 127, 60));

    return { p.begin(), p.end() };
}

JPRLooperProcessor::JPRLooperProcessor()
    : AudioProcessor (BusesProperties()),
      apvts (*this, nullptr, "STATE", createLayout())
{
    trackParam        = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("track"));
    lengthParam       = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("length"));
    spbParam          = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("spb"));
    swingParam        = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("swing"));
    gateParam         = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("gate"));
    runningParam      = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter ("running"));
    muteParam         = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter ("mute"));
    soloParam         = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter ("solo"));
    armedParam        = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter ("armed"));
    recModeParam      = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("recMode"));
    logicModeParam    = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("logicMode"));
    cmdTrigTypeParam  = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("cmdTrigType"));
    cmdTrigNumberParam = dynamic_cast<juce::AudioParameterInt*>   (apvts.getParameter ("cmdTrigNumber"));
    midiOutParam      = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("midiOut"));
    inputParam        = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("input"));
    cmdBaseParam      = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("cmdBase"));
    grooveAmountParam = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("grooveAmount"));
    grooveLengthParam = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("grooveLength"));
    grooveStepParam   = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("grooveStep"));
    grooveValueParam  = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("grooveValue"));
    inputMuteParam    = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter ("inputMute"));
    drumParam         = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("drum"));
    transposeParam    = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("transpose"));
    transTrigTypeParam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("transTrigType"));
    transTrigNumberParam = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter ("transTrigNumber"));
    transBaseParam    = dynamic_cast<juce::AudioParameterInt*>    (apvts.getParameter ("transBase"));

    for (auto& k : heldStartStep) k = -1;
    for (auto& n : heldStoredNote) n = -1;
    createdMs = juce::Time::getMillisecondCounter();
    startTimerHz (20);
}

JPRLooperProcessor::~JPRLooperProcessor()
{
    stopTimer();
    midiOutPtr.store (nullptr);
    midiOutDevice.reset();
    midiOutToRelease.reset();
}

void JPRLooperProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    lastStepIndex     = -1;
    numPending        = 0;
    outBuffer.ensureSize (4096);
    tapBuffer.ensureSize (4096);
    for (auto& h : heldSinceMs) h = 0;
    for (auto& k : heldStartStep) k = -1;
    for (auto& e : heldErased) e = false;
    for (auto& n : heldStoredNote) n = -1;
}

// =====================================================================
// Annonces VoiceOver et synchronisation interface (fil message)
// =====================================================================
void JPRLooperProcessor::announceNow (int code, int arg)
{
    // « Piste 3 muette » / "Track 3 muted"
    auto track = [arg] (const char* frenchSuffix, const char* englishSuffix)
    {
        return jprText ("Piste ", "Track ") + juce::String (arg) + jprText (frenchSuffix, englishSuffix);
    };

    juce::String text;
    switch (code)
    {
        case annActiveTrack:   text = track (" active", " active"); break;
        case annMuteOn:        text = track (" muette", " muted"); break;
        case annMuteOff:       text = track (" non muette", " unmuted"); break;
        case annSoloOn:        text = track (" solo", " solo"); break;
        case annSoloOff:       text = track (" solo désactivé", " solo off"); break;
        case annArmOn:         text = track (" armée", " armed"); break;
        case annArmOff:        text = track (" désarmée", " disarmed"); break;
        case annTrackCleared:  text = track (" effacée", " cleared"); break;
        case annAllCleared:    text = jprText ("Toutes les pistes effacées", "All tracks cleared"); break;
        case annConfirmClear:  text = (arg == 2) ? jprText ("Appuyez encore pour tout effacer", "Press again to clear everything")
                                                 : jprText ("Appuyez encore pour effacer la piste", "Press again to clear the track"); break;
        case annPlay:          text = jprText ("Lecture", "Play"); break;
        case annStop:          text = jprText ("Arrêt", "Stop"); break;
        case annAllMuteOn:     text = jprText ("Toutes les pistes muettes", "All tracks muted"); break;
        case annAllMuteOff:    text = jprText ("Toutes les pistes actives", "All tracks unmuted"); break;
        case annCommandOn:     text = jprText ("Mode commande", "Command mode"); break;
        case annCommandOff:    text = jprText ("Clavier normal", "Normal keyboard"); break;
        case annUndone:        text = jprText ("Annulé", "Undone"); break;
        case annNothingToUndo: text = jprText ("Rien à annuler", "Nothing to undo"); break;
        case annRecMode:       text = jprText ("Enregistrement : ", "Recording: ");
                               text << (arg == 1 ? jprText ("rejouer pour enlever, sans notes liées", "replay to remove, no tied notes")
                                     : arg == 2 ? jprText ("maintenir pour effacer", "hold to erase")
                                                : jprText ("normal, rejouer pour enlever, notes liées", "normal, replay to remove, tied notes")); break;
        case annUnassigned:    text = jprText ("Touche non assignée", "Unassigned key"); break;
        case annAutoDisarm:    text = jprText ("Looper arrêté, pistes désarmées", "Looper stopped, tracks disarmed"); break;
        case annInputMuteOn:   text = jprText ("Clavier muet", "Keyboard muted"); break;
        case annInputMuteOff:  text = jprText ("Clavier actif", "Keyboard active"); break;
        case annTransposeModeOn:  text = jprText ("Mode transposition", "Transpose mode"); break;
        case annTransposeModeOff: text = jprText ("Clavier normal", "Normal keyboard"); break;
        case annTranspose:     if (arg == 0)     text = jprText ("Transposition zéro", "Transpose zero");
                               else if (arg > 0) text = jprText ("Transposition plus ", "Transpose plus ") + juce::String (arg);
                               else              text = jprText ("Transposition moins ", "Transpose minus ") + juce::String (-arg);
                               break;
        default: return;
    }
    jprAnnounce (text.toRawUTF8());
}

void JPRLooperProcessor::timerCallback()
{
    auto& s = SharedState::get();
    updateMidiOutDevice();

    // ---- Annonces en attente ----
    for (int i = 0; i < 8; ++i)
    {
        int r = s.announceRead.load();
        if (r >= s.announceWrite.load()) break;
        if (! s.announceRead.compare_exchange_strong (r, r + 1)) continue;
        auto& a = s.announces[r % SharedState::announceSize];
        announceNow (a.code.load(), a.arg.load());
    }

    // ---- L'interface reflète la mémoire partagée ----
    // Seulement quand le fil audio a publié les réglages de cette instance (au moins un bloc traité
    // depuis la création ou le chargement). Avant cela, les valeurs par défaut de la mémoire partagée
    // écraseraient les réglages que l'hôte vient de restaurer, selon l'ordre d'arrivée des fils.
    if (publishedGeneration.load() != stateGeneration.load()) return;

    const int t = trackNumber.load() - 1;
    auto syncBool = [] (juce::AudioParameterBool* param, bool value)
    {
        if (param->get() != value) param->setValueNotifyingHost (value ? 1.0f : 0.0f);
    };
    auto syncInt = [] (juce::AudioParameterInt* param, int value)
    {
        if (param->get() != value) param->setValueNotifyingHost (param->convertTo0to1 ((float) value));
    };
    auto syncChoice = [] (juce::AudioParameterChoice* param, int index)
    {
        if (param->getIndex() != index) param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
    };

    syncBool (runningParam, s.running.load() != 0);
    syncBool (muteParam,    s.mute[t].load() != 0);
    syncBool (soloParam,    s.solo[t].load() != 0);
    syncBool (armedParam,   s.armed[t].load() != 0);
    syncBool (inputMuteParam, s.inputMute.load() != 0);
    syncChoice (recModeParam, juce::jlimit (0, 2, s.recMode.load()));

    // Transposition : le paramètre suit la mémoire partagée (touches du mode transposition). La valeur
    // recopiée est notée pour que le fil audio ne la republie pas par-dessus un changement plus récent.
    {
        const int tr = juce::jlimit (-24, 24, s.transpose.load());
        if (transposeParam->get() != tr)
        {
            transposeEcho.store (tr);
            transposeParam->setValueNotifyingHost (transposeParam->convertTo0to1 ((float) tr));
        }
    }

    // Quand on change de pas de groove, le curseur de décalage affiche la valeur de ce pas.
    const int gs = juce::jlimit (1, SharedState::grooveSteps, grooveStepParam->get());
    if (gs != uiLastGrooveStep)
    {
        uiLastGrooveStep = gs;
        syncInt (grooveValueParam, s.grooveTable[gs - 1].load());
    }

    if (! isLeader())
    {
        syncInt    (lengthParam,       s.patternLength.load());
        syncChoice (spbParam,          spbIndexFor (s.stepsPerBeat.load()));
        syncInt    (swingParam,        s.swing.load());
        syncInt    (gateParam,         s.gate.load());
        syncChoice (logicModeParam,    juce::jlimit (0, 1, s.logicMode.load()));
        syncChoice (cmdTrigTypeParam,  juce::jlimit (0, 1, s.cmdTrigType.load()));
        syncInt    (cmdTrigNumberParam, juce::jlimit (0, 127, s.cmdTrigNumber.load()));
        syncChoice (midiOutParam,      juce::jlimit (0, 2, s.midiOut.load()));
        syncInt    (grooveAmountParam, s.grooveAmount.load());
        syncInt    (grooveLengthParam, s.grooveLength.load());
        syncInt    (grooveValueParam,  s.grooveTable[gs - 1].load());
        syncChoice (transTrigTypeParam,   juce::jlimit (0, 1, s.transTrigType.load()));
        syncInt    (transTrigNumberParam, juce::jlimit (0, 127, s.transTrigNumber.load()));
        syncInt    (transBaseParam,       juce::jlimit (0, 127, s.transBase.load()));
    }

    // ---- Diagnostic : pas utilisés sur cette piste ----
    int used = 0;
    const int len = s.patternLength.load();
    for (int k = 0; k < len; ++k)
        for (int v = 0; v < SharedState::maxPoly; ++v)
            if (s.tracks[t].notes[k][v].note.load() >= 0) { ++used; break; }
    diag.stepsUsed.store (used);
}

void JPRLooperProcessor::clearThisTrack()
{
    auto& s = SharedState::get();
    s.takeSnapshot();
    s.clearTrack (trackNumber.load() - 1);
    announceNow (annTrackCleared, trackNumber.load());
}

void JPRLooperProcessor::clearAllTracks()
{
    auto& s = SharedState::get();
    s.takeSnapshot();
    for (int t = 0; t < SharedState::maxTracks; ++t) s.clearTrack (t);
    announceNow (annAllCleared, 0);
}

void JPRLooperProcessor::undo()
{
    auto& s = SharedState::get();
    announceNow (s.restoreSnapshot() ? annUndone : annNothingToUndo, 0);
}

// =====================================================================
// Paramètres -> mémoire partagée (détection de changement)
// =====================================================================
void JPRLooperProcessor::pushParamsToShared (bool leader, int track)
{
    auto& s = SharedState::get();
    const int t = track - 1;

    auto changed = [] (int& last, int now) { const bool c = (last != now); last = now; return c; };

    // Après la création ou un chargement d'état, tout est republié (décidé ici, sur le fil audio).
    const int  generation = stateGeneration.load();
    const bool republish  = (generation != appliedGeneration);
    if (republish)
    {
        lastLength = lastSpb = lastSwing = lastGate = lastLogicMode = lastMidiOut = lastRecMode = -1;
        lastTrigType = lastTrigNumber = -1;
        lastRunning = lastMute = lastSolo = lastArmed = -1;
        lastGrooveAmount = lastGrooveLength = -1;
        lastGrooveStep = -1;
        lastTransTrigType = lastTransTrigNumber = lastTransBase = -1;
        lastTranspose = noTranspose;
    }

    const int transposeNow  = transposeParam->get();
    const int transposeSeen = transposeEcho.exchange (noTranspose);

    const int gs = juce::jlimit (1, SharedState::grooveSteps, grooveStepParam->get());
    const bool grooveStepChanged = changed (lastGrooveStep, gs);

    if (leader)
    {
        // Les réglages que seul le chef décide sont republiés à chaque bloc : ses paramètres font foi.
        // Si une autre instance les a écrasés (par exemple en démarrant avec ses valeurs par défaut,
        // avant que l'hôte lui rende son état), la mémoire partagée se corrige d'elle-même.
        lastLength = lengthParam->get();            s.patternLength.store (lastLength);
        lastSpb = juce::jlimit (0, numSpb - 1, spbParam->getIndex()); s.stepsPerBeat.store (spbValues[lastSpb]);
        lastSwing = swingParam->get();              s.swing.store (lastSwing);
        lastGate = gateParam->get();                s.gate.store (lastGate);
        lastLogicMode = logicModeParam->getIndex(); s.logicMode.store (lastLogicMode);

        // Changer de déclencheur fait sortir du mode commande (l'ancien déclencheur ne pourrait plus le relâcher).
        const bool trigTypeChanged   = changed (lastTrigType,   cmdTrigTypeParam->getIndex());
        const bool trigNumberChanged = changed (lastTrigNumber, cmdTrigNumberParam->get());
        if (trigTypeChanged || trigNumberChanged)
        {
            s.commandMode.store (0);
        }
        s.cmdTrigType.store (cmdTrigTypeParam->getIndex());
        s.cmdTrigNumber.store (cmdTrigNumberParam->get());
        lastMidiOut = midiOutParam->getIndex();     s.midiOut.store (lastMidiOut);
        if (changed (lastRecMode,      recModeParam->getIndex()))  s.recMode.store (recModeParam->getIndex());

        // Transposition : publiée quand l'utilisateur bouge le paramètre, pas quand le timer vient d'y recopier la mémoire partagée.
        if (changed (lastTranspose, transposeNow) && transposeSeen != transposeNow)
            s.transpose.store (transposeNow);

        // Changer de déclencheur fait sortir du mode transposition, comme pour le mode commande.
        const bool transTypeChanged   = changed (lastTransTrigType,   transTrigTypeParam->getIndex());
        const bool transNumberChanged = changed (lastTransTrigNumber, transTrigNumberParam->get());
        if (transTypeChanged || transNumberChanged)
        {
            s.transposeMode.store (0);
        }
        s.transTrigType.store (transTrigTypeParam->getIndex());
        s.transTrigNumber.store (transTrigNumberParam->get());
        lastTransBase = transBaseParam->get();        s.transBase.store (lastTransBase);

        lastGrooveAmount = grooveAmountParam->get();  s.grooveAmount.store (lastGrooveAmount);
        lastGrooveLength = grooveLengthParam->get();  s.grooveLength.store (lastGrooveLength);

        // Le décalage n'est publié que si l'utilisateur l'a bougé, pas quand on change de pas.
        if (grooveStepChanged)
            lastGrooveValue = grooveValueParam->get();
        else if (changed (lastGrooveValue, grooveValueParam->get()))
            s.grooveTable[gs - 1].store (grooveValueParam->get());
    }
    else
    {
        lastLength = lengthParam->get(); lastSpb = spbParam->getIndex(); lastSwing = swingParam->get();
        lastGate = gateParam->get(); lastLogicMode = logicModeParam->getIndex();
        lastTrigType = cmdTrigTypeParam->getIndex(); lastTrigNumber = cmdTrigNumberParam->get();
        lastMidiOut = midiOutParam->getIndex(); lastRecMode = recModeParam->getIndex();
        lastGrooveAmount = grooveAmountParam->get(); lastGrooveLength = grooveLengthParam->get();
        lastGrooveValue = grooveValueParam->get();
        lastTranspose = transposeNow;
        lastTransTrigType = transTrigTypeParam->getIndex(); lastTransTrigNumber = transTrigNumberParam->get();
        lastTransBase = transBaseParam->get();
    }

    if (changed (lastRunning, runningParam->get() ? 1 : 0))
    {
        if (runningParam->get()) s.runningAtMs.store (juce::Time::getMillisecondCounter());
        s.running.store (runningParam->get() ? 1 : 0);
    }
    if (changed (lastMute,    muteParam->get() ? 1 : 0))    s.mute[t].store (muteParam->get() ? 1 : 0);
    if (changed (lastSolo,    soloParam->get() ? 1 : 0))    s.solo[t].store (soloParam->get() ? 1 : 0);
    if (changed (lastArmed,   armedParam->get() ? 1 : 0))
    {
        if (armedParam->get()) s.armedAtMs[t].store (juce::Time::getMillisecondCounter());
        s.armed[t].store (armedParam->get() ? 1 : 0);
    }

    // Clavier muet : réglage de session, commun à toutes les instances. Une instance qui arrive
    // (création ou chargement) adopte l'état courant au lieu d'imposer le sien.
    const int inputMuteNow = inputMuteParam->get() ? 1 : 0;
    if (republish || lastInputMute < 0)           lastInputMute = inputMuteNow;
    else if (changed (lastInputMute, inputMuteNow)) s.inputMute.store (inputMuteNow);

    if (republish)
    {
        appliedGeneration = generation;
        publishedGeneration.store (generation);
    }
}

// =====================================================================
// Commandes clavier (mode commande)
// =====================================================================
void JPRLooperProcessor::handleCommandSwitch (bool pressed, long long now, long long window)
{
    auto& s = SharedState::get();

    // Dédoublonnage : le même déclencheur (pédale ou touche) arrive dans toutes les instances du patch.
    {
        long long last = s.lastCcStamp.load();
        if (last >= 0 && now - last <= window) return;
        if (! s.lastCcStamp.compare_exchange_strong (last, now)) return;
    }

    // Mode maintenu : déclencheur enfoncé = mode commande, relâché = clavier normal.
    const int wanted = pressed ? 1 : 0;
    if (s.commandMode.exchange (wanted) != wanted)
        s.pushAnnounce (wanted ? annCommandOn : annCommandOff, 0);
}

// =====================================================================
// Mode transposition : déclencheur maintenu, une touche du clavier fixe la transposition
// =====================================================================
void JPRLooperProcessor::handleTransposeSwitch (bool pressed, long long now, long long window)
{
    auto& s = SharedState::get();

    // Dédoublonnage : le même déclencheur arrive dans toutes les instances du patch.
    {
        long long last = s.lastTransStamp.load();
        if (last >= 0 && now - last <= window) return;
        if (! s.lastTransStamp.compare_exchange_strong (last, now)) return;
    }

    const int wanted = pressed ? 1 : 0;
    if (s.transposeMode.exchange (wanted) != wanted)
        s.pushAnnounce (wanted ? annTransposeModeOn : annTransposeModeOff, 0);
}

void JPRLooperProcessor::handleTransposeNote (int note, int velocity, long long now, long long window)
{
    if (velocity == 0) return;
    auto& s = SharedState::get();

    {
        long long last = s.lastNoteStamp[note].load();
        if (last > 0 && now - last <= window) return;
        if (! s.lastNoteStamp[note].compare_exchange_strong (last, now)) return;
    }

    // La note de base donne zéro ; chaque demi-ton au-dessus ou en dessous transpose d'autant.
    const int value = juce::jlimit (-24, 24, note - s.transBase.load());
    s.transpose.store (value);
    s.pushAnnounce (annTranspose, value);
}

void JPRLooperProcessor::handleCommandNote (int note, int velocity, long long now, long long window, unsigned nowMs)
{
    if (velocity == 0) return;
    auto& s = SharedState::get();

    {
        long long last = s.lastNoteStamp[note].load();
        if (last > 0 && now - last <= window) return;
        if (! s.lastNoteStamp[note].compare_exchange_strong (last, now)) return;
    }

    // La note de base est la première commande : le Do de lecture / arrêt (octave des actions).
    // L'octave des pistes commence douze demi-tons plus haut.
    const int base = cmdBaseParam->get();
    const int rel  = note - base - 12;
    const int act  = s.activeTrack.load();
    const int a    = act - 1;

    auto confirmClear = [&] (int what) -> bool
    {
        if (s.clearPressedWhat.load() == what && nowMs - s.clearPressedAt.load() < doublePressMs)
        {
            s.clearPressedWhat.store (0);
            return true;
        }
        s.clearPressedWhat.store (what);
        s.clearPressedAt.store (nowMs);
        s.pushAnnounce (annConfirmClear, what);
        return false;
    };

    // ---- Octave "pistes" (une octave au-dessus de la note de base) ----
    if (rel >= 0 && rel <= 12)
    {
        for (int i = 0; i < 8; ++i)
            if (whiteKeys[i] == rel)
            {
                s.activeTrack.store (i + 1);
                s.pushAnnounce (annActiveTrack, i + 1);
                return;
            }
        switch (rel)
        {
            case 1:  { const int v = s.mute[a].load() ? 0 : 1;  s.mute[a].store (v);  s.pushAnnounce (v ? annMuteOn : annMuteOff, act); return; }
            case 3:  { const int v = s.solo[a].load() ? 0 : 1;  s.solo[a].store (v);  s.pushAnnounce (v ? annSoloOn : annSoloOff, act); return; }
            case 6:  { const int v = s.armed[a].load() ? 0 : 1; if (v) s.armedAtMs[a].store (nowMs); s.armed[a].store (v); s.pushAnnounce (v ? annArmOn : annArmOff, act); return; }
            case 8:  if (confirmClear (1)) { s.takeSnapshot(); s.clearTrack (a); s.pushAnnounce (annTrackCleared, act); } return;
            case 10: s.pushAnnounce (s.restoreSnapshot() ? annUndone : annNothingToUndo, 0); return;
            default: s.pushAnnounce (annUnassigned, 0); return;
        }
    }

    // ---- Octave "actions" (à partir de la note de base) ----
    if (rel >= -12 && rel < 0)
    {
        switch (rel + 12)
        {
            case 0:  { const int v = s.running.load() ? 0 : 1; if (v) s.runningAtMs.store (nowMs); s.running.store (v); s.pushAnnounce (v ? annPlay : annStop, 0); return; }
            case 2:
            {
                bool anyUnmuted = false;
                for (int t = 0; t < SharedState::maxTracks; ++t) if (! s.mute[t].load()) anyUnmuted = true;
                for (int t = 0; t < SharedState::maxTracks; ++t) s.mute[t].store (anyUnmuted ? 1 : 0);
                s.pushAnnounce (anyUnmuted ? annAllMuteOn : annAllMuteOff, 0);
                return;
            }
            case 4:  if (confirmClear (2)) { s.takeSnapshot(); for (int t = 0; t < SharedState::maxTracks; ++t) s.clearTrack (t); s.pushAnnounce (annAllCleared, 0); } return;
            case 5:  { const int m = (s.recMode.load() + 1) % 3; s.recMode.store (m); s.pushAnnounce (annRecMode, m); return; }
            case 7:  { const int v = s.inputMute.load() ? 0 : 1; s.inputMute.store (v); s.pushAnnounce (v ? annInputMuteOn : annInputMuteOff, 0); return; }
            default: s.pushAnnounce (annUnassigned, 0); return;
        }
    }
}

// =====================================================================
// Enregistrement
// =====================================================================
void JPRLooperProcessor::beginTakeIfNeeded (unsigned nowMs)
{
    // Une nouvelle prise commence après un silence : on garde un instantané pour l'annulation.
    if (lastRecordedMs == 0 || nowMs - lastRecordedMs > newTakeMs)
        SharedState::get().takeSnapshot();
    lastRecordedMs = nowMs;
}

void JPRLooperProcessor::recordNoteOn (int track, int note, int velocity, double stepPosition, unsigned nowMs)
{
    auto& s = SharedState::get();
    const int len = juce::jmax (1, s.patternLength.load());
    const long long nearest = (long long) std::llround (stepPosition);
    const int step = (int) (((nearest % len) + len) % len);
    const int mode = s.recMode.load();

    // « note » est la touche jouée. La boucle reçoit la note sans la transposition en cours :
    // rejouée avec la transposition, elle sonne donc comme elle a été jouée.
    const int stored = heldStoredNote[note];
    if (stored < 0) return;                                      // hors du clavier une fois la transposition retirée

    heldStartStep[note] = nearest;

    // Rejouer une note sur un pas où elle existe déjà la supprime (modes normal et rejouer).
    // La touche encore tenue ne doit alors pas redéposer de notes liées : on oublie son pas de départ.
    if ((mode == recNormal || mode == recToggle) && s.hasNote (track - 1, step, stored))
    {
        beginTakeIfNeeded (nowMs);
        s.removeNote (track - 1, step, stored);
        heldStartStep[note] = -1;
        return;
    }

    auto& slots = s.tracks[track - 1].notes[step];
    for (auto& n : slots)
        if (n.note.load() == stored) return;                     // déjà présente

    for (auto& n : slots)
        if (n.note.load() < 0)
        {
            beginTakeIfNeeded (nowMs);
            n.velocity.store (velocity);
            n.tie.store (0);
            // Le pas visé est encore à venir : on ne le rejoue pas la première fois,
            // la note est déjà entendue en direct (anti double déclenchement).
            n.skipOnce.store ((double) nearest > stepPosition ? 1 : 0);
            n.note.store (stored);
            return;
        }
}

void JPRLooperProcessor::recordTiesAtStep (int track, long long k)
{
    // Les touches encore tenues au passage du pas k y laissent une note liée.
    auto& s = SharedState::get();
    const int len  = juce::jmax (1, s.patternLength.load());
    const int step = (int) (((k % len) + len) % len);

    for (int note = 0; note < 128; ++note)
    {
        if (heldSinceMs[note] == 0 || heldStartStep[note] < 0 || heldStartStep[note] >= k) continue;
        const int stored = heldStoredNote[note];
        if (stored < 0 || s.hasNote (track - 1, step, stored)) continue;

        for (auto& n : s.tracks[track - 1].notes[step])
            if (n.note.load() < 0)
            {
                n.velocity.store (heldVelocity[note]);
                n.tie.store (1);
                n.skipOnce.store (1);                           // la note sonne déjà en direct
                n.note.store (stored);
                break;
            }
    }
}

void JPRLooperProcessor::holdToEraseCheck (int track, unsigned nowMs)
{
    auto& s = SharedState::get();
    const int len = juce::jmax (1, s.patternLength.load());
    bool snapshotTaken = false;

    for (int note = 0; note < 128; ++note)
    {
        if (heldSinceMs[note] == 0 || heldErased[note]) continue;
        if (nowMs - heldSinceMs[note] < holdEraseMs) continue;

        heldErased[note] = true;
        const int stored = heldStoredNote[note];
        if (stored < 0) continue;
        if (! snapshotTaken) { s.takeSnapshot(); snapshotTaken = true; }
        for (int k = 0; k < len; ++k)
            if (s.hasNote (track - 1, k, stored))
                s.removeNote (track - 1, k, stored);
    }
}

// =====================================================================
// Lecture
// =====================================================================
double JPRLooperProcessor::stepTime (long long k, double stepLen) const
{
    auto& s = SharedState::get();
    const int len = juce::jmax (1, s.patternLength.load());
    const int g   = (int) ((((k % len) + len) % len) % juce::jmax (1, curGrooveLength));

    double extra = ((k & 1) ? curSwingDelay : 0.0)
                 + (double) s.grooveTable[g].load() * curGrooveScale;

    // Les décalages restent inférieurs à un demi-pas : l'ordre des pas est préservé.
    extra = juce::jlimit (-0.45 * stepLen, 0.45 * stepLen, extra);
    return (double) k * stepLen + extra;
}

void JPRLooperProcessor::emitLoop (juce::MidiBuffer& midi, const juce::MidiMessage& m, int pos)
{
    midi.addEvent (m, pos);
    if (tap != nullptr) tap->addEvent (m, pos);
}

void JPRLooperProcessor::addNoteOff (int note, int samplesFromNow)
{
    if (numPending < maxPending)
        pending[numPending++] = { note, samplesFromNow };
}

bool JPRLooperProcessor::extendNoteOff (int note, int samplesFromNow)
{
    for (int i = 0; i < numPending; ++i)
        if (pending[i].note == note) { pending[i].samples = samplesFromNow; return true; }
    return false;
}

void JPRLooperProcessor::flushNoteOffs (juce::MidiBuffer& midi, int numSamples)
{
    int kept = 0;
    for (int i = 0; i < numPending; ++i)
    {
        if (pending[i].samples < numSamples)
            emitLoop (midi, juce::MidiMessage::noteOff (1, pending[i].note), juce::jmax (0, pending[i].samples));
        else
        {
            pending[kept] = pending[i];
            pending[kept].samples -= numSamples;
            ++kept;
        }
    }
    numPending = kept;
}

void JPRLooperProcessor::allNotesOff (juce::MidiBuffer& midi)
{
    for (int i = 0; i < numPending; ++i)
        emitLoop (midi, juce::MidiMessage::noteOff (1, pending[i].note), 0);
    numPending = 0;
}

void JPRLooperProcessor::playTrack (juce::MidiBuffer& midi, int track, double position, double blockLenBeats,
                                    double samplesPerBeat, int numSamples, bool recordingTies)
{
    auto& s = SharedState::get();
    const int    len         = juce::jmax (1, s.patternLength.load());
    const int    spb         = juce::jmax (1, s.stepsPerBeat.load());
    const double stepLen     = 1.0 / (double) spb;
    const int    gateSamples = juce::jmax (1, (int) (stepLen * samplesPerBeat * s.gate.load() / 100.0));

    long long k = (long long) std::floor (position / stepLen) - 2;
    if (k < 0) k = 0;
    if (lastStepIndex > k + 3) lastStepIndex = k - 1;       // saut en arrière (boucle, repositionnement)

    const long long kEnd = (long long) std::ceil ((position + blockLenBeats) / stepLen) + 1;

    for (; k <= kEnd; ++k)
    {
        const double t = stepTime (k, stepLen);
        if (t < position || t >= position + blockLenBeats) continue;
        if (k <= lastStepIndex) continue;
        lastStepIndex = k;

        const int step = (int) (k % len);
        diag.currentStep.store (step + 1);
        const int offset = juce::jlimit (0, numSamples - 1, (int) ((t - position) * samplesPerBeat));

        if (recordingTies)
            recordTiesAtStep (track, k);

        const int    nextStep       = (int) ((k + 1) % len);
        const double samplesToNext  = (stepTime (k + 1, stepLen) - t) * samplesPerBeat;

        for (auto& n : s.tracks[track - 1].notes[step])
        {
            const int note = n.note.load();
            if (note < 0) continue;
            const bool tied = n.tie.load() != 0;

            // Durée : jusqu'au pas suivant (plus une marge) si la note y est liée, sinon le gate.
            bool nextTied = false;
            for (auto& m : s.tracks[track - 1].notes[nextStep])
                if (m.note.load() == note && m.tie.load()) { nextTied = true; break; }
            const int duration = nextTied ? (int) (samplesToNext + 0.5 * stepLen * samplesPerBeat) : gateSamples;

            if (n.skipOnce.exchange (0) != 0)
                continue;

            // Transposition en temps réel : la boucle garde la note d'origine, seule la note émise change.
            // Les notes déjà en train de sonner se terminent à leur hauteur d'origine (leur fin est déjà programmée).
            const int outNote = note + curTranspose;
            if (outNote < 0 || outNote > 127)
                continue;                                        // hors du clavier après transposition

            if (tied && extendNoteOff (outNote, offset + duration))
                continue;                                        // la note continue, pas de nouvelle attaque

            // Si la même note est encore tenue (gate long, swing), on la coupe juste avant.
            for (int i = 0; i < numPending; ++i)
                if (pending[i].note == outNote)
                {
                    emitLoop (midi, juce::MidiMessage::noteOff (1, outNote), offset);
                    pending[i] = pending[--numPending];
                    break;
                }

            emitLoop (midi, juce::MidiMessage::noteOn (1, outNote, (juce::uint8) juce::jlimit (1, 127, n.velocity.load())), offset);
            addNoteOff (outNote, offset + duration);
            diag.notesEmitted.fetch_add (1);
        }
    }
}

// =====================================================================
// Bloc audio
// =====================================================================
void JPRLooperProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Une instance toute neuve ne fait rien pendant une demi-seconde, sauf si l'hôte lui a déjà rendu
    // son état. Sans cela, au chargement d'un concert ou d'un projet, elle se présentait un instant comme
    // la piste 1 avec les réglages par défaut et les imposait à tout le monde : la fenêtre du chef
    // affichait les bonnes valeurs, mais on entendait celles par défaut.
    if (stateGeneration.load() == 1 && juce::Time::getMillisecondCounter() - createdMs < settleMs)
        return;

    auto& s = SharedState::get();
    const int numSamples = buffer.getNumSamples();
    const int track      = juce::jlimit (1, SharedState::maxTracks, trackParam->getIndex() + 1);
    trackNumber.store (track);
    diag.blocks.fetch_add (1);

    // ---- Signe de vie et élection du chef ----
    const long long now = s.tick.fetch_add (1) + 1;
    s.lastSeen[track - 1].store (now);

    int leader = 0, aliveMask = 0, aliveCount = 0;
    for (int t = 0; t < SharedState::maxTracks; ++t)
    {
        const long long seen = s.lastSeen[t].load();
        if (seen > 0 && now - seen <= aliveWindow)
        {
            aliveMask |= (1 << t);
            ++aliveCount;
            if (leader == 0) leader = t + 1;
        }
    }
    const long long dedupWindow = 2 * aliveCount + 1;
    const bool amLeader = (leader == track);
    leaderFlag.store (amLeader ? 1 : 0);
    s.leaderTrack.store (leader);
    diag.aliveMask.store (aliveMask);

    pushParamsToShared (amLeader, track);

    // ---- Transport ----
    bool hostPlaying = false, hostRecording = false, hasPpq = false;
    double ppq = 0.0, bpm = 0.0;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            hostPlaying   = pos->getIsPlaying();
            hostRecording = pos->getIsRecording();
            if (auto b = pos->getBpm())           bpm = *b;
            if (auto p = pos->getPpqPosition())   { ppq = *p; hasPpq = true; }
            diag.hostState.store (hostRecording ? 3 : hostPlaying ? 2 : 1);
        }
        else diag.hostState.store (0);
    }
    else diag.hostState.store (0);

    // Une seule horloge pour tout le monde, celle de l'hôte : seul le chef la lit, les suiveurs se
    // calent sur la position qu'il publie. Les hôtes (MainStage en particulier) ne donnent pas
    // forcément la même position à chaque strip, ce qui décalait les pistes entre elles.
    const bool leaderUsesHost = hostPlaying && hasPpq;
    if (amLeader)
    {
        s.hostPlaying.store (leaderUsesHost ? 1 : 0);
        s.hostRecording.store (hostRecording ? 1 : 0);
        if (bpm > 0.0) s.hostBpm.store (bpm);
    }
    // ---- Mode Logic Pro (suivi par le chef) : à l'arrêt de l'hôte, la lecture du looper est coupée
    // et toutes les pistes sont désarmées. La région enregistrée dans Logic se rejoue donc seule, sans
    // la boucle par-dessus et sans la modifier ; lecture et armement se réactivent à la main.
    if (amLeader)
    {
        const unsigned wallMs = juce::Time::getMillisecondCounter();
        const bool     option = s.logicMode.load() != 0;
        bool anyDisarmed = false;

        if (hostPlaying)
        {
            // Si l'hôte n'a plus appelé le plug-in pendant l'arrêt, l'arrêt se reconnaît au trou dans le temps.
            // Ce qui a été réactivé à la main depuis cet arrêt reste actif.
            const unsigned lastPlay = s.lastHostPlayMs.load();
            if (option && s.transportSeenPlaying.load() != 0 && wallMs - lastPlay > 500u)
            {
                for (int t = 0; t < SharedState::maxTracks; ++t)
                    if ((int) (s.armedAtMs[t].load() - lastPlay) <= 0 && s.armed[t].exchange (0) != 0)
                        anyDisarmed = true;
                if ((int) (s.runningAtMs.load() - lastPlay) <= 0 && s.running.exchange (0) != 0)
                    anyDisarmed = true;
            }
            s.lastHostPlayMs.store (wallMs);
            s.transportSeenPlaying.store (1);
        }
        else if (s.transportSeenPlaying.exchange (0) != 0 && option)
        {
            for (int t = 0; t < SharedState::maxTracks; ++t)
                if (s.armed[t].exchange (0) != 0) anyDisarmed = true;
            if (s.running.exchange (0) != 0) anyDisarmed = true;
        }
        if (anyDisarmed) s.pushAnnounce (annAutoDisarm, 0);
    }

    const bool followHost = ! amLeader && s.hostPlaying.load() != 0;
    if (! amLeader) bpm = s.hostBpm.load();
    if (bpm <= 0.0) bpm = 120.0;
    diag.bpm.store (bpm);
    const double samplesPerBeat = currentSampleRate * 60.0 / bpm;
    const double blockLenBeats  = (double) numSamples / samplesPerBeat;

    // Chaque suiveur tient sa propre horloge, continue d'un bloc à l'autre, recalée sur le chef si besoin.
    double position = 0.0;
    bool   haveClock = true;
    if (amLeader && leaderUsesHost)
    {
        position = ppq;
        s.leaderPos.store (ppq + blockLenBeats);
        diag.clockSource.store (1);
    }
    else if (followHost)
    {
        const double reference = s.leaderPos.load();
        if (! localClockValid || std::fabs (localPos - reference) > 2.0 * blockLenBeats)
            localPos = reference;
        position = localPos;
        diag.clockSource.store (1);
    }
    else
    {
        haveClock = false;
        diag.clockSource.store (0);
    }
    if (haveClock)
    {
        localPos        = position + blockLenBeats;
        localClockValid = true;
    }

    // Sortie MIDI virtuelle : 0 aucune, 1 les boucles, 2 les boucles et le clavier transmis
    const int midiOutMode = (midiOutPtr.load() != nullptr) ? juce::jlimit (0, 2, s.midiOut.load()) : 0;
    tapBuffer.clear();
    tap = (midiOutMode != 0) ? &tapBuffer : nullptr;

    const int    spb     = juce::jmax (1, s.stepsPerBeat.load());
    const double stepLen = 1.0 / (double) spb;

    // Swing (100 % = ternaire) et groove pour ce bloc
    curSwingDelay   = stepLen * (s.swing.load() / 100.0) / 3.0;
    curGrooveScale  = stepLen * (s.grooveAmount.load() / 100.0) / 100.0;
    curGrooveLength = juce::jlimit (1, SharedState::grooveSteps, s.grooveLength.load());

    // ---- MIDI entrant ----
    auto& out = outBuffer;
    out.clear();
    const unsigned nowMs = juce::Time::getMillisecondCounter();
    const bool commandBefore = s.commandMode.load() != 0;
    const bool transBefore   = s.transposeMode.load() != 0;
    const int  recMode = juce::jlimit (0, 2, s.recMode.load());
    const int  trigType   = s.cmdTrigType.load();           // 0 = contrôleur MIDI, 1 = note du clavier
    const int  trigNumber = s.cmdTrigNumber.load();
    const int  transTrigType   = s.transTrigType.load();    // 0 = contrôleur MIDI, 1 = note du clavier
    const int  transTrigNumber = s.transTrigNumber.load();

    // Transposition de cette piste : aucune pour une piste de batterie.
    const bool isDrum = drumParam->getIndex() == 1;
    curTranspose = isDrum ? 0 : juce::jlimit (-24, 24, s.transpose.load());

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int  pos = meta.samplePosition;

        // Déclencheur du mode commande : jamais transmis à l'instrument, jamais enregistré dans la boucle.
        if (trigType == 0 && msg.isController() && msg.getControllerNumber() == trigNumber)
        {
            handleCommandSwitch (msg.getControllerValue() >= 64, now, dedupWindow);
            continue;
        }
        if (trigType == 1 && msg.isNoteOnOrOff() && msg.getNoteNumber() == trigNumber)
        {
            handleCommandSwitch (msg.isNoteOn(), now, dedupWindow);
            continue;
        }

        // Déclencheur du mode transposition : même principe, jamais transmis ni enregistré.
        if (transTrigType == 0 && msg.isController() && msg.getControllerNumber() == transTrigNumber)
        {
            handleTransposeSwitch (msg.getControllerValue() >= 64, now, dedupWindow);
            continue;
        }
        if (transTrigType == 1 && msg.isNoteOnOrOff() && msg.getNoteNumber() == transTrigNumber)
        {
            handleTransposeSwitch (msg.isNoteOn(), now, dedupWindow);
            continue;
        }

        const bool commandMode = s.commandMode.load() != 0;
        // Clavier muet : aucune piste ne reçoit le clavier (ni instrument, ni enregistrement) ; les commandes restent actives.
        const bool isActive    = s.inputMute.load() == 0
                              && ((s.activeTrack.load() == track) || (inputParam->getIndex() == 1));

        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            if (commandMode)
            {
                handleCommandNote (note, msg.getVelocity(), now, dedupWindow, nowMs);
                continue;
            }
            if (s.transposeMode.load() != 0)
            {
                handleTransposeNote (note, msg.getVelocity(), now, dedupWindow);
                continue;
            }
            if (! isActive) continue;

            out.addEvent (msg, pos);
            heldSinceMs[note]   = (nowMs == 0 ? 1 : nowMs);
            heldVelocity[note]  = msg.getVelocity();
            heldStartStep[note] = -1;
            heldErased[note]    = false;
            {
                const int stored = note - curTranspose;
                heldStoredNote[note] = (stored >= 0 && stored <= 127) ? stored : -1;
            }

            if (s.armed[track - 1].load() && s.running.load() && haveClock)
            {
                const double stepPosition = (position + (double) pos / samplesPerBeat) / stepLen;
                recordNoteOn (track, note, msg.getVelocity(), stepPosition, nowMs);
            }
        }
        else if (msg.isNoteOff())
        {
            const int note = msg.getNoteNumber();
            if (heldSinceMs[note] != 0)
            {
                out.addEvent (msg, pos);
                heldSinceMs[note]   = 0;
                heldStartStep[note] = -1;
            }
        }
        else
        {
            if (isActive && ! commandMode) out.addEvent (msg, pos);
        }
    }

    // ---- Entrée en mode commande, perte du statut actif ou clavier rendu muet : on coupe les touches tenues ----
    const bool commandAfter = s.commandMode.load() != 0;
    const bool transAfter   = s.transposeMode.load() != 0;
    const bool activeNow    = s.inputMute.load() == 0
                           && ((s.activeTrack.load() == track) || (inputParam->getIndex() == 1));
    if ((commandAfter && ! commandBefore) || (transAfter && ! transBefore) || ! activeNow)
        for (int n = 0; n < 128; ++n)
            if (heldSinceMs[n] != 0)
            {
                out.addEvent (juce::MidiMessage::noteOff (1, n), numSamples - 1);
                heldSinceMs[n] = 0;
                heldStartStep[n] = -1;
            }

    midi.swapWith (out);
    if (midiOutMode == 2) tapBuffer.addEvents (midi, 0, numSamples, 0);

    // ---- Effacement par maintien ----
    const bool recording = s.armed[track - 1].load() && s.running.load() && haveClock && activeNow && ! commandAfter && ! transAfter;
    if (recording && recMode == recHoldErase)
        holdToEraseCheck (track, nowMs);

    // ---- Notes de la boucle ----
    // Une touche de transposition reçue dans ce bloc s'applique dès les pas de ce bloc.
    curTranspose = isDrum ? 0 : juce::jlimit (-24, 24, s.transpose.load());
    flushNoteOffs (midi, numSamples);

    bool anySolo = false;
    for (int t = 0; t < SharedState::maxTracks; ++t) if (s.solo[t].load()) anySolo = true;
    const bool audible = haveClock && s.running.load() != 0
                      && ! s.mute[track - 1].load()
                      && (! anySolo || s.solo[track - 1].load());

    if (! audible)
    {
        if (wasAudible) allNotesOff (midi);
        wasAudible = false;
        if (! haveClock || ! s.running.load()) lastStepIndex = -1;
        sendToVirtualOutput (numSamples);
        return;
    }
    wasAudible = true;

    playTrack (midi, track, position, blockLenBeats, samplesPerBeat, numSamples,
               recording && recMode == recNormal);
    sendToVirtualOutput (numSamples);
}

// =====================================================================
// Sortie MIDI virtuelle
// =====================================================================
namespace
{
    std::mutex                       midiOutMutex;
    std::weak_ptr<juce::MidiOutput>  midiOutShared;     // un seul port pour toutes les instances du processus
}

void JPRLooperProcessor::updateMidiOutDevice()
{
    // Le port est libéré un tour de timer après avoir été retiré du fil audio.
    midiOutToRelease.reset();

    const bool wanted = SharedState::get().midiOut.load() != 0;
    if (wanted && midiOutDevice == nullptr)
    {
        std::lock_guard<std::mutex> lock (midiOutMutex);
        auto dev = midiOutShared.lock();
        if (dev == nullptr)
        {
            if (auto created = juce::MidiOutput::createNewDevice ("JPR MIDI Looper"))
            {
                dev = std::shared_ptr<juce::MidiOutput> (created.release());
                dev->startBackgroundThread();
                midiOutShared = dev;
            }
        }
        midiOutDevice = dev;
        midiOutPtr.store (dev.get());
    }
    else if (! wanted && midiOutDevice != nullptr)
    {
        midiOutPtr.store (nullptr);
        midiOutToRelease = midiOutDevice;
        midiOutDevice.reset();
    }
}

void JPRLooperProcessor::sendToVirtualOutput (int numSamples)
{
    auto* dev = midiOutPtr.load();
    tap = nullptr;
    if (dev == nullptr || tapBuffer.isEmpty()) return;

    // Chaque piste sort sur son propre canal MIDI (piste 1 = canal 1, etc.).
    const int channel = juce::jlimit (1, 16, trackNumber.load());
    juce::MidiBuffer channelled;
    for (const auto meta : tapBuffer)
    {
        auto m = meta.getMessage();
        if (m.getChannel() > 0) m.setChannel (channel);
        channelled.addEvent (m, meta.samplePosition);
    }

    // Les événements partent avec leur position dans le bloc, un bloc après l'instant présent
    // (latence constante égale à la taille du tampon de l'hôte, timing interne préservé).
    const double blockMs = 1000.0 * (double) numSamples / currentSampleRate;
    dev->sendBlockOfMessages (channelled, (double) juce::Time::getMillisecondCounter() + blockMs, currentSampleRate);
}

// =====================================================================
// Éditeur et sauvegarde
// =====================================================================
juce::AudioProcessorEditor* JPRLooperProcessor::createEditor()
{
    return new JPRLooperEditor (*this);
}

void JPRLooperProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto& s = SharedState::get();
    auto xml = apvts.copyState().createXml();
    if (xml == nullptr) return;

    const int t = juce::jlimit (0, SharedState::maxTracks - 1, trackParam->getIndex());
    juce::String data;
    for (int k = 0; k < SharedState::maxSteps; ++k)
        for (int v = 0; v < SharedState::maxPoly; ++v)
        {
            auto& n = s.tracks[t].notes[k][v];
            const int note = n.note.load();
            if (note >= 0)
                data << k << ":" << note << ":" << n.velocity.load() << ":" << n.tie.load() << ";";
        }

    auto* pat = xml->createNewChildElement ("PATTERN");
    pat->setAttribute ("track", t + 1);
    pat->setAttribute ("notes", data);

    juce::String groove;
    for (int g = 0; g < SharedState::grooveSteps; ++g)
        groove << s.grooveTable[g].load() << (g + 1 < SharedState::grooveSteps ? "," : "");
    auto* gr = xml->createNewChildElement ("GROOVE");
    gr->setAttribute ("table", groove);

    // Version 2 : la note de base des commandes désigne le Do de lecture / arrêt (avant : une octave au-dessus).
    // Version 3 : la liste « Pas par temps » contient 5 et 7 (avant : 1, 2, 3, 4, 6, 8).
    xml->setAttribute ("stateVersion", 3);

    copyXmlToBinary (*xml, destData);
}

void JPRLooperProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    auto& s = SharedState::get();
    stateGeneration.fetch_add (1);                 // le timer ne touche plus aux paramètres pendant le chargement
    const int version = xml->getIntAttribute ("stateVersion", 1);

    if (auto* pat = xml->getChildByName ("PATTERN"))
    {
        const int t = pat->getIntAttribute ("track", 1) - 1;
        if (t >= 0 && t < SharedState::maxTracks)
        {
            s.armed[t].store (0);
            s.clearTrack (t);
            juce::StringArray items;
            items.addTokens (pat->getStringAttribute ("notes"), ";", "");
            for (const auto& item : items)
            {
                juce::StringArray f;
                f.addTokens (item, ":", "");
                if (f.size() < 3) continue;
                const int k = f[0].getIntValue(), note = f[1].getIntValue(), vel = f[2].getIntValue();
                const int tie = f.size() >= 4 ? f[3].getIntValue() : 0;
                if (k < 0 || k >= SharedState::maxSteps || note < 0 || note > 127) continue;
                for (auto& n : s.tracks[t].notes[k])
                    if (n.note.load() < 0) { n.velocity.store (vel); n.tie.store (tie); n.note.store (note); break; }
            }
        }
        xml->removeChildElement (pat, true);
    }

    if (auto* gr = xml->getChildByName ("GROOVE"))
    {
        juce::StringArray vals;
        vals.addTokens (gr->getStringAttribute ("table"), ",", "");
        for (int g = 0; g < juce::jmin (vals.size(), SharedState::grooveSteps); ++g)
            s.grooveTable[g].store (juce::jlimit (-50, 50, vals[g].getIntValue()));
        xml->removeChildElement (gr, true);
    }

    // Une piste n'est jamais armée au chargement : l'armement est une action de la session en cours.
    // Les réglages « Horloge » et « seulement si l'hôte enregistre » des anciennes versions n'existent plus : on les ignore.
    for (auto* e = xml->getFirstChildElement(); e != nullptr;)
    {
        auto* next = e->getNextElement();
        if (e->hasTagName ("PARAM") && (e->getStringAttribute ("id") == "armed" || e->getStringAttribute ("id") == "inputMute"))
            e->setAttribute ("value", 0.0);
        else if (e->hasTagName ("PARAM") && e->getStringAttribute ("id") == "cmdBase" && version < 2)
            // Anciens projets : mêmes touches qu'avant, la note de base descend d'une octave.
            e->setAttribute ("value", (double) juce::jlimit (12, 96, juce::roundToInt (e->getDoubleAttribute ("value", 60.0)) - 12));
        else if (e->hasTagName ("PARAM") && e->getStringAttribute ("id") == "spb" && version < 3)
        {
            // Anciens projets : la liste était 1, 2, 3, 4, 6, 8. On retrouve la même valeur dans la nouvelle liste.
            const int oldValues[] = { 1, 2, 3, 4, 6, 8 };
            const int oldIndex = juce::jlimit (0, 5, juce::roundToInt (e->getDoubleAttribute ("value", 3.0)));
            e->setAttribute ("value", (double) spbIndexFor (oldValues[oldIndex]));
        }
        else if (e->hasTagName ("PARAM") && (e->getStringAttribute ("id") == "clock" || e->getStringAttribute ("id") == "hostRecOnly"
                                                || e->getStringAttribute ("id") == "autoDisarm"))
            xml->removeChildElement (e, true);
        e = next;
    }
    xml->setAttribute ("stateVersion", 3);
    s.inputMute.store (0);                         // le clavier n'est jamais muet au chargement
    apvts.replaceState (juce::ValueTree::fromXml (*xml));

    // Le fil audio republie tous les réglages restaurés à son prochain bloc ; d'ici là le timer
    // ne recopie rien de la mémoire partagée vers les paramètres.
    uiLastGrooveStep = -1;
    stateGeneration.fetch_add (1);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JPRLooperProcessor();
}
