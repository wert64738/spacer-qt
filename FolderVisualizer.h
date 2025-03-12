#ifndef FOLDERVISUALIZER_H
#define FOLDERVISUALIZER_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QDir>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include "CustomGraphicsItem.h"

class FolderVisualizer : public QMainWindow {
    Q_OBJECT

public:
    explicit FolderVisualizer(const QString& path, QWidget *parent = nullptr);

private slots:
    void handleItemDoubleClicked(const QString& path, bool isFolder);
    void navigateUp();

private:
    QGraphicsView *view;
    QGraphicsScene *scene;
    QHBoxLayout *navBarLayout;
    QLabel *pathLabel;
    QPushButton *backButton;
    QString currentPath;

    struct FileItem {
        QString path;
        qint64 size;
        bool isFolder;
    };

    QList<FileItem> scanFolder(const QString& path);
    void divideSpace(const QList<FileItem>& items, QRectF area);
    void updateVisualization(const QString& path);
    QColor getFileTypeColor(const QString& filePath);
};

#endif // FOLDERVISUALIZER_H
