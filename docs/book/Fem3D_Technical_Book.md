# Fem3D — livre technique et scientifique

Ce livre explique le fonctionnement complet du moteur éléments finis 3D de poutres Fem3D : la mécanique, les
mathématiques, les algorithmes, le code C++, les tests et les limites. Chaque notion importante suit la même
progression : intuition physique, symboles, hypothèses, équations, interprétation, algorithme, fichiers C++, tests,
exemple vérifiable, limites. Chaque chapitre renvoie au code par des liens figés à une révision Git précise.

**Comment le lire.** Un étudiant qui découvre le sujet commence par le [parcours pédagogique](../learning/index.md),
qui renvoie aux chapitres utiles au bon moment. Un lecteur qui connaît la méthode des éléments finis peut aller
directement aux chapitres 6 à 16. Un développeur lira les chapitres 17 à 20, puis le
[guide développeur](../developer/DEVELOPER_GUIDE.md).

**Ce que le livre n'est pas.** Il ne remplace pas les ouvrages de référence, qu'il cite avec leur statut de
vérification ([bibliographie](../references/REFERENCES.md)) ; il ne reproduit aucun de leurs passages. Il ne décrit
comme disponible que ce que le code fait réellement, à la révision
[75700d2](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/commit/75700d2bfd33c29b2fadc795fd3051f25399ad19).

## Sommaire

### Partie I — Fondements

1. [Présentation de Fem3D et de son rôle dans TSA](chapters/01-presentation.md)
2. [Fondements de la méthode des éléments finis](chapters/02-fondements-mef.md)
3. [Modèle physique, hypothèses et degrés de liberté](chapters/03-modele-physique.md)
4. [Repères locaux et globaux](chapters/04-reperes.md)
5. [Conventions d'unités et de signes](chapters/05-unites-signes.md)

### Partie II — Éléments et données du modèle

6. [Élément poutre 3D Euler–Bernoulli](chapters/06-euler-bernoulli.md)
7. [Élément poutre 3D Timoshenko](chapters/07-timoshenko.md)
8. [Torsion de Saint-Venant et limites concernant le gauchissement](chapters/08-torsion-gauchissement.md)
9. [Matériaux et propriétés de section](chapters/09-materiaux-sections.md)
10. [Charges et conditions aux limites](chapters/10-charges-conditions-limites.md)
11. [Ressorts d'appui](chapters/11-ressorts.md)

### Partie III — Du système global aux résultats

12. [Assemblage des matrices globales](chapters/12-assemblage.md)
13. [Conditions aux limites et stabilité](chapters/13-conditions-limites-stabilite.md)
14. [Solveur numérique et diagnostics](chapters/14-solveur-diagnostics.md)
15. [Reconstruction des efforts internes](chapters/15-efforts-internes.md)
16. [Déplacements, rotations et contraintes](chapters/16-deplacements-contraintes.md)

### Partie IV — Logiciel, intégration et validation

17. [Architecture logicielle C++](chapters/17-architecture-cpp.md)
18. [Intégration avec TSA et OpenSees](chapters/18-integration-tsa-opensees.md)
19. [Validation scientifique et reproductibilité](chapters/19-validation-reproductibilite.md)
20. [Tests, limites et erreurs connues](chapters/20-tests-limites.md)

### Partie V — Prolonger

21. [Extensions possibles du modèle](chapters/21-extensions.md)
22. [Exercices et projets universitaires](chapters/22-exercices-projets.md)

## Documents de référence associés

| Besoin | Document |
|---|---|
| Équations condensées | [THEORY.md](../theory/THEORY.md) |
| Signes, unités, repères | [SIGN_CONVENTIONS.md](../theory/SIGN_CONVENTIONS.md), [UNITS_AND_COORDINATES.md](../theory/UNITS_AND_COORDINATES.md) |
| Pseudo-code | [ALGORITHMS.md](../reference/ALGORITHMS.md) |
| API | [API.md](../reference/API.md) |
| Diagrammes | [diagrammes](../diagrams/index.md) |
| Validation | [plan](../validation/VALIDATION_PLAN.md), [guide](../validation/SCIENTIFIC_VALIDATION_GUIDE.md), [niveaux de maturité](../validation/MATURITY_LEVELS.md) |
| Code ↔ tests ↔ commits | [GIT_TRACEABILITY.md](../traceability/GIT_TRACEABILITY.md) |
| Vocabulaire | [glossaire](../reference/GLOSSARY.md) |
