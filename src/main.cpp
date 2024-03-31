#include "mainwindow.h"
#include <QApplication>
#include "Pty/ptylib.h"

int main(int argc, char *argv[]) {

    //ptylib_init();

    QApplication a(argc, argv);
    MainWindow w;

#ifndef QT_DEBUG
    w.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#endif

    QPalette pal = QPalette();

    // set black background
    // Qt::black / "#000000" / "black"
    //pal.setColor(QPalette::Window, Qt::black);
    pal.setColor(QPalette::Window, Qt::darkGray);
    //w.setAutoFillBackground(true);
    w.setPalette(pal);

    //QRect screenGeometry = QApplication::desktop()->screenGeometry();
    //int x = (screenGeometry.width()  - w.width()) / 2;
    //int y = (screenGeometry.height() - w.height()) / 2;
    //w.move(x, y);

    w.move(0,0);
    w.show();

    return a.exec();
}
