#include "Server.h"
#include "ClientHandler.h"

#include <QSqlQuery>
#include <QSqlError>

Server::Server(QObject *parent)
    : QObject{parent}
{
    // 实例化WebSocket对象
    m_webSocket = new QWebSocketServer("MyServer",QWebSocketServer::NonSecureMode,this);
    // 实例化连接池对象
    m_connectionPool = new ConnectionPool();
    // 保存全局线程池对象，并设置最大线程数
    m_threadPool = QThreadPool::globalInstance();
    m_threadPool->setMaxThreadCount(300);// 限制最多为300个线程

    // 获取并保存自身的数据库连接
    m_db = getDBConnection();

    // 开启监听
    webSocketListen();
}

Server::~Server()
{
    // 析构时释放自身的数据库连接
    releaseDBConnection(m_db);

    // 释放堆上的内存
    if(m_connectionPool){
        delete m_connectionPool;
    }
    if(m_webSocket){
        delete m_webSocket;
    }
}

void Server::webSocketListen()
{
    // 尝试开启监听，端口8090（可自定义）
    if(m_webSocket->listen(QHostAddress::Any, 8090)){
        connect(m_webSocket,&QWebSocketServer::newConnection,this,&Server::onNewConnection);
        qDebug()<<"服务器开启监听，端口8090";
    }else{
        qDebug()<<"服务器无法开启监听！";
    }
}


void Server::onNewConnection()
{
    // 获取到新连接的套接字
    QWebSocket* socket = m_webSocket->nextPendingConnection();

    // 随机创建新连接的account账号
    int accountNum = QRandomGenerator::global()->bounded(3000000,9000000);
    // 判断表中是否已经存在此id
    QSqlQuery query(m_db);
    query.prepare("select count(*) from UserTable where account = :account");
    query.bindValue(":account",QString::number(accountNum));

    // 循环判断，直到没有冲突（可以设置最大尝试次数，防止一直死循环）
    while(query.exec() && query.next()){
        if(query.value(0).toInt() != 0){// 说明有重复了
            accountNum = QRandomGenerator::global()->bounded(3000000,9000000); // 重新生成账号
            continue; // 继续检查新的账号
        }
        break; // 如果没有重复，跳出循环
    }

    //如果是因为sql语句执行失败而跳出循环的话
    if (!query.isActive()) {
        qDebug() << "创建账号的逻辑的sql语句执行失败" << query.lastError();
        return;
    }

    // 存入数据库
    QString account = QString::number(accountNum);
    query.prepare("insert into UserTable(account,id,status) values(:account,NULL,'onLine')");
    query.bindValue(":account",account);
    if(query.exec()){
        // 实例化新客户的连接
        ClientHandler* handler = new ClientHandler(socket,account,this);
        // 存入哈希表
        addClientToHash(account,handler);
        qDebug()<<"客户端："<<account<<"成功连接服务器";
    }else{
        qDebug()<<"服务端向数据库插入新用户连接执行失败"<<query.lastError();
    }
}

void Server::addClientToHash(const QString &account, ClientHandler *handler)
{
    // 由于多个用户指针共用Server，所以需要写锁来保护对哈希表的修改操作
    QWriteLocker locker(&m_lock);
    if(!m_clientMap.contains(account)){
        m_clientMap.insert(account, handler);
    }
}

void Server::removeClientToHash(const QString &account)
{
    // 由于多个用户指针共用Server，所以需要写锁来保护对哈希表的修改操作
    QWriteLocker locker(&m_lock);
    if(m_clientMap.contains(account)){
        m_clientMap.remove(account);
    }
}

QSqlDatabase Server::getDBConnection()
{
    return m_connectionPool->getConnection();
}

void Server::releaseDBConnection(QSqlDatabase &db)
{
    m_connectionPool->releaseConnection(db);
}

ClientHandler *Server::getClientHandlerByAccount(QString account)
{
    // 由于多个用户指针共用Server，所以需要读锁来保护对哈希表的修改操作
    QReadLocker locker(&m_lock);
    if(m_clientMap.contains(account)){
        return m_clientMap[account];
    }
    return nullptr;
}

