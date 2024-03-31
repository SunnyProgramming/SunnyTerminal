#include "terminal.h"
#include "Pty/ptylib.h"
#include <QDebug>
#include <QPainter>
#include <QApplication>

#include <QCoreApplication>
#include <QFontDatabase>

#include <QDesktopWidget>

Terminal::Terminal(QWidget *parent) : QWidget(parent) {

    this->setMouseTracking(true);

    receiver = new Receiver();
    core = new Core();
    core->terminal = this;

    connect( this, SIGNAL(SendCore(QString)), core, SLOT(SendCore(QString)));
    connect( this, SIGNAL(TestCore(int)), core, SLOT(TestCore(int)));
    connect( receiver, SIGNAL(UpdateCore(char *, int )), core, SLOT(UpdateCore(char *, int )));
    connect( core, SIGNAL(UpdateTerminal()), this, SLOT(UpdateTerminal()));
    connect( core, SIGNAL(LogMsg(QString)), this, SLOT(RecvLogMsg(QString)));
    connect( core, SIGNAL(LogErr(QString)), this, SLOT(RecvLogErr(QString)));

    thread = new QThread();
    connect( qApp, SIGNAL(aboutToQuit()), thread, SLOT(quit()) );
    connect( thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    receiver->moveToThread(thread);
    thread->start();
    // core may also need to move to a thread. In that case, access to core internal needs to be considered

    QFontDatabase::addApplicationFont( QCoreApplication::applicationDirPath() + "/fonts/HackGen-Regular.ttf" );
    QFontDatabase::addApplicationFont( QCoreApplication::applicationDirPath() + "/fonts/HackGen-Bold.ttf" );

    QFont font = QFont("HackGen", 18 );
    this->setFont( font );

    met = new QFontMetrics( font );
    w = met->width("a");
    h = met->height();
    hx = h / 4;
    draw_fullwidth = false;
    this->setMinimumWidth ( w * core->NCOL );
    this->setMinimumHeight( h * core->NROW + hx );

    repeat_test = false;

}

void Terminal::Initialize() {

    // for width, need to take into account wide char is needs to be even number
    // need to take into account the title height which is 1 char high, and also the bottom of characters 1/4 char high

    QRect sg = QApplication::desktop()->availableGeometry();

#ifndef QT_DEBUG
    int max_cols = ( sg.width() / w );
#else
    int max_cols = (( sg.width() - 400 ) / w );
#endif

    int max_rows = sg.height() / h - 1;
    if( max_cols & 0x1 ) max_cols -= 1;

    this->setMinimumWidth ( w * max_cols );
    this->setMinimumHeight( h * max_rows + hx );

    this->core->NCOL = max_cols;
    this->core->NROW = max_rows;

    emit ResizeWindow( sg.width(), sg.height() );

    //ChangeTitle( QString("sw = %1 sh = %2 max_rows = %3 max_cols = %4").arg( sg.width() ).arg( sg.height() ).arg(max_rows).arg(max_cols));

    ptylib_init(max_cols, max_rows);

}

//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------

bool Terminal::isFullWidth(uint c ) {

    // add some faster logic
    // https://en.wikipedia.org/wiki/List_of_Unicode_characters
    if( c <  0x20 ) {
        qDebug("unhandled control characters detected %02x", c );
        return(false);
    }

    if( c >= 0x20 && c <= 0x7E ) return(false);

    //QString s = QString::fromUcs4( &c, 1 );
    //LogMsg("Checking Full Width char: " + s );

    // inFontUcs4 does not work under cross compilation...
    //if( met->inFontUcs4( c ) == false ) return(false);
    //LogMsg("Found inFont");

    if( met->width( QString::fromUcs4( &c, 1 ) ) <= w ) return(false);

    return( true );
}

void Terminal::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(Qt::black));
    painter.setPen(QPen(Qt::black));
    //painter.drawRect( 0, 0, w * NCOL, h * NROW + h / 4 );
    //painter.setPen(QPen(Qt::white));

    uint col, row;
    uint64_t d;

    for( row = 0; row < core->NROW; row++ ) {

        int y  = ( row == 0 ) ? 0 : h * row + hx;
        int hh = ( row == 0 ) ? h + hx : h;

        for( col = 0; col < core->NCOL; col++ ) {

            d = core->GetCode( row, col );

            if( d == FLAG_WIDECHAR ) {
                draw_fullwidth = true;
            } else {

                QBrush brush = painter.brush();

                uint r, g, b;
                if( d & FLAG_REVERSE ) {
                    r = (( d >> BITS_FG_R ) & 0x3F ) << 2;
                    g = (( d >> BITS_FG_G ) & 0x3F ) << 2;
                    b = (( d >> BITS_FG_B ) & 0x3F ) << 2;
                } else {
                    r = (( d >> BITS_BG_R ) & 0x3F ) << 2;
                    g = (( d >> BITS_BG_G ) & 0x3F ) << 2;
                    b = (( d >> BITS_BG_B ) & 0x3F ) << 2;
                }

                brush.setColor( QColor::fromRgb( r, g, b ) );
                painter.setBrush(brush);

                if( draw_fullwidth ) {
                    painter.fillRect( w * ( col - 1 ), y, 2 * w, hh, painter.brush() );
                    draw_fullwidth = false;
                } else {
                    painter.fillRect( w * col, y, w, hh, painter.brush() );
                }
            }
        }
    }

    for( row = 0; row < core->NROW; row++ ) {
        for( col = 0; col < core->NCOL; col++ ) {

            d = core->GetCode( row, col );

            if( d == FLAG_WIDECHAR ) {
                draw_fullwidth = true;
            } else {

                QFont font = painter.font();
                if( d & FLAG_BOLD      ) font.setBold(true);      else font.setBold(false);
                if( d & FLAG_ITALIC    ) font.setItalic(true);    else font.setItalic(false);
                if( d & FLAG_UNDERLINE ) font.setUnderline(true); else font.setUnderline(false);
                painter.setFont(font);

                QPen pen = painter.pen();

                uint r, g, b;
                if( d & FLAG_REVERSE ) {
                    r = (( d >> BITS_BG_R ) & 0x3F ) << 2;
                    g = (( d >> BITS_BG_G ) & 0x3F ) << 2;
                    b = (( d >> BITS_BG_B ) & 0x3F ) << 2;
                } else {
                    r = (( d >> BITS_FG_R ) & 0x3F ) << 2;
                    g = (( d >> BITS_FG_G ) & 0x3F ) << 2;
                    b = (( d >> BITS_FG_B ) & 0x3F ) << 2;
                }

                pen.setColor( QColor::fromRgb( r, g, b ) );
                painter.setPen(pen);

                uint c = d & FLAG_UNICODE;

                if( draw_fullwidth ) {
                    painter.drawText( w * ( col - 1 ), h*( row + 1 ), QString::fromUcs4( &c, 1 ) );
                    draw_fullwidth = false;
                } else {
                    painter.drawText( w * col, h*( row + 1 ), QString::fromUcs4( &c, 1 ) );
                }
            }
        }
    }

    // Show current position of the cursor with a rectangle outline for now
    painter.setBrush(QBrush(Qt::NoBrush));
    painter.setPen(QPen(Qt::white));
    col = core->cx;
    row = core->cy;
    painter.drawRect( w * col, h*row + hx, w, h );

}

