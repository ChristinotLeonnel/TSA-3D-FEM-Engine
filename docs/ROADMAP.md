# Feuille de route

Chaque étape s'achève par ses tests analytiques, ses benchmarks OpenSees (ou la raison de leur absence) et la mise
à jour de [validation/VALIDATION_PLAN.md](validation/VALIDATION_PLAN.md). Rien n'est annoncé avant d'être validé.

## Fait

| Version | Contenu |
|---|---|
| 0.1 | Poutres d'Euler–Bernoulli et de Timoshenko exactes, charges nodales et réparties linéaires, appuis, déplacements imposés, efforts et déformée exacts, contraintes, format JSON, post-traitement séparé, 11 benchmarks OpenSees. |
| 0.1 | Rotules My, Mz par condensation statique (benchmarks 12 à 17). |
| 0.2 | Appuis élastiques nodaux (benchmarks 18 à 20) ; audit indépendant et corrections (critère de mécanisme indépendant des unités, Wt, avertissements de quasi-mécanisme, résidu de résolution) ; bibliographie vérifiée ; qualification par niveaux ; intégration dans TSA comme moteur secondaire `fem3d`, avec comparaison et secours OpenSees explicites. |

| 0.3 | Charges sur barre : forces ponctuelles, charges réparties linéaires sur une partie [a, b], moments répartis ; forces nodales équivalentes exactes (dont rotations de section de Timoshenko pour les couples), efforts et déformée exacts par morceaux ; benchmarks 21 et 22 (`-beamPoint`, `-beamUniform` partiel) ; intégration dans TSA (test 530). |

## Prochaines étapes

1. **Qualification dans TSA (niveau 5)** : mode comparaison sur les projets d'exemple et les cas de
   `TSA/docs/validation` ; relecture humaine de la théorie. Condition pour faire de Fem3D le moteur par défaut.
2. **Charges sur barre : niveau 4** — audit indépendant des formulations des charges ponctuelles, partielles et des
   moments répartis ; référence pour Timoshenko et pour les moments répartis (second code).
3. **Barre de treillis** (6 DDL de translation, effort normal seul), puis câble linéaire si besoin.
4. **Timoshenko dans TSA** : transmettre les aires de cisaillement des sections de TSA et proposer la formulation
   dans les options du moteur.
5. **Excentrements** (décalages rigides d'extrémité), si TSA les transmet aux moteurs.
6. **Référence pour les charges trapézoïdales** : version d'OpenSees qui les applique en 3D, ou second code.

## Plus tard, sous conditions

- Second ordre (P-Δ) et flambement linéaire : matrice géométrique, avec benchmarks.
- Analyse modale : matrice de masse cohérente, avec benchmarks.
- Torsion non uniforme (élément à 7 DDL, Vlasov) : seulement avec une théorie vérifiée sur les sources et une
  référence de comparaison.

Hors champ : non-linéarité matérielle, dynamique temporelle, coques et volumes (OpenSees les couvre dans TSA).
