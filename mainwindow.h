#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "foldermapwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void chooseFolder();
    void scanHome();
    void zoomOut();

private:
    FolderMapWidget *folderWidget;
};

#endif // MAINWINDOW_H
