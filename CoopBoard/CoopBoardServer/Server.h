#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QDebug>
#include <QReadWriteLock>
#include <QWebSocketServer>
#include <QtWebSockets>
#include <QThreadPool>
#include <QSqlDatabase>
#include "ConnectionPool.h"

class ClientHandler;
class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();
    //开启服务器监听，端口8090
    void webSocketListen();
    //向哈希表填充用户
    void addClientToHash(const QString& account, ClientHandler* handler);
    //从哈希表去除用户
    void removeClientToHash(const QString& account);
    //从连接池获取一个数据库连接
    QSqlDatabase getDBConnection();
    //从连接池释放一个数据库连接
    void releaseDBConnection(QSqlDatabase& db);
    //从哈希表取得特定的用户指针
    ClientHandler* getClientHandlerByAccount(QString account);

public slots:
    //有新的客户端连接
    void onNewConnection();


public:
    QThreadPool* m_threadPool;//保存全局线程池对象

private:
    QReadWriteLock m_lock;  // 读写锁
    QWebSocketServer* m_webSocket;//webSocket服务器对象
    ConnectionPool* m_connectionPool;//存储连接池对象
    QHash<QString, ClientHandler*> m_clientMap;//建立账号映射，存储所有的客户端连接指针
    QSqlDatabase m_db;//保存Server类自身与数据库的连接
};

#endif // SERVER_H
