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
    , m_box(0)
    , m_vertexShader(0)
    , m_environmentShader(0)
    , m_environmentProgram(0)
{
    setSceneRect(0, 0, width, height);  // устанавливаем прямоугольник отсечения сцены

    m_trackBalls[0] = TrackBall(0.05f, QVector3D(0, 1, 0), TrackBall::Sphere);  // создаём орбиту (вокруг оси Y) для центрального куба (правильного гексаэдра) (угловая скорость, ось, модель вращения)
    m_trackBalls[1] = TrackBall(0.005f, QVector3D(0, 0, 1), TrackBall::Sphere); // создаём орбиту для кольца гексаэдров (вокруг оси Z)
    m_trackBalls[2] = TrackBall(0.0f, QVector3D(0, 1, 0), TrackBall::Plane);    // создаём орбиту для камеры ???

    m_renderOptions = new RenderOptionsDialog;              // создаём панель управления №1
    m_renderOptions->move(20, 120);                         // перемещаем её в угол
    m_renderOptions->resize(m_renderOptions->sizeHint());   // устанавливаем размер по рекомендованному

    // с диалоговыми панелями сцена OpenGL общается через систему сигналов
    connect(m_renderOptions, SIGNAL(dynamicCubemapToggled(int)), this, SLOT(toggleDynamicCubemap(int)));                    //
    connect(m_renderOptions, SIGNAL(colorParameterChanged(QString,QRgb)), this, SLOT(setColorParameter(QString,QRgb)));
    connect(m_renderOptions, SIGNAL(floatParameterChanged(QString,float)), this, SLOT(setFloatParameter(QString,float)));
    connect(m_renderOptions, SIGNAL(textureChanged(int)), this, SLOT(setTexture(int)));
    connect(m_renderOptions, SIGNAL(shaderChanged(int)), this, SLOT(setShader(int)));

    // создаём панель управления №2, которая также общается посредством сигналов
    m_itemDialog = new ItemDialog;
    connect(m_itemDialog, SIGNAL(newItemTriggered(ItemDialog::ItemType)), this, SLOT(newItem(ItemDialog::ItemType)));

    // формируем двухсторонний виджет панели управления
    TwoSidedGraphicsWidget *twoSided = new TwoSidedGraphicsWidget(this);
    twoSided->setWidget(0, m_renderOptions);
    twoSided->setWidget(1, m_itemDialog);
    // связываем его сигналами
    connect(m_renderOptions, SIGNAL(doubleClicked()), twoSided, SLOT(flip()));
    connect(m_itemDialog, SIGNAL(doubleClicked()), twoSided, SLOT(flip()));

    // добавляем на сцену кубики QT
    addItem(new QtBox(64, width - 64, height - 64));
    addItem(new QtBox(64, width - 64, 64));
    addItem(new QtBox(64, 64, height - 64));
    addItem(new QtBox(64, 64, 64));

    initGL();   // инициализируем OpenGL

    // запускаем таймер анимации и привязываем его к обновлению сцены
    m_timer = new QTimer(this);
    m_timer->setInterval(20);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
    m_timer->start();

    ///m_time.start();  /// закоментируем лишнюю неиспользуемую переменную
}

Scene::~Scene()
{
    if (m_box)
        delete m_box;
    foreach (GLTexture *texture, m_textures)
        if (texture) delete texture;
    if (m_mainCubemap)
        delete m_mainCubemap;
    foreach (QGLShaderProgram *program, m_programs)
        if (program) delete program;
    if (m_vertexShader)
        delete m_vertexShader;
    foreach (QGLShader *shader, m_fragmentShaders)
        if (shader) delete shader;
    foreach (GLRenderTargetCube *rt, m_cubemaps)
        if (rt) delete rt;
    if (m_environmentShader)
        delete m_environmentShader;
    if (m_environmentProgram)
        delete m_environmentProgram;
}

