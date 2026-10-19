# Limites connues

Ce que le moteur ne fait pas, ou fait avec une approximation. Une limite n'est levée qu'avec des tests et une
validation ([../validation/VALIDATION_PLAN.md](../validation/VALIDATION_PLAN.md)).

## 1. Domaine physique

- **Statique linéaire seulement** : petits déplacements, élasticité linéaire. Ni second ordre (P-Δ), ni flambement,
  ni plasticité, ni dynamique, ni analyse modale.
- **Sections constantes** le long d'une barre ; pas de barre à inertie variable.
- **Torsion de Saint-Venant uniforme** (rigidité GJ/L). Le **gauchissement n'est pas modélisé** : 6 DDL par nœud ne
  représentent ni le bimoment ni la torsion non uniforme de Vlasov. Pour les profils ouverts à parois minces (I, U,
  cornières) dont le gauchissement est empêché aux appuis, GJ seul **sous-estime** la rigidité de torsion et les
  contraintes normales de gauchissement manquent. Il faudrait un élément à 7 DDL par nœud (Iw, bimoment), qui ne sera
  écrit qu'avec une théorie vérifiée et une référence (Vlasov ; Trahair, [../references/REFERENCES.md](../references/REFERENCES.md)).
- **Timoshenko** : aire de cisaillement constante. Pour le cercle, coefficient de Cowper avec ν = 0,3 par défaut
  (`BeamSection3D::circle` accepte le ν du matériau) ; la page d'OpenSees conseille 0,9 A, soit un écart d'environ
  1,6 % sur la souplesse de cisaillement. Les benchmarks passent les mêmes aires aux deux codes.

## 2. Éléments et liaisons

- Pas de **barre de treillis** (effort normal seul) ni de **câble** : une poutre relâchée en My et Mz aux deux bouts
  garde sa rigidité de torsion, ce n'est pas un treillis. Dans TSA, ces barres sont refusées par Fem3D.
- **Relâchements** : rotules de flexion My et Mz seulement. Pas de relâchement de N, Vy, Vz ou T, ni de rotule
  semi-rigide (ressort de rotation en extrémité de barre).
- **Appuis élastiques** : ressorts découplés selon les axes globaux, linéaires. Pas de ressort couplé (matrice 6×6),
  ni en repère incliné, ni unilatéral. Un ressort sur un DDL bloqué est refusé par le moteur (l'intégration TSA
  l'ignore et le signale, car il n'a alors aucun effet).
- Pas d'**excentrement** (décalage rigide des extrémités, `-jntOffset` d'OpenSees).
- Pas de **contrainte cinématique** (diaphragme, `equalDOF`).

## 3. Charges

- Charges réparties **linéaires sur toute la longueur** seulement. Pas de charge ponctuelle sur barre, de charge
  partielle, ni de moment réparti : dans TSA, ces charges sont refusées par Fem3D (ou confiées à OpenSees si
  l'utilisateur l'a choisi).
- Pas de charge thermique, de précontrainte, ni de défaut initial.
- Les charges trapézoïdales ne sont comparées à OpenSees qu'indirectement (OpenSees 3.8.0 ignore la variation de
  `-beamUniform` en 3D, sans message).

## 4. Numérique

- **Diagnostic des mécanismes** : les DDL du mode de mécanisme ne sont désignés que pour n ≤ 2000 DDL libres
  (décomposition propre dense). Au-delà, le mécanisme est détecté mais non localisé.
- **Quasi-mécanismes** : un pivot relatif entre 10⁻¹² et 10⁻⁸ donne un **avertissement** et le calcul se poursuit ;
  la précision peut alors perdre jusqu'à −log10(pivot) chiffres significatifs.
- La factorisation `SimplicialLDLT` d'Eigen est faite pour les matrices définies positives ; la détection des
  mécanismes s'appuie sur ses pivots, couverte par des tests en kN-m et en N-mm.
- Le **contrôle d'équilibre global** vérifie l'équivalence statique des charges et la précision de la résolution ;
  il ne vérifie pas la rigidité (une matrice K fausse mais symétrique le satisferait).
- **Orientation par défaut** des barres : v_xz = Z global, ou −X pour une barre exactement verticale. Pour une barre
  presque verticale, le repère local peut changer brutalement de 90° selon le sens de l'inclinaison : fournir v_xz
  explicitement (c'est ce que fait TSA).

## 5. Résultats

- **Contraintes** : formules de résistance des matériaux. τ = V/As est une contrainte de calcul (la moyenne vraie
  vaut V/A) ; la contrainte normale extrême est exacte pour un rectangle et majorante sinon ; von Mises est indicatif
  et ne vaut pas vérification réglementaire. Aucune vérification normative (Eurocodes) n'est faite.
- Aucune combinaison ni enveloppe de cas n'est construite par le moteur (TSA les construit et les lui transmet).
- Une seule version de référence pour la validation croisée : OpenSees 3.8.0.
