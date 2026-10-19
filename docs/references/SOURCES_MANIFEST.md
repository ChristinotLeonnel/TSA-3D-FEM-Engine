# Manifeste des sources

Toutes les sources ont été consultées le **2026-10-19**.

## Statuts

| Statut | Signification |
| --- | --- |
| **Fichier téléchargé** | Une copie locale existe dans `_downloads/`. Ce dossier n'est pas versionné : son `.gitignore` contient `*` et `!.gitignore`. |
| **Consulté intégralement** | La page ou le document en ligne a été lu en entier. |
| **Consulté partiellement** | Seule une partie a été lue : un extrait, une section, une table des matières. |
| **Notice seulement** | Seules les métadonnées ont été vérifiées (Crossref, éditeur, catalogue). Le contenu n'a **pas** été lu. |

## Bilan

| Statut | Sources |
| --- | --- |
| Fichier téléchargé | 14, toutes des documentations officielles d'OpenSees et d'Eigen |
| Consulté intégralement, sans téléchargement | 1 |
| Consulté partiellement | 3 |
| Notice seulement | 28 : 16 ouvrages et 12 articles |
| **Total** | **46** |

Aucun ouvrage ni article n'a été téléchargé :

- McGuire, Gallagher, Ziemian (n° 21) est diffusé gratuitement par les auteurs, mais son lien est protégé par une
  vérification anti-robot (Cloudflare), qui n'a pas été contournée ;
- tous les articles sont payants ;
- la copie de Vlasov (n° 29) sur archive.org a un statut juridique non établi.

## Liste des sources

Les colonnes « THEORY » et « SIGN » renvoient aux sections de [`../theory/THEORY.md`](../theory/THEORY.md) et de
[`../theory/SIGN_CONVENTIONS.md`](../theory/SIGN_CONVENTIONS.md).

