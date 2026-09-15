#include "GridScene.h"
#include <QPainter>
#include <QGraphicsView>
#include <cmath>

GridScene::GridScene(QObject* parent)
    : QGraphicsScene(parent) {
}

void GridScene::setGridColor(const QColor& color) {
    m_gridColor = color;
}

void GridScene::setLabelColor(const QColor& color) {
    m_labelColor = color;
}

double GridScene::niceInterval(double approx) {
    if (approx <= 0.0) return 1.0;
    double exponent = std::floor(std::log10(approx));
    double fraction = approx / std::pow(10.0, exponent);
    double nice;
    if (fraction <= 1.0)       nice = 1.0;
    else if (fraction <= 2.0)  nice = 2.0;
    else if (fraction <= 5.0)  nice = 5.0;
    else                       nice = 10.0;
    return nice * std::pow(10.0, exponent);
}

void GridScene::drawBackground(QPainter* painter, const QRectF& rect) {
    QGraphicsScene::drawBackground(painter, rect);

    QList<QGraphicsView*> views = this->views();
    if (views.isEmpty()) return;
    QGraphicsView* view = views.first();

    QRectF visibleRect = view->mapToScene(view->viewport()->rect()).boundingRect();

    double gridInterval = niceInterval(visibleRect.width() / 8.0);

    double left = std::floor(visibleRect.left() / gridInterval) * gridInterval;
    double top = std::floor(visibleRect.top() / gridInterval) * gridInterval;

    QPen gridPen(m_gridColor, 1, Qt::DotLine);
    painter->setPen(gridPen);

    for (double x = left; x <= visibleRect.right(); x += gridInterval) {
        painter->drawLine(QPointF(x, visibleRect.top()), QPointF(x, visibleRect.bottom()));
    }
    for (double y = top; y <= visibleRect.bottom(); y += gridInterval) {
        painter->drawLine(QPointF(visibleRect.left(), y), QPointF(visibleRect.right(), y));
    }
}

void GridScene::drawForeground(QPainter* painter, const QRectF& rect) {
    QGraphicsScene::drawForeground(painter, rect);

    QList<QGraphicsView*> views = this->views();
    if (views.isEmpty()) return;
    QGraphicsView* view = views.first();

    QRectF visibleRect = view->mapToScene(view->viewport()->rect()).boundingRect();
    double gridInterval = niceInterval(visibleRect.width() / 8.0);

    double left = std::floor(visibleRect.left() / gridInterval) * gridInterval;
    double top = std::floor(visibleRect.top() / gridInterval) * gridInterval;

    QPen labelPen(m_labelColor);
    painter->setPen(labelPen);
    QFont font("Consolas", 9);
    painter->setFont(font);

    QFontMetrics fm(font);
    const int labelHeight = fm.height();
    const int labelPadding = 4;

    QRectF viewportRect = view->viewport()->rect();
    double marginX = 30.0 / view->transform().m11();
    double marginY = 20.0 / view->transform().m22();

    visibleRect.adjust(marginX, marginY, -marginX, -marginY);

    for (double x = left; x <= visibleRect.right(); x += gridInterval) {
        if (x < visibleRect.left() || x > visibleRect.right()) continue;
        QString label = QString::number(x, 'f', (gridInterval >= 1.0) ? 0 : 1);
        QPointF scenePos(x, visibleRect.bottom());
        QPoint viewPos = view->mapFromScene(scenePos);
        painter->drawText(QPointF(x, visibleRect.bottom() + marginY * 0.6), label);
    }

    for (double y = top; y <= visibleRect.bottom(); y += gridInterval) {
        if (y < visibleRect.top() || y > visibleRect.bottom()) continue;
        QString label = QString::number(-y, 'f', (gridInterval >= 1.0) ? 0 : 1);
        QPointF scenePos(visibleRect.left(), y);
        painter->drawText(QPointF(visibleRect.left() + marginX * 0.3, y + labelHeight * 0.3), label);
    }
}