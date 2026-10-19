# Validation du moteur Fem3D contre OpenSees

**État au 19/10/2026** : 20 cas × 2 formulations ont été comparés à **OpenSees 3.8.0** (`TSA/thirdparty/OpenSees`),
et la comparaison a bien été exécutée. Les cas 12 à 17 portent sur les rotules, les cas 18 à 20 sur les appuis
élastiques (ressorts).

- **2 657 valeurs comparées, 0 hors tolérance.**
- Les deux cas de mécanisme sont détectés par les deux codes.
- Un cas est non comparable (16, Timoshenko) ; voir le §5.
- Les 39 tests unitaires, analytiques et d'intégration réussissent (`fem3d_tests`).

Cette validation porte sur la **statique linéaire de poutres prismatiques élastiques**. Elle ne vaut pas pour
d'autres domaines (non-linéarité, dynamique, gauchissement), qui ne sont pas implémentés.

## 1. Références et correspondance des éléments

| TSA Fem3D | OpenSees | Commentaire |
|---|---|---|
| `EulerBernoulli3D` | `elasticBeamColumn` (3D) | Rigidité exacte de poutre élancée, cisaillement transversal négligé, torsion GJ/L : même formulation (vérifiée, non supposée). |
| `Timoshenko3D` | `ElasticTimoshenkoBeam` (3D) | Φ = 12EI/(G·Av·L²) ; `Avy = Asy`, `Avz = Asz` : aires effectives de la section TSA (5/6 A pour un rectangle, coefficient de Cowper pour le cercle). |
| repère local, `v_xz` | `geomTransf Linear $tag vecxzX vecxzY vecxzZ` | Même définition : y = v_xz × x, z = x × y. Le script reçoit le `v_xz` réellement retenu par TSA pour chaque élément. |
| efforts d'extrémité | `eleResponse $e localForce` | Même convention (forces exercées sur l'élément, repère local), vérifiée sur un cas de traction : N_i = −P. |
| charge répartie | `eleLoad -beamUniform Wy Wz Wx` | Uniforme, en composantes locales. Une charge globale TSA est projetée par la même rotation R. |

Les deux calculs partagent les mêmes géométries, matériaux (E = 210·10⁶ kN/m², G = 81·10⁶ kN/m²), sections, appuis,
charges et unités (kN, m). Côté OpenSees, l'analyse est `Static` / `Linear` / `LoadControl 1.0`.

## 2. Cas de référence

- Scripts : `benchmarks/OpenSees/cases/*.tcl`.
- Sorties brutes d'OpenSees : `results/*_opensees.txt`.
- Détail valeur par valeur (TSA, OpenSees, écarts, statut) : `results/*_comparaison.csv`.

| # | Cas | Contenu |
|---|---|---|
| 01 | Traction axiale | Console L = 4 m, Fx en bout |
| 02 | Torsion | Console, couple Mx en bout |
| 03 | Flexion autour de y local | Console L = 3 m, Fz en bout (Iy) |
| 04 | Flexion autour de z local | Console L = 3 m, Fy en bout (Iz ≠ Iy) |
| 05 | Console sous charges transversales | Fy, Fz, My et Mz en bout, plus une charge répartie locale qz |
| 06 | Poutre courte L/h = 2 | Cisaillement significatif : Timoshenko dépasse Euler–Bernoulli de 7 % en Uy et de 19 % en Uz, chacun d'accord avec OpenSees |
| 07 | Barre inclinée dans l'espace | Orientation v_xz imposée non triviale, charge répartie globale et charge nodale |
| 08 | Portique spatial | 4 poteaux de hauteurs différentes, 4 poutres de portées différentes, deux sections |
| 09 | Charges réparties locales et globales | Portique 08 avec charges locales (qx, qy, qz) et globales projetées |
| 10 | Géométrie irrégulière | Arbalétriers inclinés, contreventement, trois sections dont un cercle, appui partiel |
| 11 | Mécanisme | Rotation autour de l'axe libre aux deux appuis : les deux codes doivent refuser le calcul |
| 12 | Poutre bi-articulée | Rotules My et Mz aux deux extrémités entre deux nœuds encastrés, charges réparties qy et qz |
| 13 | Rotule à une extrémité | Rotule Mz au nœud j, barre selon Y, charge globale répartie : console sur appui |
| 14 | Portique à poutres articulées | Portique spatial rectangulaire, poutres articulées (My, Mz) aux deux extrémités |
| 15 | Rotule intermédiaire | Rotule (My, Mz) entre deux consoles (Gerber), charges nodale et répartie |
| 16 | Barre inclinée articulée | Rotule My au nœud i d'une barre inclinée quelconque |
| 17 | Mécanisme par rotule | Console relâchée (My) au droit de son encastrement : les deux codes doivent refuser |
| 18 | Ressorts de translation | Console dont le bout repose sur deux ressorts (ky, kz), charges nodales et répartie |
| 19 | Ressorts de rotation | Poteau à pied articulé retenu par trois ressorts de rotation (krx, kry, krz), charges spatiales |
| 20 | Portique sur appuis élastiques | Portique 09 : trois pieds articulés avec ressorts de rotation, un pied sans appui rigide, sur six ressorts |

