# Chapitre 5 — Conventions d'unités et de signes

> **Objectifs**
>
> - Choisir un système d'unités cohérent : le moteur ne convertit rien.
> - Connaître la convention de signe de chaque grandeur : déplacements, rotations, efforts d'extrémité, efforts
>   internes, réactions, forces des ressorts, contraintes.
> - Comprendre l'origine du signe $\theta_y = -\,\mathrm{d}w/\mathrm{d}x$.
>
> **Prérequis**
>
> - [Chapitre 3](03-modele-physique.md) (DDL) et [chapitre 4](04-reperes.md) (repère local).

## 5.1 Unités : un système cohérent, aucune conversion

**Intuition.** Une équation comme $w = FL^3/(3EI)$ ne donne un résultat juste que si $F$, $L$, $E$ et $I$ sont exprimés
dans des unités compatibles. Fem3D ne fait aucune hypothèse : il calcule avec les nombres qu'on lui donne.

**Règle.** Toutes les données d'un modèle sont exprimées dans un même système cohérent, choisi par l'appelant et
déclaré dans `UnitSystem` pour l'export JSON ([UNITS_AND_COORDINATES.md](../../theory/UNITS_AND_COORDINATES.md) §1).

| Grandeur | Système des tests et des exemples | Autre système cohérent |
|---|---|---|
| Longueur, déplacement | m | mm |
| Force | kN | N |
| Moment | kN·m | N·mm |
| E, G, contrainte | kN/m² | N/mm² (MPa) |
| Aire ; inertie, J | m² ; m⁴ | mm² ; mm⁴ |
| Charge répartie | kN/m | N/mm |
| Raideur de ressort | kN/m ; kN·m/rad | N/mm ; N·mm/rad |
| Rotation | rad | rad |

**Exemple vérifiable.** La console du [chapitre 1](01-presentation.md) (L = 3 m, F = 20 kN, E = 210·10⁶ kN/m²,
$I_y = 3{,}125\cdot10^{-3}$ m⁴) donne $w = 2{,}742857\cdot10^{-4}$ m. Exprimée en N et mm (L = 3000 mm,
F = 20 000 N, E = 210 000 N/mm², $I_y = 3{,}125\cdot10^{9}$ mm⁴), elle donne $w = 0{,}2742857$ mm : la même flèche,
au facteur 1000 près (recalculé avec Python). Le test `Solver_MechanismCriterionIndependentOfUnits` calcule un même
modèle en kN-m et en N-mm et vérifie que flèche et rotation coïncident aux conversions près, critère de mécanisme
compris.

