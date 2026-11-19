#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "worker/ScanWorker.h"

namespace {

bool writeFile(const QString &path, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(data) == data.size();
}

}  // namespace

class TestScanWorker : public QObject
{
    Q_OBJECT

private slots:
    void emitsFinishedWithResult();
    void emitsCancelledInsteadOfPartialResult();
};

void TestScanWorker::emitsFinishedWithResult()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeFile(dir.filePath(QStringLiteral("one.bin")), "duplicate payload"));
    QVERIFY(writeFile(dir.filePath(QStringLiteral("two.bin")), "duplicate payload"));

    ScanWorker worker(dir.path());
    QSignalSpy finishedSpy(&worker, &ScanWorker::finished);
    QSignalSpy cancelledSpy(&worker, &ScanWorker::cancelled);
    QSignalSpy progressSpy(&worker, &ScanWorker::progress);

    worker.run();

    QCOMPARE(cancelledSpy.count(), 0);
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(progressSpy.count() > 0);

    const auto result = finishedSpy.at(0).at(0).value<ScanResult>();
    QCOMPARE(result.groups.size(), 1);
    QCOMPARE(result.groups.first().paths.size(), 2);
}

void TestScanWorker::emitsCancelledInsteadOfPartialResult()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeFile(dir.filePath(QStringLiteral("one.bin")), "duplicate payload"));
    QVERIFY(writeFile(dir.filePath(QStringLiteral("two.bin")), "duplicate payload"));

    ScanWorker worker(dir.path());
    QSignalSpy finishedSpy(&worker, &ScanWorker::finished);
    QSignalSpy cancelledSpy(&worker, &ScanWorker::cancelled);

    worker.cancel();
    worker.run();

    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(cancelledSpy.count(), 1);
}

QTEST_GUILESS_MAIN(TestScanWorker)
#include "tst_scanworker.moc"
