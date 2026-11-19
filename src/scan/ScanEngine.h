#pragma once

#include <QByteArray>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>

#include <atomic>
#include <functional>

// One group of byte-identical files.
struct DuplicateGroup {
    qint64 size = 0;
    QByteArray hash;      // lowercase hex SHA-256
    QStringList paths;

    qint64 wastedBytes() const { return size * (paths.size() - 1); }
};

struct ScanResult {
    QList<DuplicateGroup> groups;
    QStringList unreadable;
    qint64 filesSeen = 0;
    qint64 bytesHashed = 0;

    int fileCount() const
    {
        int n = 0;
        for (const auto &group : groups)
            n += int(group.paths.size());
        return n;
    }

    qint64 wastedBytes() const
    {
        qint64 n = 0;
        for (const auto &group : groups)
            n += group.wastedBytes();
        return n;
    }
};

// Two passes: bucket every file by exact size, then hash only the files whose
// size is not unique. Memory is proportional to the duplicate candidates, not
// to the whole volume, and the expensive hashing runs on a fraction of the tree.
class ScanEngine
{
public:
    // total <= 0 means "unknown yet" (the directory walk has no total up front).
    using Progress = std::function<void(qint64 done, qint64 total, const QString &phase)>;

    // Returns an empty result if `cancel` is set; the caller owns the "cancelled"
    // state, so a partial scan is never presented as a complete one.
    ScanResult scan(const QString &rootPath, const std::atomic<bool> &cancel,
                    const Progress &progress = {}) const;
};

Q_DECLARE_METATYPE(ScanResult)
