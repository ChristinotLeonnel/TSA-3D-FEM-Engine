# Chapitre 21 — Extensions possibles du modèle

> **Objectifs**
>
> - Savoir ce qu'il faudrait changer dans Fem3D pour lever chacune de ses limites, et dans quel ordre.
> - Pour chaque extension : la théorie à établir, les fichiers touchés, les tests et la référence de validation.
>
> **Prérequis**
>
> - Architecture ([chapitre 17](17-architecture-cpp.md)), validation ([chapitre 19](19-validation-reproductibilite.md)).

!!! warning "Rien de ce chapitre n'est implémenté"
    Ce chapitre décrit des travaux **à faire**. Aucune des fonctions ci-dessous n'existe à la révision `75700d2`. Les
    principes sont donnés pour orienter le travail ; chaque équation devra être établie, vérifiée sur les sources et
    validée avant d'être annoncée ([ROADMAP.md](../../ROADMAP.md), règle 2 de `AGENTS.md`).

## 21.1 Méthode commune à toute extension

Chaque extension suit le même chemin, dans cet ordre :

1. **Théorie écrite** dans [THEORY.md](../../theory/THEORY.md), avec ses hypothèses, ses limites et ses sources
   ([REFERENCES.md](../../references/REFERENCES.md)).
2. **Code** limité aux modules concernés, sans réécrire le reste du moteur.
3. **Tests analytiques** à $10^{-9}$ et **invariances** (orientation, maillage, unités).
4. **Référence indépendante** (OpenSees ou autre), tolérances fixées avant les essais.
5. **Documentation** : limites mises à jour, chapitres du livre, niveau de maturité de la fonction.
6. **Intégration dans TSA** : retrait du refus correspondant dans l'adaptateur, seulement après les étapes 1 à 5.

## 21.2 Charges ponctuelles, partielles et moments répartis (étape 2)

**Pourquoi d'abord.** Ce sont les charges les plus fréquentes dans les projets réels ; elles sont aujourd'hui
refusées par le moteur `fem3d` de TSA ([chapitre 18](18-integration-tsa-opensees.md)).

**Forces nodales équivalentes.** Le principe du [chapitre 10](10-charges-conditions-limites.md) §10.4 reste valable.
Pour une force ponctuelle $P$ à l'abscisse $a$, l'intégrale se réduit à une évaluation :

$$
f_{\text{eq},k} = \int_0^L N_k(x)\, P\, \delta(x - a)\, dx = N_k(a)\, P .
$$

Les fonctions de forme étant exactes, ce sont encore les forces d'encastrement parfait changées de signe. Exemple,
Euler–Bernoulli, $L = 6$ m, $a = 2$ m, $b = 4$ m, $P = 10$ kN dans le plan x-y :

| DDL | Formule classique | $N_k(a)\,P$ |
|---|---|---|
| $v_i$ | $P b^2 (3a + b)/L^3$ | 7,407 kN |
| $\theta_{z,i}$ | $P a b^2 / L^2$ | 8,889 kN·m |
| $v_j$ | $P a^2 (a + 3b)/L^3$ | 2,593 kN |
| $\theta_{z,j}$ | $-P a^2 b / L^2$ | −4,444 kN·m |

Valeurs recalculées en Python : l'évaluation des fonctions de forme d'Hermite redonne les formules classiques. En
Timoshenko, les mêmes $N_k(a)$ de la formulation donneront les valeurs propres à $\Phi$. Une charge partielle
s'intègre sur $[a, b]$ (Gauss par morceaux) ; un moment réparti $m(x)$ travaille avec les dérivées des fonctions de
forme.

**Efforts et déformée par morceaux.** C'est le vrai changement. `BeamResponse` représente chaque grandeur par **un
seul** polynôme sur $[0, L]$ ([chapitre 15](15-efforts-internes.md)). Une charge ponctuelle crée un **saut** de
l'effort tranchant et un **point anguleux** du moment ; une charge partielle, des polynômes différents de part et
d'autre de ses bornes. Il faut donc une classe de **polynômes par morceaux** (points de rupture, un polynôme par
intervalle, intégration qui raccorde les morceaux), et des stations qui incluent les points de rupture (valeurs à
gauche et à droite).

