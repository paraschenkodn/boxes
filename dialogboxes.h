#ifndef DIALOGBOX1_H
#define DIALOGBOX1_H

/// Всё что не касается отображения объектов на сцене вынесено в этот отдельный модуль
/// в частности классы диалогбоксов

#include <QtWidgets>
#include <QtOpenGL>
#include "glextensions.h"

// класс не имеющий отношения к отображению объектов
class ParameterEdit : public QWidget
{
public:
    virtual void emitChange() = 0;
};

// класс не имеющий отношения к отображению объектов
class ColorEdit : public ParameterEdit
{
    Q_OBJECT
public:
    ColorEdit(QRgb initialColor, int id);
    QRgb color() const {return m_color;}
    virtual void emitChange() Q_DECL_OVERRIDE {emit colorChanged(m_color, m_id);}
public slots:
    void editDone();
signals:
    void colorChanged(QRgb color, int id);
protected:
    virtual void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void setColor(QRgb color); // also emits colorChanged()
private:
    QGraphicsScene *m_dialogParentScene;
    QLineEdit *m_lineEdit;
    QFrame *m_button;
    QRgb m_color;
    int m_id;
};


// класс не имеющий отношения к отображению объектов
class FloatEdit : public ParameterEdit
{
    Q_OBJECT
public:
    FloatEdit(float initialValue, int id);
    float value() const {return m_value;}
    virtual void emitChange() Q_DECL_OVERRIDE {emit valueChanged(m_value, m_id);}
public slots:
    void editDone();
signals:
    void valueChanged(float value, int id);
private:
    QGraphicsScene *m_dialogParentScene;
    QLineEdit *m_lineEdit;
    float m_value;
    int m_id;
};

//класс не имеющий отношение к отображению основных объектов сцены
// отрисовка панели управления (и QT кубиков???)
class GraphicsWidget : public QGraphicsProxyWidget
{
public:
    GraphicsWidget() : QGraphicsProxyWidget(0, Qt::Window) {}
protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QGraphicsSceneResizeEvent *event) Q_DECL_OVERRIDE;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;
};

// класс не имеющий отношение к отображению основных объектов
// отрисовка панели управления
class TwoSidedGraphicsWidget : public QObject
{
    Q_OBJECT
public:
    TwoSidedGraphicsWidget(QGraphicsScene *scene);
    void setWidget(int index, QWidget *widget);
    QWidget *widget(int index);
public slots:
    void flip();
protected slots:
    void animateFlip();
private:
    GraphicsWidget *m_proxyWidgets[2];
    int m_current;
    int m_angle; // angle in degrees
    int m_delta;
};

// класс не имеющий отношение к отображению основных объектов
// отрисовка панели управления
class RenderOptionsDialog : public QDialog
{
    Q_OBJECT
public:
    RenderOptionsDialog();
    int addTexture(const QString &name);
    int addShader(const QString &name);
    void emitParameterChanged();
protected slots:
    void setColorParameter(QRgb color, int id);
    void setFloatParameter(float value, int id);
signals:
    void dynamicCubemapToggled(int);
    void colorParameterChanged(const QString &, QRgb);
    void floatParameterChanged(const QString &, float);
    void textureChanged(int);
    void shaderChanged(int);
    void doubleClicked();
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    QVector<QByteArray> m_parameterNames;
    QComboBox *m_textureCombo;
    QComboBox *m_shaderCombo;
    QVector<ParameterEdit *> m_parameterEdits;
};

// класс не имеющий отношение к отображению основных объектов
// отрисовка панели управления (вторая сторона)
class ItemDialog : public QDialog
{
    Q_OBJECT
public:
    enum ItemType {
        QtBoxItem,
        CircleItem,
        SquareItem,
    };

    ItemDialog();
public slots:
    void triggerNewQtBox();
    void triggerNewCircleItem();
    void triggerNewSquareItem();
signals:
    void doubleClicked();
    void newItemTriggered(ItemDialog::ItemType type);
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
};


#endif // DIALOGBOX1_H
