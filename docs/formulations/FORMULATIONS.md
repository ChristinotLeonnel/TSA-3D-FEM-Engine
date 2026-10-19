# Formulations

Catalogue des formulations du moteur : hypothèses, mise en œuvre, références et état de validation. Les équations
sont dans [../theory/THEORY.md](../theory/THEORY.md) ; les sources dans [../references/REFERENCES.md](../references/REFERENCES.md).

| Formulation | Statut | Théorie | Validation |
|---|---|---|---|
| Poutre 3D d'Euler–Bernoulli | implémentée | §2 | niveau 4 |
| Poutre 3D de Timoshenko (élément exact) | implémentée | §3 | niveau 4 |
| Charges nodales | implémentées | — | niveau 4 |
| Charges réparties linéaires, locales ou globales, sur toute la longueur | implémentées | §4, §5 | niveau 4 (uniformes) ; niveau 2 (linéaires) |
| Rotules de flexion My, Mz | implémentées | §6 | niveau 4 |
| Appuis élastiques nodaux | implémentés | §7 | niveau 4 |
| Barre de treillis (effort normal seul) | **non implémentée** | — | — |
| Charges ponctuelles, partielles, moments répartis | **non implémentés** | — | — |
| Torsion non uniforme, gauchissement (Vlasov) | **non implémentée** | — | — |

Niveaux : [../validation/QUALIFICATION.md](../validation/QUALIFICATION.md).

## 1. Poutre d'Euler–Bernoulli 3D (`EulerBernoulli3D`)

- Deux nœuds, 12 DDL. Sections planes et normales à la ligne moyenne : cisaillement transversal négligé.
- Rigidité **exacte** pour une barre prismatique : axiale EA/L, torsion GJ/L, flexion par les fonctions cubiques
  d'Hermite, solutions exactes de EI w'''' = 0.
- Équivalente à `elasticBeamColumn` d'OpenSees (vérifié terme à terme par l'audit indépendant et par les benchmarks).
- Références : Przemieniecki, *Theory of Matrix Structural Analysis* ; McGuire, Gallagher et Ziemian, *Matrix
  Structural Analysis* ; Bathe, *Finite Element Procedures* (chapitres à vérifier, voir REFERENCES.md).

## 2. Poutre de Timoshenko 3D (`Timoshenko3D`)

- Déformation de cisaillement prise en compte par plan : Φy = 12 E Iz / (G Asy L²), Φz = 12 E Iy / (G Asz L²).
- **Élément exact à interpolation interdépendante** : les fonctions de forme sont les solutions exactes de
  l'équation de Timoshenko pour une barre sans charge ; aucun verrouillage en cisaillement, aucune intégration réduite.
  Φ → 0 redonne Euler–Bernoulli.
- Équivalente à `ElasticTimoshenkoBeam` d'OpenSees (même Φ, mêmes termes (4 + Φ) et (2 − Φ), vérifiés dans le code
  source d'OpenSees).
- Aires de cisaillement : 5/6 A pour le rectangle ; coefficient de Cowper k = 6 (1 + ν) / (7 + 6 ν) pour le cercle.
- Références : Timoshenko (1921) ; Davis, Henshell et Warburton (1972) ; Friedman et Kosmatka (1993) ; Reddy (1997) ;
  Bazoune, Khulief et Stephen (2003) pour le 3D ; Cowper (1966) pour le coefficient.

## 3. Charges

- **Nodales** : forces et moments en repère global.
- **Réparties** : linéaires de q(0) à q(L) sur toute la longueur, par unité de longueur réelle, en repère local ou
  global (projection q_local = R q_global). Forces nodales équivalentes f_eq = ∫ Nᵀ q dx par Gauss à 4 points,
  exactes ; par le théorème de Betti, ce sont exactement les forces d'encastrement parfait, **pour les deux
  formulations** (démontré et vérifié par l'audit indépendant). La charge est reprise exactement dans les efforts
  internes et la déformée, qui sont des polynômes exacts.
- **Locales et globales** : une charge globale sur une barre inclinée est donnée par unité de longueur de la barre,
  jamais par unité de longueur projetée.

## 4. Relâchements d'extrémité (`EndReleases`)

- Rotules de flexion My et/ou Mz, en i et/ou en j. Condensation statique des DDL locaux relâchés, commune aux deux
  formulations : K_c = K_aa − K_ab K_bb⁻¹ K_ba, f_c = f_a − K_ab K_bb⁻¹ f_b ; reconstitution exacte des rotations de
  la barre à la rotule, d_b = K_bb⁻¹ (f_b − K_ba d_a).
- Pas de relâchement de N, V ou T ; pas de rotule semi-rigide.
- Référence : Wilson, *The static condensation algorithm* (1974) ; Przemieniecki ch. 6.

## 5. Appuis élastiques (`NodalSpring`)

- Six ressorts linéaires découplés par nœud, selon les axes globaux, raideur k ≥ 0 ajoutée au terme diagonal de K.
- Force sur la structure : −k·u, publiée à part des réactions rigides, et comprise dans le contrôle d'équilibre.
- Équivalents à des éléments `zeroLength` d'OpenSees reliés à un nœud fixe (cas 18 à 20).

## 6. Torsion et gauchissement

Le moteur modélise la **torsion de Saint-Venant uniforme** : un seul DDL de rotation axiale par nœud, rigidité GJ/L,
J de Saint-Venant de la section. Il ne modélise **pas** la torsion non uniforme : six DDL par nœud ne peuvent pas
représenter le gauchissement, le bimoment ni la constante Iw. Ce n'est pas une approximation à corriger par un
coefficient : il faudrait un autre élément, à 7 DDL par nœud (théorie de Vlasov), qui ne sera écrit qu'avec une
théorie vérifiée et une référence. Voir [../limitations/KNOWN_LIMITATIONS.md](../limitations/KNOWN_LIMITATIONS.md) §1.

## 7. Barres de treillis

Non implémentées. Une poutre relâchée en My et Mz aux deux bouts n'est pas un treillis : elle garde sa rigidité de
torsion (et un mécanisme de torsion si aucune autre barre ne la retient). Un élément axial à 6 DDL de translation
est prévu ([../ROADMAP.md](../ROADMAP.md)).
