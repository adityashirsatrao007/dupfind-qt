#pragma once

#include <QMainWindow>
#include <QString>

#include "scan/ScanEngine.h"

class QCloseEvent;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSortFilterProxyModel;
class QTableView;
class QThread;
class DuplicatesModel;
class ScanWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void chooseFolder();
    void startScan();
    void cancelScan();
    void onProgress(qint64 done, qint64 total, const QString &phase);
    void onFinished(const ScanResult &result);
    void onCancelled();

private:
    void setScanning(bool scanning);
    void stopWorker();
    void loadSettings();
    void saveSettings();

    QLineEdit *m_folderEdit = nullptr;
    QPushButton *m_scanButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QTableView *m_view = nullptr;
    QLabel *m_status = nullptr;
    QProgressBar *m_progress = nullptr;
    DuplicatesModel *m_model = nullptr;
    QSortFilterProxyModel *m_proxy = nullptr;

    QThread *m_thread = nullptr;
    ScanWorker *m_worker = nullptr;
    QString m_folder;
};
