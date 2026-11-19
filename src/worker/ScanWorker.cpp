#include "worker/ScanWorker.h"

#include <utility>

ScanWorker::ScanWorker(QString rootPath, QObject *parent)
    : QObject(parent)
    , m_rootPath(std::move(rootPath))
{
}

void ScanWorker::run()
{
    const ScanEngine engine;
    const ScanResult result = engine.scan(
        m_rootPath, m_cancel,
        [this](qint64 done, qint64 total, const QString &phase) {
            emit progress(done, total, phase);
        });

    if (m_cancel.load(std::memory_order_relaxed)) {
        emit cancelled();
        return;
    }
    emit finished(result);
}
