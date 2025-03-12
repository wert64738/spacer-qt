#ifndef FOLDERVISUALIZER_H
#define FOLDERVISUALIZER_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QDir>
#include <QMap>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>

class FolderVisualizer : public QMainWindow {
    Q_OBJECT

public:
    explicit FolderVisualizer(const QString& path, QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override; // Enables zooming

private slots:
    void onItemClicked(QGraphicsItem *item); // Handles file/folder clicks
    void navigateUp(); // Navigate up the directory

private:
    // UI Elements
    QGraphicsView *view;
    QGraphicsScene *scene;
    QHBoxLayout *navBarLayout;
    QLabel *pathLabel;
    QPushButton *backButton;
    
    QString currentPath; // Stores the current folder

    struct FileItem {
        QString path;
        qint64 size;
    };

    // Core Functions
    QList<FileItem> scanFolder(const QString& path);
    void divideSpace(const QList<FileItem>& items, QRectF area);
    void updateVisualization(const QString& path);
    QColor getFileTypeColor(const QString& filePath);
};

#endif // FOLDERVISUALIZER_H