void Terminal::keyPressEvent(QKeyEvent *e) {

    //uint c;

    int key = e->key();

    switch ( key ) {

        case Qt::Key_Escape:
        emit RecvLogMsg("Escape Detected");
        //emit SendCore( "\u001B" );
        //c = 0x1B;
        //emit SendCore( QString::fromUcs4(&c,1) );
        emit SendCore( "\e" );
        break;

        case Qt::Key_Backspace:
        emit RecvLogMsg("Backspace Detected");
        emit SendCore( "\b" );
		break;

        case Qt::Key_Return:
        emit RecvLogMsg("Return Detected");
        emit SendCore( "\r" );
		break;

        case Qt::Key_Tab:
        emit RecvLogMsg("Tab Detected");
        emit SendCore( "\t" );
        break;

        case Qt::Key_Up:
        emit RecvLogMsg("Up Detected");
        if( core->app_cursor ) emit SendCore("\eOA");
        else                   emit SendCore("\e[A");
        break;

        case Qt::Key_Down:
        emit RecvLogMsg("Down Detected");
        if( core->app_cursor ) emit SendCore("\eOB");
        else                   emit SendCore("\e[B");
        break;

        case Qt::Key_Left:
        emit RecvLogMsg("Left Detected");
        if( core->app_cursor ) emit SendCore("\eOD");
        else                   emit SendCore("\e[D");
        break;

        case Qt::Key_Right:
        emit RecvLogMsg("Right Detected");
        if( core->app_cursor ) emit SendCore("\eOC");
        else                   emit SendCore("\e[C");
        break;

        case Qt::Key_Delete:
        emit RecvLogMsg("Delete Detected");
        emit SendCore( "\e[3~" );
        break;

        case Qt::Key_F11:
        case Qt::Key_PageUp:
        core->ScrollUp();
        emit RecvLogMsg("PageUp Detected");
        break;

        case Qt::Key_F12:
        case Qt::Key_PageDown:
        core->ScrollDown();
        emit RecvLogMsg("PageDown Detected");
        break;

        case Qt::Key_F9:
        case Qt::Key_Home:
        core->ScrollHome();
        emit RecvLogMsg("HomeKey Detected");
        break;

        case Qt::Key_F10:
        case Qt::Key_End:
        core->ScrollEnd();
        emit RecvLogMsg("EndKey Detected");
        break;

        case Qt::Key_Print: // Most likely PrnScreen but this get captured by OS...
        case Qt::Key_F8:
        core->ScrollSave();
        emit RecvLogMsg("Print Detected");
        break;

        //case Qt::Key_F1:
        //ptylib_init();
        //break;

        case Qt::Key_F7:
        QApplication::exit();
        break;

	default: 

        if( e->modifiers() & Qt::AltModifier ) {
            if( key == Qt::Key_F4 ) {
                QApplication::exit();
                break;
            }
        }

        if( e->modifiers() & Qt::ControlModifier ) {
            if(( key >= 0x40 ) && ( key <= 0x5f ) ) {
                uint c = key - 0x40;
                emit SendCore( QString::fromUcs4(&c,1) );
                break;
            }
        }

        if(( key >= 0x20 ) && ( key <= 0x7E)) {
            //emit RecvLogMsg( e->text() );
            emit SendCore( e->text() );
        }
        else emit RecvLogErr( QString("Unhandled key: %1").arg( key ) );
	    break;
    }

}

