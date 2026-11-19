#include "ui/MainWindow.h"

#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QThread>
#include <QVBoxLayout>

#include "ui/DuplicatesModel.h"
#include "worker/ScanWorker.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("DupFind - Duplicate File Finder"));
    resize(1000, 640);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    auto *controls = new QHBoxLayout;
    m_folderEdit = new QLineEdit(central);
    m_folderEdit->setReadOnly(true);
    m_folderEdit->setPlaceholderText(tr("No folder selected"));
    auto *browseButton = new QPushButton(tr("Browse..."), central);
    m_scanButton = new QPushButton(tr("Scan"), central);
    m_cancelButton = new QPushButton(tr("Cancel"), central);
    m_cancelButton->setEnabled(false);
    controls->addWidget(m_folderEdit, 1);
    controls->addWidget(browseButton);
    controls->addWidget(m_scanButton);
    controls->addWidget(m_cancelButton);

    m_model = new DuplicatesModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(Qt::UserRole);

    m_view = new QTableView(central);
    m_view->setModel(m_proxy);
    m_view->setSortingEnabled(true);
    m_view->sortByColumn(DuplicatesModel::GroupColumn, Qt::AscendingOrder);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setAlternatingRowColors(true);
    m_view->setWordWrap(false);
    m_view->verticalHeader()->hide();
    m_view->horizontalHeader()->setSectionResizeMode(DuplicatesModel::PathColumn,
                                                     QHeaderView::Stretch);
    m_view->horizontalHeader()->setSectionResizeMode(DuplicatesModel::SizeColumn,
                                                     QHeaderView::ResizeToContents);
    m_view->horizontalHeader()->setSectionResizeMode(DuplicatesModel::HashColumn,
                                                     QHeaderView::ResizeToContents);
    m_view->horizontalHeader()->setSectionResizeMode(DuplicatesModel::GroupColumn,
                                                     QHeaderView::ResizeToContents);

    m_status = new QLabel(tr("Choose a folder to scan."), central);
    m_progress = new QProgressBar(central);
    m_progress->setVisible(false);
    m_progress->setMaximumWidth(280);

    auto *statusRow = new QHBoxLayout;
    statusRow->addWidget(m_status, 1);
    statusRow->addWidget(m_progress);

    layout->addLayout(controls);
    layout->addWidget(m_view, 1);
    layout->addLayout(statusRow);
    setCentralWidget(central);

    connect(browseButton, &QPushButton::clicked, this, &MainWindow::chooseFolder);
    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::startScan);
    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::cancelScan);

    loadSettings();
}

MainWindow::~MainWindow() = default;

void MainWindow::chooseFolder()
{
    const QString start = m_folder.isEmpty() ? QDir::homePath() : m_folder;
    const QString chosen = QFileDialog::getExistingDirectory(this, tr("Choose a folder to scan"), start);
    if (chosen.isEmpty())
        return;

    m_folder = QDir::toNativeSeparators(chosen);
    m_folderEdit->setText(m_folder);
    m_status->setText(tr("Ready to scan."));
}

void MainWindow::startScan()
{
    if (m_thread || m_folder.isEmpty())
        return;

    m_model->clear();
    setScanning(true);
    m_status->setText(tr("Scanning..."));

    m_thread = new QThread(this);
    m_worker = new ScanWorker(m_folder);
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &ScanWorker::run);
    connect(m_worker, &ScanWorker::progress, this, &MainWindow::onProgress);
    connect(m_worker, &ScanWorker::finished, this, &MainWindow::onFinished);
    connect(m_worker, &ScanWorker::cancelled, this, &MainWindow::onCancelled);

    m_thread->start();
}

void MainWindow::cancelScan()
{
    if (!m_worker)
        return;
    m_worker->cancel();
    m_cancelButton->setEnabled(false);
    m_status->setText(tr("Cancelling..."));
}

void MainWindow::onProgress(qint64 done, qint64 total, const QString &phase)
{
    if (!m_thread)
        return;

    if (total > 0) {
        m_progress->setRange(0, 100);
        m_progress->setValue(int(done * 100 / total));
        m_status->setText(tr("%1: %2 of %3 files").arg(phase).arg(done).arg(total));
    } else {
        m_progress->setRange(0, 0);  // unknown total: busy indicator
        m_status->setText(tr("%1: %2 files").arg(phase).arg(done));
    }
}

void MainWindow::onFinished(const ScanResult &result)
{
    if (!m_thread)
        return;

    m_model->setResult(result);
    stopWorker();
    setScanning(false);

    if (result.groups.isEmpty()) {
        m_status->setText(tr("No duplicates found in %1 files.").arg(result.filesSeen));
        return;
    }

    QString text = tr("%1 groups, %2 extra copies, %3 reclaimable, %4 files scanned")
                       .arg(result.groups.size())
                       .arg(result.fileCount() - result.groups.size())
                       .arg(DuplicatesModel::formatBytes(result.wastedBytes()))
                       .arg(result.filesSeen);
    if (!result.unreadable.isEmpty())
        text += tr("  (%1 unreadable, listed in the log)").arg(result.unreadable.size());
    m_status->setText(text);
}

void MainWindow::onCancelled()
{
    if (!m_thread)
        return;

    stopWorker();
    setScanning(false);
    m_status->setText(tr("Scan cancelled; partial results discarded."));
}

void MainWindow::stopWorker()
{
    if (!m_thread)
        return;

    // The thread's event loop is idle by now (run() has returned), so quit/wait
    // cannot deadlock, and the join makes deleting the worker safe.
    m_thread->quit();
    m_thread->wait();
    delete m_worker;
    m_worker = nullptr;
    delete m_thread;
    m_thread = nullptr;
}

void MainWindow::setScanning(bool scanning)
{
    m_scanButton->setEnabled(!scanning);
    m_cancelButton->setEnabled(scanning);
    m_folderEdit->setEnabled(!scanning);
    m_progress->setVisible(scanning);
    if (scanning)
        m_progress->setRange(0, 0);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_worker)
        m_worker->cancel();
    stopWorker();
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    m_folder = settings.value(QStringLiteral("scan/lastFolder")).toString();
    if (!m_folder.isEmpty())
        m_folderEdit->setText(m_folder);

    const QByteArray geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("scan/lastFolder"), m_folder);
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
}
