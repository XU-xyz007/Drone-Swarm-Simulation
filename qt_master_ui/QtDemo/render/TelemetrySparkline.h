#pragma once

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QSizePolicy>
#include <QString>
#include <QVector>
#include <QWidget>

// A lightweight QtWidgets chart with no QtCharts dependency. It deliberately
// shows only the recent throughput trend, so the surrounding metrics remain
// the primary operational signal.
class TelemetrySparkline : public QWidget {
public:
    explicit TelemetrySparkline(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(92);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void appendSample(double value) {
        samples_.append(value < 0.0 ? 0.0 : value);
        while (samples_.size() > 45) samples_.removeFirst();
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(20, 31, 42));

        // Reserve the left gutter for an auto-scaled kbit/s axis.
        const QRect plot = rect().adjusted(54, 10, -10, -16);
        double maximum = 1.0;
        for (int i = 0; i < samples_.size(); ++i) maximum = qMax(maximum, samples_[i]);

        QFont axisFont = painter.font();
        axisFont.setPointSize(10);
        painter.setFont(axisFont);
        painter.setPen(QColor(126, 152, 169));
        painter.setPen(QPen(QColor(59, 79, 94), 1));
        for (int row = 0; row < 4; ++row) {
            const int y = plot.top() + row * plot.height() / 3;
            painter.drawLine(plot.left(), y, plot.right(), y);
            const double value = maximum * (3 - row) / 3.0;
            painter.setPen(QColor(126, 152, 169));
            painter.drawText(QRect(2, y - 10, plot.left() - 8, 20),
                             Qt::AlignRight | Qt::AlignVCenter,
                             formatAxisValue(value));
            painter.setPen(QPen(QColor(59, 79, 94), 1));
        }
        if (samples_.size() < 2) return;

        QPainterPath path;
        for (int i = 0; i < samples_.size(); ++i) {
            const double x = plot.left() + (static_cast<double>(i) / (samples_.size() - 1)) * plot.width();
            const double y = plot.bottom() - (samples_[i] / maximum) * plot.height();
            if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        painter.setPen(QPen(QColor(0, 220, 210), 3));
        painter.drawPath(path);
    }

private:
    static QString formatAxisValue(double value) {
        if (value >= 1000.0) return QString::number(value / 1000.0, 'f', 1) + "k";
        return QString::number(value, 'f', value < 10.0 ? 1 : 0);
    }

    QVector<double> samples_;
};
