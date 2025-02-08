#include "Controller.h"
#include "ui_Controller.h"
#include "MyGraphicsScene.h"
#include <QCloseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QThreadPool>
#include <QTimer>

Controller::Controller(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Controller)
    ,m_drawType(DrawType::LineType)
{
    ui->setupUi(this);
    setWindowTitle("分布式画板程序");// 设置窗口标题
    setFixedSize(1200,800);// 设置窗口的固定大小
    initUi();// 初始化ui
    initConnection();// 初始化连接(与服务端)
    changeSelect(m_drawType);// 初始化选择的绘制类型

    // 连接信号和槽
    connect(m_scene,&MyGraphicsScene::drawEvent,this,&Controller::onDrawEvent);
    connect(m_scene,&MyGraphicsScene::rubberItem,this,&Controller::onRubberItem);
}

Controller::~Controller()
{
    delete ui;
}

void Controller::initUi()
{
    // 实例化自己的scene对象
    m_scene = new MyGraphicsScene(this);

    //给QGraphView控件设置场景
    ui->graphicsView->setScene(m_scene);

    //连接传递颜色、字号、类型的信号和槽
    connect(m_scene,&MyGraphicsScene::needData,[=](){
        m_scene->setData(ui->colorBtn->getColor(),ui->weightLabel->text().toInt(),m_drawType);
    });

    //在成功连接协作画板前设置一些按钮为不可用，另一些启用
    setDrawToolEnable(false);

    //初始化焦点事件（避免一打开窗口时，邀请码输入框获得焦点，太难看）
    ui->connectBtn->setFocus();
}

void Controller::initConnection()
{
    // 实例化套接字对象
    m_socket = new QWebSocket();

    // 连接信号槽
    connect(m_socket, &QWebSocket::connected, this, &Controller::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &Controller::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &Controller::onTextMessageReceived);

    // 连接服务端，如果服务端不运行在本地，则将localhost改为相应的ip地址
    m_socket->open(QUrl("ws://localhost:8090"));
}

void Controller::setDrawToolEnable(bool flag)
{
    // 分析：如果 “颜色、直线、曲线、椭圆、矩形、字号滑块、橡皮擦、画布、断开连接” 部分可用，
    // 那么 “建立连接、创建连接、邀请码输入框” 部分就不能再被触发，即状态相反
    ui->colorBtn->setEnabled(flag);
    ui->lineBtn->setEnabled(flag);
    ui->curveBtn->setEnabled(flag);
    ui->ellipseBtn->setEnabled(flag);
    ui->rectangleBtn->setEnabled(flag);
    ui->weightSlide->setEnabled(flag);
    ui->rubberBtn->setEnabled(flag);
    ui->graphicsView->setEnabled(flag);
    ui->disconnectBtn->setEnabled(flag);

    ui->connectBtn->setEnabled(!flag);// 建立连接按钮相反
    ui->createBtn->setEnabled(!flag);// 创建连接按钮相反
    ui->initveLineEdit->setEnabled(!flag);// 邀请码输入框相反
}

void Controller::changeSelect(DrawType type)
{
    // 被选中的有一个蓝色边框，其他的恢复默认
    ui->lineBtn->setStyleSheet("border: 0px;");
    ui->curveBtn->setStyleSheet("border: 0px;");
    ui->ellipseBtn->setStyleSheet("border: 0px;");
    ui->rectangleBtn->setStyleSheet("border: 0px;");
    ui->rubberBtn->setStyleSheet("border: 0px;");

    if(type == DrawType::LineType){
        ui->lineBtn->setStyleSheet("border: 1px solid blue;");
    }
    else if(type == DrawType::CurveType){
        ui->curveBtn->setStyleSheet("border: 1px solid blue;");
    }
    else if(type == DrawType::EllipseType){
        ui->ellipseBtn->setStyleSheet("border: 1px solid blue;");
    }
    else if(type == DrawType::RectangleType){
        ui->rectangleBtn->setStyleSheet("border: 1px solid blue;");
    }
    else if(type == DrawType::RubberType){
        ui->rubberBtn->setStyleSheet("border: 1px solid blue;");
    }
}

void Controller::sendJsonObject(QJsonObject &jsonObj)
{
    jsonObj["end"] = "end";// 添加结束标志
    QJsonDocument jsonDoc(jsonObj);
    QByteArray jsonData = jsonDoc.toJson();
    QString jsonStr = QString::fromUtf8(jsonData);
    m_socket->sendTextMessage(jsonStr);
    m_socket->flush(); //刷新，立即发送
}

