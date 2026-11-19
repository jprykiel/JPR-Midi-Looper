# JPR MIDI Looper

JPR MIDI Looper is a free MIDI pattern sequencer for Logic Pro and MainStage. It records what you play into loops, on up to eight tracks, and is designed to be driven entirely from a MIDI keyboard, with every command spoken by VoiceOver. It is a step sequencer that works in real time.

By its nature, it does not record MIDI controllers such as pitch bend, modulation and sustain.

It is an Audio Unit MIDI effect.

A French version of this page is available: [LISEZMOI.md](LISEZMOI.md).

## Download

Download the file JPR-MIDI-Looper.zip from the [latest release page](https://github.com/jprykiel/JPR-Midi-Looper/releases/latest). The zip contains the plug-in ready to use, the installer and this guide.

This page uses headings. With VoiceOver, press VO, Command, H to move to the next heading, and VO, Command, Shift, H to move to the previous heading.

## What you need

- A Mac with macOS 12 or later. The plug-in is built for both Apple silicon and Intel Macs. It has been developed and tested on Apple silicon.
- Logic Pro or MainStage. In Logic Pro you need a version that offers the options named Record MIDI to Track Here and Internal MIDI In.
- A MIDI keyboard. A sustain-style pedal or a spare button that sends a MIDI controller is very useful, but a key of the keyboard can do the same job.

## Installation

The plug-in is not notarized by Apple, because its author does not have a paid Apple developer account. For that reason macOS will not load it if you simply copy it into place. The small installer included here copies the plug-in and tells macOS that you trust it. It does not ask for your password and it changes nothing else on your Mac.

1. Download JPR-MIDI-Looper.zip from the [latest release page](https://github.com/jprykiel/JPR-Midi-Looper/releases/latest) and unzip it. You should now have a folder named JPR-MIDI-Looper in your Downloads folder. Safari usually unzips it for you. Otherwise, select the zip file in the Finder and press Command, O.
2. Quit Logic Pro and MainStage if they are open.
3. Open the Terminal application: press Command, Space, type Terminal, then press Return.
4. Type or paste the following line, then press Return: `bash ~/Downloads/JPR-MIDI-Looper/install.sh`
5. Terminal answers that JPR MIDI Looper is installed. You can then quit Terminal.
6. Open Logic Pro or MainStage. The first launch may take a little longer while the new plug-in is checked.

If you moved the folder somewhere else, replace the part after the word bash with the location of the file install.sh.

The plug-in is installed for your user account only, in the folder Library, Audio, Plug-Ins, Components of your home folder. To remove it, delete the item named JPR MIDI Looper.component from that folder.

### Where to find it

The looper is a MIDI effect. On a software instrument channel strip, open the MIDI FX slot, then choose Audio Units, JPR, JPR MIDI Looper. It does not appear in the list of instruments.

If it is not in the menu in Logic Pro, open the Logic Pro menu, Settings, Plug-in Manager, find JPR MIDI Looper in the list and choose Reset and Rescan Selection.

### Language

The plug-in speaks English, or French when Logic Pro or MainStage runs in French. This applies to the window, the parameter names and the spoken announcements.

## How it works, in short

- You put one looper on each software instrument channel strip, and you give each one a different Track number, from 1 to 8.
- All the loopers share the same memory. The looper with the lowest track number is the leader. Its global settings apply to all the others: pattern length, steps per beat, swing, gate, groove, host mode, triggers and transposition. The other windows show these settings but cannot change them.
- Only one track is active at a time. The keyboard only plays the instrument of the active track.
- A track records only when it is armed inside the looper, when it is the active track, and when the looper is playing. Notes are snapped to the nearest step.
- The loops run on the clock of Logic Pro or MainStage. They play when the host plays, at its tempo, and stop with it.
- Each looper saves its own loop with the project or the concert.

## Good to know

- A pattern can have up to 64 steps, with 1 to 8 steps per beat.
- Each step of each track can hold up to eight notes.
- Until a looper window has been opened once after loading a project, the spoken announcements may come from the system speech voice, using your VoiceOver voice settings, instead of from VoiceOver itself. After that, VoiceOver speaks them.
- The Virtual MIDI output setting creates a MIDI port named JPR MIDI Looper that carries the loops, one MIDI channel per track. It is off by default and is not needed for the Logic Pro setup described below.

## Why two different modes?

This plug-in has two operating modes, one for MainStage and one for Logic Pro. We will come back to them later, but it is important to understand that in MainStage the notion of a track does not exist; the looper therefore records the loops and plays them back until the transport is stopped.

Logic Pro mode, on the other hand, is designed to record each track of the loop, including every change made to it while it is running; when the transport stops, the looper is switched off and the notes are recorded on each track as if you had played them yourself. The looper is therefore no longer needed at that point, unless you want to record a new take or add something to the previous take.

## The plug-in window

The window has four sections, each with a heading: Track, Global settings, Actions and Status. Every control has a name and a help text that VoiceOver reads.

The Action menu runs the chosen action one and a half seconds after you select it. This gives you time to move through the list without triggering the items you pass.

The available actions are:

- Make this track active.
- Clear this track.
- Clear all.
- Undo.
- Toggle command mode.

The Status text describes the situation in one sentence: this track, the leader, the active track, command mode, the tracks present, the steps used, the transport and the tempo.

### The Controls view, not to be confused with command mode

You probably already know that there are two ways to display a plug-in in MainStage or Logic Pro. Because this looper was designed to be accessible, I strongly recommend choosing the Editor view, which in this case is named JPR MIDI Looper: its layout is clearer and the parameters are arranged in a more logical order.

If you still want to use the Controls view, here are a few details about it.

In the Controls view, settings with two choices are shown as Off and On. Their names are written so that this stays clear. Logic Pro mode: Off means MainStage. Command mode triggered by a keyboard note: Off means a MIDI controller. Keyboard always passed to the instrument: Off means the active track only. Drum track, not transposed: Off means a normal track. Transpose mode triggered by a keyboard note: Off means a MIDI controller.

## Command mode

This is one of the original features of this looper: it lets you operate it without touching your computer, a big plus for spontaneous use.

I invite you to get familiar with this mode before going any further; it will save you a lot of time.

Command mode turns the keys of your keyboard into commands. It is on for as long as you hold the trigger, and off as soon as you release it. While it is on, the keyboard does not play the instruments, and VoiceOver announces each command.

### The trigger

- The trigger is chosen in the leader's window, with two settings: Command trigger, which lets you choose between MIDI controller and Keyboard note, and Trigger number, from 0 to 127.
- The default is MIDI controller 65. If that controller already does something else in your Logic Pro setup, choose another one.
- If you choose a keyboard note, that key never plays the instrument and is never recorded. Pick a key you do not use, for example the lowest key of the keyboard, which is note 21 on an 88-key keyboard. Do not pick a key inside the two octaves of commands.
- The setting is saved with the project or the concert.

### The command keys

The commands use two octaves, starting from the Command base note. The default base note is 48, which Logic Pro calls C2. You can change it in the leader's window.

First octave, the actions octave, starting on the base note:

- C: play or stop the looper.
- D: mute or unmute all tracks.
- E: clear all tracks. Press twice to confirm.
- F: next recording mode.
- G: mute or unmute the keyboard input.

Second octave, the tracks octave, twelve semitones above the base note:

- The eight white keys, from C up to the next C: make track 1 to 8 the active track.
- C sharp: mute or unmute the active track.
- D sharp: solo the active track, or switch solo off.
- F sharp: arm or disarm the active track.
- G sharp: clear the active track. Press twice to confirm.
- A sharp: undo.

## Recording modes

The recording mode is global. Change it in the leader's window, or with F in the actions octave.

- Normal, replay to remove, tied notes: playing a note again on a step where it already exists removes it. A key held beyond one step creates a tied note, which is held on playback without a new attack.
- Replay to remove, no tied notes: the same, but a held key never creates a tie.
- Hold to erase: holding a key for half a second erases that note from the whole track.

## Keyboard mute

G in the actions octave, or the Keyboard muted checkbox, cuts the keyboard for all tracks. Nothing is passed to the instruments or recorded into the loops, which keep playing. Commands stay active. The keyboard is never muted when a project or concert is loaded.

## Real-time transposition

- The loops can be transposed while they play, all together, except the tracks whose Track type is Drums.
- Transpose mode works like command mode, with its own trigger, which you hold. The default is MIDI controller 66. It must be different from the command mode trigger.
- While the transpose trigger is held, the key you press sets the transposition and does not play the instrument. The Transpose base note, 60 by default, which Logic Pro calls C3, gives a transposition of zero. Each semitone above or below it transposes by that amount, from minus 24 to plus 24.
- VoiceOver says Transpose mode, then for example Transpose plus 2.
- The change applies to the next note of each loop. What you play on the keyboard is never transposed, and notes you record while a transposition is active are stored as you played them.

## Swing and groove

- Swing delays every second step. 0 is straight, 100 is a triplet feel.
- The groove table is the second great original feature of this looper, and I am particularly proud of it. Even though this is still a step sequencer, the groove brings the rhythm to life by shifting some steps against the others in a fully controlled way. The table holds one offset per step, from minus 50 to plus 50 percent of a step. A negative value is early. Choose the step with Groove step, then set its value with Step offset.
- Groove length is the number of steps in the groove cycle, from 1 to 32. The cycle restarts at the beginning of the pattern.
- Groove amount scales the whole table, from 0 to 200 percent.
- The groove table is saved with the project or the concert.

## Undo

There are eight levels of undo. A snapshot is taken before each clear, before each note removal, and at the start of each recording take, which means after two seconds without playing. Use A sharp in the tracks octave, or Undo in the Action menu.

## Setting up Logic Pro

The setup in Logic Pro needs a little care, because Logic normally records what you play on the keyboard, not what a MIDI effect produces. Follow these steps once, then save the project as a template.

### Build the tracks

1. Create one software instrument track for each loop you want, up to eight, and choose an instrument on each track.
2. On each of these tracks, insert JPR MIDI Looper in the MIDI FX slot. Each track holds both its instrument and its looper.
3. Open each looper window and set Track to a different number: 1 on the first track, 2 on the second, and so on.
4. On drum or percussion tracks, set Track type to Drums, so that they are not transposed. See the section Real-time transposition above.

### Set the leader

Open the window of the looper whose Track is 1. It is the leader. In its Global settings section:

1. Set Host mode to Logic Pro.
2. Set the pattern Length and the Steps per beat. For example, 16 steps with 4 steps per beat is one bar of sixteenth notes in four four.
3. Choose the command mode trigger. See the section Command mode above.

### Three Logic Pro settings that are required on every track

1. **Turn on Record MIDI to Track Here on every looper track.** This option is not easy to find with VoiceOver. On the channel strip, go to the MIDI FX slot that holds JPR MIDI Looper and open its pop-up menu, exactly as if you wanted to replace the looper with another plug-in. The option Record MIDI to Track Here is at the very bottom of that list. Select it so that it is checked. The looper stays in place.
2. **Set Internal MIDI In on every looper track to the track itself.** In the Track inspector, open the Internal MIDI In pop-up menu, then the MIDI to Track submenu, and choose the track you are on. It only appears in this submenu once the previous setting is turned on. Be careful not to confuse the track number in Logic Pro with the looper's Track number: in this submenu, each track is listed by its Logic Pro track number followed by its name. For example, if track 1 in Logic Pro is the metronome and your looper tracks, named looper 1, looper 2 and so on, start at track 2, you will choose 2 looper 1 on track 2, 3 looper 2 on track 3, and so on. Together with the previous setting, this makes the track record the notes coming out of the looper, as well as the notes you play.
3. **Record-enable all the looper tracks at the same time.** Every looper track must be armed for recording in Logic Pro, all together, not only the selected one. This lets every looper receive the keyboard, including the commands. You will still hear only the active track, because each looper passes the keyboard to its instrument only when its track is active.

If one of these three settings is missing on a track, that track will not follow the commands or will not record its loop.

### Recording a take

1. Start recording in Logic Pro. The loops only run while Logic is playing or recording.
2. Hold the command mode trigger and press the low C of the commands to start looper playback. VoiceOver says Play.
3. Still in command mode, choose the active track with a white key of the tracks octave, then press F sharp to arm it. VoiceOver says, for example, Track 1 armed.
4. Release the trigger and play. What you play is added to the loop of the active track. Change the active track to build the other loops. Remember to arm each track you want to record into.
5. Stop Logic Pro. In Logic Pro mode, the looper stops playing and all its tracks are disarmed. VoiceOver says Looper stopped, tracks disarmed.

The regions you have just recorded contain the output of the loops, with every change you made live. Because the looper is now stopped, they play back on their own when you press Play, without the loops on top.

Before a new take, switch looper playback and arming back on by hand, as in steps 2 and 3. You can do this while Logic is stopped: what you switch on while stopped stays on when Logic starts again.

## Setting up MainStage

1. In a patch, create one software instrument channel strip for each loop, and insert JPR MIDI Looper in the MIDI FX slot of each one. Put everything in the patch, nothing at the concert level.
2. Give each looper a different Track number.
3. In the leader, leave Host mode on MainStage.
4. Press Play in MainStage. The loops run while MainStage plays, at the tempo of the patch or the concert.

A track is never armed automatically. Arm it with F sharp in command mode, or with the checkbox in its window.

Important: the loops live in the memory of the application. Save the concert to keep them. Quitting MainStage without saving loses the loops recorded since the last save.

JPR MIDI Looper is shared free of charge, as it is, without any warranty.

## Source code and license

The source code of the plug-in is in this repository. It is built with the JUCE framework and CMake. You do not need it to use the plug-in: the zip file contains the plug-in already built.

JPR MIDI Looper is free software, distributed under the GNU Affero General Public License version 3. See the file LICENSE.
