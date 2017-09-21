#ifndef GRAPHICSSCIENE_H
#define GRAPHICSSCIENE_H

#include <QGraphicsScene>

class GraphicsScene : public QGraphicsScene
{
public:
    GraphicsScene(QObject *parent = Q_NULLPTR);
    GraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject *parent = Q_NULLPTR);

    void drawBackground(QPainter *painter, const QRectF &rect);
};

#endif // GRAPHICSSCIENE_H
