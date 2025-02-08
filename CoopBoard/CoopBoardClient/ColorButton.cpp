#include "ColorButton.h"

#include <QColorDialog>
#include <QPainter>

ColorButton::ColorButton(QWidget *parent): QPushButton(parent), m_color(Qt::black)
{
    // 设置按钮的大小，连接信号和槽
    this->setFixedSize(25,25);
    connect(this, &QPushButton::clicked, this, &ColorButton::onButtonClicked);
}

void ColorButton::onButtonClicked()
{
    // 弹出颜色对话框
    QColor color = QColorDialog::getColor(m_color, this, "选择颜色");
    if (color.isValid()) {
        m_color = color;  // 更新按钮颜色
        update();  // 重新绘制按钮
    }
}

QColor ColorButton::getColor()
{
    return m_color;
}

void ColorButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // 绘制按钮本身为对应的颜色
    QPainter painter(this);
    painter.setBrush(QBrush(m_color));
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, width(), height());
}
