#include "PluginEditor.h"
#include "Announcer.h"
#include "Lang.h"

namespace
{
    // Texte bilingue (français, anglais). Tout texte passe par là : JUCE lirait sinon les accents en Latin-1.
    juce::String tx (const char* french, const char* english) { return jprText (french, english); }
    constexpr unsigned actionDelayMs = 1500;

    enum Actions { actNone = 1, actMakeActive, actClearTrack, actClearAll, actUndo, actCommandToggle };
}

void JPRLooperEditor::setupLabel (juce::Label& label, const juce::String& text, juce::Component& target)
{
    label.setText (text, juce::dontSendNotification);
    label.attachToComponent (&target, true);
    addAndMakeVisible (label);
}

void JPRLooperEditor::setupHeading (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    label.setAccessible (true);
    label.setTitle (text);
    addAndMakeVisible (label);
}

JPRLooperEditor::JPRLooperEditor (JPRLooperProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    auto& apvts = proc.apvts;

    const juce::StringArray trigNames { tx ("Contrôleur MIDI", "MIDI controller"), tx ("Note du clavier", "Keyboard note") };

    // ================= Piste =================
    setupHeading (headTrack, tx ("Piste", "Track"));

    for (int i = 1; i <= SharedState::maxTracks; ++i) trackBox.addItem (juce::String (i), i);
    trackBox.setTitle (tx ("Piste", "Track"));
    trackBox.setDescription (tx ("Numéro de la boucle jouée par cette instance. La plus petite piste active est le chef.",
                                 "Number of the loop played by this instance. The lowest track present is the leader."));
    addAndMakeVisible (trackBox);
    setupLabel (trackLabel, tx ("Piste", "Track"), trackBox);
    trackAtt = std::make_unique<Apvts::ComboBoxAttachment> (apvts, "track", trackBox);

    inputBox.addItemList ({ tx ("Piste active seulement", "Active track only"), tx ("Toujours", "Always") }, 1);
    inputBox.setTitle (tx ("Entrée clavier", "Keyboard input"));
    inputBox.setDescription (tx ("Quand le clavier est transmis à l'instrument de ce strip.",
                                 "When the keyboard is passed to the instrument of this channel strip."));
    addAndMakeVisible (inputBox);
    setupLabel (inputLabel, tx ("Entrée clavier", "Keyboard input"), inputBox);
    inputAtt = std::make_unique<Apvts::ComboBoxAttachment> (apvts, "input", inputBox);

    muteButton.setButtonText (tx ("Muet", "Mute"));
    muteButton.setTitle (tx ("Muet", "Mute"));
    addAndMakeVisible (muteButton);
    muteAtt = std::make_unique<Apvts::ButtonAttachment> (apvts, "mute", muteButton);

    soloButton.setTitle ("Solo");
    addAndMakeVisible (soloButton);
    soloAtt = std::make_unique<Apvts::ButtonAttachment> (apvts, "solo", soloButton);

    recModeBox.addItemList ({ tx ("Normal : rejouer pour enlever, notes liées", "Normal: replay to remove, tied notes"),
                              tx ("Rejouer pour enlever, sans notes liées", "Replay to remove, no tied notes"),
                              tx ("Maintenir pour effacer", "Hold to erase") }, 1);
    recModeBox.setTitle (tx ("Mode d'enregistrement", "Recording mode"));
    recModeBox.setDescription (tx ("Réglage global, modifiable depuis la fenêtre du chef ou par la touche Fa de l'octave des actions en mode commande. "
                                   "Normal : rejouer une note sur son pas la supprime, et une touche tenue au-delà d'un pas crée une note liée. "
                                   "Rejouer pour enlever, sans notes liées : idem, mais sans notes liées. "
                                   "Maintenir pour effacer : tenir une touche une demi-seconde efface cette note de toute la piste.",
                                   "Global setting, changed from the leader's window or with the F key of the actions octave in command mode. "
                                   "Normal: replaying a note on its step removes it, and a key held beyond one step creates a tied note. "
                                   "Replay to remove, no tied notes: the same, but without tied notes. "
                                   "Hold to erase: holding a key for half a second erases that note from the whole track."));
    addAndMakeVisible (recModeBox);
    setupLabel (recModeLabel, tx ("Enregistrement", "Recording"), recModeBox);
    recModeAtt = std::make_unique<Apvts::ComboBoxAttachment> (apvts, "recMode", recModeBox);

    armedButton.setButtonText (tx ("Armé pour l'enregistrement", "Armed for recording"));
    armedButton.setTitle (tx ("Armé pour l'enregistrement", "Armed for recording"));
    armedButton.setDescription (tx ("Les notes jouées sur la piste active sont ajoutées à la boucle.",
                                    "Notes played on the active track are added to the loop."));
    addAndMakeVisible (armedButton);
    armedAtt = std::make_unique<Apvts::ButtonAttachment> (apvts, "armed", armedButton);

    drumBox.addItemList ({ tx ("Normale", "Normal"), tx ("Batterie", "Drums") }, 1);
    drumBox.setTitle (tx ("Type de piste", "Track type"));
    drumBox.setDescription (tx ("Normale : la piste suit la transposition. Batterie : la piste n'est jamais transposée. "
                                "Réglage propre à chaque piste.",
                                "Normal: the track follows the transposition. Drums: the track is never transposed. "
                                "Set separately for each track."));
    addAndMakeVisible (drumBox);
    setupLabel (drumLabel, tx ("Type de piste", "Track type"), drumBox);
    drumAtt = std::make_unique<Apvts::ComboBoxAttachment> (apvts, "drum", drumBox);

    // ================= Global =================
    setupHeading (headGlobal, tx ("Réglages globaux (le chef décide)", "Global settings (the leader decides)"));

    auto setupSlider = [this] (juce::Slider& sl, juce::Label& lb, const juce::String& name, const juce::String& desc)
    {
        sl.setSliderStyle (juce::Slider::LinearHorizontal);
        sl.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 24);
        sl.setTitle (name);
        sl.setDescription (desc);
        addAndMakeVisible (sl);
        setupLabel (lb, name, sl);
    };

    setupSlider (lengthSlider, lengthLabel, tx ("Longueur", "Length"),
                 tx ("Nombre de pas du pattern, de 1 à 64.", "Number of steps in the pattern, from 1 to 64."));
    lengthAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "length", lengthSlider);

    spbBox.addItemList ({ "1", "2", "3", "4", "5", "6", "7", "8" }, 1);
    spbBox.setTitle (tx ("Pas par temps", "Steps per beat"));
    addAndMakeVisible (spbBox);
    setupLabel (spbLabel, tx ("Pas par temps", "Steps per beat"), spbBox);
    spbAtt = std::make_unique<Apvts::ComboBoxAttachment> (apvts, "spb", spbBox);

    setupSlider (swingSlider, swingLabel, "Swing", tx ("0 = droit, 100 = ternaire.", "0 = straight, 100 = triplet feel."));
    swingAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "swing", swingSlider);

    setupSlider (gateSlider, gateLabel, "Gate", tx ("Durée des notes en pourcentage du pas.", "Note length as a percentage of the step."));
    gateAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "gate", gateSlider);

    logicModeBox.addItemList ({ "MainStage", "Logic Pro" }, 1);
    logicModeBox.setTitle (tx ("Mode de l'hôte", "Host mode"));
    logicModeBox.setDescription (tx ("MainStage : le looper fonctionne comme d'habitude. Logic Pro : dès que Logic s'arrête, la lecture du "
                                     "looper est coupée et toutes les pistes sont désarmées ; la région enregistrée se rejoue donc seule. "
                                     "Lecture et armement se réactivent à la main avant chaque nouvelle prise.",
                                     "MainStage: the looper works as usual. Logic Pro: as soon as Logic stops, looper playback is switched "
                                     "off and all tracks are disarmed, so the recorded region plays back on its own. "
                                     "Playback and arming are switched back on by hand before each new take."));
    addAndMakeVisible (logicModeBox);
    setupLabel (logicModeLabel, tx ("Mode de l'hôte", "Host mode"), logicModeBox);
    logicModeAtt = std::make_unique<Apvts::ComboBoxAttachment> (apvts, "logicMode", logicModeBox);

    midiOutBox.addItemList ({ tx ("Désactivée", "Off"), tx ("Boucles seulement", "Loops only"), tx ("Boucles et clavier", "Loops and keyboard") }, 1);
    midiOutBox.setTitle (tx ("Sortie MIDI virtuelle", "Virtual MIDI output"));
    midiOutBox.setDescription (tx ("Crée un port MIDI « JPR MIDI Looper » visible par Logic et MainStage. Chaque piste y sort sur son canal "
                                   "(piste 1 = canal 1). Dans Logic, régler le port d'entrée MIDI d'une autre piste sur ce port pour y "
                                   "enregistrer la sortie du looper, et celui de la piste du looper sur le clavier seulement.",
                                   "Creates a MIDI port named JPR MIDI Looper, visible to Logic and MainStage. Each track is sent on its own "
                                   "channel (track 1 = channel 1). In Logic, set the MIDI input port of another track to this port to "
                                   "record the looper output there, and set the port of the looper track to the keyboard only."));
    addAndMakeVisible (midiOutBox);
    setupLabel (midiOutLabel, tx ("Sortie MIDI virtuelle", "Virtual MIDI output"), midiOutBox);
    midiOutAtt = std::make_unique<Apvts::ComboBoxAttachment> (apvts, "midiOut", midiOutBox);

    setupSlider (grooveAmountSlider, grooveAmountLabel, tx ("Intensité du groove", "Groove amount"),
                 tx ("Pourcentage appliqué aux décalages de la table de groove. 0 = aucun groove.",
                     "Percentage applied to the offsets of the groove table. 0 = no groove."));
    grooveAmountAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "grooveAmount", grooveAmountSlider);

    setupSlider (grooveLengthSlider, grooveLengthLabel, tx ("Longueur du groove", "Groove length"),
                 tx ("Nombre de pas du cycle de groove, de 1 à 32. Le cycle repart au début du pattern.",
                     "Number of steps in the groove cycle, from 1 to 32. The cycle restarts at the beginning of the pattern."));
    grooveLengthAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "grooveLength", grooveLengthSlider);

    setupSlider (grooveStepSlider, grooveStepLabel, tx ("Pas de groove", "Groove step"),
                 tx ("Choisit le pas dont on règle le décalage, de 1 à 32.",
                     "Selects the step whose offset is being set, from 1 to 32."));
    grooveStepAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "grooveStep", grooveStepSlider);

    setupSlider (grooveValueSlider, grooveValueLabel, tx ("Décalage du pas", "Step offset"),
                 tx ("Décalage du pas choisi, en pourcentage du pas, de moins 50 à plus 50. Négatif = en avance.",
                     "Offset of the selected step, as a percentage of the step, from minus 50 to plus 50. Negative = early."));
    grooveValueAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "grooveValue", grooveValueSlider);

    setupSlider (cmdBaseSlider, cmdBaseLabel, tx ("Base des commandes", "Command base note"),
                 tx ("Note MIDI de la première commande : le Do de lecture et arrêt, début de l'octave des actions. "
                     "L'octave des pistes commence douze demi-tons plus haut. 48 = Do2, valeur par défaut.",
                     "MIDI note of the first command: the C for play and stop, start of the actions octave. "
                     "The tracks octave starts twelve semitones higher. 48 = C2, the default."));
    cmdBaseAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "cmdBase", cmdBaseSlider);

    cmdTrigTypeBox.addItemList (trigNames, 1);
    cmdTrigTypeBox.setTitle (tx ("Déclencheur du mode commande", "Command mode trigger"));
    cmdTrigTypeBox.setDescription (tx ("Ce qui fait entrer en mode commande tant qu'il est maintenu : un contrôleur MIDI (pédale ou bouton) "
                                       "ou une touche du clavier. Le déclencheur n'est jamais transmis à l'instrument.",
                                       "What switches to command mode for as long as it is held: a MIDI controller (pedal or button) "
                                       "or a key on the keyboard. The trigger is never passed to the instrument."));
    addAndMakeVisible (cmdTrigTypeBox);
    setupLabel (cmdTrigTypeLabel, tx ("Déclencheur commande", "Command trigger"), cmdTrigTypeBox);
    cmdTrigTypeAtt = std::make_unique<Apvts::ComboBoxAttachment> (apvts, "cmdTrigType", cmdTrigTypeBox);

    setupSlider (cmdTrigNumberSlider, cmdTrigNumberLabel, tx ("Numéro du déclencheur", "Trigger number"),
                 tx ("Numéro du contrôleur MIDI, ou numéro de la note du clavier, de 0 à 127. 65 par défaut. "
                     "Pour une note : 60 = Do3, 21 = La le plus grave d'un clavier 88 touches.",
                     "MIDI controller number, or keyboard note number, from 0 to 127. Default 65. "
                     "For a note: 60 = C3, 21 = the lowest A of an 88-key keyboard."));
    cmdTrigNumberAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "cmdTrigNumber", cmdTrigNumberSlider);

    setupSlider (transposeSlider, transposeLabel, tx ("Transposition", "Transpose"),
                 tx ("Transposition des boucles en demi-tons, de moins 24 à plus 24, appliquée en temps réel à toutes les pistes "
                     "sauf les pistes de batterie. Se règle aussi au clavier en mode transposition.",
                     "Transposition of the loops in semitones, from minus 24 to plus 24, applied in real time to all tracks "
                     "except drum tracks. Can also be set from the keyboard in transpose mode."));
    transposeAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "transpose", transposeSlider);

    setupSlider (transBaseSlider, transBaseLabel, tx ("Base de la transposition", "Transpose base note"),
                 tx ("Note MIDI de la touche qui remet la transposition à zéro en mode transposition. Chaque demi-ton au-dessus "
                     "ou en dessous transpose d'autant. 60 = Do3, valeur par défaut.",
                     "MIDI note of the key that resets the transposition to zero in transpose mode. Each semitone above "
                     "or below transposes by that amount. 60 = C3, the default."));
    transBaseAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "transBase", transBaseSlider);

    transTrigTypeBox.addItemList (trigNames, 1);
    transTrigTypeBox.setTitle (tx ("Déclencheur du mode transposition", "Transpose mode trigger"));
    transTrigTypeBox.setDescription (tx ("Ce qui fait entrer en mode transposition tant qu'il est maintenu : un contrôleur MIDI ou une touche "
                                         "du clavier. Dans ce mode, la touche jouée fixe la transposition et ne joue pas l'instrument. "
                                         "Doit être différent du déclencheur du mode commande.",
                                         "What switches to transpose mode for as long as it is held: a MIDI controller or a key on the "
                                         "keyboard. In this mode, the key played sets the transposition and does not play the instrument. "
                                         "Must be different from the command mode trigger."));
    addAndMakeVisible (transTrigTypeBox);
    setupLabel (transTrigTypeLabel, tx ("Déclencheur transpo.", "Transpose trigger"), transTrigTypeBox);
    transTrigTypeAtt = std::make_unique<Apvts::ComboBoxAttachment> (apvts, "transTrigType", transTrigTypeBox);

    setupSlider (transTrigNumberSlider, transTrigNumberLabel, tx ("Numéro déclencheur transpo.", "Transpose trigger number"),
                 tx ("Numéro du contrôleur MIDI, ou numéro de la note du clavier, qui déclenche le mode transposition, de 0 à 127. "
                     "66 par défaut.",
                     "MIDI controller number, or keyboard note number, that triggers transpose mode, from 0 to 127. "
                     "Default 66."));
    transTrigNumberAtt = std::make_unique<Apvts::SliderAttachment> (apvts, "transTrigNumber", transTrigNumberSlider);

    runningButton.setButtonText (tx ("Lecture", "Playback"));
    runningButton.setTitle (tx ("Lecture", "Playback"));
    addAndMakeVisible (runningButton);
    runningAtt = std::make_unique<Apvts::ButtonAttachment> (apvts, "running", runningButton);

    inputMuteButton.setButtonText (tx ("Clavier muet", "Keyboard muted"));
    inputMuteButton.setTitle (tx ("Clavier muet", "Keyboard muted"));
    inputMuteButton.setDescription (tx ("Le clavier n'est plus transmis aux instruments ni enregistré dans les boucles, qui continuent de jouer. "
                                        "Les commandes restent actives. Touche Sol de l'octave des actions en mode commande. "
                                        "Jamais actif au chargement d'un projet ou d'un concert.",
                                        "The keyboard is no longer passed to the instruments or recorded into the loops, which keep playing. "
                                        "Commands stay active. G key of the actions octave in command mode. "
                                        "Never on when a project or concert is loaded."));
    addAndMakeVisible (inputMuteButton);
    inputMuteAtt = std::make_unique<Apvts::ButtonAttachment> (apvts, "inputMute", inputMuteButton);

    // ================= Actions =================
    setupHeading (headActions, "Actions");

    actionBox.addItem (tx ("Aucune", "None"), actNone);
    actionBox.addItem (tx ("Rendre cette piste active", "Make this track active"), actMakeActive);
    actionBox.addItem (tx ("Effacer cette piste", "Clear this track"), actClearTrack);
    actionBox.addItem (tx ("Tout effacer", "Clear all"), actClearAll);
    actionBox.addItem (tx ("Annuler", "Undo"), actUndo);
    actionBox.addItem (tx ("Basculer le mode commande", "Toggle command mode"), actCommandToggle);
    actionBox.setSelectedId (actNone, juce::dontSendNotification);
    actionBox.setTitle ("Action");
    actionBox.setDescription (tx ("L'action choisie s'exécute une seconde et demie après la sélection.",
                                  "The selected action runs one and a half seconds after it is chosen."));
    actionBox.onChange = [this]
    {
        pendingAction  = actionBox.getSelectedId();
        pendingSinceMs = juce::Time::getMillisecondCounter();
    };
    addAndMakeVisible (actionBox);
    setupLabel (actionLabel, "Action", actionBox);

    // ================= État =================
    setupHeading (headStatus, tx ("État", "Status"));
    statusLabel.setTitle (tx ("État", "Status"));
    statusLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (statusLabel);

    setSize (560, 990);
    startTimerHz (5);
    timerCallback();
}

