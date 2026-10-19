# Qualification du moteur Fem3D

La qualification décide **ce que Fem3D a le droit de calculer** dans TSA. Un niveau s'atteint fonction par fonction ;
un moteur n'est promu que sur le périmètre où toutes ses fonctions ont le niveau requis.

## 1. Niveaux

| Niveau | Exigence | Preuve |
|---|---|---|
| **1. Implémenté** | Code écrit, données invalides refusées avec un message qui désigne l'objet fautif. | Tests unitaires. |
| **2. Vérifié** | Résultats égaux aux solutions analytiques exactes (écart relatif ≤ 1e-9). | Tests analytiques. |
| **3. Validé** | Résultats égaux à une référence indépendante sur des cas élémentaires, avec des tolérances fixées avant les essais. | Benchmarks OpenSees. |
| **4. Validé sur modèles représentatifs** | Niveau 3 sur des modèles multi-éléments (portiques spatiaux, orientations quelconques, combinaisons de charges) **et** audit indépendant des formulations. | Cas 08 à 20, [../verification/INDEPENDENT_FORMULATION_AUDIT.md](../verification/INDEPENDENT_FORMULATION_AUDIT.md). |
| **5. Qualifié dans TSA** | Comparaison systématique Fem3D / OpenSees, dans TSA, sur les modèles réels : projets d'exemple et cas de `TSA/docs/validation`, sans aucun écart ; relecture humaine de la théorie. | Mode comparaison de TSA ; rapport de qualification. |

## 2. État au 19/10/2026

| Fonction | Niveau |
|---|---|
| Poutre d'Euler–Bernoulli 3D, charges nodales | 4 |
| Charges réparties uniformes sur toute la longueur (locales, globales, poids propre) | 4 |
| Charges réparties linéaires sur toute la longueur | 2, plus une comparaison indirecte à OpenSees dans TSA (test 477) |
| Rotules My, Mz (Euler–Bernoulli) | 4 |
| Appuis élastiques nodaux (axes globaux) | 4 |
| Détection des mécanismes | 4 |
| Poutre de Timoshenko | 4 dans Fem3D ; **non exposée** dans TSA (le moteur OpenSees de TSA utilise `elasticBeamColumn`, et les aires de cisaillement des sections de TSA ne sont pas transmises) |
| Rotules de Timoshenko hors axes globaux, déplacements imposés, contraintes | 2 |
| **Intégration dans TSA** (moteur `fem3d`) | 4 sur un portique spatial représentatif (test 477) ; **niveau 5 non atteint** |

## 3. Décision

1. Fem3D est **enregistré dans TSA comme moteur secondaire** (`fem3d`), sur le seul périmètre de niveau 4 :
   ossatures 3D de poutres et poteaux d'Euler–Bernoulli, rotules My et Mz, appuis rigides et élastiques, charges
   nodales et charges réparties uniformes ou linéaires sur toute la longueur, poids propre, statique linéaire.
2. **OpenSees reste le moteur 3D par défaut.** Fem3D ne devient moteur par défaut qu'au niveau 5.
3. Tout ce qui sort du périmètre est **refusé avec la raison**, ou confié à OpenSees si l'utilisateur a choisi ce
   secours. Ce secours est annoncé avant le calcul, et les résultats portent l'identité d'OpenSees.
4. Un mécanisme, ou une donnée invalide, n'est jamais confié à un autre moteur.

## 4. Pour atteindre le niveau 5

1. Calculer en mode comparaison tous les projets d'exemple et les cas de `TSA/docs/validation` qui entrent dans le
   périmètre, et archiver les tables de comparaison.
2. Relecture de [../theory/THEORY.md](../theory/THEORY.md) et [../theory/SIGN_CONVENTIONS.md](../theory/SIGN_CONVENTIONS.md)
   par un ingénieur.
3. Étendre le périmètre aux charges ponctuelles, partielles et aux moments répartis (avec leurs benchmarks), car ces
   charges sont courantes dans les projets réels.
