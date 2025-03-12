#include "foldermapwidget.h"
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QFileInfoList>
#include <QToolTip>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QDebug>
#include <algorithm>
#include <QRectF>
#include <cmath>

// Adjustable Parameters
static const int CORNER_ROUNDNESS = 4;              // Corner roundness in pixels.
static const double GAP_BETWEEN_ITEMS = 2.0;        // Gap between adjacent items (files/folders).
static const int EXTRA_SIDE_BUFFER = 2;             // Additional side buffer for folder child area.
static const int TOP_MARGIN_FOLDER_LABEL = 6;       // Top margin for folder label.
static const int SIDE_MARGIN_FOLDER_LABEL = 2;      // Left/right margin for folder label.
static const int BOTTOM_MARGIN_FOLDER_LABEL = 2;    // Bottom margin for folder label.
static const int FILE_FONT_SIZE = 5;                // Fixed font size for files.
static const int FOLDER_FONT_SIZE = 7;              // Fixed font size for folders.
static const double ROLLUP_THRESHOLD = 3.0;         // Threshold (in pixels) below which items are rolled up.

// Color settings (you can adjust these as needed)
static const QColor COLOR_ROLLUP = Qt::darkGray;

// ---------------------------------------------------------------------------
// Color helper functions
// ---------------------------------------------------------------------------
static QColor getFileTypeColor(const QString &filePath) {
    QString ext = QFileInfo(filePath).suffix().toLower();
    if(ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" ||
       ext == "bmp" || ext == "tiff" || ext == "ico")
        return QColor("PeachPuff");
    else if(ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "mov" ||
            ext == "wmv" || ext == "flv" || ext == "webm")
        return QColor("LemonChiffon");
    else if(ext == "mp3" || ext == "wav" || ext == "aac" || ext == "ogg" ||
            ext == "flac" || ext == "m4a")
        return QColor("MediumOrchid");
    else if(ext == "txt" || ext == "md" || ext == "log" || ext == "csv" ||
            ext == "rtf")
        return QColor("Thistle");
    else if(ext == "doc" || ext == "docx" || ext == "xls" || ext == "xlsx" ||
            ext == "ppt" || ext == "pptx")
        return QColor("PaleGreen");
    else if(ext == "pdf")
        return QColor("Khaki");
    else if(ext == "zip" || ext == "7z" || ext == "rar" || ext == "tar" ||
            ext == "gz" || ext == "bz2" || ext == "xz" || ext == "iso")
        return QColor("Gold");
    else if(ext == "cs" || ext == "cpp" || ext == "c" || ext == "java" ||
            ext == "py" || ext == "js" || ext == "html" || ext == "css" ||
            ext == "php" || ext == "rb" || ext == "go")
        return QColor("LightSlateGray");
    else if(ext == "dll" || ext == "bin" || ext == "dat" || ext == "sys")
        return QColor("PowderBlue");
    else if(ext == "exe" || ext == "cmd" || ext == "com" || ext == "bat" ||
            ext == "scr")
        return QColor("DarkRed");
    else if(ext == "db" || ext == "sql" || ext == "mdb" || ext == "accdb" ||
            ext == "sqlite")
        return QColor("DarkSeaGreen");
    else if(ext == "svg" || ext == "eps" || ext == "ai")
        return QColor("LightPink");
    else
        return QColor("LightBlue");
}

static QColor getFolderDepthColor(int depth) {
    double maxDepth = 10.0;
    double factor = std::min(depth / maxDepth, 1.0);
    int r = static_cast<int>(144 * (1 - factor) + 85 * factor);
    int g = static_cast<int>(238 * (1 - factor) + 107 * factor);
    int b = static_cast<int>(144 * (1 - factor) + 47 * factor);
    return QColor(r, g, b);
}

// ---------------------------------------------------------------------------
// Treemap subdivision algorithm (emulating DivideDisplayArea)
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
    setMouseTracking(true);
}

void FolderMapWidget::buildFolderTree(const QString &path)
{
    rootFolder = buildFolderTreeRecursive(path);
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
    QFileInfoList fileInfos = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : fileInfos) {
        if (fi.size() >= 100) {
            node->files.append(qMakePair(fi.absoluteFilePath(), fi.size()));
            node->totalSize += fi.size();
        }
    }
    QFileInfoList dirInfos = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &di : dirInfos) {
        auto child = buildFolderTreeRecursive(di.absoluteFilePath());
        if (child && child->totalSize > 0) {
            node->subFolders.append(child);
            node->totalSize += child->totalSize;
        }
    }
    return node;
}

void FolderMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    m_renderItems.clear();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (rootFolder)
        renderFolderMap(painter, rootFolder, rect());
}

