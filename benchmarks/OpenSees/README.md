# Benchmarks OpenSees

Le programme `fem3d_opensees_benchmarks` (source : `opensees_benchmarks.cpp`) traite 20 cas avec les deux
formulations (dont 2 mécanismes). Pour chacun :

1. il construit le modèle et le calcule avec Fem3D ;
2. il écrit le script Tcl équivalent dans `cases/<cas>_<formulation>.tcl` : mêmes nœuds, appuis et propriétés, même
   `v_xz` de `geomTransf` que le repère TSA, mêmes charges ; rotules par `-releasey`/`-releasez` (Euler–Bernoulli)
   ou nœud doublé et `equalDOF` (Timoshenko) ; ressorts par `zeroLength` relié à un nœud fixe doublé ;
3. il exécute OpenSees, dont la sortie est écrite dans `results/<cas>_<formulation>_opensees.txt` (version, statut
   d'`analyze`, `nodeDisp`, `nodeReaction`, `localForce`, à 17 chiffres) et le journal dans `*_opensees.log` ;
4. il compare les déplacements, réactions, forces des ressorts, efforts d'extrémité et rotations aux rotules, valeur par valeur, dans
   `results/<cas>_<formulation>_comparaison.csv` ;
5. il écrit la synthèse dans `results/synthese.md` et `results/synthese.csv`.

Les statuts possibles sont :

- `OK` : toutes les valeurs sont dans la tolérance ;
- `ECART` : au moins une valeur est hors tolérance, ou le calcul TSA a échoué ;
- `NON COMPARABLE` : pas de référence OpenSees 3.8.0 exacte (charge répartie trapézoïdale, rotule de Timoshenko hors axes globaux) ;
- `NON EXÉCUTÉ` : OpenSees est absent ou n'a produit aucune sortie.

Le code de sortie du programme vaut le nombre de cas en `ECART`. Les tolérances et leur justification sont dans
[../../docs/validation/VALIDATION_OPENSEES.md](../../docs/validation/VALIDATION_OPENSEES.md) §3.

Les résultats de référence ne sont jamais modifiés à la main : ils sont régénérés par OpenSees à chaque exécution.
