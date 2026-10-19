"""Exercice 1 du chapitre 22 : numérotation des DDL de Fem3D (DofMap).

Reproduit l'algorithme de DofMap::DofMap (src/Assembly/Assembly.cpp) et vérifie l'exemple du chapitre 12, §12.2.
Lancer : python ex01_dofmap.py
"""

DOF_NAMES = ("Ux", "Uy", "Uz", "Rx", "Ry", "Rz")


def global_dof(node_order, node_id, k):
    """DDL global g = 6 p + k, où p est l'indice interne du nœud (ordre de création)."""
    return 6 * node_order.index(node_id) + k


def partition(node_order, restraints):
    """Retourne (libres, imposés, rang) comme DofMap : listes de DDL globaux et rang de chaque DDL dans sa liste."""
    total = 6 * len(node_order)
    is_free = [True] * total
    for node_id, fixed in restraints.items():
        for k, blocked in enumerate(fixed):
            if blocked:
                is_free[global_dof(node_order, node_id, k)] = False

    free, restrained = [], []
    partition_index = [-1] * total
    # Comme DofMap : g croissant, donc les deux listes restent triées ; le rang est la longueur avant l'ajout.
    for g in range(total):
        target = free if is_free[g] else restrained
        partition_index[g] = len(target)
        target.append(g)

    return free, restrained, partition_index


def label(node_order, g):
    return f"nœud {node_order[g // 6]}, {DOF_NAMES[g % 6]}"


if __name__ == "__main__":
    # Exemple du chapitre 12 : nœuds créés dans l'ordre 20, 10, 30 ; 20 encastré ; 30 avec Uy et Uz bloqués.
    order = [20, 10, 30]
    restraints = {20: (True,) * 6, 30: (False, True, True, False, False, False)}
    free, restrained, rank = partition(order, restraints)

    assert global_dof(order, 10, 5) == 11, "nœud 10, Rz"
    assert global_dof(order, 30, 1) == 13, "nœud 30, Uy"
    assert free == [6, 7, 8, 9, 10, 11, 12, 15, 16, 17], free
    assert restrained == [0, 1, 2, 3, 4, 5, 13, 14], restrained
    assert rank[17] == 9 and rank[14] == 7, (rank[17], rank[14])

    print(f"n_f = {len(free)}, n_r = {len(restrained)}")
    for g in free:
        print(f"  libre   {g:2d} (rang {rank[g]}) : {label(order, g)}")
    for g in restrained:
        print(f"  imposé  {g:2d} (rang {rank[g]}) : {label(order, g)}")
    print("Exemple du chapitre 12 retrouvé.")
