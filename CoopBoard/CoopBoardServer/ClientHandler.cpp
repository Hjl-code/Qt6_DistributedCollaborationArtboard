#include "ClientHandler.h"

#include <QSqlQuery>
#include <QSqlError>

//UserTable(account, id, status(onLine, cooping, offLine))
//CoopTable(id, account, account1, account2, account3, timestamp)
//DataTable(id, account, data, timestamp)

ClientHandler::ClientHandler(QWebSocket* socket,const QString& account,QObject *parent)
    : QObject{parent},m_socket(socket), m_account(account),m_id(-1),m_isOwner(false)
{
    // 使用qobject_cast安全地将parent转换为Server*类型并保存
    m_server = qobject_cast<Server*>(parent);
    if (!m_server) {
        qWarning() << "将parent转换为Server*类型的操作失败";
    }
    // 获取一个自身的数据库连接
    m_db = m_server->getDBConnection();

    // 信号槽的连接
    connect(m_socket,&QWebSocket::disconnected,this,&ClientHandler::onDisconnected);
    connect(m_socket,&QWebSocket::textMessageReceived,this,&ClientHandler::onTextMessageReceived);
}

void ClientHandler::sendJsonObject(QJsonObject &jsonObj)
{
    jsonObj["end"] = "end";// 添加结束标志（用来保证数据包完整并且区别开非法数据包）
    QJsonDocument jsonDoc(jsonObj);
    QByteArray jsonData = jsonDoc.toJson();
    QString jsonStr = QString::fromUtf8(jsonData);
    m_socket->sendTextMessage(jsonStr);
}

void ClientHandler::addDbDrawData(QJsonObject &jsonObj)
{
    // 获取标准时间
    QDateTime currTime = QDateTime::currentDateTimeUtc();
    QString timestamp = currTime.toString("yyyy-MM-dd HH:mm:ss"); // 标准成SQL格式

    // 获取json字符串
    QJsonDocument jsonDoc(jsonObj);
    QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);
    QString jsonStr(jsonData);

    // 由于是在子线程中进行的，为了防止竞争，获取一个独立的数据库连接实例
    QSqlDatabase db = m_server->getDBConnection();
    if(!db.open()){
        qDebug()<<"将绘制信息存入数据库的逻辑的数据库连接实例无效";
        return;
    }

    // 将json数据（存储着绘制信息）放入数据库
    QSqlQuery query(db);
    query.prepare("insert into DataTable(id,account,data,timestamp) values(:id,:account,:data,:timestamp)");
    query.bindValue(":id",m_id);
    query.bindValue(":account",m_account);
    query.bindValue(":data",jsonStr);
    query.bindValue(":timestamp",timestamp);
    if(!query.exec()){
        qDebug()<<"一条新的绘制信息插入失败！"<<query.lastError();
    }

    // 释放独立的数据库连接
    m_server->releaseDBConnection(db);
}

void ClientHandler::teammateChange(bool flag, ClientHandler* c, bool isRecevied)
{
    // 如果指针无效则直接返回
    if(c == nullptr){
        return;
    }

    // 尝试查找看看是否已经存在这个队友（未找到返回值是-1）
    int index = m_teammate.indexOf(c);

    // 为flag真表示是新增队友(建立连接)
    if(flag){
        if(index == -1){
            m_teammate.append(c);
            // 连接不同用户实例间需要的所有的信号槽
            connect(this,&ClientHandler::drawEventBoard,c,&ClientHandler::onDrawEventBoard);
            connect(this,&ClientHandler::outOfBoard,c,&ClientHandler::onOutOfBoard);
            connect(this,&ClientHandler::cutTeamate,c,&ClientHandler::teammateChange);
            connect(this,&ClientHandler::newTeammate,c,&ClientHandler::teammateChange);
            connect(this,&ClientHandler::rubberItem,c,&ClientHandler::onRubberItem);

            // 如果自己不是被通知连接的一方，则通知对方也要连接我（相互断开连接）
            if(!isRecevied){
                emit newTeammate(true,this,true);
            }
        }

    }// flag为假表示是删除队友(删除连接)
    else{
        if(index != -1){
            m_teammate.removeAt(index);

            // 如果自己不是被通知断开的一方，则通知对方也要断开（相互断开连接）
            if(!isRecevied){
                emit cutTeamate(false,this,true);
            }

            // 断开不同用户实例间需要的所有的信号槽
            disconnect(this,&ClientHandler::drawEventBoard,c,&ClientHandler::onDrawEventBoard);
            disconnect(this,&ClientHandler::outOfBoard,c,&ClientHandler::onOutOfBoard);
            disconnect(this,&ClientHandler::cutTeamate,c,&ClientHandler::teammateChange);
            disconnect(this,&ClientHandler::newTeammate,c,&ClientHandler::teammateChange);
            disconnect(this,&ClientHandler::rubberItem,c,&ClientHandler::onRubberItem);
        }
    }
}

