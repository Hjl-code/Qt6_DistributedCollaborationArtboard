#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QWebSocket>
#include "Server.h"
#include "ConnectionPool.h"
#include <QDebug>

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QWebSocket* socket,const QString& account,QObject *parent = nullptr);
    //封装打包发送json数据包的逻辑
    Q_INVOKABLE void sendJsonObject(QJsonObject& jsonObj);
    //向数据库存入单条绘制数据的逻辑
    void addDbDrawData(QJsonObject& jsonObj);
    //用户的协作用户发生变化的逻辑（针对信号槽的连接与断开）
    void teammateChange(bool flag,ClientHandler* c, bool isRecevied);
    //斩断用户的所有协作用户的逻辑（断开所有协作的信号槽）
    void teamateAllCut();
    //用户刚加入协作时，发送画板原有绘制数据的逻辑
    void sendOldDataWhenJoin();
    //删除数据库中某一条绘制数据的逻辑（被用户擦掉了）
    void deleteDrawItem(QJsonObject& jsonObj);


    //收到来自客户端的消息请求处理逻辑集合
public:
    void dealCreatorBoard();//处理创建协作画板逻辑
    void dealExitBoard();//处理用户离开协作画板逻辑
    void dealJoinBoard(int id);//处理用户加入协作画板逻辑
    void dealDrawEvent(QJsonObject& jsonObj);//处理收到客户端的单条绘制数据逻辑
    void dealRubberItem(QJsonObject& jsonObj);//处理收到客户端擦除单条绘制数据逻辑

    //槽函数的处理逻辑集合
public slots:
    void onDisconnected();//连接被客户端断开服务端连接信号的槽函数
    void onTextMessageReceived(const QString &message);//连接服务端收到客户端消息信号的槽函数
    void onOutOfBoard();//连接需要被踢出协作信号的槽函数
    void onDrawEventBoard(QJsonObject jsonObj);//连接收到别人单条绘制数据信号的槽函数
    void onRubberItem(QJsonObject jsonObj);//连接收到别人删除单条绘制数据信号的槽函数

    //信号的逻辑集合
signals:
    void outOfBoard();//通知别人被踢出画板的信号（由画板主人发出）
    void drawEventBoard(QJsonObject jsonObj);//通知别人我有一条绘制数据的信号
    void childThreadSend(QString jsonStr);//子线程处理好后通知主线程的信号
    void rubberItem(QJsonObject jsonObj);//通知别人我删除了一条绘制数据的信号
    void newTeammate(bool flag,ClientHandler* c, bool isRecevied);//通知别人我是新的队友的信号（让对方也加上自己）
    void cutTeamate(bool flag,ClientHandler* c, bool isRecevied);//通知别人我离开协作的信号（让对方和自己断开）

private:
    Server* m_server;//保存Server类对象（用于对哈希表进行操作、获取数据库连接等操作）
    QWebSocket* m_socket;//保存自身的套接字，用于和客户端进行通信
    QString m_account;//保存自身的账号信息
    int m_id;//保存自己当前加入的画板的id信息
    QSqlDatabase m_db;//保存自己的数据库连接对象
    bool m_isOwner;//保存当前加入的画板，自己是否是主人
    //保存队友的指针，便于信号等的连接（最多3个）（使用智能指针，对象被销毁时自动置空）
    QVector<QPointer<ClientHandler>> m_teammate;
};

#endif // CLIENTHANDLER_H
