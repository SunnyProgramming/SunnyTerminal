#include "title.h"
#include <QPainter>
#include <QMouseEvent>

#include <QCoreApplication>
#include <QFontDatabase>

Title::Title(QWidget *parent) : QWidget(parent) {

    this->setMouseTracking(true);

    QFont font = QFont("HackGen", 14 );
    this->setFont( font );

    QFontMetrics met(font);
    w = met.width("x");
    h = met.height();

    this->setMinimumHeight( h + 2 );
    offset = h - h / 4;

    s.clear();
    s.append( APPNAME );

}

void Title::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e);
    qDebug("Title paint event\n");
    QPainter painter(this);

    //QLinearGradient m_gradient(0,0,width(),0);
    //m_gradient.setColorAt(0.0, Qt::blue);
    //m_gradient.setColorAt(1.0, Qt::red);
    //painter.fillRect( 0, 0, width(), height(), m_gradient );

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(Qt::black));
    painter.setPen(QPen(Qt::white, 2 ) );

    painter.fillRect( 0, 0, width(), height(), painter.brush() );
    painter.drawText( w * 0.5, offset, s );
    painter.drawLine( 0, height(), width(), height() );

    painter.drawText( width() - w * 2.5, offset + h / 8, QString("✕") ); // needed less offset to center x

}

void Title::mousePressEvent(QMouseEvent * e) {
    sp = e->pos();
}

void Title::mouseReleaseEvent(QMouseEvent *e) {

    QPoint point = e->pos();

    int x = point.x();
    if(( x > this->width() - w * 3.0 )  && ( x < this->width() )) { // Close Application
        QCoreApplication::quit();
    }

    QWidget * w = window();
    if( w ) w->move( w->pos() + ( point - sp  ));

}

void Title::mouseMoveEvent(QMouseEvent *event) {
    emit MouseShow();
}

void Title::ChangeTitle(QString str ) {
    s.clear();
    s.append( str );
}
