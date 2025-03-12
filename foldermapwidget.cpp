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
#include <QDesktopServices>
#include <QUrl>
#include <cmath>

// Adjustable Parameters
static const int CORNER_ROUNDNESS = 3;
static const double GAP_BETWEEN_ITEMS = 1.5;
static const int EXTRA_SIDE_BUFFER = 2;
static const int TOP_MARGIN_FOLDER_LABEL = 4;
static const int SIDE_MARGIN_FOLDER_LABEL = 2;
static const int BOTTOM_MARGIN_FOLDER_LABEL = 2;
static const int FILE_FONT_SIZE = 5;
static const int FOLDER_FONT_SIZE = 5;
static const double ROLLUP_THRESHOLD = 3.0;

// Color settings
//static const QColor COLOR_ROLLUP = Qt::darkGray;
static const QColor COLOR_ROLLUP = QColor(180, 180, 180);  // Softer gray

#include <QColor>
#include <QFileInfo>
#include <QString>
#include <vector>

#include <QColor>
#include <QFileInfo>
#include <QString>
#include <vector>

// Function to adjust color based on index
static QColor adjustColor(QColor baseColor, int index, int variation = 20) {
    int r = qBound(0, baseColor.red() + index * variation - 10, 255);
    int g = qBound(0, baseColor.green() + index * variation - 10, 255);
    int b = qBound(0, baseColor.blue() + index * variation - 10, 255);
    return QColor(r, g, b);
}

// Extended File Type Coloring with Less Pastel Tones
static QColor getFileTypeColor(const QString &filePath) {
    QString ext = QFileInfo(filePath).suffix().toLower();

    struct FileTypeGroup {
        std::vector<QString> extensions;
        QColor baseColor;
    };

    // Define file type groups with slightly richer colors
    std::vector<FileTypeGroup> fileGroups = {
        {{"jpg", "jpeg", "png", "gif", "bmp", "tiff", "ico"}, QColor(240, 180, 140)}, // Warmer Peach
        {{"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm"}, QColor(255, 225, 100)}, // Stronger Yellow
        {{"mp3", "wav", "aac", "ogg", "flac", "m4a"}, QColor(200, 90, 200)}, // Richer Lavender
        {{"txt", "md", "log", "csv", "rtf"}, QColor(180, 160, 180)}, // Deeper Lavender
        {{"doc", "docx", "xls", "xlsx", "ppt", "pptx"}, QColor(100, 230, 100)}, // Deeper Green
        {{"pdf"}, QColor(230, 200, 80)}, // Stronger Gold
        {{"zip", "7z", "rar", "tar", "gz", "bz2", "xz", "iso"}, QColor(230, 210, 150)}, // Warmer Beige
        {{"cs", "cpp", "c", "java", "py", "js", "html", "css", "php", "rb", "go"}, QColor(120, 170, 220)}, // Richer Blue
        {{"dll", "bin", "dat", "sys"}, QColor(120, 200, 220)}, // Stronger Cyan
        {{"exe", "cmd", "com", "bat", "scr"}, QColor(190, 60, 60)}, // Stronger Red
        {{"db", "sql", "mdb", "accdb", "sqlite"}, QColor(100, 150, 100)}, // Deeper Sage
        {{"svg", "eps", "ai"}, QColor(250, 120, 140)}, // Stronger Pink
        {{"sh"}, QColor(80, 200, 80)}, // Vibrant Green
        {{"conf", "ini", "cfg"}, QColor(190, 140, 80)}, // Deeper Tan
        {{"out", "run", "appimage"}, QColor(90, 110, 140)}, // Deeper Blue Gray
        {{"log"}, QColor(220, 110, 90)}, // Stronger Coral
    };

    // Search for the extension in the file groups
    for (const auto &group : fileGroups) {
        auto it = std::find(group.extensions.begin(), group.extensions.end(), ext);
        if (it != group.extensions.end()) {
            int index = std::distance(group.extensions.begin(), it);
            return adjustColor(group.baseColor, index);
        }
    }

    return QColor(100, 170, 220); // Default: Richer Sky Blue
}



static QColor getFolderDepthColor(int depth) {
    double maxDepth = 10.0;
    double factor = std::min(depth / maxDepth, 1.0);
    int r = static_cast<int>(200 * (1 - factor) + 150 * factor);
    int g = static_cast<int>(238 * (1 - factor) + 170 * factor);
    int b = static_cast<int>(200 * (1 - factor) + 120 * factor);
    return QColor(r, g, b);
}

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

