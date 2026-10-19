# Comparaison TSA Fem3D / OpenSees — synthèse générée

OpenSees 3.8.0 ; moteur TSA Fem3D 0.2.0. Tolérance : |écart| ≤ max(1e-06 · |réf|, 1e-09 · échelle).

| Cas | Formulation | Statut | Valeurs | Hors tolérance | Écart absolu max | Écart relatif max |
|---|---|---|---|---|---|---|
| Traction axiale (console, L = 4 m) | Euler-Bernoulli | OK | 30 | 0 | 1.69e-21 | 1.33e-16 |
| Traction axiale (console, L = 4 m) | Timoshenko | OK | 30 | 0 | 1.69e-21 | 1.33e-16 |
| Torsion de Saint-Venant (console, L = 4 m) | Euler-Bernoulli | OK | 30 | 0 | 1.78e-15 | 1.48e-16 |
| Torsion de Saint-Venant (console, L = 4 m) | Timoshenko | OK | 30 | 0 | 1.78e-15 | 1.48e-16 |
| Flexion autour de y local (console, charge Fz) | Euler-Bernoulli | OK | 30 | 0 | 4.97e-14 | 8.29e-16 |
| Flexion autour de y local (console, charge Fz) | Timoshenko | OK | 30 | 0 | 2.84e-14 | 7.11e-16 |
| Flexion autour de z local (console, charge Fy) | Euler-Bernoulli | OK | 30 | 0 | 3.55e-14 | 7.11e-16 |
| Flexion autour de z local (console, charge Fy) | Timoshenko | OK | 30 | 0 | 3.55e-14 | 5.92e-16 |
| Console sous charges transversales combinées et charge répartie locale | Euler-Bernoulli | OK | 30 | 0 | 4.09e-14 | 8.17e-15 |
| Console sous charges transversales combinées et charge répartie locale | Timoshenko | OK | 30 | 0 | 2.84e-14 | 5.68e-16 |
| Poutre courte L/h = 2 (cisaillement significatif) | Euler-Bernoulli | OK | 30 | 0 | 5.68e-14 | 1.14e-15 |
| Poutre courte L/h = 2 (cisaillement significatif) | Timoshenko | OK | 30 | 0 | 1.42e-14 | 4.74e-16 |
| Barre inclinée dans l'espace, orientation v_xz imposée | Euler-Bernoulli | OK | 30 | 0 | 2.02e-12 | 1.51e-13 |
| Barre inclinée dans l'espace, orientation v_xz imposée | Timoshenko | OK | 30 | 0 | 2.34e-12 | 2.27e-13 |
| Portique spatial multiéléments (hauteurs et portées différentes) | Euler-Bernoulli | OK | 168 | 0 | 4.18e-13 | 7.30e-14 |
| Portique spatial multiéléments (hauteurs et portées différentes) | Timoshenko | OK | 168 | 0 | 3.41e-13 | 9.55e-14 |
| Portique spatial, charges réparties locales et globales | Euler-Bernoulli | OK | 168 | 0 | 2.35e-13 | 6.06e-13 |
| Portique spatial, charges réparties locales et globales | Timoshenko | OK | 168 | 0 | 4.48e-13 | 2.62e-13 |
| Géométrie irrégulière : arbalétriers inclinés, contreventement, sections différentes | Euler-Bernoulli | OK | 186 | 0 | 2.98e-13 | 8.15e-13 |
| Géométrie irrégulière : arbalétriers inclinés, contreventement, sections différentes | Timoshenko | OK | 186 | 0 | 4.55e-13 | 1.04e-12 |
| Rotules My et Mz aux deux extrémités entre nœuds encastrés, charges réparties | Euler-Bernoulli | OK | 36 | 0 | 1.42e-14 | 5.34e-16 |
| Rotules My et Mz aux deux extrémités entre nœuds encastrés, charges réparties | Timoshenko | OK | 40 | 0 | 1.42e-14 | 5.34e-16 |
| Rotule Mz au nœud j seulement (barre selon Y), console sur appui | Euler-Bernoulli | OK | 36 | 0 | 7.11e-15 | 2.84e-16 |
| Rotule Mz au nœud j seulement (barre selon Y), console sur appui | Timoshenko | OK | 37 | 0 | 7.11e-15 | 2.85e-16 |
| Portique spatial rectangulaire, poutres articulées (My, Mz) aux deux extrémités | Euler-Bernoulli | OK | 168 | 0 | 1.62e-13 | 2.63e-14 |
| Portique spatial rectangulaire, poutres articulées (My, Mz) aux deux extrémités | Timoshenko | OK | 184 | 0 | 4.25e-13 | 5.75e-14 |
| Rotule intermédiaire (Gerber) entre deux consoles, charges nodale et répartie | Euler-Bernoulli | OK | 54 | 0 | 2.84e-14 | 9.98e-16 |
| Rotule intermédiaire (Gerber) entre deux consoles, charges nodale et répartie | Timoshenko | OK | 56 | 0 | 4.26e-14 | 9.95e-16 |
| Barre inclinée dans l'espace, rotule My au nœud i | Euler-Bernoulli | OK | 54 | 0 | 1.21e-13 | 3.93e-14 |
| Barre inclinée dans l'espace, rotule My au nœud i | Timoshenko | NON COMPARABLE | 0 | 0 | 0.00e+00 | 0.00e+00 |
| Console sur appui élastique en bout (ky, kz), charges nodale et répartie | Euler-Bernoulli | OK | 36 | 0 | 2.13e-14 | 7.67e-15 |
| Console sur appui élastique en bout (ky, kz), charges nodale et répartie | Timoshenko | OK | 36 | 0 | 2.84e-14 | 7.67e-15 |
| Poteau sur pied articulé retenu par trois ressorts de rotation, charges spatiales | Euler-Bernoulli | OK | 36 | 0 | 5.83e-13 | 1.47e-14 |
| Poteau sur pied articulé retenu par trois ressorts de rotation, charges spatiales | Timoshenko | OK | 36 | 0 | 5.90e-13 | 1.50e-14 |
| Portique spatial sur appuis élastiques (pieds semi-rigides, un pied entièrement sur ressorts) | Euler-Bernoulli | OK | 192 | 0 | 1.17e-12 | 9.64e-13 |
| Portique spatial sur appuis élastiques (pieds semi-rigides, un pied entièrement sur ressorts) | Timoshenko | OK | 192 | 0 | 6.82e-13 | 1.57e-12 |
| Mécanisme : console relâchée (My) au droit de son encastrement | Euler-Bernoulli | OK | 0 | 0 | 0.00e+00 | 0.00e+00 |
| Mécanisme : console relâchée (My) au droit de son encastrement | Timoshenko | OK | 0 | 0 | 0.00e+00 | 0.00e+00 |
| Mécanisme : rotation autour de l'axe libre aux deux appuis | Euler-Bernoulli | OK | 0 | 0 | 0.00e+00 | 0.00e+00 |
| Mécanisme : rotation autour de l'axe libre aux deux appuis | Timoshenko | OK | 0 | 0 | 0.00e+00 | 0.00e+00 |

Détails par valeur : `results/<cas>_<formulation>_comparaison.csv` ; scripts : `cases/*.tcl`.