void Scene::initGL()
{
    m_box = new GLRoundedBox(0.25f, 1.0f, 10);                                              // рисуем кексаэдры

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
    m_environment = new GLTextureCube(list, qMin(1024, m_maxTextureSize));                  // создаём куб фона
    m_environmentShader = new QGLShader(QGLShader::Fragment);                               //
    m_environmentShader->compileSourceCode(environmentShaderText);
    m_environmentProgram = new QGLShaderProgram;
    m_environmentProgram->addShader(m_vertexShader);        //  добавляем программу
    m_environmentProgram->addShader(m_environmentShader);   //  к ней ещё одну (в GPU программа одна, это у нас она разбита)
    m_environmentProgram->link();

    // формируем текстурную маску из шума
    const int NOISE_SIZE = 128; // for a different size, B and BM in fbm.c must also be changed
    m_noise = new GLTexture3D(NOISE_SIZE, NOISE_SIZE, NOISE_SIZE);
    QRgb *data = new QRgb[NOISE_SIZE * NOISE_SIZE * NOISE_SIZE];
    memset(data, 0, NOISE_SIZE * NOISE_SIZE * NOISE_SIZE * sizeof(QRgb));
    QRgb *p = data;
    float pos[3];
    for (int k = 0; k < NOISE_SIZE; ++k) {
        pos[2] = k * (0x20 / (float)NOISE_SIZE);
        for (int j = 0; j < NOISE_SIZE; ++j) {
            for (int i = 0; i < NOISE_SIZE; ++i) {
                for (int byte = 0; byte < 4; ++byte) {
                    pos[0] = (i + (byte & 1) * 16) * (0x20 / (float)NOISE_SIZE);
                    pos[1] = (j + (byte & 2) * 8) * (0x20 / (float)NOISE_SIZE);
                    *p |= (int)(128.0f * (noise3(pos) + 1.0f)) << (byte * 8);
                }
                ++p;
            }
        }
    }
    m_noise->load(NOISE_SIZE, NOISE_SIZE, NOISE_SIZE, data);
    delete[] data;

    m_mainCubemap = new GLRenderTargetCube(512);        //

    QStringList filter;                                                                     // фильтр выбора файлов
    QList<QFileInfo> files;                                                                 // список файлов

    // Load all .png files as textures                                                      // загружаем все png файлы как текстуры  (куда грузим??)
    m_currentTexture = 0;                                                                   // индекс текущей текстуры
    filter = QStringList("*.png");
    files = QDir(":/res/boxes/").entryInfoList(filter, QDir::Files | QDir::Readable);       // наполняем список в соответствии с фильтром из файлов зарегистрированных как ресурс

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

        program->bind();                                            // связываем программу (с чем???)
        m_cubemaps << ((program->uniformLocation("env") != -1)                      // если в шейдерной программе есть переменная "env" то в массив (??? cubemaps)
                       ? new GLRenderTargetCube(qMin(256, m_maxTextureSize)) : 0);  // пихаем новый объект (??? карты текстур) либо 0
        program->release();                                                                     // удаляем уже ненужный экземпляр программы
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

/// Рисуем все кубики разом
// If one of the boxes should not be rendered, set excludeBox to its index.
// If the main box should not be rendered, set excludeBox to -1.
void Scene::renderBoxes(const QMatrix4x4 &view, int excludeBox)
{
    QMatrix4x4 invView = view.inverted();           //
    //excludeBox=2;

    // If multi-texturing is supported, use three saplers.
    //if (glActiveTexture) {                  // старьё выкидываем
        glActiveTexture(GL_TEXTURE0);
        m_textures[m_currentTexture]->bind();
        glActiveTexture(GL_TEXTURE2);
        m_noise->bind();
        glActiveTexture(GL_TEXTURE1);
    /*} else {
        m_textures[m_currentTexture]->bind();
    }*/

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);        // без этого не отрисовывается фон, почему ???

    QMatrix4x4 viewRotation(view);                                          // создаём матрицу viewRotation из матрицы view
    viewRotation(3, 0) = viewRotation(3, 1) = viewRotation(3, 2) = 0.0f;    // инициализируем матрицу поворота
    viewRotation(0, 3) = viewRotation(1, 3) = viewRotation(2, 3) = 0.0f;    //
    viewRotation(3, 3) = 1.0f;
    loadMatrix(viewRotation);                                               // грузим сформированную матрицу glLoadMatrixf(mat);
    glScalef(20.0f, 20.0f, 20.0f);                  // растягиваем куб (сцены???) если взять 10, фигуры тонут, если взять 50, пропадает фон

    // Don't render the environment if the environment texture can't be set for the correct sampler.
    // if (glActiveTexture) {  // старьё выкидываем
        m_environment->bind();
        m_environmentProgram->bind();
        m_environmentProgram->setUniformValue("tex", GLint(0));
        m_environmentProgram->setUniformValue("env", GLint(1));
        m_environmentProgram->setUniformValue("noise", GLint(2));
        m_box->draw();
        m_environmentProgram->release();
        m_environment->unbind();
    //}

    loadMatrix(view);

    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);

    for (int i = 0; i < m_programs.size(); ++i) {
        if (i == excludeBox)
            continue;

        glPushMatrix();
        QMatrix4x4 m;
        m.rotate(m_trackBalls[1].rotation());
        glMultMatrixf(m.constData());

        glRotatef(360.0f * i / m_programs.size(), 0.0f, 0.0f, 1.0f);
        glTranslatef(2.0f, 0.0f, 0.0f);
        glScalef(0.3f, 0.6f, 0.6f);

        // if (glActiveTexture) {  // старьё выкидываем
            if (m_dynamicCubemap && m_cubemaps[i])
                m_cubemaps[i]->bind();
            else
                m_environment->bind();
        //}
        m_programs[i]->bind();
        m_programs[i]->setUniformValue("tex", GLint(0));
        m_programs[i]->setUniformValue("env", GLint(1));
        m_programs[i]->setUniformValue("noise", GLint(2));
        m_programs[i]->setUniformValue("view", view);
        m_programs[i]->setUniformValue("invView", invView);
        m_box->draw();
        m_programs[i]->release();

        // if (glActiveTexture) {  // старьё выкидываем
            if (m_dynamicCubemap && m_cubemaps[i])
                m_cubemaps[i]->unbind();
            else
                m_environment->unbind();
        //}
        glPopMatrix();
    }

    if (-1 != excludeBox) {
        QMatrix4x4 m;
        m.rotate(m_trackBalls[0].rotation());
        glMultMatrixf(m.constData());

        // if (glActiveTexture) {  // старьё выкидываем
            if (m_dynamicCubemap)
                m_mainCubemap->bind();
            else
                m_environment->bind();
        //}

        m_programs[m_currentShader]->bind();
        m_programs[m_currentShader]->setUniformValue("tex", GLint(0));
        m_programs[m_currentShader]->setUniformValue("env", GLint(1));
        m_programs[m_currentShader]->setUniformValue("noise", GLint(2));
        m_programs[m_currentShader]->setUniformValue("view", view);
        m_programs[m_currentShader]->setUniformValue("invView", invView);
        m_box->draw();
        m_programs[m_currentShader]->release();

        // if (glActiveTexture) {  // старьё выкидываем
            if (m_dynamicCubemap)
                m_mainCubemap->unbind();
            else
                m_environment->unbind();
        //}
    }

    // if (glActiveTexture) {  // старьё выкидываем
        glActiveTexture(GL_TEXTURE2);
        m_noise->unbind();
        glActiveTexture(GL_TEXTURE0);
    //}
    m_textures[m_currentTexture]->unbind();
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

