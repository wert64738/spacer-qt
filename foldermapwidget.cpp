#include "foldermapwidget.h"
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPaintEvent>
#include <QFileInfoList>
#include <QToolTip>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QDebug>
#include <algorithm>
#include <QRectF>
#include <cmath>

// ---------------------------------------------------------------------------
// Treemap subdivision algorithm (emulating DivideDisplayArea)
// Recursively assigns a rectangle to each RenderItem based on its proportional size.
// ---------------------------------------------------------------------------
static void divideDisplayArea(QList<RenderItem> &items, int start, int count, const QRectF &area, double totalSize, double gap)
{
    double safeWidth = std::max(0.0, area.width());
    double safeHeight = std::max(0.0, area.height());
    QRectF safeArea(area.x(), area.y(), safeWidth, safeHeight);

    if (count <= 0)
        return;
    if (count == 1) {
        items[start].rect = safeArea;
        return;
    }

    // Group items until adding the next would push the sum beyond half of totalSize.
    int groupACount = 0;
    double sizeA = 0;
    int i = start;
    sizeA += items[i].size;
    groupACount++;
    i++;
    while (i < start + count && (sizeA + items[i].size) * 2 < totalSize && items[i].size > 0) {
        sizeA += items[i].size;
        groupACount++;
        i++;
    }
    int groupBCount = count - groupACount;
    double sizeB = totalSize - sizeA;

    bool horizontalSplit = safeWidth >= safeHeight;
    if (horizontalSplit) {
        double effectiveWidth = std::max(0.0, safeWidth - gap);
        double mid = (totalSize > 0) ? (sizeA / totalSize) * effectiveWidth : effectiveWidth / 2.0;
        QRectF areaA(safeArea.x(), safeArea.y(), std::max(0.0, mid), safeHeight);
        QRectF areaB(safeArea.x() + mid + gap, safeArea.y(), std::max(0.0, safeWidth - mid - gap), safeHeight);
        divideDisplayArea(items, start, groupACount, areaA, sizeA, gap);
        divideDisplayArea(items, start + groupACount, groupBCount, areaB, sizeB, gap);
    } else {
        double effectiveHeight = std::max(0.0, safeHeight - gap);
        double mid = (totalSize > 0) ? (sizeA / totalSize) * effectiveHeight : effectiveHeight / 2.0;
        QRectF areaA(safeArea.x(), safeArea.y(), safeWidth, std::max(0.0, mid));
        QRectF areaB(safeArea.x(), safeArea.y() + mid + gap, safeWidth, std::max(0.0, safeHeight - mid - gap));
        divideDisplayArea(items, start, groupACount, areaA, sizeA, gap);
        divideDisplayArea(items, start + groupACount, groupBCount, areaB, sizeB, gap);
    }
}

// ---------------------------------------------------------------------------
// FolderMapWidget Implementation
// ---------------------------------------------------------------------------
FolderMapWidget::FolderMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    setMouseTracking(true); // Enable mouse tracking for hover events.
}

void FolderMapWidget::buildFolderTree(const QString &path)
{
    qDebug() << "Building folder tree for path:" << path;
    rootFolder = buildFolderTreeRecursive(path);
    qDebug() << "Finished building folder tree. Total size:" << rootFolder->totalSize;
    update();
}

std::shared_ptr<FolderNode> FolderMapWidget::buildFolderTreeRecursive(const QString &path)
{
    auto node = std::make_shared<FolderNode>();
    node->path = path;
    QDir dir(path);
    if (!dir.exists()) {
        qDebug() << "Directory does not exist:" << path;
        return node;
    }

    // Process files (only include those >= 100 bytes)
    QFileInfoList fileInfos = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : fileInfos) {
        if (fi.size() >= 100) {
            node->files.append(qMakePair(fi.absoluteFilePath(), fi.size()));
            node->totalSize += fi.size();
        }
    }

    // Process subfolders recursively
    QFileInfoList dirInfos = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &di : dirInfos) {
        auto child = buildFolderTreeRecursive(di.absoluteFilePath());
        if (child && child->totalSize > 0) {
            node->subFolders.append(child);
            node->totalSize += child->totalSize;
        }
    }
    qDebug() << "Scanned" << path << ": files =" << node->files.size()
             << ", subfolders =" << node->subFolders.size()
             << ", total size =" << node->totalSize;
    return node;
}

void FolderMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    m_renderItems.clear(); // Clear previous render items.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (rootFolder)
        renderFolderMap(painter, rootFolder, rect());
}

void FolderMapWidget::renderFolderMap(QPainter &painter, const std::shared_ptr<FolderNode> &node, const QRectF &rect, int depth)
{
    QList<RenderItem> items;
    // Add subfolders.
    for (const auto &sub : node->subFolders) {
        items.append({sub->path, sub->totalSize, true, sub, QRectF()});
    }
    // Add files.
    for (const auto &file : node->files) {
        items.append({file.first, file.second, false, nullptr, QRectF()});
    }
    if (items.isEmpty()) {
        qDebug() << "No items to render for node:" << node->path;
        return;
    }
    qDebug() << "Rendering node:" << node->path << "with" << items.size() << "items.";

    // Sort items by descending size.
    std::sort(items.begin(), items.end(), [](const RenderItem &a, const RenderItem &b) {
        return a.size > b.size;
    });

    qint64 total = 0;
    for (const auto &item : items)
        total += item.size;

    double gap = 1.0;
    // Subdivide the area among items.
    divideDisplayArea(items, 0, items.size(), rect, total, gap);

    // Save the rendered items for mouseover lookup.
    for (const auto &item : items)
        m_renderItems.append(item);

    // Save original font.
    QFont originalFont = painter.font();
    // Define explicit fonts:
    // For files, use a 7-point font.
    QFont fileFont = originalFont;
    fileFont.setPointSize(7);
    // For folders, use a 9-point font.
    QFont folderFont = originalFont;
    folderFont.setPointSize(9);

    // Draw each item.
    for (const auto &item : items) {
        QColor fillColor = item.isFolder ? ((depth % 2 == 0) ? Qt::lightGray : Qt::gray) : Qt::cyan;
        painter.fillRect(item.rect, fillColor);
        painter.drawRect(item.rect);

        // Select the appropriate font.
        if (item.isFolder)
            painter.setFont(folderFont);
        else
            painter.setFont(fileFont);

        QFontMetrics metrics(painter.font());
        QString filename = QFileInfo(item.path).fileName();
        QString elidedText = metrics.elidedText(filename, Qt::ElideRight, static_cast<int>(item.rect.width()));

        if (item.isFolder) {
            // For folders, leave a 2-pixel margin on left, right, and bottom, and 9 pixels at the top.
            QRectF textRect = item.rect.adjusted(2, 9, -2, -2);
            // Align text to the top center.
            painter.drawText(textRect, Qt::AlignTop | Qt::AlignHCenter, elidedText);
        } else {
            painter.drawText(item.rect, Qt::AlignCenter, elidedText);
        }

        // If this is a folder and there’s enough space, recursively render its contents.
        if (item.isFolder && item.rect.width() > 50 && item.rect.height() > 30) {
            renderFolderMap(painter, item.folder, item.rect.adjusted(2, 2, -2, -2), depth + 1);
        }
    }
    // Restore the original font.
    painter.setFont(originalFont);
}

void FolderMapWidget::zoomOut()
{
    if (!rootFolder)
        return;
    QDir dir(rootFolder->path);
    if (dir.cdUp()) {
        qDebug() << "Zooming out from" << rootFolder->path << "to" << dir.absolutePath();
        buildFolderTree(dir.absolutePath());
    } else {
        qDebug() << "Cannot zoom out further from" << rootFolder->path;
    }
}

void FolderMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    // Iterate over rendered items to see if the mouse is hovering over one.
    for (const auto &item : m_renderItems) {
        if (item.rect.contains(event->pos())) {
            QToolTip::showText(event->globalPos(), item.path, this);
            return;
        }
    }
    QToolTip::hideText();
    event->ignore();
}
