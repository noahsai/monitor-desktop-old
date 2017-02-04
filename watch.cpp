#include "watch.h"
#include "ui_watch.h"

watch::watch(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::watch)
{
    ui->setupUi(this);
    this->setGeometry(QApplication::desktop()->width()-120,QApplication::desktop()->height()-120,this->sizeHint().width(),this->sizeHint().height());
   // move();
    setLayout(ui->horizontalLayout);
    setWindowFlags(Qt::FramelessWindowHint //去边框
           |Qt::WindowStaysOnTopHint //最ding层显示           |Qt::Tool //不在任务栏显示
         );
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    tuichu  = new QAction("退出", this);
    moniter = new QAction("系统监视器",this);
    connect(tuichu, SIGNAL(triggered(bool)), qApp, SLOT(quit()));//若触发了退出就退出程序
    connect(moniter,SIGNAL(triggered(bool)),this,SLOT(openmoniter()));
//    trayIcon = new QSystemTrayIcon(this);
//    trayIcon->setIcon(QIcon(":/icon.png"));
//    connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(trayiconactive()));
//    creattrayicon();
//    trayIcon->show();
    mousepressed=false;
    oldpos.setX(0);
    oldpos.setY(0);
    oup=0;
    odown=0;
    pro = new QProcess(this);
    timer = new QTimer(this);
    timer->setInterval(1000);
    timer->setSingleShot(false);
    connect(timer,SIGNAL(timeout()),this,SLOT(timeout()));
    timer->start();
}

watch::~watch()
{
    delete ui;
}

void watch::timeout()
{
   // QRegularExpressionMatch match;
    QRegularExpressionMatchIterator matchs;
    QString result,tmp;
    qint64 up,down,t;
    up=0;
    down=0;
    pro->start("ip -s link");
    if(pro->waitForStarted(2000))
    {
        if(pro->waitForFinished(1000))
        {
            result=pro->readAllStandardOutput();
          //  qDebug()<<result;
            if(!result.isEmpty())
            {
                result.remove(QRegularExpression("link/(?!loopback|ether)[\\S\\s]+?collsns[\\S\\s]+?(\\d+ +){6}"));
                reg.setPattern("collsns[\\s\\S]+?\\d+");
                matchs=reg.globalMatch(result);
                while(matchs.hasNext())
                {
                    tmp=matchs.next().captured(0);
                    tmp.remove(QRegularExpression("[\\s\\S]+ "));
                    t=tmp.toInt();
                    up+=t;
                }
               // qDebug()<<"got up-data"<<up<<"oup"<<oup;
                QString text;
                t=up-oup;
                t=t/1024;
                if(t>999)
                {
                    t=t*100/1024;
                    float f=t*1.0/100;
                    text="↑"+QString().setNum(f)+"M";

                }
                else if(t>0) text+="↑"+QString().setNum(t)+"K";
                else text="↑"+QString().setNum(up-oup)+"B";

                if(oup!=0) ui->up->setText(text);
                oup=up;

                reg.setPattern("mcast[\\s\\S]+?\\d+");
                matchs=reg.globalMatch(result);
                while(matchs.hasNext())
                {
                    tmp=matchs.next().captured(0);
                    tmp.remove(QRegularExpression("[\\s\\S]+ "));
                    t=tmp.toLongLong();
                    down+=t;
                   // qDebug()<<"got down"<<tmp;
                }
               // qDebug()<<"got down-data"<<down<<"odown"<<odown;
                t=down-odown;
                t=t/1024;
                if(t>999)
                {
                    t=t*100/1024;
                    float f=t*1.0/100;
                    text="↓"+QString().setNum(f)+"M";

                }
                else if(t>0) text="↓"+QString().setNum(t)+"K";
                else text="↓"+QString().setNum(down-odown)+"B";

                if(odown!=0) ui->down->setText(text);
                odown=down;
            }
        }
    }
    pro->start("free -m");
    if(pro->waitForStarted(2000))
    {
        if(pro->waitForFinished(1000))
        {
            result=pro->readAllStandardOutput();
          //  qDebug()<<result;
            if(!result.isEmpty())
            {
                qint64 total,used;
                int omem=ui->mem->value();//先获取之前的数
                int per=0;
                total=0;
                used=0;
                reg.setPattern("\\d+.+");
                //reg.setPattern("Mem:.+");
                tmp=reg.match(result).captured(0);
                qDebug()<<tmp;
                QStringList list=tmp.split(QRegularExpression(" +"));
                result=list.at(0);//total
                tmp=list.at(5);//available
                total=result.toLongLong();
                used=tmp.toLongLong();
                per=used*1000/total;
                per=1000-per;
                int m=per%10;
                per/=10;
                if(m>=5) per+=1;//四舍五入

                ui->mem->setValue(per);//再设置现在的数值

                //============================【内存占用颜色在这改】================================
                //===========这个顺序必须时从大到小判断，否则，65的判断会截或>80的数据
                if(per>=80)//内存大时的颜色！默认红色
               {
                   if(omem>=80) return;
                   QString style;
                   style="QProgressBar {\
                          background-color:#AC0000;\
                          }\
                          QProgressBar::chunk {\
                           background-color: #FF0000;\
                          }";
                   ui->mem->setStyleSheet(style);

               }
                else if(per>=65&&(omem>=80||omem<65))//内存占用较多时的颜色！默认橙色
                {

                    QString style;
                    style="QProgressBar {\
                           background-color:#D06000;\
                           }\
                           QProgressBar::chunk {\
                            background-color: #FF9224;\
                           }";
                    ui->mem->setStyleSheet(style);
                }
                else if(per<65)//内存降回正常状态时的颜色！默认蓝色
                {
                    if(omem<65) return;
                    QString style;
                    style="QProgressBar {\
                            background-color:rgb(40, 130, 160) ;\
                            }\
                            QProgressBar::chunk {\
                             background-color:  rgb(74, 183, 255);\
                            }";
                    ui->mem->setStyleSheet(style);
                }
               // qDebug()<<total<<used<<used*1.0/total;
            }
        }
    }
}

