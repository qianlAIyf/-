#include "widget.h"
#include "ui_widget.h"
#include<QFileDialog>

#include "videotem.h"
#include<qmessagebox.h>
#include<QRect>
#include<QKeyEvent>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    player=new QMediaPlayer;
    videoWidget=new videoTem;
    ui->verticalLayout_2->addWidget(videoWidget);
    player->setVideoOutput(videoWidget);

    videoWidget->installEventFilter(this);
    connect(ui->startButton,SIGNAL(clicked()),this,SLOT(startVideo()));
    connect(ui->pauseButton,SIGNAL(clicked()),this,SLOT(pauseVideo()));
    connect(ui->closeButton,SIGNAL(clicked()),this,SLOT(closeVideo()));


    //创建两个套接字
    this->tcpSocket = new QTcpSocket(this);
    this->fileSocket = new QTcpSocket(this);

    this->initTCP();
    //显示当前路径
    //qDebug()<<QDir::currentPath();
        /////文件传送相关变量初始化
        ///每次发送数据大小为64kb
        perDataSize = 64*1024;
        totalBytes = 0;
        bytestoWrite = 0;
        bytesWritten = 0;
        bytesReceived = 0;
        filenameSize = 0;
        //按键使能
        this->ui->pushButton_disconnect->setEnabled(false);

}

Widget::~Widget()
{
    delete ui;
}

void Widget::initTCP()
{
    //连接
    connect(ui->pushButton_connect,SIGNAL(clicked()),this,SLOT(connectServer()));
    qDebug()<<"test";
    connect(ui->pushButton_send,SIGNAL(clicked()),this,SLOT(sendData()));

    connect(this->ui->pushButton_openFile,SIGNAL(clicked()),this,SLOT(selectFile()));
    connect(this->ui->pushButton_sendFile,SIGNAL(clicked()),this,SLOT(sendFile()));
}

void Widget::connectServer()
{
    tcpSocket->abort();
    fileSocket->abort();

    tcpSocket->connectToHost("127.1.1.0",8765);
    //连接服务器，发送文件
    fileSocket->connectToHost("127.1.10",8766);

//    tcpSocket->connectToHost("47.101.212.55",8765);
//    //连接服务器，发送文件
//    fileSocket->connectToHost("47.101.212.55",8766);

    //自动发送一个文件到服务器
//    this->filename = "readme.txt";
//    sendFile();
//    connect(fileSocket,SIGNAL(bytesWritten(qint64)),this,SLOT(updateFileProgress(qint64)));
    /////获取当前时间
    current_date_time = QDateTime::currentDateTime();
    str_date_time = current_date_time.toString("yyyy-MM-dd hh:mm:ss")+"\n";
    QString str = "连接成功！ "+str_date_time;
    this->ui->textEdit->append(str);



    connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(receiveData()));
    connect(fileSocket,SIGNAL(readyRead()),this,SLOT(updateFileProgress()));

    //更改键文字
    this->ui->pushButton_connect->setText("已连接");
    //按键使能
    this->ui->pushButton_connect->setEnabled(false);
    this->ui->pushButton_disconnect->setText("断开连接");
    this->ui->pushButton_disconnect->setEnabled(true);

}

void Widget::receiveData()
{
    //更改键文字
    this->ui->pushButton_disconnect->setText("断开连接");
    //按键使能
    this->ui->pushButton_disconnect->setEnabled(true);
    /////获取当前时间
    current_date_time = QDateTime::currentDateTime();
    str_date_time = current_date_time.toString("yyyy-MM-dd hh:mm:ss")+"\n";
    ////接收数据
    QString str = tcpSocket->readAll();
    ////显示
    str = "Server "+str_date_time+str;
    this->ui->textEdit->append(str);
}

void Widget::sendData()
{
    ////发送数据
    QString str = ui->lineEdit->text();

    this->tcpSocket->write(ui->lineEdit->text().toLocal8Bit());
    ////显示
    current_date_time = QDateTime::currentDateTime();
    str_date_time = current_date_time.toString("yyyy-MM-dd hh:mm:ss");
    str = "You "+str_date_time+"\n"+str;
    ui->textEdit->append(str);
}

void Widget::selectFile()
{

    ////文件传送进度更新
    connect(fileSocket,SIGNAL(bytesWritten(qint64)),this,SLOT(updateFileProgress(qint64)));
    connect(fileSocket,SIGNAL(readyRead()),this,SLOT(updateFileProgress()));

    this->ui->progressBar->setValue(0);
    this->filename = QFileDialog::getOpenFileName(this,"Open a file","../","files (*)");
    ui->lineEdit_filename->setText(filename);
}