void ClientHandler::teamateAllCut()
{
    // 断开自身与其他用户的所有需要的信号槽（teammateChange函数中也有让对方断开我的逻辑）
    // 使用临时变量复制后操作，是因为teammateChange函数有对容器的移除操作，影响遍历
    QVector<QPointer<ClientHandler>> tmp = m_teammate;
    for(ClientHandler* c: tmp){
        teammateChange(false, c, false);
    }
}

void ClientHandler::sendOldDataWhenJoin()
{
    // 多线程发送(涉及到的对socket的发送操作通过信号回给主线程)
    m_server->m_threadPool->start([this]() mutable{
        // 申请独立的数据库连接
        QSqlDatabase db = m_server->getDBConnection();
        if(!db.open()){
            qDebug()<<"发送已有的绘制数据逻辑的数据库实例无效";
            return;
        }

        // 将已有的绘制数据从数据库中取出并存为字节数组
        QJsonArray dataArray;
        QSqlQuery query(db);
        query.prepare("select data from DataTable where id = :id order by timestamp");
        query.bindValue(":id",m_id);
        if(!query.exec()){
            qDebug()<<"用户"<<m_account<<"刚加入连接，进行数据同步的sql语句执行失败"<<query.lastError();
            return;
        }

        while (query.next()) {
            QString dataStr = query.value("data").toString();
            QByteArray dataBytes = dataStr.toUtf8();
            QJsonDocument doc = QJsonDocument::fromJson(dataBytes);
            if (doc.isNull()) {
                continue; // 如果数据无法解析为 JSON，则跳过
            }

            QJsonObject js = doc.object();
            if (!js.isEmpty()) { // 无效则跳过
                dataArray.append(js);
            }
        }

        if(dataArray.isEmpty()){
            qDebug()<<"服务器发现用户"<<m_account<<"需要的初始化数据为空";
            return;// 为空则直接返回
        }

        // 释放申请的独立连接
        m_server->releaseDBConnection(db);

        // 分段发送数据
        const int chunkSize = 1024;  // 每次发送的最大数据量
        int totalChunks = (dataArray.size() + chunkSize - 1) / chunkSize;  // 计算总的分段数
        // 遍历每一个块（段）
        for (int i = 0; i < totalChunks; ++i) {
            QJsonArray chunkData;
            for (int j = i * chunkSize; j < (i + 1) * chunkSize && j < dataArray.size(); ++j) {
                // 往块（段）中填充绘制数据条目
                chunkData.append(dataArray[j]);
            }

            QJsonObject jsonObj;
            jsonObj["tag"] = "initData";
            jsonObj["chunk"] = i;
            jsonObj["totalChunks"] = totalChunks;
            jsonObj["data"] = chunkData;

            // 子线程不能直接对m_socket操作（子线程不应该操作关键数据），调用主线程的函数来实现发送逻辑
            QMetaObject::invokeMethod(this,"sendJsonObject",Qt::QueuedConnection,Q_ARG(QJsonObject, jsonObj));
        }
    });
}

void ClientHandler::deleteDrawItem(QJsonObject &jsonObj)
{
    // 申请独立的数据库连接
    QSqlDatabase db = m_server->getDBConnection();
    if(!db.open()){
        qDebug()<<"删除某一条绘制数据逻辑的数据库实例无效";
        return;
    }

    //从数据库中删除
    QSqlQuery query(db);
    query.prepare("DELETE FROM DataTable WHERE account = :account AND JSON_UNQUOTE(data->>'$.number') = :number");
    query.bindValue(":account",jsonObj["account"].toString());
    query.bindValue(":number",jsonObj["number"].toString());
    if(!query.exec()){
        qDebug()<<"数据库删除客户端"<<jsonObj["account"].toString()<<"编号"<<jsonObj["number"].toString()
                 <<"的sql语句执行失败"<<query.lastError();
    }

    //释放独立的数据库连接
    m_server->releaseDBConnection(db);
}

