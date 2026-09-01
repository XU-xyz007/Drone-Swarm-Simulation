#pragma once
#include <QGraphicsView>
#include <vector>

struct DroneState;

class CoordinateMapper {
public:
    CoordinateMapper() = default;

    void autoFit(QGraphicsView* view,
                 const std::vector<DroneState>& states,
                 double padding = 0.2);

    void resetView(QGraphicsView* view);

    void reset() { m_hasFitted = false; }

private:
    bool m_hasFitted = false;
};