FolderMapWidget::FolderMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    setMouseTracking(true);
}

void FolderMapWidget::buildFolderTree(const QString &path)
{
    rootFolder = buildFolderTreeRecursive(path);
    if (rootFolder)
        emit rootFolderChanged(rootFolder->path);
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

    if (!rootFolder)
        return;

    // Define the outermost folder rectangle with margins
    QRectF outerRect = rect().adjusted(1, 1, -1, -1);

    painter.setPen(QColor(150, 150, 150));
    QPainterPath outerPath;
    outerPath.addRoundedRect(outerRect, CORNER_ROUNDNESS, CORNER_ROUNDNESS);
    painter.fillPath(outerPath, painter.brush());
    painter.drawPath(outerPath);

    // Draw the root folder name label inside the outer rectangle
    QFont originalFont = painter.font();
    QFont folderLabelFont = originalFont;
    folderLabelFont.setBold(true);
    folderLabelFont.setPointSize(FOLDER_FONT_SIZE + 2);
    painter.setFont(folderLabelFont);

    QFontMetrics fm(painter.font());
    QString folderName = QFileInfo(rootFolder->path).fileName();
    if (folderName.isEmpty())
        folderName = rootFolder->path;
    QString elidedText = fm.elidedText(folderName, Qt::ElideRight, static_cast<int>(outerRect.width() - 20));

    QRectF labelRect(outerRect.left() + SIDE_MARGIN_FOLDER_LABEL,
                     outerRect.top() + TOP_MARGIN_FOLDER_LABEL,
                     outerRect.width() - 2 * SIDE_MARGIN_FOLDER_LABEL,
                     fm.height());
    painter.setPen(Qt::black);
    painter.drawText(labelRect, Qt::AlignCenter, elidedText);

    // Adjust the inner area (treemap) so it doesn't overlap the label
    QRectF treeRect = outerRect.adjusted(10, fm.height() + 10, -10, -10);

    // Render the inner folder map starting at depth 1 so inner folders get a different shade
    renderFolderMap(painter, rootFolder, treeRect, 1);
}

