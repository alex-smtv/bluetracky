/* Copyright (c) 2013, 2015 Stanislaw Halik <sthalik@misaki.pl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#pragma once

#include "api/plugin-api.hpp"
#include "compat/euler.hpp"

#include "export.hpp"

#include <QCheckBox>
#include <QImage>
#include <QWidget>
#include <QLabel>

//#define TEST
namespace pose_widget_impl
{
using namespace euler;

struct OTR_POSE_WIDGET_EXPORT pose_widget final : QWidget
{
public:
    explicit pose_widget(QWidget* parent = nullptr);
    void present(double xAngle, double yAngle, double zAngle, double x, double y, double z, bool is_running);
    QCheckBox mirror{ "Mirror", this };
    QLabel orig_soft{ "original software: opentrack", this };

private:
    bool is_running;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent*) override;

    Pose_ R, T;
    QImage front{ QImage{ ":/images/bluetracky-face.png" }.convertToFormat(QImage::Format_ARGB32) };
    QImage back{ QImage{ ":/images/bluetracky-back.png" }.convertToFormat(QImage::Format_ARGB32).mirrored(true, false) };
    QImage shine_front{ QImage{ front.width(), front.height(), QImage::Format_ARGB32 } };
    QImage shadow_front{ QImage{ front.width(), front.height(), QImage::Format_ARGB32 } };
    QImage shine_back{ QImage{ back.width(), back.height(), QImage::Format_ARGB32 } };
    QImage shadow_back{ QImage{ back.width(), back.height(), QImage::Format_ARGB32 } };
};

} // namespace pose_widget_impl

using pose_widget = pose_widget_impl::pose_widget;
