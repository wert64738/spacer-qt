#ifndef FOLDERMAPWIDGET_H
#define FOLDERMAPWIDGET_H

#include <QWidget>
#include <QString>
#include <QList>
#include <QRectF>
#include <memory>

// Represents a folder node in the tree.
struct FolderNode {
    QString path;
    QList<QPair<QString, qint64>> files; // Pair of file path and size.
    QList<std::shared_ptr<FolderNode>> subFolders;
    qint64 totalSize = 0;
};

// RenderItem holds information needed for drawing an item (file or folder).
struct RenderItem {
    QString path;
    qint64 size;
    bool isFolder;
    std::shared_ptr<FolderNode> folder; // Valid if isFolder is true.
    QRectF rect;                      // Assigned by the treemap subdivision algorithm.
    // Rollup fields:
    bool isRollup = false;
    int rollupCount = 0;
    qint64 rollupMaxSize = 0;
};

class FolderMapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FolderMapWidget(QWidget *parent = nullptr);
    void buildFolderTree(const QString &path);
    void zoomOut();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    std::shared_ptr<FolderNode> buildFolderTreeRecursive(const QString &path);
    void renderFolderMap(QPainter &painter, const std::shared_ptr<FolderNode> &node, const QRectF &rect, int depth = 0);

    std::shared_ptr<FolderNode> rootFolder;
    QList<RenderItem> m_renderItems; // Stores rendered items for mouseover lookup.
};

#endif // FOLDERMAPWIDGET_H