**Dans TSA**, l'adaptateur calcule en kN et m : E est converti de Pa en kN/m² (× 10⁻³) et G = E / (2 (1 + ν))
([ligne 95](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DAdapter.cpp#L95-L95)).

**Limite.** Une incohérence d'unités (par exemple E en MPa avec des longueurs en m) n'est pas détectable par le moteur :
les résultats seraient faux sans message.

## 5.2 Signes des déplacements et des rotations

- Déplacements nodaux : composantes globales, positives selon $+X, +Y, +Z$.
- Rotations : vecteurs, positives selon la règle de la main droite autour de chaque axe.
- Le long d'une barre : $u, v, w$ et $\theta_x, \theta_y, \theta_z$ en repère local.

**Le signe moins du plan x-z.** Dans le plan x-y, une rotation positive $\theta_z$ (autour de $+z$) fait monter $v$
quand x croît : $\theta_z = \mathrm{d}v/\mathrm{d}x$. Dans le plan x-z, une rotation positive $\theta_y$ (autour de $+y$)
amène l'axe $+x$ vers $-z$ (puisque $\mathbf{y}\times\mathbf{z} = \mathbf{x}$, $\mathbf{z}\times\mathbf{x} = \mathbf{y}$) :
elle fait **descendre** $w$. D'où

$$
\theta_z = \frac{\mathrm{d}v}{\mathrm{d}x},\qquad \theta_y = -\,\frac{\mathrm{d}w}{\mathrm{d}x}\qquad\text{(Euler–Bernoulli)}.
$$

Ce signe se retrouve dans les termes croisés $w$-$\theta_y$ de la matrice de rigidité ([chapitre 6](06-euler-bernoulli.md))
et dans les fonctions de forme du plan x-z, de signe opposé à celles du plan x-y
([`EulerBernoulli3D::shapeFunctions`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Elements/BeamElements.cpp#L62-L75)). En Timoshenko, $\theta_y$ est la
rotation de la section, distincte de la pente ([chapitre 7](07-timoshenko.md)).

## 5.3 Efforts d'extrémité

Les **efforts d'extrémité** (`endForcesLocal`, 12 valeurs) sont les forces et moments exercés **sur l'élément** par
les nœuds i et j, dans le repère local :

$$
\mathbf{f} = \mathbf{k}_e\,\mathbf{d}_e - \mathbf{f}_{eq}.
$$

C'est la convention de `localForce` d'OpenSees. Aux rotules, les moments relâchés valent exactement 0
([`BeamResponse::BeamResponse`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L53-L98)).

## 5.4 Efforts internes

Les **efforts internes** $N, V_y, V_z, T, M_y, M_z$ sont les résultantes exercées sur la **face positive** de la coupe
(normale $+x$ local), dans le repère local ([SIGN_CONVENTIONS.md](../../theory/SIGN_CONVENTIONS.md) §3).

| Effort | Positif si | Relation |
|---|---|---|
| $N$ | traction | $\mathrm{d}N/\mathrm{d}x = -q_x$ |
| $V_y$, $V_z$ | dirigé selon $+y$, $+z$ sur la face positive | $\mathrm{d}V_y/\mathrm{d}x = -q_y$, $\mathrm{d}V_z/\mathrm{d}x = -q_z$ |
| $T$ | moment autour de $+x$ | constant (pas de couple réparti) |
| $M_y$, $M_z$ | moment autour de $+y$, $+z$ | $\mathrm{d}M_z/\mathrm{d}x = -V_y$, $\mathrm{d}M_y/\mathrm{d}x = +V_z$ |

Ils sont obtenus par l'équilibre du tronçon $[0, x]$ à partir des efforts d'extrémité en i, par exemple
$N(x) = -f_{ix} - \int_0^x q_x$, $T = -m_{ix}$, $M_y(x) = -m_{iy} - x\,f_{iz} - \int_0^x (x-s)\,q_z(s)\,\mathrm{d}s$.

**Exemple vérifiable** (test `Analytical_CantileverTipLoad_BothPlanesBothFormulations`) : console selon X, L = 3 m,
encastrée en i, charge en bout.

- Charge $F_z = +20$ kN : $V_z = +20$ kN ; $M_y(x) = -F(L - x)$, soit $M_y(0) = -60$ kN·m ;
  $\theta_{y,j} = -FL^2/(2EI_y) = -1{,}371429\cdot10^{-4}$ rad.
- Charge $F_y = +20$ kN : $V_y = +20$ kN ; $M_z(0) = +FL = +60$ kN·m ; $\theta_{z,j} = +FL^2/(2EI_z) = 3{,}809524\cdot10^{-4}$ rad.
- Efforts d'extrémité correspondants en i pour $F_z$ : $f_{iz} = -20$ kN et $m_{iy} = +60$ kN·m. Vérification :
  le moment en i de la charge, $(L, 0, 0)\times(0, 0, 20) = (0, -60, 0)$, est bien équilibré par $+60$.

Valeurs recalculées avec Python ; le test les vérifie à 10⁻⁹ près.

## 5.5 Réactions, forces des ressorts, équilibre

- **Réactions** : forces exercées **par l'appui sur la structure**, en repère global :
  $\mathbf{R} = \mathbf{K}_{rf}\mathbf{U}_f + \mathbf{K}_{rr}\mathbf{U}_r - \mathbf{F}_r$. Elles valent 0 sur les DDL
  libres d'un nœud d'appui. Exemple : traction P = 100 kN en bout de console, réaction $R_x = -100$ kN.
- **Forces des ressorts** : exercées **par le ressort sur la structure**, en repère global : $\mathbf{F} = -k\,u$
  ([chapitre 11](11-ressorts.md)).
- **Équilibre global** (contrôlé après chaque calcul, moments pris par rapport à l'origine) :
  $\sum \mathbf{F}_{appliquées} + \sum \mathbf{R} + \sum \mathbf{F}_{ressorts} = \mathbf{0}$
  ([`checkEquilibrium`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L124-L170)).

## 5.6 Contraintes

Calculées en valeur absolue pour la flexion et le cisaillement
([`stressesAt`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L108-L122)) : $\sigma = N/A$ (signée) ;
$\sigma_{flexion} = |M_y|\,z_{ext}/I_y$ et $|M_z|\,y_{ext}/I_z$ ; $\tau_V = |V|/A_s$ (contrainte de calcul ; la moyenne
vraie vaut $|V|/A$) ; $\tau_T = |T|/W_t$. Une valeur est absente si la donnée de section manque
([chapitre 9](09-materiaux-sections.md)).

## À retenir

- Aucune conversion d'unités : un système cohérent, choisi par l'utilisateur (kN, m dans les tests et dans TSA).
- $\theta_z = v'$ mais $\theta_y = -w'$ : conséquence de la règle de la main droite.
- Efforts d'extrémité : sur l'élément, $\mathbf{f} = \mathbf{k}\mathbf{d} - \mathbf{f}_{eq}$ ; efforts internes : sur la face
  positive ; réactions et forces de ressort : sur la structure.
- Console chargée par $+F_z$ : $V_z = +F$, $M_y(0) = -FL$ ; par $+F_y$ : $V_y = +F$, $M_z(0) = +FL$.

## Dans le code source

- Efforts d'extrémité, efforts internes, déformée : [`BeamResponse::BeamResponse`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L53-L98).
- Résultantes des efforts internes : [`SectionForces`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/AnalysisResults.h#L39-L47).
- Réactions et forces des ressorts : [`LinearStaticAnalysis::run`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L40-L115).
- Équilibre : [`checkEquilibrium`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L124-L170).
- Contraintes : [`stressesAt`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L108-L122).

## Vérifier par les tests

```bat
build\fem3d_tests.exe | findstr /C:"CantileverTipLoad" /C:"AxialTension" /C:"IndependentOfUnits"
```

- [`Analytical_CantileverTipLoad_BothPlanesBothFormulations`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L75-L100) :
  signes de $\theta_y$, $\theta_z$, $M_y$, $M_z$, $V_y$, $V_z$.
- [`Analytical_AxialTension`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp#L52-L63) : $N = +P$, $R = -P$.
- [`Solver_MechanismCriterionIndependentOfUnits`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L97-L128) : même modèle en
  kN-m et en N-mm.
- Cas OpenSees `03_flexion_y` et `04_flexion_z` (convention `localForce` comparée).

## Références

- [SIGN_CONVENTIONS.md](../../theory/SIGN_CONVENTIONS.md), [UNITS_AND_COORDINATES.md](../../theory/UNITS_AND_COORDINATES.md)
  (sources du dépôt ; signes vérifiés par l'audit indépendant, [INDEPENDENT_FORMULATION_AUDIT.md](../../verification/INDEPENDENT_FORMULATION_AUDIT.md)).
- A9 — Timoshenko, Gere, *Mechanics of Materials* : notice vérifiée ; chapitres à vérifier.

---

Chapitre précédent : [Chapitre 4 — Repères locaux et globaux](04-reperes.md) ·
Chapitre suivant : [Chapitre 6 — Élément poutre 3D Euler–Bernoulli](06-euler-bernoulli.md)
