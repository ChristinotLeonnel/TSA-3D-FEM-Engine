#pragma once
// Export PDF de la note (QtGui : QTextDocument + QPdfWriter). Exige une QGuiApplication
// (plateforme « offscreen ») et le greffon d'images qsvg pour rendre les diagrammes.

#include "fem3dpost/Report.h"

#include <QString>
#include <QStringList>

namespace fem3dpost {

// report doit avoir été construit en mode SvgMode::ImageReference.
// Retourne false et renseigne error en cas d'échec ; warnings reçoit les avertissements non bloquants.
bool exportPdf(const Report& report, const QString& title, const QString& path, QString* error, QStringList* warnings);

} // namespace fem3dpost
