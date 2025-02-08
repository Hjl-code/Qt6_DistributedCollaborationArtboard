#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QPushButton>

// 实现点击选取颜色的按钮
class ColorButton : public QPushButton
{
    Q_OBJECT
public:
    ColorButton(QWidget *parent = nullptr);
    // 按钮被点击时的槽函数（实现颜色选取逻辑）
    void onButtonClicked();
    // 返回自身当前颜色的函数
    QColor getColor();

protected:
    // 重写绘制事件，将按钮本身变为对应的颜色
    void paintEvent(QPaintEvent* event);

private:
    QColor m_color;// 保存按钮当前选择的颜色
};

#endif // COLORBUTTON_H
