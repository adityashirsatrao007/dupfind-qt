#pragma once

#include <QObject>
#include <QString>

#include <atomic>

#include "scan/ScanEngine.h"

// Lives in a worker QThread; talks to the window only through signals.
class ScanWorker : public QObject
{
    Q_OBJECT

public:
    explicit ScanWorker(QString rootPath, QObject *parent = nullptr);

    // Only flips an atomic, so it is safe to call from the GUI thread.
    void cancel() { m_cancel.store(true, std::memory_order_relaxed); }

public slots:
    void run();

signals:
    void progress(qint64 done, qint64 total, const QString &phase);
    void finished(const ScanResult &result);
    void cancelled();

private:
    QString m_rootPath;
    std::atomic<bool> m_cancel{false};
};
