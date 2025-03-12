#include "mainwindow.h"
#include "foldermapwidget.h" // Ensure the full definition is included

#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QLineEdit>
#include <QLabel>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QDebug>

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
    toolbar->addWidget(rootPathEdit);
    QAction *zoomOutAction = toolbar->addAction("Zoom Out");

    connect(chooseFolderAction, &QAction::triggered, this, &MainWindow::chooseFolder);
    connect(homeAct, &QAction::triggered, this, &MainWindow::scanHome);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);

    processingIndicator = new QLabel("Scanning directories...", this);
    processingIndicator->setVisible(false);
    statusBar()->addPermanentWidget(processingIndicator);
}

void MainWindow::chooseFolder()
{
    QString folder = QFileDialog::getExistingDirectory(this, "Select Folder", QString());
    if (!folder.isEmpty()) {
        setProcessing(true);
        folderWidget->buildFolderTree(folder);
        updateRootPath(folder);
        setProcessing(false);
    }
}

void MainWindow::zoomOut()
{
    setProcessing(true);
    folderWidget->zoomOut();
    setProcessing(false);
}

void MainWindow::scanHome()
{
    folderWidget->buildFolderTree(QDir::homePath());
}

void MainWindow::updateRootPath(const QString &path)
{
    rootPathEdit->setText(path);
}

void MainWindow::setProcessing(bool processing)
{
    processingIndicator->setVisible(processing);
}
