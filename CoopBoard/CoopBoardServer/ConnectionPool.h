#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <QMutex>
#include <QQueue>
#include <QDebug>
#include <QSqlDatabase>

#define MAX_CONNECTIONS 300//宏定义最大连接数

class ConnectionPool
{
public:
    ConnectionPool();
    ~ConnectionPool();
    //获取一个数据库连接池连接实例
    QSqlDatabase getConnection();
    //释放一个数据库连接池连接实例
    void releaseConnection(QSqlDatabase& db);

private:
    QMutex m_mutex;//互斥锁
    int m_connectionCount = 0;//递增，用于命名（唯一命名数据库连接）
    QQueue<QSqlDatabase> m_connectionPool;//保存空闲数据库连接的队列
};

#endif // CONNECTIONPOOL_H
