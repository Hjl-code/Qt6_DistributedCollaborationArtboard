#include "Curve.h"

Curve::Curve(QColor c, int w, QPointF p1, QPointF p2) :Element(c,w,p1,p2){
    m_path.moveTo(p1);
    m_pen.setColor(c);
    m_pen.setWidth(w);
}

QRectF Curve::boundingRect() const
{
    qreal penWidth = m_pen.widthF();
    return m_path.boundingRect().adjusted(-penWidth,-penWidth,penWidth,penWidth);
}

QPainterPath Curve::shape() const
{
    QPainterPathStroker stroker;
    stroker.setWidth(m_pen.widthF());
    stroker.setCapStyle(m_pen.capStyle());
    return stroker.createStroke(m_path); // 生成包含线条宽度的形状
}

void Curve::addPathPoint(QPointF point)
{
    m_path.lineTo(point);
}

void Curve::setPathByArray(QJsonArray pathArray)
{
    if(pathArray.isEmpty())
        return;
    for (const QJsonValue &value : pathArray) {
        QJsonObject pointObj = value.toObject();
        double x = pointObj["x"].toDouble();
        double y = pointObj["y"].toDouble();
        QPointF point(x,y);
        m_path.lineTo(point);
    }
}

void Curve::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(m_pen);
    painter->drawPath(m_path);
}

QJsonArray Curve::getCurvePoints()
{
    QJsonArray pointsArray;
    // 遍历路径中的所有元素
    for (int i = 0; i < m_path.elementCount(); ++i) {
        const QPainterPath::Element &element = m_path.elementAt(i);
        QJsonObject pointObj;
        pointObj["x"] = element.x;
        pointObj["y"] = element.y;
        pointsArray.append(pointObj);
    }
    return pointsArray;
}
