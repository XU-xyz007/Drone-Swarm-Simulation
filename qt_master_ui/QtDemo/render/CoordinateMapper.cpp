#include "CoordinateMapper.h"
#include "core/DroneState.h"
#include <algorithm>
#include <cmath>

void CoordinateMapper::autoFit(QGraphicsView* view,
                               const std::vector<DroneState>& states,
                               double padding) {
    if (states.empty() || !view || !view->scene()) return;

    double minX = 0, maxX = 0, minY = 0, maxY = 0;
    bool first = true;

    for (const auto& s : states) {
        if (s.transitioning) continue;
        if (s.status == DroneStatus::ST_LANDED) continue;
        double displayY = -s.y;
        if (first) {
            minX = s.x; maxX = s.x;
            minY = displayY; maxY = displayY;
            first = false;
        } else {
            minX = std::min(minX, s.x);
            maxX = std::max(maxX, s.x);
            minY = std::min(minY, displayY);
            maxY = std::max(maxY, displayY);
        }
    }

    if (first) return;

    double dx = maxX - minX;
    double dy = maxY - minY;
    if (dx < 400.0) dx = 400.0;
    if (dy < 400.0) dy = 400.0;

    double padX = dx * padding;
    double padY = dy * padding;

    QRectF newRect(minX - padX, minY - padY, dx + 2 * padX, dy + 2 * padY);

    if (!m_hasFitted) {
        view->scene()->setSceneRect(newRect);
        view->fitInView(newRect, Qt::KeepAspectRatio);
        m_hasFitted = true;
        return;
    }

    QRectF currentRect = view->scene()->sceneRect();
    double marginX = currentRect.width() * 0.1;
    double marginY = currentRect.height() * 0.1;

    if (!currentRect.adjusted(marginX, marginY, -marginX, -marginY).contains(newRect)) {
        double smoothPadX = std::max(padX, currentRect.width() * 0.15);
        double smoothPadY = std::max(padY, currentRect.height() * 0.15);
        QRectF expandedRect(minX - smoothPadX, minY - smoothPadY,
                            dx + 2 * smoothPadX, dy + 2 * smoothPadY);
        view->scene()->setSceneRect(expandedRect);
    }
}

void CoordinateMapper::resetView(QGraphicsView* view) {
    if (!view || !view->scene()) return;
    view->fitInView(view->scene()->sceneRect(), Qt::KeepAspectRatio);
}