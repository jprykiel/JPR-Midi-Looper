#pragma once

// Fait prononcer un texte : par VoiceOver quand l'application offre une fenêtre pour porter l'annonce,
// sinon par la synthèse vocale du système (avec les réglages de voix de VoiceOver).
// Peut être appelé depuis n'importe quel fil.
void jprAnnounce (const char* utf8Text);

// Courte description pour la zone État : processus qui héberge le plug-in et voie utilisée par la dernière annonce.
const char* jprAnnounceDiagnostic();

// Vrai si l'application hôte tourne en français : le looper parle alors français, sinon anglais.
bool jprSystemIsFrench();
