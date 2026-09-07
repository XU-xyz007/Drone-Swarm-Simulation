#pragma once
#include <QGraphicsScene>

class GridScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit GridScene(QObject* parent = nullptr);

    void setGridColor(const QColor& color);
    void setLabelColor(const QColor& color);

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;

private:
    static double niceInterval(double approx);

    QColor m_gridColor = QColor(60, 60, 60);
    QColor m_labelColor = QColor(150, 150, 150);
};