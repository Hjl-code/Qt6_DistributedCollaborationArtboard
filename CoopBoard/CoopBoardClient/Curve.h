#ifndef CURVE_H
#define CURVE_H

#include "Element.h"
#include <QPen>
#include <QPainter>
#include <QJsonArray>
#include <QJsonObject>

class Curve : public Element
{
public:
    Curve(QColor c, int w, QPointF p1, QPointF p2);
    // 向路径添加点的函数
    void addPathPoint(QPointF point);
    // 直接将一个json数组设置为路径的函数
    void setPathByArray(QJsonArray pathArray);
    // 以json数组的形式返回自身路径的函数
    QJsonArray getCurvePoints();

    // 重写虚函数
    QRectF boundingRect() const override;// 图元的边界矩形函数，定义外接矩形，用于视图更新
    QPainterPath shape() const override;// shape方法，定义真实形状，用于碰撞检测
    // 绘制图元方法
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;


private:
    QPainterPath m_path;
    QPen m_pen;
};

#endif // CURVE_H
