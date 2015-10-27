#ifndef OGLW_H
#define OGLW_H
#include <QDebug>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_0>

// объявляем класс производный от QOpenGLWidget и показываем что будем использовать функции OpenGL версии 3.0
class OGLW : public QOpenGLWidget , protected QOpenGLFunctions_3_0
{
    Q_OBJECT    //будем использовать сигналы

public:
  explicit OGLW(QWidget *parent = 0);  // explicit отключает неявные преобразования аргументов при инициализации класса
  //~OGLW();

signals:

protected:
  void initializeGL() Q_DECL_OVERRIDE;
  void paintGL() Q_DECL_OVERRIDE;
  void resizeGL(int width, int height) Q_DECL_OVERRIDE;

private:

};

#endif // OGLW_H
