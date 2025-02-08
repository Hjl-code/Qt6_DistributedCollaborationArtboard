#ifndef ELEMENT_H
#define ELEMENT_H

#include <QGraphicsItem>

class Element : public QGraphicsItem
{
public:
    Element(QColor c, int w, QPointF p1, QPointF p2);
    void setNumber(qint64 number);
    qint64 getNumber();
    void setAccount(QString account);
    QString getAccount();

    //protected属性使得子类也可以直接使用成员变量
protected:
    qint64 m_number;// 标识号
    QString m_account;// 账号信息（如果是自己的则是默认）
    QColor m_color;// 颜色
    int m_weight;// 字号
    QPointF m_pointStart;// 起始点
    QPointF m_pointEnd;// 终止点
};

#endif // ELEMENT_H
