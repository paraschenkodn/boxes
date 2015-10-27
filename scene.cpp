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

#include <QDebug>
#include "scene.h"
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qvector3d.h>
#include <cmath>

#include "3rdparty/fbm.h"

void qgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    const GLdouble ymax = zNear * tan(fovy * M_PI / 360.0);
    const GLdouble ymin = -ymax;
    const GLdouble xmin = ymin * aspect;
    const GLdouble xmax = ymax * aspect;
    glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

void checkGLErrors(const QString& prefix)
{
    switch (glGetError()) {
    case GL_NO_ERROR:
        //qDebug() << prefix << tr("No error.");
        break;
    case GL_INVALID_ENUM:
        qDebug() << prefix << QObject::tr("Invalid enum.");
        break;
    case GL_INVALID_VALUE:
        qDebug() << prefix << QObject::tr("Invalid value.");
        break;
    case GL_INVALID_OPERATION:
        qDebug() << prefix << QObject::tr("Invalid operation.");
        break;
    case GL_STACK_OVERFLOW:
        qDebug() << prefix << QObject::tr("Stack overflow.");
        break;
    case GL_STACK_UNDERFLOW:
        qDebug() << prefix << QObject::tr("Stack underflow.");
        break;
    case GL_OUT_OF_MEMORY:
        qDebug() << prefix << QObject::tr("Out of memory.");
        break;
    default:
        qDebug() << prefix << QObject::tr("Unknown error.");
        break;
    }
}

//============================================================================//
//                                    Scene                                   //
//============================================================================//

Scene::Scene(int width, int height, int maxTextureSize)
    : m_distExp(600)
    , m_frame(0)
    , m_maxTextureSize(maxTextureSize)
    , m_currentShader(0)
    , m_currentTexture(0)
    , m_dynamicCubemap(false)
    , m_updateAllCubemaps(true)
    , m_vertexShader(0)
    , m_environmentShader(0)
    , m_environmentProgram(0)
{
    setSceneRect(0, 0, width, height);  // устанавливаем прямоугольник отсечения сцены

    m_renderOptions = new RenderOptionsDialog;              // создаём панель управления №1
    m_renderOptions->move(20, 120);                         // перемещаем её в угол
    m_renderOptions->resize(m_renderOptions->sizeHint());   // устанавливаем размер по рекомендованному

    // с диалоговыми панелями сцена OpenGL общается через систему сигналов
    connect(m_renderOptions, SIGNAL(dynamicCubemapToggled(int)), this, SLOT(toggleDynamicCubemap(int)));                    //
    connect(m_renderOptions, SIGNAL(colorParameterChanged(QString,QRgb)), this, SLOT(setColorParameter(QString,QRgb)));
    connect(m_renderOptions, SIGNAL(floatParameterChanged(QString,float)), this, SLOT(setFloatParameter(QString,float)));
    //connect(m_renderOptions, SIGNAL(textureChanged(int)), this, SLOT(setTexture(int)));
    connect(m_renderOptions, SIGNAL(shaderChanged(int)), this, SLOT(setShader(int)));

    // создаём панель управления №2, которая также общается посредством сигналов
    m_itemDialog = new ItemDialog;
    //connect(m_itemDialog, SIGNAL(newItemTriggered(ItemDialog::ItemType)), this, SLOT(newItem(ItemDialog::ItemType)));

    // формируем двухсторонний виджет панели управления
    TwoSidedGraphicsWidget *twoSided = new TwoSidedGraphicsWidget(this);
    twoSided->setWidget(0, m_renderOptions);
    twoSided->setWidget(1, m_itemDialog);
    // связываем его сигналами
    connect(m_renderOptions, SIGNAL(doubleClicked()), twoSided, SLOT(flip()));
    connect(m_itemDialog, SIGNAL(doubleClicked()), twoSided, SLOT(flip()));

    // добавляем на сцену кубики QT
    //addItem(new QtBox(64, width - 64, height - 64));
    //addItem(new QtBox(64, width - 64, 64));
    //addItem(new QtBox(64, 64, height - 64));
    //addItem(new QtBox(64, 64, 64));

    //initGL();   // инициализируем OpenGL

    // запускаем таймер анимации и привязываем его к обновлению сцены
    m_timer = new QTimer(this);
    m_timer->setInterval(20);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
    m_timer->start();

    ///m_time.start();  /// закоментируем лишнюю неиспользуемую переменную
}

