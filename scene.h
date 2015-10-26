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

#ifndef SCENE_H
#define SCENE_H

//#include <GL/glew.h>
#include "glextensions.h"

#include <QtWidgets>
#include <QtOpenGL>

//#include "roundedbox.h"
#include "gltrianglemesh.h"
//#include "trackball.h"
#include "glbuffers.h"
#include "qtbox.h"
#include "dialogboxes.h"

#define PI 3.14159265358979

QT_BEGIN_NAMESPACE
class QMatrix4x4;
QT_END_NAMESPACE

// УСТАНОВКА СЦЕНЫ, констуктор и инициализация объектов
class Scene : public QGraphicsScene
{
    Q_OBJECT
public:
    Scene(int width, int height, int maxTextureSize);
    ~Scene();
    virtual void drawBackground(QPainter *painter, const QRectF &rect) Q_DECL_OVERRIDE;

public slots:
    void setShader(int index);                  // функция установки шейдеров на центральный куб, в параметрах индекс шейдера
    void setTexture(int index);                 // функция установки тестур на центральный куб, в параметрах индекс текстуры
    void toggleDynamicCubemap(int state);                           // установка динамических текстур для объектов (включает отражение других объектов)
    void setColorParameter(const QString &name, QRgb color);        // установка цвета объетов, в параметрах - ??????
    void setFloatParameter(const QString &name, float value);       // установка цвета объетов, в параметрах - ??????
    void newItem(ItemDialog::ItemType type);                    // рисуем статические объекты
protected:
    void setStates();                                               //
    void setLights();                                               //
    void defaultStates();                                           //

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE;      // работка с мышью, переопределяем функции обработки сообщений мыши (нажатие кнопок)
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE;    // переопределяем функции обработки сообщений мыши (отпускание кнопки)
    ///*********
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) Q_DECL_OVERRIDE;       // переопределяем функции обработки сообщений мыши (перемещение)
    ///* если будем водить мышью по форме диалога с нажатой клавишей, сообщения также получает сцена
    /// ********
    virtual void wheelEvent(QGraphicsSceneWheelEvent * event) Q_DECL_OVERRIDE;          // переопределяем функции обработки сообщений мыши (колесо)
    // надо заметить что при включенных динамических текстурах здесь масштаб отражений не меняется
    /// это моя вставка
    virtual void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;   // переопределяем функцию обработки сообщений клавиатуры
    /// если мы объявим функцию обработки сообщений клавиатуры как
    // void keyPressEvent(QKeyEvent *event); // можете попробовать
    /// то она будет принадлежать конкретно к нашей сцене и итемы, объекты которые не принадлежат к нашей сцене
    /// (т.е. parent не указывает на неё)) не будут получать нажатий клавиш клавиатуры, а так будут получать ВСЕ,
    /// нужно фильтровать, какому итему принадлежит сообщение
    /// *** это моя вставка end
private:
    void initGL();                                      // инициализация OpenGL
    QPointF pixelPosToViewPos(const QPointF& p);        // пересчёт координат экрана и сцены (ArcBall Rotation - http://pmg.org.ru/nehe/nehe48.htm)

    ///QTime m_time;    /// закоментируем лишнюю неиспользуемую переменную
    int m_lastTime;
    int m_mouseEventTime;
    int m_distExp;
    int m_frame;
    int m_maxTextureSize;

    int m_currentShader;            // текущий шейдер (индекс шейдера)
    int m_currentTexture;           // текущая текстура (индекс текстуры)
    bool m_dynamicCubemap;          // флаг включения динамической текстуры
    bool m_updateAllCubemaps;       // флаг необходимости перерисовки динамических текстур ???

    RenderOptionsDialog *m_renderOptions;       // окно параметров  (1 сторона)
    ItemDialog *m_itemDialog;                   // окно выбора новых объектов (2 сторона)
    QTimer *m_timer;                            // таймер анимации
    QVector<GLTexture *> m_textures;            //
    GLTexture3D *m_noise;                       //
    QVector<GLRenderTargetCube *> m_cubemaps;   //  -- динамические текстуры для круга кубов
    QVector<QGLShaderProgram *> m_programs;     //
    QGLShader *m_vertexShader;                  // переменная текущего ??? шейдера
    QVector<QGLShader *> m_fragmentShaders;     //
    GLTextureCube *m_environment;               // - фон - http://antongerdelan.net/opengl/cubemaps.html
    QGLShader *m_environmentShader;             //
    QGLShaderProgram *m_environmentProgram;     //
};

#endif
