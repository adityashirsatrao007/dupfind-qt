#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "scan/ScanEngine.h"

#if defined(Q_OS_UNIX)
#include <unistd.h>
#endif

namespace {

bool writeFile(const QString &path, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(data) == data.size();
}

QString pathIn(const QTemporaryDir &dir, const QString &relative)
{
    return dir.filePath(relative);
}

}  // namespace

class TestScanEngine : public QObject
{
    Q_OBJECT

private slots:
    void findsIdenticalFiles();
    void ignoresFilesWithEqualSizeButDifferentContent();
    void ignoresEmptyFiles();
    void missingRootYieldsNothing();
    void honoursCancellationBeforeStarting();
    void reportsUnreadableFile();
};

void TestScanEngine::findsIdenticalFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("a/one.bin")), "hello world"));
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("b/nested/two.bin")), "hello world"));
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("c/unique.bin")), "not a duplicate"));

    const std::atomic<bool> cancel{false};
    const ScanResult result = ScanEngine{}.scan(dir.path(), cancel);

    QCOMPARE(result.groups.size(), 1);
    const DuplicateGroup &group = result.groups.first();
    QCOMPARE(group.size, qint64(11));
    QCOMPARE(group.paths.size(), 2);
    QCOMPARE(group.wastedBytes(), qint64(11));
    QCOMPARE(result.wastedBytes(), qint64(11));
    QCOMPARE(result.filesSeen, qint64(3));
    QVERIFY(result.unreadable.isEmpty());
}

void TestScanEngine::ignoresFilesWithEqualSizeButDifferentContent()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("x.bin")), "abcdef"));
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("y.bin")), "abcdeg"));

    const std::atomic<bool> cancel{false};
    const ScanResult result = ScanEngine{}.scan(dir.path(), cancel);

    QVERIFY2(result.groups.isEmpty(), "same size alone must not make a duplicate");
    QCOMPARE(result.filesSeen, qint64(2));
    QVERIFY(result.bytesHashed > 0);
}

void TestScanEngine::ignoresEmptyFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("a.bin")), ""));
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("b.bin")), ""));

    const std::atomic<bool> cancel{false};
    const ScanResult result = ScanEngine{}.scan(dir.path(), cancel);

    QVERIFY(result.groups.isEmpty());
    QCOMPARE(result.filesSeen, qint64(0));
}

void TestScanEngine::missingRootYieldsNothing()
{
    const std::atomic<bool> cancel{false};
    const ScanResult result = ScanEngine{}.scan(QStringLiteral("/no/such/folder/here"), cancel);

    QVERIFY(result.groups.isEmpty());
    QCOMPARE(result.filesSeen, qint64(0));
}

void TestScanEngine::honoursCancellationBeforeStarting()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("a.bin")), "data"));
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("b.bin")), "data"));

    std::atomic<bool> cancel{true};
    const ScanResult result = ScanEngine{}.scan(dir.path(), cancel);

    QVERIFY(result.groups.isEmpty());
    QCOMPARE(result.filesSeen, qint64(0));
}

void TestScanEngine::reportsUnreadableFile()
{
#if defined(Q_OS_UNIX)
    if (::geteuid() == 0)
        QSKIP("root can read anything, so permissions cannot simulate a failure");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString locked = pathIn(dir, QStringLiteral("locked.bin"));
    QVERIFY(writeFile(locked, "secret"));
    QVERIFY(writeFile(pathIn(dir, QStringLiteral("copy.bin")), "secret"));
    QVERIFY(QFile::setPermissions(locked, QFileDevice::Permissions()));

    const std::atomic<bool> cancel{false};
    const ScanResult result = ScanEngine{}.scan(dir.path(), cancel);

    QFile::setPermissions(locked, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    QCOMPARE(result.unreadable.size(), 1);
    QVERIFY(result.groups.isEmpty());
#else
    QSKIP("permission-based failure cannot be simulated reliably on Windows");
#endif
}

QTEST_GUILESS_MAIN(TestScanEngine)
#include "tst_scanengine.moc"
