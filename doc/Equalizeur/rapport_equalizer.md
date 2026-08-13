# Rapport sur l'Effet Égaliseur (Equalizer)

L'égaliseur (EQ) est un effet audio fondamental qui permet d'ajuster l'amplitude de différentes bandes de fréquences d'un signal sonore. Contrairement à un simple contrôle de tonalité (graves/aigus), un égaliseur graphique ou paramétrique offre un contrôle précis sur plusieurs régions spécifiques du spectre audio, permettant de corriger des défauts acoustiques, d'améliorer la clarté d'un mix ou de sculpter le son de manière créative.

Ce rapport détaille la théorie derrière cet effet et explique son implémentation en C++ pour le microcontrôleur Teensy, qui repose sur l'utilisation de filtres biquadratiques en cascade.

## 1. Théorie de l'Égaliseur

L'égalisation fonctionne en appliquant des filtres à des bandes de fréquences spécifiques. Le type de filtre le plus couramment utilisé pour l'égalisation multi-bandes est le **filtre en cloche (Peaking EQ)**. 

### 1.1 Le Filtre Biquadratique (Biquad)

Un filtre biquadratique est un filtre IIR (Infinite Impulse Response) du second ordre. Sa fonction de transfert dans le domaine en z est donnée par la formule :

$$ H(z) = \frac{b_0 + b_1 z^{-1} + b_2 z^{-2}}{a_0 + a_1 z^{-1} + a_2 z^{-2}} $$

Pour obtenir un filtre en cloche (qui amplifie ou atténue une fréquence centrale spécifique tout en laissant les autres fréquences inchangées), les coefficients $b_0, b_1, b_2, a_0, a_1, a_2$ sont calculés à partir de trois paramètres principaux :
- $f_0$ : La fréquence centrale (en Hz).
- $\text{Gain}$ : L'amplification ou l'atténuation (en décibels, dB).
- $Q$ : Le facteur de qualité (Q-factor), qui détermine la largeur de la bande de fréquences affectée (la bande passante).

### 1.2 Calcul des coefficients (Audio EQ Cookbook)

L'implémentation repose sur les formules standard de l'Audio EQ Cookbook, le document de référence rédigé par Robert Bristow-Johnson (voir la section Sources). Pour un filtre *peaking EQ*, on procède ainsi :

1. **Calcul de l'amplitude $A$** :
   $$ A = 10^{\frac{\text{Gain}_{dB}}{40}} $$
   *(Note : L'utilisation de $40$ au lieu de $20$ dans la division vient de la définition spécifique de l'amplitude dans les équations des filtres peaking).*

