#pragma once
#include <atomic>

// Mémoire partagée entre toutes les instances du plug-in chargées dans le même
// processus hôte (Logic ou MainStage). Équivalent du "gmem" de REAPER.
//
// Il n'y a pas de hub : chaque instance se déclare vivante à chaque bloc audio,
// et celle dont le numéro de piste est le plus petit parmi les vivantes est le
// chef. Le chef publie les réglages globaux et fait avancer l'horloge libre.
struct SharedState
{
    static constexpr int maxTracks   = 8;
    static constexpr int maxSteps    = 64;
    static constexpr int maxPoly     = 8;     // notes simultanées par pas
    static constexpr int grooveSteps = 32;    // longueur maximale du cycle de groove
    static constexpr int undoLevels  = 8;

    struct Note
    {
        std::atomic<int> note     { -1 };   // -1 = vide
        std::atomic<int> velocity { 0 };
        std::atomic<int> skipOnce { 0 };    // 1 = ne pas rejouer ce pas la première fois (anti double déclenchement)
        std::atomic<int> tie      { 0 };    // 1 = prolonge la même note du pas précédent (note liée)
    };

    struct Track
    {
        Note notes[maxSteps][maxPoly];
    };

    Track tracks[maxTracks];

    // ---- Vie et élection ----
    std::atomic<long long> tick { 0 };
    std::atomic<long long> lastSeen[maxTracks] {};
    std::atomic<int>       leaderTrack { 0 };          // 1..8, 0 = aucun

    // ---- Horloge : une seule, celle de l'hôte, lue par le chef et publiée aux suiveurs ----
    std::atomic<double>    leaderPos { 0.0 };          // position en temps (noires) du prochain bloc, publiée par le chef
    std::atomic<int>       hostPlaying { 0 };          // 1 = le chef voit l'hôte en lecture (les boucles tournent)
    std::atomic<int>       hostRecording { 0 };        // 1 = le chef voit l'hôte en enregistrement
    std::atomic<double>    hostBpm { 120.0 };          // tempo lu par le chef

    // ---- Réglages globaux (publiés par le chef, modifiables par les commandes) ----
    std::atomic<int> running       { 1 };
    std::atomic<int> patternLength { 16 };
    std::atomic<int> stepsPerBeat  { 4 };
    std::atomic<int> swing         { 0 };              // 0..100 % (100 = ternaire)
    std::atomic<int> gate          { 50 };             // 10..100 %
    std::atomic<int> logicMode     { 0 };              // 0 = MainStage ; 1 = Logic Pro : à l'arrêt de l'hôte, lecture du looper coupée et pistes désarmées
    std::atomic<int> cmdTrigType   { 0 };              // déclencheur du mode commande : 0 contrôleur MIDI, 1 note du clavier
    std::atomic<int> cmdTrigNumber { 65 };             // numéro du contrôleur ou de la note
    std::atomic<int> midiOut       { 0 };              // sortie MIDI virtuelle : 0 aucune, 1 boucles, 2 boucles et clavier
    std::atomic<int> recMode       { 0 };              // 0 normal (liées + rejouer pour enlever), 1 rejouer sans liaisons, 2 maintenir pour effacer

    // ---- Transposition en temps réel (les pistes de batterie ne sont pas transposées) ----
    std::atomic<int> transpose       { 0 };            // demi-tons, -24..+24
    std::atomic<int> transTrigType   { 0 };            // déclencheur du mode transposition : 0 contrôleur MIDI, 1 note du clavier
    std::atomic<int> transTrigNumber { 66 };           // numéro du contrôleur ou de la note
    std::atomic<int> transBase       { 60 };           // touche qui correspond à une transposition de zéro

    // ---- Groove : décalage de chaque pas, en % du pas, cyclique ----
    std::atomic<int> grooveTable[grooveSteps] {};      // -50..+50
    std::atomic<int> grooveAmount  { 100 };            // 0..200 %
    std::atomic<int> grooveLength  { 16 };             // 1..32

    // ---- État des pistes ----
    std::atomic<int> activeTrack   { 1 };
    std::atomic<int> inputMute     { 0 };              // 1 = clavier muet : rien n'est transmis aux instruments ni enregistré
    std::atomic<int> mute[maxTracks]  {};
    std::atomic<int> solo[maxTracks]  {};
    std::atomic<int> armed[maxTracks] {};
    std::atomic<unsigned> armedAtMs[maxTracks] {};     // instant du dernier armement (désarmement automatique)

    // ---- Suivi du transport pour le désarmement automatique (tenu par le chef) ----
    std::atomic<int>      transportSeenPlaying { 0 };
    std::atomic<unsigned> lastHostPlayMs { 0 };
    std::atomic<unsigned> runningAtMs { 0 };           // instant de la dernière remise en lecture du looper

    // ---- Mode commande ----
    std::atomic<int>       commandMode      { 0 };
    std::atomic<unsigned>  cc65PressedAt    { 0 };
    std::atomic<unsigned>  clearPressedAt   { 0 };     // double appui pour effacer
    std::atomic<int>       clearPressedWhat { 0 };     // 1 = piste, 2 = tout