void Controller::closeEvent(QCloseEvent *event)
{
    // 关闭前发送给服务端一个离开画板的请求，让服务端从数据库删掉相应的数据
    sendExitBoardMsg();

    // 延迟100ms再关闭窗口，确保上面的请求发送完成
    QTimer::singleShot(100, this, [this, event]() {
        event->accept();
        close();
    });
}

void Controller::sendCreatorBoardMsg()
{
    // 准备创建画板的数据包，通知服务器创建画板
    QJsonObject jsonObj;
    jsonObj["tag"] = "creatorBoard";
    sendJsonObject(jsonObj);
}

void Controller::sendExitBoardMsg()
{
    // 准备离开画板的数据包，通知服务器退出协作
    QJsonObject jsonObj;
    jsonObj["tag"] = "exitBoard";
    sendJsonObject(jsonObj);
}

void Controller::sendJoinBoardMsg()
{
    // 准备加入画板的数据包，通知服务器加入协作
    int invite = ui->initveLineEdit->text().toInt();
    QJsonObject jsonObj;
    jsonObj["tag"] = "joinBoard";
    jsonObj["inviteCode"] = QString::number(invite);
    sendJsonObject(jsonObj);
}

void Controller::dealExitBoardMsg(const QJsonObject &jsonObj)
{
    if(jsonObj["answer"].toString() == "faild"){
        ui->statusbar->showMessage("当前无法离开协作，请稍后再尝试",5000);
    }
    else if(jsonObj["answer"].toString() == "succeed"){
        m_scene->clear();
        m_receivedChunks.clear();

        // 离开之后改变按钮状态
        setDrawToolEnable(false);
    }
}

void Controller::dealOutOfBoard()
{
    m_scene->clear();

    // 踢出之后改变按钮状态
    setDrawToolEnable(false);
}

void Controller::dealJoinBoard(const QJsonObject &jsonObj)
{
    if(jsonObj["answer"].toString() == "faild"){
        ui->statusbar->showMessage("通过邀请码加入协作失败，请检查邀请码是否正确",5000);
    }
    else if(jsonObj["answer"].toString() == "succeed"){
        // 成功之后改变按钮状态
        setDrawToolEnable(true);
    }
}

void Controller::dealDrawEvent(QJsonObject jsonObj)
{
    // 由于绘制事件比较频繁，因此drawNewEvent方法的实现是多线程的，避免阻塞
    m_scene->drawNewEvent(jsonObj);
}

void Controller::dealOldDataWhenJoin(QJsonObject jsonObj)
{
    // 取出数据
    int chunk = jsonObj["chunk"].toInt();// 块编号
    int totalChunks = jsonObj["totalChunks"].toInt();// 总块数
    QJsonArray chunkData = jsonObj["data"].toArray(); // 块的数据

    m_receivedChunks[chunk] = chunkData;// 将收到的块存入容器

    // 如果传输完毕
    if(m_receivedChunks.size() == totalChunks){
        // 合并片段
        QJsonArray completeData;
        for(QJsonArray& it : m_receivedChunks.values()){
            for(int j = 0; j < it.size(); j++){
                completeData.append(it[j]);
            }
        }

        m_receivedChunks.clear();//清空防止影响下一次

        // 进行绘制
        QJsonObject js;
        for(int i = 0;i<completeData.size();i++){
            js = completeData[i].toObject();
            if(!js.isEmpty()){
                // 由于绘制事件比较频繁，因此drawNewEvent方法的实现是多线程的，避免阻塞
                m_scene->drawNewEvent(js);
            }
        }
    }
}

void Controller::dealRubberItem(QJsonObject jsonObj)
{
    // 取出数据（账号和编号）
    qint64 number = jsonObj["number"].toVariant().toLongLong();
    QString account = jsonObj["account"].toString();

    // 遍历m_scene中的控件，账号和编号可以唯一确定一个绘制项
    QList<QGraphicsItem*> items = m_scene->items();
    for(QGraphicsItem* item : items){
        Element* element = qgraphicsitem_cast<Element*>(item);
        if(element){
            // 比较是否相同
            if(number == element->getNumber() && account == element->getAccount()){
                // 相同则进行删除
                m_scene->removeItem(item);
                delete item;
            }
        }
    }
}

