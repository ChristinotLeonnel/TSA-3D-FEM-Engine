# Chapitre 22 — Exercices et projets universitaires

> **Objectifs**
>
> - S'exercer sur chaque étape de la chaîne de calcul, avec des réponses vérifiables.
> - Proposer des projets de semestre ou de fin d'études qui font avancer réellement le moteur.
>
> **Prérequis**
>
> - Selon l'exercice : chapitres indiqués entre parenthèses. Python 3 pour les exercices numériques ; un compilateur
>   C++20 pour les exercices sur le code.

Les exercices suivent la progression du [parcours pédagogique](../../learning/index.md). Les réponses chiffrées sont
données à la fin ; elles ont été recalculées indépendamment (Python) et, quand c'est indiqué, comparées aux
résultats de Fem3D ou d'OpenSees. Unités : kN, m ; acier $E = 210 \cdot 10^6$ kN/m², $G = 81 \cdot 10^6$ kN/m² ;
section rectangulaire $b \times h = 0{,}30 \times 0{,}50$ m, d'où $I_y = 3{,}125 \cdot 10^{-3}$ m⁴,
$I_z = 1{,}125 \cdot 10^{-3}$ m⁴, $A_s = \tfrac56 A = 0{,}125$ m².

## Exercices

### Exercice 1 — Numérotation des DDL (chapitre 12)

Trois nœuds sont créés dans l'ordre 20, 10, 30. Le nœud 20 est encastré ; le nœud 30 a Uy et Uz bloqués.

1. Donner le DDL global $g$ de (nœud 10, Rz) et de (nœud 30, Uy).
2. Écrire la liste des DDL libres et celle des DDL imposés, dans l'ordre où `DofMap` les range.
3. Quel est le rang du DDL 17 dans la liste des libres ? Du DDL 14 dans la liste des imposés ?
4. Compléter le script [`ex01_dofmap.py`](../exercises/ex01_dofmap.py), qui reproduit `DofMap::DofMap` et vérifie
   vos réponses par des assertions.

### Exercice 2 — Forces nodales équivalentes (chapitre 10)

1. Calculer à la main $\int_0^L N_k(x)\,q\,dx$ pour une charge uniforme $q_y$ avec les fonctions d'Hermite, et
   retrouver $(q_yL/2,\ q_yL^2/12,\ q_yL/2,\ -q_yL^2/12)$.
2. Pourquoi les termes de moment changent-ils de signe pour une charge $q_z$ ?
3. Charge triangulaire de 0 à $w = 12$ kN/m sur $L = 5$ m (plan x-y) : calculer les quatre forces équivalentes.
4. Vérifier numériquement qu'une quadrature de Gauss à 4 points les donne exactement, et qu'une quadrature à 2 points
   ne suffit pas pour la charge triangulaire. Expliquer par le degré de l'intégrande.

### Exercice 3 — Charge globale sur une barre inclinée (chapitre 10)

Un arbalétrier de 4 m, incliné de 30° dans le plan X-Z, porte une charge de neige de 1,5 kN/m² en projection
horizontale sur une bande de 1 m. Donner la charge à saisir dans Fem3D (par mètre de barre, repère global), puis ses
composantes axiale et transversale.

### Exercice 4 — Console sur ressort (chapitre 11)

Console de $L = 4$ m, ressort vertical $k = 5000$ kN/m en bout, $F_Z = -20$ kN en bout, Euler–Bernoulli.

1. Calculer la flèche, la force du ressort et la réaction verticale de l'encastrement.
2. Refaire le calcul en Timoshenko.
3. Contrôler l'équilibre vertical. Où Fem3D publie-t-il la force du ressort ?

### Exercice 5 — Reconnaître un mécanisme (chapitre 13)

Pour chaque modèle d'une barre unique selon X, dire si Fem3D refuse le calcul, et quels DDL il désigne :

