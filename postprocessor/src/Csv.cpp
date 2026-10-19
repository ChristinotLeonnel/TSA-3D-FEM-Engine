#include "fem3dpost/Csv.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <charconv>
#include <stdexcept>

namespace fem3dpost {

std::string formatNumber(double v)
{
    char buf[64];
    const auto res = std::to_chars(buf, buf + sizeof(buf), v);  // indépendant de la locale
    return std::string(buf, res.ptr);
}

std::string cellText(const Cell& c)
{
    if (const auto* i = std::get_if<long long>(&c))
        return std::to_string(*i);
    if (const auto* d = std::get_if<double>(&c))
        return formatNumber(*d);
    if (const auto* s = std::get_if<std::string>(&c))
        return *s;
    return {};
}

namespace {

std::string quoteIfNeeded(const std::string& s)
{
    if (s.find_first_of(";\"\r\n") == std::string::npos)
        return s;
    std::string out = "\"";
    for (char ch : s) {
        if (ch == '"')
            out += '"';
        out += ch;
    }
    out += '"';
    return out;
}

} // namespace

std::string toCsv(const Table& t)
{
    std::string out = kUtf8Bom;
    for (std::size_t i = 0; i < t.columns.size(); ++i) {
        if (i)
            out += ';';
        out += quoteIfNeeded(t.columns[i].header());
    }
    out += "\r\n";
    for (const auto& row : t.rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i)
                out += ';';
            out += quoteIfNeeded(cellText(row[i]));
        }
        out += "\r\n";
    }
    return out;
}

void writeTextFile(const QString& path, const std::string& content)
{
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath()))
        throw std::runtime_error("impossible de créer le dossier « " + info.absolutePath().toStdString() + " »");
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        throw std::runtime_error("impossible d'écrire « " + path.toStdString() + " » : " +
                                 f.errorString().toStdString());
    const QByteArray bytes = QByteArray::fromStdString(content);
    if (f.write(bytes) != bytes.size())
        throw std::runtime_error("écriture incomplète de « " + path.toStdString() + " »");
}

std::vector<std::vector<std::string>> parseDelimited(const std::string& textIn, char separator)
{
    std::string text = textIn;
    if (text.rfind(kUtf8Bom, 0) == 0)
        text.erase(0, 3);
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;
    bool quoted = false;
    bool any = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (quoted) {
            if (ch == '"') {
                if (i + 1 < text.size() && text[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    quoted = false;
                }
            } else {
                field += ch;
            }
            continue;
        }
        if (ch == '"') {
            quoted = true;
            any = true;
        } else if (ch == separator) {
            row.push_back(field);
            field.clear();
            any = true;
        } else if (ch == '\r') {
            // ignoré : la fin de ligne est traitée sur '\n'
        } else if (ch == '\n') {
            row.push_back(field);
            field.clear();
            if (any || row.size() > 1 || !row.front().empty())
                rows.push_back(row);
            row.clear();
            any = false;
        } else {
            field += ch;
            any = true;
        }
    }
    if (any || !field.empty() || !row.empty()) {
        row.push_back(field);
        rows.push_back(row);
    }
    return rows;
}

TextTable readDelimitedFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error("impossible d'ouvrir « " + path.toStdString() + " » : " +
                                 f.errorString().toStdString());
    const std::string text = f.readAll().toStdString();
    const std::size_t eol = text.find('\n');
    const std::string first = text.substr(0, eol);
    const char sep = first.find(';') != std::string::npos ? ';' : (first.find('\t') != std::string::npos ? '\t' : ',');
    auto rows = parseDelimited(text, sep);
    if (rows.empty())
        throw std::runtime_error("fichier « " + path.toStdString() + " » vide");
    TextTable t;
    t.header = rows.front();
    t.rows.assign(rows.begin() + 1, rows.end());
    return t;
}

} // namespace fem3dpost
