#ifndef LINE_H
#define LINE_H

#include "Element.h"
#include <QPen>
#include <QPainter>

class Line : public Element
{
public:
    Line(QColor c, int w, QPointF p1, QPointF p2);
    // 设置结束点，鼠标移动时进行改变，以形成对动态效果
    void setEndPoint(QPointF p2);

    // 重写虚函数
    QRectF boundingRect() const override;// 图元的边界矩形函数，定义外接矩形，用于视图更新
    // 绘制图元方法
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // LINE_H