1. nœud 1 `pinned`, nœud 2 avec Uy, Uz bloqués ;
2. même modèle, mais Rx bloqué au nœud 1 ;
3. console encastrée en 1 avec une rotule My à l'extrémité i ;
4. nœud 1 `pinned` et un ressort de raideur nulle sur Ry.

### Exercice 6 — Cisaillement : quand Timoshenko compte (chapitre 7)

Console de $L = 1$ m ($L/h = 2$) sous $F_Y = 30$ kN et $F_Z = -50$ kN en bout.

1. Calculer $\Phi_y$ et $\Phi_z$, puis la flèche $U_Z$ en Euler–Bernoulli et en Timoshenko.
2. De combien de pour-cent Timoshenko dépasse-t-il Euler–Bernoulli en $U_Z$ ? En $U_Y$ ?
3. Comparer avec le cas OpenSees 06 (`benchmarks/OpenSees/results/06_poutre_courte_*_comparaison.csv`).

### Exercice 7 — Relire un fichier de résultats (chapitre 16)

Lancer `build\fem3d_example.exe sortie.json`. Avec Python et la description de [RESULTS_FORMAT.md](../../RESULTS_FORMAT.md),
relire les charges et les réactions d'un cas, et refaire le contrôle d'équilibre global en force. Comparer avec les
résidus affichés par le programme.

### Exercice 8 — Écrire un test analytique (chapitre 20)

Écrire un test `FEM3D_TEST` pour une poutre encastrée-appuyée sous charge uniforme, en Euler–Bernoulli : réaction de
l'appui simple $3qL/8$, moment d'encastrement $qL^2/8$. (En Timoshenko, la poutre étant hyperstatique, ces valeurs
dépendent de $\Phi$ : comparer avec `Analytical_ProppedCantileverTriangularLoad_Timoshenko`.) Fixer la tolérance avant d'exécuter, puis faire
échouer le test une fois volontairement (signe inversé) pour vérifier qu'il peut échouer.

### Exercice 9 — Lire un écart (chapitre 19)

Dans les fichiers `results/*_comparaison.csv`, trouver la plus grande valeur de la colonne `ecart_relatif`. Expliquer
pourquoi des écarts de $10^{-16}$ à $10^{-12}$ sont attendus et pourquoi un écart de $10^{-3}$ serait le signe d'une
erreur.

## Projets

Chaque projet suit la méthode du [chapitre 21](21-extensions.md) §21.1 et se termine par une demande d'intégration
relue (`CONTRIBUTING.md`).

| Projet | Niveau | Contenu | Livrables |
|---|---|---|---|
| A. Charges ponctuelles et partielles | master | forces équivalentes $N(a)\,P$, efforts et déformée par morceaux ([chapitre 21](21-extensions.md) §21.2) | code, tests analytiques, benchmark `-beamPoint`, chapitre du livre mis à jour |
| B. Barre de treillis | licence / master | élément à effort normal seul ; traitement des rotations sans rigidité ([chapitre 21](21-extensions.md) §21.3) | code, tests (méthode des nœuds), benchmark `truss` |
| C. Campagne de qualification (niveau 5) | master | mode comparaison de TSA sur les projets d'exemple et les cas de `TSA/docs/validation` ([chapitre 18](18-integration-tsa-opensees.md)) | tables archivées, rapport de qualification |
| D. Réimplémentation pédagogique | licence | solveur Python pour portiques 3D (EB), sur les chapitres 4 à 15, comparé à Fem3D | script, comparaison sur 3 cas |
| E. Analyse modale | master / doctorat | matrice de masse cohérente, problème aux valeurs propres | théorie vérifiée, tests (fréquences analytiques), benchmark `eigen` |
| F. Gauchissement (Vlasov) | doctorat | élément à 7 DDL par nœud, bimoment | théorie vérifiée sur les sources, référence de comparaison |

Les commandes OpenSees citées comme références (`truss`, `eigen`) sont à vérifier dans sa documentation au début du
projet ; seule `eleLoad -beamPoint` est déjà inventoriée ([WEBOGRAPHY.md](../../references/WEBOGRAPHY.md)).

