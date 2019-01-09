#ifndef VIDEOTEM_H
#define VIDEOTEM_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include<QVideoWidget>

class videoTem :public QVideoWidget
{
public:
    videoTem();
    void exitFull();
    void keyPressEvent(QKeyEvent *event);
};

#endif // VIDEOTEM_H
