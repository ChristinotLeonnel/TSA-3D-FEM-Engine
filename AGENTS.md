# Règles de travail du dépôt TSA-3D-FEM-Engine

Ces règles s'appliquent à toute personne et à tout agent (humain ou outil d'IA) qui modifie ce dépôt. Elles sont
l'unique fichier de règles : `CLAUDE.md` l'importe, `CONTRIBUTING.md` en donne la procédure pratique.

## 1. Vérité technique

1. **Vérifier le code avant d'écrire.** Toute affirmation documentaire (équation, signe, comportement, message)
   doit être confrontée au code source et aux tests. Une affirmation non vérifiée est marquée « à vérifier ».
2. **Fonctionnalité disponible ou prévue.** Ne jamais décrire une fonctionnalité prévue comme disponible. Les
   fonctions non implémentées sont listées dans `docs/limitations/KNOWN_LIMITATIONS.md` et `docs/ROADMAP.md`.
3. **Aucune fausse déclaration** de compilation, de test, de benchmark, de publication ou de commit. On écrit ce qui a
   été exécuté, avec son résultat réel ; ce qui n'a pas été exécuté est dit tel quel.
4. **Limites explicites.** Toute formulation documentée indique ses hypothèses et ses limites (par exemple : pas de
   gauchissement avec 6 DDL par nœud).

## 2. Résultats de référence

5. **Tests de référence préservés.** On ne supprime, ne désactive ni n'affaiblit un test pour obtenir une suite verte.
   Un test faux est corrigé avec une justification écrite dans le commit.
6. **Tolérances.** Les tolérances numériques sont fixées avant les essais et justifiées
   (`docs/validation/VALIDATION_PLAN.md` §2). Les modifier exige une justification scientifique écrite, jamais
   l'observation d'un écart.
7. **Benchmarks OpenSees.** Les valeurs de référence sont régénérées par OpenSees, jamais retouchées à la main.
8. **Code de calcul.** Le moteur n'est pas réécrit sans nécessité démontrée ; toute modification est accompagnée de
   tests, de la reproduction des benchmarks et d'une revue.

## 3. Traçabilité

9. **Équations et sources.** Chaque équation renvoie à `docs/theory/THEORY.md` ou à une source de
   `docs/references/REFERENCES.md`, avec son statut de vérification (notice seulement, contenu consulté…). On
   n'invente aucune référence, aucun numéro de page, aucun DOI. On ne recopie pas de passages d'ouvrages protégés.
10. **Références Git.** Les liens vers le code utilisent un hash de commit complet (40 caractères), jamais `main`.
    Ils sont produits et vérifiés par `tools/docs/gitlinks.py` (`permalink`, `check`). Un commit n'est attribué à une
    fonctionnalité qu'après lecture de son contenu (`git show`).
11. **Niveaux.** Les niveaux de maturité du moteur (`docs/validation/MATURITY_LEVELS.md`) et les niveaux
    pédagogiques (`docs/learning/`) sont distincts. Un niveau de maturité n'est déclaré atteint qu'avec ses preuves.

## 4. Documentation

12. Langue : français. Équations en LaTeX (`$…$`, `$$…$$`), diagrammes en Mermaid (sources versionnées dans
    `docs/diagrams/` ou dans la page qui les utilise). Fins de ligne CRLF pour les fichiers Markdown existants.
13. Une seule source : le site (`mkdocs.yml`) est généré à partir de `docs/` ; pas de copie manuelle d'un chapitre.
14. Liens internes relatifs entre pages de `docs/` ; liens vers les fichiers hors de `docs/` par permalien GitHub.
15. Avant de proposer une modification de la documentation :
    `python tools/docs/gitlinks.py check` et `mkdocs build --strict` doivent réussir.

## 5. Git et publication

16. Travailler sur une branche ; aucune réécriture d'historique publié, aucun push forcé, aucune suppression de
    branche sans accord explicite.
17. Ne jamais publier de secret, de jeton, de donnée personnelle, de fichier téléchargé soumis à des droits
    (`docs/references/_downloads/` n'est jamais versionné) ni de chemin local absolu (scripts Tcl générés).
18. La publication du site (GitHub Pages) est automatique à chaque push sur la branche `main` après succès de
    toutes les vérifications strictes de la CI (`docs/developer/DEVELOPER_GUIDE.md` §8).

## 6. Travail multi-agents

19. Un agent reçoit un périmètre de fichiers précis ; deux agents ne modifient jamais le même fichier en même temps.
20. Le rapport d'un agent n'est pas une preuve : l'intégrateur relit le diff complet et relance les vérifications.
21. Un audit indépendant relit les livrables des autres agents avant intégration.