| # | Titre exact | Auteur ou organisme | Édition, année | URL officielle | Consulté le | Licence et redistribution | Statut | Parties du moteur |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Elastic Beam Column Element | OpenSees (UC Berkeley) | Doc. en ligne, © 2022 | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/elements/elasticBeamColumn.html> | 2026-10-19 | © Regents of the University of California : reproduction non commerciale ou interne seulement | Fichier téléchargé (consulté intégralement) | THEORY §2, §6 (`-releasey`/`-releasez`) ; validation |
| 2 | Elastic Timoshenko Beam Column Element | OpenSeesWiki (UC Berkeley) | Wiki, rév. du 25 mars 2018 | <https://opensees.berkeley.edu/wiki/index.php/Elastic_Timoshenko_Beam_Column_Element> | 2026-10-19 | © Regents et contributeurs ; conditions du wiki non précisées : pas de redistribution | Fichier téléchargé (consulté intégralement) | THEORY §3 ; aires de cisaillement (`BeamSection3D`) |
| 3 | Elastic Timoshenko Beam Column Element | Documentation OpenSeesPy | Doc. en ligne, version *latest* | <https://openseespydoc.readthedocs.io/en/latest/src/ElasticTimoshenkoBeam.html> | 2026-10-19 | Conditions non indiquées : pas de redistribution | Fichier téléchargé (consulté intégralement) | THEORY §3 ; validation |
| 4 | ElasticTimoshenkoBeam3d.cpp | A. Schellenberg (2013), rév. P. Ranchal (2020) ; OpenSees | Branche `master` | <https://github.com/OpenSees/OpenSees/blob/master/SRC/element/elasticBeamColumn/ElasticTimoshenkoBeam3d.cpp> | 2026-10-19 | Licence OpenSees (n° 11) : **ne pas copier dans Fem3D (licence libre à choisir)** | Fichier téléchargé (consulté partiellement : en-tête et `setUp()`) | THEORY §3 (Φ et rigidité) |
| 5 | Geometric Transformation Command | OpenSees | Doc. en ligne | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/geomTransf.html> | 2026-10-19 | Comme n° 1 | Fichier téléchargé (consulté intégralement) | SIGN §2 |
| 6 | Linear Transformation | OpenSees | Doc. en ligne | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/geomTransf/Linear.html> | 2026-10-19 | Comme n° 1 | Fichier téléchargé (consulté intégralement) | SIGN §2 |
| 7 | Linear Transformation | OpenSeesWiki | Wiki | <https://opensees.berkeley.edu/wiki/index.php/Linear_Transformation> | 2026-10-19 | Comme n° 2 | Fichier téléchargé (consulté intégralement) | SIGN §2 (y = vecxz × x) |
| 8 | eleLoad Command | OpenSees | Doc. en ligne | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/eleLoad.html> | 2026-10-19 | Comme n° 1 | Fichier téléchargé (consulté intégralement) | THEORY §4 ; SIGN §4 |
| 9 | ZeroLength Element | OpenSees | Doc. en ligne | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/elements/zeroLength.html> | 2026-10-19 | Comme n° 1 | Fichier téléchargé (consulté intégralement) | Ressorts d'appui (THEORY §7) |
| 10 | EqualDOF command | OpenSeesWiki | Wiki | <https://opensees.berkeley.edu/wiki/index.php/EqualDOF_command> | 2026-10-19 | Comme n° 2 | Fichier téléchargé (consulté intégralement) | Ressorts d'appui, liaisons |
| 11 | COPYRIGHT | The Regents of the University of California | 1999-2020 | <https://github.com/OpenSees/OpenSees/blob/master/COPYRIGHT> | 2026-10-19 | Texte de la licence lui-même | Fichier téléchargé (consulté intégralement) | Licences : validation seulement |
| 12 | Eigen::SimplicialLDLT Class Template Reference | Projet Eigen | Doc. nightly 5.0.1-dev | <https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1SimplicialLDLT.html> | 2026-10-19 | MPL 2.0 | Fichier téléchargé (consulté intégralement) | THEORY §8 |
| 13 | Eigen: Sparse matrix manipulations | Projet Eigen | Doc. nightly 5.0.1-dev | <https://libeigen.gitlab.io/eigen/docs-nightly/group__TutorialSparse.html> | 2026-10-19 | MPL 2.0 | Fichier téléchargé (consulté intégralement) | THEORY §8 (assemblage, solveurs) |
| 14 | Eigen::SparseMatrix Class Template Reference | Projet Eigen | Doc. nightly 5.0.1-dev | <https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1SparseMatrix.html> | 2026-10-19 | MPL 2.0 | Fichier téléchargé (non relu en détail) | THEORY §8 |
| 15 | Eigen (page d'accueil) | Projet Eigen | Version 5.0.0 du 30 septembre 2025 | <https://libeigen.gitlab.io/> | 2026-10-19 | MPL 2.0 depuis la 3.1.1 | Consulté partiellement (licence, version) | THEORY §8 ; licences |
| 16 | The Finite Element Method: Its Basis and Fundamentals | O. C. Zienkiewicz, R. L. Taylor, J. Z. Zhu | 7ᵉ éd., Butterworth-Heinemann, 2013 ; ISBN 978-1-85617-633-0 | — (notice : <https://vufind.katalog.k.utb.cz/Record/kn-ocn857713191/TOC>) | 2026-10-19 | © Elsevier : pas de redistribution | Notice seulement (table des matières vue au catalogue UTB) | THEORY §3, §4 |
| 17 | The Finite Element Method for Solid and Structural Mechanics | O. C. Zienkiewicz, R. L. Taylor, D. D. Fox | 7ᵉ éd., Butterworth-Heinemann, 2014 ; ISBN 978-1-85617-634-7 (à vérifier) | — | 2026-10-19 | © Elsevier | Notice seulement | THEORY §2, §3 |
| 18 | Finite Element Procedures | K.-J. Bathe | 2ᵉ éd., K.-J. Bathe, Watertown MA, 2014 ; ISBN 978-0-9790049-5-7 | <https://web.mit.edu/kjb/www/> | 2026-10-19 | © K.-J. Bathe ; aucun PDF gratuit trouvé | Notice seulement (§4.2, §8.1-8.2 via MIT OCW) | THEORY §4, §6, §8 |
| 19 | The Finite Element Method: Linear Static and Dynamic Finite Element Analysis | T. J. R. Hughes | Dover, 2000 ; ISBN 978-0-486-41181-1 | — | 2026-10-19 | © : pas de redistribution | Notice seulement | THEORY §3 (verrouillage) |
| 20 | Concepts and Applications of Finite Element Analysis | R. D. Cook, D. S. Malkus, M. E. Plesha, R. J. Witt | 4ᵉ éd., Wiley, 2002 ; ISBN 978-0-471-35605-9 | — | 2026-10-19 | © Wiley | Notice seulement | THEORY §2, §6, §8 |
| 21 | Matrix Structural Analysis, 2nd Edition | W. McGuire, R. H. Gallagher, R. D. Ziemian | 2ᵉ éd., Wiley, 2000 ; ISBN 978-0-471-12918-9 ; réimpr. 2015, ISBN 978-1-5075-8513-9 | <https://digitalcommons.bucknell.edu/books/7/> | 2026-10-19 | Diffusion gratuite par les auteurs (dépôt Bucknell, MASTAN2) ; **aucune licence explicite** : lien seulement, pas de redistribution | Notice seulement (pages du dépôt et de MASTAN2 lues ; PDF non téléchargé, protégé par Cloudflare) | THEORY §1-§8 ; SIGN §2 |
| 22 | Theory of Matrix Structural Analysis | J. S. Przemieniecki | McGraw-Hill, 1968 ; Dover, 1985 ; ISBN 978-0-486-64948-1 | <https://store.doverpublications.com/collections/paperback/products/9780486649481> | 2026-10-19 | © | Notice seulement (contenu du ch. 6 vu dans une notice) | THEORY §2, §3, §6 ; SIGN §2 |
| 23 | Matrix Analysis of Framed Structures | W. Weaver Jr., J. M. Gere | 3ᵉ éd., Van Nostrand Reinhold, 1990 ; ISBN 0-442-23485-6 | — | 2026-10-19 | © | Notice seulement | THEORY §4, §6 ; ressorts |
| 24 | Mechanics of Materials | S. P. Timoshenko, J. M. Gere | 1ʳᵉ éd., Van Nostrand Reinhold, 1972 ; ISBN 0-442-22637-3 | — | 2026-10-19 | © | Notice seulement | SIGN §5 ; THEORY §5 |
| 25 | Theory of Elastic Stability | S. P. Timoshenko, J. M. Gere | 2ᵉ éd., McGraw-Hill, 1961 ; Dover, 2009 ; ISBN 978-0-486-47207-2 | — | 2026-10-19 | © | Notice seulement (préface vue en extrait : ch. 5 et 6) | THEORY §1 (limite de Vlasov) |
| 26 | Flexural-Torsional Buckling of Structures | N. S. Trahair | E & FN Spon / Chapman & Hall, 1993 ; ISBN 0-419-17110-5 ; CRC, ISBN 0-8493-7763-3 | — | 2026-10-19 | © | Notice seulement | THEORY §1 (limite) |
| 27 | Roark's Formulas for Stress and Strain | W. C. Young, R. G. Budynas, A. M. Sadegh | 9ᵉ éd., McGraw-Hill, 2020 ; ISBN 978-1-260-45375-1 | <https://www.mheducation.com/highered/mhp/product/roark-s-formulas-stress-strain-9e.html> | 2026-10-19 | © McGraw-Hill | Notice seulement (formule de J vue dans un extrait OCR secondaire de la 7ᵉ éd.) | `BeamSection3D::rectangle` (J, Wt) ; SIGN §5 |
| 28 | Analysis and Design of Elastic Beams: Computational Methods | W. D. Pilkey | Wiley, 2002 ; ISBN 978-0-471-38152-5 | <https://doi.org/10.1002/9780470172667> | 2026-10-19 | © Wiley | Notice seulement | Sections ; THEORY §1 (limite) |
| 29 | Thin-Walled Elastic Beams | V. Z. Vlasov | 2ᵉ éd., IPST, Jérusalem, 1961 (TT 61-11400) | — | 2026-10-19 | Statut non établi ; copie archive.org sans mention de droits, **non téléchargée** | Notice seulement | THEORY §1 (limite) |
| 30 | Formulas for Stress, Strain, and Structural Matrices | W. D. Pilkey | 2ᵉ éd., Wiley, 2004 ; ISBN 978-0-471-03221-2 | <https://doi.org/10.1002/9780470172681> | 2026-10-19 | © Wiley | Notice seulement | Sections ; THEORY §2 |
| 31 | An Introduction to the Finite Element Method | J. N. Reddy | 4ᵉ éd., McGraw-Hill, 2019 ; ISBN 978-1-259-86190-1 | — | 2026-10-19 | © McGraw-Hill | Notice seulement | THEORY §3 |
| 32 | On the correction for shear of the differential equation for transverse vibrations of prismatic bars | S. P. Timoshenko | *Phil. Mag.* 41(245), 1921 | <https://doi.org/10.1080/14786442108636264> | 2026-10-19 | Accès payant (Taylor & Francis) ; peut-être du domaine public aux États-Unis (publié en 1921) : à vérifier, pas de redistribution | Notice seulement | THEORY §3 |
| 33 | The Shear Coefficient in Timoshenko's Beam Theory | G. R. Cowper | *J. Appl. Mech.* 33(2), 1966 | <https://doi.org/10.1115/1.3625046> | 2026-10-19 | © ASME, payant | Notice seulement | `BeamSection3D::circle` (k du cercle) ; THEORY §3 |
| 34 | Shear Coefficients for Timoshenko Beam Theory | J. R. Hutchinson | *J. Appl. Mech.* 68(1), 2001 | <https://doi.org/10.1115/1.1349417> | 2026-10-19 | © ASME, payant | Notice seulement | Aires de cisaillement |
| 35 | Consistent matrix formulations for structural analysis using finite-element techniques | J. S. Archer | *AIAA J.* 3(10), 1965 | <https://doi.org/10.2514/3.3279> | 2026-10-19 | © AIAA, payant | Notice seulement | THEORY §4 |
| 36 | A Timoshenko beam element | R. Davis, R. D. Henshell, G. B. Warburton | *J. Sound Vib.* 22(4), 1972 | <https://doi.org/10.1016/0022-460X(72)90457-9> | 2026-10-19 | © Elsevier, payant | Notice seulement | THEORY §3 |
| 37 | Inclusion of Transverse Shear Deformation in Finite Element Displacement Formulations | R. Narayanaswami, H. M. Adelman | *AIAA J.* 12(11), 1974 | <https://doi.org/10.2514/3.49563> | 2026-10-19 | © AIAA, payant | Notice seulement | THEORY §3 |
| 38 | The static condensation algorithm | E. L. Wilson | *IJNME* 8(1), 1974 | <https://doi.org/10.1002/nme.1620080115> | 2026-10-19 | © Wiley, payant | Notice seulement | THEORY §6 |
| 39 | On a hierarchy of conforming Timoshenko beam elements | A. Tessler, S. B. Dong | *Comput. Struct.* 14(3-4), 1981 | <https://doi.org/10.1016/0045-7949(81)90017-1> | 2026-10-19 | © Elsevier, payant | Notice seulement | THEORY §3 |
| 40 | Reduced integration and the shear-flexible beam element | G. Prathap, G. R. Bhashyam | *IJNME* 18(2), 1982 | <https://doi.org/10.1002/nme.1620180205> | 2026-10-19 | © Wiley, payant | Notice seulement | THEORY §3 (verrouillage) |
| 41 | An improved two-node Timoshenko beam finite element | Z. Friedman, J. B. Kosmatka | *Comput. Struct.* 47(3), 1993 | <https://doi.org/10.1016/0045-7949(93)90243-7> | 2026-10-19 | © Elsevier, payant | Notice seulement | THEORY §3, §4 |
| 42 | On locking-free shear deformable beam finite elements | J. N. Reddy | *CMAME* 149(1-4), 1997 | <https://doi.org/10.1016/S0045-7825(97)00075-3> | 2026-10-19 | © Elsevier, payant | Notice seulement | THEORY §3 |
| 43 | Shape functions of three-dimensional Timoshenko beam element | A. Bazoune, Y. A. Khulief, N. G. Stephen | *J. Sound Vib.* 259(2), 2003 | <https://doi.org/10.1006/jsvi.2002.5122> | 2026-10-19 | © Elsevier, payant | Notice seulement | THEORY §3, §4 |
| 44 | 2.092 Finite Element Analysis of Solids and Fluids I : Readings | MIT OpenCourseWare (K.-J. Bathe) | Automne 2009 | <https://ocw.mit.edu/courses/2-092-finite-element-analysis-of-solids-and-fluids-i-fall-2009/pages/readings> | 2026-10-19 | CC BY-NC-SA 4.0 | Consulté intégralement | Correspondance avec Bathe (THEORY §4, §8) |
| 45 | RES.2-002 Finite Element Procedures for Solids and Structures | MIT OpenCourseWare (K.-J. Bathe) | Printemps 2010 | <https://ocw.mit.edu/courses/res-2-002-finite-element-procedures-for-solids-and-structures-spring-2010/> | 2026-10-19 | CC BY-NC-SA 4.0 | Consulté partiellement (page d'accueil) | Ressource d'enseignement générale |
| 46 | Structural Mechanics Module User's Guide : *Shape Functions* | COMSOL | Doc. 6.3 | <https://doc.comsol.com/6.3/doc/com.comsol.help.sme/sme_ug_theory.06.146.html> | 2026-10-19 | © COMSOL, tous droits réservés | Consulté partiellement (formules en images, non lues) | THEORY §3 (recoupement qualitatif) |

## Contenu de `_downloads/`

`.gitignore` (`*`, `!.gitignore`), puis les copies locales des sources n° 1 à 14 :

```text
opensees_elasticBeamColumn.html              opensees_geomTransf.html
opensees_geomTransf_Linear.html              opensees_eleLoad.html
opensees_zeroLength.html                     openseespy_ElasticTimoshenkoBeam.html
openseeswiki_Elastic_Timoshenko_Beam_Column_Element.html
openseeswiki_Linear_Transformation.html      openseeswiki_EqualDOF_command.html
OpenSees_ElasticTimoshenkoBeam3d.cpp         OpenSees_COPYRIGHT.txt
eigen_SimplicialLDLT.html                    eigen_TutorialSparse.html
eigen_SparseMatrix.html
```
