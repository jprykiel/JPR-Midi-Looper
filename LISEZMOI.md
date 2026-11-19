# JPR MIDI Looper

JPR MIDI Looper est un séquenceur de patterns MIDI gratuit pour Logic Pro et MainStage. Il enregistre ce que vous jouez dans des boucles, sur huit pistes au plus, et il est conçu pour être piloté entièrement depuis un clavier MIDI, chaque commande étant annoncée par VoiceOver. C'est un séquenceur pas à pas, qui fonctionne en temps réel.

De par sa nature, il n'enregistre pas les contrôleurs MIDI, comme le pitch bend, la modulation et le sustain.

C'est un effet MIDI au format Audio Unit.

An English version of this page is available: [README.md](README.md).

## Téléchargement

Téléchargez le fichier JPR-MIDI-Looper.zip depuis la [page de la dernière version](https://github.com/jprykiel/JPR-Midi-Looper/releases/latest). Le zip contient le plug-in prêt à l'emploi, l'installateur et ce guide.

Cette page utilise des titres. Avec VoiceOver, appuyez sur VO, Commande, H pour passer au titre suivant, et sur VO, Commande, Majuscule, H pour passer au titre précédent.

## Ce qu'il vous faut

- Un Mac avec macOS 12 ou plus récent. Le plug-in est compilé pour les Mac Apple silicon et Intel. Il a été développé et testé sur Apple silicon.
- Logic Pro ou MainStage. Dans Logic Pro, il faut une version qui propose les options « Enregistrer les données MIDI sur la piste ici » et « Entrée MIDI interne ».
- Un clavier MIDI. Une pédale de type sustain ou un bouton libre qui envoie un contrôleur MIDI est très utile, mais une touche du clavier peut jouer le même rôle.

## Installation

Le plug-in n'est pas certifié par Apple, car son auteur n'a pas de compte développeur Apple payant. Pour cette raison, macOS refuse de le charger si vous le copiez simplement à sa place. Le petit installateur fourni copie le plug-in et indique à macOS que vous lui faites confiance. Il ne demande pas votre mot de passe et ne change rien d'autre sur votre Mac.

1. Téléchargez JPR-MIDI-Looper.zip depuis la [page de la dernière version](https://github.com/jprykiel/JPR-Midi-Looper/releases/latest) et décompressez-le. Vous devez obtenir un dossier nommé JPR-MIDI-Looper dans votre dossier Téléchargements. Safari le décompresse en général pour vous. Sinon, sélectionnez le fichier zip dans le Finder et appuyez sur Commande, O.
2. Quittez Logic Pro et MainStage s'ils sont ouverts.
3. Ouvrez l'application Terminal : appuyez sur Commande, Espace, tapez Terminal, puis appuyez sur Retour.
4. Tapez ou collez la ligne suivante, puis appuyez sur Retour : `bash ~/Downloads/JPR-MIDI-Looper/install.sh`
5. Terminal répond, en anglais, « JPR MIDI Looper is installed ». Vous pouvez alors quitter Terminal.
6. Ouvrez Logic Pro ou MainStage. Le premier lancement peut être un peu plus long, le temps que le nouveau plug-in soit vérifié.

Si vous avez déplacé le dossier ailleurs, remplacez ce qui suit le mot bash par l'emplacement du fichier install.sh.

Le plug-in est installé pour votre compte seulement, dans le dossier Bibliothèque, Audio, Plug-Ins, Components de votre dossier personnel. Pour le retirer, supprimez de ce dossier l'élément nommé JPR MIDI Looper.component.

### Où le trouver

Le looper est un effet MIDI. Sur une tranche d'instrument logiciel, ouvrez le slot Effets MIDI, puis choisissez Audio Units, JPR, JPR MIDI Looper. Il n'apparaît pas dans la liste des instruments.

S'il n'est pas dans le menu de Logic Pro, ouvrez le menu Logic Pro, Réglages, Gestionnaire de modules, trouvez JPR MIDI Looper dans la liste et demandez une nouvelle analyse de la sélection.

### Langue

Le plug-in parle français quand Logic Pro ou MainStage tourne en français, anglais sinon. Cela concerne la fenêtre, les noms des réglages et les annonces vocales.

## Le principe, en bref

- Vous placez un looper sur chaque tranche d'instrument logiciel, et vous donnez à chacun un numéro de Piste différent, de 1 à 8.
- Tous les loopers partagent la même mémoire. Celui qui a le plus petit numéro de piste est le chef. Ses réglages globaux s'imposent à tous les autres : longueur du pattern, pas par temps, swing, gate, groove, mode de l'hôte, déclencheurs et transposition. Les autres fenêtres affichent ces réglages mais ne peuvent pas les changer.
- Une seule piste est active à la fois. Le clavier ne joue que l'instrument de la piste active.
- Une piste n'enregistre que si elle est armée dans le looper, si elle est la piste active et si le looper est en lecture. Les notes sont calées sur le pas le plus proche.
- Les boucles suivent l'horloge de Logic Pro ou de MainStage. Elles jouent quand l'hôte joue, à son tempo, et s'arrêtent avec lui.
- Chaque looper sauvegarde sa boucle avec le projet ou le concert.

## Bon à savoir

- Un pattern peut compter jusqu'à 64 pas, avec 1 à 8 pas par temps.
- Chaque pas de chaque piste peut contenir jusqu'à huit notes.
- Tant qu'aucune fenêtre de looper n'a été ouverte après le chargement d'un projet, les annonces peuvent être dites par la voix de synthèse du système, avec vos réglages de voix VoiceOver, au lieu de VoiceOver lui-même. Ensuite, c'est VoiceOver qui les dit.
- Le réglage Sortie MIDI virtuelle crée un port MIDI nommé JPR MIDI Looper qui transporte les boucles, un canal MIDI par piste. Il est désactivé par défaut et n'est pas nécessaire pour la configuration de Logic Pro décrite plus bas.

## Pourquoi deux modes différents ?

Ce plug-in dispose de deux modes de fonctionnement, l'un pour MainStage, l'autre pour Logic Pro. Nous y reviendrons plus tard, mais il est important de comprendre que dans MainStage la notion de piste n'existe pas ; le looper va donc enregistrer les boucles, et les rejouer jusqu'à ce que le transport soit arrêté.

Le mode Logic Pro, par contre, est conçu pour enregistrer chaque piste de la boucle, y compris toutes les modifications qui lui sont apportées pendant qu'elle tourne ; à l'arrêt du transport, le looper est désactivé et les notes sont enregistrées sur chaque piste comme si vous les aviez jouées vous-même. Le looper devient donc inutile à ce stade, sauf si vous voulez faire une nouvelle prise ou ajouter quelque chose à la prise précédente.

## La fenêtre du plug-in

La fenêtre a quatre sections, chacune avec un titre : Piste, Réglages globaux, Actions et État. Chaque réglage a un nom et un texte d'aide que VoiceOver lit.

Le menu Action exécute l'action choisie une seconde et demie après la sélection. Cela laisse le temps de parcourir la liste sans déclencher les éléments traversés.

Les actions disponibles sont :

- Rendre cette piste active.
- Effacer cette piste.
- Tout effacer.
- Annuler.
- Basculer le mode commande.

Le texte État décrit la situation en une phrase : cette piste, le chef, la piste active, le mode commande, les pistes présentes, les pas utilisés, le transport et le tempo.

### La vue Commandes, à ne pas confondre avec le mode commande

Vous savez sûrement déjà qu'il existe deux manières d'afficher un plug-in dans MainStage ou Logic Pro. Ce looper ayant été conçu pour être accessible, je vous conseille vivement de choisir la vue Éditeur, qui, dans ce cas, s'appelle JPR MIDI Looper : la présentation y est plus claire et les paramètres y sont rangés d'une manière plus logique.

Si, malgré tout, vous voulez utiliser la vue Commandes, voici quelques détails la concernant.

Dans la vue Commandes, les réglages à deux choix s'affichent Off et On. Leurs noms sont écrits pour que cela reste clair. Mode Logic Pro : Off veut dire MainStage. Mode commande déclenché par une note du clavier : Off veut dire un contrôleur MIDI. Clavier toujours transmis à l'instrument : Off veut dire la piste active seulement. Piste de batterie, non transposée : Off veut dire une piste normale. Mode transposition déclenché par une note du clavier : Off veut dire un contrôleur MIDI.

## Mode commande

C'est une des originalités de ce looper : il permet de le manipuler sans toucher votre ordinateur, un grand plus pour une utilisation spontanée.

Je vous invite à vous familiariser avec ce mode avant d'aller plus loin, cela vous fera gagner beaucoup de temps.

Le mode commande transforme les touches de votre clavier en commandes. Il est actif tant que vous maintenez le déclencheur, et s'arrête dès que vous le relâchez. Pendant ce temps, le clavier ne joue pas les instruments, et VoiceOver annonce chaque commande.

### Le déclencheur

- Le déclencheur se choisit dans la fenêtre du chef, avec deux réglages : Déclencheur commande, qui permet de choisir entre Contrôleur MIDI et Note du clavier, et Numéro du déclencheur, de 0 à 127.
- Par défaut, c'est le contrôleur MIDI 65. Si ce contrôleur sert déjà à autre chose dans votre configuration de Logic Pro, choisissez-en un autre.
- Si vous choisissez une note du clavier, cette touche ne joue jamais l'instrument et n'est jamais enregistrée. Prenez une touche inutilisée, par exemple la plus grave du clavier, la note 21 sur un clavier de 88 touches. Ne prenez pas une touche située dans les deux octaves de commandes.
- Le réglage est sauvegardé avec le projet ou le concert.

### Les touches de commande

Les commandes occupent deux octaves, à partir de la note de base des commandes. Par défaut, cette note est 48, que Logic Pro appelle Do2. Vous pouvez la changer dans la fenêtre du chef.

Première octave, l'octave des actions, à partir de la note de base :

- Do : lecture ou arrêt du looper.
- Ré : rendre toutes les pistes muettes, ou les rétablir.
- Mi : tout effacer. Appuyer deux fois pour confirmer.
- Fa : mode d'enregistrement suivant.
- Sol : rendre le clavier muet, ou le rétablir.

Deuxième octave, l'octave des pistes, douze demi-tons au-dessus de la note de base :

- Les huit touches blanches, de Do au Do suivant : rendre active la piste 1 à 8.
- Do dièse : rendre muette la piste active, ou la rétablir.
- Ré dièse : mettre la piste active en solo, ou enlever le solo.
- Fa dièse : armer ou désarmer la piste active.
- Sol dièse : effacer la piste active. Appuyer deux fois pour confirmer.
- La dièse : annuler.

## Modes d'enregistrement

Le mode d'enregistrement est global. Changez-le dans la fenêtre du chef, ou avec le Fa de l'octave des actions.

- Normal, rejouer pour enlever, notes liées : rejouer une note sur un pas où elle existe déjà la supprime. Une touche tenue au-delà d'un pas crée une note liée, tenue à la lecture sans nouvelle attaque.
- Rejouer pour enlever, sans notes liées : pareil, mais une touche tenue ne crée jamais de liaison.
- Maintenir pour effacer : tenir une touche une demi-seconde efface cette note de toute la piste.

## Clavier muet

Le Sol de l'octave des actions, ou la case Clavier muet, coupe le clavier pour toutes les pistes. Plus rien n'est transmis aux instruments ni enregistré dans les boucles, qui continuent de jouer. Les commandes restent actives. Le clavier n'est jamais muet au chargement d'un projet ou d'un concert.

## Transposition en temps réel

- Les boucles peuvent être transposées pendant qu'elles jouent, toutes ensemble, sauf les pistes dont le Type de piste est Batterie.
- Le mode transposition fonctionne comme le mode commande, avec son propre déclencheur, que l'on maintient. Par défaut, c'est le contrôleur MIDI 66. Il doit être différent du déclencheur du mode commande.
- Tant que le déclencheur de transposition est maintenu, la touche jouée fixe la transposition et ne joue pas l'instrument. La note de base de la transposition, 60 par défaut, que Logic Pro appelle Do3, donne une transposition de zéro. Chaque demi-ton au-dessus ou en dessous transpose d'autant, de moins 24 à plus 24.
- VoiceOver dit « Mode transposition », puis par exemple « Transposition plus 2 ».
- Le changement s'applique à la prochaine note de chaque boucle. Ce que vous jouez au clavier n'est jamais transposé, et les notes enregistrées pendant une transposition sont retenues telles que vous les avez jouées.

## Swing et groove

- Le swing retarde un pas sur deux. 0 est droit, 100 est ternaire.
- La table de groove est la deuxième grande originalité de ce looper, j'en suis particulièrement fier. Malgré le fait qu'il s'agit toujours d'un séquenceur pas à pas, le groove permet de donner vie au rythme en décalant certains pas par rapport aux autres d'une manière totalement contrôlée. Elle contient un décalage par pas, de moins 50 à plus 50 pour cent du pas. Une valeur négative est en avance. Choisissez le pas avec Pas de groove, puis réglez sa valeur avec Décalage du pas.
- La Longueur du groove est le nombre de pas du cycle de groove, de 1 à 32. Le cycle repart au début du pattern.
- L'Intensité du groove s'applique à toute la table, de 0 à 200 pour cent.
- La table de groove est sauvegardée avec le projet ou le concert.

## Annulation

Il y a huit niveaux d'annulation. Un instantané est pris avant chaque effacement, avant chaque suppression de note et au début de chaque prise d'enregistrement, c'est-à-dire après deux secondes sans jouer. Utilisez le La dièse de l'octave des pistes, ou Annuler dans le menu Action.

## Configurer Logic Pro

La configuration dans Logic Pro demande un peu de soin, car Logic enregistre normalement ce que vous jouez au clavier, et non ce que produit un effet MIDI. Suivez ces étapes une fois, puis enregistrez le projet comme modèle.

### Créer les pistes

1. Créez une piste d'instrument logiciel pour chaque boucle voulue, huit au plus, et choisissez un instrument sur chaque piste.
2. Sur chacune de ces pistes, insérez JPR MIDI Looper dans le slot Effets MIDI. Chaque piste contient à la fois son instrument et son looper.
3. Ouvrez la fenêtre de chaque looper et réglez Piste sur un numéro différent : 1 sur la première piste, 2 sur la deuxième, et ainsi de suite.
4. Sur les pistes de batterie ou de percussions, réglez Type de piste sur Batterie, pour qu'elles ne soient pas transposées. Voir plus haut la section Transposition en temps réel.

### Régler le chef

Ouvrez la fenêtre du looper dont la Piste est 1. C'est le chef. Dans sa section Réglages globaux :

1. Réglez Mode de l'hôte sur Logic Pro.
2. Réglez la Longueur du pattern et les Pas par temps. Par exemple, 16 pas avec 4 pas par temps donnent une mesure de doubles croches à quatre temps.
3. Choisissez le déclencheur du mode commande. Voir plus haut la section Mode commande.

### Trois réglages de Logic Pro obligatoires sur chaque piste

1. **Activer « Enregistrer les données MIDI sur la piste ici » sur chaque piste de looper.** Cette option n'est pas facile à trouver avec VoiceOver. Sur la tranche, allez sur le slot Effets MIDI qui contient JPR MIDI Looper et ouvrez son menu, exactement comme si vous vouliez remplacer le looper par un autre plug-in. L'option se trouve tout en bas de cette liste. Sélectionnez-la pour qu'elle soit cochée. Le looper reste en place.
2. **Régler l'entrée MIDI interne de chaque piste de looper sur la piste elle-même.** Dans l'inspecteur de piste, ouvrez le menu Entrée MIDI interne, puis le sous-menu MIDI sur piste, et choisissez la piste sur laquelle vous êtes. Elle n'apparaît dans ce sous-menu qu'une fois le réglage précédent activé. Attention à ne pas confondre le numéro de la piste dans Logic Pro et le numéro de Piste du looper : dans ce sous-menu, chaque piste est désignée par son numéro dans Logic Pro, suivi de son nom. Par exemple, si la piste 1 de Logic Pro est le métronome et que vos pistes de looper, nommées looper 1, looper 2 et ainsi de suite, commencent à la piste 2, vous choisirez « 2 looper 1 » sur la piste 2, « 3 looper 2 » sur la piste 3, etc. Combiné au réglage précédent, ce réglage fait enregistrer à la piste les notes qui sortent du looper, ainsi que les notes que vous jouez.
3. **Armer toutes les pistes de looper en même temps.** Toutes les pistes de looper doivent être armées pour l'enregistrement dans Logic Pro, toutes ensemble, et pas seulement la piste sélectionnée. Chaque looper reçoit ainsi le clavier, commandes comprises. Vous n'entendrez quand même que la piste active, car chaque looper ne transmet le clavier à son instrument que lorsque sa piste est active.

Si l'un de ces trois réglages manque sur une piste, cette piste ne suivra pas les commandes ou n'enregistrera pas sa boucle.

### Enregistrer une prise

1. Lancez l'enregistrement dans Logic Pro. Les boucles ne tournent que pendant que Logic joue ou enregistre.
2. Maintenez le déclencheur du mode commande et appuyez sur le Do grave des commandes pour lancer la lecture du looper. VoiceOver dit « Lecture ».
3. Toujours en mode commande, choisissez la piste active avec une touche blanche de l'octave des pistes, puis appuyez sur Fa dièse pour l'armer. VoiceOver dit par exemple « Piste 1 armée ».
4. Relâchez le déclencheur et jouez. Ce que vous jouez s'ajoute à la boucle de la piste active. Changez de piste active pour construire les autres boucles. Pensez à armer chaque piste dans laquelle vous voulez enregistrer.
5. Arrêtez Logic Pro. En mode Logic Pro, le looper cesse de jouer et toutes ses pistes sont désarmées. VoiceOver dit « Looper arrêté, pistes désarmées ».

Les régions que vous venez d'enregistrer contiennent la sortie des boucles, avec toutes les modifications faites en direct. Comme le looper est maintenant arrêté, elles se rejouent seules quand vous lancez la lecture, sans les boucles par-dessus.

Avant une nouvelle prise, réactivez à la main la lecture du looper et l'armement, comme aux étapes 2 et 3. Vous pouvez le faire pendant que Logic est arrêté : ce qui est réactivé à l'arrêt reste actif au redémarrage.

## Configurer MainStage

1. Dans un patch, créez une tranche d'instrument logiciel pour chaque boucle et insérez JPR MIDI Looper dans le slot Effets MIDI de chacune. Tout se place dans le patch, rien au niveau du concert.
2. Donnez à chaque looper un numéro de Piste différent.
3. Dans le chef, laissez Mode de l'hôte sur MainStage.
4. Lancez la lecture dans MainStage. Les boucles tournent pendant que MainStage joue, au tempo du patch ou du concert.

Une piste n'est jamais armée automatiquement. Armez-la avec Fa dièse en mode commande, ou avec la case à cocher de sa fenêtre.

Important : les boucles vivent dans la mémoire de l'application. Enregistrez le concert pour les garder. Quitter MainStage sans enregistrer fait perdre les boucles enregistrées depuis la dernière sauvegarde.

JPR MIDI Looper est partagé gratuitement, tel quel, sans aucune garantie.

## Code source et licence

Le code source du plug-in se trouve dans ce dépôt. Il est construit avec le framework JUCE et CMake. Vous n'en avez pas besoin pour utiliser le plug-in : le fichier zip contient le plug-in déjà compilé.

JPR MIDI Looper est un logiciel libre, distribué sous la licence GNU Affero General Public License version 3. Voir le fichier LICENSE.
