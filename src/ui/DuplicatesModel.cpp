#include "ui/DuplicatesModel.h"

DuplicatesModel::DuplicatesModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void DuplicatesModel::setResult(const ScanResult &result)
{
    beginResetModel();
    m_rows.clear();
    m_rows.reserve(result.fileCount());
    m_groupCount = result.groups.size();
    m_totalWasted = 0;

    for (int g = 0; g < result.groups.size(); ++g) {
        const DuplicateGroup &group = result.groups.at(g);
        for (const QString &path : group.paths) {
            Row row;
            row.path = path;
            row.size = group.size;
            row.hash = group.hash;
            row.group = g;
            m_rows.append(row);
        }
        m_totalWasted += group.wastedBytes();
    }
    endResetModel();
}

void DuplicatesModel::clear()
{
    setResult(ScanResult{});
}

int DuplicatesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

int DuplicatesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant DuplicatesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};

    const Row &row = m_rows.at(index.row());

    if (role == Qt::TextAlignmentRole && index.column() == SizeColumn)
        return int(Qt::AlignRight | Qt::AlignVCenter);

    // Raw values for the sort proxy: "9 KiB" must not sort above "10 KiB".
    if (role == Qt::UserRole) {
        switch (index.column()) {
        case PathColumn:
            return row.path;
        case SizeColumn:
            return row.size;
        case HashColumn:
            return QString::fromLatin1(row.hash);
        case GroupColumn:
            return row.group;
        default:
            return {};
        }
    }

    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case PathColumn:
        return row.path;
    case SizeColumn:
        return formatBytes(row.size);
    case HashColumn:
        return QString::fromLatin1(row.hash.left(16)) + QStringLiteral("...");
    case GroupColumn:
        return row.group + 1;
    default:
        return {};
    }
}

QVariant DuplicatesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    switch (section) {
    case PathColumn:
        return QStringLiteral("File");
    case SizeColumn:
        return QStringLiteral("Size");
    case HashColumn:
        return QStringLiteral("SHA-256");
    case GroupColumn:
        return QStringLiteral("Group");
    default:
        return {};
    }
}

QString DuplicatesModel::formatBytes(qint64 bytes)
{
    static const char *const units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    if (bytes < 1024)
        return QString::number(bytes) + QStringLiteral(" B");

    double value = double(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    return QString::number(value, 'f', 1) + QLatin1Char(' ') + QLatin1String(units[unit]);
}