void Controller::dealCreatorBoardMsg(const QJsonObject &jsonObj)
{
    if(jsonObj["answer"].toString() == "faild"){
        ui->statusbar->showMessage("创建协作画板失败，请稍后再试",5000);
    }
    else if(jsonObj["answer"].toString() == "succeed"){
        QString inviteCode = jsonObj["inviteCode"].toString();
        ui->initveLineEdit->setText(inviteCode);

        // 创建之后改变按钮状态
        setDrawToolEnable(true);
    }
}

void Controller::onConnected()
{
    ui->statusbar->showMessage("成功连接到服务器",5000);
}

void Controller::onDisconnected()
{
    ui->statusbar->showMessage("服务器连接失败或中断，请检查网络连接",5000);
}

void Controller::onTextMessageReceived(const QString &message)
{
    // 检查收到的数据是不是json格式
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if(!jsonDoc.isObject()){
        qDebug()<<"客户端收到一条非法数据";
        return;
    }

    // 转为json对象
    QJsonObject jsonObj = jsonDoc.object();
    if(jsonObj["end"] != "end"){
        qDebug()<<"客户端收到一条没有结束标志的非法数据";
        return;
    }

    //根据json["tag"]标签进行分类处理
    QString tag = jsonObj["tag"].toString();

    if(tag == "creatorBoard"){
        dealCreatorBoardMsg(jsonObj);
    }
    else if(tag == "draw"){
        dealDrawEvent(jsonObj);
    }
    else if(tag == "exitBoard"){
        dealExitBoardMsg(jsonObj);
    }
    else if(tag == "outOfBoard"){
        dealOutOfBoard();
    }
    else if(tag == "joinBoard"){
        dealJoinBoard(jsonObj);
    }
    else if(tag == "initData"){
        dealOldDataWhenJoin(jsonObj);
    }
    else if(tag == "rubber"){
        dealRubberItem(jsonObj);
    }
}

void Controller::onDrawEvent(QJsonObject jsonObj)
{
    // 收到MyGraphicsScene的一条绘制数据包，转发到服务端
    sendJsonObject(jsonObj);
}

void Controller::onRubberItem(QJsonObject jsonObj)
{
    // 收到MyGraphicsScene的一条删除绘制项的数据包，转发到服务端
    sendJsonObject(jsonObj);
}

void Controller::on_weightSlide_valueChanged(int value)
{
    // 当字号滑块发生改变时，同步字号的显示
    ui->weightLabel->setText(QString::number(value));
}


void Controller::on_lineBtn_clicked()
{
    // 改变当前绘制类型，启动changeSelect函数更新选中
    m_drawType = DrawType::LineType;
    changeSelect(m_drawType);
}


void Controller::on_curveBtn_clicked()
{
    // 改变当前绘制类型，启动changeSelect函数更新选中
    m_drawType = DrawType::CurveType;
    changeSelect(m_drawType);
}


void Controller::on_ellipseBtn_clicked()
{
    // 改变当前绘制类型，启动changeSelect函数更新选中
    m_drawType = DrawType::EllipseType;
    changeSelect(m_drawType);
}


void Controller::on_rectangleBtn_clicked()
{
    // 改变当前绘制类型，启动changeSelect函数更新选中
    m_drawType = DrawType::RectangleType;
    changeSelect(m_drawType);
}


void Controller::on_rubberBtn_clicked()
{
    // 改变当前绘制类型，启动changeSelect函数更新选中
    m_drawType = DrawType::RubberType;
    changeSelect(m_drawType);
}


void Controller::on_createBtn_clicked()
{
    // 发送创建画板的消息给服务端
    sendCreatorBoardMsg();
}


void Controller::on_disconnectBtn_clicked()
{
    // 改变按钮的状态
    setDrawToolEnable(false);

    // 清除所有已经绘制的内容
    m_scene->clear();

    // 发送离开协作的消息给服务端
    sendExitBoardMsg();
}


void Controller::on_connectBtn_clicked()
{
    //检查邀请码是否为空或者非法（0-3000000）
    int invite = ui->initveLineEdit->text().toInt();
    if(invite < 0 || invite >= 3000000){
        ui->statusbar->showMessage("邀请码不正确，请确认后重试",5000);
        return;
    }

    //发送建立连接的消息给服务端
    sendJoinBoardMsg();
}

