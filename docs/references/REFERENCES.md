# Bibliographie du moteur de poutres 3D (Fem3D)

Recherche bibliographique faite le **2026-10-19**. Elle couvre les formulations décrites dans
[`../theory/THEORY.md`](../theory/THEORY.md) et [`../theory/SIGN_CONVENTIONS.md`](../theory/SIGN_CONVENTIONS.md).

## Comment lire ce document

Chaque référence porte un niveau de vérification :

- **Vérifié (notice)** : auteurs, titre, année, éditeur, ISBN ou DOI contrôlés sur une source fiable : Crossref,
  page de l'éditeur ou catalogue de bibliothèque. Le **contenu** de l'ouvrage n'a pas été lu.
- **Vérifié (contenu)** : le passage cité a été lu en ligne : documentation officielle, code source, table des
  matières.
- **À vérifier** : information plausible, mais non contrôlée sur l'ouvrage lui-même. C'est le cas de tous les
  numéros de chapitre et de section qui ne sont pas marqués « vérifié ».

Aucun numéro de page n'est donné : aucun ouvrage n'a été consulté en texte intégral. Aucun article payant n'a été
lu ; pour les articles, seules les métadonnées Crossref ont été contrôlées. Les liens, les dates de consultation et
le statut de chaque source sont dans [`WEBOGRAPHY.md`](WEBOGRAPHY.md) et
[`SOURCES_MANIFEST.md`](SOURCES_MANIFEST.md).

---

## A. Ouvrages

### A1. Zienkiewicz, Taylor, Zhu : *The Finite Element Method: Its Basis and Fundamentals*

- O. C. Zienkiewicz, R. L. Taylor, J. Z. Zhu, 7ᵉ éd., Butterworth-Heinemann (Elsevier), Oxford / Waltham MA, 2013.
- ISBN 978-1-85617-633-0 (imprimé), 978-0-08-095135-5 (e-book). **Vérifié (notice)** : catalogues BL et UTB.
- Table des matières, **vérifiée** sur le catalogue UTB :
  - ch. 3 et 4 : formes faibles et variationnelles en 1-D ;
  - ch. 6 : fonctions de forme, dérivées et intégration (quadrature de Gauss) ;
  - ch. 8 : *The patch test, reduced integration, and nonconforming elements* ;
  - ch. 9 : *Mixed formulation and constraints* ;
  - ch. 13 : *Plate bending approximation: thin and thick plates*, qui traite les contraintes de cisaillement et le
    verrouillage ;
  - ch. 18 : procédures informatiques.
- Usage pour Fem3D : forces nodales consistantes ∫ N q dx et quadrature de Gauss (THEORY §4) ; contexte du
  verrouillage en cisaillement et de l'intégration réduite, que Fem3D **n'utilise pas** (THEORY §3).

### A2. Zienkiewicz, Taylor, Fox : *The Finite Element Method for Solid and Structural Mechanics*

- O. C. Zienkiewicz, R. L. Taylor, D. D. Fox, 7ᵉ éd., Butterworth-Heinemann, 2014.
- ISBN 978-1-85617-634-7. Un catalogue signale cet ISBN comme erroné et donne 978-0-08-095136-2 (e-book) :
  **à vérifier**.
- Table des matières d'après un catalogue (UPNM) : ch. 9 *Structural mechanics problems in one dimension – rods*,
  ch. 12 *Curved rods and axisymmetric shells*. La notice du catalogue paraît imparfaite : **à vérifier**.
- Usage pour Fem3D : poutres de Timoshenko 3D en contexte EF (THEORY §2 et §3).

### A3. Bathe : *Finite Element Procedures*

- K.-J. Bathe, 2ᵉ éd., Klaus-Jürgen Bathe (autoédition), Watertown MA, 2014.
- ISBN 978-0-9790049-5-7. **Vérifié (notice)**. La 1ʳᵉ éd. a paru chez Prentice Hall en 1996.
- Sections dont la correspondance avec le thème est **vérifiée** par le programme MIT OCW 2.092 (automne 2009) :
  - §4.2 : formulation EF, dont les vecteurs de charges consistants (THEORY §4) ;
  - §8.1 et §8.2 : élimination de Gauss et factorisation LDLᵀ (THEORY §7).
- **À vérifier** : §5.4 (éléments de structure, dont la poutre) et la condensation statique traitée au ch. 8.
- Accès : le PDF intégral n'est **pas** en ligne sur le site MIT de l'auteur au 2026-10-19. Les URL testées
  renvoient une erreur 404.

