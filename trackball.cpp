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

#include "trackball.h"
#include "scene.h"
#include <cmath>

//============================================================================//
//                                 TrackBall (Сфера вращения) (описаная сфера)//
//============================================================================//

/// Заметки
/// Кватернионы расширяют концепцию поворота в трех направлениях до концепции поворота в четырех направлениях.
/// Кватернионы можно использовать для поворота объекта вокруг вектора (x, y, z) на угол theta,
/// где w = cos(theta/2). С точки зрения вычислений операции с кватернионами более эффективны, чем операции умножения матриц 4 × 4,
/// используемые для преобразований и поворотов.
/// Кватернион представляет также наиболее эффективный поворот для интерполяции между двумя ориентациями объекта.
/// Кватернионы добавляют четвертый элемент к значениям [x, y, z], определяющим вектор, в результате чего получаются произвольные 4-мерные вектора.
/// Однако следующие формулы показывают, как каждый элемент единичного кватерниона связан с поворотом на угол относительно оси,
/// где q представляет единичный кватернион (x, y, z, w), ось нормализована и theta — желаемый поворот против часовой стрелки вокруг этой оси.
/// q.x = sin(theta/2) * axis.x
/// q.y = sin(theta/2) * axis.y
/// q.z = sin(theta/2) * axis.z
/// q.w = cos(theta/2)

TrackBall::TrackBall(TrackMode mode)  // создаём орбиту по умолчанию (угловая скорость, ось, модель вращения)
    : m_angularVelocity(0)
    , m_paused(false)
    , m_pressed(false)
    , k_pressed(false)
    , m_mode(mode)
{
    m_axis = QVector3D(0, 1, 0);            // вектор вращения вдоль оси Y
    m_rotation = QQuaternion();             // создаём квартернион вращения
    m_lastTime = QTime::currentTime();      // зафиксируем текущее время (для вычислений с угловой скоростью)
}

// создаём орбиту объекта (угловая скорость, ось, модель вращения)
TrackBall::TrackBall(float angularVelocity, const QVector3D& axis, TrackMode mode)
    : m_axis(axis)
    , m_angularVelocity(angularVelocity)
    , m_paused(false)
    , m_pressed(false)
    , k_pressed(false)
    , m_mode(mode)                          // задаём модель вращения
{
    m_rotation = QQuaternion();             // создаём кватернион вращения
    m_lastTime = QTime::currentTime();      // зафиксируем текущее время (для вычислений с угловой скоростью)
}

// штатная функция анимации поворота
// функция возвращает итоговый кватернион (поворот вокруг заданой оси за прошедшее время с учётом последнего поворота)
QQuaternion TrackBall::rotation() const
{
    if (m_paused || m_pressed)
        return m_rotation;

    QTime currentTime = QTime::currentTime();                           // получаем текущее время
    float angle = m_angularVelocity * m_lastTime.msecsTo(currentTime);  // вычисляем угол поворота от прошлого отсчёта времени (на сколько уже повернулся объект)
    return QQuaternion::fromAxisAndAngle(m_axis, angle) * m_rotation;   // возвращает нормализированный квартернион соответствующий повороту вокруг оси m_axis на угол поворота angle c учётом предыдущего поворота
}

// Эта функция .... (для чего??)
// первый параметр текущая точка в которой щёлкнули кнопкой (пересчитанной из экранной в координаты (сцены??))
// второй параметр это сопряженный кватернион (-x, -y, -z, w) от текущего ()
///
/// \brief TrackBall::push  //
/// \param p                //
///
void TrackBall::push(const QPointF& p, const QQuaternion &)
{
    m_rotation = rotation();            // запоминаем текущее значение поворота в кватернионе (с учётом прошедшего времени)
    m_pressed = true;                   // ВЗВОДИМ флаг нажатия клавиш мыши
    m_lastTime = QTime::currentTime();  // запоминаем время когда запомнили текущий поворот (здесь возникает лаг с currentTime так как прошло время машшинной обработки команд и currentTime в функции rotation() != m_lastTime)
    m_lastPos = p;                      // запоминаем текущую точку в которой щёлкнули кнопкой (пересчитанной из экранной в координаты (сцены??))
    m_angularVelocity = 0.0f;           // устанавливаем угловую скорость равной 0 (тормозим вращение)
}

