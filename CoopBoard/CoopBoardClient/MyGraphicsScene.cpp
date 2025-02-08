#include "MyGraphicsScene.h"
#include <QGraphicsSceneMouseEvent>

MyGraphicsScene::MyGraphicsScene(QObject *parent)
    : QGraphicsScene{parent}
{
    // 将scene的大小设置成和QGraphicsView控件一样大
    setSceneRect(0,77,800,503);

    // 实例化全局线程池指针和最大大小（由于单个客户端没有服务器频繁，12个就可以了）
    m_threadPool = QThreadPool::globalInstance();
    m_threadPool->setMaxThreadCount(12);//限制最大值为12个

    m_counter = 0;// 初始化计数器

    // 连接子线程和主线程的信号和槽
    connect(this,&MyGraphicsScene::addNewItem,this,&MyGraphicsScene::onAddNewItem);
}

void MyGraphicsScene::setData(QColor c, int w, Controller::DrawType type)
{
    m_color = c;
    m_weight = w;
    m_drawType = type;
}

void MyGraphicsScene::drawNewEvent(QJsonObject jsonObj)
{
    // 由于绘制事件比较频繁，在子线程进行对绘制对象进行打包操作
    m_threadPool->start([this,jsonObj](){
        // 取出数据
        QString type = jsonObj["type"].toString();
        QString colorStr = jsonObj["color"].toString();
        QColor color(colorStr);
        int weight = jsonObj["weight"].toInt();
        qint64 number = jsonObj["number"].toVariant().toLongLong();
        QString account = jsonObj["account"].toString();
        QPointF startPoint(jsonObj["point1x"].toDouble(),jsonObj["point1y"].toDouble());
        QPointF endPoint(jsonObj["point2x"].toDouble(),jsonObj["point2y"].toDouble());

        // 根据类型进行绘制（子线程不能直接进行ui操作，因此发送回主线程进行添加）
        if(type == "line"){
            Line* line = new Line(color,weight,startPoint,endPoint);
            if(line){
                // 设置账号和编号值，方便删除比对（账号+编号可以唯一确定一条绘制）
                line->setNumber(number);
                line->setAccount(account);
                // 通过信号发送回主线程进行添加
                emit addNewItem(line);
            }

        }else if(type == "curve"){
            Curve* curve = new Curve(color,weight,startPoint,endPoint);//其实最后那个坐标没必要
            if(curve){
                QJsonArray pathArray = jsonObj["curvePoints"].toArray();
                curve->setPathByArray(pathArray);
                // 设置账号和编号值，方便删除比对（账号+编号可以唯一确定一条绘制）
                curve->setNumber(number);
                curve->setAccount(account);
                // 通过信号发送回主线程进行添加
                emit addNewItem(curve);
            }

        }else if(type == "ellipse"){
            Ellipse* ellipse = new Ellipse(color,weight,startPoint,endPoint);
            if(ellipse){
                // 设置账号和编号值，方便删除比对（账号+编号可以唯一确定一条绘制）
                ellipse->setNumber(number);
                ellipse->setAccount(account);
                // 通过信号发送回主线程进行添加
                emit addNewItem(ellipse);
            }

        }else if(type == "rectangle"){
            Rectangle* rectangle = new Rectangle(color,weight,startPoint,endPoint);
            if(rectangle){
                // 设置账号和编号值，方便删除比对（账号+编号可以唯一确定一条绘制）
                rectangle->setNumber(number);
                rectangle->setAccount(account);
                // 通过信号发送回主线程进行添加
                emit addNewItem(rectangle);
            }
        }
    });
}

void MyGraphicsScene::sendDrawEventSingal(QPointF pos)
{
    QJsonObject jsonObj;// 构建JSON对象
    jsonObj["tag"] = "draw";

    // 获取类型
    QString type;
    switch (m_drawType) {
    case Controller::LineType:
        type = "line";
        break;
    case Controller::CurveType:
        type = "curve";
        break;
    case Controller::EllipseType:
        type = "ellipse";
        break;
    case Controller::RectangleType:
        type = "rectangle";
        break;
    default:
        type = "unKnown";
        break;
    }
    jsonObj["type"] = type;

    // 获取颜色
    jsonObj["color"] = m_color.name(QColor::HexArgb);

    // 获取字号
    jsonObj["weight"] = m_weight;// 整形

    // 添加标识符
    jsonObj["number"] = QString::number(m_counter);
    jsonObj["account"] = "-1";// 自己绘制的默认是"-1"

    // 起始点坐标
    jsonObj["point1x"] = m_startPoint.x();// 双精度浮点型
    jsonObj["point1y"] = m_startPoint.y();// 双精度浮点型

    // 终点坐标
    QPointF endPoint = pos;// 获取终点坐标
    jsonObj["point2x"] = endPoint.x();// 双精度浮点型
    jsonObj["point2y"] = endPoint.y();// 双精度浮点型

    // 曲线的额外处理（路径数组）
    if(m_currentItem && m_drawType == Controller::CurveType){
        Curve* curve = qgraphicsitem_cast<Curve*>(m_currentItem);
        if(curve){
            // 获取json数组（包含了路径的所有点）
            QJsonArray curvePoints = curve->getCurvePoints();
            jsonObj["curvePoints"] = curvePoints;
        }
    }

    // 发送信号
    emit drawEvent(jsonObj);
}

void MyGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // 如果按下的是鼠标左键
    if(event->button() == Qt::LeftButton){
        emit needData();// 发送信号，更新颜色和字号信息
        m_isDrawing = true;// 标记为正在绘制
        m_startPoint = event->scenePos();// 记录第一次按下的点
        QString account = "-1";// 编号为默认值"-1"

        // 根据类型进行实例化对象
        switch (m_drawType)
        {
        case Controller::LineType:{
            m_currentItem = new Line(m_color, m_weight, m_startPoint, m_startPoint);
            m_currentItem->setNumber(m_counter);
            m_currentItem->setAccount(account);
            break;
        }
        case Controller::CurveType:{
            m_currentItem = new Curve(m_color,m_weight,m_startPoint,m_startPoint);
            m_currentItem->setNumber(m_counter);
            m_currentItem->setAccount(account);
            break;
        }
        case Controller::EllipseType:{
            m_currentItem = new Ellipse(m_color, m_weight, m_startPoint, m_startPoint);
            m_currentItem->setNumber(m_counter);
            m_currentItem->setAccount(account);
            break;
        }
        case Controller::RectangleType:{
            m_currentItem = new Rectangle(m_color, m_weight, m_startPoint, m_startPoint);
            m_currentItem->setNumber(m_counter);
            m_currentItem->setAccount(account);
            break;
        }
        default:
            break;
        }

        // 绘制项有效则添加到画布（会自动更新画布显示）
        if (m_currentItem) {
            this->addItem(m_currentItem);
        }
    }
}

void MyGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // 优先处理橡皮擦事件
    if(m_drawType == Controller::RubberType && (event->buttons() & Qt::LeftButton)){
        m_isDrawing = false;

        // 定义橡皮矩形(5X5)
        const qreal eraseSize = 5;
        QPointF pos = event->scenePos();
        QRectF eraseArea(pos.x() - eraseSize/2, pos.y() - eraseSize/2, eraseSize, eraseSize);

        // 遍历画布元素，获取与区域相交的项,将其删除
        QList<QGraphicsItem*> items = this->items(eraseArea, Qt::IntersectsItemShape);
        foreach (QGraphicsItem* item, items) {
            // 向主类发送某一项被删除的信号
            QJsonObject jsonObj;
            jsonObj["tag"] = "rubber";
            Element* element = qgraphicsitem_cast<Element*>(item);
            if(element){
                jsonObj["number"] = QString::number(element->getNumber());
                jsonObj["account"] = element->getAccount();//如果是自己的，则值为-1
                emit rubberItem(jsonObj);
            }else{
                qDebug()<<"当前删除的项无法转换成Element对象";
            }

            // 从自身画布中删除该项
            this->removeItem(item);
            delete item;
        }

        // 更新画布显示
        update();
        return;
    }

    // 如果不在绘制中或者没有有效的绘制项则返回
    if (!m_isDrawing || !m_currentItem) return;

    // 获取当前鼠标的坐标
    QPointF currentPos = event->scenePos();

    // 对于每一个类型分别处理
    switch (m_drawType)
    {
    case Controller::LineType:{
        Line *lineItem = qgraphicsitem_cast<Line*>(m_currentItem);
        if (lineItem) {
            lineItem->setEndPoint(currentPos);// 修改结束点，实现动态效果
            lineItem->update();
        }
        break;
    }
    case Controller::CurveType:{
        Curve* curveItem = qgraphicsitem_cast<Curve*>(m_currentItem);
        if(curveItem){
            curveItem->addPathPoint(currentPos);// 向路径添加点，实现动态效果
            curveItem->update();
        }
        break;
    }
    case Controller::EllipseType:{
        Ellipse *ellipseItem = qgraphicsitem_cast<Ellipse*>(m_currentItem);
        if (ellipseItem) {
            ellipseItem->setEndPoint(currentPos);// 修改结束点，实现动态效果
            ellipseItem->update();
        }
        break;
    }
    case Controller::RectangleType:{
        Rectangle *rectItem = qgraphicsitem_cast<Rectangle*>(m_currentItem);
        if (rectItem) {
            rectItem->setEndPoint(currentPos);// 修改结束点，实现动态效果
            rectItem->update();
        }
        break;
    }
    default:
        break;
    }

    // 更新画布显示
    update();
}

void MyGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // 如果正在绘制
    if(m_isDrawing){
        // 左键松开表示绘制完成，标记为结束
        m_isDrawing = false;

        //发送绘制信号（将绘制信息打包发给主类）
        sendDrawEventSingal(event->scenePos());

        // 每完成一次绘制，计数器（编号）自增
        m_counter++;

        // 如果绘制项有效，再进行一次更新显示
        if (m_currentItem) {
            m_currentItem->update();
            m_currentItem = nullptr;  // 结束绘制
        }

        // 更新画布显示
        update();
    }
}

void MyGraphicsScene::onAddNewItem(QGraphicsItem *item)
{
    // 如果子线程打包的绘制对象有效，则将其添加到画布上
    if(item){
        this->addItem(item);
    }
}
