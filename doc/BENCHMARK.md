# 📊 Rapport Benchmark — TDM Teensy

> Mesures de consommation CPU effectuées sur **Teensy**.
> Chaque effet est mesuré en configuration **1 corde** et **6 cordes**.

---

## Delay

| Configuration | CPU (%) |
|---------------|---------|
| 1 corde       | 3       |
| 6 cordes      | 10      |

> **Note :** Le temps de Delay ne fait pas varier la consommation CPU.

---

## Distortion

### Hard Clip

| Configuration              | CPU (%) |
|-----------------------------|---------|
| 1 corde                    | 3       |
| 1 corde — Oversamplé (2×)  | 4       |
| 6 cordes                   | 9       |
| 6 cordes — Oversamplé (2×) | 15      |

### Soft Clip

| Configuration              | CPU (%) |
|-----------------------------|---------|
| 1 corde                    | 3       |
| 1 corde — Oversamplé (2×)  | 4       |
| 6 cordes                   | 10      |
| 6 cordes — Oversamplé (2×) | 16      |

### Fuzz

| Configuration              | CPU (%) |
|-----------------------------|---------|
| 1 corde                    | 3       |
| 1 corde — Oversamplé (2×)  | 4       |
| 6 cordes                   | 8       |
| 6 cordes — Oversamplé (2×) | 11      |

### Tube

| Configuration              | CPU (%) |
|-----------------------------|---------|
| 1 corde                    | 3       |
| 1 corde — Oversamplé (2×)  | 4       |
| 6 cordes                   | 9       |
| 6 cordes — Oversamplé (2×) | 14      |

### Multi

| Configuration              | CPU (%) |
|-----------------------------|---------|
| 1 corde                    | 3       |
| 1 corde — Oversamplé (2×)  | 5       |
| 6 cordes                   | 10      |
| 6 cordes — Oversamplé (2×) | 20      |

### Diode

| Configuration              | CPU (%) |
|-----------------------------|---------|
| 1 corde                    | 3       |
| 1 corde — Oversamplé (2×)  | 4       |
| 6 cordes                   | 9       |
| 6 cordes — Oversamplé (2×) | 15      |

### Disto DAFX

| Configuration              | CPU (%) |
|-----------------------------|---------|
| 1 corde                    | 3       |
| 1 corde — Oversamplé (2×)  | 4       |
| 6 cordes                   | 10      |
| 6 cordes — Oversamplé (2×) | 19      |

### OverDrive DAFX

| Configuration              | CPU (%) |
|-----------------------------|---------|
| 1 corde                    | 3       |
| 1 corde — Oversamplé (2×)  | 4       |
| 6 cordes                   | 9       |
| 6 cordes — Oversamplé (2×) | 14      |

### Récapitulatif Distortion — 6 cordes Oversamplé (2×)

| Algorithme     | CPU (%) |
|----------------|---------|
| Fuzz           | 11      |
| Tube           | 14      |
| OverDrive DAFX | 14      |
| Hard Clip      | 15      |
| Diode          | 15      |
| Soft Clip      | 16      |
| Disto DAFX     | 19      |
| Multi          | 20      |

---

## Octaver

| Configuration     | CPU (%) |
|--------------------|---------|
| 1 corde — Up 1    | 5       |
| 1 corde — Down 1  | 6       |
| 1 corde — Down 2  | 7       |
| 6 cordes — Up 1   | 23      |
| 6 cordes — Down 1 | 26      |
| 6 cordes — Down 2 | 35      |

> **Note :** L'Octaver est l'effet le plus coûteux en CPU (jusqu'à 35 % sur 6 cordes Down 2).

---

## Tremolo

| Configuration      | CPU (%) |
|---------------------|---------|
| 1 corde — Sin      | 3       |
| 1 corde — Triangle | 2       |
| 1 corde — Square   | 2       |
| 1 corde — Saw      | 2       |
| 6 cordes — Sin     | 10      |
| 6 cordes — Triangle| 5       |
| 6 cordes — Square  | 4       |
| 6 cordes — Saw     | 4       |

> **Note :** La forme d'onde Sin est significativement plus coûteuse que les autres formes d'onde.

---

## EQ (Égaliseur)

| Configuration | CPU (%) |
|---------------|---------|
| 1 corde       | 2       |
| 6 cordes      | 5       |

---

## Noise Gate

| Configuration | CPU (%) |
|---------------|---------|
| 1 corde       | 2       |
| 6 cordes      | 4       |

---

## Compressor

| Configuration | CPU (%) |
|---------------|---------|
| 1 corde       | 3       |
| 6 cordes      | 12      |

---

## Récapitulatif global — 6 cordes (sans oversampling)

| Effet               | CPU (%) |
|----------------------|---------|
| Noise Gate           | 4       |
| EQ                   | 5       |
| Tremolo (Saw)        | 4       |
| Tremolo (Sin)        | 10      |
| Distortion (Fuzz)    | 8       |
| Distortion (Multi)   | 10      |
| Delay                | 10      |
| Compressor           | 12      |
| Octaver (Up 1)       | 23      |
| Octaver (Down 2)     | 35      |
