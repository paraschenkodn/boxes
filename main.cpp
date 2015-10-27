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

#include "scene.h"

#include <QtWidgets>
#include <QOpenGLWidget>  //Это теперь пишем вместо QtWidgets
#include <QSurfaceFormat> //Это теперь пишем вместо QGLFormat

#include "oglw.h"

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView()
    {
        setWindowTitle(tr("Boxes"));
        setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        //setRenderHints(QPainter::SmoothPixmapTransform);
    }

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE {             // блокируем прорисовку сцены по умолчанию пока её нет // объявляем перекрывающую функцию и сразу пишем реализацию (и так тоже оказывается можно)
        if (scene())                                                    // если есть сцена, при изменении окна, то
            scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));  // устанавливаем новый размер отсечения области рисования  (в котором располагается диалоговое окно, на саму сцену это не влияет)
        QGraphicsView::resizeEvent(event);                              // реализовываем перекрывающую функцию в классе (техника написания ...)
    }
};


int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    //**************************
    /// Определяем версию OpenGL
    /// Если всё плохо, выходим
    //**************************

    if ((QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_1_5) == 0) {
        QMessageBox::critical(0, "OpenGL features missing",
            "OpenGL version 1.5 or higher is required to run this demo.\n"
            "The program will now exit.");
        return -1;
    }

    // Создаём виджет, на котором будем рисовать
    int maxTextureSize = 1024;
    //**** старое
    ///QGLWidget *widget = new QGLWidget(QGLFormat(QGL::SampleBuffers)); /// старое в мусор

    //**** новое
    // QOpenGLWidget ВСЕГДА отрисовывает в буфере, поэтому надо использовать буфер
    //QOpenGLWidget *widget = new QOpenGLWidget(0);  // пишем на новом классе
    OGLW *widget = new OGLW();
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 0);
    format.setProfile(QSurfaceFormat::NoProfile);
    widget->setFormat(format); // must be called before the widget or its parent window gets shown//*/


    GraphicsView view;                          // создаём экземпляр дочернего класса (смотрим определение выше)
    view.setViewport(widget);
    view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    Scene scene(1024, 768, maxTextureSize);     // создаём экземпляр дочернего класса (смотрим, чито у нас там в классе наворочено)
    view.setScene(&scene);
    view.show();

    return app.exec();
}