**Fichiers** : `Loads/Loads3D.h` (données), `StructuralModel3D::validate` (abscisses dans $[0, L]$), `BeamResponse`
(forces équivalentes, efforts, déformée), `checkEquilibrium` (résultante et moment), `ResultsJsonWriter`, post-traitement,
adaptateur TSA. Procédure détaillée : [DEVELOPER_GUIDE.md](../../developer/DEVELOPER_GUIDE.md) §5.

**Validation.** Analytique (poutre bi-encastrée, console, poutre sur appuis, charge en tout point) ; OpenSees
`eleLoad -beamPoint` pour la charge ponctuelle ; invariance « une charge ponctuelle au nœud intermédiaire d'une barre
coupée en deux = la même charge sur la barre entière ».

## 21.3 Barre de treillis et câble (étape 3)

**Modèle.** Une barre de treillis ne transmet qu'un effort normal : sa rigidité locale n'a que les termes $EA/L$ des
translations axiales. **Ce n'est pas** une poutre relâchée en My et Mz aux deux bouts, qui garde sa rigidité de
torsion ([chapitre 13](13-conditions-limites-stabilite.md) §13.3).

**Difficulté de conception.** Les nœuds ont toujours 6 DDL. Un nœud qui ne relie que des barres de treillis a des
rotations **sans aucune rigidité** : c'est un mécanisme au sens de Fem3D. Deux voies possibles, à trancher avant
d'écrire le code :

- bloquer ces rotations dans le **modèle transmis** (c'est le rôle des « blocages automatiques » de TSA, contrôlés
  après le calcul), en gardant la règle « Fem3D n'ajoute jamais d'appui de lui-même » ;
- ou faire exclure par `DofMap` les DDL qui ne reçoivent aucune rigidité, en le signalant.

**Validation.** Treillis isostatiques (méthode des nœuds) ; élément `truss` d'OpenSees, dont la documentation est
**à inventorier** dans [WEBOGRAPHY.md](../../references/WEBOGRAPHY.md). Le câble linéaire (rigidité en
traction seule) relève d'une analyse **non linéaire** et sort du domaine actuel.

## 21.4 Timoshenko dans TSA (étape 4)

Le moteur sait déjà calculer en Timoshenko (niveau 4). Il manque, côté TSA : la transmission des aires de
cisaillement $A_{sy}$, $A_{sz}$ des sections, une option de formulation dans l'onglet de Fem3D, et une référence de
comparaison (`ElasticTimoshenkoBeam` dans le script OpenSees de TSA). Aucune équation nouvelle.

## 21.5 Excentrements (étape 5)

**Modèle.** Un décalage rigide $\mathbf{e}$ entre le nœud et l'extrémité réelle de la barre relie leurs déplacements
par la cinématique du corps rigide (petites rotations) :

$$
\mathbf{u}_\text{extrémité} = \mathbf{u}_\text{nœud} + \boldsymbol{\theta}_\text{nœud} \times \mathbf{e},
\qquad
\boldsymbol{\theta}_\text{extrémité} = \boldsymbol{\theta}_\text{nœud} .
$$

Sous forme matricielle $d_\text{extrémité} = H\, d_\text{nœud}$, d'où $K' = H^T K H$ et $f' = H^T f$. Les efforts
publiés doivent rester ceux de l'extrémité réelle. **Référence** : option `-jntOffset` de `geomTransf` dans OpenSees
([REFERENCES.md](../../references/REFERENCES.md) §C7). Condition : que TSA transmette les excentrements aux moteurs.

## 21.6 Référence pour les charges trapézoïdales (étape 6)

