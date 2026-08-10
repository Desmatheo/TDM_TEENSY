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

L'implémentation repose sur les formules standard de l'Audio EQ Cookbook (souvent attribuées à Robert Bristow-Johnson). Pour un filtre *peaking EQ*, on procède ainsi :

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

## 2. Implémentation du Code

L'effet est implémenté dans la classe `EqualizerEffect` (`Equalizer.cpp` et `Equalizer.h`). Il s'agit d'un **égaliseur à 5 bandes** utilisant la bibliothèque CMSIS DSP de ARM pour des performances optimales sur le microcontrôleur.

### 2.1 Initialisation et Configuration

Les 5 fréquences centrales sont prédéfinies pour couvrir le spectre audio :
- **80 Hz** (Basses profondes)
- **250 Hz** (Bas-médiums)
- **750 Hz** (Médiums)
- **2200 Hz** (Haut-médiums)
- **6600 Hz** (Aigus)

Le facteur de qualité $Q$ est fixé à `1.414` (ce qui correspond à $\sqrt{2}$), une valeur classique offrant un bon compromis entre la largeur de la cloche et le chevauchement des bandes.

```cpp
float frequencies[5] = {80.0f, 250.0f, 750.0f, 2200.0f, 6600.0f};
float Q = 1.414f; 
float Fs = 44100.0f; 
```

### 2.2 Calcul des Coefficients (`calculateCoeffs()`)

La méthode `calculateCoeffs()` implémente exactement la théorie mentionnée ci-dessus. Pour chaque bande de fréquence :
1. Elle récupère le gain en dB (qui peut aller de -12 dB à +12 dB).
2. Elle calcule les variables intermédiaires $A$, $\omega_0$ et $\alpha$.
3. Elle déduit les coefficients $b_0, b_1, b_2, a_1, a_2$ (normalisés par $a_0$).

> [!TIP]
> **Stockage CMSIS DSP**
> La bibliothèque `arm_math.h` exige que les coefficients soient passés dans un ordre spécifique pour chaque biquad : `b0, b1, b2, -a1, -a2`. L'inversion de signe pour $a_1$ et $a_2$ est une optimisation de l'architecture DSP pour utiliser l'instruction MAC (Multiply-Accumulate) plus efficacement.

```cpp
// CMSIS DSP stocke : b0, b1, b2, -a1, -a2
int idx = i * 5;
pCoeffs[idx]     = b0;
pCoeffs[idx + 1] = b1;
pCoeffs[idx + 2] = b2;
pCoeffs[idx + 3] = -a1;
pCoeffs[idx + 4] = -a2;
```

### 2.3 Traitement du Signal (`update()`)

Le traitement audio principal s'effectue dans la méthode `update()`.

Si le routage Teensy est activé, il récupère un bloc audio de 128 échantillons (entiers 16 bits) et le convertit en flottants (entre -1.0 et 1.0) :
```cpp
float f32_block[128];
for (int i = 0; i < 128; i++) {
    f32_block[i] = (float)block->data[i] / 32768.0f;
}
```

Ensuite, la magie s'opère en une seule ligne grâce à la fonction ultra-optimisée de la bibliothèque CMSIS DSP qui applique la cascade de nos 5 filtres biquadratiques (Direct Form I) :
```cpp
arm_biquad_cascade_df1_f32(&iir_inst, f32_block, f32_out, 128);
```
L'instance `iir_inst` a été initialisée avec le nombre de stages (5), les coefficients calculés et un tableau d'états (mémoire des échantillons précédents nécessaires pour les équations de récurrence $z^{-1}$ et $z^{-2}$).

Enfin, un volume global est appliqué, le signal est "clampé" (limité entre -1.0 et 1.0 pour éviter l'overflow numérique en sortie) et reconverti en entier 16 bits.

### 2.4 Contrôle des Paramètres

Les paramètres sont mis à jour via `setParameter()`. Les bandes sont indexées de 0 à 4, et le volume est le paramètre 5.

La méthode `setBand()` reçoit une valeur normalisée entre `0.0` et `1.0`. Cette valeur est mise à l'échelle pour correspondre à une plage de **-12 dB à +12 dB**.
```cpp
// Mappe [0.0, 1.0] vers [-12dB, +12dB]
gains_db[band_index] = (value_norm - 0.5f) * 24.0f;
calculateCoeffs();
```
À chaque modification d'un gain, `calculateCoeffs()` est appelée pour régénérer dynamiquement les coefficients de l'égaliseur.

## Conclusion

Cette implémentation de l'égaliseur est à la fois robuste et très performante. L'utilisation de mathématiques standard pour les filtres IIR (*Audio EQ Cookbook*) garantit une réponse en fréquence musicale, tandis que l'utilisation de la bibliothèque **CMSIS DSP** (avec ses instructions optimisées SIMD et MAC) permet au microcontrôleur d'appliquer 5 filtres biquadratiques à chaque échantillon audio avec un impact minimal sur le CPU.
