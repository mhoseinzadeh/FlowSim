#include "graphicsscene.h"
#include "block.h"
#include <QPainter>

GraphicsScene::GraphicsScene(QObject *parent):
    QGraphicsScene(parent)
{

}

GraphicsScene::GraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject *parent):
    QGraphicsScene(x, y, width, height, parent)
{

}

void GraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    for(int i=0; i<width()/CycleWidth; i++) {
        if(i%5==0) {
            painter->setPen(Qt::darkGray);
        } else {
            painter->setPen(Qt::lightGray);
        }
        painter->drawLine(i*CycleWidth, rect.top(), i*CycleWidth, rect.bottom());
    }
    painter->setPen(Qt::darkGray);
    for(int i=0; i<width()/CycleWidth/5; i++) {
        painter->drawText(i*CycleWidth*5, rect.bottom()-20, CycleWidth, 20, Qt::AlignCenter, QString::number(i*5));
    }
}