### Références pour les rotules

- **Euler–Bernoulli** : options natives `-releasey` et `-releasez` d'`elasticBeamColumn`, valables quelle que soit
  l'orientation. Codes : 1 = nœud i, 2 = nœud j, 3 = les deux. Une sonde les a vérifiées : moment d'encastrement de
  la console sur appui qL²/8 et effort 5qL/8.
- **Timoshenko** : `ElasticTimoshenkoBeam` n'a pas d'option de relâchement. On dédouble le nœud à l'extrémité de la
  barre, et le nœud dédoublé est relié par `equalDOF` sur tous les DDL, sauf la rotation globale de l'axe local
  relâché. Ce montage est **exact si l'axe local relâché est un axe global** (cas 12 à 15) ; sinon, le cas est
  « non comparable » (16). La rotation de ce nœud dédoublé est la rotation de la barre à la rotule : elle est comparée
  à celle que reconstitue TSA (24 valeurs, toutes conformes).

### Référence pour les appuis élastiques

Chaque ressort nodal de TSA est modélisé dans OpenSees par un élément `zeroLength` entre un nœud fixe doublé
(200000 + nœud, encastré) et le nœud, avec un matériau `uniaxialMaterial Elastic` par DDL retenu (`-dir` 1 à 6).
L'orientation par défaut de `zeroLength` est celle des axes globaux, comme les ressorts de TSA. La force du ressort sur
la structure est la réaction du nœud fixe doublé (`nodeReaction`) : elle est comparée à −k·u de TSA, avec les mêmes
tolérances que les réactions (grandeur « ressort » des fichiers CSV).

## 3. Tolérances (fixées avant les essais)

`|TSA − OpenSees| ≤ max(1e-6 · |OpenSees|, 1e-9 · échelle)`. L'échelle est la plus grande valeur absolue de même
nature dans le cas : translations, rotations, forces ou moments.

- **Formulations identiques et exactes** pour une poutre prismatique : l'écart attendu se réduit à l'arrondi. Les
  deux codes factorisent différemment (OpenSees : `BandGeneral` avec numérotation `RCM` ; TSA : LDLᵀ creuse) et les
  sorties sont écrites à 17 chiffres significatifs.
- La valeur 1e-6 est donc large, mais 10⁴ à 10⁶ fois plus petite que l'effet d'une erreur de formulation : un signe,
  un Φ ou un axe faux produit un écart de 1 à 100 %.
