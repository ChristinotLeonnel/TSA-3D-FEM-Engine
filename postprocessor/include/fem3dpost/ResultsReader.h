#pragma once
// Lecture validée d'un fichier « tsa-fem3d-results/1 ».

#include "fem3dpost/Results.h"

#include <QByteArray>
#include <QString>

#include <stdexcept>

namespace fem3dpost {

// Erreur de format : message explicite avec le chemin du champ fautif (ex. « loadCases[0].id : … »).
class FormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Analyse un contenu JSON. Lève FormatError si le format n'est pas exactement
// « tsa-fem3d-results/1 », si un champ obligatoire manque ou a un type incorrect,
// ou si les références croisées (nœuds, éléments, stations) sont incohérentes.
Results parseResults(const QByteArray& json);

// Lit puis analyse un fichier. Lève FormatError (y compris si le fichier est illisible).
Results readResultsFile(const QString& path);

} // namespace fem3dpost