void FolderMapWidget::renderFolderMap(QPainter &painter, const std::shared_ptr<FolderNode> &node, const QRectF &rect, int depth)
{
    QList<RenderItem> items;
    for (const auto &sub : node->subFolders)
        items.append({sub->path, sub->totalSize, true, sub, QRectF()});
    for (const auto &file : node->files)
        items.append({file.first, file.second, false, nullptr, QRectF()});
    if (items.isEmpty()) {
        qDebug() << "No items to render for node:" << node->path;
        return;
    }
    std::sort(items.begin(), items.end(), [](const RenderItem &a, const RenderItem &b) {
        return a.size > b.size;
    });
    qint64 total = 0;
    for (const auto &item : items)
        total += item.size;
    double gap = GAP_BETWEEN_ITEMS;
    divideDisplayArea(items, 0, items.size(), rect, total, gap);

    // --- Rollup small items ---
    QList<RenderItem> tinyItems;
    for (int i = items.size() - 1; i >= 0; i--) {
        const RenderItem &it = items.at(i);
        if (it.rect.width() < ROLLUP_THRESHOLD || it.rect.height() < ROLLUP_THRESHOLD) {
            tinyItems.append(it);
            items.removeAt(i);
        }
    }
    if (!tinyItems.isEmpty()) {
        int rollupCount = tinyItems.size();
        qint64 rollupSize = 0, rollupMaxSize = 0;
        for (const RenderItem &it : tinyItems) {
            rollupSize += it.size;
            if (it.size > rollupMaxSize)
                rollupMaxSize = it.size;
        }
        RenderItem rollupItem;
        rollupItem.path = QString("Rollup (%1)").arg(rollupCount);
        rollupItem.size = rollupSize;
        rollupItem.isFolder = false;
        rollupItem.isRollup = true;
        rollupItem.rollupCount = rollupCount;
        rollupItem.rollupMaxSize = rollupMaxSize;
        items.append(rollupItem);
        std::sort(items.begin(), items.end(), [](const RenderItem &a, const RenderItem &b) {
            return a.size > b.size;
        });
        total = 0;
        for (const RenderItem &it : items)
            total += it.size;
        divideDisplayArea(items, 0, items.size(), rect, total, gap);
    }
    for (const auto &item : items)
        m_renderItems.append(item);

    QFont originalFont = painter.font();
    QFont fileFont = originalFont; fileFont.setPointSize(FILE_FONT_SIZE);
    QFont folderFont = originalFont; folderFont.setPointSize(FOLDER_FONT_SIZE);

    // Draw each item with rounded corners and a 1-pixel gap.
    for (const auto &item : items) {
        QColor fillColor;
        if (item.isRollup)
            fillColor = COLOR_ROLLUP;
        else if (item.isFolder)
            fillColor = getFolderDepthColor(depth);
        else
            fillColor = getFileTypeColor(item.path);
        QRectF innerRect = item.rect.adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(innerRect, CORNER_ROUNDNESS, CORNER_ROUNDNESS);
        painter.fillPath(path, fillColor);
        painter.drawPath(path);

        if (item.isRollup) {
            painter.setFont(folderFont);
            QFontMetrics fmRollup(painter.font());
            QString rollupText = item.path;
            painter.drawText(innerRect, Qt::AlignCenter, rollupText);
        } else if (item.isFolder) {
            painter.setFont(folderFont);
            QFontMetrics fmFolder(painter.font());
            QString filename = QFileInfo(item.path).fileName();
            QString elidedText = fmFolder.elidedText(filename, Qt::ElideRight, static_cast<int>(item.rect.width() - 2 * SIDE_MARGIN_FOLDER_LABEL));
            QRectF labelRect(item.rect.left() + SIDE_MARGIN_FOLDER_LABEL,
                             item.rect.top() + TOP_MARGIN_FOLDER_LABEL,
                             item.rect.width() - 2 * SIDE_MARGIN_FOLDER_LABEL,
                             fmFolder.height());
            painter.drawText(labelRect, Qt::AlignCenter, elidedText);
            QRectF childRect(item.rect.left() + SIDE_MARGIN_FOLDER_LABEL + EXTRA_SIDE_BUFFER,
                             item.rect.top() + TOP_MARGIN_FOLDER_LABEL + fmFolder.height(),
                             item.rect.width() - 2 * SIDE_MARGIN_FOLDER_LABEL - 2 * EXTRA_SIDE_BUFFER,
                             item.rect.height() - TOP_MARGIN_FOLDER_LABEL - fmFolder.height() - BOTTOM_MARGIN_FOLDER_LABEL);
            if (childRect.width() > 50 && childRect.height() > 30) {
                renderFolderMap(painter, item.folder, childRect, depth + 1);
            }
        } else {
            painter.setFont(fileFont);
            QFontMetrics fmFile(painter.font());
            QString filename = QFileInfo(item.path).fileName();
            QString elidedText = fmFile.elidedText(filename, Qt::ElideRight, static_cast<int>(item.rect.width()));
            painter.drawText(innerRect, Qt::AlignCenter, elidedText);
        }
    }
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
    for (int i = m_renderItems.size() - 1; i >= 0; i--) {
        const RenderItem &item = m_renderItems.at(i);
        if (item.rect.contains(event->pos())) {
            QString tooltipText;
            if (item.isRollup)
                tooltipText = QString("Rolled up %1 items").arg(item.rollupCount);
            else
                tooltipText = QFileInfo(item.path).fileName();
            QToolTip::showText(event->globalPos(), tooltipText, this);
            return;
        }
    }
    QToolTip::hideText();
    event->ignore();
}

void FolderMapWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    for (int i = m_renderItems.size() - 1; i >= 0; i--) {
        const RenderItem &item = m_renderItems.at(i);
        if (item.rect.contains(event->pos())) {
            if (item.isFolder && !item.isRollup && item.folder) {
                rootFolder = item.folder;
                update();
                return;
            }
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}
