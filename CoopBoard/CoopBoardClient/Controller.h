#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QMainWindow>
#include <QWebSocket>
#include <QDebug>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QGraphicsScene>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQueue>

QT_BEGIN_NAMESPACE
namespace Ui {
class Controller;
}
QT_END_NAMESPACE

class MyGraphicsScene;// 前向声明
class Controller : public QMainWindow
{
    Q_OBJECT

public:
    // 绘制类型枚举
    enum DrawType{
        LineType,// 直线
        CurveType,// 曲线
        EllipseType,// 椭圆
        RectangleType,// 矩形
        RubberType// 橡皮
    };

    Controller(QWidget *parent = nullptr);
    ~Controller();
    // 初始化UI相关的函数
    void initUi();
    // 客户端与服务端建立连接函数
    void initConnection();
    // 控制按钮启用与禁用的函数
    void setDrawToolEnable(bool flag);
    // 控制绘制类型按钮的单一选中逻辑函数
    void changeSelect(DrawType type);
    // 封装打包发送json数据包的逻辑函数
    void sendJsonObject(QJsonObject& jsonObj);
    // 窗口的关闭事件
    void closeEvent(QCloseEvent* event);

private:
    // 向服务端发送信息函数集合
    void sendCreatorBoardMsg();// 发送创建画板消息
    void sendExitBoardMsg();// 发送离开画板的消息
    void sendJoinBoardMsg();// 发送根据邀请码加入画板的消息

public:
    // 收到来自服务端的消息请求处理函数集合
    void dealCreatorBoardMsg(const QJsonObject& jsonObj);// 收到创建画板成功与否的消息
    void dealExitBoardMsg(const QJsonObject& jsonObj);// 收到离开协作成功与否的消息
    void dealOutOfBoard();// 收到被踢出协作的消息
    void dealJoinBoard(const QJsonObject& jsonObj);// 收到加入协作成功与否的消息
    void dealDrawEvent(QJsonObject jsonObj);// 收到来自其他客户端的绘制事件
    void dealOldDataWhenJoin(QJsonObject jsonObj);// 收到刚加入新画板时服务端发来的已有绘画数据
    void dealRubberItem(QJsonObject jsonObj);// 收到需要删除某一项的消息

public slots:
    void onConnected();// 与服务端的连接成功信号的槽函数
    void onDisconnected();// 与服务端断开连接信号的槽函数
    void onTextMessageReceived(const QString &message);// 收到服务端消息的槽函数
    void onDrawEvent(QJsonObject jsonObj);// 向服务端发送自己的绘制数据的槽函数
    void onRubberItem(QJsonObject jsonObj);// 向服务端发送自己删除了某一项的数据的槽函数


private slots:
    void on_weightSlide_valueChanged(int value);// 改变了字号大小
    void on_lineBtn_clicked();// 点击了直线按钮
    void on_curveBtn_clicked();// 点击了曲线按钮
    void on_ellipseBtn_clicked();// 点击了椭圆按钮
    void on_rectangleBtn_clicked();// 点击了矩形按钮
    void on_rubberBtn_clicked();// 点击了橡皮擦按钮
    void on_createBtn_clicked();// 点击了创建画板按钮
    void on_disconnectBtn_clicked();// 点击了断开连接按钮
    void on_connectBtn_clicked();// 点击了建立连接按钮

private:
    Ui::Controller *ui;
    QWebSocket* m_socket;// 保存自身的套接字
    MyGraphicsScene* m_scene;// 保存自定义的scene对象（用来设置给QGraphicsView控件）
    DrawType m_drawType;// 保存当前绘图的类型
    QMap<int, QJsonArray> m_receivedChunks;// 保存刚建立连接时，收到的初始化绘制信息块
};
#endif // CONTROLLER_H
