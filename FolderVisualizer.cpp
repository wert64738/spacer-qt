#include "FolderVisualizer.h"
#include <QGraphicsTextItem>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QMessageBox>

FolderVisualizer::FolderVisualizer(const QString& path, QWidget *parent)
    : QMainWindow(parent), currentPath(path) {

    this->setWindowTitle("Linux Folder Visualizer");
    this->resize(800, 600);

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
    view->setDragMode(QGraphicsView::ScrollHandDrag);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(navBarWidget);
    mainLayout->addWidget(view);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    updateVisualization(path);
}

void FolderVisualizer::updateVisualization(const QString& path) {
    qDebug() << "Updating visualization for:" << path;

    // Step 1: Clear existing scene items properly
    QList<QGraphicsItem *> allItems = scene->items();
    for (QGraphicsItem *item : allItems) {
        delete item; // Free memory
    }
    scene->clear(); // Ensure everything is cleared

    // Step 2: Update the path
    currentPath = path;
    pathLabel->setText(path);

    // Step 3: Scan the new folder and visualize it
    QList<FileItem> fileData = scanFolder(path);
    divideSpace(fileData, QRectF(10, 10, 780, 580));
}


QList<FolderVisualizer::FileItem> FolderVisualizer::scanFolder(const QString& path) {
    QList<FileItem> fileData;
    QDir dir(path);

    // Get all files in the current folder
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &file, files) {
        qDebug() << "File detected:" << file.filePath() << "Size:" << file.size();
        fileData.append({file.filePath(), file.size(), false});
    }

    // Get all subfolders and scan them recursively
    QFileInfoList folders = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &folder, folders) {
        qint64 folderSize = 0;
        QList<FileItem> subFolderItems = scanFolder(folder.filePath()); // Recursive call

        foreach (const FileItem &subFile, subFolderItems) {
            folderSize += subFile.size;
            fileData.append(subFile);
        }

        qDebug() << "Folder detected:" << folder.filePath() << "Size:" << folderSize;
        fileData.append({folder.filePath(), folderSize, true});
    }

    if (fileData.isEmpty()) {
        qDebug() << "No files or folders found in" << path;
    }

    // Sort largest first
    std::sort(fileData.begin(), fileData.end(), [](const FileItem &a, const FileItem &b) {
        return a.size > b.size;
    });

    return fileData;
}

void FolderVisualizer::divideSpace(const QList<FileItem>& items, QRectF area) {
    if (items.isEmpty() || area.width() <= 0 || area.height() <= 0) {
        qDebug() << "divideSpace() skipped: empty items or zero area!";
        return;
    }

    qDebug() << "Dividing area:" << area << "for" << items.size() << "items";

    // Calculate total size of all items
    qint64 totalSize = 0;
    for (const auto &item : items) totalSize += item.size;

    bool horizontalSplit = area.width() >= area.height(); // Decide split direction

    qreal x = area.x();
    qreal y = area.y();
    qreal width = area.width();
    qreal height = area.height();

    for (int i = 0; i < items.size(); i++) {
        const FileItem &item = items[i];

        if (item.size == 0) continue; // Ignore zero-size items

        // Determine proportional size
        qreal proportion = static_cast<qreal>(item.size) / totalSize;
        QRectF itemRect;

        if (horizontalSplit) {
            qreal itemWidth = width * proportion;
            itemRect = QRectF(x, y, itemWidth, height);
            x += itemWidth;
        } else {
            qreal itemHeight = height * proportion;
            itemRect = QRectF(x, y, width, itemHeight);
            y += itemHeight;
        }

        // Ensure the rectangle has a minimum visible size
        if (itemRect.width() < 5 || itemRect.height() < 5)
            continue;

        qDebug() << "Rendering item:" << item.path << "Size:" << item.size << "Rect:" << itemRect;

        // Create visual representation
        CustomGraphicsItem *rect = new CustomGraphicsItem(item.path, item.isFolder, itemRect);
        scene->addItem(rect);
        connect(rect, &CustomGraphicsItem::itemDoubleClicked, this, &FolderVisualizer::handleItemDoubleClicked);

        // Recursively divide remaining space among smaller items
        if (i == items.size() - 1) break; // Stop if last item

        QList<FileItem> remainingItems = items.mid(i + 1);
        QRectF remainingArea;

        if (horizontalSplit) {
            remainingArea = QRectF(x, y, width - x, height);
        } else {
            remainingArea = QRectF(x, y, width, height - y);
        }

        if (!remainingItems.isEmpty() && remainingArea.width() > 0 && remainingArea.height() > 0) {
            divideSpace(remainingItems, remainingArea);
        }
        break;
    }
}

void FolderVisualizer::handleItemDoubleClicked(const QString& path, bool isFolder) {
    if (isFolder) {
        qDebug() << "Navigating into folder:" << path;
        updateVisualization(path);
    } else {
        qDebug() << "Ignoring double-click on file:" << path;
    }
}


void FolderVisualizer::navigateUp() {
    QDir dir(currentPath);
    if (dir.cdUp()) {
        updateVisualization(dir.absolutePath());
    }
}