Scene::~Scene()
{
    //foreach (GLTexture *texture, m_textures) if (texture) delete texture;
    foreach (QGLShaderProgram *program, m_programs)
        if (program) delete program;
    if (m_vertexShader)
        delete m_vertexShader;
    foreach (QGLShader *shader, m_fragmentShaders)
        if (shader) delete shader;
    //foreach (GLRenderTargetCube *rt, m_cubemaps) if (rt) delete rt;
    if (m_environmentShader)
        delete m_environmentShader;
    if (m_environmentProgram)
        delete m_environmentProgram;
}

void Scene::initGL()
{

    m_vertexShader = new QGLShader(QGLShader::Vertex);                                      // создаём переменную шейдеров
    m_vertexShader->compileSourceFile(QLatin1String(":/res/boxes/basic.vsh"));              // компилируем шейдеры

    // рисуем фон
    const static char environmentShaderText[] =             // шейдер для куба фона
        "uniform samplerCube env;"
        "void main() {"
            "gl_FragColor = textureCube(env, gl_TexCoord[1].xyz);"
        "}";
    QStringList list;                                                                       // формируем список текстур фона
    list << ":/res/boxes/cubemap_posx.jpg" << ":/res/boxes/cubemap_negx.jpg" << ":/res/boxes/cubemap_posy.jpg"
         << ":/res/boxes/cubemap_negy.jpg" << ":/res/boxes/cubemap_posz.jpg" << ":/res/boxes/cubemap_negz.jpg";
    //m_environment = new GLTextureCube(list, qMin(1024, m_maxTextureSize));                  // создаём куб фона
    m_environmentShader = new QGLShader(QGLShader::Fragment);                               //
    m_environmentShader->compileSourceCode(environmentShaderText);
    m_environmentProgram = new QGLShaderProgram;
    m_environmentProgram->addShader(m_vertexShader);        //  добавляем программу
    m_environmentProgram->addShader(m_environmentShader);   //  к ней ещё одну (в GPU программа одна, это у нас она разбита)
    m_environmentProgram->link();


    QStringList filter;                                                                     // фильтр выбора файлов
    QList<QFileInfo> files;                                                                 // список файлов

    // Load all .png files as textures                                                      // загружаем все png файлы как текстуры  (куда грузим??)
    m_currentTexture = 0;                                                                   // индекс текущей текстуры
    filter = QStringList("*.png");
    files = QDir(":/res/boxes/").entryInfoList(filter, QDir::Files | QDir::Readable);       // наполняем список в соответствии с фильтром из файлов зарегистрированных как ресурс
/*
    foreach (QFileInfo file, files) {                                                       // для каждого файла
        GLTexture *texture = new GLTexture2D(file.absoluteFilePath(), qMin(256, m_maxTextureSize), qMin(256, m_maxTextureSize));        // m_maxTextureSize определено 1024 в main.cpp, вот только qMin вернёт 256
        if (texture->failed()) {
            delete texture;
            continue;
        }
        m_textures << texture;                              // ??? закидываем в массив текстур  (куда закидываем???)
        m_renderOptions->addTexture(file.baseName());       // с соответствующим индексом будет имя текстуры в панели управления
    }

    if (m_textures.size() == 0)                                                                 // если не удалось запихать текстуры
        m_textures << new GLTexture2D(qMin(64, m_maxTextureSize), qMin(64, m_maxTextureSize));  // ??? формируем текстуру по умолчанию???
//*/

    // Load all .fsh files as fragment shaders                                         // загружаем все фрагментные шейдеры
    m_currentShader = 0;                                                                        // указатель индекса текущего шейдера
    filter = QStringList("*.fsh");                                                              // устанавливаем маску выбора файлов
    files = QDir(":/res/boxes/").entryInfoList(filter, QDir::Files | QDir::Readable);           //
    foreach (QFileInfo file, files) {
        QGLShaderProgram *program = new QGLShaderProgram;                                       // создаём новую программу для каждого файла
        QGLShader* shader = new QGLShader(QGLShader::Fragment);                                 // создаём новый шейдер для каждого файла
        shader->compileSourceFile(file.absoluteFilePath());                                     // компилируем шейдеры
        /// The program does not take ownership over the shaders, so store them in a vector so they can be deleted afterwards.
        program->addShader(m_vertexShader);                                                     // комбинируем программу из уже созданной основной вертексной и дополнительными фрагментными программами
        program->addShader(shader);                                                             //
        if (!program->link()) {                                                                 // линкуем программу  (куда?)
            qWarning("Failed to compile and link shader program");
            qWarning("Vertex shader log:");
            qWarning() << m_vertexShader->log();
            qWarning() << "Fragment shader log ( file =" << file.absoluteFilePath() << "):";
            qWarning() << shader->log();
            qWarning("Shader program log:");
            qWarning() << program->log();

            delete shader;
            delete program;
            continue;                   // дальше обрабатывать файл бесполезно, возвращаемся к началу цикла
        }

        m_fragmentShaders << shader;                    // запихиваем фрагментный шейдер в массив фрагментных шейдеров
        m_programs << program;                          // программу в массив программ
        m_renderOptions->addShader(file.baseName());    // имя файлов в массив списка эффектов
/*
        program->bind();                                            // связываем программу (с чем???)
        m_cubemaps << ((program->uniformLocation("env") != -1)                      // если в шейдерной программе есть переменная "env" то в массив (??? cubemaps)
                       ? new GLRenderTargetCube(qMin(256, m_maxTextureSize)) : 0);  // пихаем новый объект (??? карты текстур) либо 0
        program->release();                                                                     // удаляем уже ненужный экземпляр программы
//*/
    }

    if (m_programs.size() == 0)                         // если с программами потерпели фиаско,
        m_programs << new QGLShaderProgram;             // ???? запихиваем в массив программу по умолчанию

    m_renderOptions->emitParameterChanged();            // отсылаем сигналы изменения параметров отрисовки (для рисования)
}

