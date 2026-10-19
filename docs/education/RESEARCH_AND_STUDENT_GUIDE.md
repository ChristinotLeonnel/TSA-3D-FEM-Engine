# Guide pour les étudiants, les enseignants et les chercheurs

Fem3D est un moteur d'éléments finis de poutres 3D petit (environ 2 500 lignes de C++), entièrement documenté et
validé contre des solutions analytiques et contre OpenSees. Il se prête donc à l'enseignement et à la recherche :
chaque équation a son code, chaque code a son test, chaque test a sa référence. Ce guide indique par où commencer
selon votre rôle.

## Étudiants

**Parcours conseillé.**

1. [Démarrage rapide](../getting-started.md) : compiler, lancer les tests et l'exemple.
2. [Parcours pédagogique](../learning/index.md), niveaux 0 à 6 : chaque niveau renvoie aux chapitres utiles du
   [livre](../book/Fem3D_Technical_Book.md).
3. [Exercices](../book/chapters/22-exercices-projets.md) : réponses vérifiables à la fin du chapitre.

**Bonnes habitudes.**

- Refaire chaque exemple à la main ou en Python avant de regarder la valeur de Fem3D.
- Lire le test associé à chaque notion (section « Vérifier par les tests » des chapitres) : c'est la forme exécutable
  de la formule.
- Distinguer ce que le moteur **fait** de ce qui est **prévu** : les [limites connues](../limitations/KNOWN_LIMITATIONS.md)
  et la [feuille de route](../ROADMAP.md) font foi.
- Un résultat de logiciel ne se recopie pas : il se contrôle (équilibre, ordre de grandeur, cas limite).

## Enseignants

**Utilisations possibles.**

| Cours | Supports | Activité |
|---|---|---|
| Résistance des matériaux | chapitres 5, 6, 15, 16 | retrouver les formules classiques avec un seul élément exact ; lire les diagrammes |
| Calcul matriciel des structures | chapitres 4, 6, 10, 12, 13 | assembler à la main un petit portique, puis comparer à Fem3D |
| Méthode des éléments finis | chapitres 2, 7, 10, 14 | fonctions de forme, charges consistantes, Timoshenko exact, conditionnement |
| Génie logiciel scientifique | chapitres 17, 19, 20 | écrire un test qui peut échouer, fixer une tolérance, lire un benchmark |

**Travaux pratiques.** Les exercices 1, 2, 4 et 6 du [chapitre 22](../book/chapters/22-exercices-projets.md) tiennent
chacun dans une séance de deux heures. L'exercice 8 (écrire un test) convient à une évaluation : le critère est que
le test échoue quand on introduit une erreur, et réussit sur le code juste.

**Licence.** Le code est sous licence MIT : il peut être utilisé, modifié et redistribué en cours, avec la notice de
licence ([LICENSING.md](../LICENSING.md)). La documentation ne reproduit aucun passage d'ouvrage protégé ; les
ouvrages sont cités avec leur statut de vérification ([REFERENCES.md](../references/REFERENCES.md)).

## Chercheurs

**Reproductibilité.** Toute affirmation de la documentation renvoie au code par un lien figé sur une révision Git.
La validation se relance par deux commandes ([chapitre 19](../book/chapters/19-validation-reproductibilite.md)
§19.5) ; les scripts OpenSees générés, ses sorties brutes et les comparaisons valeur par valeur sont versionnés.

**Citer Fem3D.** Il n'existe pas encore de fichier de citation ni de DOI. Citer le dépôt
`https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine`, la version (0.2.0) et le **hash complet du commit**
utilisé, pour que le résultat soit reproductible.

**Rigueur attendue d'une contribution.** Celle du moteur : théorie écrite et sourcée, tests analytiques,
référence indépendante avec tolérances fixées avant les essais, limites documentées
([chapitre 21](../book/chapters/21-extensions.md) §21.1, `CONTRIBUTING.md`).

## Sujets de recherche ouverts

Chaque sujet découle d'une limite documentée. Aucun n'est traité à ce jour.

| Sujet | Question | Point de départ |
|---|---|---|
| Charges par morceaux | Représenter exactement efforts et déformée sous charges ponctuelles et partielles, avec des discontinuités, dans les deux formulations | [chapitre 21](../book/chapters/21-extensions.md) §21.2 |
| Torsion non uniforme | Élément à 7 DDL (Vlasov) : formulation exacte, conditions de gauchissement aux nœuds, assemblage de barres non colinéaires | [KNOWN_LIMITATIONS.md](../limitations/KNOWN_LIMITATIONS.md) §1 ; Vlasov, Trahair ([REFERENCES.md](../references/REFERENCES.md) §C10) |
| Stabilité | Matrice géométrique cohérente avec l'élément exact de Timoshenko ; flambement linéaire validé | [chapitre 21](../book/chapters/21-extensions.md) §21.7 |
| Dynamique | Matrice de masse cohérente de l'élément exact de Timoshenko ; analyse modale validée | idem |
| Diagnostic numérique | Seuils de quasi-mécanisme adaptatifs ; désignation des modes de mécanisme au-delà de 2000 DDL libres sans décomposition dense | [chapitre 14](../book/chapters/14-solveur-diagnostics.md) |
| Validation croisée | Seconde référence indépendante d'OpenSees, en particulier pour les charges trapézoïdales en 3D | [chapitre 19](../book/chapters/19-validation-reproductibilite.md) §19.6 |
| Qualification | Méthode de qualification d'un moteur sur des projets réels : choix des modèles, critères, archivage | [QUALIFICATION.md](../validation/QUALIFICATION.md) §4 |

## Pour aller plus loin

- [Livre technique](../book/Fem3D_Technical_Book.md), [glossaire](../reference/GLOSSARY.md),
  [référence de l'API](../reference/API.md).
- [Guide développeur](../developer/DEVELOPER_GUIDE.md), [traçabilité Git](../traceability/GIT_TRACEABILITY.md).
