#include "DroneItem.h"
#include "ColorConfig.h"
#include "core/DroneState.h"
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <vector>

DroneItem::DroneItem(uint32_t droneId, QGraphicsItem* parent)
    : QGraphicsEllipseItem(-RADIUS, -RADIUS, RADIUS * 2, RADIUS * 2, parent)
    , m_droneId(droneId)
{
    // The body radius is 0.5 m, so the former 1 m outline would cover the
    // entire fill colour. Keep a fine outline without hiding status colours.
    setPen(QPen(Qt::white, 0.08));
    setBrush(QColor(128, 128, 128));
    setZValue(2);
    setFlag(ItemIgnoresTransformations, false);

    m_trailTimer.start();
}

void DroneItem::updateFromState(const DroneState& state) {
    if (state.status == DroneStatus::ST_LANDED) {
        if (!m_landing) {
            m_landing = true;
            m_landingTimer.restart();
            m_trailPoints.clear();
            setBrush(colorForStatus(DroneStatus::ST_LANDED));
            setOpacity(1.0);
            update();
        }
        return;
    }

    // A reused ID has become active again; restore its normal presentation.
    if (m_landing) {
        m_landing = false;
        setOpacity(1.0);
    }
    const QPointF reportedPos(state.x, -state.y);
    if (!m_hasDisplayTarget) {
        m_displayTarget = reportedPos;
        m_hasDisplayTarget = true;
        setPos(reportedPos);
        setBrush(colorForStatus(state.status));
        update();
        return;
    }

    // DDS state arrives less frequently than the render timer.  Smooth only
    // the displayed item position to avoid visible step/jitter while keeping
    // the received state unchanged for all control and quality calculations.
    m_displayTarget = reportedPos;
    const QPointF oldPos = pos();
    const QPointF offset = m_displayTarget - oldPos;
    QPointF newPos = oldPos + offset * 0.45;
    if (std::hypot(offset.x(), offset.y()) < 0.01) {
        newPos = m_displayTarget;
    }

    if (m_trailPoints.empty() ||
        std::hypot(newPos.x() - oldPos.x(), newPos.y() - oldPos.y()) > 0.05) {
        m_trailPoints.push_back({newPos, m_trailTimer.elapsed()});
    }

    prepareGeometryChange();
    setPos(newPos);
    setBrush(colorForStatus(state.status));
    pruneTrail();
    update();
}

void DroneItem::pruneTrail() {
    qint64 now = m_trailTimer.elapsed();
    while (!m_trailPoints.empty() &&
           (now - m_trailPoints.front().elapsedMs) > TRAIL_DURATION_MS) {
        m_trailPoints.pop_front();
    }
}

QRectF DroneItem::boundingRect() const {
    QRectF base = QGraphicsEllipseItem::boundingRect();

    if (m_trailPoints.size() < 2)
        return base;

    QPointF origin = pos();
    for (const auto& tp : m_trailPoints) {
        QPointF local = tp.pos - origin;
        base = base.united(QRectF(local.x() - 3, local.y() - 3, 6, 6));
    }
    return base;
}

void DroneItem::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem* option,
                      QWidget* widget) {
    drawTrail(painter);
    QGraphicsEllipseItem::paint(painter, option, widget);
}

void DroneItem::drawTrail(QPainter* painter) {
    int n = static_cast<int>(m_trailPoints.size());
    if (n < 2) return;

    qint64 now = m_trailTimer.elapsed();
    QPointF origin = pos();

    std::vector<QPointF> pts(n);
    for (int i = 0; i < n; ++i) {
        pts[i] = m_trailPoints[i].pos - origin;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);

    for (int i = 0; i < n - 1; ++i) {
        qint64 age = now - m_trailPoints[i].elapsedMs;
        int alpha = static_cast<int>(200.0 * (1.0 - static_cast<double>(age) / TRAIL_DURATION_MS));
        if (alpha < 0) alpha = 0;
        if (alpha > 200) alpha = 200;
        if (alpha < 5) continue;

        QPointF p0 = pts[i];
        QPointF p1 = pts[i + 1];

        QPointF segDir = p1 - p0;
        double segLen = std::sqrt(segDir.x() * segDir.x() + segDir.y() * segDir.y());
        if (segLen < 0.001) continue;
        segDir /= segLen;

        QPointF ep0 = p0 + segDir * RADIUS;
        QPointF ep1 = p1 - segDir * RADIUS;

        QPointF t0, t1;
        if (i == 0) {
            t0 = (pts[1] - pts[0]) * 0.5;
        } else {
            t0 = (pts[i + 1] - pts[i - 1]) * 0.5;
        }
        if (i == n - 2) {
            t1 = (pts[n - 1] - pts[n - 2]) * 0.5;
        } else {
            t1 = (pts[i + 2] - pts[i]) * 0.5;
        }

        QPointF cp1 = ep0 + t0 / 3.0;
        QPointF cp2 = ep1 - t1 / 3.0;

        QPainterPath segPath;
        segPath.moveTo(ep0);
        segPath.cubicTo(cp1, cp2, ep1);

        // Keep the motion trail narrower than the 1 m body diameter.
        painter->setPen(QPen(QColor(255, 255, 255, alpha), 0.12));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(segPath);
    }
}

bool DroneItem::advanceLandingAnimation() {
    if (!m_landing) return false;

    const qreal progress = std::min<qreal>(
        1.0, static_cast<qreal>(m_landingTimer.elapsed()) / LANDING_FADE_DURATION_MS);
    setOpacity(1.0 - progress);
    update();
    return progress >= 1.0;
}
