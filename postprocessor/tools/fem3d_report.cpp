// fem3d_report : génère tableaux CSV, diagrammes (CSV + SVG) et note de calcul (HTML, PDF optionnel)
// à partir d'un fichier de résultats « tsa-fem3d-results/1 ».
//
//   fem3d_report <resultats.json> --out <dossier> [--case <id>] [--pdf] [--opensees-summary <synthese.csv>]

#include "PdfExport.h"

#include "fem3dpost/Csv.h"
#include "fem3dpost/Diagrams.h"
#include "fem3dpost/Report.h"
#include "fem3dpost/ResultsReader.h"
#include "fem3dpost/Tables.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>

#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace fem3dpost;

namespace {

void out(const std::string& s)
{
    std::fputs(s.c_str(), stdout);
    std::fputs("\n", stdout);
}

void err(const std::string& s)
{
    std::fputs(s.c_str(), stderr);
    std::fputs("\n", stderr);
}

const char* kUsage =
    "Utilisation : fem3d_report <resultats.json> --out <dossier> [--case <id>] [--pdf]\n"
    "                           [--opensees-summary <synthese.csv>]\n"
    "  --out <dossier>              dossier de sortie (créé si besoin)\n"
    "  --case <id>                  limite les sorties au cas de charge <id>\n"
    "  --pdf                        produit aussi note_de_calcul.pdf\n"
    "  --opensees-summary <fichier> synthèse de comparaison OpenSees intégrée en section 14";

std::string u8(const QString& s)
{
    return s.toStdString();
}

// Résumé console : station de plus grande valeur absolue d'une colonne (utilise le tri de l'API).
void printExtreme(const Table& t, const char* label, int caseId)
{
    const auto col = t.column(label);
    if (!col)
        return;
    const Table top = head(sortByAbsDescending(filterByCase(t, caseId), *col), 1);
    if (top.rows.empty() || !cellNumber(top.rows[0][*col]))
        return;
    const auto& row = top.rows[0];
    out("    |" + std::string(label) + "| max = " + cellText(row[*col]) + " " + t.columns[*col].unit + " (élément " +
        cellText(row[*t.column(kColElement)]) + ", x = " + cellText(row[*t.column(kColPosition)]) + ")");
}

} // namespace

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    bool wantPdf = false;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--pdf") == 0)
            wantPdf = true;
    // L'export PDF a besoin de QtGui, sans affichage : plateforme « offscreen ».
    if (wantPdf) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
#ifdef _WIN32
        // La plateforme « offscreen » ne connaît pas les polices du système sous Windows :
        // sans elles, le texte du PDF serait rendu en pavés. On lui indique le dossier des polices.
        if (qEnvironmentVariableIsEmpty("QT_QPA_FONTDIR")) {
            const QByteArray windir = qgetenv("WINDIR");
            qputenv("QT_QPA_FONTDIR", (windir.isEmpty() ? QByteArray("C:\\Windows") : windir) + "\\Fonts");
        }
