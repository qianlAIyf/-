#include "videotem.h"
#include"QMessageBox"
#include<QKeyEvent>
videoTem::videoTem()
{

}

void videoTem::exitFull()
{
    this->setWindowFlags(Qt::SubWindow);
    this->showNormal();
     //QMessageBox::information(this,tr("2222222"),tr("22222222"));
}

void videoTem::keyPressEvent(QKeyEvent *event){
    if(event->key() == Qt::Key_Escape){
        exitFull();
            //QMessageBox::information(this,tr("111111"),tr("1111"));

    }
}
