#pragma once
// Écriture CSV (UTF-8 avec BOM, séparateur « ; », point décimal) et lecture de fichiers délimités.

#include "fem3dpost/Tables.h"

#include <QString>

#include <string>
#include <vector>

namespace fem3dpost {

inline constexpr const char* kUtf8Bom = "\xEF\xBB\xBF";

// Représentation la plus courte qui relit exactement le même double, point décimal.
std::string formatNumber(double v);
// Texte d'une cellule pour le CSV : vide si indisponible.
std::string cellText(const Cell& c);

// CSV complet : BOM UTF-8 (pour Excel), ligne d'en-têtes avec unités, « ; » et fins de ligne CRLF.
std::string toCsv(const Table& t);

// Écrit un fichier texte (octets UTF-8 tels quels). Lève std::runtime_error en cas d'échec.
void writeTextFile(const QString& path, const std::string& content);

// Tableau de texte brut (ex. synthèse OpenSees importée).
struct TextTable {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
};

// Découpe un texte délimité (guillemets « " » gérés). Le BOM éventuel est retiré.
std::vector<std::vector<std::string>> parseDelimited(const std::string& text, char separator);

// Lit un fichier CSV dont le séparateur (« ; » ou « , ») est détecté sur la première ligne.
// La première ligne non vide est l'en-tête. Lève std::runtime_error si illisible ou vide.
TextTable readDelimitedFile(const QString& path);

} // namespace fem3dpost
