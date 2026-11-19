#include "scan/ScanEngine.h"

#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>

#include <algorithm>

namespace {

constexpr int kChunkSize = 1 << 20;  // 1 MiB: keeps the read buffer small on huge files

QByteArray hashFile(const QString &path, const std::atomic<bool> &cancel,
                    bool *ok, qint64 *bytesRead)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *ok = false;
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray buffer(kChunkSize, Qt::Uninitialized);
    qint64 total = 0;
    for (;;) {
        if (cancel.load(std::memory_order_relaxed)) {
            *ok = false;
            return {};
        }
        const qint64 read = file.read(buffer.data(), kChunkSize);
        if (read < 0) {
            *ok = false;
            return {};
        }
        if (read == 0)
            break;
        hash.addData(QByteArrayView(buffer.constData(), read));
        total += read;
    }

    *ok = true;
    *bytesRead = total;
    return hash.result().toHex();
}

}  // namespace

ScanResult ScanEngine::scan(const QString &rootPath, const std::atomic<bool> &cancel,
                            const Progress &progress) const
{
    ScanResult result;
    if (rootPath.isEmpty())
        return result;

    // Pass 1: group by exact size. Symlinks are skipped so a link cannot drag
    // the walk into a cycle or hash the same bytes twice.
    QHash<qint64, QStringList> bySize;
    QDirIterator it(rootPath, QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        if (cancel.load(std::memory_order_relaxed))
            return result;

        const QFileInfo info = it.fileInfo();
        if (!info.isFile() || info.isSymLink())
            continue;
        if (info.size() == 0)
            continue;  // empty files are all identical; reporting them is noise
        bySize[info.size()].append(path);
        ++result.filesSeen;

        if (progress && result.filesSeen % 512 == 0)
            progress(result.filesSeen, 0, QStringLiteral("Scanning"));
    }

    QList<qint64> candidateSizes;
    qint64 candidateFiles = 0;
    for (auto size = bySize.cbegin(); size != bySize.cend(); ++size) {
        if (size.value().size() > 1) {
            candidateSizes.append(size.key());
            candidateFiles += size.value().size();
        }
    }

    // Pass 2: hash only the candidates, and group equal hashes inside each size bucket.
    qint64 done = 0;
    for (const qint64 size : std::as_const(candidateSizes)) {
        QHash<QByteArray, QStringList> byHash;
        for (const QString &path : bySize.value(size)) {
            if (cancel.load(std::memory_order_relaxed))
                return result;

            bool ok = false;
            qint64 read = 0;
            const QByteArray hash = hashFile(path, cancel, &ok, &read);
            if (cancel.load(std::memory_order_relaxed))
                return result;
            if (!ok) {
                result.unreadable.append(path);
            } else {
                byHash[hash].append(path);
                result.bytesHashed += read;
            }

            ++done;
            if (progress)
                progress(done, candidateFiles, QStringLiteral("Hashing"));
        }

        for (auto entry = byHash.cbegin(); entry != byHash.cend(); ++entry) {
            if (entry.value().size() < 2)
                continue;
            DuplicateGroup group;
            group.size = size;
            group.hash = entry.key();
            group.paths = entry.value();
            group.paths.sort();
            result.groups.append(group);
        }
    }

    // Biggest win first: that is the order a person decides in.
    std::sort(result.groups.begin(), result.groups.end(),
              [](const DuplicateGroup &a, const DuplicateGroup &b) {
                  if (a.wastedBytes() != b.wastedBytes())
                      return a.wastedBytes() > b.wastedBytes();
                  return a.size > b.size;
              });

    return result;
}
