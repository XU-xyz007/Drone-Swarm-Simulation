#include "GridScene.h"
#include <QPainter>
#include <QGraphicsView>
#include <algorithm>
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

    const double scaleX = std::max(0.001, std::abs(view->transform().m11()));
    const double scaleY = std::max(0.001, std::abs(view->transform().m22()));
    const double marginX = 42.0 / scaleX;
    const double marginY = 28.0 / scaleY;

    visibleRect.adjust(marginX, marginY, -marginX, -marginY);

    // Labels are painted in viewport pixels rather than scene coordinates, so
    // their size remains stable when the user zooms the map.
    painter->save();
    painter->resetTransform();
    painter->setPen(QPen(m_labelColor));
    QFont font("Consolas");
    font.setPixelSize(13);
    painter->setFont(font);
    const QFontMetrics fm(font);
    const QRect viewportRect = view->viewport()->rect();

    for (double x = left; x <= visibleRect.right(); x += gridInterval) {
        if (x < visibleRect.left() || x > visibleRect.right()) continue;
        QString label = QString::number(x, 'f', (gridInterval >= 1.0) ? 0 : 1);
        const QPoint viewPos = view->mapFromScene(QPointF(x, visibleRect.bottom()));
        painter->drawText(viewPos.x() - fm.horizontalAdvance(label) / 2,
                          viewportRect.bottom() - 8, label);
    }

    for (double y = top; y <= visibleRect.bottom(); y += gridInterval) {
        if (y < visibleRect.top() || y > visibleRect.bottom()) continue;
        QString label = QString::number(-y, 'f', (gridInterval >= 1.0) ? 0 : 1);
        const QPoint viewPos = view->mapFromScene(QPointF(visibleRect.left(), y));
        painter->drawText(8, viewPos.y() + fm.ascent() / 2, label);
    }

    painter->restore();
}