void Terminal::mousePressEvent(QMouseEvent *) {
    this->setFocus();
}

void Terminal::mouseMoveEvent(QMouseEvent *) {
    emit MouseShow();
}

bool Terminal::focusNextPrevChild(bool next) {
    Q_UNUSED(next);
    return(false);
}

void Terminal::UpdateTerminal() {
    this->repeat_ok = true;
    this->repaint();
    // Update display buffer...
}


void Terminal::SendTerminal( QString s ) {
    emit SendCore( s );
}

void Terminal::TestTerminal( int n ) {
    emit TestCore(n);
}

void Terminal::RecvLogMsg(QString s) {
    emit LogMsg( s );
}

void Terminal::RecvLogErr(QString s) {

    emit LogErr( s );

    QString errMsg;
    errMsg = "<span style=\" color:#ff0000;\" >";
    errMsg += s;
    errMsg += ("</span>");
    //self.myTextEdit.write(errMsg)
    emit LogMsg( errMsg );
}

//--------------------------------------------------------

#include <QTimer>

void Terminal::RepeatTest() {
    this->repeat_test = true;
    this->repeat_ok = false;
    this->repeat_count = 0;
    SendCore("test 0\n");
    QTimer::singleShot(100, this, SLOT(RepeatCheck()) );
}

void Terminal::RepeatCheck() {
    this->repeat_count++;
    if( repeat_ok ) {
        //emit LogErr("repeat_ok\n");
        if( repeat_count < 10 ) {
            SendCore(QString("test %1\n").arg(repeat_count));
            QTimer::singleShot(100, this, SLOT(RepeatCheck()) );
        }
    } else {
        emit LogErr("repeat_ng\n");
    }
}