Sujets de recherche détaillés : [guide pour les étudiants, les enseignants et les chercheurs](../../education/RESEARCH_AND_STUDENT_GUIDE.md).

## Réponses

**Exercice 1.** $g = 11$ et $g = 13$. Libres : 6, 7, 8, 9, 10, 11, 12, 15, 16, 17 ($n_f = 10$) ; imposés : 0 à 5, 13,
14. Rang du DDL 17 : 9 ; rang du DDL 14 : 7 ([chapitre 12](12-assemblage.md) §12.2).

**Exercice 2.** 2 : $dw/dx = -\theta_y$. 3 : 9 kN ; 10 kN·m ; 21 kN ; −15 kN·m. 4 : l'intégrande (cubique × linéaire)
est de degré 4 ; 2 points de Gauss intègrent exactement jusqu'au degré 3 seulement.

**Exercice 3.** $1{,}5 \cos 30° = 1{,}299$ kN/m, soit $q_Z = -1{,}299$ kN/m global par mètre de barre ; composantes :
axiale $-1{,}299 \sin 30° = -0{,}650$ kN/m, transversale $-1{,}299 \cos 30° = -1{,}125$ kN/m.

**Exercice 4.** EB : $\delta = -5{,}593 \cdot 10^{-4}$ m, ressort $+2{,}796$ kN, $R_Z = 17{,}204$ kN. Timoshenko : raideur
de console $30\,392$ kN/m, $\delta = -5{,}651 \cdot 10^{-4}$ m. $-20 + 2{,}796 + 17{,}204 = 0$. La force du ressort est
publiée à part des réactions (`springs`, `springForces`) ([chapitre 11](11-ressorts.md)).

**Exercice 5.** 1 : refus, rotation autour de l'axe (Rx) libre. 2 : calcul accepté. 3 : refus, nœud 2 désigné (Uz, Ry).
4 : refus, le ressort nul ne retient rien. Cas couverts par `Integration_MechanismDetected`,
`Releases_HingeAtSupportCreatesMechanism` et `Springs_ZeroStiffnessDoesNotHideMechanism`.

**Exercice 6.** $\Phi_z = 12 E I_y / (G A_s L^2) = 7{,}78 \cdot 10^{-1}$ ; $\Phi_y = 12 E I_z / (G A_s L^2) = 2{,}80 \cdot 10^{-1}$.
$U_Z$ : EB $-2{,}540 \cdot 10^{-5}$ m ; Timoshenko $-3{,}034 \cdot 10^{-5}$ m (+19,4 %). $U_Y$ : EB
$4{,}233 \cdot 10^{-5}$ m ; Timoshenko $4{,}529 \cdot 10^{-5}$ m (+7,0 %). Valeurs identiques à celles de Fem3D et
d'OpenSees dans les fichiers du cas 06.

**Exercice 9.** Les formulations comparées sont identiques : seul l'arrondi subsiste. Les écarts relatifs maximaux
vont de $10^{-16}$ à $1{,}6 \cdot 10^{-12}$ selon les cas (le plus grand : cas 20 en Timoshenko).
Une erreur de formulation produit 1 à 100 % d'écart ([chapitre 19](19-validation-reproductibilite.md) §19.3).

## À retenir

- Chaque exercice a une réponse vérifiable, recalculée hors de Fem3D.
- Les projets prolongent la feuille de route ; ils suivent la même exigence de validation que le moteur.

## Références

- Exemples numériques : chapitres [10](10-charges-conditions-limites.md), [11](11-ressorts.md),
  [12](12-assemblage.md), [19](19-validation-reproductibilite.md) ; cas OpenSees 06
  ([VALIDATION_OPENSEES.md](../../validation/VALIDATION_OPENSEES.md)).

---

Précédent : [Chapitre 21 — Extensions possibles du modèle](21-extensions.md) · Retour au
[sommaire du livre](../Fem3D_Technical_Book.md)
