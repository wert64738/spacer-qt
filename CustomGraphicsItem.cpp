#include "CustomGraphicsItem.h"
#include <QDesktopServices>
#include <QUrl>

CustomGraphicsItem::CustomGraphicsItem(const QString& path, bool isFolder, QRectF rect, QGraphicsItem *parent)
    : QGraphicsRectItem(rect, parent), itemPath(path), isFolder(isFolder) {
    setFlag(QGraphicsItem::ItemIsSelectable);
    setToolTip(QString("File: %1").arg(itemPath.section("/", -1)));
}

void CustomGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    emit itemDoubleClicked(itemPath, isFolder);
}
