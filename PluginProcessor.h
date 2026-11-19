#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <memory>
#include <mutex>
#include "SharedState.h"

class JPRLooperProcessor : public juce::AudioProcessor,
                           private juce::Timer
{
public:
    JPRLooperProcessor();
    ~JPRLooperProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "JPR MIDI Looper"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    int  getTrack() const { return trackNumber.load(); }
    bool isLeader() const { return leaderFlag.load() != 0; }

    struct Diag
    {
        std::atomic<int>    blocks       { 0 };
        std::atomic<int>    notesEmitted { 0 };
        std::atomic<int>    aliveMask    { 0 };
        std::atomic<int>    hostState    { 0 };   // 0 = pas d'info, 1 = arrêté, 2 = en lecture, 3 = en enregistrement
        std::atomic<int>    clockSource  { 0 };   // 0 = aucune, 1 = hôte
        std::atomic<int>    currentStep  { 0 };
        std::atomic<int>    stepsUsed    { 0 };
        std::atomic<double> bpm          { 0.0 };
    };
    Diag diag;

    // Actions demandées depuis l'interface (fil message)
    void clearThisTrack();
    void clearAllTracks();
    void undo();

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void timerCallback() override;
    void announceNow (int code, int arg);

    void pushParamsToShared (bool leader, int track);
    void handleCommandNote (int note, int velocity, long long now, long long window, unsigned nowMs);
    void handleCommandSwitch (bool pressed, long long now, long long window);
    void handleTransposeSwitch (bool pressed, long long now, long long window);
    void handleTransposeNote (int note, int velocity, long long now, long long window);

    // Enregistrement
    void beginTakeIfNeeded (unsigned nowMs);
    void recordNoteOn (int track, int note, int velocity, double stepPosition, unsigned nowMs);
    void recordTiesAtStep (int track, long long k);
    void holdToEraseCheck (int track, unsigned nowMs);

    // Lecture
    double stepTime (long long k, double stepLen) const;     // position du pas k, en temps, avec swing et groove
    void   playTrack (juce::MidiBuffer& midi, int track, double position, double blockLenBeats,
                      double samplesPerBeat, int numSamples, bool recordingTies);
    void   emitLoop (juce::MidiBuffer& midi, const juce::MidiMessage& m, int pos);
    void   addNoteOff (int note, int samplesFromNow);
    bool   extendNoteOff (int note, int samplesFromNow);
    void   flushNoteOffs (juce::MidiBuffer& midi, int numSamples);
    void   allNotesOff (juce::MidiBuffer& midi);

    // Sortie MIDI virtuelle (port CoreMIDI « JPR MIDI Looper », partagé par toutes les instances)
    void   updateMidiOutDevice();                             // fil message
    void   sendToVirtualOutput (int numSamples);              // fil audio
    std::shared_ptr<juce::MidiOutput> midiOutDevice;
    std::shared_ptr<juce::MidiOutput> midiOutToRelease;
    std::atomic<juce::MidiOutput*>    midiOutPtr { nullptr };
    juce::MidiBuffer                  tapBuffer;              // ce que l'instance envoie au port virtuel ce bloc
    juce::MidiBuffer*                 tap = nullptr;          // non nul quand la sortie virtuelle est active

    // Paramètres
    juce::AudioParameterChoice* trackParam    = nullptr;
    juce::AudioParameterInt*    lengthParam   = nullptr;
    juce::AudioParameterChoice* spbParam      = nullptr;
    juce::AudioParameterInt*    swingParam    = nullptr;
    juce::AudioParameterInt*    gateParam     = nullptr;
    juce::AudioParameterBool*   runningParam  = nullptr;
    juce::AudioParameterBool*   muteParam     = nullptr;
    juce::AudioParameterBool*   soloParam     = nullptr;
    juce::AudioParameterBool*   armedParam    = nullptr;
    juce::AudioParameterChoice* recModeParam  = nullptr;
    juce::AudioParameterChoice* logicModeParam = nullptr;
    juce::AudioParameterChoice* cmdTrigTypeParam = nullptr;
    juce::AudioParameterInt*    cmdTrigNumberParam = nullptr;
    juce::AudioParameterChoice* midiOutParam  = nullptr;
    juce::AudioParameterChoice* inputParam    = nullptr;
    juce::AudioParameterInt*    cmdBaseParam  = nullptr;
    juce::AudioParameterInt*    grooveAmountParam = nullptr;
    juce::AudioParameterInt*    grooveLengthParam = nullptr;
    juce::AudioParameterInt*    grooveStepParam   = nullptr;
    juce::AudioParameterInt*    grooveValueParam  = nullptr;
    juce::AudioParameterBool*   inputMuteParam    = nullptr;
    juce::AudioParameterChoice* drumParam         = nullptr;
    juce::AudioParameterInt*    transposeParam    = nullptr;
    juce::AudioParameterChoice* transTrigTypeParam = nullptr;
    juce::AudioParameterInt*    transTrigNumberParam = nullptr;
    juce::AudioParameterInt*    transBaseParam    = nullptr;

    // Valeurs vues au bloc précédent (détection de changement -> mémoire partagée)
    int lastLength = -1, lastSpb = -1, lastSwing = -1, lastGate = -1, lastLogicMode = -1, lastMidiOut = -1, lastRecMode = -1;
    int lastTrigType = -1, lastTrigNumber = -1;
    int lastRunning = -1, lastMute = -1, lastSolo = -1, lastArmed = -1;
    int lastGrooveAmount = -1, lastGrooveLength = -1, lastGrooveStep = -1, lastGrooveValue = -1;
    int lastInputMute = -1;
    int lastTransTrigType = -1, lastTransTrigNumber = -1, lastTransBase = -1;

    // Transposition : la valeur -1 est valide, d'où des sentinelles hors plage.
    static constexpr int noTranspose = -1000;
    int              lastTranspose = noTranspose;
    std::atomic<int> transposeEcho { noTranspose };  // valeur que le timer vient de recopier dans le paramètre (à ne pas republier)
    int              curTranspose  = 0;              // transposition de cette piste pour ce bloc (0 pour une batterie)
    int uiLastGrooveStep = -1;

    // Chargement d'état : la génération augmente à chaque setStateInformation. Le fil audio republie
    // alors tous les réglages vers la mémoire partagée, et tant qu'il ne l'a pas fait le timer ne
    // recopie rien de la mémoire partagée vers les paramètres (sinon les valeurs restaurées se perdent).
    std::atomic<int> stateGeneration     { 1 };
    std::atomic<int> publishedGeneration { 0 };
    int              appliedGeneration   = 0;        // fil audio seulement

    unsigned         createdMs = 0;                  // instant de création de l'instance (délai d'installation)

    std::atomic<int> trackNumber { 1 };
    std::atomic<int> leaderFlag  { 0 };

    double    currentSampleRate = 44100.0;
    long long lastStepIndex     = -1;
    bool      wasAudible        = true;
    double    localPos          = 0.0;
    bool      localClockValid   = false;

    // Swing et groove tels que vus par cette instance pour ce bloc
    double    curSwingDelay = 0.0;       // en temps
    double    curGrooveScale = 0.0;      // facteur appliqué à la table (% -> temps)
    int       curGrooveLength = 16;

    struct PendingOff { int note; int samples; };
    static constexpr int maxPending = 128;
    PendingOff pending[maxPending];
    int        numPending = 0;

    // Touches du clavier tenues : instant d'appui (ms, 0 = relâchée) et pas de départ
    unsigned  heldSinceMs[128] {};
    long long heldStartStep[128] {};
    int       heldVelocity[128] {};
    bool      heldErased[128] {};
    int       heldStoredNote[128] {};    // note inscrite dans la boucle pour cette touche (touche moins transposition), -1 = aucune
    unsigned  lastRecordedMs = 0;

    juce::MidiBuffer outBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JPRLooperProcessor)
};