- **Près de zéro**, on applique un critère absolu (1e-9 de l'échelle), car l'écart relatif d'une composante
  théoriquement nulle est instable.
- **Plancher des échelles** : une grandeur toute nulle, comme les moments d'une barre bi-articulée, aurait une
  échelle nulle. Ses planchers sont donc dimensionnellement cohérents : échelle des moments ≥ échelle des forces ×
  plus grande longueur d'élément, échelle des rotations ≥ échelle des translations / plus grande longueur.

## 4. Résultats mesurés (OpenSees 3.8.0)

| Cas | Formulation | Statut | Valeurs | Hors tolérance | Écart absolu max | Écart relatif max |
|---|---|---|---|---|---|---|
| Traction axiale (console, L = 4 m) | Euler-Bernoulli | OK | 30 | 0 | 1.69e-21 | 1.33e-16 |
| Traction axiale (console, L = 4 m) | Timoshenko | OK | 30 | 0 | 1.69e-21 | 1.33e-16 |
| Torsion de Saint-Venant (console, L = 4 m) | Euler-Bernoulli | OK | 30 | 0 | 1.78e-15 | 1.48e-16 |
| Torsion de Saint-Venant (console, L = 4 m) | Timoshenko | OK | 30 | 0 | 1.78e-15 | 1.48e-16 |
| Flexion autour de y local (console, charge Fz) | Euler-Bernoulli | OK | 30 | 0 | 4.97e-14 | 8.29e-16 |
| Flexion autour de y local (console, charge Fz) | Timoshenko | OK | 30 | 0 | 2.84e-14 | 7.11e-16 |
| Flexion autour de z local (console, charge Fy) | Euler-Bernoulli | OK | 30 | 0 | 3.55e-14 | 7.11e-16 |
| Flexion autour de z local (console, charge Fy) | Timoshenko | OK | 30 | 0 | 3.55e-14 | 5.92e-16 |
| Console sous charges transversales combinées et charge répartie locale | Euler-Bernoulli | OK | 30 | 0 | 4.09e-14 | 8.17e-15 |
| Console sous charges transversales combinées et charge répartie locale | Timoshenko | OK | 30 | 0 | 2.84e-14 | 5.68e-16 |
| Poutre courte L/h = 2 (cisaillement significatif) | Euler-Bernoulli | OK | 30 | 0 | 5.68e-14 | 1.14e-15 |
| Poutre courte L/h = 2 (cisaillement significatif) | Timoshenko | OK | 30 | 0 | 1.42e-14 | 4.74e-16 |
| Barre inclinée dans l'espace, orientation v_xz imposée | Euler-Bernoulli | OK | 30 | 0 | 2.02e-12 | 1.51e-13 |
| Barre inclinée dans l'espace, orientation v_xz imposée | Timoshenko | OK | 30 | 0 | 2.34e-12 | 2.27e-13 |
| Portique spatial multiéléments (hauteurs et portées différentes) | Euler-Bernoulli | OK | 168 | 0 | 4.18e-13 | 7.30e-14 |
| Portique spatial multiéléments (hauteurs et portées différentes) | Timoshenko | OK | 168 | 0 | 3.41e-13 | 9.55e-14 |
| Portique spatial, charges réparties locales et globales | Euler-Bernoulli | OK | 168 | 0 | 2.35e-13 | 6.06e-13 |
| Portique spatial, charges réparties locales et globales | Timoshenko | OK | 168 | 0 | 4.48e-13 | 2.62e-13 |
| Géométrie irrégulière : arbalétriers inclinés, contreventement, sections différentes | Euler-Bernoulli | OK | 186 | 0 | 2.98e-13 | 8.15e-13 |
| Géométrie irrégulière : arbalétriers inclinés, contreventement, sections différentes | Timoshenko | OK | 186 | 0 | 4.55e-13 | 1.04e-12 |
| Rotules My et Mz aux deux extrémités entre nœuds encastrés, charges réparties | Euler-Bernoulli | OK | 36 | 0 | 1.42e-14 | 5.34e-16 |
| Rotules My et Mz aux deux extrémités entre nœuds encastrés, charges réparties | Timoshenko | OK | 40 | 0 | 1.42e-14 | 5.34e-16 |
| Rotule Mz au nœud j seulement (barre selon Y), console sur appui | Euler-Bernoulli | OK | 36 | 0 | 7.11e-15 | 2.84e-16 |
| Rotule Mz au nœud j seulement (barre selon Y), console sur appui | Timoshenko | OK | 37 | 0 | 7.11e-15 | 2.85e-16 |
| Portique spatial rectangulaire, poutres articulées (My, Mz) aux deux extrémités | Euler-Bernoulli | OK | 168 | 0 | 1.62e-13 | 2.63e-14 |
| Portique spatial rectangulaire, poutres articulées (My, Mz) aux deux extrémités | Timoshenko | OK | 184 | 0 | 4.25e-13 | 5.75e-14 |
| Rotule intermédiaire (Gerber) entre deux consoles, charges nodale et répartie | Euler-Bernoulli | OK | 54 | 0 | 2.84e-14 | 9.98e-16 |
| Rotule intermédiaire (Gerber) entre deux consoles, charges nodale et répartie | Timoshenko | OK | 56 | 0 | 4.26e-14 | 9.95e-16 |
| Barre inclinée dans l'espace, rotule My au nœud i | Euler-Bernoulli | OK | 54 | 0 | 1.21e-13 | 3.93e-14 |
| Barre inclinée dans l'espace, rotule My au nœud i | Timoshenko | NON COMPARABLE | 0 | 0 | 0.00e+00 | 0.00e+00 |
| Console sur appui élastique en bout (ky, kz), charges nodale et répartie | Euler-Bernoulli | OK | 36 | 0 | 2.13e-14 | 7.67e-15 |
| Console sur appui élastique en bout (ky, kz), charges nodale et répartie | Timoshenko | OK | 36 | 0 | 2.84e-14 | 7.67e-15 |
| Poteau sur pied articulé retenu par trois ressorts de rotation, charges spatiales | Euler-Bernoulli | OK | 36 | 0 | 5.83e-13 | 1.47e-14 |
| Poteau sur pied articulé retenu par trois ressorts de rotation, charges spatiales | Timoshenko | OK | 36 | 0 | 5.90e-13 | 1.50e-14 |
| Portique spatial sur appuis élastiques (pieds semi-rigides, un pied entièrement sur ressorts) | Euler-Bernoulli | OK | 192 | 0 | 1.17e-12 | 9.64e-13 |
| Portique spatial sur appuis élastiques (pieds semi-rigides, un pied entièrement sur ressorts) | Timoshenko | OK | 192 | 0 | 6.82e-13 | 1.57e-12 |
| Mécanisme : console relâchée (My) au droit de son encastrement | Euler-Bernoulli | OK | 0 | 0 | 0.00e+00 | 0.00e+00 |
| Mécanisme : console relâchée (My) au droit de son encastrement | Timoshenko | OK | 0 | 0 | 0.00e+00 | 0.00e+00 |
| Mécanisme : rotation autour de l'axe libre aux deux appuis | Euler-Bernoulli | OK | 0 | 0 | 0.00e+00 | 0.00e+00 |
| Mécanisme : rotation autour de l'axe libre aux deux appuis | Timoshenko | OK | 0 | 0 | 0.00e+00 | 0.00e+00 |

- Écarts relatifs maximaux : 1e-16 à 1e-12, soit la précision machine.
- Résidus d'équilibre global de TSA : inférieurs à 10⁻¹¹ (kN et kN·m) dans tous les cas.
- Pour régénérer : `build/fem3d_opensees_benchmarks [OpenSees.exe] [dossier]`. La synthèse est écrite dans
  `benchmarks/OpenSees/results/synthese.md` et `synthese.csv`.

## 5. Problèmes identifiés

- **Charges trapézoïdales non comparables** : OpenSees 3.8.0 accepte la syntaxe `-beamUniform` à 8 arguments en 3D
  mais **ignore** la variation, sans message. La sonde du 19/10/2026 donne les mêmes efforts qu'avec une charge
  uniforme. Le générateur refuse donc ce type de charge (statut « NON COMPARABLE »).
  - Les charges linéaires de TSA sont validées autrement : analytiquement
    (`Analytical_FixedFixedTriangularLoad_EulerBernoulli` : wL²/30, wL²/20, 3wL/20 et 7wL/20) et par la cohérence de
    la déformée.
  - Elles ne sont **pas encore comparées à OpenSees**.
- **Réactions avec equalDOF (référence Timoshenko des rotules)** : `nodeReaction` du nœud d'appui maître n'inclut
  pas l'effort transmis par la contrainte au nœud dédoublé esclave. Au premier essai, 9 valeurs de réaction
  semblaient donc en écart dans les cas 12 et 13, là où un nœud dédoublé coïncide avec un appui. La réaction d'appui
  réelle étant la somme maître + esclave, c'est cette somme qui est désormais extraite. Aucun résultat de référence
  n'a été modifié.
- **Mécanisme 17 dans OpenSees** : OpenSees ne signale pas d'échec (`analyze` = 0) mais rend une solution absurde
  (Uz ≈ 6·10¹¹ m). Le critère « solution non bornée » (|u| > 10⁶) le détecte. TSA, lui, refuse le calcul et désigne
  les DDL du mécanisme (nœud 2, Uz et Ry).
- **Deux défauts du moteur, corrigés pendant le développement et désormais testés** :
  - un modèle sans DDL libre (poutre bi-encastrée en un seul élément) était refusé à tort ;
  - la factorisation ne désignait pas les DDL d'un mécanisme dont le pivot est exactement nul.

## 6. Limites restantes

- **Non comparé à OpenSees** :
  - rotule de Timoshenko sur un axe local non parallèle à un axe global (cas 16 : validé seulement en
    Euler–Bernoulli, et analytiquement en Timoshenko par la condensation commune) ;
  - charges trapézoïdales ;
  - déplacements imposés non nuls (validés analytiquement) ;
  - contraintes : OpenSees ne les calcule pas pour ces éléments ; les formules de résistance des matériaux employées
    sont documentées.
- **Domaine du moteur** : statique linéaire et sections constantes, avec une torsion uniforme. Il gère les rotules
  My et Mz et les appuis élastiques nodaux (ressorts découplés en repère global), mais pas encore les relâchements de
  N, V ou T, les rotules semi-rigides (ressort de rotation en extrémité de barre), les ressorts couplés ou en repère
  incliné, les excentrements, les charges partielles ni les couples répartis.
- **Une seule version de référence** : OpenSees 3.8.0.
