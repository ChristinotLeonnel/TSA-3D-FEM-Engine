# Stratégie de traduction (future)

**Aucune traduction n'est faite à ce stade.** La documentation est rédigée en français, langue de référence. Ce
document fixe la méthode pour la traduire plus tard sans créer de versions divergentes.

## 1. Principes

1. **Une source, des traductions dérivées.** Le français reste la source de vérité. Une page traduite indique la
   révision (hash de commit) de la page française dont elle est issue ; une page française modifiée après cette
   révision marque sa traduction comme « à mettre à jour ».
2. **Ce qui ne se traduit pas** : identifiants de code (`LinearStaticAnalysis`, `NodalSpring`), noms de tests, noms
   de fichiers, permaliens, équations, unités, titres originaux des références bibliographiques.
3. **Terminologie** : le [glossaire](../reference/GLOSSARY.md) est traduit en premier et sert de référence ; un terme
   technique reçoit une seule traduction par langue.
4. **Relecture technique** par une personne compétente en mécanique des structures dans la langue cible ; une
   traduction non relue est marquée comme telle.
5. **Ordre de priorité** : page d'accueil, démarrage rapide, niveaux pédagogiques 0 et 1, glossaire, puis le reste.

## 2. Langues visées

Celles de TSA : anglais (`en`) et malgache (`mg`), en plus du français (`fr`).

## 3. Mise en œuvre technique prévue

- Site : extension `mkdocs-static-i18n` (pages `page.en.md` à côté de `page.md`), sélecteur de langue de Material,
  recherche par langue. Elle n'est **pas** installée aujourd'hui ; son adoption sera vérifiée (compatibilité avec la
  version de Material utilisée) au moment de la première traduction.
- Contrôle en CI : un script listera les pages traduites dont la source française a changé depuis la révision indiquée.
- Les diagrammes Mermaid seront dupliqués par langue seulement pour leurs libellés ; la structure reste identique.

## 4. Interface de TSA (moteur Fem3D)

Les textes d'interface ajoutés par l'intégration (contextes Qt `TSA::Analysis::Fem3DEngine`,
`TSA::Analysis::Fem3DAdapter`, `TSA::UI::Fem3DOptionsWidget`) passent par `QCoreApplication::translate` ou `tr` :
ils sont prêts pour les fichiers `translations/tsa_en.ts` et `tsa_mg.ts` de TSA, mais **n'y figurent pas encore**
(vérifié le 2026-10-19 : 0 occurrence). Procédure, dans le dépôt TSA : `tools/i18n/update_translations.py`, traduction,
puis `tools/i18n/check_translations.py --strict` (test CTest `TSA_I18nCheck`).
