#ifndef TERMINAL_H
#define TERMINAL_H

#include <QWidget>
#include <QString>
#include <QKeyEvent>
#include <QThread>
#include <core.h>
#include <receiver.h>
#include <QPen>

class Terminal : public QWidget {

    Q_OBJECT

public:
    explicit Terminal(QWidget *parent = 0);
    bool isFullWidth( uint );
    bool repeat_test;
    bool repeat_ok;
    int  repeat_count;

protected:
    void paintEvent    ( QPaintEvent * );
    void keyPressEvent ( QKeyEvent   * );
    void mousePressEvent ( QMouseEvent * );
    void mouseMoveEvent(QMouseEvent *);
    bool focusNextPrevChild( bool );

private:
    Core     *core;
    Receiver *receiver;
    QThread  *thread;

    QFontMetrics *met;
    int w;
    int h, hx;
    bool draw_fullwidth;

signals:
    void SendCore(QString);
    void TestCore(int n);
    void LogMsg(QString);
    void LogErr(QString);
    void ChangeTitle(QString);
    void ResizeWindow( int, int );
    void MouseShow();

public slots:
    void Initialize();
    void SendTerminal(QString);
    void UpdateTerminal();
    void TestTerminal(int);
    void RecvLogMsg(QString);
    void RecvLogErr(QString);
    void RepeatTest();
    void RepeatCheck();

};

#endif // TERMINAL_H