### A4. Hughes : *The Finite Element Method: Linear Static and Dynamic Finite Element Analysis*

- T. J. R. Hughes, Dover Publications, Mineola NY, 2000. C'est une réédition de Prentice-Hall, 1987.
- ISBN 978-0-486-41181-1. **Vérifié (notice)**.
- Chapitres **à vérifier** : ch. 4 (méthodes mixtes et de pénalité, intégration réduite et sélective) ; ch. 5
  (approche C⁰ des plaques et des poutres, poutre de Timoshenko et verrouillage).
- Usage pour Fem3D : justifier l'absence de verrouillage de l'élément exact par rapport aux interpolations
  indépendantes (THEORY §3).

### A5. Cook, Malkus, Plesha, Witt : *Concepts and Applications of Finite Element Analysis*

- R. D. Cook, D. S. Malkus, M. E. Plesha, R. J. Witt, 4ᵉ éd., John Wiley & Sons, New York, 2002. Paru en
  octobre 2001, © 2002.
- ISBN 978-0-471-35605-9. **Vérifié (notice)**.
- Thèmes de chapitres d'après la table des matières de Wiley, vue par un moteur de recherche ; numéros
  **à vérifier** : éléments 1-D (poutre), contraintes et verrouillage, flexion des plaques, annexe sur la
  résolution des systèmes.
- Usage pour Fem3D : rigidité de poutre, condensation statique et rotules, résolution (THEORY §2, §6 et §7).

### A6. Przemieniecki : *Theory of Matrix Structural Analysis*

- J. S. Przemieniecki, McGraw-Hill, New York, 1968. Réédition Dover corrigée et non abrégée, 1985.
- ISBN 978-0-486-64948-1 (Dover). **Vérifié (notice)**.
- Contenu partiellement vu dans une notice : le ch. 6 traite des matrices de transformation, de la **condensation
  des matrices de rigidité** et des rigidités déduites des souplesses ; le ch. 7 traite des propriétés de souplesse
  des éléments, dont la poutre.
- **À vérifier** : le ch. 5, sur la rigidité des éléments. Il contiendrait la poutre spatiale 12 DDL avec
  cisaillement, à paramètre Φ, une forme proche de THEORY §3.
- Usage pour Fem3D : rigidité 12×12 (EB et Timoshenko), condensation statique (THEORY §2, §3 et §6),
  transformation de repère (SIGN_CONVENTIONS §2).

### A7. McGuire, Gallagher, Ziemian : *Matrix Structural Analysis*

- W. McGuire, R. H. Gallagher, R. D. Ziemian, 2ᵉ éd., John Wiley & Sons, 2000. ISBN 978-0-471-12918-9.
- Réimpression CreateSpace en 2015, ISBN 978-1-5075-8513-9. **Vérifié (notice)**.
- **Diffusion gratuite vérifiée** :
  - le site du logiciel des auteurs, MASTAN2, propose « Download E-Book (Free) » ;
  - le lien mène au dépôt institutionnel de Bucknell University (*Faculty Books* 7), qui héberge un PDF de
    65,3 Mo ;
  - aucune licence explicite (Creative Commons ou autre) n'est indiquée.
- Le téléchargement automatisé est bloqué par une vérification anti-robot (Cloudflare). Elle n'a **pas** été
  contournée : le fichier n'est donc **pas** téléchargé. À télécharger manuellement depuis un navigateur.
- Chapitres **à vérifier** : rigidité des éléments de portique 3D, relâchements d'extrémité, forces d'encastrement,
  résolution des équations, torsion non uniforme et gauchissement.
- Usage pour Fem3D : THEORY §2 et §4 à §7, limite de Vlasov (THEORY §1).

### A8. Weaver, Gere : *Matrix Analysis of Framed Structures*

- W. Weaver Jr., J. M. Gere, 3ᵉ éd., Van Nostrand Reinhold, New York, 1990.
- ISBN 0-442-23485-6. Il existe aussi un tirage Chapman & Hall, ISBN 0-412-07861-9. **Vérifié (notice)**.
- L'ISBN 0-442-30735-7, parfois cité, n'a pas été retrouvé.
- Chapitres **à vérifier** : actions d'encastrement parfait (charges équivalentes), relâchements, appuis élastiques,
  portiques spatiaux.
