#ifndef MYGRAPHICSSCENE_H
#define MYGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <QWidget>
#include "Line.h"
#include "Ellipse.h"
#include "Rectangle.h"
#include "Controller.h"
#include "Curve.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QThreadPool>

// 实现自己的QGraphicsScene类，以实现更多的效果
class MyGraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit MyGraphicsScene(QObject *parent = nullptr);
    // 设置当前绘制的数据（颜色、字号、类型）
    void setData(QColor c, int w, Controller::DrawType type);
    // 有新的绘制操作（用户收到别人的）
    void drawNewEvent(QJsonObject jsonObj);
    // 发送自己绘制的数据给主类
    void sendDrawEventSingal(QPointF pos);

    // 重写鼠标事件，实现绘制
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private slots:
    // 收到子线程完成的绘制对象，将其添加到控件中
    void onAddNewItem(QGraphicsItem* item);

signals:
    void needData();// 发送给主类的信号，获得最新的绘制属性
    void drawEvent(QJsonObject jsonObj);// 将自己的绘制信息副本发送给主类（主类再发给服务端）
    void addNewItem(QGraphicsItem* item);// 子线程在完成绘制对象的创建后，发送的信号
    void rubberItem(QJsonObject jsonObj);// 将自己的删除绘制的信息发送给主类（主类再发给服务端）

private:
    QColor m_color;
    int m_weight;
    Element *m_currentItem;// 当前正在绘制的图形指针
    QPointF m_startPoint;// 起始点
    Controller::DrawType m_drawType;//当前绘图的类型
    bool m_isDrawing;// 当前是否正在绘制
    QThreadPool* m_threadPool;// 保存全局线程池实例
    qint64 m_counter;// 项的计数器，唯一标识某一项（配合账号，实现定位某一条绘制）
};

#endif // MYGRAPHICSSCENE_H
