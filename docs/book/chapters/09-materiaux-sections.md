# Chapitre 9 — Matériaux et propriétés de section

> **Objectifs**
>
> - Savoir définir un matériau élastique (E, G) et une section (A, Iy, Iz, J, Asy, Asz, Wt, fibres extrêmes).
> - Connaître les formules des sections prédéfinies : rectangle et cercle plein.
> - Mesurer la précision de $J$ et $W_t$ du rectangle par rapport aux séries exactes de Saint-Venant.
> - Comprendre le coefficient de cisaillement (5/6 pour le rectangle, Cowper pour le cercle).
>
> **Prérequis**
>
> - [Chapitre 6](06-euler-bernoulli.md), [chapitre 7](07-timoshenko.md) (rôle de Asy, Asz),
>   [chapitre 8](08-torsion-gauchissement.md) (rôle de J et Wt).

## 9.1 Matériau élastique

**Intuition.** Un matériau élastique linéaire isotrope est décrit par deux constantes : le module d'Young $E$
(traction, flexion) et le module de cisaillement $G$ (effort tranchant, torsion).

**Définition** ([`ElasticMaterial`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Materials/ElasticMaterial.h#L11-L22)) : un nom, $E$ et $G$,
dans les unités cohérentes du modèle ([chapitre 5](05-unites-signes.md)). $G$ est une donnée **indépendante** ; la
fabrique [`fromPoisson`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Materials/ElasticMaterial.cpp#L10-L15) le calcule par

$$
G = \frac{E}{2\,(1 + \nu)},\qquad -1 < \nu < 0{,}5 ,
$$

et refuse un $\nu$ hors de cet intervalle. [`validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Materials/ElasticMaterial.cpp#L17-L23) refuse $E$ ou
$G$ non finis ou ≤ 0 (`InvalidInput`).

**Exemple.** Les tests utilisent $E = 210\cdot10^6$ et $G = 81\cdot10^6$ kN/m², fournis directement
([`tests/TestModels.h`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/TestModels.h#L12-L12)), ce qui correspond à $\nu = E/(2G) - 1 = 0{,}2963$. Avec
`fromPoisson(…, 210e6, 0.3)`, on obtiendrait $G = 80{,}769\cdot10^6$ kN/m². Dans TSA, l'adaptateur calcule
$G = E/(2(1+\nu))$ avec le $\nu$ du matériau de TSA.

## 9.2 Données d'une section

[`BeamSection3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Sections/BeamSection3D.h#L18-L40), constante le long de l'élément, axes principaux
= axes locaux y et z :

| Champ | Symbole | Obligatoire | Usage |
|---|---|---|---|
| `area` | $A$ | oui | traction, $\sigma = N/A$ |
| `inertiaY` | $I_y$ | oui | flexion dans le plan x-z |
| `inertiaZ` | $I_z$ | oui | flexion dans le plan x-y |
| `torsionConstant` | $J$ | oui | torsion de Saint-Venant |
| `shearAreaY`, `shearAreaZ` | $A_{sy}$, $A_{sz}$ | Timoshenko seulement | $\Phi_y$, $\Phi_z$ ; $\tau_V = |V|/A_s$ |
| `yExtreme`, `zExtreme` | $y_{ext}$, $z_{ext}$ | non | contraintes de flexion |
| `torsionModulus` | $W_t$ | non | $\tau_T = |T|/W_t$ |

[`validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Sections/BeamSection3D.cpp#L66-L82) refuse toute valeur obligatoire non finie ou ≤ 0, et toute
valeur facultative fournie non finie ou ≤ 0. Les aires de cisaillement ne sont exigées qu'à la création d'un élément
de Timoshenko. Une donnée facultative absente n'est **jamais inventée** : la contrainte correspondante est simplement
absente des résultats ([`stressesAt`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L108-L122) ; pour $\tau_V$, $A$ remplace $A_s$
si l'aire de cisaillement manque).

## 9.3 Section rectangulaire

**Définition** ([`BeamSection3D::rectangle`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Sections/BeamSection3D.cpp#L23-L45)) : largeur $b$ selon y,
hauteur $h$ selon z.

$$
A = b\,h,\qquad I_y = \frac{b\,h^3}{12},\qquad I_z = \frac{h\,b^3}{12},\qquad
A_{sy} = A_{sz} = \frac{5}{6}A,\qquad y_{ext} = \frac{b}{2},\quad z_{ext} = \frac{h}{2}.
$$

**Torsion** (formules de Roark, côtés complets, grand côté $a = \max(b,h)$, petit côté $c = \min(b,h)$, $r = c/a$) :

$$
J = a\,c^3\left[\frac13 - 0{,}21\,r\left(1 - \frac{r^4}{12}\right)\right],\qquad
W_t = \frac{a\,c^2}{3\,\bigl(1 + 0{,}6095\,r + 0{,}8865\,r^2 - 1{,}8023\,r^3 + 0{,}9100\,r^4\bigr)} .
$$

**Exemple : section 0,30 × 0,50 m** (celle des tests). Recalcul Python :

| Propriété | Valeur |
|---|---|
| $A$ | 0,15 m² |
| $I_y$ | 3,125·10⁻³ m⁴ |
| $I_z$ | 1,125·10⁻³ m⁴ |
| $J$ | 2,817371·10⁻³ m⁴ |
| $W_t$ | 1,061211·10⁻² m³ |
| $A_{sy} = A_{sz}$ | 0,125 m² |

### Précision de J et Wt : écarts aux séries exactes

La solution exacte de Saint-Venant du rectangle s'exprime par des séries, que le test
`Section_RectangleTorsionMatchesExactSeries` somme lui-même (400 termes impairs) :

$$
J_{ex} = \beta\,a\,c^3,\quad \beta = \frac13\left[1 - \frac{192}{\pi^5}\frac{c}{a}\sum_{n\ \text{impair}} \frac{\tanh(n\pi a/2c)}{n^5}\right],
\qquad
W_{t,ex} = \frac{\beta\,a\,c^2}{k},\quad k = 1 - \frac{8}{\pi^2}\sum_{n\ \text{impair}} \frac{1}{n^2\cosh(n\pi a/2c)} .
$$

J'ai refait cette sommation en Python et comparé les formules du code ; « ancien $W_t$ » désigne la forme
$a c^2/(3 + 1{,}8\,r)$ utilisée avant la correction ([AUDIT.md](../../AUDIT.md), constat 2).

| a/c | écart J | écart Wt (code) | écart ancien Wt |
|---|---|---|---|
| 1,0 | +0,182 % | −0,150 % | +0,081 % |
| 1,2 | −0,453 % | +0,207 % | +1,502 % |
| 1,5 | −0,063 % | −0,199 % | +3,085 % |
| 2,0 | +0,087 % | −0,168 % | +4,283 % |
| 2,5 | +0,059 % | +0,035 % | +4,359 % |
| 3,0 | +0,034 % | +0,106 % | +3,956 % |
| 4,0 | +0,013 % | +0,022 % | +2,907 % |
| 10 | +0,003 % | −0,079 % | +0,685 % |
| 20 | +0,001 % | +0,006 % | +0,246 % |

Sur un balayage de a/c de 1 à 50 (pas de 0,01) : $|\text{écart } J| \le 0{,}49\ \%$, $|\text{écart } W_t| \le 0{,}27\ \%$,
et l'ancien $W_t$ atteignait **+4,4 %** (vers a/c ≈ 2,3). Ces valeurs confirment celles d'[AUDIT.md](../../AUDIT.md)
(J ≤ 0,5 %, Wt ≤ 0,3 %, ancienne forme jusqu'à 4,4 %).

**Interprétation.** Un $W_t$ surestimé donne une contrainte $\tau = T/W_t$ sous-estimée : l'ancienne forme n'était pas
du côté de la sécurité (jusqu'à environ 4 % sur $\tau$). Le polynôme actuel reste à moins de 0,3 %, dans les deux sens.
Pour la section 0,30 × 0,50 m (a/c = 5/3) : $J$ à +0,04 %, $W_t$ à −0,26 % de la valeur exacte.

**Limite.** L'audit indépendant annonce un écart sur $J$ « ≤ 0,18 % » ([INDEPENDENT_FORMULATION_AUDIT.md](../../verification/INDEPENDENT_FORMULATION_AUDIT.md),
ligne D8) d'après quatre rapports seulement ; à a/c = 1,2, l'écart vaut −0,45 %, toujours dans la borne de 0,5 %.

## 9.4 Section circulaire pleine et coefficient de Cowper

**Définition** ([`BeamSection3D::circle`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Sections/BeamSection3D.cpp#L47-L64)), diamètre $d$ :

$$
A = \frac{\pi d^2}{4},\quad I_y = I_z = \frac{\pi d^4}{64},\quad J = \frac{\pi d^4}{32}\ (\text{exact}),\quad
W_t = \frac{\pi d^3}{16}\ (\text{exact}),\quad y_{ext} = z_{ext} = \frac d2 .
$$

**Aire de cisaillement** : $A_{sy} = A_{sz} = k\,A$ avec le coefficient attribué à Cowper

$$
k = \frac{6\,(1 + \nu)}{7 + 6\,\nu},\qquad -1 < \nu < 0{,}5\ \ (\nu = 0{,}3\ \text{par défaut}).
$$

**Intuition.** L'effort tranchant ne se répartit pas uniformément sur la section : l'aire « efficace » qui donne la bonne
énergie de cisaillement est plus petite que $A$. Le coefficient dépend de la forme de la section et, selon la théorie
retenue, du coefficient de Poisson.

**Exemple vérifiable** (recalcul Python, test `Section_CircleShearCoefficientUsesPoisson`) : $k(0{,}3) = 0{,}886364$,
$k(0{,}49) = 0{,}899396$ ; $\nu = 0{,}5$ est refusé. Pour $d = 0{,}3$ m : $A = 0{,}070686$ m², $J = 7{,}952156\cdot10^{-4}$ m⁴,
$W_t = 5{,}301438\cdot10^{-3}$ m³, $A_s = 0{,}062653$ m².

**Limites.**

- Le wiki d'OpenSees conseille 0,9 A pour le cercle plein : la souplesse d'effort tranchant diffère alors de
  0,9/0,886 − 1 ≈ 1,5 % (recalcul ; les documents du dépôt disent « environ 1,6 % »). Les benchmarks passent les mêmes
  aires aux deux codes.
- Le code mélange deux origines : 5/6 pour le rectangle, Cowper pour le cercle. La formule de Cowper du rectangle,
  $10(1+\nu)/(12+11\nu)$, donnerait 0,850 pour $\nu = 0{,}3$, soit 2 % de moins que 5/6 ; ce n'est pas une erreur,
  plusieurs définitions coexistent (audit indépendant §5.4).
- La section ne connaît pas le matériau : il faut passer à `circle` le $\nu$ du matériau, sinon 0,3 est utilisé.

## 9.5 Sections quelconques

Toute autre section se définit en remplissant directement la structure (c'est ce que fait TSA, qui transmet $A$,
$I_y$, $I_z$ et $J$ de ses sections, sans aires de cisaillement, sans $W_t$ ni fibres extrêmes :
[`convert`](https://github.com/ChristinotLeonnel/Tsaraloha-Structural-Analysis/blob/2f5d7d4342e903fe536fbab91a7dcf80e8b89f7d/src/Analysis/Engines/Fem3D/Fem3DAdapter.cpp#L38-L189)). L'utilisateur est alors responsable de la
cohérence des valeurs, notamment de $J$ (qui n'est pas $I_y + I_z$, [chapitre 8](08-torsion-gauchissement.md)) et de
l'orientation des axes principaux.

## À retenir

- Matériau : $E$ et $G$ indépendants ; `fromPoisson` donne $G = E/(2(1+\nu))$.
- Rectangle : $J$ et $W_t$ par les formules de Roark ; écarts aux séries exactes ≤ 0,5 % ($J$) et ≤ 0,3 % ($W_t$) ;
  l'ancien $W_t$ (jusqu'à +4,4 %) a été corrigé.
- Aires de cisaillement : 5/6 A (rectangle), $6(1+\nu)/(7+6\nu)\,A$ (cercle, Cowper).
- Données facultatives absentes ⇒ contraintes absentes, jamais inventées.

## Dans le code source

- [`ElasticMaterial`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Materials/ElasticMaterial.h#L11-L22),
  [`fromPoisson`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Materials/ElasticMaterial.cpp#L10-L15), [`validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Materials/ElasticMaterial.cpp#L17-L23).
- [`BeamSection3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Sections/BeamSection3D.h#L18-L40),
  [`rectangle`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Sections/BeamSection3D.cpp#L23-L45), [`circle`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Sections/BeamSection3D.cpp#L47-L64),
  [`validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Sections/BeamSection3D.cpp#L66-L82).
- Contraintes : [`stressesAt`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Results/BeamResponse.cpp#L108-L122).

## Vérifier par les tests

```bat
build\fem3d_tests.exe | findstr /C:"Section_" /C:"Input_Invalid"
```

- [`Section_RectangleTorsionMatchesExactSeries`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L130-L152) : $J$ et $W_t$
  contre les séries exactes, tolérance 0,6 % fixée avant l'essai.
- [`Section_CircleShearCoefficientUsesPoisson`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp#L183-L192) : coefficient de
  Cowper, refus de $\nu = 0{,}5$.
- [`Input_InvalidPropertiesRejected`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp#L121-L134) : E ≤ 0, J nul,
  Timoshenko sans aire de cisaillement.
- Cas OpenSees : sections `R30x50`, `R20x40`, `D300` ([`base`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/benchmarks/OpenSees/opensees_benchmarks.cpp#L66-L74)).

## Références

- [REFERENCES.md](../../references/REFERENCES.md) §C2 et §C3 ; [AUDIT.md](../../AUDIT.md) constats 2 et 4.
- A13 — *Roark's Formulas for Stress and Strain* : notice vérifiée ; formule de $J$ vue dans un extrait secondaire de
  la 7ᵉ éd. (Table 10.1), équivalence algébrique avec le code vérifiée ; coefficients du polynôme de $W_t$ à vérifier
  sur l'ouvrage imprimé. La précision a été vérifiée indépendamment par les séries exactes.
- B2 — Cowper (1966) : notice vérifiée (Crossref) ; l'attribution de la formule du cercle vient de la littérature
  secondaire, texte non lu : à vérifier. B3 — Hutchinson (2001) : notice vérifiée.
- A9 — Timoshenko, Gere, *Mechanics of Materials* (contraintes de RDM) : notice vérifiée ; chapitres à vérifier.
- A14, A15 — Pilkey : notices vérifiées ; contenu à vérifier.

---

Chapitre précédent : [Chapitre 8 — Torsion de Saint-Venant et gauchissement](08-torsion-gauchissement.md) ·
Chapitre suivant : [Chapitre 10 — Charges et conditions aux limites](10-charges-conditions-limites.md)
