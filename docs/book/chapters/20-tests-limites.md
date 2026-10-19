# Chapitre 20 — Tests, limites et erreurs connues

> **Objectifs**
>
> - Connaître l'organisation des 41 tests du moteur et savoir en écrire un nouveau.
> - Comprendre chaque message d'erreur ou d'avertissement et savoir quoi corriger dans le modèle.
> - Avoir une vue d'ensemble des limites du moteur et des défauts trouvés puis corrigés.
>
> **Prérequis**
>
> - Validation et tolérances ([chapitre 19](19-validation-reproductibilite.md)).
> - Architecture et codes d'erreur ([chapitre 17](17-architecture-cpp.md) §17.6).

## 20.1 Le cadre de test

**Sans dépendance.** `tests/TestFramework.h` est un mini-cadre maison : la macro `FEM3D_TEST(nom)` enregistre une
fonction au démarrage du programme, `CHECK(condition, message)` et `CHECK_NEAR(a, b, rel, message)` lèvent une
`Failure` qui décrit l'échec. Un test en échec n'arrête pas les autres ; le code de sortie de `fem3d_tests` est le
nombre de tests en échec (0 si tout réussit), ce que CTest exploite.

**Comparaison numérique.** `near(a, b, rel, abs = 1e-12)` accepte $|a - b| \le \max(\text{abs},\ \text{rel}\,|b|)$.
Les tests de solutions exactes passent `rel = 1e-9` ([chapitre 19](19-validation-reproductibilite.md) §19.2) ;
quelques vérifications utilisent une tolérance plus large : séries de torsion du rectangle (`6e-3`, précision des
formules de Roark), quasi-mécanisme (`1e-5`, précision réduite attendue), limite $\Phi \to 0$ et différence pente −
rotation (`1e-7`), invariances au maillage et aux unités, cohérence de la déformée avec les nœuds (`1e-8`).

**Modèles communs.** `tests/TestModels.h` fournit l'acier ($E = 210 \cdot 10^6$ kN/m², $G = 81 \cdot 10^6$ kN/m²), la
section rectangulaire 0,30 × 0,50 m, la console `cantilever(L, formulation, axe)` et la lecture d'un déplacement
nodal `tipValue`. Unités des tests : kN, m.

## 20.2 Les 41 tests

| Fichier | Tests | Rôle |
|---|---|---|
| [`unit/test_geometry_and_elements.cpp`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/unit/test_geometry_and_elements.cpp) | 7 | repères, géométrie dégénérée, symétrie et modes rigides, termes explicites de $K$, limite Timoshenko → EB, polynômes, données invalides |
| [`analytical/test_analytical.cpp`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_analytical.cpp) | 8 | traction, torsion, consoles, poutre sur appuis, charge triangulaire, poutre courte, tassement |
| [`analytical/test_releases.cpp`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_releases.cpp) | 7 | rotules : condensation, Gerber, mécanisme, cohérence dans un portique, exemple du README |
| [`analytical/test_springs.cpp`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_springs.cpp) | 6 | ressorts ([chapitre 11](11-ressorts.md)) |
| [`analytical/test_audit_gaps.cpp`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/analytical/test_audit_gaps.cpp) | 7 | lacunes relevées par l'audit : Timoshenko sous charge triangulaire, $A_{sy} \ne A_{sz}$, critère de mécanisme et unités, quasi-mécanisme, sections |
| [`integration/test_frames.cpp`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/integration/test_frames.cpp) | 6 | invariances, portique spatial, charges locales / globales, mécanismes, format JSON |

**Exécution constatée** : `build\fem3d_tests.exe` → « RESULTATS : 41 / 41 tests reussis » (19/10/2026, sources
identiques à `75700d2`). Le tableau complet test ↔ code ↔ commit est dans
[GIT_TRACEABILITY.md](../../traceability/GIT_TRACEABILITY.md).

## 20.3 Écrire un nouveau test

Règles ([DEVELOPER_GUIDE.md](../../developer/DEVELOPER_GUIDE.md) §6) :

1. **Choisir le niveau** : unitaire (propriété ou refus), analytique (solution exacte), intégration (invariance,
   équilibre, format), référence (benchmark OpenSees).
2. **Écrire la formule dans le test**, en commentaire et dans le message : le lecteur doit pouvoir refaire le calcul
   sans ouvrir un livre.
3. **Couvrir les deux formulations** quand c'est pertinent (boucle sur `EulerBernoulli` et `Timoshenko`).
4. **Fixer la tolérance avant d'exécuter** : $10^{-9}$ pour une solution exacte ; une autre valeur doit être
   justifiée dans le commentaire.
5. **Vérifier que le test peut échouer** : le faire échouer une fois (signe inversé, valeur modifiée) avant de le
   garder ; un test qui ne peut pas échouer ne prouve rien.