OpenSees 3.8.0 ignore la variation de `-beamUniform` en 3D ([chapitre 19](19-validation-reproductibilite.md) §19.6).
Pistes : une version d'OpenSees qui l'applique, après vérification sur une sonde ; un second code de calcul ; ou une
comparaison exacte par superposition (charge uniforme + charge triangulaire, chacune validée séparément) si une
référence de la charge triangulaire seule existe.

## 21.7 Plus tard, sous conditions

| Extension | Principe | Changement de l'interface d'élément | Référence |
|---|---|---|---|
| Second ordre (P-Δ) et flambement linéaire | matrice géométrique $K_G(N)$ ; flambement : $(K + \lambda K_G)\,\phi = 0$ | ajouter $K_G$ | solutions d'Euler ; second ordre : transformation `PDelta` d'OpenSees ; aucune commande de flambement linéaire d'OpenSees n'est inventoriée (**à vérifier**) ; sources A6, A10 **à vérifier** |
| Analyse modale | matrice de masse cohérente $M$ ; $(K - \omega^2 M)\,\phi = 0$ | ajouter $M$ | poutre en flexion (fréquences analytiques) ; commande `eigen` d'OpenSees, documentation **à inventorier** ; source A6 **à vérifier** |
| Torsion non uniforme (Vlasov) | 7ᵉ DDL de gauchissement, bimoment, $I_\omega$ | élément à 14 DDL, numérotation à 7 DDL par nœud | Vlasov (A11), Trahair (A12) ; référence de comparaison à trouver |

Le gauchissement est le plus lourd : il change la numérotation (7 DDL par nœud), la transformation de repère, les
conditions aux limites (gauchissement libre ou empêché) et les résultats (bimoment, contraintes normales de
gauchissement). Il ne sera écrit qu'avec une théorie vérifiée sur les sources **et** une référence de comparaison
([KNOWN_LIMITATIONS.md](../../limitations/KNOWN_LIMITATIONS.md) §1).

**Hors champ** : non-linéarité matérielle, dynamique temporelle, coques et volumes. OpenSees les couvre dans TSA.

## À retenir

- Toute extension suit : théorie, code ciblé, analytique, référence indépendante, documentation, puis TSA.
- Charge ponctuelle : $f_\text{eq} = N(a)\,P$ est immédiat ; le vrai travail est la représentation **par morceaux**
  des efforts et de la déformée.
- Le treillis pose un problème de DDL de rotation sans rigidité, à trancher avant de coder.
- Flambement, modale et gauchissement demandent d'enrichir l'interface d'élément.

## Dans le code source

- Représentation actuelle (un polynôme par grandeur) : [`BeamResponse.h`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Results/BeamResponse.h#L41-L64).
- Interface à enrichir : [`IBeamElement3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Elements/IBeamElement3D.h#L35-L64).
- Données de charge à étendre : [`DistributedLoad3D`](https://github.com/ChristinotLeonnel/TSA-3D-FEM-Engine/blob/75700d2bfd33c29b2fadc795fd3051f25399ad19/include/Fem3D/Loads/Loads3D.h#L31-L37).

## Références

- A6 Przemieniecki, *Theory of Matrix Structural Analysis* — vérifié (notice) ; matrices géométrique et de masse :
  **à vérifier** dans le texte.
- A10 Timoshenko, Gere, *Theory of Elastic Stability* ; A11 Vlasov ; A12 Trahair — statuts dans
  [REFERENCES.md](../../references/REFERENCES.md) §C10.
- Documentation d'OpenSees : `eleLoad -beamPoint` ([WEBOGRAPHY.md](../../references/WEBOGRAPHY.md)),
  `geomTransf -jntOffset` ([REFERENCES.md](../../references/REFERENCES.md) §C7).

---

Précédent : [Chapitre 20 — Tests, limites et erreurs connues](20-tests-limites.md) · Suivant :
[Chapitre 22 — Exercices et projets universitaires](22-exercices-projets.md)
