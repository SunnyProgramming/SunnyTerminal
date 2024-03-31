#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    bool mouse_init;
    bool mouse_visible;
    QTimer *timer;

public slots:
    void MouseHide();
    void MouseShow();

protected:
    void showEvent(QShowEvent *);

signals:
    void SendTerminal( QString );
    void TestTerminal( int );
    void ResizeTerminal( int, int );
    void UpdateClient( char *, int );

    void RepeatTest();

public slots:
    void ResizeWindow( int, int );

private slots:
    void on_actionExit_triggered();
    void on_pushButton_clicked();
    void on_actionTest0_triggered();
    void on_actionTest1_triggered();
    void on_actionTest2_triggered();
    void on_actionAltEnable_triggered();
    void on_actionAltDisable_triggered();

    void on_actionTest3_triggered();

    void on_actionTest4_triggered();

    void on_actionTest5_triggered();

    void on_actionTest6_triggered();

    void on_actionTest7_triggered();

    void on_actionTest8_triggered();

    void on_actionTest9_triggered();

    void on_actionCapEnable_triggered();

    void on_actionCapDisable_triggered();

    void on_actionRepeatTest_triggered();

    void on_actionResize_triggered();

private:
    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