6. Ajouter le fichier au `CMakeLists.txt` s'il est nouveau.

Exemple réel, réduit à l'essentiel (console sous charge uniforme, `Analytical_CantileverUniformLoad_ExactAlongMember`) :

```cpp
FEM3D_TEST(Analytical_CantileverUniformLoad_ExactAlongMember)
{
    const double l = 3.0, q = -8.0;
    StructuralModel3D m = cantilever(l, BeamFormulation::EulerBernoulli);
    const auto r = solve(m, { 0, "q", {}, { { 1, LoadFrame::Local, Vector3(0, 0, q), Vector3(0, 0, q) } } });
    const double ei = kE * rectangle().inertiaY;
    CHECK_NEAR(tipValue(r, 2, Dof::Uz), q * l * l * l * l / (8 * ei), kExact, "w = qL⁴/8EI");
}
```

**Interdits** (`AGENTS.md`, règles 5 à 7) : supprimer, désactiver ou affaiblir un test pour obtenir une suite verte ;
ajuster une tolérance après avoir vu l'écart ; retoucher une valeur de référence d'OpenSees.

## 20.4 Messages d'erreur et remèdes

Chaque message désigne l'objet fautif. Les messages sont ceux du code à la révision `75700d2` ; ceux qui naissent à
la création d'un élément (géométrie, aires de cisaillement) sont précédés de « élément n : ».

| Message (début) | Code | Cause probable | Remède |
|---|---|---|---|
| « nœud n inconnu » ; « matériau « M » inconnu » ; « section … inconnue » | `UnknownReference` | faute de frappe, objet non déclaré | déclarer l'objet ou corriger la référence |
| « … déjà défini » | `DuplicateId` | deux objets de même identifiant | renuméroter |
| « élément n : ses deux nœuds sont identiques » | `DegenerateGeometry` | barre de longueur nulle | supprimer la barre ou corriger le nœud |
| « élément de longueur nulle ou quasi nulle (L = …) » | `DegenerateGeometry` | $L \le 10^{-9} \max(\lvert\text{coordonnées des extrémités}\rvert, 1)$ : nœuds confondus | fusionner les nœuds |
| « vecteur d'orientation v_xz nul ou parallèle à l'axe de l'élément » | `DegenerateGeometry` | $v_{xz}$ mal choisi, souvent sur un poteau | donner un $v_{xz}$ perpendiculaire à la barre ([chapitre 4](04-reperes.md)) |
| « module d'Young E non fini ou ≤ 0 » ; « coefficient de Poisson hors de ]-1 ; 0,5[ » | `InvalidInput` | propriété manquante ou d'une autre unité | vérifier le matériau et les unités |
| « aires de cisaillement Asy et Asz requises par Timoshenko » | `InvalidInput` | section sans aires de cisaillement | les fournir, ou choisir Euler–Bernoulli |
| « raideur de ressort négative ou non finie » ; « ressort sur un DDL bloqué » | `InvalidInput` | [chapitre 11](11-ressorts.md) §11.5 | corriger la raideur, ou retirer le blocage ou le ressort |
| « charge … non finie » ; « déplacement imposé non fini » | `InvalidInput` | NaN venu d'un calcul amont | corriger la donnée |
| « matrice de rigidité singulière : mécanisme … DDL des modes de mécanisme : nœud n, Ry … » | `Mechanism` | appuis insuffisants, rotules mal placées, torsion libre | retenir les DDL désignés ([chapitre 13](13-conditions-limites-stabilite.md) §13.3) |
| « écriture impossible : … » | `IoError` | dossier absent ou protégé | changer le chemin de sortie |

**Avertissements** (le calcul se poursuit, `LoadCaseResults::warnings`) :

| Message (début) | Signification | Que faire |
|---|---|---|
| « conditionnement faible (quasi-mécanisme possible) : pivot relatif minimal … au DDL … ; les résultats peuvent perdre jusqu'à k chiffres significatifs » | un DDL est presque libre (ressort très faible, barre très souple devant ses voisines) | examiner le DDL désigné ; ne pas faire confiance aux derniers chiffres ([chapitre 14](14-solveur-diagnostics.md) §14.4) |
| « résidu de résolution élevé : … (seuil 1.00e-08) » | la résolution a perdu en précision | revoir le conditionnement du modèle, les unités |

## 20.5 Défauts trouvés et corrigés

L'[audit](../../AUDIT.md) recense douze constats ; ceux qui touchaient le moteur sont corrigés et couverts par des
tests. Pour les deux premiers (critère de mécanisme, $W_t$), il est attesté que ces tests échouaient avec l'ancien
code ([VALIDATION_PLAN.md](../../validation/VALIDATION_PLAN.md) §3) :

