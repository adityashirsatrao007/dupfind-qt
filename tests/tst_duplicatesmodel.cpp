#include <QtTest>

#include "ui/DuplicatesModel.h"

class TestDuplicatesModel : public QObject
{
    Q_OBJECT

private slots:
    void formatsBytes_data();
    void formatsBytes();
    void flattensGroupsWithOneBasedNumbers();
    void exposesRawValuesForSorting();
    void emptyResultHasNoRows();
};

void TestDuplicatesModel::formatsBytes_data()
{
    QTest::addColumn<qint64>("bytes");
    QTest::addColumn<QString>("text");

    QTest::newRow("zero") << qint64(0) << QStringLiteral("0 B");
    QTest::newRow("bytes") << qint64(512) << QStringLiteral("512 B");
    QTest::newRow("one kib") << qint64(1024) << QStringLiteral("1.0 KiB");
    QTest::newRow("one and a half kib") << qint64(1536) << QStringLiteral("1.5 KiB");
    QTest::newRow("one mib") << qint64(1024 * 1024) << QStringLiteral("1.0 MiB");
    QTest::newRow("three gib") << qint64(3) * 1024 * 1024 * 1024 << QStringLiteral("3.0 GiB");
}

void TestDuplicatesModel::formatsBytes()
{
    QFETCH(qint64, bytes);
    QFETCH(QString, text);
    QCOMPARE(DuplicatesModel::formatBytes(bytes), text);
}

void TestDuplicatesModel::flattensGroupsWithOneBasedNumbers()
{
    DuplicateGroup first;
    first.size = 100;
    first.hash = QByteArrayLiteral("aa");
    first.paths = {QStringLiteral("c:/a.bin"), QStringLiteral("c:/b.bin")};

    DuplicateGroup second;
    second.size = 40;
    second.hash = QByteArrayLiteral("bb");
    second.paths = {QStringLiteral("c:/x.bin"), QStringLiteral("c:/y.bin"),
                    QStringLiteral("c:/z.bin")};

    ScanResult result;
    result.groups = {first, second};

    DuplicatesModel model;
    model.setResult(result);

    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.columnCount(), int(DuplicatesModel::ColumnCount));
    QCOMPARE(model.groupCount(), 2);
    QCOMPARE(model.totalWasted(), qint64(100 + 40 * 2));

    QCOMPARE(model.data(model.index(0, DuplicatesModel::GroupColumn)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, DuplicatesModel::SizeColumn)).toString(),
             QStringLiteral("100 B"));
    QCOMPARE(model.data(model.index(2, DuplicatesModel::GroupColumn)).toInt(), 2);
    QCOMPARE(model.data(model.index(4, DuplicatesModel::PathColumn)).toString(),
             QStringLiteral("c:/z.bin"));
}

void TestDuplicatesModel::exposesRawValuesForSorting()
{
    DuplicateGroup group;
    group.size = 1024 * 1024;
    group.hash = QByteArrayLiteral("cc");
    group.paths = {QStringLiteral("c:/big.bin"), QStringLiteral("c:/big2.bin")};

    ScanResult result;
    result.groups = {group};

    DuplicatesModel model;
    model.setResult(result);

    // The sort proxy reads Qt::UserRole, so "1.0 MiB" sorts by byte count.
    QCOMPARE(model.data(model.index(0, DuplicatesModel::SizeColumn), Qt::UserRole).toLongLong(),
             qint64(1024 * 1024));
    QCOMPARE(model.data(model.index(0, DuplicatesModel::GroupColumn), Qt::UserRole).toInt(), 0);
}

void TestDuplicatesModel::emptyResultHasNoRows()
{
    DuplicatesModel model;
    model.setResult(ScanResult{});
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.groupCount(), 0);
    QCOMPARE(model.totalWasted(), qint64(0));
}

QTEST_GUILESS_MAIN(TestDuplicatesModel)
#include "tst_duplicatesmodel.moc"
