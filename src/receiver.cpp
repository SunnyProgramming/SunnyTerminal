#include "receiver.h"
#include "Pty/ptylib.h"
#include <QTimer>
#include <QApplication>

Receiver::Receiver(QObject *parent) : QObject(parent) {
    QTimer::singleShot( 100, this, SLOT(Check()));
}

void Receiver::Check() {

    if( ptylib_initialized() ) {

        char *str = new char[1000];
        int rc = ptylib_recv( str, 1000 ); // this may block if select misdetects available data

        if( rc > 0 ) {
            emit UpdateCore( str, rc );
        } else {
            delete str;
        }

        if( rc == -3 ) {
            QApplication::exit();
        }
    }

    // TODO: rc == 0, rc < 0
    QTimer::singleShot(50, this, SLOT(Check()));
}

/*
void Receiver::Check() {

    char *str = new char[1000];

    UpdateCore(NULL,100);

    ptylib_recv_select();

    if( ptylib_recv_isset() ) {

        UpdateCore(NULL,200);

        int rc = ptylib_recv_read( str, 1000 ); // this may block if select misdetects available data

        UpdateCore(NULL,300);

        if( rc >= 0 ) {
            emit UpdateCore( str, rc );
        } else {
            emit UpdateCore( str, -1 );
        }

    } else {
        UpdateCore(NULL,400);
        emit UpdateCore( str, -2 );
    }

    UpdateCore(NULL,500);

    // TODO: rc == 0, rc < 0
    QTimer::singleShot(50, this, SLOT(Check()));
}
*/
