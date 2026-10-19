# Format des résultats « tsa-fem3d-results/1 »

Fichier JSON UTF-8 produit par `writeResultsJson` (`include/Fem3D/Results/ResultsJsonWriter.h`). C'est l'unique
interface entre le moteur et le post-traitement. Toute évolution incompatible change le numéro de version du format.
Les nombres sont écrits avec 17 chiffres significatifs. Une valeur indisponible vaut `null`, jamais une valeur inventée.

```text
{
  "format": "tsa-fem3d-results/1",
  "engine": "TSA Fem3D 0.2.0",
  "project": "…",
  "units": { "force": "kN", "length": "m", "moment": "kN.m", "rotation": "rad", "stress": "kN/m2" },
  "conventions": { "dofOrder": ["Ux","Uy","Uz","Rx","Ry","Rz"], "localAxes": "…", "sectionForces": "…",
                   "endForces": "…", "reactions": "…", "springForces": "…" },
  "nodes":    [ { "id": 1, "xyz": [x, y, z] }, … ],
  "elements": [ { "id": 1, "nodes": [i, j], "formulation": "Euler-Bernoulli" | "Timoshenko", "length": L,
                  "releases": [My_i, Mz_i, My_j, Mz_j],     // booléens : rotules (champ ajouté, absent = aucune)
                  "localX": [..], "localY": [..], "localZ": [..],
                  "material": { "name", "E", "G" },
                  "section":  { "name", "A", "Iy", "Iz", "J", "Asy", "Asz", "yExtreme", "zExtreme", "Wt" } }, … ],
  "supports": [ { "node": 1, "fixed": [6 booléens], "prescribed": [6 valeurs] }, … ],
  "springs":  [ { "node": 1, "stiffness": [kx, ky, kz, krx, kry, krz] }, … ],   // appuis élastiques, repère global
  "loadCases": [ {
      "id": 1, "name": "…",
      "displacements": [ { "node": 1, "values": [Ux, Uy, Uz, Rx, Ry, Rz] }, … ],     // repère global
      "reactions":     [ { "node": 1, "values": [Fx, Fy, Fz, Mx, My, Mz] }, … ],     // repère global, DDL libres : 0
      "springForces":  [ { "node": 1, "values": [Fx, Fy, Fz, Mx, My, Mz] }, … ],     // −k·u, exercées PAR le ressort
      "elements": [ {
          "id": 1,
          "endForcesLocal": [12],          // forces exercées SUR l'élément aux nœuds i puis j, repère local
          "localDisplacements": [12],      // DDL de l'élément, repère local (aux rotules : rotation de la barre)
          "maxDeflection": f, "maxDeflectionPosition": x,   // flèche locale max (écart à la corde), sa position
          "stations": [ {
              "x": x,                                       // abscisse locale 0 ≤ x ≤ L
              "forces": [N, Vy, Vz, T, My, Mz],             // face positive, repère local
              "localDisplacement": [u, v, w],
              "sectionRotation": [θx, θy, θz],
              "slopes": [dv/dx, dw/dx],                     // ≠ rotations de section en Timoshenko
              "globalDisplacement": [Ux, Uy, Uz],
              "stresses": { "axial", "bendingY", "bendingZ", "extremeNormal", "shearY", "shearZ",
                            "torsion", "vonMisesIndicative" }   // null si la donnée de section manque
          }, … ] }, … ],
      "equilibrium": { "appliedForce": [3], "appliedMoment": [3], "reactionForce": [3], "reactionMoment": [3],   // appuis + ressorts
                       "forceResidual": r, "momentResidual": r, "solutionResidual": r },
      "warnings": [ "…" ] }, … ]
}
```

Conventions détaillées : [theory/SIGN_CONVENTIONS.md](theory/SIGN_CONVENTIONS.md). Les stations sont équidistantes, extrémités
comprises (11 par défaut). Les efforts sont exacts en chaque station, même sous charge répartie : un diagramme tracé
entre les stations est une interpolation, à signaler comme telle.

Contraintes : `shearY` et `shearZ` sont des contraintes de cisaillement de calcul V / As (la moyenne vraie vaut V / A). `extremeNormal` vaut
|N/A| + |My| zExt/Iy + |Mz| yExt/Iz : exact pour un rectangle, majorant sinon. `vonMisesIndicative` est indicatif et
ne constitue pas une vérification réglementaire.

## Champs ajoutés sans changer de version

Ces champs s'ajoutent au format 1 sans rien changer aux champs existants : un lecteur qui les ignore reste correct,
sauf pour les ressorts, dont les forces manqueraient à son propre bilan d'équilibre.

- `elements[].releases` : rotules d'extrémité.
- `springs`, `loadCases[].springForces`, `conventions.springForces` : appuis élastiques. Les sommes
  `equilibrium.reactionForce` et `reactionMoment` incluent les forces des ressorts.
- `equilibrium.solutionResidual` : résidu relatif de la résolution ‖K_ff U_f − F‖ / ‖F‖.
- `loadCases[].warnings` : avertissements non bloquants du moteur (quasi-mécanisme, résidu de résolution élevé).
  Un lecteur doit les montrer à l'utilisateur, jamais les taire.
