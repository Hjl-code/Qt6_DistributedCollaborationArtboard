#include "Controller.h"
#include <QApplication>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //窗口1
    Controller w;
    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    // 获取主窗口的大小
    int width = w.width();
    int height = w.height();
    // 计算主窗口的位置，使其居中
    int x = (screenGeometry.width() - width) / 2;
    int y = (screenGeometry.height() - height) / 2;
    w.move(x,y);
    w.show();

    // //第二个窗口（用于测试）
    // Controller w2;
    // w2.move(x+100,y);
    // w2.show();

    return a.exec();
}
