#include "Rectangle.h"

#include <QPainter>
#include <QPen>

Rectangle::Rectangle(QColor c, int w, QPointF p1, QPointF p2) :Element(c,w,p1,p2) {}

QRectF Rectangle::boundingRect() const
{
    return QRectF(m_pointStart,m_pointEnd).normalized().adjusted(-m_weight,-m_weight,m_weight,m_weight);
}

void Rectangle::setEndPoint(QPointF p2)
{
    m_pointEnd = p2;
}

void Rectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    QPen pen;
    pen.setColor(m_color);
    pen.setWidth(m_weight);
    painter->setPen(pen);
    painter->drawRect(QRectF(m_pointStart,m_pointEnd));
}