// функция вращения мышью по орбите
// ArcBall Rotation (Куб вращения (вписанная в куб сфера вращения)).
// http://pmg.org.ru/nehe/nehe48.htm
// sphere - пересчёт разницы в позициях точек в (сферических и угловых???) координатах
// plane - пересчёт разницы в позициях точек в (линейных???) координатах
/// входящие параметры
/// 1 - текущая точка в которой щёлкнули кнопкой (пересчитанной из экранной в координаты (сцены??))
/// 2 - новый сопряженный кватернион текущей расчитанной позиции расчитанной из угловой скорости
void TrackBall::move(const QPointF& p, const QQuaternion &transformation)
{
    if (!m_pressed)
        return;

    int msecs;                                  // перенесли объявление повыше
    QTime currentTime = QTime::currentTime();   // и взятие текущего времени тоже
    msecs = m_lastTime.msecsTo(currentTime);    // дельта времени

    if (k_pressed)      /// моя вставка
        msecs = 21;     // время нажатия клавиши для расчёта угловой скорости

    if (msecs <= 20)
        return;

    switch (m_mode) {
    case Plane:
        {
            QLineF delta(m_lastPos, p);
            m_angularVelocity = 180*delta.length() / (PI*msecs);
            m_axis = QVector3D(-delta.dy(), delta.dx(), 0.0f).normalized();
            m_axis = transformation.rotatedVector(m_axis);
            m_rotation = QQuaternion::fromAxisAndAngle(m_axis, 180 / PI * delta.length()) * m_rotation;
        }
        break;
    case Sphere:
        {
            QVector3D lastPos3D = QVector3D(m_lastPos.x(), m_lastPos.y(), 0.0f);
            float sqrZ = 1 - QVector3D::dotProduct(lastPos3D, lastPos3D);
            if (sqrZ > 0)
                lastPos3D.setZ(std::sqrt(sqrZ));
            else
                lastPos3D.normalize();

            QVector3D currentPos3D = QVector3D(p.x(), p.y(), 0.0f);
            sqrZ = 1 - QVector3D::dotProduct(currentPos3D, currentPos3D);
            if (sqrZ > 0)
                currentPos3D.setZ(std::sqrt(sqrZ));
            else
                currentPos3D.normalize();

            m_axis = QVector3D::crossProduct(lastPos3D, currentPos3D);                // расчитываем новый вектор оси вращения
            float angle = 180 / PI * std::asin(std::sqrt(QVector3D::dotProduct(m_axis, m_axis)));

            m_angularVelocity = angle / msecs;      // от движения мышью расчитываем новую угловую скорость
            m_axis.normalize();                     //
            m_axis = transformation.rotatedVector(m_axis);  // расчитываем новый вектор оси вращения
            m_rotation = QQuaternion::fromAxisAndAngle(m_axis, angle) * m_rotation; // расчитываем и запоминаем новое положение объекта
        }
        break;
    }


    m_lastPos = p;              // запоминаем последнюю позицию мыши
    m_lastTime = currentTime;   // запоминаем последнюю позицию времени
}

// функция выполняется в момент отпускания кнопок мыши, при этом запоминаются последняя угловая скорость и вектор оси вращения
void TrackBall::release(const QPointF& p, const QQuaternion &transformation)
{
    // Calling move() caused the rotation to stop if the framerate was too low.
    move(p, transformation);
    m_pressed = false;  //  функция move() больше не отрабатывает, далее работает только rotation()
}

// функция проста как три рубля, запоминает текущую позицию времени для дальнейшего расчёта углов поворота по орбитам (сферам врещения)
// и сбрасывает флаг паузы, стартуя анимацию
void TrackBall::start()
{
    m_lastTime = QTime::currentTime();
    m_paused = false;
}

// функция остановки анимации объектов, устанавливается флаг паузы
// (в оригинале требует доработки, т.к. кубы вращаются с миром, а не удерживаются зажатой мышью, в одном положении перед камерой)
// запоминает последнюю текущую позицию вращения (далее по идее шевелится мышью)
void TrackBall::stop()
{
    m_rotation = rotation();
    m_paused = true;
}

