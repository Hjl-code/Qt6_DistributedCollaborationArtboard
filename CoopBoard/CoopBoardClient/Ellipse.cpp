#include "Ellipse.h"

#include <QPen>
#include <QPainter>

Ellipse::Ellipse(QColor c, int w, QPointF p1, QPointF p2) :Element(c,w,p1,p2) {}

QRectF Ellipse::boundingRect() const
{
    //返回椭圆的区域
    return QRectF(m_pointStart,m_pointEnd).normalized().adjusted(-m_weight,-m_weight,m_weight,m_weight);
}

void Ellipse::setEndPoint(QPointF p2)
{
    m_pointEnd = p2;
}

void Ellipse::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    QPen pen;
    pen.setColor(m_color);
    pen.setWidth(m_weight);
    painter->setPen(pen);
    painter->drawEllipse(QRectF(m_pointStart,m_pointEnd));
}
