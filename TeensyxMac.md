# 🎸 Guide d'installation — Pédale Hexaphonique Multi-Effets

> **Version** : Août 2026
> **OS cible** : macOS (testé sur Apple Silicon)
> Guide "clé en main" pour installer, configurer et utiliser le projet complet depuis zéro.

---

## Table des matières

1. [Vue d'ensemble du projet](#1-vue-densemble-du-projet)
2. [Matériel nécessaire (Hardware)](#2-matériel-nécessaire-hardware)
3. [Signal path — Comment tout se branche](#3-signal-path--comment-tout-se-branche)
4. [Installation du firmware Teensy (TDM_TEENSY)](#4-installation-du-firmware-teensy-tdm_teensy)
5. [Installation de l'interface graphique (GUI)](#5-installation-de-linterface-graphique-gui)
6. [Premier lancement — Tout connecter](#6-premier-lancement--tout-connecter)
7. [Utilisation de base](#7-utilisation-de-base)
8. [Dépannage (Troubleshooting)](#8-dépannage-troubleshooting)
9. [Annexes](#9-annexes)

---

## 1. Vue d'ensemble du projet

Ce projet transforme un **Teensy 4.1** en pédalier d'effets **hexaphonique** : chaque corde de la guitare est traitée **individuellement** avec sa propre chaîne d'effets (distorsion sur les graves, delay sur les aigus, etc.).

Le système se compose de **deux briques logicielles** et d'un **PCB custom** :

| Composant | Rôle | Dépôt Git |
|---|---|---|
| **TDM_TEENSY** (firmware C++) | Traitement audio DSP temps réel sur Teensy 4.1 | [github.com/Desmatheo/TDM_TEENSY](https://github.com/Desmatheo/TDM_TEENSY) |
| **GUI** (application Python) | Interface graphique pour contrôler les effets via MIDI USB | [github.com/Desmatheo/GUI](https://github.com/Desmatheo/GUI) |
| **PCB Pédale Hexaphonique** | Carte électronique avec codec CS42448, entrées DIN13, sorties Jack | — (Hardware physique) |

### Architecture simplifiée

```
┌─────────────┐     DIN13 ou      ┌──────────────┐     USB      ┌──────────────┐
│   Guitare   │ ──────────────→   │  PCB + Teensy │ ←─────────→ │ Mac (GUI)    │
│ hexaphonique│   6× Jack mono    │  (firmware)   │    MIDI      │   Python     │
└─────────────┘                   └──────┬───────┘              └──────────────┘
                                         │
                                    Jack sortie
                                         │
                                         ▼
                                    ┌─────────┐
                                    │  Ampli   │
                                    └─────────┘
```

---

## 2. Matériel nécessaire (Hardware)

### Composants électroniques

| Composant | Référence | Remarques |
|---|---|---|
| **Teensy 4.1** | PJRC Teensy 4.1 | ARM Cortex-M7 @ 600 MHz |
| **Codec Audio** | Cirrus Logic CS42448 | 6 entrées ADC / 8 sorties DAC, bus TDM |
| **PSRAM** (recommandé) | PSRAM 8 Mo pour Teensy 4.1 | À souder sous la Teensy. Nécessaire pour les delays longs (jusqu'à 4 sec/corde) |
| **PCB Pédale Hexaphonique** | Carte custom | Contient le codec, les connecteurs DIN13 et Jack |
| **Câble USB** | USB Micro-B → USB-A/C | Pour connecter la Teensy au Mac |

### Côté guitare

Deux options pour envoyer le signal hexaphonique (6 cordes séparées) au PCB :

#### Option A — Guitare avec sortie DIN13 native
> Certaines guitares hexaphoniques (ex. **Godin xtSA**, **Godin Multiac**) possèdent directement une sortie **DIN 13 broches**. Un seul câble DIN13 suffit.

```
Guitare ──[câble DIN13]──→ Entrée DIN13 du PCB
```

#### Option B — Micro Submarine Pickups + Boîtier convertisseur
> Si la guitare est équipée d'un micro **hexaphonique à 6 sorties mono** (type "Submarine Pickups"), chaque corde sort sur un **jack mono 6.35mm** individuel. Ces 6 jacks se branchent sur un **boîtier convertisseur** qui regroupe les signaux en un connecteur DIN13.

```
Guitare
  ├── Jack mono corde 1 (Mi grave) ──┐
  ├── Jack mono corde 2 (La)    ─────┤
  ├── Jack mono corde 3 (Ré)    ─────┤──→ Boîtier convertisseur ──[DIN13]──→ PCB
  ├── Jack mono corde 4 (Sol)   ─────┤
  ├── Jack mono corde 5 (Si)    ─────┤
  └── Jack mono corde 6 (Mi aigu) ──┘
```

### Côté sortie (amplification)

Le PCB dispose de **sorties Jack 6.35mm** (stéréo ou 2× mono) pour brancher directement sur :
- Un **ampli guitare** (combo ou tête + baffle)
- Une **table de mixage**
- Un **moniteur de studio**

> [!IMPORTANT]
> Les détails exacts du branchement des connecteurs DIN13 et des sorties Jack sur le PCB dépendent de la version du PCB. Référez-vous au schéma électronique fourni avec la carte.

---

## 3. Signal path — Comment tout se branche

### Schéma complet du flux de signal

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SIGNAL PATH COMPLET                                │
│                                                                             │
│  ┌──────────┐                                                               │
│  │ GUITARE  │                                                               │
│  │ hexa-    │                                                               │
│  │ phonique │                                                               │
│  └────┬─────┘                                                               │
│       │                                                                     │
│       │  DIN13 (ou 6× Jack → boîtier → DIN13)                              │
│       ▼                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐            │
│  │                    PCB PÉDALE HEXAPHONIQUE                  │            │
│  │                                                             │            │
│  │  ┌─────────┐     ┌──────────┐     ┌──────────────────────┐ │            │
│  │  │Connecteur│     │  Codec   │     │     TEENSY 4.1       │ │            │
│  │  │ DIN13   │────→│ CS42448  │────→│                      │ │            │
│  │  │ Entrée  │     │ (ADC 6ch)│ TDM │  Préampli logiciel   │ │            │
│  │  └─────────┘     │          │     │  ┌──┐┌──┐┌──┐┌──┐   │ │            │
│  │                  │          │     │  │S1││S2││S3││S4│   │ │            │
│  │                  │          │     │  │  ││  ││  ││  │   │ │            │
│  │                  │          │     │  └──┘└──┘└──┘└──┘   │ │            │
│  │                  │          │     │  (× 6 cordes)        │ │            │
│  │                  │          │◄────│                      │ │            │
│  │  ┌─────────┐     │ (DAC 2ch)│ TDM │  Mixer → Master Out  │ │            │
│  │  │ Sortie  │◄────│          │     │                      │ │            │
│  │  │  Jack   │     └──────────┘     └──────────┬───────────┘ │            │
│  │  │ (ampli) │                                  │ USB         │            │
│  │  └─────────┘                                  │             │            │
│  └───────────────────────────────────────────────┼─────────────┘            │
│                                                  │                          │
│                                                  ▼                          │
│                                           ┌──────────────┐                  │
│                                           │   MAC (USB)  │                  │
│                                           │  ┌────────┐  │                  │
│                                           │  │  GUI   │  │                  │
│                                           │  │ Python │  │                  │
│                                           │  └────────┘  │                  │
│                                           └──────────────┘                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Détail des connexions Teensy ↔ Codec (pour info / debug)

| Signal | Pin Teensy 4.1 | Description |
|---|:---:|---|
| TDM Frame Sync (LRCLK) | `20` | Synchronisation des trames |
| TDM Bit Clock (BCLK) | `21` | Horloge bit |
| TDM Data In (DIN) | `8` | Audio entrant (guitare → Teensy) |
| TDM Data Out (DOUT) | `7` | Audio sortant (Teensy → ampli) |
| Master Clock (MCLK) | `23` | Horloge maître |
| I2C SDA | `18` | Configuration des registres du codec |
| I2C SCL | `19` | Configuration des registres du codec |
| Reset Codec | `2` | Reset matériel du CS42448 |
| LED intégrée | `13` | Indicateur d'état |

### Mapping des canaux TDM → Cordes

| Canal TDM | Corde | Note |
|:---:|:---:|:---:|
| 10 | 0 | Mi grave (E) |
| 8 | 1 | La (A) |
| 6 | 2 | Ré (D) |
| 4 | 3 | Sol (G) |
| 2 | 4 | Si (B) |
| 0 | 5 | Mi aigu (E) |

---

## 4. Installation du firmware Teensy (TDM_TEENSY)

### Étape 1 — Installer Homebrew (si pas déjà fait)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Après l'installation, suivre les instructions affichées pour ajouter Homebrew au PATH :
```bash
# Pour Apple Silicon (M1/M2/M3/M4) :
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"
```

### Étape 2 — Installer Python 3

```bash
brew install python@3.13
```

> [!NOTE]
> Python 3.13 est recommandé pour la stabilité. Python 3.14 fonctionne aussi mais est plus récent.

Vérifier :
```bash
python3 --version
# Doit afficher Python 3.13.x ou supérieur
```

### Étape 3 — Installer PlatformIO

PlatformIO est l'outil de compilation et téléversement du firmware sur la Teensy.

**Option A — Via VS Code (Recommandé pour les débutants) :**
1. Installer [Visual Studio Code](https://code.visualstudio.com/)
2. Dans VS Code, aller dans Extensions (⇧⌘X)
3. Chercher **"PlatformIO IDE"** et l'installer
4. Redémarrer VS Code

**Option B — En ligne de commande :**
```bash
# Installer PlatformIO Core (CLI)
pip3 install platformio

# Vérifier l'installation
pio --version
```

> [!WARNING]
> Si `pio` n'est pas trouvé après l'installation, ajouter le répertoire des scripts Python au PATH :
> ```bash
> echo 'export PATH="$HOME/Library/Python/3.13/bin:$PATH"' >> ~/.zprofile
> source ~/.zprofile
> ```

### Étape 4 — Cloner le projet firmware

```bash
cd ~/Workspace  # ou le dossier de votre choix
# Le paramètre --recurse-submodules est indispensable pour récupérer les librairies Q, gcem et infra
git clone --recurse-submodules https://github.com/Desmatheo/TDM_TEENSY.git
cd TDM_TEENSY
```

### Étape 5 — Ouvrir le projet dans VS Code

1. Ouvrir VS Code
2. **File → Open Folder** et sélectionner le dossier `TDM_TEENSY`
3. PlatformIO détecte automatiquement le projet (icône PlatformIO 🐜 dans la barre latérale)

### 🔧 Étape 6 — Patch 6 canaux USB (OBLIGATOIRE)

> [!CAUTION]
> **Cette étape est indispensable.** Par défaut, le framework Teensy ne supporte que **2 canaux audio USB** (stéréo). Pour avoir les **6 canaux hexaphoniques**, il faut patcher manuellement les fichiers internes du framework. Sans ce patch, le système ne fonctionnera pas correctement.

#### 6.1 — Faire un premier Build pour télécharger le framework

Dans VS Code, cliquez sur **✓ Build** (barre du bas). PlatformIO va télécharger le framework Teensy (~200 Mo). **Attendez que la compilation se termine** (même si elle échoue, le framework sera téléchargé).

#### 6.2 — Sauvegarder les fichiers d'origine

Avant de modifier quoi que ce soit, faites une copie de sauvegarde :

```bash
cp -r ~/.platformio/packages/framework-arduinoteensy/cores/teensy4 \
      ~/.platformio/packages/framework-arduinoteensy/cores/teensy4_backup
```

> [!TIP]
> En cas de problème, vous pourrez toujours restaurer les fichiers d'origine :
> ```bash
> rm -rf ~/.platformio/packages/framework-arduinoteensy/cores/teensy4
> cp -r ~/.platformio/packages/framework-arduinoteensy/cores/teensy4_backup \
>       ~/.platformio/packages/framework-arduinoteensy/cores/teensy4
> ```

#### 6.3 — Télécharger et appliquer le patch

```bash
# Télécharger le patch communautaire
cd /tmp
git clone https://github.com/alex6679/teensy-4-usbAudio.git
cd teensy-4-usbAudio

# Copier les fichiers patchés dans le framework
cp usb_audio.cpp usb_audio.h \
   usb_audio_interface.cpp usb_audio_interface.h \
   usb_desc.c usb_desc.h \
   ~/.platformio/packages/framework-arduinoteensy/cores/teensy4/
```

Ce patch permet au framework de lire le flag `-D USB_AUDIO_CHANNEL_COUNT=6` qui est déjà configuré dans le [`platformio.ini`](file:///Users/paulliaras/Workspace/TDM_TEENSY/platformio.ini) du projet.

#### 6.4 — Nettoyer le cache PlatformIO

Après avoir appliqué le patch, il faut **obligatoirement** nettoyer le cache pour que les fichiers modifiés soient pris en compte :

- **Via VS Code** : Cliquer sur l'icône **PlatformIO** (🏠) dans la barre latérale → **Project Tasks** → **teensy41** → **General** → **Clean**
- **Ou en ligne de commande** :
  ```bash
  cd ~/Workspace/TDM_TEENSY
  pio run -t clean
  ```

> [!WARNING]
> Si vous ne faites pas le Clean après le patch, PlatformIO utilisera les anciens fichiers compilés en cache et le patch ne sera pas pris en compte.

---

### Étape 7 — Compiler et flasher avec les boutons VS Code

Dans la **barre du bas** de VS Code, PlatformIO ajoute une rangée de boutons :

```
┌──────────────────────────────────────────────────────────┐
│  🏠  ✓ Build  → Upload  🗑️ Clean  🔌 Monitor  ...     │
└──────────────────────────────────────────────────────────┘
```

Le workflow est le suivant :

1. **🗑️ Clean** (icône poubelle) — Nettoie les fichiers de compilation précédents. À faire au moins la première fois ou si des erreurs bizarres apparaissent.

2. **✓ Build** (icône coche / flèche) — Compile le firmware. Si tout se passe bien, le terminal affiche :
   ```
   SUCCESS [teensy41]
   ```

3. **→ Upload** (icône flèche vers la droite) — Brancher la Teensy 4.1 au Mac via USB, puis cliquer sur Upload pour flasher le firmware.

> [!TIP]
> Si c'est le **premier flash** ou si la Teensy ne répond pas, **appuyer sur le bouton physique** de la Teensy pour la mettre en mode bootloader (le programme Teensy Loader s'ouvre automatiquement).

> [!TIP]
> Si la compilation échoue avec une erreur liée au standard C++, vérifiez que le fichier [`platformio.ini`](file:///Users/paulliaras/Workspace/TDM_TEENSY/platformio.ini) contient bien `-std=gnu++2a` dans les `build_flags`.

**Alternative en ligne de commande** (si vous préférez le terminal) :
```bash
pio run               # Compiler
pio run -t upload     # Compiler et flasher
pio run -t clean      # Nettoyer
```

> [!IMPORTANT]
> Sous macOS, le premier téléversement nécessite parfois d'autoriser l'accès USB dans **Préférences Système → Confidentialité et sécurité**.
> Si `pio run -t upload` échoue, essayez d'appuyer sur le **bouton Reset** de la Teensy juste avant de relancer la commande.

### Vérifier que le firmware fonctionne

Après le flash, la Teensy redémarre automatiquement. Pour vérifier :

```bash
pio device monitor -b 115200
```

Vous devriez voir des messages de debug (si `SerialUSB` est activé dans le code).

---

## 5. Installation de l'interface graphique (GUI)

### Étape 1 — Installer Tkinter (prérequis macOS)

`customtkinter` a besoin de Tkinter qui n'est pas toujours inclus avec Python sur Mac :

```bash
brew install python-tk@3.13
```

> [!WARNING]
> **Erreur courante** : Si vous voyez `ModuleNotFoundError: No module named '_tkinter'` en lançant la GUI, c'est que Tkinter n'est pas installé. Exécutez la commande ci-dessus puis recréez le venv.

### Étape 2 — Cloner le projet GUI

```bash
cd ~/Workspace  # ou le dossier de votre choix
git clone https://github.com/Desmatheo/GUI.git
cd GUI
```

### Étape 3 — Créer l'environnement virtuel Python

```bash
cd Pedal
python3 -m venv venv_mac
source venv_mac/bin/activate
```

Votre terminal devrait maintenant afficher `(venv_mac)` au début de la ligne.

### Étape 4 — Installer les dépendances

```bash
pip install customtkinter mido python-rtmidi
```

Les packages installés seront :
| Package | Version | Rôle |
|---|---|---|
| `customtkinter` | 6.0.0 | Framework GUI moderne |
| `mido` | 1.3.3 | Messages MIDI en Python |
| `python-rtmidi` | 1.5.8 | Backend pour communiquer avec les ports MIDI USB |
| `darkdetect` | — | Détection automatique du mode sombre (installé avec customtkinter) |

> [!NOTE]
> Il n'y a pas de fichier `requirements.txt` dans le repo. Les 3 packages ci-dessus (`customtkinter`, `mido`, `python-rtmidi`) sont les seuls nécessaires.

### Étape 5 — Lancer la GUI

Un script de lancement est fourni pour ne **jamais avoir à activer le venv manuellement** :

```bash
# Depuis le dossier GUI/Pedal :
./launch_gui.sh
```

C'est tout ! Le script active automatiquement le venv et lance l'application. Ça marche même si vous venez d'ouvrir un nouveau terminal.

> [!TIP]
> **Raccourci** : Vous pouvez aussi lancer la GUI depuis n'importe où avec le chemin complet :
> ```bash
> ~/Workspace/GUI/Pedal/launch_gui.sh
> ```

<details>
<summary>Alternative : activation manuelle du venv</summary>

Si vous préférez activer le venv vous-même :
```bash
cd ~/Workspace/GUI/Pedal
source venv_mac/bin/activate
python main_merge.py
```
Note : le venv se désactive à chaque fermeture de terminal. Il faut refaire `source venv_mac/bin/activate` à chaque nouveau terminal.

</details>

La fenêtre **"Pédale Hexa — Contrôleur MIDI"** devrait s'ouvrir.

> [!TIP]
> Si la Teensy n'est pas branchée, la GUI fonctionne quand même en mode "simulation" — elle affiche simplement un message "MIDI désactivé" dans le terminal.

---

## 6. Premier lancement — Tout connecter

### Checklist de branchement

```
1. ☐ Brancher la guitare hexaphonique → entrée DIN13 du PCB
2. ☐ Brancher la sortie Jack du PCB → ampli / casque
3. ☐ Brancher le câble USB de la Teensy → Mac
4. ☐ Vérifier que la Teensy est alimentée (LED allumée)
```

### Lancer le système

```bash
~/Workspace/GUI/Pedal/launch_gui.sh
```

### Vérifier la connexion MIDI

Dans le terminal, vous devriez voir :
```
Connecté à : Teensy MIDI (port X)
OK Port MIDI IN ouvert : Teensy MIDI (port X)
```

Si vous voyez `MIDI désactivé` :
1. Vérifiez que la Teensy est bien branchée
2. Cliquez sur le bouton **"🔄 Rescanner MIDI"** dans la GUI
3. Si ça ne marche toujours pas → voir section [Dépannage MIDI](#midi-non-détecté)

---

## 7. Utilisation de base

### Sélectionner une corde

En haut de la GUI, cliquez sur le bouton de la corde à configurer :
- **Mi** (E grave), **La**, **Ré**, **Sol**, **Si**, **Mi** (E aigu)
- **ALL** : applique les modifications à toutes les cordes simultanément

### Ajouter un effet dans la chaîne

Chaque corde possède **4 slots** d'effets reconfigurables :
1. Dans le menu déroulant du slot souhaité, sélectionner un effet
2. Les paramètres de l'effet apparaissent en dessous
3. Ajuster les sliders — les changements sont envoyés en temps réel via MIDI

### Effets disponibles

| Effet | Ce qu'il fait |
|---|---|
| **Compresseur** | Égalise le volume, augmente le sustain |
| **Distorsion** | 8 modes de saturation (Hard Clip → Tube → Fuzz...) |
| **Octaver** | Ajoute une octave au-dessus (+1) ou en dessous (-1, -2) |
| **Tremolo** | Modulation cyclique du volume (vibrato d'amplitude) |
| **Delay** | Écho/répétition (jusqu'à 4 secondes, synchro BPM possible) |
| **Noise Gate** | Coupe le bruit quand vous ne jouez pas |
| **Égaliseur** | EQ 5 bandes (80 Hz → 6600 Hz, ±12 dB) |

### Presets

- **Sauvegarder** : Cliquez sur le bouton de sauvegarde, donnez un nom au preset
- **Charger** : Sélectionnez un preset dans la liste déroulante
- **Presets Hexa** : Sauvegardent la configuration complète des 6 cordes

### Boutons d'action

| Bouton | Action |
|---|---|
| **BYPASS** | Désactive tous les effets (son direct) |
| **RANDOM** | Génère des paramètres aléatoires (pour expérimenter !) |
| **RESET** | Remet tout à zéro |

---

## 8. Dépannage (Troubleshooting)

### Compilation du firmware

#### ❌ `command not found: pio`

PlatformIO n'est pas dans le PATH.

```bash
# Solution 1 : Installer via pip
pip3 install platformio

# Solution 2 : Ajouter au PATH
echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.zprofile
source ~/.zprofile
```

#### ❌ `Error: Unknown board ID 'teensy41'`

Le package de la plateforme Teensy n'est pas installé. PlatformIO le télécharge normalement automatiquement, mais si ce n'est pas le cas :

```bash
pio pkg install -g -p "teensy"
```

#### ❌ Erreur de compilation C++ (`std::` / template)

Vérifiez que `platformio.ini` contient :
```ini
build_unflags = -std=gnu++14 -std=gnu++17
build_flags = 
    ...
    -std=gnu++2a
```

#### ❌ `Upload failed` / Teensy non détectée

1. **Débrancher** et **rebrancher** le câble USB
2. **Appuyer sur le bouton** de la Teensy (petit bouton poussoir sur la carte)
3. Relancer `pio run -t upload`
4. Sur macOS, vérifier **Préférences Système → Confidentialité et sécurité** pour autoriser l'accès USB

---

### GUI Python

#### ❌ `ModuleNotFoundError: No module named '_tkinter'`

Tkinter n'est pas installé pour votre version de Python.

```bash
# Installer Tkinter
brew install python-tk@3.13

# Recréer le venv proprement
cd ~/Workspace/GUI/Pedal
rm -rf venv_mac
python3 -m venv venv_mac
source venv_mac/bin/activate
pip install customtkinter mido python-rtmidi
```

#### ❌ `ModuleNotFoundError: No module named 'customtkinter'`

Le venv n'est pas activé, ou les dépendances ne sont pas installées.

```bash
# Activer le venv
source ~/Workspace/GUI/Pedal/venv_mac/bin/activate

# Réinstaller
pip install customtkinter mido python-rtmidi
```

#### ❌ `ModuleNotFoundError: No module named 'rtmidi'`

Le backend MIDI n'est pas installé.

```bash
pip install python-rtmidi
```

> [!CAUTION]
> Le package s'appelle `python-rtmidi` (avec le préfixe `python-`), **pas** `rtmidi`. Installer `rtmidi` (sans `python-`) installe un autre package incompatible.

---

### MIDI non détecté

#### La GUI affiche "MIDI désactivé"

1. **Vérifier le câble USB** : Est-ce que la Teensy est alimentée ? (LED allumée ?)
2. **Vérifier les ports MIDI** : Dans le terminal, la GUI liste les ports MIDI détectés. Elle cherche un port contenant `"teensy"`, `"daisy"` ou `"usb"` dans le nom.
3. **Lister les ports manuellement** :
   ```bash
   python3 -c "import mido; print('OUT:', mido.get_output_names()); print('IN:', mido.get_input_names())"
   ```
4. **Rescanner** : Utiliser le bouton "🔄 Rescanner MIDI" dans la GUI
5. **Utilitaire MIDI** : Ouvrir **Configuration audio et MIDI** sur Mac (`/Applications/Utilitaires/Configuration audio et MIDI.app`) et vérifier que le périphérique Teensy apparaît

#### Le MIDI se connecte mais aucun son ne sort

1. Vérifier que le **bypass global** n'est pas activé (bouton BYPASS dans la GUI)
2. Vérifier que les **cordes ne sont pas mutées** individuellement
3. Vérifier que **au moins un effet** est assigné dans un slot (ou que `UtilBypassRoutage = 1` et `UtilEffet = 1` dans le firmware)
4. Vérifier les **sorties Jack** du PCB → l'ampli est bien branché et allumé

---

### Problèmes audio / hardware

#### Aucun son du tout

1. Vérifier les **connexions DIN13** (ou les 6 jack mono si option Submarine)
2. Vérifier la **sortie Jack** → ampli
3. Dans le firmware, vérifier que `Osc = 0` et `GuitareCodec = 1` dans [`src/includes/Utils.h`](file:///Users/paulliaras/Workspace/TDM_TEENSY/src/includes/Utils.h)
4. **Test avec oscillateur interne** : Changer `Osc = 1` dans le code, recompiler et flasher. Si vous entendez un son de test, le problème vient de l'entrée guitare, pas du firmware.

#### Certaines cordes ne sonnent pas

> C'est un problème connu de la carte prototype.

1. **Soudures** : Vérifier les soudures sur le PCB (particulièrement les connecteurs DIN13 et les pistes vers le codec)
2. **Gains de préamplification** : Les cordes La et Mi aigu ont des gains très élevés (×21.0) dans le firmware car leur signal est faible par défaut. Ajuster si nécessaire dans le code source.
3. **Câbles** : Tester chaque câble mono individuellement (option Submarine)

#### Bruit / souffle excessif

Le préampli analogique du PCB prototype est **désactivé volontairement** car trop bruyant. L'amplification est gérée en logiciel. Si vous entendez du bruit :
1. Vérifier que le préampli matériel est bien désactivé sur le PCB
2. Ajuster le **Noise Gate** via la GUI
3. Vérifier les masses / blindage des câbles

---

## 9. Annexes

### Récapitulatif des commandes

```bash
# ─── FIRMWARE ───
cd ~/Workspace/TDM_TEENSY
pio run                    # Compiler
pio run -t upload          # Compiler et flasher
pio device monitor -b 115200  # Moniteur série (debug)

# ─── GUI ───
~/Workspace/GUI/Pedal/launch_gui.sh  # Lancer la GUI (venv activé automatiquement)

# ─── VÉRIFICATION MIDI ───
python3 -c "import mido; print(mido.get_output_names()); print(mido.get_input_names())"
```

### Configuration du firmware — Drapeaux principaux

Fichier : [`src/includes/Utils.h`](file:///Users/paulliaras/Workspace/TDM_TEENSY/src/includes/Utils.h)

| Define | Valeur par défaut | Description |
|---|:---:|---|
| `Osc` | `0` | `0` = entrée guitare, `1` = oscillateurs de test |
| `Usb` | `1` | Active les fonctionnalités USB |
| `SerialUSB` | `0` | Debug verbose sur le port série |
| `USE_MIDI_USB` | `1` | Active la réception MIDI USB |
| `USBOut` | `0` | Envoie l'audio sur USB (monitoring PC) |
| `UtilEffet` | `1` | Active les effets (`0` = bypass total) |
| `UtilBypassRoutage` | `1` | Active le routage dynamique 4 slots |

### Mapping MIDI complet (GUI → Teensy)

Les canaux MIDI 1 à 6 correspondent aux cordes Mi grave → Mi aigu.

| Effet | CC paramètres | CC bypass |
|---|---|---|
| Delay | 10–16 | 48 |
| Distorsion | 50–56 | 88 |
| Octaver | 90, 91, 95 | 89 |
| Tremolo | 110–116 | 118 |
| Égaliseur | 70–75 | 78 |
| Noise Gate | 30–32 | 38 |
| Compresseur | 100–104 | 99 |

> [!NOTE]
> Le mapping MIDI dans la GUI peut différer légèrement de celui documenté dans le README du firmware. Les valeurs ci-dessus sont celles effectivement utilisées dans le code source de la GUI ([`main_merge.py`](file:///Users/paulliaras/Workspace/GUI/Pedal/main_merge.py)).

### Ressources et liens utiles

| Ressource | Lien |
|---|---|
| PlatformIO | [platformio.org](https://platformio.org/) |
| Teensy 4.1 (PJRC) | [pjrc.com/store/teensy41.html](https://www.pjrc.com/store/teensy41.html) |
| CS42448 Datasheet | [cirrus.com](https://www.cirrus.com/products/cs42448/) |
| CustomTkinter docs | [customtkinter.tomschimansky.com](https://customtkinter.tomschimansky.com/) |
| Mido docs | [mido.readthedocs.io](https://mido.readthedocs.io/) |
| Dépôt firmware | [github.com/Desmatheo/TDM_TEENSY](https://github.com/Desmatheo/TDM_TEENSY) |
| Dépôt GUI | [github.com/Desmatheo/GUI](https://github.com/Desmatheo/GUI) |
