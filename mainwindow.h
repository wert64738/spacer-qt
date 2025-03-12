#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>

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

private:
    FolderMapWidget *folderWidget;
    QLineEdit *rootPathEdit;
};

#endif // MAINWINDOW_H