void JPRLooperEditor::runPendingAction()
{
    auto& s = SharedState::get();
    const int action = pendingAction;
    pendingAction = 0;
    actionBox.setSelectedId (actNone, juce::dontSendNotification);

    switch (action)
    {
        case actMakeActive:
            s.activeTrack.store (proc.getTrack());
            jprAnnounce ((tx ("Piste ", "Track ") + juce::String (proc.getTrack()) + " active").toRawUTF8());
            break;
        case actClearTrack:  proc.clearThisTrack(); break;
        case actClearAll:    proc.clearAllTracks(); break;
        case actUndo:        proc.undo(); break;
        case actCommandToggle:
        {
            const int v = s.commandMode.load() ? 0 : 1;
            s.commandMode.store (v);
            s.cc65PressedAt.store (0);
            jprAnnounce ((v ? tx ("Mode commande", "Command mode") : tx ("Clavier normal", "Normal keyboard")).toRawUTF8());
            break;
        }
        default: break;
    }
}

void JPRLooperEditor::timerCallback()
{
    if (pendingAction > actNone && juce::Time::getMillisecondCounter() - pendingSinceMs >= actionDelayMs)
        runPendingAction();

    auto& s = SharedState::get();
    auto& d = proc.diag;
    const juce::String hostStates[] = { tx ("aucune information", "no information"), tx ("arrêté", "stopped"),
                                        tx ("en lecture", "playing"), tx ("en enregistrement", "recording") };
    const juce::String clockNames[] = { tx ("aucune (les boucles tournent quand l'hôte joue)", "none (the loops run when the host plays)"),
                                        tx ("hôte", "host") };
    const juce::String midiOutNames[] = { tx ("désactivée", "off"), tx ("boucles", "loops"), tx ("boucles et clavier", "loops and keyboard") };

    juce::String alive;
    const int mask = d.aliveMask.load();
    for (int t = 0; t < SharedState::maxTracks; ++t)
        if (mask & (1 << t)) alive << (alive.isEmpty() ? "" : ", ") << (t + 1);
    if (alive.isEmpty()) alive = tx ("aucune", "none");

    juce::String text;
    text << tx ("Piste ", "Track ") << proc.getTrack()
         << (proc.isLeader() ? tx (", chef", ", leader") : tx (", chef : piste ", ", leader: track ") + juce::String (s.leaderTrack.load()))
         << tx (". Piste active : ", ". Active track: ") << s.activeTrack.load()
         << tx (". Mode commande : ", ". Command mode: ") << (s.commandMode.load() ? tx ("oui", "yes") : tx ("non", "no"))
         << tx (". Transposition : ", ". Transpose: ") << s.transpose.load() << tx (" demi-tons", " semitones")
         << (s.transposeMode.load() ? tx (", mode transposition", ", transpose mode") : juce::String())
         << tx (". Clavier : ", ". Keyboard: ") << (s.inputMute.load() ? tx ("muet", "muted") : tx ("actif", "active"))
         << tx (". Pistes actives : ", ". Tracks present: ") << alive
         << tx (". Pas utilisés : ", ". Steps used: ") << d.stepsUsed.load() << tx (" sur ", " of ") << s.patternLength.load()
         << tx (". Pas courant : ", ". Current step: ") << d.currentStep.load()
         << ". Transport : " << hostStates[juce::jlimit (0, 3, d.hostState.load())]
         << tx (", horloge ", ", clock ") << clockNames[juce::jlimit (0, 1, d.clockSource.load())]
         << ", tempo " << juce::String (d.bpm.load(), 1)
         << tx (". Sortie MIDI virtuelle : ", ". Virtual MIDI output: ") << midiOutNames[juce::jlimit (0, 2, s.midiOut.load())]
         << tx (". Notes émises : ", ". Notes sent: ") << d.notesEmitted.load()
         << tx (". Annonces : ", ". Announcements: ") << juce::String::fromUTF8 (jprAnnounceDiagnostic()) << ".";

    if (text != statusLabel.getText())
        statusLabel.setText (text, juce::dontSendNotification);
}

void JPRLooperEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void JPRLooperEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    const int labelWidth = 170;
    const int rowHeight  = 24;
    const int gap        = 3;

    auto heading = [&] (juce::Label& l)
    {
        l.setBounds (area.removeFromTop (rowHeight));
        area.removeFromTop (gap);
    };
    auto row = [&] (juce::Component& c)
    {
        auto r = area.removeFromTop (rowHeight);
        r.removeFromLeft (labelWidth);
        c.setBounds (r);
        area.removeFromTop (gap);
    };
    auto full = [&] (juce::Component& c)
    {
        c.setBounds (area.removeFromTop (rowHeight));
        area.removeFromTop (gap);
    };

    heading (headTrack);
    row (trackBox);
    row (inputBox);
    row (recModeBox);
    row (drumBox);
    full (muteButton);
    full (soloButton);
    full (armedButton);

    area.removeFromTop (gap);
    heading (headGlobal);
    row (logicModeBox);
    row (lengthSlider);
    row (spbBox);
    row (swingSlider);
    row (gateSlider);
    row (midiOutBox);
    row (grooveAmountSlider);
    row (grooveLengthSlider);
    row (grooveStepSlider);
    row (grooveValueSlider);
    row (cmdBaseSlider);
    row (cmdTrigTypeBox);
    row (cmdTrigNumberSlider);
    row (transposeSlider);
    row (transBaseSlider);
    row (transTrigTypeBox);
    row (transTrigNumberSlider);
    full (runningButton);
    full (inputMuteButton);

    area.removeFromTop (gap);
    heading (headActions);
    row (actionBox);

    area.removeFromTop (gap);
    heading (headStatus);
    statusLabel.setBounds (area);
}
