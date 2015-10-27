#include "oglw.h"

OGLW::OGLW(QWidget *parent) : QOpenGLWidget(parent)
{
// здесь мы не инициализируем OpenGL, это делается только в OGLW::initializeGL(), иначе будет ошибка
}

// Run once when widget is set up
void OGLW::initializeGL()
{
    /*f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_0>();
    if (!f) {
      qWarning("Could not obtain OpenGL versions object");
      exit(1);
    }
    f->initializeOpenGLFunctions();//*/

  initializeOpenGLFunctions();

  qDebug() << QString((const char*)glGetString(GL_VERSION)) << "\n" << QString((const char*)glGetString(GL_VENDOR))<< "\n" << QString((const char*)glGetString(GL_RENDERER));//<< "\n" << glGetString(GL_EXTENTIONS);

  // очищаем поле
  glClearColor(0.1f,0.1f,0.2f,1.0f); // тёмно-синенький

  glEnable(GL_DEPTH_TEST);
  // glEnable(GL_CULL_FACE);
}

void OGLW::paintGL()
{

}

void OGLW::resizeGL(int width, int height)
{
      glViewport(0, 0, width, height);
}
