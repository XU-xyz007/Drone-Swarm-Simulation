#pragma once
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <QElapsedTimer>
#include <cstdint>
#include <deque>

struct DroneState;

struct TrailPoint {
    QPointF pos;
    qint64 elapsedMs;
};

class DroneItem : public QGraphicsEllipseItem {
public:
    explicit DroneItem(uint32_t droneId, QGraphicsItem* parent = nullptr);

    void updateFromState(const DroneState& state);

    uint32_t droneId() const { return m_droneId; }

    QRectF boundingRect() const override;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

private:
    void drawTrail(QPainter* painter);
    void pruneTrail();

    uint32_t m_droneId;
    std::deque<TrailPoint> m_trailPoints;
    QElapsedTimer m_trailTimer;
    static constexpr qreal PHYSICAL_RADIUS = 0.5;
    static constexpr qint64 TRAIL_DURATION_MS = 1500;
};