    // ---- Mode transposition (déclencheur maintenu : les touches du clavier choisissent la transposition) ----
    std::atomic<int>       transposeMode    { 0 };

    // ---- Dédoublonnage des commandes entre instances ----
    std::atomic<long long> lastNoteStamp[128] {};
    std::atomic<long long> lastCcStamp { -1 };
    std::atomic<long long> lastTransStamp { -1 };

    // ---- Annonces VoiceOver ----
    static constexpr int announceSize = 32;
    struct Announce { std::atomic<int> code { 0 }; std::atomic<int> arg { 0 }; };
    Announce announces[announceSize];
    std::atomic<int> announceWrite { 0 };
    std::atomic<int> announceRead  { 0 };

    void pushAnnounce (int code, int arg)
    {
        const int w = announceWrite.load();
        announces[w % announceSize].code.store (code);
        announces[w % announceSize].arg.store (arg);
        announceWrite.store (w + 1);
    }

    // ---- Annulation : instantanés de toutes les pistes ----
    struct Snapshot
    {
        int note[maxTracks][maxSteps][maxPoly];
        int velocity[maxTracks][maxSteps][maxPoly];
        int tie[maxTracks][maxSteps][maxPoly];
    };
    Snapshot        undo[undoLevels];
    std::atomic<int> undoTop   { 0 };   // prochain emplacement d'écriture
    std::atomic<int> undoCount { 0 };   // instantanés disponibles

    void takeSnapshot()
    {
        auto& s = undo[undoTop.load() % undoLevels];
        for (int t = 0; t < maxTracks; ++t)
            for (int k = 0; k < maxSteps; ++k)
                for (int v = 0; v < maxPoly; ++v)
                {
                    s.note[t][k][v]     = tracks[t].notes[k][v].note.load();
                    s.velocity[t][k][v] = tracks[t].notes[k][v].velocity.load();
                    s.tie[t][k][v]      = tracks[t].notes[k][v].tie.load();
                }
        undoTop.store (undoTop.load() + 1);
        if (undoCount.load() < undoLevels) undoCount.store (undoCount.load() + 1);
    }

    bool restoreSnapshot()
    {
        if (undoCount.load() <= 0) return false;
        undoTop.store (undoTop.load() - 1);
        undoCount.store (undoCount.load() - 1);
        auto& s = undo[((undoTop.load() % undoLevels) + undoLevels) % undoLevels];
        for (int t = 0; t < maxTracks; ++t)
            for (int k = 0; k < maxSteps; ++k)
                for (int v = 0; v < maxPoly; ++v)
                {
                    auto& n = tracks[t].notes[k][v];
                    n.velocity.store (s.velocity[t][k][v]);
                    n.tie.store (s.tie[t][k][v]);
                    n.skipOnce.store (0);
                    n.note.store (s.note[t][k][v]);
                }
        return true;
    }

    // ---- Édition ----
    void clearTrack (int t)
    {
        if (t < 0 || t >= maxTracks) return;
        for (auto& step : tracks[t].notes)
            for (auto& n : step)
            {
                n.note.store (-1);
                n.velocity.store (0);
                n.skipOnce.store (0);
                n.tie.store (0);
            }
    }

    // Supprime une note d'un pas, ainsi que ses liaisons sur les pas suivants.
    void removeNote (int t, int step, int note)
    {
        const int len = patternLength.load() > 0 ? patternLength.load() : 1;
        bool first = true;
        for (int i = 0; i < len; ++i)
        {
            const int k = (step + i) % len;
            bool found = false;
            for (auto& n : tracks[t].notes[k])
                if (n.note.load() == note && (first || n.tie.load()))
                {
                    n.note.store (-1); n.velocity.store (0); n.skipOnce.store (0); n.tie.store (0);
                    found = true;
                }
            if (! found && ! first) break;
            if (first && ! found) break;
            first = false;
        }
    }

    bool hasNote (int t, int step, int note) const
    {
        for (auto& n : tracks[t].notes[step])
            if (n.note.load() == note) return true;
        return false;
    }

    static SharedState& get();
};

// Codes d'annonce
enum AnnounceCode
{
    annNone = 0,
    annActiveTrack,      // arg = piste
    annMuteOn, annMuteOff,
    annSoloOn, annSoloOff,
    annArmOn, annArmOff,
    annTrackCleared,
    annAllCleared,
    annConfirmClear,     // arg = 1 piste, 2 tout
    annPlay, annStop,
    annAllMuteOn, annAllMuteOff,
    annCommandOn, annCommandOff,
    annUndone, annNothingToUndo,
    annRecMode,          // arg = mode 0..2
    annUnassigned,
    annAutoDisarm,
    annInputMuteOn, annInputMuteOff,
    annTransposeModeOn, annTransposeModeOff,
    annTranspose         // arg = demi-tons, négatif ou positif
};
