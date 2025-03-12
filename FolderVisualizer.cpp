#include "FolderVisualizer.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QFont>

// Constructor: Initializes UI components
FolderVisualizer::FolderVisualizer(const QString& path, QWidget *parent)
    : QMainWindow(parent), currentPath(path) {

    this->setWindowTitle("Linux Folder Visualizer");
    this->resize(800, 600);

    // Layout for navigation bar
    QWidget *navBarWidget = new QWidget(this);
    navBarLayout = new QHBoxLayout(navBarWidget);
    
    backButton = new QPushButton("⬅ Up", this);
    pathLabel = new QLabel(this);
    pathLabel->setText(currentPath);

    connect(backButton, &QPushButton::clicked, this, &FolderVisualizer::navigateUp);
    navBarLayout->addWidget(backButton);
    navBarLayout->addWidget(pathLabel);
    
    view = new QGraphicsView(this);
    scene = new QGraphicsScene(this);
    view->setScene(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::ScrollHandDrag); // Enable panning

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(navBarWidget);
    mainLayout->addWidget(view);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    updateVisualization(path);
}

// Updates the visualization when navigating to a different folder
void FolderVisualizer::updateVisualization(const QString& path) {
    scene->clear();
    currentPath = path;
    pathLabel->setText(path);
    
    QList<FileItem> fileData = scanFolder(path);
    divideSpace(fileData, QRectF(10, 10, 780, 580));
}

// Scans a folder and retrieves file/folder sizes
QList<FolderVisualizer::FileItem> FolderVisualizer::scanFolder(const QString& path) {
    QList<FileItem> fileData;
    QDir dir(path);

    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &file, files) {
        fileData.append({file.filePath(), file.size()});
    }

    QFileInfoList folders = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &folder, folders) {
        qint64 folderSize = 0;
        QDir subDir(folder.filePath());
        foreach (const QFileInfo &subFile, subDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Size)) {
            folderSize += subFile.size();
        }
        fileData.append({folder.filePath(), folderSize});
    }

    // Sort by size (largest first)
    std::sort(fileData.begin(), fileData.end(), [](const FileItem &a, const FileItem &b) {
        return a.size > b.size;
    });

    return fileData;
}

// Recursive function to divide space proportionally
void FolderVisualizer::divideSpace(const QList<FileItem>& items, QRectF area) {
    if (items.isEmpty() || area.width() <= 0 || area.height() <= 0)
        return;

    if (items.size() == 1) {
        FileItem item = items.first();
        
        QColor fillColor = getFileTypeColor(item.path);
        QGraphicsRectItem *rect = scene->addRect(area, QPen(Qt::black), QBrush(fillColor));
        rect->setToolTip(QString("File: %1\nSize: %2 KB").arg(item.path.section("/", -1)).arg(item.size / 1024));
        rect->setData(0, item.path); // Store path in data

        QGraphicsTextItem *text = scene->addText(item.path.section("/", -1));
        text->setFont(QFont("Arial", 8));
        text->setPos(area.x() + 5, area.y() + 5);
        text->setZValue(1);

        rect->setFlag(QGraphicsItem::ItemIsSelectable);
        connect(scene, &QGraphicsScene::selectionChanged, this, [=]() {
            if (rect->isSelected()) {
                onItemClicked(rect);
            }
        });

        return;
    }

    qint64 totalSize = 0;
    for (const auto &item : items) totalSize += item.size;

    qint64 halfSize = totalSize / 2;
    qint64 accumulatedSize = 0;
    int splitIndex = 0;

    while (accumulatedSize < halfSize && splitIndex < items.size()) {
        accumulatedSize += items[splitIndex].size;
        splitIndex++;
    }

    bool horizontalSplit = area.width() >= area.height();
    if (horizontalSplit) {
        qreal midX = area.x() + (area.width() * accumulatedSize / totalSize);
        divideSpace(items.mid(0, splitIndex), QRectF(area.x(), area.y(), midX - area.x(), area.height()));
        divideSpace(items.mid(splitIndex), QRectF(midX, area.y(), area.right() - midX, area.height()));
    } else {
        qreal midY = area.y() + (area.height() * accumulatedSize / totalSize);
        divideSpace(items.mid(0, splitIndex), QRectF(area.x(), area.y(), area.width(), midY - area.y()));
        divideSpace(items.mid(splitIndex), QRectF(area.x(), midY, area.width(), area.bottom() - midY));
    }
}

// Handles clicking on a file/folder
void FolderVisualizer::onItemClicked(QGraphicsItem *item) {
    QString path = item->data(0).toString();
    if (QFileInfo(path).isDir()) {
        updateVisualization(path);
    }
}

// Navigate up one directory
void FolderVisualizer::navigateUp() {
    QDir dir(currentPath);
    if (dir.cdUp()) {
        updateVisualization(dir.absolutePath());
    }
}

// Enable zooming with the mouse wheel
void FolderVisualizer::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() > 0)
        view->scale(1.2, 1.2);
    else
        view->scale(0.8, 0.8);
}

// Assign colors based on file type
QColor FolderVisualizer::getFileTypeColor(const QString& filePath) {
    QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == "jpg" || ext == "png" || ext == "gif") return QColor(Qt::yellow);
    if (ext == "mp4" || ext == "avi" || ext == "mkv") return QColor(Qt::red);
    if (ext == "mp3" || ext == "wav") return QColor(Qt::green);
    if (ext == "pdf" || ext == "docx" || ext == "xlsx") return QColor(Qt::cyan);
    return QColor(Qt::gray);
}
