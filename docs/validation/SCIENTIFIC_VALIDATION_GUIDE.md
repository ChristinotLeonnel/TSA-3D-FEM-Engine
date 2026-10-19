# Guide de validation scientifique

Comment vérifier un calcul de Fem3D, comment interpréter un écart, et ce que la validation prouve ou ne prouve pas.
Les cas et leurs verdicts sont dans [VALIDATION_PLAN.md](VALIDATION_PLAN.md) ; les résultats mesurés dans
[VALIDATION_OPENSEES.md](VALIDATION_OPENSEES.md).

## 1. Trois questions distinctes

| Question | Méthode | Dans ce dépôt |
|---|---|---|
| **Vérification** : le code résout-il correctement les équations choisies ? | solutions analytiques exactes, invariances | `fem3d_tests` (41 tests) |
| **Validation croisée** : donne-t-il les mêmes résultats qu'un code indépendant reconnu ? | comparaison à OpenSees | `fem3d_opensees_benchmarks` (20 cas × 2 formulations) |
| **Validité du modèle** : les équations représentent-elles la structure réelle ? | hypothèses, limites | [KNOWN_LIMITATIONS.md](../limitations/KNOWN_LIMITATIONS.md) — responsabilité de l'ingénieur |

Un calcul peut être parfaitement vérifié et validé contre OpenSees et rester faux pour une structure dont le
comportement sort des hypothèses (gauchissement empêché d'un profil ouvert, grands déplacements, plasticité).

## 2. Fixer les tolérances avant les essais

Une tolérance se justifie par la précision attendue de la méthode, jamais par l'écart observé.

- **Éléments exacts** (barres prismatiques) : la solution par éléments finis coïncide avec la solution analytique ;
  seul l'arrondi subsiste. D'où l'écart relatif ≤ 1e-9 des tests analytiques.
- **Comparaison à OpenSees** : mêmes formulations, factorisations différentes, sorties à 16 ou 17 chiffres. La
  tolérance max(1e-6·|réf|, 1e-9·échelle) est 10⁴ à 10⁶ fois plus petite que l'effet d'une erreur de formulation
  (un signe, un Φ ou un axe faux produit un écart de 1 à 100 %).
- **Près de zéro**, l'écart relatif n'a pas de sens : on compare à l'échelle de la grandeur dans le cas. Une grandeur
  toute nulle reçoit une échelle plancher dimensionnellement cohérente (moments ≥ forces × longueur).

## 3. Reproduire la validation

```bat
build\fem3d_tests.exe
build\fem3d_opensees_benchmarks.exe [OpenSees.exe] [dossier de sortie]
```

La synthèse est régénérée dans `benchmarks/OpenSees/results/synthese.md`. Les scripts Tcl et les sorties brutes
d'OpenSees permettent de rejouer chaque cas à la main. Les valeurs de référence ne sont jamais modifiées à la main.

## 4. Interpréter un écart

| Constat | Causes probables | Démarche |
|---|---|---|
| Écart de l'ordre de 1e-12 à 1e-9 | arrondi | normal, dans la tolérance |
| Écart de 1 % à 100 % sur une seule grandeur | signe, axe local, association Iy/Iz ou Asy/Asz, unité | comparer un cas élémentaire (console) ; vérifier le repère (v_xz) |
| Écart sur les réactions seulement, avec des nœuds dédoublés | convention de la référence (`equalDOF` : la réaction se partage entre maître et esclave) | sommer les réactions ; ne pas modifier la référence |
| Valeurs identiques à une charge uniforme pour une charge trapézoïdale | la référence ignore la variation (OpenSees 3.8.0, `-beamUniform` à 8 arguments) | cas « non comparable » ; valider analytiquement |
| La référence rend des valeurs énormes sans échec | mécanisme non signalé par la référence | critère « solution non bornée » ; Fem3D doit refuser |
| Différence de comportement entre unités | critère numérique dépendant des unités | le test d'invariance doit échouer (il existe : `Solver_MechanismCriterionIndependentOfUnits`) |

Tout écart est d'abord supposé être une erreur de Fem3D, puis une différence de convention, et seulement en dernier
une erreur de la référence, à démontrer par une solution analytique.

## 5. Convergence et invariances

Les éléments d'Euler–Bernoulli et de Timoshenko de Fem3D sont exacts pour des barres prismatiques sous charges
nodales et réparties linéaires : **un seul élément par barre suffit**, et subdiviser une barre ne change pas le
résultat. C'est vérifié par `Integration_SpatialFrame_EquilibriumAndMeshInvariance` (invariance par le maillage) et
`Analytical_TimoshenkoDeepBeam_NoShearLocking` (pas de verrouillage, même résultat avec 10 éléments). Une étude de
convergence n'a de sens que pour des formulations approchées, qui n'existent pas dans le moteur actuel.

Autres invariances testées : orientation (`Integration_InclinedCantilever_OrientationInvariance`), repère des charges
(`Integration_GlobalAndLocalDistributedLoadsEquivalent`), unités (`Solver_MechanismCriterionIndependentOfUnits`).

## 6. Ce que la validation ne prouve pas

- La justesse de la rigidité n'est pas prouvée par le contrôle d'équilibre global : il vérifie l'équivalence
  statique des charges et la précision de la résolution.
- La comparaison à OpenSees ne prouve rien hors du domaine commun aux deux codes (charges trapézoïdales non
  comparables, rotules de Timoshenko hors axes globaux).
- Les numéros de chapitres des ouvrages cités n'ont pas été vérifiés sur les textes
  ([SOURCES_MANIFEST.md](../references/SOURCES_MANIFEST.md)).
