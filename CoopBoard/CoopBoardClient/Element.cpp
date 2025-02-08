#include "Element.h"

Element::Element(QColor c, int w, QPointF p1, QPointF p2)
    :m_number(-1)
    ,m_account("-1")
    ,m_color(c)
    ,m_weight(w)
    ,m_pointStart(p1)
    ,m_pointEnd(p2) {}

void Element::setNumber(qint64 number)
{
    m_number = number;
}

qint64 Element::getNumber()
{
    return m_number;
}

void Element::setAccount(QString account)
{
    m_account = account;
}

QString Element::getAccount()
{
    return m_account;
}