#endif
    }
    std::unique_ptr<QCoreApplication> app;
    if (wantPdf)
        app = std::make_unique<QGuiApplication>(argc, argv);
    else
        app = std::make_unique<QCoreApplication>(argc, argv);

    // --- Arguments ---------------------------------------------------------
    const QStringList args = QCoreApplication::arguments();
    QString input, outDir, openSeesPath;
    std::optional<int> caseId;
    for (qsizetype i = 1; i < args.size(); ++i) {
        const QString& a = args[i];
        auto next = [&](const char* opt) -> QString {
            if (i + 1 >= args.size()) {
                err(std::string("Erreur : valeur manquante après ") + opt + "\n" + kUsage);
                std::exit(2);
            }
            return args[++i];
        };
        if (a == "-h" || a == "--help") {
            out(kUsage);
            return 0;
        } else if (a == "--out") {
            outDir = next("--out");
        } else if (a == "--case") {
            bool ok = false;
            const int v = next("--case").toInt(&ok);
            if (!ok) {
                err("Erreur : --case attend un identifiant entier de cas");
                return 2;
            }
            caseId = v;
        } else if (a == "--pdf") {
            // déjà traité
        } else if (a == "--opensees-summary") {
            openSeesPath = next("--opensees-summary");
        } else if (a.startsWith("--")) {
            err("Erreur : option inconnue « " + u8(a) + " »\n" + kUsage);
            return 2;
        } else if (input.isEmpty()) {
            input = a;
        } else {
            err("Erreur : un seul fichier de résultats est attendu\n" + std::string(kUsage));
            return 2;
        }
    }
    if (input.isEmpty() || outDir.isEmpty()) {
        err(std::string("Erreur : fichier de résultats et --out obligatoires\n") + kUsage);
        return 2;
    }

    // --- Lecture validée ---------------------------------------------------
    Results results;
    try {
        results = readResultsFile(input);
    } catch (const FormatError& e) {
        err(std::string("Erreur de lecture de « ") + u8(input) + " » : " + e.what());
        return 1;
    }
    if (caseId && !results.loadCase(*caseId)) {
        err("Erreur : le cas " + std::to_string(*caseId) + " n'existe pas dans le fichier");
        return 2;
    }

    std::optional<TextTable> openSees;
    if (!openSeesPath.isEmpty()) {
        try {
            openSees = readDelimitedFile(openSeesPath);
        } catch (const std::exception& e) {
            err(std::string("Erreur : synthèse OpenSees : ") + e.what());
            return 1;
        }
    }

    const QDir dir(outDir);
    int written = 0;
    try {
        // --- Tableaux ------------------------------------------------------
        Table disp = displacementTable(results);
        Table reac = reactionTable(results);
        Table forces = sectionForceTable(results);
        Table stress = strainStressTable(results);
        if (caseId) {
            disp = filterByCase(disp, *caseId);
            reac = filterByCase(reac, *caseId);
            forces = filterByCase(forces, *caseId);
            stress = filterByCase(stress, *caseId);
        }
        writeTextFile(dir.filePath("deplacements.csv"), toCsv(disp));
        writeTextFile(dir.filePath("reactions.csv"), toCsv(reac));
        writeTextFile(dir.filePath("sollicitations.csv"), toCsv(forces));
        writeTextFile(dir.filePath("deformations_contraintes.csv"), toCsv(stress));
        written += 4;
        if (!results.springs.empty()) {
            Table springs = springForceTable(results);
            if (caseId)
                springs = filterByCase(springs, *caseId);
            writeTextFile(dir.filePath("ressorts.csv"), toCsv(springs));
            ++written;
        }

        // --- Diagrammes ----------------------------------------------------
        const QDir ddir(dir.filePath("diagrammes"));
        for (const LoadCase& lc : results.loadCases) {
            if (caseId && lc.id != *caseId)
                continue;
            for (const ElementResult& er : lc.elements) {
                writeTextFile(ddir.filePath(QString::fromStdString(diagramDataStem(lc.id, er.id) + ".csv")),
                              toCsv(diagramDataTable(results, lc, er)));
                ++written;
                for (const Diagram& d : elementDiagrams(results, lc, er)) {
                    writeTextFile(ddir.filePath(QString::fromStdString(diagramStem(d) + ".svg")), diagramSvg(d));
                    ++written;
                }
            }
        }

        // --- Note de calcul -------------------------------------------------
        ReportOptions opt;
        opt.inputFile = u8(QFileInfo(input).fileName());
        opt.generatedAt = u8(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
        opt.caseId = caseId;
        opt.openSees = openSees;
        opt.openSeesSource = u8(QFileInfo(openSeesPath).fileName());
        const Report html = buildReport(results, opt);
        writeTextFile(dir.filePath("note_de_calcul.html"), html.html);
        ++written;

        if (wantPdf) {
            opt.svgMode = SvgMode::ImageReference;
            const Report forPdf = buildReport(results, opt);
            QString error;
            QStringList warnings;
            const QString title = QString::fromStdString("Note de calcul — " + results.project);
            if (!exportPdf(forPdf, title, dir.filePath("note_de_calcul.pdf"), &error, &warnings)) {
                err("Erreur PDF : " + u8(error));
                return 1;
            }
            for (const QString& w : warnings)
                err("Avertissement PDF : " + u8(w));
            ++written;
        }

        // --- Résumé console --------------------------------------------------
        out("Fichier lu : " + u8(input) + " (" + results.format + ", " + std::to_string(results.nodes.size()) +
            " nœuds, " + std::to_string(results.elements.size()) + " éléments, " +
            std::to_string(results.loadCases.size()) + " cas)");
        const Table allForces = sectionForceTable(results);
        const Table allStress = strainStressTable(results);
        for (const LoadCase& lc : results.loadCases) {
            if (caseId && lc.id != *caseId)
                continue;
            out("  Cas " + std::to_string(lc.id) + " — " + lc.name);
            printExtreme(allForces, "N", lc.id);
            printExtreme(allForces, "My", lc.id);
            printExtreme(allForces, "Mz", lc.id);
            printExtreme(allStress, "von Mises indicatif", lc.id);
        }
        out(std::to_string(written) + " fichiers écrits dans " + u8(QDir::toNativeSeparators(dir.absolutePath())));
    } catch (const std::exception& e) {
        err(std::string("Erreur : ") + e.what());
        return 1;
    }
    return 0;
}
