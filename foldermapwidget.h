#ifndef FOLDERMAPWIDGET_H
#define FOLDERMAPWIDGET_H

#include <QWidget>
#include <QString>
#include <QList>
#include <QPair>
#include <memory>

struct FolderNode {
    QString path;
    QList<QPair<QString, qint64>> files;  // file path and size
    QList<std::shared_ptr<FolderNode>> subFolders;
    qint64 totalSize = 0;
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

private:
    std::shared_ptr<FolderNode> rootFolder;
    std::shared_ptr<FolderNode> buildFolderTreeRecursive(const QString &path);
    void renderFolderMap(QPainter &painter, const std::shared_ptr<FolderNode>& node, const QRectF &rect, int depth = 0);
};

#endif // FOLDERMAPWIDGET_H
