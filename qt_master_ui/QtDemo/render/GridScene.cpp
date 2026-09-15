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

    double scaleX = std::abs(painter->worldTransform().m11());
    double scaleY = std::abs(painter->worldTransform().m22());
    double scale = std::max(scaleX, scaleY);
    if (scale < 0.001) scale = 0.001;

    QPen labelPen(m_labelColor);
    painter->setPen(labelPen);

    QFont font("Consolas");
    font.setPixelSize(static_cast<int>(std::max(1.0, 9.0 / scale)));
    painter->setFont(font);

    QFontMetrics fm(font);

    QRectF viewportRect = view->viewport()->rect();
    double marginX = 30.0 / scale;
    double marginY = 20.0 / scale;

    visibleRect.adjust(marginX, marginY, -marginX, -marginY);

    for (double x = left; x <= visibleRect.right(); x += gridInterval) {
        if (x < visibleRect.left() || x > visibleRect.right()) continue;
        QString label = QString::number(x, 'f', (gridInterval >= 1.0) ? 0 : 1);
        QPointF scenePos(x, visibleRect.bottom());
        painter->drawText(QPointF(x, visibleRect.bottom() + marginY * 0.6), label);
    }

    for (double y = top; y <= visibleRect.bottom(); y += gridInterval) {
        if (y < visibleRect.top() || y > visibleRect.bottom()) continue;
        QString label = QString::number(-y, 'f', (gridInterval >= 1.0) ? 0 : 1);
        painter->drawText(QPointF(visibleRect.left() + marginX * 0.3,
                                   y + fm.height() * 0.3), label);
    }
}