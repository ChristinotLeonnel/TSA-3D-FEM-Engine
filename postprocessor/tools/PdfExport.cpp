#include "PdfExport.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QStringList>
#include <QTextDocument>
#include <QUrl>

namespace fem3dpost {

bool exportPdf(const Report& report, const QString& title, const QString& path, QString* error, QStringList* warnings)
{
    const bool svgSupported = QImageReader::supportedImageFormats().contains("svg");
    if (!svgSupported && warnings)
        warnings->append(QStringLiteral("greffon d'images « qsvg » introuvable (vérifier QT_PLUGIN_PATH) : "
                                        "les diagrammes seront absents du PDF"));

    QTextDocument doc;
    doc.setHtml(QString::fromUtf8(report.html.data(), static_cast<qsizetype>(report.html.size())));
    doc.setMetaInformation(QTextDocument::DocumentTitle, title);

    // Diagrammes : SVG rasterisés à 1,5 fois la taille d'affichage, sur fond blanc opaque
    // (sans canal alpha, l'image est bien plus compacte dans le PDF).
    int failed = 0;
    for (const ReportDiagram& d : report.diagrams) {
        QByteArray bytes = QByteArray::fromStdString(d.svg);
        // qsvg lit « font-family » comme un nom unique (pas de liste de repli) : on impose Arial,
        // sinon la plateforme « offscreen » retombe sur une police de symboles illisible.
        bytes.replace("font-family=\"Segoe UI, Arial, sans-serif\"", "font-family=\"Arial\"");
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer, "svg");
        reader.setScaledSize(QSize(kReportImageWidth * 3 / 2, kReportImageHeight * 3 / 2));
        const QImage img = reader.read();
        if (img.isNull()) {
            ++failed;
            continue;
        }
        QImage opaque(img.size(), QImage::Format_RGB32);
        opaque.fill(Qt::white);
        {
            QPainter p(&opaque);
            p.drawImage(0, 0, img);
        }
        doc.addResource(QTextDocument::ImageResource, QUrl(QString::fromStdString(d.imageRef)), opaque);
    }
    if (failed > 0 && warnings)
        warnings->append(QStringLiteral("%1 diagramme(s) n'ont pas pu être rendus dans le PDF").arg(failed));

    QPdfWriter writer(path);
    writer.setTitle(title);
    writer.setCreator(QString::fromLatin1(kPostProcessorVersion));
    writer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(15, 15, 15, 15),
                                     QPageLayout::Millimeter));
    doc.print(&writer);  // pagination et numéros de page assurés par QTextDocument

    QFile check(path);
    if (!check.exists() || check.size() == 0) {
        if (error)
            *error = QStringLiteral("le fichier PDF « %1 » n'a pas été produit").arg(path);
        return false;
    }
    return true;
}

} // namespace fem3dpost