- Usage pour Fem3D : THEORY §4 et §6, ressorts d'appui.

### A9. Timoshenko, Gere : *Mechanics of Materials*

- S. P. Timoshenko, J. M. Gere, 1ʳᵉ éd., Van Nostrand Reinhold, New York, 1972.
- ISBN 0-442-22637-3. Un catalogue donne aussi 0-442-29996-6, qui est peut-être l'édition SI de 1973.
  **Vérifié (notice)**.
- Les éditions ultérieures ont paru sous Gere et Timoshenko, puis Gere et Goodno : éditions et ISBN
  **à vérifier**.
- Usage pour Fem3D : contraintes de RDM (σ = N/A ± M y/I, τ moyenne, torsion), conventions de signe
  (SIGN_CONVENTIONS §5). Chapitres **à vérifier**.

### A10. Timoshenko, Gere : *Theory of Elastic Stability*

- S. P. Timoshenko, J. M. Gere, 2ᵉ éd., McGraw-Hill, 1961. Réédition Dover, 2009.
- ISBN 978-0-486-47207-2 (Dover). **Vérifié (notice)**.
- **Vérifié** par la préface de la 2ᵉ éd., vue en extrait : ch. 5 *torsional buckling*, ajouté dans la 2ᵉ éd. et
  qui expose la torsion non uniforme des profils minces ; ch. 6 *lateral buckling of beams*.
- Usage pour Fem3D : référence de la **limite** « torsion non uniforme et gauchissement non implémentés »
  (THEORY §1).

### A11. Vlasov : *Thin-Walled Elastic Beams*

- V. Z. Vlasov, 2ᵉ éd. revue et augmentée, Israel Program for Scientific Translations, Jérusalem, 1961.
- Traduction du russe publiée pour la National Science Foundation, diffusée par l'Office of Technical Services,
  réf. TT 61-11400. **Vérifié (notice)** : catalogue Texas A&M.
- Il existe une copie sur archive.org (`v.-z.-vlasov-thin-walled-elastic-beams-1961`), déposée par un
  particulier, sans mention de droits. Elle n'est **pas** téléchargée : statut juridique non établi.
- Usage pour Fem3D : théorie du gauchissement, du bimoment et de la torsion mixte. C'est une **limite documentée**
  (THEORY §1).

### A12. Trahair : *Flexural-Torsional Buckling of Structures*

- N. S. Trahair, E & FN Spon / Chapman & Hall, Londres, 1993. Également CRC Press, Boca Raton, 1993.
- ISBN 0-419-17110-5 (Spon) et 0-8493-7763-3 (CRC). **Vérifié (notice)**. Les ISBN varient selon l'éditeur.
- Usage pour Fem3D : torsion non uniforme, constante de gauchissement Iw et éléments à 7 DDL par nœud. C'est une
  **limite documentée** (THEORY §1). Chapitres **à vérifier**.

### A13. *Roark's Formulas for Stress and Strain*

- 9ᵉ éd. : W. C. Young, R. G. Budynas, A. M. Sadegh, McGraw-Hill Education, 2020.
  - ISBN 978-1-260-45375-1 ; e-book 978-1-260-45376-8. **Vérifié (notice)** : page de l'éditeur.
  - La page de l'éditeur ne cite que Budynas et Sadegh ; les catalogues ajoutent W. C. Young.
- Torsion : ch. 10 ; la formule du rectangle plein figure dans la **Table 10.1** de la 7ᵉ éd. (2002), d'après des
  sources secondaires. La numérotation de la 9ᵉ éd. est **à vérifier**.
- Usage pour Fem3D : constante de torsion J et module Wt du rectangle (`src/Sections/BeamSection3D.cpp`, voir §C3).

### A14. Pilkey : *Analysis and Design of Elastic Beams: Computational Methods*

