#include "ConnectionPool.h"
#include <QSqlError>

ConnectionPool::ConnectionPool():m_connectionCount(0) {}

ConnectionPool::~ConnectionPool()
{
    // 析构前，释放所有的数据库连接
    while (!m_connectionPool.isEmpty()) {
        QSqlDatabase db = m_connectionPool.dequeue();
        db.close();// 调用close方法关闭并销毁连接
    }
}

QSqlDatabase ConnectionPool::getConnection()
{
    // 由于多个用户指针共用连接池，所以使用互斥锁来保护对连接池的修改操作
    QMutexLocker locker(&m_mutex);//确保线程安全

    // 优先使用空闲连接
    if (!m_connectionPool.isEmpty()) {
        return m_connectionPool.dequeue();
    }

    // 连接池未满，创建新连接
    if(m_connectionCount < MAX_CONNECTIONS){
        QString connectionName = QString("connection_%1").arg(++m_connectionCount);
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL",connectionName);
        db.setHostName("127.0.0.1");// 如果数据库不是部署在本地，需要更换成对应的ip地址
        db.setUserName("root");
        db.setDatabaseName("coopboard");
        db.setPassword("root");
        db.setPort(3306);

        // 尝试是否能打开数据库连接实例
        if(!db.open()){
            qDebug()<<"连接池创建新的数据库连接失败"<< db.lastError().text();
            m_connectionCount--;
            return QSqlDatabase();// 返回无效连接
        }

        return db;//返回有效连接

    }

    // 连接池已满且无可用连接
    qDebug()<<"数据库连接达到上限且无空闲连接";
    return QSqlDatabase();// 返回无效连接
}

void ConnectionPool::releaseConnection(QSqlDatabase &db)
{
    // 由于多个用户指针共用连接池，所以使用互斥锁来保护对连接池的修改操作
    QMutexLocker locker(&m_mutex);

    if (db.isOpen()) { // 确保连接仍然打开
        if (m_connectionPool.size() < MAX_CONNECTIONS) {
            // 存入空闲队列
            m_connectionPool.enqueue(db);

        } else {
            QString connectionName = db.connectionName();
            db.close();
            QSqlDatabase::removeDatabase(connectionName); // 移除连接
        }
    }
}
