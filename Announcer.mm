// Annonces vocales du looper.
//
// Voie normale : une demande d'annonce adressée à VoiceOver par une fenêtre de l'application.
// Voie de secours : quand l'application qui héberge le plug-in n'a aucune fenêtre (cas d'un plug-in
// chargé dans un processus à part, tant que sa fenêtre n'a jamais été ouverte), VoiceOver n'écoute pas
// ce processus. Le texte est alors prononcé par la synthèse vocale du système, à qui l'on demande
// d'utiliser la voix et le débit de VoiceOver.
#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>
#include "Announcer.h"

static AVSpeechSynthesizer* jprSynth    = nil;
static int                  jprLastPath = 0;      // 0 = aucune annonce, 1 = VoiceOver, 2 = synthèse vocale
static char                 jprDiagText[256] = { 0 };

static void jprAnnounceOnMainThread (NSString* text)
{
    // Une fenêtre pour porter l'annonce : la principale, sinon celle qui a le clavier, sinon la première.
    id target = [NSApp mainWindow];
    if (target == nil) target = [NSApp keyWindow];
    if (target == nil && [[NSApp windows] count] > 0) target = [[NSApp windows] firstObject];

    if (target == nil && [[NSWorkspace sharedWorkspace] isVoiceOverEnabled])
    {
        // Aucune fenêtre : VoiceOver n'entendrait pas l'annonce. Synthèse vocale du système.
        if (jprSynth == nil) jprSynth = [[AVSpeechSynthesizer alloc] init];
        [jprSynth stopSpeakingAtBoundary: AVSpeechBoundaryImmediate];

        AVSpeechUtterance* utterance = [AVSpeechUtterance speechUtteranceWithString: text];
        // Voix accordée à la langue du texte ; les réglages de VoiceOver passent devant quand il est actif.
        NSString* language = jprSystemIsFrench() ? @"fr-FR" : [AVSpeechSynthesisVoice currentLanguageCode];
        AVSpeechSynthesisVoice* voice = (language != nil) ? [AVSpeechSynthesisVoice voiceWithLanguage: language] : nil;
        if (voice != nil) utterance.voice = voice;
        utterance.prefersAssistiveTechnologySettings = YES;
        [jprSynth speakUtterance: utterance];
        jprLastPath = 2;
        return;
    }

    NSDictionary* info = @{ NSAccessibilityAnnouncementKey : text,
                            NSAccessibilityPriorityKey     : @(NSAccessibilityPriorityHigh) };

    if (target != nil)
        NSAccessibilityPostNotificationWithUserInfo (target, NSAccessibilityAnnouncementRequestedNotification, info);

    // Et aussi à l'application, au cas où la fenêtre visée ne relaie pas.
    NSAccessibilityPostNotificationWithUserInfo (NSApp, NSAccessibilityAnnouncementRequestedNotification, info);
    jprLastPath = 1;
}

void jprAnnounce (const char* utf8Text)
{
    if (utf8Text == nullptr || *utf8Text == 0) return;
    NSString* text = [NSString stringWithUTF8String: utf8Text];
    if (text == nil) return;

    if ([NSThread isMainThread])
        jprAnnounceOnMainThread (text);
    else
        dispatch_async (dispatch_get_main_queue(), ^{ jprAnnounceOnMainThread (text); });
}

bool jprSystemIsFrench()
{
    static int cached = -1;
    if (cached < 0)
    {
        NSString* language = [[NSLocale preferredLanguages] firstObject];
        cached = (language != nil && [[language lowercaseString] hasPrefix: @"fr"]) ? 1 : 0;
    }
    return cached == 1;
}

const char* jprAnnounceDiagnostic()
{
    const bool french = jprSystemIsFrench();
    NSString* process = [[NSProcessInfo processInfo] processName];
    if (process == nil) process = french ? @"inconnu" : @"unknown";

    NSString* path = nil;
    if (jprLastPath == 1)      path = @"VoiceOver";
    else if (jprLastPath == 2) path = french ? @"synthèse vocale" : @"speech synthesis";

    NSString* text = nil;
    if (french)
        text = (path != nil) ? [NSString stringWithFormat: @"processus %@, %d fenêtres, dernière annonce par %@", process, (int) [[NSApp windows] count], path]
                             : [NSString stringWithFormat: @"processus %@, %d fenêtres, aucune annonce pour l'instant", process, (int) [[NSApp windows] count]];
    else
        text = (path != nil) ? [NSString stringWithFormat: @"process %@, %d windows, last announcement by %@", process, (int) [[NSApp windows] count], path]
                             : [NSString stringWithFormat: @"process %@, %d windows, no announcement yet", process, (int) [[NSApp windows] count]];

    strlcpy (jprDiagText, [text UTF8String], sizeof (jprDiagText));
    return jprDiagText;
}