void Scene::renderCubemaps()
{
    // To speed things up, only update the cubemaps for the small cubes every N frames.
    const int N = (m_updateAllCubemaps ? 1 : 3);

    QMatrix4x4 mat;
    GLRenderTargetCube::getProjectionMatrix(mat, 0.1f, 100.0f);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    loadMatrix(mat);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    QVector3D center;

    for (int i = m_frame % N; i < m_cubemaps.size(); i += N) {
        if (0 == m_cubemaps[i])
            continue;

        float angle = 2.0f * PI * i / m_cubemaps.size();

        center = m_trackBalls[1].rotation().rotatedVector(QVector3D(std::cos(angle), std::sin(angle), 0.0f));

        for (int face = 0; face < 6; ++face) {
            m_cubemaps[i]->begin(face);

            GLRenderTargetCube::getViewMatrix(mat, face);
            QVector4D v = QVector4D(-center.x(), -center.y(), -center.z(), 1.0);
            mat.setColumn(3, mat * v);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderBoxes(mat, i);

            m_cubemaps[i]->end();
        }
    }

    for (int face = 0; face < 6; ++face) {
        m_mainCubemap->begin(face);
        GLRenderTargetCube::getViewMatrix(mat, face);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderBoxes(mat, -1);

        m_mainCubemap->end();
    }

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    m_updateAllCubemaps = false;
}

