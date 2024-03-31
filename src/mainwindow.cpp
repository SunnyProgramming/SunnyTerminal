#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

    this->setMouseTracking(true);
    ui->setupUi(this);

    timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(5000);

#ifndef QT_DEBUG
    ui->debugFrame->setVisible(false);
    ui->menuBar->setVisible(false);
#endif

    this->mouse_init = false;
    this->mouse_visible = true;
    this->MouseHide();

    connect( this, SIGNAL(SendTerminal(QString)), ui->terminal, SLOT(SendTerminal(QString)));
    connect( this, SIGNAL(TestTerminal(int)), ui->terminal, SLOT(TestTerminal(int)));
    connect( this, SIGNAL(RepeatTest()), ui->terminal, SLOT(RepeatTest()));
    connect( ui->terminal, SIGNAL(LogMsg(QString)), ui->textEdit, SLOT(append(QString)));
    connect( ui->terminal, SIGNAL(LogErr(QString)), ui->textEdit_2, SLOT(insertPlainText(QString)));
    connect( ui->terminal, SIGNAL(ChangeTitle(QString)), ui->title, SLOT(ChangeTitle(QString)));
    connect( ui->terminal, SIGNAL(ResizeWindow(int,int)), this, SLOT(ResizeWindow(int,int)));

    connect( ui->terminal, SIGNAL(MouseShow()), this, SLOT(MouseShow()));
    connect( ui->title   , SIGNAL(MouseShow()), this, SLOT(MouseShow()));
    connect( timer, SIGNAL(timeout()), this, SLOT(MouseHide()));

}

MainWindow::~MainWindow() {
    delete timer;
    delete ui;
}

void MainWindow::MouseHide() {
    if( mouse_visible ) {
        qDebug("Mouse Hide");
        QCursor cursor(Qt::BlankCursor);
        QApplication::setOverrideCursor(cursor);
        QApplication::changeOverrideCursor(cursor);
        mouse_visible = false;
    }
}

void MainWindow::MouseShow() {

    if( !mouse_init ) {
        mouse_init = true; // needed to prevent MouseShow on load
        return;
    }

    if( !mouse_visible ) {
        qDebug("Mouse Show");
        QCursor cursor(Qt::ArrowCursor);
        QApplication::setOverrideCursor(cursor);
        QApplication::changeOverrideCursor(cursor);
        mouse_visible = true;
        timer->start();
    }
}

void MainWindow::showEvent(QShowEvent *e) {
    QMainWindow::showEvent(e);
    ui->terminal->setFocus();
    ui->terminal->Initialize();
}

void MainWindow::ResizeWindow(int w, int h ) {
    this->setGeometry(0,0,w,h);
}

void MainWindow::on_actionExit_triggered() {
    QApplication::exit();
}

void MainWindow::on_pushButton_clicked() {

    QString s;
    s = ui->lineEdit->text() + "\n";
    emit SendTerminal(s);
    ui->lineEdit->clear();
}

void MainWindow::on_actionTest0_triggered() {
    emit TestTerminal(0);
}

void MainWindow::on_actionTest1_triggered() {
    emit TestTerminal(1);
}

void MainWindow::on_actionTest2_triggered() {
    emit TestTerminal(2);
}

void MainWindow::on_actionTest3_triggered() {
    emit TestTerminal(3);
}

void MainWindow::on_actionTest4_triggered() {
    emit TestTerminal(4);
}

void MainWindow::on_actionTest5_triggered() {
    emit TestTerminal(5);
}

void MainWindow::on_actionTest6_triggered() {
    emit TestTerminal(6);
}

void MainWindow::on_actionTest7_triggered() {
    emit TestTerminal(7);
}

void MainWindow::on_actionTest8_triggered() {
    emit TestTerminal(8);
}

void MainWindow::on_actionTest9_triggered() {
    emit TestTerminal(9);
}

void MainWindow::on_actionAltEnable_triggered() {
    emit TestTerminal(100);
}

void MainWindow::on_actionAltDisable_triggered() {
    emit TestTerminal(101);
}


void MainWindow::on_actionCapEnable_triggered() {
    emit TestTerminal(102);
}

void MainWindow::on_actionCapDisable_triggered() {
    emit TestTerminal(103);
}

void MainWindow::on_actionRepeatTest_triggered() {
    emit RepeatTest();
}

void MainWindow::on_actionResize_triggered() {
    // Nothing for now
}
