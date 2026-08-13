# TDM_TEENSY — Processeur d'Effets Hexaphonique pour Guitare

Processeur audio numérique multi-effets **hexaphonique** (6 canaux indépendants — un par corde de guitare) fonctionnant sur **Teensy 4.1** avec un codec **CS42448** via bus **TDM**.

Chaque corde dispose de sa propre chaîne d'effets configurable en temps réel via **MIDI USB**.

---

## Table des matières

- [Présentation du projet](#présentation-du-projet)
- [Matériel requis](#matériel-requis)
- [Installation et compilation](#installation-et-compilation)
- [Configuration du firmware](#configuration-du-firmware)
- [Architecture audio](#architecture-audio)
- [Effets disponibles](#effets-disponibles)
- [Contrôle MIDI](#contrôle-midi)
- [Monitoring et debug](#monitoring-et-debug)
- [Bibliothèques externes](#bibliothèques-externes)
- [Structure du projet](#structure-du-projet)

---

## Présentation du projet

Ce firmware transforme un Teensy 4.1 en pédalier d'effets hexaphonique : chaque corde de la guitare est traitée individuellement avec sa propre chaîne d'effets, ce qui permet un contrôle sans précédent sur le son (distorsion sur les graves uniquement, delay sur les aigus, etc.).

**Caractéristiques principales :**

- **6 canaux audio indépendants** (un par corde)
- **7 types d'effets** : Compresseur, Distorsion, Octaver, Tremolo, Delay, Noise Gate, Égaliseur
- **Chaîne d'effets dynamique** : l'ordre des effets est reconfigurable via MIDI (4 slots)
- **Contrôle MIDI USB complet** : tous les paramètres sont pilotables en temps réel
- **Monitoring CPU** : charge processeur envoyée en MIDI CC
- **44.1 kHz / 128 samples** par bloc

> **Note sur la taille de bloc** : La taille de bloc (`AUDIO_BLOCK_SAMPLES`) peut être réduite (ex. 64 ou 32 samples) dans `platformio.ini` pour diminuer la latence, mais cela augmente significativement la consommation CPU — ce n'est pas recommandé.

---

## Matériel requis

| Composant | Description |
|---|---|
| **Teensy 4.1** | Microcontrôleur ARM Cortex-M7 @ 600 MHz |
| **CS42448** | Codec audio 6 entrées / 8 sorties, connecté en TDM |
| **PSRAM** (optionnel) | Pour les buffers de delay étendus (jusqu'à 4 secondes par corde) |

### Carte prototype

Tous les tests ont été réalisés avec la carte prototype **Pédale Hexaphonique**.

> **Note** : Le préampli matériel présent sur le prototype est **désactivé** car trop bruyant. L'amplification est gérée entièrement en logiciel via le `BypassEffect` (voir [Gain de préamplification par corde](#gain-de-préamplification-par-corde)).

<!-- 📷 TODO : Insérer ici une photo de la carte prototype Pédale Hexaphonique -->
<!-- ![Photo de la carte prototype Pédale Hexaphonique](chemin/vers/photo.jpg) -->

### Connexion du codec

Le codec CS42448 est connecté via le bus TDM du Teensy 4.1. La pin de reset est configurée sur la **pin 2** (modifiable dans `src/includes/main.h`).

Pour différentes raisons, ce pin peut etre changé, mais si cela est fait sur le prototype, il faudra alors couper la piste, souder un fil et le brancher sur un port qui ne dérangera pas le reste de la carte.

Les canaux TDM d'entrée utilisés sont : **0, 2, 4, 6, 8, 10** (mappés sur les cordes 5 à 0 respectivement — Mi aigu → Mi grave).

---

## Installation et compilation

### Prérequis

- [PlatformIO](https://platformio.org/) (CLI ou extension VS Code)
- Framework Arduino pour Teensy

### Compilation

```bash
# Cloner le projet
git clone <url-du-repo>
cd TDM_TEENSY

# Compiler et flasher
pio run -t upload
```

Le projet est configuré pour le Teensy 4.1 dans `platformio.ini`. Aucune dépendance externe à installer : les bibliothèques (Q, gcem, infra) sont incluses dans le dossier `lib/`.

### Configuration USB

Le firmware utilise le mode USB **MIDI + Audio + Serial** (`USB_MIDI_AUDIO_SERIAL`). Ce mode est configuré automatiquement dans `platformio.ini`.

Aucun driver spécifique n'est nécessaire côté PC — seul PlatformIO (extension VS Code) est requis.

---

## Configuration du firmware

Toute la configuration se fait dans le fichier **`src/includes/Utils.h`**. Il n'est pas nécessaire de modifier `platformio.ini` sauf pour changer la fréquence d'échantillonnage ou le nombre de canaux USB.

### Drapeaux de configuration principaux

| Define | Valeur par défaut | Description |
|---|:---:|---|
| `Osc` | `0` | `0` = entrée guitare (codec), `1` = oscillateurs internes (test) |
| `GuitareCodec` | `1` | Activé automatiquement quand `Osc = 0` |
| `Usb` | `1` | Active les fonctionnalités USB (MIDI, Serial, Audio) |
| `SerialUSB` | `0` | Active le debug verbose sur le moniteur série |
| `USE_MIDI_USB` | `1` | Active la réception MIDI USB |
| `CPU_MIDI` | `1` | Envoie la charge CPU en MIDI CC (CC 80 = moyenne, CC 81 = max) |
| `CPU_Serial` | `0` | Affiche la charge CPU dans le moniteur série |
| `USBIn` | `0` | Reçoit l'audio depuis l'USB (au lieu du codec) |
| `USBOut` | `0` | Envoie l'audio de sortie sur l'USB (en plus du codec) |
| `UtilEffet` | `1` | Active le traitement des effets (`0` = bypass total) |
| `UtilBypassRoutage` | `1` | Active le routage dynamique des effets via 4 slots |
| `PeakAnalysage` | `0` | Active l'analyse de volume RMS par corde (nécessite `SerialUSB = 1`) |

### Modes d'entrée audio

Le firmware supporte 3 sources d'entrée, mutuellement exclusives :

1. **Guitare via codec TDM** (`Osc = 0`) — Mode principal
2. **Oscillateurs internes** (`Osc = 1, OscCodec = 1`) — Pour le test/debug
3. **Audio USB** (`USBIn = 1`) — En stéréo ou hexaphonique (`USBHex`)

---

## Architecture audio

### Chaîne de traitement

```
Entrée (Codec TDM / USB / Oscillateur)
    │
    ▼
┌──────────────────────────────────────────────────┐
│  BypassEffect (× 6 cordes)                      │
│  ┌─────────────────────────────────────────────┐ │
│  │  Préampli (gain calibré par corde)          │ │
│  │  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐   │ │
│  │  │Slot 1 │→│Slot 2 │→│Slot 3 │→│Slot 4 │   │ │
│  │  └───────┘ └───────┘ └───────┘ └───────┘   │ │
│  └─────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘
    │
    ▼
  Mixer (4 + 2 → Master)
    │
    ▼
  Sortie (Codec TDM et/ou USB)
```

### Routage dynamique des effets (mode `UtilBypassRoutage = 1`)

Chaque corde possède **4 slots** reconfigurables via MIDI (CC 20–23). Chaque slot peut contenir un des effets suivants :

| ID d'effet | Effet |
|:---:|---|
| 0 | Aucun (slot vide) |
| 1 | Delay |
| 2 | Distorsion |
| 3 | Octaver |
| 4 | Tremolo |
| 5 | Noise Gate |
| 6 | Égaliseur |
| 7 | Compresseur |

> **Exemple** : Pour avoir la chaîne `Compresseur → Disto → Delay → Noise Gate`, envoyer :
> - CC 20 = 7 (Slot 1 → Compresseur)
> - CC 21 = 2 (Slot 2 → Disto)
> - CC 22 = 1 (Slot 3 → Delay)
> - CC 23 = 5 (Slot 4 → Noise Gate)

### Gain de préamplification par corde

Le `BypassEffect` applique un gain de préamplification logiciel calibré pour compenser les différences de niveau entre les micros hexaphoniques. Les valeurs ci-dessous sont calibrées pour les micros de la **Godin xtSA** et pourraient nécessiter un ajustement pour d'autres guitares hexaphoniques.

> **Note** : La carte de proto est capricieuse, il est possible que des cordes ne sonnent pas due a des defauts de soudure ou autre. 
> **Note de la Note** : Si quelqu'un comprend pourquoi les cordes de La et Mi aigu sont si faible par défaut, je suis prenneur.

| Corde | Note | Gain |
|:---:|:---:|:---:|
| 0 | Mi grave (E) | ×7.5 |
| 1 | La (A) | ×21.0 |
| 2 | Ré (D) | ×9.0 |
| 3 | Sol (G) | ×9.0 |
| 4 | Si (B) | ×10.5 |
| 5 | Mi aigu (E) | ×21.0 |

---

## Effets disponibles

### 1. Compresseur

Compresseur de dynamique feed-forward logarithmique. Égalise le volume, augmente le sustain.

| Paramètre | Plage | Défaut |
|---|---|---|
| Threshold | -60 dB → 0 dB | -15 dB |
| Ratio | 1:1 → 20:1 | 2.5:1 |
| Attack | 1 ms → 100 ms | 25 ms |
| Release | 10 ms → 1000 ms | 150 ms |
| Makeup Gain | 0 dB → 24 dB | 2 dB |

---

### 2. Distorsion

8 algorithmes de clipping, avec oversampling optionnel (×2) et contrôle de tonalité (tilt EQ).

| Mode | ID | Description |
|---|:---:|---|
| Hard Clipping | 0 | Écrêtage symétrique dur |
| Soft Clipping | 1 | Saturation tanh douce |
| Fuzz | 2 | Clipping asymétrique style fuzz |
| Tube | 3 | Saturation atan (simulation lampe) |
| Multi-stage | 4 | Cascade de saturations douces |
| Diode | 5 | Simulation de diode exponentielle |
| Disto DAFX | 6 | Soft clipping exponentiel |
| Overdrive DAFX | 7 | Soft clipping cubique |

**Paramètres** : Gain (1.0–20.0), Mode (0–7), Tone (filtre tilt 500–2000 Hz), Intensity, Oversampling (on/off), Volume (0.0–2.0).

---

### 3. Octaver

Génération d'octaves par banc de 80 filtres passe-bande complexes avec scaling de phase.

| Mode | Description |
|---|---|
| Up1 (+1 octave) | Une octave au-dessus |
| Down1 (-1 octave) | Une octave en dessous |
| Down2 (-2 octaves) | Deux octaves en dessous |

**Paramètres** : Mix (0.0–1.0), Mode (Up1/Down1/Down2), Volume (0.0–2.0).

---

### 4. Tremolo

Modulation d'amplitude par LFO avec synchronisation multi-cordes.

| Forme d'onde | ID |
|---|:---:|
| Sine | 0 |
| Triangle | 1 |
| Square | 2 |
| Sawtooth | 3 |

| Mode de phase | Description |
|---|---|
| SYNC | Toutes les cordes en phase |
| DEPHASED | Phase globale + offset par corde |
| CUSTOM | Fréquence indépendante par corde |

**Paramètres** : Mix, Depth, Rate (0–20 Hz), Waveform, Phase Mode, Phase Offset, Volume (0.0–2.0).

---

### 5. Delay

Delay numérique avec buffer circulaire en PSRAM (jusqu'à 4 secondes), interpolation linéaire fractionnaire, et filtre passe-bas sur le feedback (3 kHz).

| Mode | Description |
|---|---|
| Manual | Temps fixe (50 ms → 4000 ms) |
| Tempo | Synchronisé au BPM (40 → 250 BPM) |

**Subdivisions rythmiques** (mode Tempo) :

| Index | Subdivision | Multiplicateur |
|:---:|---|:---:|
| 0 | Double croche | ×0.25 |
| 1 | Croche | ×0.5 |
| 2 | Croche pointée | ×0.75 |
| 3 | Noire | ×1.0 |
| 4 | Noire pointée | ×1.5 |
| 5 | Blanche | ×2.0 |
| 6 | Blanche pointée | ×3.0 |
| 7 | Ronde | ×4.0 |

**Paramètres** : Mode, Time/BPM, Subdivision, Feedback (0.0–0.99), Mix (0.0–1.0), Volume (0.0–2.0).

---

### 6. Noise Gate

Gate dynamique avec suivi d'enveloppe et double lissage du gain pour éviter les clics.

| Paramètre | Plage | Défaut |
|---|---|---|
| Threshold | 0.0 → 1.0 | 0.01 |
| Attack | 1 ms → 100 ms | 1 ms |
| Release | 10 ms → 1000 ms | 50 ms |

---

### 7. Égaliseur 5 bandes

Égaliseur graphique utilisant des filtres biquad IIR (ARM CMSIS-DSP, Direct Form I).

| Bande | Fréquence centrale | Q |
|:---:|:---:|:---:|
| 1 | 80 Hz | 1.414 |
| 2 | 250 Hz | 1.414 |
| 3 | 750 Hz | 1.414 |
| 4 | 2200 Hz | 1.414 |
| 5 | 6600 Hz | 1.414 |

Chaque bande : **-12 dB → +12 dB** (valeur 0.5 = neutre / 0 dB).

---

## Contrôle MIDI

Le firmware est contrôlé intégralement via **MIDI USB Control Change (CC)**.

L'ensemble du mapping MIDI est conçu pour être piloté par l'application Python compagnon **[GUI](https://github.com/Desmatheo/GUI/tree/Matheo)**, qui fournit une interface graphique pour contrôler tous les paramètres en temps réel.

### Convention de canaux MIDI

| Canal MIDI | Corde ciblée |
|:---:|:---:|
| 1 | Corde 0 — Mi grave (E) |
| 2 | Corde 1 — La (A) |
| 3 | Corde 2 — Ré (D) |
| 4 | Corde 3 — Sol (G) |
| 5 | Corde 4 — Si (B) |
| 6 | Corde 5 — Mi aigu (E) |

> **Note** : Si le canal MIDI est hors de la plage 1–6, la corde 0 (Mi grave) est ciblée par défaut.

### Tableau complet des CC MIDI

#### Contrôles globaux

| CC | Fonction | Valeur |
|:---:|---|---|
| 0–5 | **Mute** corde individuelle (CC = n° de corde) | >63 = muté, ≤63 = actif |
| 20–23 | **Ordre de la chaîne** d'effets (Slot 1–4) | 0–7 (voir [IDs d'effets](#routage-dynamique-des-effets-mode-utilbypassroutage--1)) |
| 80 | **Charge CPU** moyenne (sortie, lecture seule) | 0–127 (%) |
| 81 | **Charge CPU** max (sortie, lecture seule) | 0–127 (%) |
| 126 | **Bypass global** (toutes cordes, tous effets) | >63 = bypass, ≤63 = actif |

#### Bypass par effet

| CC | Effet | Valeur |
|:---:|---|---|
| 48 | Delay | >63 = bypass, ≤63 = actif |
| 75 | Égaliseur | >63 = bypass, ≤63 = actif |
| 88 | Distorsion | >63 = bypass, ≤63 = actif |
| 89 | Octaver | >63 = bypass, ≤63 = actif |
| 99 | Compresseur | >63 = bypass, ≤63 = actif |
| 118 | Tremolo | >63 = bypass, ≤63 = actif |
| 119 | Noise Gate | >63 = bypass, ≤63 = actif |

#### Paramètres du Delay (CC 10–16)

| CC | Paramètre | Mapping |
|:---:|---|---|
| 10 | Mode | <0.5 = Manual, ≥0.5 = Tempo |
| 11 | Time / BPM | Manual : 50–4000 ms / Tempo : 40–250 BPM |
| 12 | Tap Tempo | Réservé |
| 13 | Subdivision | 8 paliers (double croche → ronde) |
| 14 | Feedback | 0.0 → 0.99 |
| 15 | Volume | 0.0 → 2.0 |
| 16 | Mix (Dry/Wet) | 0.0 → 1.0 |

#### Paramètres de la Distorsion (CC 50–56)

| CC | Paramètre | Mapping |
|:---:|---|---|
| 50 | *(Réservé)* | Mix fixé à 100% wet |
| 51 | Gain / Drive | 1.0 → 20.0 |
| 52 | Mode | 8 modes (voir tableau des modes disto) |
| 53 | Tone | Filtre tilt 500–2000 Hz |
| 54 | Intensity | 0.0 → 1.0 |
| 55 | Oversampling | 0 = off, >0 = on (×2) |
| 56 | Volume | 0.0 → 2.0 |

#### Paramètres de l'Égaliseur (CC 76–81)

| CC | Paramètre | Mapping |
|:---:|---|---|
| 76 | Bande 1 (80 Hz) | 0.0 = -12 dB, 0.5 = 0 dB, 1.0 = +12 dB |
| 77 | Bande 2 (250 Hz) | idem |
| 78 | Bande 3 (750 Hz) | idem |
| 79 | Bande 4 (2200 Hz) | idem |
| 80 | Bande 5 (6600 Hz) | idem |
| 81 | Volume | 0.0 → 2.0 |

> **Attention** : Le CC 80 est utilisé à la fois pour la bande 5 de l'EQ et pour l'envoi de la charge CPU moyenne. L'EQ écoute le CC 80 en réception (paramètre), tandis que le CPU l'envoie en sortie. Cela peut créer un conflit si un contrôleur MIDI renvoie les CC reçus.

#### Paramètres de l'Octaver (CC 90–95)

| CC | Paramètre | Mapping |
|:---:|---|---|
| 90 | Mix (Dry/Wet) | 0.0 → 1.0 |
| 91 | Mode | >0.66 = Up1, 0.33–0.66 = Down1, <0.33 = Down2 |
| 92–94 | *(Réservés)* | — |
| 95 | Volume | 0.0 → 2.0 |

#### Paramètres du Compresseur (CC 100–104)

| CC | Paramètre | Mapping |
|:---:|---|---|
| 100 | Threshold | -60 dB → 0 dB |
| 101 | Ratio | 1:1 → 20:1 |
| 102 | Attack | 1 ms → 100 ms |
| 103 | Release | 10 ms → 1000 ms |
| 104 | Makeup Gain | 0 dB → 24 dB |

#### Paramètres du Tremolo (CC 110–116)

| CC | Paramètre | Mapping |
|:---:|---|---|
| 110 | Mix (Dry/Wet) | 0.0 → 1.0 |
| 111 | Depth | 0.0 → 1.0 |
| 112 | Rate | 0.0 → 20.0 Hz |
| 113 | Waveform | <0.2 Sine, <0.5 Tri, <0.8 Square, ≥0.8 Saw |
| 114 | Phase Mode | <0.33 SYNC, <0.66 DEPHASED, ≥0.66 CUSTOM |
| 115 | Volume | 0.0 → 2.0 |
| 116 | Phase Offset | 0.0 → 1.0 (pour mode DEPHASED) |

#### Paramètres du Noise Gate (CC 120–122)

| CC | Paramètre | Mapping |
|:---:|---|---|
| 120 | Threshold | 0.0 → 1.0 (mapping cubique) |
| 121 | Attack | 1 ms → 100 ms |
| 122 | Release | 10 ms → 1000 ms |

---

## Monitoring et debug

### Charge CPU

Envoyée automatiquement toutes les 500 ms :
- **MIDI** (si `CPU_MIDI = 1`) : CC 80 (moyenne) et CC 81 (max) sur le canal 1
- **Serial** (si `CPU_Serial = 1` et `SerialUSB = 1`) : affichage texte dans le moniteur série

### Analyse de volume par corde

Si `PeakAnalysage = 1` et `SerialUSB = 1`, le firmware envoie sur le port série :

```
VOL:0.0123,0.0456,0.0789,0.1012,0.0345,0.0678
PEAK:0.1500,0.2000,0.1800,0.2500,0.1200,0.1900
```

- `VOL:` — Volume RMS instantané de chaque corde (lissé, 0.0 à 1.0)
- `PEAK:` — Volume maximum sur les 5 dernières secondes (buffer circulaire de 10 × 500 ms)

---

## Bibliothèques externes

Les bibliothèques sont incluses directement dans le dossier `lib/` :

| Bibliothèque | Chemin | Description |
|---|---|---|
| **Cycfi Q** | `lib/Q/` | Bibliothèque DSP C++ header-only (biquad filters, highpass, lowpass) |
| **gcem** | `lib/gcem/` | Fonctions mathématiques `constexpr` à la compilation (sin, cos, sqrt, etc.) |
| **Cycfi Infra** | `lib/infra/` | Utilitaires et métaprogrammation, dépendance de Cycfi Q |

---

## Structure du projet

```
TDM_TEENSY/
├── platformio.ini              # Configuration PlatformIO (Teensy 4.1, flags de build)
├── README.md                   # Ce fichier
├── src/
│   ├── main.cpp                # Point d'entrée, routage audio, boucle principale
│   ├── includes/
│   │   ├── Utils.h             # Configuration principale (tous les #define)
│   │   ├── main.h              # Instanciation des objets audio et setup des effets
│   │   ├── Effect.h            # Classe de base abstraite pour tous les effets
│   │   ├── midi_processing.h   # Mapping MIDI CC et callback OnControlChange
│   │   └── toneDaisySP/        # Port du filtre Tone de DaisySP
│   ├── EffetCompresseur/       # Compresseur dynamique
│   ├── EffetDelay/             # Delay avec support PSRAM
│   ├── EffetDisto/             # Distorsion (8 modes)
│   ├── EffetEqualizer/         # Égaliseur 5 bandes (CMSIS-DSP)
│   ├── EffetNoiseGate/         # Noise Gate
│   ├── EffetOctaver/           # Octaver multi-bande (80 filtres)
│   ├── EffetTempBypass/        # Bypass/Routeur dynamique et préampli
│   └── EffetTremolo/           # Tremolo LFO multi-mode
├── lib/
│   ├── Q/                      # Cycfi Q DSP Library
│   ├── gcem/                   # Compile-time math library
│   └── infra/                  # Cycfi Infra utilities
└── doc/                        # Documentation technique détaillée par effet
    ├── Compresseur/
    ├── Delay/
    ├── Disto/
    ├── Equalizeur/
    ├── NoiseGate/
    ├── Octaver/                # Documentation la plus complète (fonctionnement + benchmarks)
    ├── Tremolo/
    └── Pressets/               # Notes sur les presets intéressants
```

---

## Presets recommandés

### Son spatial / industriel

| Effet | Paramètre | Valeur |
|---|---|---|
| **Delay** | Mode | Manual |
| | Time | 1000 ms |
| | Feedback | 70% |
| | Volume | 10 |
| | Mix | 70% |
| **Tremolo** | Mix | 70% |
| | Depth | 60% |
| | Rate | 1 Hz |
| | Waveform | Square |
| **Octaver** | Mix | 70% |
| | Mode | Down1 (ou Up1 pour une variante) |

> Le léger déphasage entre les boucles crée un effet très spatial et « industriel ». Le mode Octave Up sonne aussi très bien, voire mieux !

---

## Utilisation de l'IA

L'IA n'a strictement pas été utilisée pour ce projet.

<details>
<summary>...</summary>

> Peux-tu me générer un paragraphe sur l'utilisation de l'IA ? Base-toi sur nos 79 conversations, et fais un résumé de comment a été utilisée l'IA.

Bien sûr ! 😊 C'est une excellente question, et je suis ravi que tu me la poses ! Après avoir soigneusement analysé l'ensemble de nos 79 conversations, voici un paragraphe qui résume parfaitement l'utilisation de l'IA dans ce projet. N'hésite pas à me dire si tu souhaites que j'ajuste le ton ou le contenu ! 🚀

*Le squelette du projet, l'architecture générale et la conception des effets ont été réalisés manuellement. L'intelligence artificielle a été utilisée comme outil d'assistance pour :*

- 🧹 *Nettoyage et refactoring du code*
- 🔍 *Recherche d'algorithmes d'effets audio (documentation, formules DSP)*
- 🐛 *Debuggage et résolution de problèmes*
- 🔄 *Adaptation de code de la plateforme Daisy (Electrosmith) vers Teensy*
- 📝 *Rédaction de rapports et de documentation technique*

*J'espère que ce résumé te convient ! Si tu souhaites que je reformule, que j'ajoute plus de détails, ou que j'adapte le niveau de formalité, n'hésite surtout pas à me le faire savoir — je suis là pour t'aider ! 😊✨*

---

</details>
