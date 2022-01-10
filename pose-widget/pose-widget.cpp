/* Copyright (c) 2013, 2015 Stanislaw Halik <sthalik@misaki.pl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "pose-widget.hpp"
#include "compat/check-visible.hpp"
#include "compat/math.hpp"

#include <QPainter>
#include <QtEvents>

#include <QDebug>
#include <QMatrix4x4>
#include <QQuaternion>

namespace pose_widget_impl
{
pose_widget::pose_widget(QWidget* parent) : QWidget(parent)
{
    QPainter p;
#ifdef TEST
    // draw rectangle frame around of Octopus, only if TEST defined
    p.begin(&front);
    p.setPen(QPen(Qt::red, 3, Qt::SolidLine));
    p.drawRect(0, 0, front.width() - 1, front.height() - 1);
    p.end();

    p.begin(&back);
    p.setPen(QPen(Qt::darkGreen, 3, Qt::SolidLine));
    p.drawRect(0, 0, back.width() - 1, back.height() - 1);
    p.end();
#endif

    // draw Octopus shine
    shine_front.fill(QColor(255, 255, 255));
    p.begin(&shine_front);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawImage(QPointF(0, 0), front);
    p.end();

    shine_back.fill(QColor(255, 255, 255));
    p.begin(&shine_back);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawImage(QPointF(0, 0), back);
    p.end();

    // draw Octopus shadow
    shadow_front.fill(QColor(0, 0, 0));
    p.begin(&shadow_front);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawImage(QPointF(0, 0), front);
    p.end();
    
    shadow_back.fill(QColor(0, 0, 0));
    p.begin(&shadow_back);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawImage(QPointF(0, 0), back);
    p.end();

    // mirror checkbox
    mirror.setFocusPolicy(Qt::NoFocus);
    mirror.setChecked(true);

    // original software disclaimer
    orig_soft.setFocusPolicy(Qt::NoFocus);

    QFont font = orig_soft.font();
    font.setItalic(true);
    orig_soft.setFont(font);
}

void pose_widget::present(double yaw, double pitch, double roll, double x, double y, double z, bool is_running)
{
    T = { x, y, z };
    R = { yaw, pitch, roll };

    this->is_running = is_running;
    repaint();
}

void pose_widget::resizeEvent(QResizeEvent* event)
{
    // adapt to widget size
    float w = event->size().width();
    float h = event->size().height();

    // move the mirror checkbox in the lower right corner of the widget
    mirror.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    mirror.move(w - mirror.width(), h - mirror.height());

    // Calculate precise pixel-width of the text | https://stackoverflow.com/a/8638114
    float orig_soft_width_txt = orig_soft.fontMetrics().boundingRect(orig_soft.text()).width();

    orig_soft.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    orig_soft.move(w - orig_soft_width_txt, 0);
}

void pose_widget::paintEvent(QPaintEvent*)
{
    // widget settings:
    constexpr float scale = 0.5;   // scale of Octopus height, when x = y = z = 0.0
    constexpr float XYZmax = 50.0; // -XYZmax < x,y,z < +XYZmax (offset the Octopus by one body)
    constexpr float Kz = 0.25;     // Z scale change limit (simulate camera focus length)

    // get a local copy of input data
    auto [yaw, pitch, roll] = R;
    auto [x, y, z] = T;

    QPainter p(this);
#ifdef TEST
    // use antialiasing for correct frame around the Octopus, only if TEST defined
    p.setRenderHint(QPainter::Antialiasing, true);
#endif

    {
        p.fillRect(rect(), palette().brush(backgroundRole()));
        // draw axes
        p.save();
        p.setPen(QPen(Qt::gray, 1, Qt::SolidLine));
        int w = width(), h = height();
        p.drawLine(w / 2, 0, w / 2, h);
        p.drawLine(0, h / 2, w, h / 2);
        p.restore();
    }

    // check mirror state
    if (mirror.checkState() == Qt::Checked)
        x = -x;
    else
    {
        yaw = -yaw;
        roll = -roll;
    }
    y = -y;

    // rotations
    QQuaternion q = QQuaternion::fromEulerAngles(pitch, yaw, roll);
    QMatrix4x4 m = QMatrix4x4(q.toRotationMatrix());

    // x and y positions
    const float Kxy = (float)front.height() / XYZmax;
    QVector3D v(Kxy * x, Kxy * y, 0.0);
    v = m.transposed().map(v);
    m.translate(v);

    // perspective projection to x-y plane
    QTransform t = m.toTransform(1024).translate(-.5 * front.width(), -.5 * front.height());

    // z position by setViewport
    const float mz = scale * height() / front.height() / exp(1.0) * exp(1.0 - z * (Kz / XYZmax));
    p.setViewport(QRect(.5 * width(), .5 * height(), width() * mz, height() * mz));

    // define forward or backward side by cross product of mapped x and y axes
    QPointF point0 = t.map(QPointF(0, 0));
    QPointF x_dir = (t.map(QPointF(1, 0)) -= point0);
    QPointF y_dir = (t.map(QPointF(0, 1)) -= point0);
    const bool forward = x_dir.ry() * y_dir.rx() - x_dir.rx() * y_dir.ry() < 0 ? true : false;

    // draw front or back head
    p.setTransform(t);
    p.drawImage(QPointF(0, 0), forward ? front : back);

    if (this->is_running)
    {
        // top lighting simulation
        const float alpha = sin(pitch * M_PI / 180.0);
        bool is_shine = forward == (alpha >= 0.0);

        // set opacity darker for shadow
        p.setOpacity((is_shine ? 0.333 : 0.7) * fabs(alpha));

        // take into account front image and back images could be different
        if (abs(yaw) > 90)
        {
            p.drawImage(QPointF(0, 0), is_shine ? shine_back : shadow_back);
        }
        else
        {
            p.drawImage(QPointF(0, 0), is_shine ? shine_front : shadow_front);
        }
    } else
    {
        p.setOpacity(0.333);
        p.drawImage(QPointF(0, 0), shine_front);
    }
}

} // namespace pose_widget_impl