void ClientHandler::dealCreatorBoard()
{
    // 保证创建画板时断开所有旧的连接
    teamateAllCut();

    // 尝试获取一个唯一的画板id
    int id = QRandomGenerator::global()->bounded(3000000);//随机获取一个id（0-2999999）

    // 判断表中是否已经存在此id
    QSqlQuery query(m_db);
    query.prepare("select count(*) from CoopTable where id = :id");
    query.bindValue(":id",id);
    // 循环判断（但其实设置一个最大尝试次数比较好）
    while(query.exec() && query.next()){
        if(query.value(0) != 0){//说明有重复了
            id = QRandomGenerator::global()->bounded(3000000); // 重新生成 id
            continue; // 继续检查新的 id
        }
        break; // 如果没有重复，跳出循环
    }

    // 如果是因为sql语句执行失败而退出循环的
    if (!query.isActive()) {
        qDebug() << "检查CoopTable表中id是否重复的sql语句执行失败" << query.lastError();
        //通知客户端创建失败
        QJsonObject jsonObj;
        jsonObj["tag"] = "creatorBoard";
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }

    // 获取标准时区的时间
    QDateTime currTime = QDateTime::currentDateTimeUtc();// 获取标准时区时间
    QString timestamp = currTime.toString("yyyy-MM-dd HH:mm:ss"); // 标准成SQL格式

    // 插入与更新数据库
    if(!m_db.transaction()){
        qDebug() << "创建画板启动数据库事务失败：";
        return;
    }

    // 插入画板表新记录
    query.prepare("insert into CoopTable(id,account,account1,account2,account3,timestamp) values(:id,:account,NULL,NULL,NULL,:timestamp)");
    query.bindValue(":id",id);
    query.bindValue(":account",m_account);
    query.bindValue(":timestamp",timestamp);
    if(!query.exec()){
        qDebug()<<"服务器插入新建画板到数据库的sql语句执行失败"<<query.lastError();
        //通知客户端创建失败
        QJsonObject jsonObj;
        jsonObj["tag"] = "creatorBoard";
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }

    // 更改UserTable的status字段为cooping（由于当前画板程序比较简单，这个字段并没有用到）
    query.prepare("update UserTable set id = :id,status = 'cooping' where account = :account");
    query.bindValue(":id",id);
    query.bindValue(":account",m_account);
    if(!query.exec()){
        qDebug()<<"服务器在创建画板时，更新该用户的状态为cooping的sql语句执行失败"<<query.lastError();
        m_db.rollback();//回滚数据库事务

        //通知客户端创建失败
        QJsonObject jsonObj;
        jsonObj["tag"] = "creatorBoard";
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }
    m_db.commit();//提交数据库事务

    // 通知客户端创建成功
    QJsonObject jsonObj;
    jsonObj["tag"] = "creatorBoard";
    jsonObj["answer"] = "succeed";
    jsonObj["inviteCode"] = QString::number(id);//邀请码就是画板ID
    sendJsonObject(jsonObj);

    // 自己创建的，所以自己就是主人
    m_isOwner = true;
    m_id = id;
}

void ClientHandler::dealExitBoard()
{
    QJsonObject jsonObj;
    jsonObj["tag"] = "exitBoard";

    if(!m_db.transaction()){
        qDebug() << "离开画板启动数据库事务失败：";
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }

    // 在数据库中的CoopTable中删除记录
    QSqlQuery query(m_db);
    if(m_isOwner){
        // 如果是主人，则通知其他协作用户被踢出去了(同时删除画板)
        emit outOfBoard();

        // 删除画板（此时数据库中的相关的绘制信息也会被清除，因为外键约束）
        query.prepare("delete from CoopTable where id = :id");
        query.bindValue(":id",m_id);
        if(!query.exec()){
            qDebug()<<"当主人离开时删除画板"<<m_id<<"的sql语句执行失败（脏数据）"<<query.lastError();
            jsonObj["answer"] = "faild";
            sendJsonObject(jsonObj);
            return;
        }
    }
    else{
        //如果不是主人，在CoopTable表中自己的位置置空
        query.prepare("UPDATE CoopTable "
                      "SET account1 = CASE WHEN account1 = :account THEN NULL ELSE account1 END, "
                      "account2 = CASE WHEN account2 = :account THEN NULL ELSE account2 END, "
                      "account3 = CASE WHEN account3 = :account THEN NULL ELSE account3 END "
                      "WHERE id = :id");
        query.bindValue(":account",m_account);
        query.bindValue(":id",m_id);
        if(!query.exec()){
            qDebug()<<"离开画板在数据库中的CoopTable中清空自身字段记录的sql语句执行失败"<<query.lastError();
            jsonObj["answer"] = "faild";
            sendJsonObject(jsonObj);
            return;
        }
    }

    // 在UserTable中的id字段删除，status字段改为onLine
    query.prepare("update UserTable set id = NULL, status = 'onLine' where account = :account");
    query.bindValue(":account",m_account);
    if(!query.exec()){
        m_db.rollback();
        qDebug()<<"离开画板在UserTable中的id字段删除，status字段改为onLine的sql语句执行失败"<<query.lastError();
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }
    m_db.commit();

    // 自己断开与旧队友的所有连接
    teamateAllCut();

    // 通知客户端，断开成功
    jsonObj["answer"] = "succeed";
    sendJsonObject(jsonObj);
    m_isOwner = false;
    m_id = -1;
}

