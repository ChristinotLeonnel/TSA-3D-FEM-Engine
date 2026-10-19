# Glossaire

| Terme | Définition dans Fem3D |
|---|---|
| **Appui élastique, ressort nodal** | Liaison linéaire entre un DDL d'un nœud et un point fixe, de raideur k ≥ 0 selon un axe global ; force sur la structure −k·u (`NodalSpring`). |
| **Assemblage** | Somme des matrices élémentaires (en repère global) dans la matrice de la structure, aux positions de leurs DDL. |
| **Benchmark** | Cas de comparaison à OpenSees, numérotés 01 à 20 dans `opensees_benchmarks.cpp`. |
| **Bimoment** | Effort généralisé de la torsion non uniforme (théorie de Vlasov). **Non modélisé** par Fem3D. |
| **Condensation statique** | Élimination des DDL relâchés d'un élément : K_c = K_aa − K_ab K_bb⁻¹ K_ba. |
| **Contrainte de calcul de cisaillement** | V/As ; la moyenne vraie sur la section vaut V/A. |
| **DDL (degré de liberté)** | Translation ou rotation inconnue d'un nœud : 6 par nœud, ordre Ux, Uy, Uz, Rx, Ry, Rz. |
| **DDL libre / imposé** | Inconnu (partition f) / bloqué ou de valeur imposée (partition r). |
| **Efforts d'extrémité** | Forces et moments exercés **sur** l'élément par ses nœuds, repère local ; convention `localForce` d'OpenSees. |
| **Efforts internes** | N, Vy, Vz, T, My, Mz sur la face positive d'une coupe (normale +x local). |
| **Élément exact** | Élément dont les fonctions de forme sont les solutions exactes des équations de la barre : un seul élément par barre donne la solution exacte. |
| **Euler–Bernoulli** | Théorie de poutre sans déformation de cisaillement (sections planes et normales à la ligne moyenne). |
| **Forces nodales équivalentes** | f_eq = ∫ Nᵀ q dx : charges nodales qui produisent le même travail que la charge répartie ; opposées des forces d'encastrement parfait. |
| **Gauchissement** | Déplacement axial non uniforme d'une section tordue. **Non modélisé** (6 DDL par nœud). |
| **Mécanisme** | Structure dont la matrice K_ff est singulière : un mouvement sans déformation est possible. Fem3D refuse le calcul et désigne les DDL. |
| **Mode comparaison** | Dans TSA, calcul par Fem3D et par OpenSees ; résultats affichés de Fem3D, écarts en table, sans fusion. |
| **Moteur secondaire** | Moteur sélectionnable dans TSA qui n'est pas le moteur par défaut (OpenSees). |
| **Niveau de maturité** | Niveau 1 à 5 de qualification d'une fonction du moteur ([MATURITY_LEVELS.md](../validation/MATURITY_LEVELS.md)). |
| **Niveau pédagogique** | Étape 0 à 6 du [parcours d'apprentissage](../learning/index.md) ; sans rapport avec la maturité. |
| **Permalien** | Lien GitHub vers un fichier à un commit précis (hash complet) : stable quand `main` évolue. |
| **Φ (Phi)** | Paramètre de cisaillement de Timoshenko : Φy = 12 E Iz / (G Asy L²), Φz = 12 E Iy / (G Asz L²). |
| **Pivot relatif** | Pivot de la factorisation divisé par le terme diagonal de son propre DDL ; ≤ 1e-12 : mécanisme ; < 1e-8 : avertissement. |
| **Quasi-mécanisme** | Structure stable mais très mal conditionnée ; calculée avec un avertissement. |
| **Repère local** | x de i vers j, y = (v_xz × x)/‖…‖, z = x × y (convention `geomTransf` d'OpenSees). |
| **Résidu de résolution** | ‖K_ff U_f − F‖ / ‖F‖ ; avertissement au-delà de 1e-8. |
| **Rotule** | Relâchement d'un moment de flexion (My ou Mz) à une extrémité d'élément (`EndReleases`). |
| **Secours (fallback)** | Dans TSA, calcul confié à OpenSees d'un modèle hors du périmètre de Fem3D, seulement si l'utilisateur l'a choisi ; jamais pour un mécanisme. |
| **Saint-Venant (torsion de)** | Torsion uniforme, rigidité GJ/L, section libre de gauchir. |
| **Timoshenko** | Théorie de poutre avec déformation de cisaillement transversal (aires As). |
| **v_xz** | Vecteur du plan local x-z qui oriente la section. |