- W. D. Pilkey, John Wiley & Sons, 2002.
- ISBN 978-0-471-38152-5. DOI [10.1002/9780470172667](https://doi.org/10.1002/9780470172667). **Vérifié (notice)**
  par Crossref.
- **À vérifier** : propriétés de section (aires de cisaillement, J, constante de gauchissement), poutre de
  Timoshenko, torsion avec gauchissement.

### A15. Pilkey : *Formulas for Stress, Strain, and Structural Matrices*

- W. D. Pilkey, 2ᵉ éd., John Wiley & Sons, 2004.
- ISBN 978-0-471-03221-2. DOI [10.1002/9780470172681](https://doi.org/10.1002/9780470172681). **Vérifié (notice)**
  par Crossref.
- Usage pour Fem3D : matrices de rigidité de poutre et formules de section, en recoupement de Roark. Contenu
  **à vérifier**.

### A16. Reddy : *An Introduction to the Finite Element Method*

- J. N. Reddy, 4ᵉ éd., McGraw-Hill Education, 2019. Parution annoncée en octobre 2018.
- ISBN 978-1-259-86190-1. **Vérifié (notice)**.
- D'après une table des matières de catalogue, le ch. 5 traite des poutres : éléments d'Euler–Bernoulli et de
  Timoshenko. L'élément à interpolation interdépendante y figurerait : **à vérifier**.

---

## B. Articles

Toutes les métadonnées ci-dessous sont **vérifiées sur Crossref**. Aucun texte intégral n'a été lu : ce sont des
articles payants, sans version en accès libre selon Semantic Scholar. Le rôle de chaque article est donné d'après
son titre et la littérature secondaire : **à vérifier sur le texte**.

| Réf. | Article | DOI | Rôle pour Fem3D |
| --- | --- | --- | --- |
| B1 | S. P. Timoshenko, « On the correction for shear of the differential equation for transverse vibrations of prismatic bars », *Phil. Mag.* (6) **41** (245), 744–746, 1921 | [10.1080/14786442108636264](https://doi.org/10.1080/14786442108636264) | Article fondateur de la théorie de Timoshenko (THEORY §3) |
| B2 | G. R. Cowper, « The Shear Coefficient in Timoshenko's Beam Theory », *J. Appl. Mech.* **33** (2), 335–340, 1966 | [10.1115/1.3625046](https://doi.org/10.1115/1.3625046) | Coefficient k du cercle, 6(1+ν)/(7+6ν), dans `BeamSection3D::circle` (§C2) |
| B3 | J. R. Hutchinson, « Shear Coefficients for Timoshenko Beam Theory », *J. Appl. Mech.* **68** (1), 87–92, 2001 | [10.1115/1.1349417](https://doi.org/10.1115/1.1349417) | Recoupement du coefficient de cisaillement (§C2) |
| B4 | J. S. Archer, « Consistent matrix formulations for structural analysis using finite-element techniques », *AIAA J.* **3** (10), 1910–1918, 1965 | [10.2514/3.3279](https://doi.org/10.2514/3.3279) | Matrices et charges « consistantes » (THEORY §4) |
| B5 | R. Davis, R. D. Henshell, G. B. Warburton, « A Timoshenko beam element », *J. Sound Vib.* **22** (4), 475–487, 1972 | [10.1016/0022-460X(72)90457-9](https://doi.org/10.1016/0022-460X(72)90457-9) | Élément de Timoshenko à fonctions de forme dépendant du cisaillement (THEORY §3) |
| B6 | R. Narayanaswami, H. M. Adelman, « Inclusion of Transverse Shear Deformation in Finite Element Displacement Formulations », *AIAA J.* **12** (11), 1613–1614, 1974 | [10.2514/3.49563](https://doi.org/10.2514/3.49563) | Rigidité de Timoshenko par un paramètre de cisaillement (THEORY §3) |
| B7 | A. Tessler, S. B. Dong, « On a hierarchy of conforming Timoshenko beam elements », *Comput. Struct.* **14** (3–4), 335–344, 1981 | [10.1016/0045-7949(81)90017-1](https://doi.org/10.1016/0045-7949(81)90017-1) | Éléments de Timoshenko conformes (THEORY §3) |
| B8 | G. Prathap, G. R. Bhashyam, « Reduced integration and the shear-flexible beam element », *Int. J. Numer. Meth. Eng.* **18** (2), 195–210, 1982 | [10.1002/nme.1620180205](https://doi.org/10.1002/nme.1620180205) | Verrouillage et intégration réduite, que Fem3D évite (THEORY §3) |
| B9 | Z. Friedman, J. B. Kosmatka, « An improved two-node Timoshenko beam finite element », *Comput. Struct.* **47** (3), 473–481, 1993 | [10.1016/0045-7949(93)90243-7](https://doi.org/10.1016/0045-7949(93)90243-7) | Élément à 2 nœuds à fonctions de forme dépendant de Φ, souvent cité comme forme exacte (THEORY §3) |
| B10 | J. N. Reddy, « On locking-free shear deformable beam finite elements », *Comput. Methods Appl. Mech. Eng.* **149** (1–4), 113–132, 1997 | [10.1016/S0045-7825(97)00075-3](https://doi.org/10.1016/S0045-7825(97)00075-3) | Élément à interpolation interdépendante (IIE), sans verrouillage (THEORY §3) |
| B11 | A. Bazoune, Y. A. Khulief, N. G. Stephen, « Shape functions of three-dimensional Timoshenko beam element », *J. Sound Vib.* **259** (2), 473–480, 2003 | [10.1006/jsvi.2002.5122](https://doi.org/10.1006/jsvi.2002.5122) | Fonctions de forme 3D dépendant de Φy et Φz (THEORY §3 et §4) |
| B12 | E. L. Wilson, « The static condensation algorithm », *Int. J. Numer. Meth. Eng.* **8** (1), 198–203, 1974 | [10.1002/nme.1620080115](https://doi.org/10.1002/nme.1620080115) | Condensation statique (THEORY §6) |

Remarques :

- B1 : Crossref donne le titre avec le préfixe « LXVI. » (numéro d'article de la revue).
- B2 : des listes de références écrivent parfois le titre au pluriel, « The shear coefficients… ». Le titre exact,
  vérifié sur Crossref, est au singulier.

---

## C. Correspondance entre formulations et sources

Les marques **[v]** et **[à v.]** désignent les sources vérifiées et à vérifier. Les mentions « contenu » et
« notice » désignent ce qui a été réellement contrôlé.

### C1. Poutre 3D d'Euler–Bernoulli à 12 DDL (THEORY §2)

- Rigidité exacte et fonctions d'Hermite : Przemieniecki ch. 5 [à v.], McGuire et al. [à v.], Weaver et Gere
  [à v.], Cook et al. [à v.], Bathe §5.4 [à v.].
- Recoupement numérique **[v, contenu]** : OpenSees `elasticBeamColumn` en 3D (`$A $E $G $J $Iy $Iz $transfTag`).

### C2. Poutre de Timoshenko exacte à interpolation interdépendante (THEORY §3)

- Origine de la théorie : Timoshenko 1921 (B1).
- Élément : Davis et al. 1972 (B5), Narayanaswami et Adelman 1974 (B6), Tessler et Dong 1981 (B7), Friedman et
  Kosmatka 1993 (B9), Reddy 1997 (B10) et Reddy (A16), Bazoune et al. 2003 (B11) pour le 3D. Toutes
  **[v, notice]**.
- **Vérifié dans le code source d'OpenSees** (`ElasticTimoshenkoBeam3d.cpp`, fonction `setUp()`) :
  - `phiY = 12 E Iy/(L² G Avz)` et `phiZ = 12 E Iz/(L² G Avy)` ;
  - termes `12`, `6L`, `(4+φ)L²` et `(2−φ)L²` multipliés par `E I/(L³(1+φ))`.

  C'est la même forme que la matrice de THEORY §3, avec la même association de Iz à Asy (Avy).
- Verrouillage et intégration réduite (contexte de comparaison) : Prathap et Bhashyam 1982 (B8), Hughes ch. 4 et 5
  [à v.], ZTZ ch. 8 et 13 **[v, table des matières]**.
- Coefficient de cisaillement :
  - Cowper 1966 (B2) ; la formule du cercle, 6(1+ν)/(7+6ν), lui est attribuée par la littérature secondaire. Le
    texte de l'article n'a pas été lu : **à vérifier**. Avec ν = 0,3, k ≈ 0,886.
  - La page du wiki OpenSees (`ElasticTimoshenkoBeam`) conseille 9/10 A pour le cercle plein et 5/6 A pour le
    rectangle **[v, contenu]**. La valeur de Fem3D pour le cercle, 0,886 A, diffère donc d'environ 1,6 % de cette
    recommandation. Ce n'est pas une erreur : il existe plusieurs définitions du coefficient (Cowper, Hutchinson
    B3). Il faut en tenir compte dans les comparaisons avec OpenSees.

### C3. Propriétés de section : torsion de Saint-Venant du rectangle (Roark) et contraintes de RDM (SIGN_CONVENTIONS §5)

Formule de J (code : `J = a b³ [1/3 − 0,21 (b/a)(1 − b⁴/(12 a⁴))]`, a et b étant les côtés complets) :

- La formule classique de Roark s'écrit avec les demi-côtés : `K = a b³ [16/3 − 3,36 (b/a)(1 − b⁴/(12 a⁴))]`. Elle a
  été vue dans un extrait de la 7ᵉ éd., Table 10.1, par une copie OCR secondaire.
- Avec les côtés complets (A = 2a, B = 2b), elle devient `A B³ [1/3 − 0,21 (B/A)(1 − B⁴/(12 A⁴))]`, c'est-à-dire
  exactement la formule du code. **Équivalence algébrique vérifiée** ; coefficients du livre **à vérifier** sur
  l'ouvrage imprimé.

Contrainte maximale :

- La première version du code utilisait `Wt = a b² / (3 + 1,8 b/a)`, soit en demi-côtés `τ = 3T/(8 a b²) · (1 + 0,6 r)`.
- La 7ᵉ éd. de Roark donnerait le polynôme `3T/(8 a b²) · [1 + 0,6095 r + 0,8865 r² − 1,8023 r³ + 0,9100 r⁴]`. Ses
  coefficients restent **à vérifier** sur l'ouvrage imprimé.
- **Vérifié indépendamment de Roark** (2026-10-19) : les deux formes ont été comparées à la solution exacte de
  Saint-Venant (séries de Timoshenko et Goodier, sommées numériquement) pour a/b de 1 à 50.
  - L'ancienne forme surestime Wt jusqu'à 4,4 % (vers a/b ≈ 2,5), donc sous-estime τ_max : ce n'est pas du côté
    de la sécurité.
  - Le polynôme reste à 0,3 % de la solution exacte, et J à 0,5 %.
- **Le code utilise désormais le polynôme.** Le test `Section_RectangleTorsionMatchesExactSeries` le compare aux
  séries exactes, avec une tolérance de 0,6 %. Ce constat a été fait en même temps par l'audit indépendant
  (`../verification/INDEPENDENT_FORMULATION_AUDIT.md`).

Contraintes de RDM, σ = N/A et σ = M c/I : Timoshenko et Gere, *Mechanics of Materials* (A9) [à v.]. La
contrainte de cisaillement de calcul V/As est une convention propre à Fem3D ; la moyenne vraie sur la section vaut
V/A (SIGN_CONVENTIONS §5).

### C4. Forces nodales équivalentes consistantes (THEORY §4)

- f_eq = ∫ Nᵀ q dx : Archer 1965 (B4) [v, notice], ZTZ ch. 3 et 4 **[v, table des matières]**, Bathe §4.2 [v,
  correspondance donnée par MIT OCW].
- Égalité avec l'opposé des forces d'encastrement parfait (Betti), y compris en Timoshenko avec les fonctions de
  forme de B9 à B11 : Przemieniecki, Weaver et Gere, McGuire et al. [à v.].
- Recoupement **[v, contenu]** : OpenSees `eleLoad -beamUniform $Wy $Wz <$Wx>`, dans les axes locaux.
  - La documentation ne décrit **pas** de charge linéairement variable ni partielle : la charge trapézoïdale de
    Fem3D ne peut être comparée à OpenSees qu'en la décomposant.
  - La documentation précise que `eleLoad` ne fonctionne pas avec `geomTransf Corotational` en 3D.

### C5. Efforts internes par équilibre et déformée par intégration exacte (THEORY §5)

- C'est de la RDM classique : Timoshenko et Gere (A9), McGuire et al. (A7) [à v.].
- Le terme de cisaillement dv/dx = θz + V/(G As) relève de la théorie de Timoshenko (B1, B2).
- Pas de référence unique « vérifiée » pour l'algorithme lui-même : il est validé par les tests analytiques du dépôt.

### C6. Condensation statique des rotules My et Mz (THEORY §6)

- Wilson 1974 (B12) [v, notice] ; Przemieniecki ch. 6, « condensation of stiffness matrices » [v, table des
  matières partielle] ; Bathe ch. 8 [à v.] ; McGuire et al., Weaver et Gere (relâchements) [à v.].
- Recoupement **[v, contenu]** : OpenSees `elasticBeamColumn ... <-releasez $relcode> <-releasey $relcode>`.
  - Codes : 0 sans rotule, 1 en i, 2 en j, 3 aux deux extrémités.
  - La documentation 3D n'existe que pour `elasticBeamColumn` : `ElasticTimoshenkoBeam` n'offre pas d'option de
    relâchement dans sa documentation.

### C7. Transformation de repère de type `geomTransf` (SIGN_CONVENTIONS §2)

Vérifié sur le wiki OpenSees (*Linear Transformation*) et sur la documentation de `geomTransf Linear` :

- x va de i vers j ;
- `vecxz` est un vecteur du plan local x-z, qui ne doit pas être parallèle à x ;
- **y = vecxz × x** et **z = x × y**.

C'est exactement la convention de Fem3D. Le texte de la page opensees.github.io donne les opérandes dans un ordre
ambigu ; le wiki, avec sa formule `Vy = Vxz × Vx`, fait foi. Les décalages rigides (`-jntOffset`) existent dans
OpenSees mais pas dans Fem3D.

### C8. Résolution LDLᵀ creuse (THEORY §8)

Vérifié sur la documentation d'Eigen, version nightly 5.0.1-dev :

- `SimplicialLDLT` : factorisation LDLᵀ sans racine carrée, pour une matrice **autoadjointe définie positive** ;
  permutation symétrique de réduction du remplissage, `AMDOrdering<>` par défaut ; `vectorD()` donne D ; `info()`
  indique le succès ;
- `SparseMatrix` : stockage compressé par colonnes ; `setFromTriplets` additionne les doublons, ce qui convient à
  l'assemblage.

La détection des mécanismes de Fem3D, par les pivots de D, exploite le comportement de la factorisation sur une
matrice **non** définie positive, que la documentation ne garantit pas formellement. Ce choix est cohérent avec
Bathe §8.2 [à v.], mais c'est un point à garder en tête : il est couvert par des tests de mécanisme, en kN-m et en
N-mm, et chaque pivot est comparé à la diagonale de son propre DDL (critère indépendant des unités, THEORY §8).

Recoupement théorique : Bathe §8.1 et §8.2 [v, correspondance MIT OCW].

### C9. Ressorts d'appui (THEORY §7, implémentés et validés)

- Équivalent OpenSees **[v, contenu]** : élément `zeroLength` (`-mat`, `-dir` 1 à 6 en axes locaux, `-orient`) et
  contrainte `equalDOF` (nœud retenu, nœud contraint, liste de DDL).
- Théorie (appuis élastiques ajoutés sur la diagonale de K) : Weaver et Gere, McGuire et al. [à v.].
- Validation : cas OpenSees 18 à 20 avec `zeroLength` ([../validation/VALIDATION_OPENSEES.md](../validation/VALIDATION_OPENSEES.md)) et tests analytiques
  (`tests/analytical/test_springs.cpp`).

### C10. Torsion non uniforme et gauchissement de Vlasov (NON implémentés, limite de THEORY §1)

- Vlasov 1961 (A11) ;
- Timoshenko et Gere, *Theory of Elastic Stability*, ch. 5 *torsional buckling* [v, préface] ;
- Trahair 1993 (A12) ; Pilkey 2002 (A14) [à v.] ; McGuire et al. 2ᵉ éd. [à v.].

À documenter dans Fem3D : pour les profils ouverts à parois minces (I, U, cornières), le gauchissement empêché aux
appuis ajoute une rigidité de torsion (terme E·Iw) : GJ seul **sous-estime** donc la rigidité de torsion, et il omet
les contraintes normales de gauchissement (bimoment). Il faudrait alors un élément à 7 DDL par nœud, avec bimoment et
constante Iw ([../limitations/KNOWN_LIMITATIONS.md](../limitations/KNOWN_LIMITATIONS.md) §1). *(Corrigé le 2026-10-19 : la
première rédaction disait « surestime ».)*

---

## D. Licences et réutilisation

**OpenSees.** Le code et la documentation sont © The Regents of the University of California (fichier
`COPYRIGHT`, vérifié). La reproduction et la modification sont autorisées pour un usage **non commercial** (enseignement,
recherche, organismes sans but lucratif) ou pour un **usage interne**. Conséquences pour Fem3D, destiné à être publié sous une licence libre (à choisir par l'auteur) :

- OpenSees sert de **référence de validation** seulement ;
- aucun extrait de son code ne doit être copié dans le moteur ;
- la copie locale de `ElasticTimoshenkoBeam3d.cpp` dans `_downloads/` n'est pas versionnée.

**Eigen.** Licence MPL 2.0 depuis la version 3.1.1 (vérifié sur la page d'accueil d'Eigen) ; compatible avec une
distribution sous licence permissive (MIT, par exemple) si les fichiers d'Eigen restent sous MPL.
