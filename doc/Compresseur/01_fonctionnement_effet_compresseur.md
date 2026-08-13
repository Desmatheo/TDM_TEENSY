# Fonctionnement de l'effet Compresseur

## 1. Introduction

Le compresseur est un effet de contrôle de la dynamique (Dynamic Range Control ou DRC). Son but principal est de réduire l'écart de volume entre les sons les plus faibles et les sons les plus forts (la "plage dynamique") d'un signal audio. 

Sur une guitare (ou dans le cas de notre traitement hexaphonique corde par corde), il permet :
- D'atténuer les attaques trop agressives (transitoires).
- D'augmenter artificiellement la durée de résonance des notes (le sustain), en remontant le volume des fins de notes.
- De lisser les écarts de niveau entre les cordes jouées avec des intensités différentes.

---

## 2. Origine et Sources de l'Algorithme

L'algorithme implémenté dans ce projet repose sur une architecture standard appelée **Compresseur numérique de type "Feed-Forward" dans le domaine logarithmique**.

### Sources scientifiques et littéraires :
1. **Giannoulis, D., Massberg, M., & Reiss, J. D. (2012). *"Digital Dynamic Range Compressor Design—A Tutorial and Analysis"*. Journal of the Audio Engineering Society.**
   > **Note :** C'est le papier de référence dans le traitement du signal audio moderne pour le design de compresseurs. Il décrit précisément comment découpler les constantes de temps (attaque/relâchement) du calcul de la courbe statique de compression (Seuil/Ratio) pour éviter des artefacts indésirables, et recommande l'utilisation de filtres IIR (Infinite Impulse Response) du premier ordre pour le détecteur. Notre implémentation suit ces recommandations (Topology "Branching" avec détection peak).

2. **Zölzer, U. (2011). *"DAFX: Digital Audio Effects" (2nd Edition)*.**
   > **Note :** Le chapitre 4 ("Dynamic Range Control") détaille les mathématiques permettant de convertir les dépassements de seuil linéaire en décibels pour calculer le gain d'atténuation. L'équation `gainDb = overshootDb * (1.0f - (1.0f / ratio))` implémentée dans notre code en est directement tirée.

---

## 3. Architecture Mathématique de l'Implémentation

Le traitement de chaque échantillon audio suit un cheminement précis en 4 étapes :

### Étape 1 : Le Détecteur d'Enveloppe (Envelope Follower)
Le signal est d'abord redressé (on prend sa valeur absolue). Ensuite, on utilise un filtre IIR (passe-bas) du premier ordre pour "lisser" ce signal et obtenir son enveloppe.
Les constantes de temps `attackCoef_` et `releaseCoef_` dépendent du choix de l'utilisateur :

```cpp
float absSample = fabsf(sample);
if (absSample > envelope_) {
    // Si le signal monte (attaque)
    envelope_ = attackCoef_ * envelope_ + (1.0f - attackCoef_) * absSample;
} else {
    // Si le signal descend (relâchement)
    envelope_ = releaseCoef_ * envelope_ + (1.0f - releaseCoef_) * absSample;
}
```

### Étape 2 : Le Calcul du Gain (Domaine Logarithmique)
Une fois l'enveloppe détectée, on la convertit en Décibels (dB) pour pouvoir la comparer au Seuil (`Threshold`). 
Si l'enveloppe dépasse le seuil, on calcule de combien de dB il faut réduire le son en utilisant le `Ratio` :

```cpp
float envDb = linearToDb(envelope_);
float gainDb = 0.0f; // Gain par défaut : 0 dB (pas de changement)

if (envDb > thresholdDb_) {
    // On calcule de combien on dépasse le seuil
    float overshootDb = envDb - thresholdDb_;
    // On calcule la réduction de gain basée sur le Ratio
    float attenuationDb = overshootDb * (1.0f - (1.0f / ratio_));
    // Le gain devient négatif (on baisse le volume)
    gainDb = -attenuationDb;
}
```

### Étape 3 : Conversion Linéaire et Application du Gain
Le gain en dB est reconverti en échelle linéaire (un multiplicateur, ex: 0.5 pour diviser le volume par 2). 
On y ajoute le `Makeup Gain` (gain de compensation) qui permet de rehausser le volume global qui a été écrasé par la compression.

```cpp
float gainLinear = dbToLinear(gainDb);
float output = sample * gainLinear * makeupGainLinear_;
```

### Étape 4 : Saturation (Hard Clipping) de sécurité
Pour éviter tout débordement numérique (qui génère des artefacts très désagréables), une limitation stricte entre `-1.0f` et `1.0f` est appliquée à la fin.

---

## 4. Les Paramètres et Leurs Rôles

- **Threshold (Seuil - en dB) :** Le niveau au-dessus duquel le compresseur commence à agir. S'il est à `-20 dB`, tous les sons en dessous de `-20 dB` ne seront pas compressés.
- **Ratio :** Détermine la force de la compression. Un ratio de `4:1` signifie que si le son dépasse le seuil de `4 dB`, la sortie ne dépassera le seuil que de `1 dB` (le son est écrasé des 3/4).
- **Attack (Attaque - en ms) :** Le temps mis par l'enveloppe pour réagir à une hausse de volume. Une attaque lente (ex: 30ms) laisse passer le "clac" de la corde avant de compresser. Une attaque très rapide (ex: 1ms) agit immédiatement.
- **Release (Relâchement - en ms) :** Le temps mis par l'enveloppe pour retomber à zéro quand le volume baisse. S'il est trop rapide, on entend un effet de "pompage". S'il est trop lent, le compresseur restera actif pour la note suivante.
- **Makeup Gain (Gain de compensation - en dB) :** Rehausse manuellement le volume de sortie pour compenser la perte de dynamique causée par la compression. C'est grâce à cela qu'on augmente le "sustain" apparent.