void Widget::sendFile()
{
    this->localFile = new QFile(filename);
    if(!localFile->open(QFile::ReadOnly))
    {
        ui->textEdit->append(tr("Client:open file error!"));
        return;
    }
    ///获取文件大小
    this->totalBytes = localFile->size();
    QDataStream sendout(&outBlock,QIODevice::WriteOnly);
    sendout.setVersion(QDataStream::Qt_4_8);
    QString currentFileName = filename.right(filename.size()-filename.lastIndexOf('/')-1);

    qDebug()<<sizeof(currentFileName);
    ////保留总代大小信息空间、文件名大小信息空间、文件名
    sendout<<qint64(0)<<qint64(0)<<currentFileName;
    totalBytes += outBlock.size();
    sendout.device()->seek(0);
    sendout<<totalBytes<<qint64((outBlock.size()-sizeof(qint64)*2));

    bytestoWrite = totalBytes-fileSocket->write(outBlock);
    outBlock.resize(0);
}

void Widget::updateFileProgress(qint64 numBytes)
{
    ////已经发送的数据大小
    bytesWritten += (int)numBytes;

    ////如果已经发送了数据
    if(bytestoWrite > 0)
    {
        outBlock = localFile->read(qMin(bytestoWrite,perDataSize));
        ///发送完一次数据后还剩余数据的大小
        bytestoWrite -= ((int)fileSocket->write(outBlock));
        ///清空发送缓冲区
        outBlock.resize(0);
    }
    else
        localFile->close();

    ////更新进度条
    this->ui->progressBar->setMaximum(totalBytes);
    this->ui->progressBar->setValue(bytesWritten);

    ////如果发送完毕
    if(bytesWritten == totalBytes)
    {
        localFile->close();
        //fileSocket->close();
    }
}

void Widget::updateFileProgress()
{
    QDataStream inFile(this->fileSocket);
    inFile.setVersion(QDataStream::Qt_4_8);

    ///如果接收到的数据小于16个字节，保存到来的文件头结构
    if(bytesReceived <= sizeof(qint64)*2)
    {
        if((fileSocket->bytesAvailable()>=(sizeof(qint64))*2) && (filenameSize==0))
        {
            inFile>>totalBytes>>filenameSize;
            bytesReceived += sizeof(qint64)*2;
        }
        if((fileSocket->bytesAvailable()>=filenameSize) && (filenameSize != 0))
        {
            inFile>>filename;
            bytesReceived += filenameSize;
            filename = "../"+filename;
            localFile = new QFile(filename);
            if(!localFile->open(QFile::WriteOnly))
            {
                qDebug()<<"Server::open file error!";
                return;
            }
        }
        else
            return;
    }
    /////如果接收的数据小于总数据，则写入文件
    if(bytesReceived < totalBytes)
    {
        bytesReceived += fileSocket->bytesAvailable();
        inBlock = fileSocket->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }

    ////数据接收完成时
    if(bytesReceived == totalBytes)
    {
        this->ui->textEdit->append("Receive file successfully!");
        bytesReceived = 0;
        totalBytes = 0;
        filenameSize = 0;
        localFile->close();
        //fileSocket->close();
    }
}



void Widget::on_pushButton_disconnect_clicked()
{
    /////获取当前时间
    current_date_time = QDateTime::currentDateTime();
    str_date_time = current_date_time.toString("yyyy-MM-dd hh:mm:ss")+"\n";
    QString str = "已断开连接！ "+str_date_time;
    this->ui->textEdit->append(str);

    tcpSocket->abort();
    fileSocket->abort();

    this->ui->pushButton_disconnect->setText("已断开");
    this->ui->pushButton_disconnect->setEnabled(false);
    this->ui->pushButton_connect->setText("连接服务器");
    this->ui->pushButton_connect->setEnabled(true);
    this->initTCP();
}

void Widget::startVideo()
{
    player->setMedia(QUrl::fromLocalFile("../"+ui->lineEdit->text()));
    videoWidget->show();
    player->play();
}

void Widget::pauseVideo()
{
    player->pause();
}

void Widget::closeVideo()
{
    player->stop();
}


void Widget::on_pushButton_clicked()
{
    videoWidget->setWindowFlags(Qt::Window);
    videoWidget->showFullScreen();
}