void Scene::drawBackground(QPainter *painter, const QRectF &)
{
    float width = float(painter->device()->width());
    float height = float(painter->device()->height());

    painter->beginNativePainting();
    setStates();

    if (m_dynamicCubemap)
        renderCubemaps();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    qgluPerspective(60.0, width / height, 0.01, 15.0);

    glMatrixMode(GL_MODELVIEW);

    QMatrix4x4 view;
    view.rotate(m_trackBalls[2].rotation());
    view(2, 3) -= 2.0f * std::exp(m_distExp / 1200.0f);
    renderBoxes(view);

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
        m_trackBalls[0].move(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    } else {
        m_trackBalls[0].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
    }

    if (event->buttons() & Qt::RightButton) {
        m_trackBalls[1].move(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    } else {
        m_trackBalls[1].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
    }

    if (event->buttons() & Qt::MidButton) {
        m_trackBalls[2].move(pixelPosToViewPos(event->scenePos()), QQuaternion());
        event->accept();
    } else {
        m_trackBalls[2].release(pixelPosToViewPos(event->scenePos()), QQuaternion());
    }
}

void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;

    if (event->buttons() & Qt::LeftButton) {
        m_trackBalls[0].push(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();  // если убрать ничего не меняется, движение не компенсируется
        qDebug() << "X=" << pixelPosToViewPos(event->scenePos()).x() << " Y=" << pixelPosToViewPos(event->scenePos()).y() << " x=" << event->scenePos().x() << " y=" << event->scenePos().y();
    }

    if (event->buttons() & Qt::RightButton) {
        m_trackBalls[1].push(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    }

    if (event->buttons() & Qt::MidButton) {
        m_trackBalls[2].push(pixelPosToViewPos(event->scenePos()), QQuaternion());
        event->accept();
    }
}

void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    if (event->isAccepted())
        return;

    if (event->button() == Qt::LeftButton) {
        m_trackBalls[0].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    }

    if (event->button() == Qt::RightButton) {
        m_trackBalls[1].release(pixelPosToViewPos(event->scenePos()), m_trackBalls[2].rotation().conjugate());
        event->accept();
    }

    if (event->button() == Qt::MidButton) {
        m_trackBalls[2].release(pixelPosToViewPos(event->scenePos()), QQuaternion());
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

    m_trackBalls[0].push(QPointF(0,0), m_trackBalls[2].rotation().conjugate());  // воспользуемся уже имеющейся функцией запоминания текущей позиции сферы вращения
    m_trackBalls[0].k_pressed=true;                                              // указываем что работаем клавиатурой (по этому флагу задаётся время расчёта угловой скорости)

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
  m_trackBalls[0].release(point, m_trackBalls[2].rotation().conjugate());       // вызываем функцию расчёта угловой скорости и расчёта вектора оси вращения
  event->accept();                                                          //блокируем передачу сообщения конкретно дальше по сцене
  m_trackBalls[0].k_pressed=false;                                              // сбрасываем флаг работы с клавиатурой
}//*/
/// *** это моя вставка end

void Scene::setShader(int index)
{
    if (index >= 0 && index < m_fragmentShaders.size())
        m_currentShader = index;
}

void Scene::setTexture(int index)
{
    if (index >= 0 && index < m_textures.size())
        m_currentTexture = index;
}

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

void Scene::newItem(ItemDialog::ItemType type)
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
}
