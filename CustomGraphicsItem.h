#ifndef CUSTOMGRAPHICSITEM_H
#define CUSTOMGRAPHICSITEM_H

#include <QGraphicsRectItem>
#include <QObject>

class CustomGraphicsItem : public QObject, public QGraphicsRectItem {
    Q_OBJECT  // <-- REQUIRED for signals/slots!

public:
    CustomGraphicsItem(const QString& path, bool isFolder, QRectF rect, QGraphicsItem *parent = nullptr);

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

signals:
    void itemDoubleClicked(const QString& path, bool isFolder);

private:
    QString itemPath;
    bool isFolder;
};

#endif // CUSTOMGRAPHICSITEM_H