static void loadMatrix(const QMatrix4x4& m)             //// грузим массив данных из матрицы одного типа в другой
{
    // static to prevent glLoadMatrixf to fail on certain drivers
    static GLfloat mat[16];
    const float *data = m.constData();
    for (int index = 0; index < 16; ++index)
        mat[index] = data[index];
    glLoadMatrixf(mat);                             // грузим массив данных из матрицы одного типа в другой (зачем????)
}

void Scene::setStates()
{
    //glClearColor(0.25f, 0.25f, 0.5f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    //glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    setLights();

    float materialSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);
}

void Scene::setLights()
{
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    float lightColour[] = {1.0f, 0.9f, 0.9f, 1.0f};
    //float lightColour[] = {1.0f, 1.0f, 1.0f, 1.0f};
    //float lightDir[] = {0.0f, 0.0f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColour);
    //glLightfv(GL_LIGHT0, GL_SPECULAR, lightColour);
    //glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
    glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);
    glEnable(GL_LIGHT0);
}

void Scene::defaultStates()
{
    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    //glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHT0);
    glDisable(GL_NORMALIZE);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 0.0f);
    float defaultMaterialSpecular[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultMaterialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
}

void Scene::drawBackground(QPainter *painter, const QRectF &)
{
    float width = float(painter->device()->width());
    float height = float(painter->device()->height());

    painter->beginNativePainting();
    setStates();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    qgluPerspective(60.0, width / height, 0.01, 15.0);

    glMatrixMode(GL_MODELVIEW);

    QMatrix4x4 view;
    //view.rotate(m_trackBalls[2].rotation());
    view(2, 3) -= 2.0f * std::exp(m_distExp / 1200.0f);
    //renderBoxes(view);

    defaultStates();
    ++m_frame;

    painter->endNativePainting();
}

// ArcBall Rotation
// http://pmg.org.ru/nehe/nehe48.htm
// масштабируем, координаты мыши из диапазона [0…ширина], [0...высота] в диапазон [-1...1], [1...-1]
// (запомните, что мы меняем знак координаты Y, чтобы получить корректный результат в OpenGL)
QPointF Scene::pixelPosToViewPos(const QPointF& p)
{
    return QPointF(2.0 * float(p.x()) / width() - 1.0,
                   1.0 - 2.0 * float(p.y()) / height());
}

void Scene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseMoveEvent(event);
    if (event->isAccepted())
        return;

    if (event->buttons() & Qt::LeftButton) {
        //m_trackBalls[0].move(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    } else {
        //m_trackBalls[0].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
    }

    if (event->buttons() & Qt::RightButton) {
        //m_trackBalls[1].move(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    } else {
        //m_trackBalls[1].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
    }

    if (event->buttons() & Qt::MidButton) {
        //m_trackBalls[2].move(pixelPosToViewPos(event->scenePos()), QQuaternion());
        event->accept();
    } else {
        //m_trackBalls[2].release(pixelPosToViewPos(event->scenePos()), QQuaternion());
    }
}

void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;

    if (event->buttons() & Qt::LeftButton) {
        //m_trackBalls[0].push(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();  // если убрать ничего не меняется, движение не компенсируется
        qDebug() << "X=" << pixelPosToViewPos(event->scenePos()).x() << " Y=" << pixelPosToViewPos(event->scenePos()).y() << " x=" << event->scenePos().x() << " y=" << event->scenePos().y();
    }

    if (event->buttons() & Qt::RightButton) {
        //m_trackBalls[1].push(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    }

    if (event->buttons() & Qt::MidButton) {
        //m_trackBalls[2].push(pixelPosToViewPos(event->scenePos()), QQuaternion());
        event->accept();
    }
}