// Function to format file sizes in a human-readable format
static QString formatFileSize(qint64 size) {
    if (size < 1024) return QString::number(size) + " B";
    double kb = size / 1024.0;
    if (kb < 1024) return QString::number(kb, 'f', 1) + " KB";
    double mb = kb / 1024.0;
    if (mb < 1024) return QString::number(mb, 'f', 1) + " MB";
    double gb = mb / 1024.0;
    return QString::number(gb, 'f', 1) + " GB";
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
        for (const auto &it : tinyItems) {
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
        for (const auto &it : items)
            total += it.size;
        divideDisplayArea(items, 0, items.size(), rect, total, gap);
    }
    for (const auto &item : items)
        m_renderItems.append(item);

    QFont originalFont = painter.font();
    QFont fileFont = originalFont;
    fileFont.setPointSize(FILE_FONT_SIZE);
    fileFont.setBold(false);

    QFont folderFont = originalFont;
    folderFont.setPointSize(FOLDER_FONT_SIZE);
    folderFont.setBold(true);  // Folder names in bold

    // Constants for minimum label size.
    const qreal MIN_LABEL_WIDTH = 50.0;

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
        painter.setPen(QColor(100, 100, 100));
        painter.fillPath(path, fillColor);
        painter.drawPath(path);
        painter.setPen(Qt::black);
        if (item.isRollup) {
            painter.setFont(folderFont);
            QFontMetrics fmRollup(painter.font());
            QString rollupText = item.path;
            painter.drawText(innerRect, Qt::AlignCenter, rollupText);
        } else if (item.isFolder) {
            // For folders: display the name with its size (in brackets) on the same line.
            painter.setFont(folderFont);
            QFontMetrics fmFolder(painter.font());
            // Check if there is enough space to print the text.
            bool canDrawText = (item.rect.width() >= MIN_LABEL_WIDTH &&
                                item.rect.height() >= (TOP_MARGIN_FOLDER_LABEL + fmFolder.height()));
            if (canDrawText) {
                QString folderName = QFileInfo(item.path).fileName();
                QString sizeStr = QString(" (%1)").arg(formatFileSize(item.size));

                // Use a smaller, non-bold font for the size text.
                QFont folderSizeFont = folderFont;
                folderSizeFont.setBold(false);
                folderSizeFont.setPointSize(folderFont.pointSize() - 1);
                QFontMetrics fmFolderSize(folderSizeFont);

                int availableWidth = static_cast<int>(item.rect.width() - 2 * SIDE_MARGIN_FOLDER_LABEL);
                int sizeTextWidth = fmFolderSize.horizontalAdvance(sizeStr);
                QString elidedFolderName = fmFolder.elidedText(folderName, Qt::ElideRight, availableWidth - sizeTextWidth);
                int folderNameWidth = fmFolder.horizontalAdvance(elidedFolderName);
                int totalTextWidth = folderNameWidth + sizeTextWidth;
                double startX = item.rect.left() + SIDE_MARGIN_FOLDER_LABEL +
                                (item.rect.width() - 2 * SIDE_MARGIN_FOLDER_LABEL - totalTextWidth) / 2.0;
                int baseline = item.rect.top() + TOP_MARGIN_FOLDER_LABEL + fmFolder.ascent();

                // Draw folder name.
                painter.setFont(folderFont);
                painter.drawText(QPointF(startX, baseline), elidedFolderName);
                // Draw the size in the smaller font.
                painter.setFont(folderSizeFont);
                painter.drawText(QPointF(startX + folderNameWidth, baseline), sizeStr);
            }
            // Compute child area. If text wasn't drawn, the child area shifts up.
            int textHeight = canDrawText ? fmFolder.height() : 0;
            QRectF childRect(item.rect.left() + SIDE_MARGIN_FOLDER_LABEL + EXTRA_SIDE_BUFFER,
                             item.rect.top() + TOP_MARGIN_FOLDER_LABEL + textHeight,
                             item.rect.width() - 2 * SIDE_MARGIN_FOLDER_LABEL - 2 * EXTRA_SIDE_BUFFER,
                             item.rect.height() - TOP_MARGIN_FOLDER_LABEL - textHeight - BOTTOM_MARGIN_FOLDER_LABEL);
            if (childRect.width() > 50 && childRect.height() > 30) {
                renderFolderMap(painter, item.folder, childRect, depth + 1);
            }
        } else {
            // For files: display name on the first line and size underneath.
            painter.setFont(fileFont);
            QFontMetrics fmFile(painter.font());
            // Check if there is enough space to print the filename and size.
            bool canDrawText = (innerRect.width() >= MIN_LABEL_WIDTH && innerRect.height() >= (fmFile.height() * 2));
            if (canDrawText) {
                QString filename = QFileInfo(item.path).fileName();
                QString elidedName = fmFile.elidedText(filename, Qt::ElideRight, static_cast<int>(innerRect.width()));
                QString sizeStr = formatFileSize(item.size);
                QRectF nameRect(innerRect.left(), innerRect.top(), innerRect.width(), fmFile.height());
                QRectF sizeRect(innerRect.left(), innerRect.top() + fmFile.height(), innerRect.width(), fmFile.height());
                painter.drawText(nameRect, Qt::AlignCenter, elidedName);
                painter.drawText(sizeRect, Qt::AlignCenter, sizeStr);
            }
        }
    }
    painter.setFont(originalFont);
}



void FolderMapWidget::zoomOut()
{
    if (!rootFolder)
        return;
    QDir dir(rootFolder->path);

    // Prevent zooming out to root '/'
    if (dir.cdUp() && dir.absolutePath() != "/") {
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
                QDir d(rootFolder->path);
                if (d.cdUp())
                    emit rootFolderChanged(d.absolutePath());
                else
                    emit rootFolderChanged(rootFolder->path);
                update();
                return;
            } else if (!item.isFolder) { // Open media files in default viewer
                QString ext = QFileInfo(item.path).suffix().toLower();
                if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" ||
                    ext == "bmp" || ext == "tiff" || ext == "ico" ||
                    ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "mov" ||
                    ext == "wmv" || ext == "flv" || ext == "webm" ||
                    ext == "mp3" || ext == "wav" || ext == "aac" || ext == "ogg" ||
                    ext == "flac" || ext == "m4a") {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(item.path));
                    return;
                }
            }
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}