void watch::mousePressEvent(QMouseEvent* event)
{
    if(event->button()==Qt::LeftButton && !mousepressed)
    {
        mousepressed=true;
        oldpos=event->globalPos()-this->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void watch::mouseMoveEvent(QMouseEvent * event){
    if(mousepressed)
    {
        move(event->globalPos()-oldpos);//貌似linux要这样
        event->accept();
    }
}

void watch::mouseReleaseEvent(QMouseEvent * event){
    if(event->button()==Qt::LeftButton && mousepressed)
    {
        int x=this->x();
        int y=this->y();
//        qDebug()<<x<<endl<<y;

        if(this->pos().x()<0) x=0;
        else if(QApplication::desktop()->width()-x<this->width()){
            x=QApplication::desktop()->width()-this->width();
        }

        if(this->pos().y()<0) y=0;
        else if(QApplication::desktop()->height()-y<this->height())
        {
            y=QApplication::desktop()->height()-this->height();
        }
//        qDebug()<<x<<endl<<y;
        move(x,y);

        mousepressed=false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
}

//void watch::creattrayicon()
//{
//    QAction *qu  = new QAction(tr("退出"), this);
//    connect(qu, SIGNAL(triggered()), qApp, SLOT(quit()));//若触发了退出就退出程序
//    QMenu* trayIconMenu = new QMenu(this);//菜单
//    trayIconMenu->addAction(qu);//把退出加到入菜单项
//    trayIcon->setContextMenu(trayIconMenu);//设置托盘上下文菜单为trayIconMenu

//}
void watch::contextMenuEvent(QContextMenuEvent *) //右键菜单项编辑
{
QCursor cur=this->cursor();
QMenu *menu=new QMenu(this);
menu->addAction(moniter);//添加系统监视器
menu->addAction(tuichu); //添加退出
menu->exec(cur.pos()); //关联到光标
}

void watch::openmoniter()
{
    QProcess process;
    process.startDetached("gnome-system-monitor");
}