void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    if (event->isAccepted())
        return;

    if (event->button() == Qt::LeftButton) {
        //m_trackBalls[0].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    }

    if (event->button() == Qt::RightButton) {
        //m_trackBalls[1].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    }

    if (event->button() == Qt::MidButton) {
        //m_trackBalls[2].release(pixelPosToViewPos(event->scenePos()), QQuaternion());
        event->accept();
    }
}

void Scene::wheelEvent(QGraphicsSceneWheelEvent * event)
{
    QGraphicsScene::wheelEvent(event);
    if (!event->isAccepted()) {
        m_distExp += event->delta();
        if (m_distExp < -8 * 120)
            m_distExp = -8 * 120;
        if (m_distExp > 10 * 120)
            m_distExp = 10 * 120;
        event->accept();
    }
}

/// это моя вставка
QPointF point=QPointF(0,0);             // задаём переменную точки смещения для задания угловой скорости и расчёта вектора оси вращения
void Scene::keyPressEvent(QKeyEvent *event)
{
    QGraphicsScene::keyPressEvent(event);
    if (event->isAccepted()) return; // блокирует приём обработанных сообщений, например из диалогового окна

    //m_trackBalls[0].push(QPointF(0,0), m_trackBalls[2].rotation().conjugate());  // воспользуемся уже имеющейся функцией запоминания текущей позиции сферы вращения
    //m_trackBalls[0].k_pressed=true;                                              // указываем что работаем клавиатурой (по этому флагу задаётся время расчёта угловой скорости)

    switch (event->key()) {
    case Qt::Key_Up:
          point.setY(point.y()+0.001);                  // устанавливаем координаты точки смещения
      break;
    case Qt::Key_Left:
          point.setX(point.x()-0.001);
      break;
    case Qt::Key_Right:
          point.setX(point.x()+0.001);
      break;
    case Qt::Key_Down:
          point.setY(point.y()-0.001);
      break;
  case Qt::Key_W:
    break;
  case Qt::Key_S:
    break;
  case Qt::Key_A:
        //--m_angle;
        //if (m_angle<0) m_angle=359;
    break;
  case Qt::Key_D:
        //++m_angle;
        //if (m_angle>=360) m_angle=0;
    break;
    default:
      break;
    }
  //m_trackBalls[0].release(point, m_trackBalls[2].rotation().conjugate());       // вызываем функцию расчёта угловой скорости и расчёта вектора оси вращения
  event->accept();                                                          //блокируем передачу сообщения конкретно дальше по сцене
  //m_trackBalls[0].k_pressed=false;                                              // сбрасываем флаг работы с клавиатурой
}//*/
/// *** это моя вставка end

void Scene::setShader(int index)
{
    if (index >= 0 && index < m_fragmentShaders.size())
        m_currentShader = index;
}

/*void Scene::setTexture(int index)
{
    if (index >= 0 && index < m_textures.size())
        m_currentTexture = index;
}//*/

void Scene::toggleDynamicCubemap(int state)
{
    if ((m_dynamicCubemap = (state == Qt::Checked)))
        m_updateAllCubemaps = true;
}

void Scene::setColorParameter(const QString &name, QRgb color)
{
    // set the color in all programs
    foreach (QGLShaderProgram *program, m_programs) {
        program->bind();
        program->setUniformValue(program->uniformLocation(name), QColor(color));
        program->release();
    }
}

void Scene::setFloatParameter(const QString &name, float value)
{
    // set the color in all programs
    foreach (QGLShaderProgram *program, m_programs) {
        program->bind();
        program->setUniformValue(program->uniformLocation(name), value);
        program->release();
    }
}

/*void Scene::newItem(ItemDialog::ItemType type)
{
    QSize size = sceneRect().size().toSize();
    switch (type) {
    case ItemDialog::QtBoxItem:
        addItem(new QtBox(64, rand() % (size.width() - 64) + 32, rand() % (size.height() - 64) + 32));
        break;
    case ItemDialog::CircleItem:
        addItem(new CircleItem(64, rand() % (size.width() - 64) + 32, rand() % (size.height() - 64) + 32));
        break;
    case ItemDialog::SquareItem:
        addItem(new SquareItem(64, rand() % (size.width() - 64) + 32, rand() % (size.height() - 64) + 32));
        break;
    default:
        break;
    }
}//*/
