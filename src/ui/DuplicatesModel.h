#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "scan/ScanEngine.h"

// Flat table: one row per duplicate file, tagged with a 1-based group number.
// Rows come from the engine already ordered by reclaimable size.
class DuplicatesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { PathColumn = 0, SizeColumn, HashColumn, GroupColumn, ColumnCount };

    explicit DuplicatesModel(QObject *parent = nullptr);

    void setResult(const ScanResult &result);
    void clear();

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int groupCount() const { return m_groupCount; }
    qint64 totalWasted() const { return m_totalWasted; }

    static QString formatBytes(qint64 bytes);

private:
    struct Row {
        QString path;
        qint64 size = 0;
        QByteArray hash;
        int group = 0;
    };

    QVector<Row> m_rows;
    int m_groupCount = 0;
    qint64 m_totalWasted = 0;
};
