#include "Line.h"

Line::Line(QColor c, int w, QPointF p1, QPointF p2) :Element(c,w,p1,p2){}

QRectF Line::boundingRect() const
{
    //返回图形项的外界矩形，易于管理
    return QRectF(m_pointStart,m_pointEnd).normalized().adjusted(-m_weight,-m_weight,m_weight,m_weight);
}

void Line::setEndPoint(QPointF p2)
{
    m_pointEnd = p2;
}

void Line::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    // 使用画笔设置颜色和宽度
    QPen pen;
    pen.setColor(m_color);
    pen.setWidth(m_weight);
    painter->setPen(pen);

    // 绘制直线
    painter->drawLine(m_pointStart, m_pointEnd);
}

