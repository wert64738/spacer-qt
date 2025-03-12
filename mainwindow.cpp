#include "mainwindow.h"
#include "foldermapwidget.h" // Ensure the full definition is included

#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QLineEdit>
#include <QDebug>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    folderWidget = new FolderMapWidget(this);
    setCentralWidget(folderWidget);

    QToolBar *toolbar = addToolBar("Main Toolbar");
    QAction *chooseFolderAction = toolbar->addAction("Choose Folder");
    QAction *homeAct = toolbar->addAction("Home");
    rootPathEdit = new QLineEdit(this);
    rootPathEdit->setReadOnly(true);
    connect(folderWidget, &FolderMapWidget::rootFolderChanged, this, &MainWindow::updateRootPath);

    toolbar->addWidget(rootPathEdit);
    QAction *zoomOutAction = toolbar->addAction("Zoom Out");

    connect(chooseFolderAction, &QAction::triggered, this, &MainWindow::chooseFolder);
    connect(homeAct, &QAction::triggered, this, &MainWindow::scanHome);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
}

void MainWindow::chooseFolder()
{
    QString folder = QFileDialog::getExistingDirectory(this, "Select Folder", QString());
    if (!folder.isEmpty()) {
        folderWidget->buildFolderTree(folder);
        updateRootPath(folder);
    }
}

void MainWindow::zoomOut()
{
    folderWidget->zoomOut();
}

void MainWindow::scanHome()
{
    folderWidget->buildFolderTree(QDir::homePath());
}

void MainWindow::updateRootPath(const QString &path)
{
    rootPathEdit->setText(path);
}
