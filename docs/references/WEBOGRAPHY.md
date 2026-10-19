# Webographie du moteur de poutres 3D (Fem3D)

Toutes les pages ont été consultées le **2026-10-19**. La colonne « Vérifié » indique ce qui a réellement été lu.
Le signe ⬇ marque une page téléchargée dans `_downloads/` ; ce dossier n'est pas versionné.

## 1. Documentation officielle

### OpenSees

| Ressource | URL | Vérifié |
| --- | --- | --- |
| `elasticBeamColumn` ⬇ | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/elements/elasticBeamColumn.html> | Syntaxe 3D (`$A $E $G $J $Iy $Iz $transfTag`) ; `-releasez`/`-releasey` avec codes 0 à 3 (aucune rotule, en i, en j, aux deux extrémités) ; `-mass`, `-cMass`. La page ne cite aucune référence. |
| `ElasticTimoshenkoBeam` (wiki) ⬇ | <https://opensees.berkeley.edu/wiki/index.php/Elastic_Timoshenko_Beam_Column_Element> | Syntaxe 3D `$E $G $A $Jx $Iy $Iz $Avy $Avz $transfTag`. Aires conseillées : 5/6 A (rectangle), 9/10 A (cercle), aire de l'âme (I). Auteur du code : A. Schellenberg. Aucune référence bibliographique. |
| `ElasticTimoshenkoBeam` (OpenSeesPy) ⬇ | <https://openseespydoc.readthedocs.io/en/latest/src/ElasticTimoshenkoBeam.html> | Arguments 3D identiques ; masse concentrée par défaut. La page `opensees.github.io/.../ElasticTimoshenkoBeam.html` renvoie une erreur 404. |
| Source `ElasticTimoshenkoBeam3d.cpp` ⬇ | <https://github.com/OpenSees/OpenSees/blob/master/SRC/element/elasticBeamColumn/ElasticTimoshenkoBeam3d.cpp> | `setUp()` : `phiY = 12EIy/(L²G Avz)`, `phiZ = 12EIz/(L²G Avy)`, termes 12, 6L, (4+φ)L², (2−φ)L² pondérés par 1/(1+φ). Ces termes sont identiques à THEORY §3. |
| `geomTransf` (vue d'ensemble) ⬇ | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/geomTransf.html> | Rôle de l'objet `CrdTransf` ; sous-pages Linear, PDelta et Corotational02. |
| `geomTransf Linear` ⬇ | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/geomTransf/Linear.html> | Syntaxe 3D avec `vecxz` ; `-jntOffset` en repère global. Le texte est ambigu sur l'ordre du produit vectoriel. |
| *Linear Transformation* (wiki) ⬇ | <https://opensees.berkeley.edu/wiki/index.php/Linear_Transformation> | Formules `Vy = Vxz × Vx` et `Vz = Vx × Vy`, conformes à SIGN_CONVENTIONS §2. Donne aussi des exemples de `vecxz`. |
| `eleLoad` ⬇ | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/eleLoad.html> | `-beamUniform $Wy $Wz <$Wx>` et `-beamPoint $Py $Pz $xL <$Px>` en axes locaux. Pas de charge linéaire ni partielle documentée. Ne fonctionne pas avec la transformation Corotational en 3D. |
| `zeroLength` ⬇ | <https://opensees.github.io/OpenSeesDocumentation/user/manual/model/elements/zeroLength.html> | `-mat`, `-dir` (1 à 3 : translations, 4 à 6 : rotations, en axes locaux), `-orient $x $yp`, `-doRayleigh`. |
| `equalDOF` (wiki) ⬇ | <https://opensees.berkeley.edu/wiki/index.php/EqualDOF_command> | `equalDOF $rNodeTag $cNodeTag $dof1 ...` : contrainte multipoint, nœud retenu et nœud contraint. La page `opensees.github.io` correspondante renvoie une erreur 404 (deux URL essayées). |
| Licence `COPYRIGHT` ⬇ | <https://github.com/OpenSees/OpenSees/blob/master/COPYRIGHT> | Usage non commercial (enseignement, recherche) ou interne seulement. Pas de reprise de code dans Fem3D (licence libre à choisir par l'auteur). |

### Eigen

| Ressource | URL | Vérifié |
| --- | --- | --- |
| `SimplicialLDLT` ⬇ | <https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1SimplicialLDLT.html> | LDLᵀ sans racine carrée pour une matrice autoadjointe définie positive ; `AMDOrdering<>` par défaut ; triangle `Lower` par défaut ; `vectorD()`, `info()`. Version de la page : 5.0.1-dev. L'ancienne URL `eigen.tuxfamily.org/dox/...` redirige ici. |
| Didacticiel *Sparse matrix manipulations* ⬇ | <https://libeigen.gitlab.io/eigen/docs-nightly/group__TutorialSparse.html> | Stockage compressé par colonnes ; `setFromTriplets`, qui additionne les doublons ; solveurs directs SimplicialLLT, SimplicialLDLT (matrices SPD), SparseLU, SparseQR. |
| Classe `SparseMatrix` ⬇ | <https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1SparseMatrix.html> | Page téléchargée, non relue en détail. |
| Page d'accueil d'Eigen | <https://libeigen.gitlab.io/> | Licence MPL 2.0 depuis la version 3.1.1 ; dernière version publiée annoncée : 5.0.0, le 30 septembre 2025. |

## 2. Ouvrages : pages des éditeurs et notices

| Ouvrage | URL | Vérifié |
| --- | --- | --- |
| McGuire, Gallagher, Ziemian, *Matrix Structural Analysis* 2ᵉ éd. : dépôt Bucknell | <https://digitalcommons.bucknell.edu/books/7/> | Notice (2000, *Faculty Books* 7) ; lien PDF de 65,3 Mo : <https://digitalcommons.bucknell.edu/cgi/viewcontent.cgi?article=1006&context=books>. Aucune licence explicite. Le PDF est derrière une vérification anti-robot Cloudflare, **non contournée** : il n'est pas téléchargé. |
| Même ouvrage : site MASTAN2 | <https://www.mastan2.com/textbook.html> | Mention « Download E-Book (Free) » vers le dépôt Bucknell ; version imprimée à 39,99 USD. |
| *Roark's Formulas for Stress and Strain*, 9ᵉ éd. | <https://www.mheducation.com/highered/mhp/product/roark-s-formulas-stress-strain-9e.html> | Titre ; Budynas et Sadegh ; 9 mars 2020 ; ISBN 978-1-260-45375-1, e-book 978-1-260-45376-8. Pas de table des matières. |
| Przemieniecki, *Theory of Matrix Structural Analysis* (Dover) | <https://store.doverpublications.com/collections/paperback/products/9780486649481> | ISBN 978-0-486-64948-1 seulement : ni auteur ni table des matières sur la page. |
| Zienkiewicz, Taylor, Zhu, 7ᵉ éd. : table des matières | <https://vufind.katalog.k.utb.cz/Record/kn-ocn857713191/TOC> | Liste des 18 chapitres et ISBN (catalogue de bibliothèque). |
| Zienkiewicz, Taylor, Fox, 7ᵉ éd. : table des matières | <https://libexplorer.upnm.edu.my/home/Record/99031/TOC> | 15 chapitres, dont le ch. 9 « rods ». La notice paraît imparfaite. |
| Vlasov, *Thin-Walled Elastic Beams* (notice) | <https://libcat.library.tamu.edu/Record/in00000694947> | Vu par un moteur de recherche : 2ᵉ éd., IPST, Jérusalem, 1961, TT 61-11400. |
| Vlasov : copie archive.org | <https://archive.org/details/v.-z.-vlasov-thin-walled-elastic-beams-1961> | Existence de l'objet seulement (API de recherche) ; aucun droit indiqué. **Non téléchargé**. |
| Bathe : site de l'auteur | <https://web.mit.edu/kjb/www/> | Les PDF `FEP_2nd_Edition_*_Printing.pdf` testés renvoient une erreur 404 : pas d'accès gratuit constaté. |
| Wiley : notices de livres par DOI | <https://doi.org/10.1002/9780470172667>, <https://doi.org/10.1002/9780470172681> | Métadonnées des deux livres de Pilkey, vérifiées sur Crossref. |

Les autres ouvrages (Hughes, Cook et al., Weaver et Gere, Timoshenko et Gere, Trahair, Reddy) n'ont été vérifiés
que par des catalogues de bibliothèque et des notices de libraires, lus dans les résultats de recherche. Voir
[`SOURCES_MANIFEST.md`](SOURCES_MANIFEST.md).

## 3. Articles scientifiques (DOI)

Pour chaque article, les métadonnées ont été vérifiées sur l'API Crossref : `https://api.crossref.org/works/<DOI>`
ou une recherche bibliographique. Aucun texte intégral n'a été lu : les pages des éditeurs (ASME, Elsevier)
renvoient une erreur 403 aux requêtes automatisées. Semantic Scholar n'indique aucune version en accès libre pour
B9 et B10.

| Article | DOI |
| --- | --- |
| Timoshenko 1921, *Phil. Mag.* 41 | <https://doi.org/10.1080/14786442108636264> |
| Archer 1965, *AIAA J.* 3 | <https://doi.org/10.2514/3.3279> |
| Cowper 1966, *J. Appl. Mech.* 33 | <https://doi.org/10.1115/1.3625046> |
| Davis, Henshell, Warburton 1972, *J. Sound Vib.* 22 | <https://doi.org/10.1016/0022-460X(72)90457-9> |
| Narayanaswami, Adelman 1974, *AIAA J.* 12 | <https://doi.org/10.2514/3.49563> |
| Wilson 1974, *IJNME* 8 | <https://doi.org/10.1002/nme.1620080115> |
| Tessler, Dong 1981, *Comput. Struct.* 14 | <https://doi.org/10.1016/0045-7949(81)90017-1> |
| Prathap, Bhashyam 1982, *IJNME* 18 | <https://doi.org/10.1002/nme.1620180205> |
| Friedman, Kosmatka 1993, *Comput. Struct.* 47 | <https://doi.org/10.1016/0045-7949(93)90243-7> |
| Reddy 1997, *CMAME* 149 | <https://doi.org/10.1016/S0045-7825(97)00075-3> |
| Hutchinson 2001, *J. Appl. Mech.* 68 | <https://doi.org/10.1115/1.1349417> |
| Bazoune, Khulief, Stephen 2003, *J. Sound Vib.* 259 | <https://doi.org/10.1006/jsvi.2002.5122> |

## 4. Ressources secondaires

| Ressource | URL | Vérifié |
| --- | --- | --- |
| MIT OCW 2.092, *Finite Element Analysis of Solids and Fluids I* (2009) : lectures | <https://ocw.mit.edu/courses/2-092-finite-element-analysis-of-solids-and-fluids-i-fall-2009/pages/readings> | Correspondance entre séances et sections de Bathe : §4.2 (formulation EF), §8.1 et §8.2 (élimination de Gauss). |
| MIT OCW RES.2-002, K.-J. Bathe, *Finite Element Procedures for Solids and Structures* | <https://ocw.mit.edu/courses/res-2-002-finite-element-procedures-for-solids-and-structures-spring-2010/> | Licence CC BY-NC-SA 4.0 ; vidéos de 1982 et 1986. Pas de liste de séances détaillée sur la page d'accueil. |
| COMSOL, *Structural Mechanics User's Guide*, page *Shape Functions* (poutre) | <https://doc.comsol.com/6.3/doc/com.comsol.help.sme/sme_ug_theory.06.146.html> | Fonctions de Timoshenko modifiées selon le rapport flexion/cisaillement dans chaque direction principale ; Hermite exact sans charge répartie variable. Aucune référence citée ; formules en images, non lues. |
| Semantic Scholar (API) | <https://api.semanticscholar.org/graph/v1/paper/DOI:10.1016/S0045-7825(97)00075-3> | Statut d'accès de Reddy 1997 et Friedman–Kosmatka 1993 : « CLOSED », sans PDF libre. |
