#include <QCoreApplication>
#include <QDebug>
#include "Server.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 确保 QJsonObject 作为参数正确传递
    qRegisterMetaType<QJsonObject>("QJsonObject&");

    Server s;
    return a.exec();
}
