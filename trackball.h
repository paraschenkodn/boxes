/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TRACKBALL_H
#define TRACKBALL_H

#include <QtWidgets>

#include <QtGui/qvector3d.h>
#include <QtGui/qquaternion.h>

class TrackBall
{
public:
    enum TrackMode
    {
        Plane,
        Sphere,
    };
    TrackBall(TrackMode mode = Sphere);                                                 //создаём орбиту для объекта по умоланию
    TrackBall(float angularVelocity, const QVector3D& axis, TrackMode mode = Sphere);   //создаём орбиту для объекта (угловая скорость, ось, модель вращения)
    // coordinates in [-1,1]x[-1,1]   // для операций используются нормализованые вектора (приведённые к радиусу единичной сферы ??)
    void push(const QPointF& p, const QQuaternion &transformation);
    void move(const QPointF& p, const QQuaternion &transformation);
    void release(const QPointF& p, const QQuaternion &transformation);
    void start(); // starts clock
    void stop(); // stops clock
    QQuaternion rotation() const;
    bool k_pressed;         /// моя вставка
private:
    QQuaternion m_rotation;     // квартерион вращения
    QVector3D m_axis;           // ось вращения
    float m_angularVelocity;    // угловая скорость вращения

    QPointF m_lastPos;
    QTime m_lastTime;
    bool m_paused;
    bool m_pressed;
    TrackMode m_mode;
};

#endif