| Constat | Correction | Test |
|---|---|---|
| Critère de mécanisme dépendant des unités (stable en kN-m, refusé en N-mm) | pivot comparé à la diagonale de **son propre** DDL | `Solver_MechanismCriterionIndependentOfUnits` |
| $W_t$ du rectangle surestimé jusqu'à 4,4 % (torsion non conservative) | polynôme de Roark | `Section_RectangleTorsionMatchesExactSeries` |
| Quasi-mécanismes acceptés en silence ; pas de résidu | avertissement et résidu relatif | `Solver_QuasiMechanismWarnsButSolves` |
| Coefficient de Cowper du cercle avec $\nu = 0{,}3$ figé | $\nu$ en paramètre | `Section_CircleShearCoefficientUsesPoisson` |
| Modèle sans DDL libre refusé ; pivot nul non localisé | cas $n_f = 0$ accepté ; DDL désignés | `Analytical_SupportSettlement`, `Integration_MechanismDetected` |

Les autres constats sont documentaires, ou concernent la référence (deux comportements d'OpenSees 3.8.0, décrits au
[chapitre 19](19-validation-reproductibilite.md) §19.6).

## 20.6 Limites, en résumé

Liste de référence : [KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md).

| Domaine | Limite principale | Chapitre |
|---|---|---|
| Physique | statique linéaire, petits déplacements, sections constantes, torsion de Saint-Venant sans gauchissement | [8](08-torsion-gauchissement.md) |
| Éléments | pas de treillis ni de câble ; rotules My, Mz seulement ; pas d'excentrement ni de contrainte cinématique | [21](21-extensions.md) |
| Appuis | axes globaux ; ressorts découplés, linéaires, bilatéraux | [10](10-charges-conditions-limites.md), [11](11-ressorts.md) |
| Charges | réparties linéaires sur toute la longueur seulement ; pas de charge thermique ni de précontrainte | [10](10-charges-conditions-limites.md) |
| Numérique | DDL d'un mécanisme désignés jusqu'à 2000 DDL libres ; seuils de quasi-mécanisme fixes | [14](14-solveur-diagnostics.md) |
| Résultats | contraintes de RDM, sans vérification réglementaire ; pas de combinaison | [16](16-deplacements-contraintes.md) |
| Validation | une seule version de référence (OpenSees 3.8.0) ; niveau 5 non atteint ; relecture humaine à faire | [19](19-validation-reproductibilite.md) |

## 20.7 Points ouverts

- Qualification de niveau 5 dans TSA ([QUALIFICATION.md](../../validation/QUALIFICATION.md) §4).
- Charges ponctuelles, partielles et moments répartis ([feuille de route](../../ROADMAP.md), étape 2).
- Exécutable de tests de TSA lancé directement en mode `offscreen` : arrêt brutal observé une fois, non reproduit
  par CTest ([KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md) §6).
- Numéros de chapitre et de page des ouvrages cités : vérifiés sur les notices seulement
  ([SOURCES_MANIFEST.md](../../references/SOURCES_MANIFEST.md)).

## À retenir

- 41 tests en six fichiers, un mini-cadre sans dépendance, un code de sortie égal au nombre d'échecs.
- Un test écrit sa formule, fixe sa tolérance avant d'être exécuté, et doit pouvoir échouer.
- Chaque erreur a un code stable et désigne l'objet fautif ; les avertissements n'arrêtent pas le calcul.
- Les défauts trouvés par l'audit sont corrigés et gardés par des tests.

## Dans le code source

- Cadre : [`near`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/TestFramework.h#L40-L43), [`CHECK_NEAR`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/TestFramework.h#L65-L76), [`main.cpp`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/tests/main.cpp).
- Messages : [`StructuralModel3D::validate`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Model/StructuralModel3D.cpp#L113-L158), [`LocalCoordinateSystem3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Geometry/LocalCoordinateSystem3D.cpp#L11-L42),
  [`LinearSystemSolver::factorize`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Solver/LinearSystemSolver.cpp#L59-L98), avertissements :
  [`LinearStaticAnalysis`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/src/Analysis/LinearStaticAnalysis.cpp#L14-L33).

## Vérifier par les tests

| Commande | Vérification |
|---|---|
| `build\fem3d_tests.exe` | 41 tests ; code de sortie = nombre d'échecs |
| `ctest --test-dir build --output-on-failure` | même suite, plus les benchmarks si OpenSees est trouvé |

## Références

- [AUDIT.md](../../AUDIT.md), [KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md),
  [DEVELOPER_GUIDE.md](../../developer/DEVELOPER_GUIDE.md).
- A13 *Roark's Formulas for Stress and Strain* — module de torsion du rectangle ; statut dans
  [REFERENCES.md](../../references/REFERENCES.md) §C3.

---

Précédent : [Chapitre 19 — Validation scientifique et reproductibilité](19-validation-reproductibilite.md) · Suivant :
[Chapitre 21 — Extensions possibles du modèle](21-extensions.md)