void ClientHandler::dealJoinBoard(int id)
{
    QSqlQuery query(m_db);
    QJsonObject jsonObj;
    jsonObj["tag"] = "joinBoard";

    // 根据邀请码，查找是否存在画板
    query.prepare("select * from CoopTable where id = :id");
    query.bindValue(":id",id);
    if(!query.exec() || !query.next()){
        qDebug()<<"根据邀请码查找画板的sql语句执行失败或没有找到"<<query.lastError();
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }

    // 判断对应画板是否有空位
    int index = 0;
    if(query.value("account1").toString().isEmpty()){//有空位1
        index = 1;
    }
    else{
        // 不为空则将其加到队友列表（先不管最后能不能成功加入）
        ClientHandler* c = m_server->getClientHandlerByAccount(query.value("account1").toString());
        if(c) teammateChange(true, c, false);

        // 继续判断第二个
        if(query.value("account2").toString().isEmpty()){//有空位2
            index = 2;
        }
        else{
            // 不为空则将其加到队友列表（先不管最后能不能成功加入）
            ClientHandler* c = m_server->getClientHandlerByAccount(query.value("account2").toString());
            if(c) teammateChange(true, c, false);

            // 继续判断第三个
            if(query.value("account3").toString().isEmpty()){//有空位3
                index = 3;
            }
            else{
                // 不为空则将其加到队友列表（先不管最后能不能成功加入）
                ClientHandler* c = m_server->getClientHandlerByAccount(query.value("account3").toString());
                if(c) teammateChange(true, c, false);
            }
        }
    }

    if(index == 0){
        qDebug()<<"当前画板已经达到最大数量了";

        // 断开与队友的连接（由于最后没加进去，已经无效了）
        teamateAllCut();

        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }

    // 将自己与画板主人也建立连接
    QString hostAccount = query.value("account").toString();
    ClientHandler* host = m_server->getClientHandlerByAccount(hostAccount);
    // 如果画板的主人的账号和指针获取失败
    if(hostAccount.isEmpty() || host == nullptr){
        teamateAllCut();
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }
    // 添加画板的主人
    teammateChange(true, host, false);

    // 更新CoopTable中对应空位为自己的账号
    if(!m_db.transaction()){
        qDebug() << "加入画板逻辑启动数据库事务失败：";
        teamateAllCut();
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }

    QString sql = QString("update CoopTable set account%1 = :account where id = :id").arg(index);
    query.prepare(sql);
    query.bindValue(":account",m_account);
    query.bindValue(":id",id);
    if(!query.exec()){
        qDebug()<<"建立连接逻辑更新数据库的sql语句执行出错"<<query.lastError();
        teamateAllCut();
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }

    // 更新自己在UserTable中的id和status字段
    query.prepare("update UserTable set id = :id, status = 'cooping' where account = :account");
    query.bindValue(":id",id);
    query.bindValue(":account",m_account);
    if(!query.exec()){
        qDebug()<<"建立连接逻辑更新数据库的sql语句执行出错"<<query.lastError();
        m_db.rollback();
        teamateAllCut();
        jsonObj["answer"] = "faild";
        sendJsonObject(jsonObj);
        return;
    }
    m_db.commit();// 提交数据库事务

    jsonObj["answer"] = "succeed";
    sendJsonObject(jsonObj);
    m_isOwner = false;
    m_id = id;

    // 准备发送画板上已有的数据(多线程处理)
    sendOldDataWhenJoin();
}