2. **Calcul de la pulsation normalisée $\omega_0$** :
   $$ \omega_0 = 2\pi \frac{f_0}{F_s} $$
   *(où $F_s$ est la fréquence d'échantillonnage, ex: 44100 Hz).*

3. **Calcul du paramètre de largeur de bande $\alpha$** :
   $$ \alpha = \frac{\sin(\omega_0)}{2Q} $$

4. **Coefficients du filtre** :
   - $b_0 = 1 + \alpha A$
   - $b_1 = -2 \cos(\omega_0)$
   - $b_2 = 1 - \alpha A$
   - $a_0 = 1 + \frac{\alpha}{A}$
   - $a_1 = -2 \cos(\omega_0)$
   - $a_2 = 1 - \frac{\alpha}{A}$

### 1.3 Origine mathématique des coefficients (Transformation Bilinéaire)

Pour comprendre d'où viennent ces formules exactes, il faut examiner comment un filtre numérique est conçu à partir d'un modèle analogique.

**A. Le filtre analogique de départ (Domaine s)**
La fonction de transfert d'un filtre analogique d'égalisation "en cloche" (qui amplifie d'un facteur $A$ à la fréquence de résonance $\omega_0$), s'écrit de manière standard via la transformée de Laplace :
$$ H(s) = \frac{s^2 + s\left(\frac{A}{Q}\right) + 1}{s^2 + s\left(\frac{1}{A \cdot Q}\right) + 1} $$

**B. Le passage au numérique**
Un processeur travaille avec des échantillons discrets $z^{-1}$ (où $z^{-1}$ signifie "l'échantillon précédent"). Pour convertir l'équation analogique en numérique, on utilise la **Transformation Bilinéaire**. Elle consiste à remplacer chaque $s$ par une approximation en $z$ :
$$ s \approx \frac{1}{\tan(\omega_0 / 2)} \cdot \frac{1 - z^{-1}}{1 + z^{-1}} $$

En injectant cette fraction à la place de tous les $s$ dans $H(s)$ et en développant algébriquement, on obtient la fonction de transfert en Z :
$$ H(z) = \frac{b_0 + b_1 z^{-1} + b_2 z^{-2}}{a_0 + a_1 z^{-1} + a_2 z^{-2}} $$

Les termes $b$ (au numérateur) représentent la partie **Feed-forward** (échantillons d'entrée actuels et passés) et les termes $a$ (au dénominateur) représentent la partie **Feed-back** (échantillons de sortie passés, créant la résonance "IIR").

**C. L'équation de récurrence (Le code)**
La fonction de transfert peut être réécrite sous la forme d'une équation temporelle (l'équation de récurrence calculée par le processeur pour chaque échantillon $y[n]$) :
$$ a_0 \cdot y[n] = b_0 \cdot x[n] + b_1 \cdot x[n-1] + b_2 \cdot x[n-2] - a_1 \cdot y[n-1] - a_2 \cdot y[n-2] $$

**D. La normalisation par $a_0$**
Pour isoler $y[n]$, on divise l'ensemble de l'équation par $a_0$ :
$$ y[n] = \left(\frac{b_0}{a_0}\right)x[n] + \left(\frac{b_1}{a_0}\right)x[n-1] + \left(\frac{b_2}{a_0}\right)x[n-2] - \left(\frac{a_1}{a_0}\right)y[n-1] - \left(\frac{a_2}{a_0}\right)y[n-2] $$
C'est la raison pour laquelle, lors de l'implémentation, les coefficients sont systématiquement divisés par $a_0$ avant d'être envoyés au DSP.

### 1.4 Filtres en cascade

Un égaliseur à $N$ bandes est simplement constitué de $N$ filtres biquadratiques placés en série (en cascade). Le signal d'entrée traverse le filtre de la bande 1, puis le résultat traverse le filtre de la bande 2, et ainsi de suite.

## 2. Implémentation du Code et Algorithmique

L'effet est implémenté dans la classe `EqualizerEffect` (`Equalizer.cpp` et `Equalizer.h`). L'algorithme repose sur une approche par **blocs d'échantillons** (buffer) et utilise la bibliothèque logicielle de traitement du signal d'ARM, **CMSIS-DSP**, pour traiter **5 filtres biquadratiques en série**.

### 2.1 Variables d'état et Initialisation (Le Constructeur)

Un aspect fondamental des filtres IIR (qui utilisent les échantillons passés, $z^{-1}$ et $z^{-2}$) est qu'ils possèdent une "mémoire". En C++, cette mémoire s'appelle l'état du filtre.

Dans le fichier d'en-tête (`Equalizer.h`), nous avons les variables suivantes :
```cpp
float pCoeffs[5 * 5]; // 5 coefficients pour 5 bandes = 25 float
float pState[5 * 4];  // 4 variables d'état pour 5 bandes = 20 float
```
*   **`pCoeffs`** : Stocke les coefficients $b_0, b_1, b_2, -a_1, -a_2$ pour chaque bande.
*   **`pState`** : Stocke l'historique ($x[n-1], x[n-2], y[n-1], y[n-2]$) pour que le filtre sache où il en était à la fin du bloc audio précédent. L'algorithme de CMSIS DSP a besoin de 4 variables d'état par filtre biquadratique.

Dans le constructeur, nous initialisons la structure `iir_inst` qui lie ensemble les coefficients, l'état et l'architecture :
```cpp
// 5 = le nombre de filtres en cascade
arm_biquad_cascade_df1_init_f32(&iir_inst, 5, pCoeffs, pState);
```

### 2.2 Algorithme de Calcul des Coefficients (`calculateCoeffs()`)

Cette fonction est appelée chaque fois qu'un paramètre (gain d'une bande) change. L'algorithme suit ces étapes pour chacune des 5 bandes :

1.  **Récupération de la cible** : On lit le gain souhaité en dB pour la bande (ex: +3 dB pour les basses à 80 Hz).
2.  **Conversion mathématique** : On applique les formules de l'Audio EQ Cookbook (vues en section 1.2) pour générer les 5 coefficients mathématiques bruts.
3.  **Stockage formaté pour le DSP** : La fonction stocke les résultats dans le tableau `pCoeffs`.

> [!TIP]
> **Optimisation d'architecture**
> La bibliothèque `arm_math.h` exige que les coefficients soient ordonnés de cette manière : `b0, b1, b2, -a1, -a2`. L'inversion de signe pour $a_1$ et $a_2$ permet à l'algorithme interne de la puce ARM d'utiliser presque exclusivement des additions via des instructions matérielles MAC (Multiply-Accumulate). C'est beaucoup plus rapide que d'alterner les additions et les soustractions dans la boucle.

### 2.3 Algorithme de Traitement du Signal (`update()`)

C'est ici qu'est exécutée la boucle audio principale, appelée en continu par le framework audio Teensy. Le système ne donne pas le son échantillon par échantillon, mais par **blocs de 128 échantillons** pour plus d'efficacité.

**Étape 1 : Conversion d'Entrée (Fixed-point vers Floating-point)**
Les données audio provenant du synthétiseur ou de l'entrée audio sont en entiers 16 bits (`int16_t`, entre -32768 et +32767).
Le filtre biquad nécessite une grande précision numérique pour ne pas créer d'artefacts (distorsion due aux arrondis). L'algorithme commence donc par tout convertir en nombres à virgule flottante (`float`, entre -1.0 et 1.0).
```cpp
float f32_block[128];
for (int i = 0; i < 128; i++) {
    // Division par 32768.0 pour normaliser entre -1.0 et 1.0
    f32_block[i] = (float)block->data[i] / 32768.0f; 
}
```

**Étape 2 : Le filtrage DSP**
Au lieu d'écrire nous-mêmes les boucles `for` imbriquées pour appliquer l'équation de récurrence (vue en section 1.3) à chaque échantillon et chaque bande, on fait appel au moteur DSP ARM :
```cpp
arm_biquad_cascade_df1_f32(&iir_inst, f32_block, f32_out, 128);
```
Algorithmiquement, cette unique fonction C exécute la logique suivante :
*   Pour chaque échantillon (de l'index 0 à 127) :
    *   Faire passer l'échantillon dans le Filtre 1 (Basses à 80 Hz)
    *   Faire passer le résultat du Filtre 1 dans le Filtre 2 (250 Hz)
    *   ... Et ainsi de suite jusqu'au Filtre 5
    *   Mettre à jour l'historique `pState` de chaque filtre pour le calcul du prochain échantillon.

**Étape 3 : Application du volume, Saturation et Conversion de Sortie**
Le signal filtré doit ensuite être reconverti en entier 16 bits. 
```cpp
for (int i = 0; i < 128; i++) {
    // 1. Application du gain global (Master Volume)
    float sample = f32_out[i] * volume; 
    
    // 2. Saturation algorithmique (Clipping)
    sample = clampf(sample, -1.0f, 1.0f); 
    
    // 3. Re-multiplication par 32767 et cast en entier
    block->data[i] = (int16_t)(sample * 32767.0f);
}
```
L'étape de **saturation (clamp)** est algorithmiquement cruciale : si l'égaliseur a trop amplifié une fréquence, la valeur flottante va dépasser `1.0` ou descendre sous `-1.0`. Sans la fonction `clampf`, la multiplication par 32767 produirait un dépassement de capacité ("integer overflow"), ce qui inverserait brusquement le signe du signal et produirait un bruit numérique assourdissant. Le clamp agit comme un limiteur de sécurité ("hard clipper").

### 2.4 Contrôle des Paramètres et Mapping

La fonction `setParameter(id, value)` est le pont entre l'interface utilisateur (boutons physiques ou interface graphique MIDI) et l'algorithme mathématique.
Les valeurs entrantes (`value`) sont standardisées entre `0.0` (potentiomètre au minimum) et `1.0` (potentiomètre au maximum).

L'algorithme de `setBand()` s'occupe de la mise à l'échelle, appelée le "mapping" :
```cpp
// Transforme l'entrée [0.0 , 1.0] vers la sortie en décibels [-12.0 dB , +12.0 dB]
gains_db[band_index] = (value_norm - 0.5f) * 24.0f;
```
*   En soustrayant `0.5`, on centre la valeur sur `0.0` (la plage devient `[-0.5, 0.5]`). 
*   En multipliant par `24.0` (l'amplitude totale voulue, de -12 à +12), le bouton à `0.0` donne bien `-12 dB`, à `0.5` on a `0 dB` (filtre transparent), et à `1.0` on a `+12 dB`.

## Conclusion

Cette implémentation de l'égaliseur est à la fois robuste et très performante. L'utilisation de mathématiques standard pour les filtres IIR (*Audio EQ Cookbook*) garantit une réponse en fréquence musicale. Au niveau algorithmique, l'utilisation d'un traitement par blocs de 128 échantillons et de la bibliothèque **CMSIS DSP** permet au microcontrôleur d'appliquer les 5 filtres en cascade avec une efficacité redoutable, minimisant l'impact sur le CPU tout en conservant une grande flexibilité de contrôle.

## 3. Sources et Bibliographie

- **Robert Bristow-Johnson (RBJ)**, *Cookbook formulae for audio EQ biquad filter coefficients*. Document de référence de l'industrie pour calculer les filtres IIR.
  [Lien vers la spécification du W3C Audio WG (Audio EQ Cookbook)](https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html)
- **ARM**, *Documentation officielle CMSIS-DSP (Cortex Microcontroller Software Interface Standard)*. Détaille l'optimisation mathématique des filtres Biquad sur architecture ARM, le format des variables d'états, et la normalisation des coefficients.
  [Lien vers la documentation ARM CMSIS-DSP (Biquad Cascade IIR Filters Using Direct Form I Structure)](https://arm-software.github.io/CMSIS_5/DSP/html/group__BiquadCascadeDF1.html)
