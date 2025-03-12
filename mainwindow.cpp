#include "mainwindow.h"
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Spacer");
    resize(1000, 600);

    QToolBar *toolbar = addToolBar("Main Toolbar");
    QAction *chooseFolderAct = toolbar->addAction("Choose Folder");
    QAction *homeAct = toolbar->addAction("Home");
    QAction *zoomOutAct = toolbar->addAction("Zoom Out");

    connect(chooseFolderAct, &QAction::triggered, this, &MainWindow::chooseFolder);
    connect(homeAct, &QAction::triggered, this, &MainWindow::scanHome);
    connect(zoomOutAct, &QAction::triggered, this, &MainWindow::zoomOut);

    folderWidget = new FolderMapWidget(this);
    setCentralWidget(folderWidget);
}

void MainWindow::chooseFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select a folder", QDir::homePath());
    if (!dir.isEmpty())
        folderWidget->buildFolderTree(dir);
}

void MainWindow::scanHome()
{
    folderWidget->buildFolderTree(QDir::homePath());
}

void MainWindow::zoomOut()
{
    folderWidget->zoomOut();
}
