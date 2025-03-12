#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>   // Added
#include <QLabel>      // Added

class FolderMapWidget; // Forward declaration is sufficient here.

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void chooseFolder();
    void zoomOut();
    void scanHome();
    void updateRootPath(const QString &path);
    void setProcessing(bool processing);

private:
    FolderMapWidget *folderWidget;
    QLineEdit *rootPathEdit;
    QLabel *processingIndicator;
};

#endif // MAINWINDOW_H