void ClientHandler::dealDrawEvent(QJsonObject &jsonObj)
{
    // 在广播之前，将自己的account信息存入，目的是便于其他写作用户识别出这条绘制数据是谁的（擦除操作）
    if(jsonObj["account"].toString() == "-1"){
        jsonObj["account"] = m_account;
    }

    // 由于绘制操作比较频繁，在子线程进行广播和插入数据库的处理
    m_server->m_threadPool->start([this,jsonObj]() mutable{
        // 广播给同一群组的人（信号和槽已经通过teammateChange函数连接好了）
        emit drawEventBoard(jsonObj);
        // 插入到数据库中
        addDbDrawData(jsonObj);
    });
}

void ClientHandler::dealRubberItem(QJsonObject &jsonObj)
{
    // 如果未定义或者是默认值，说明是自己删除了自己的项，存入自己的账号信息再进行广播
    if(jsonObj["account"].isUndefined() || jsonObj["account"].toString() == "-1"){
        jsonObj["account"] = m_account;
    }

    // 由于擦除操作比较频繁，在子线程进行广播和删除处理
    m_server->m_threadPool->start([this,jsonObj]() mutable{
        // 广播给同一群组的人（信号和槽已经通过teammateChange函数连接好了）
        emit rubberItem(jsonObj);
        // 数据库中进行同步删除记录
        deleteDrawItem(jsonObj);
    });
}

void ClientHandler::onDisconnected()
{
    // 先执行离开画板的操作
    dealExitBoard();

    // 将自己从UserTable表中删除（执行离开画板的操作时，主人就会删除自己的画板，避免了外键约束）
    QSqlQuery query(m_db);
    query.prepare("delete from UserTable where account = :account");
    query.bindValue(":account",m_account);
    if(!query.exec()){
        qDebug()<<"用户："<<m_account<<"关闭了程序，但是从用户表中去除的sql语句执行失败"<<query.lastError()<<"脏数据";
    }

    // 将自己从哈希表中删除,并释放数据库连接
    m_server->removeClientToHash(m_account);
    m_server->releaseDBConnection(m_db);

    qDebug()<<"客户端："<<m_account << "断开连接";
}

void ClientHandler::onTextMessageReceived(const QString &message)
{
    // 将收到的消息转换为json字符串
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if(!jsonDoc.isObject()){// 如果不是json格式，直接退出
        qDebug()<<"服务器收到非法数据";
        return;
    }

    // 转为json对象
    QJsonObject jsonObj = jsonDoc.object();
    if(jsonObj["end"] != "end"){// 没有结束标志则不处理
        qDebug()<<"服务器收到缺少结束标志的非法数据";
        return;
    }

    //根据json["tag"]标签进行分类处理
    QString tag = jsonObj["tag"].toString();

    if(tag == "creatorBoard"){
        dealCreatorBoard();
    }
    else if(tag == "exitBoard"){
        dealExitBoard();
    }
    else if(tag == "joinBoard"){
        QString inviteStr = jsonObj["inviteCode"].toString();
        int invite = inviteStr.toInt();
        dealJoinBoard(invite);
    }
    else if(tag == "draw"){
        dealDrawEvent(jsonObj);
    }
    else if(tag == "rubber"){
        dealRubberItem(jsonObj);
    }
}

void ClientHandler::onOutOfBoard()
{
    // 先将自身的连接全部断开
    teamateAllCut();

    // 画板主人已经将整个画板删除，因此不用自己去除自己CoopTable表中对应字段的逻辑

    // 将UserTable中自己的id字段置空，status字段改为onLine
    QSqlQuery query(m_db);
    query.prepare("update UserTable set id = NULL, status = 'onLine' where account = :account");
    query.bindValue(":account",m_account);
    if(!query.exec()){
        qDebug()<<"被踢出画板逻辑中的在UserTable中的id字段删除，status字段改为onLine的sql语句执行失败"<<query.lastError();
        // 没有成功删除也不影响后续操作
    }

    // 通知客户端，被踢出画板
    QJsonObject jsonObj;
    jsonObj["tag"] = "outOfBoard";
    sendJsonObject(jsonObj);
    m_isOwner = false;
    m_id = -1;
}

void ClientHandler::onDrawEventBoard(QJsonObject jsonObj)
{
    // 收到一条队友的绘制信息，转发给客户端
    sendJsonObject(jsonObj);
}

void ClientHandler::onRubberItem(QJsonObject jsonObj)
{
    // 如果是自己的话，将账号字段设置回默认值，因为客户端并不知道自己的账号，是用默认值代替的
    if(jsonObj["account"].toString() == m_account){
        jsonObj["account"] = "-1";
    }

    // 收到一条队友删除某一项的消息，转发给客户端
    sendJsonObject(jsonObj);
}
