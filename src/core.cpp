#include "core.h"
#include <QDebug>
#include "Pty/ptylib.h"
#include <terminal.h>
#include <QString>
#include <QStringList>

Core::Core(QObject *parent) : QObject(parent) {

    this->NCOL = DEF_NCOL;
    this->NROW = DEF_NROW;

    this->alt_mode = false;
    this->echo = false;
    this->capture = false;

    this->scroll_mode = false;
    this->scroll_next = 0;
    this->scroll_last = 0;
    this->scroll_user = 0;

    this->app_cursor = false;

    this->cx = 0;
    this->cy = 0;
    this->esc = false;
    this->csi_mode = false;
    this->sss_mode = 0;
    this->attr = FLAG_DEFAULT;
    this->crem = 0;

    uint row, col;
    for( row = 0; row < MAX_NROW; row++ ) {
        for( col = 0; col < MAX_NCOL; col++ ) {
            tbuf[row][col] = FLAG_DEFAULT;
        }
    }

}

uint64_t Core::GetCode( uint row, uint col ) {

    if(!scroll_mode) return( this->tbuf[row][col]);

    uint k = scroll_user + row;

    if( k < scroll_next ) return( this->scroll_tbuf[ k % SCROLL_MAX ][col] );

    k = row - ( scroll_next - scroll_user );

    return( this->tbuf[k][col]);

}

bool Core::ScrollUp() {
    if( alt_mode ) return(false);
    if( scroll_next == 0 ) return(false); // Nothing to scroll...

    if( scroll_mode == false ) {
        emit LogMsg("ScrollEnter");
        scroll_user = scroll_next;
    }

    if( scroll_user > scroll_last ) scroll_user = scroll_user - 1;

    scroll_mode = true;
    emit UpdateTerminal();
    return(true);
}

bool Core::ScrollDown() {
    if( alt_mode ) return(false);
    if(!scroll_mode ) return(false);

    scroll_user = scroll_user + 1;

    if( scroll_user == scroll_next ) {
        return ScrollEnd();
    }
    emit UpdateTerminal();
    return(true);
}

bool Core::ScrollHome() {
    if( alt_mode ) return(false);
    if( scroll_next == 0 ) return(false); // Nothing in scroll_buffer
    emit LogMsg("ScrollHome");

    scroll_user = scroll_last;
    scroll_mode = true;

    emit LogMsg( QString("scroll_next = %1").arg( scroll_next ) );
    emit LogMsg( QString("scroll_user = %1").arg( scroll_user ) );
    emit LogMsg( QString("scroll_last = %1").arg( scroll_last ) );

    emit UpdateTerminal();
    return(true);
}

bool Core::ScrollEnd() {
    if( scroll_mode ) {
        emit LogMsg("ScrollExit");
        scroll_mode = false;
        emit UpdateTerminal();
        return(true);
    }
    return(false);
}

void Core::ScrollAppend() {

    uint col;

    //emit LogMsg("ScrollAppend");

    uint row = scroll_next % SCROLL_MAX;
    for( col = 0; col < NCOL; col++ ) {
        scroll_tbuf[row][col] = tbuf[0][col];
    }

    scroll_next++;
    if( scroll_next >= SCROLL_MAX ) scroll_last++;

}

void Core::ScrollAppendScreen() {

    uint col;

    for( uint offset = 0; offset < NROW; offset++ ) {

        uint row = scroll_next % SCROLL_MAX;
        for( col = 0; col < NCOL; col++ ) {
            scroll_tbuf[row][col] = tbuf[offset][col];
        }

        scroll_next++;
        if( scroll_next >= SCROLL_MAX ) scroll_last++;

    }
}

bool Core::ScrollSave() {
    qDebug("Scroll Save");

    char str[200];

    QFile file("scrlog.txt");
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text) ) {
        qDebug("Failed to open file");
        return(false);
    }

    QTextStream stream( &file );

    for( uint k = scroll_last; k < scroll_next; k++ ) {

        for( uint col = 0; col < NCOL; col++ ) {
            str[col] = (char)(this->scroll_tbuf[ k % SCROLL_MAX ][col] & 0xFF );
            if( str[col] == '\0' ) str[col] = ' ';
        }
        str[NCOL] = '\0';

        QString s = QString::fromLocal8Bit( str );
        stream << s << endl;
    }

    for( uint k = 0; k < NROW; k++ ) {

        for( uint col = 0; col < NCOL; col++ ) {
            str[col] = (char)(this->tbuf[k][col] & 0xFF);
            if( str[col] == '\0' ) str[col] = ' ';
        }
        str[NCOL] = '\0';

        QString s = QString::fromLocal8Bit( str );
        stream << s << endl;
    }

    file.close();


    return(true);
}


//---------------------------------------------------------------------------------

void Core::ClearScreen() {

    emit LogMsg("ClearScreen");

    if(!alt_mode) ScrollAppendScreen();

    uint row, col;
    for( row = 0; row < NROW; row++ ) {
        for( col = 0; col < NCOL; col++ ) {
            tbuf[row][col] = FLAG_DEFAULT;
        }
    }

    cx = 0;
    cy = 0;
    esc = false;

    csi_mode = false;
    sss_mode = 0;

    attr = FLAG_DEFAULT;
    crem = 0;

}

void Core::ClearScreenToCursor() {

    emit LogErr("ClearScreen Start To Cursor"); // Has not been called by application...yet, not sure if it should delete char at cursor...

    uint row, col;

    row = cy;
    for( col = 0; col <= cx; col++ ) tbuf[row][col] = FLAG_DEFAULT;
    for( row = 0; row < cy; row++ ) {
        for( col = 0; col < NCOL; col++ ) {
            tbuf[row][col] = FLAG_DEFAULT;
        }
    }
}

void Core::ClearScreenFromCursor() {

    emit LogMsg("ClearScreen From Cursor to End");

    if(( cy == 0 ) && ( cx == 0 )) {
        emit LogMsg("Same as ClearScreen");
        return ClearScreen();
    }

    uint row, col;

    row = cy;
    for( col = cx; col < NCOL; col++ ) tbuf[row][col] = FLAG_DEFAULT;
    for( row = cy + 1; row < NROW; row++ ) {
        for( col = 0; col < NCOL; col++ ) {
            tbuf[row][col] = FLAG_DEFAULT;
        }
    }
}

void Core::ShiftScreen() {

    cx = 0;
    cy++;

    if( cy >= NROW ) {

        if(!alt_mode) ScrollAppend();

        cy = NROW - 1;

        uint row, col;
        for( row = 0; row < NROW - 1; row++ ) {
            for( col = 0; col < NCOL; col++ ) {
                tbuf[row][col] = tbuf[row+1][col];
            }
        }

        for( col = 0; col < NCOL; col++ ) {
            tbuf[NROW-1][col] = FLAG_DEFAULT;
        }
    }

}

void Core::ClearLine() {

    uint col;
    for( col = cx; col < NCOL; col++ ) {
        tbuf[cy][col] = FLAG_DEFAULT;
    }
}

void Core::DeleteLine( uint lines ) {

    uint row, col, row_end;

    row_end = cy + lines;
    if( row_end >= NROW ) row_end = NROW;

    for( row = cy; row < row_end; row++ ) {
        for( col = 0; col < NCOL; col++ ) {
            tbuf[row][col] = FLAG_DEFAULT;
        }
    }

    uint k = cy;
    for( row = row_end; row < NROW; row++ ) {
        for( col = 0; col < NCOL; col++ ) {
            tbuf[k][col] = tbuf[row][col];
            tbuf[row][col] = FLAG_DEFAULT;
        }
        k++;
    }

    cx = 0;
}

void Core::InsertLine( uint lines ) {

    uint row, col;
    uint k = NROW - 1 + lines;
    row = NROW - 1;
    for( uint n = 0; n < ( NROW - cy ); n++ ) {

        if( k < NROW ) {
            for( col = 0; col < NCOL; col++ ) {
                tbuf[k][col] = tbuf[row][col];
            }
        }
        k--;
        row--;
    }

    uint row_end = cy + lines;
    if( row_end >= NROW ) row_end = NROW;

    for( row = cy; row < row_end; row++ ) {
        for( col = 0; col < NCOL; col++ ) {
            tbuf[row][col] = FLAG_DEFAULT;
        }
    }

    cx = 0;

}

void Core::DeleteChar( uint nchar ) {

    // TODO: wide-char handling???

    uint n = cx;
    for( uint k = cx + nchar; k < NCOL;  k++ ) {
        tbuf[cy][n] = tbuf[cy][k];
        n++;
    }

    for( uint k = NCOL - nchar; k < NCOL; k++ ) {
        tbuf[cy][k] = FLAG_DEFAULT;
    }

    // TODO: clear characters

}

//-------------------------------------------------------------------

void Core::AltEnter() {

    if( alt_mode ) {
        LogErr("Trying to enter alt when already in alt mode");
        return;
    }

    LogMsg("Enter Alt");
    alt_mode = true;

    uint row, col;
    for( row = 0; row < NROW; row++ ) {
        for( col = 0; col < NCOL; col++ ) {
            alt_tbuf[row][col] = tbuf[row][col];
        }
    }

    alt_cx = cx;
    alt_cy = cy;
    alt_attr = attr;

    this->ClearScreen();

}

void Core::AltExit() {

    if(!alt_mode ) {
        LogErr("Trying to exit alt when already not in alt mode");
        return;
    }

    LogMsg("Exit Alt");
    alt_mode = false;

    this->ClearScreen();

    uint row, col;
    for( row = 0; row < NROW; row++ ) {
        for( col = 0; col < NCOL; col++ ) {
            tbuf[row][col] = alt_tbuf[row][col];
        }
    }

    cx = alt_cx;
    cy = alt_cy;
    attr = alt_attr;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void Core::SetAttrFG(uint r, uint g, uint b) {
    uint64_t ATTR_FG_R = (( ((uint64_t)r) >> 2 ) & 0x3FULL ) << BITS_FG_R;
    uint64_t ATTR_FG_G = (( ((uint64_t)g) >> 2 ) & 0x3FULL ) << BITS_FG_G;
    uint64_t ATTR_FG_B = (( ((uint64_t)b) >> 2 ) & 0x3FULL ) << BITS_FG_B;
    attr = ( attr & ~ATTR_FG_ALL) | ATTR_FG_R | ATTR_FG_G | ATTR_FG_B;
}

void Core::SetAttrBG(uint r, uint g, uint b) {
    uint64_t ATTR_BG_R = (( ((uint64_t)r) >> 2 ) & 0x3FULL ) << BITS_BG_R;
    uint64_t ATTR_BG_G = (( ((uint64_t)g) >> 2 ) & 0x3FULL ) << BITS_BG_G;
    uint64_t ATTR_BG_B = (( ((uint64_t)b) >> 2 ) & 0x3FULL ) << BITS_BG_B;
    attr = ( attr & ~ATTR_BG_ALL) | ATTR_BG_R | ATTR_BG_G | ATTR_BG_B;
}

int Core::SetCodeCsiSgr(QString csi ) {

    QStringList list = csi.split(";");

    attr = FLAG_DEFAULT; // Reset When Called

    if( list[0].length() == 0 ) {
        LogMsg("SGR 0 attr reset to normal");
        attr = FLAG_DEFAULT;
        return(0);
    }

    for( int k = 0; k < list.length(); k++ ) {
        QString s = list[k];
        int n = s.toInt();

        switch( n ) {
        case   0:  attr  = FLAG_DEFAULT;      break;
        case   1:  attr |= FLAG_BOLD;         break;
        case   3:  attr |= FLAG_ITALIC;       break;
        case   4:  attr |= FLAG_UNDERLINE;    break;

        case   7:  attr |= FLAG_REVERSE;      break;

        case  10: break; // Default font ( do nothing... )
        case  11: case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19: LogErr("alternative font unsupported"); break;

        case  30: SetAttrFG(   0,   0,   0 ); break; // Black
        case  31: SetAttrFG( 205,   0,   0 ); break; // Red
        case  32: SetAttrFG(   0, 205,   0 ); break; // Green
        case  33: SetAttrFG( 255, 199,   6 ); break; // Yellow
        case  34: SetAttrFG(   0,   0, 238 ); break; // Blue
        case  35: SetAttrFG( 205,   0, 205 ); break; // Magenta
        case  36: SetAttrFG(   0, 205, 205 ); break; // Cyan
        case  37: SetAttrFG( 232, 232, 232 ); break; // White

        case  39: SetAttrFG( 232, 232, 232 ); break; // Default Foreground

        case  40: SetAttrBG(   0,   0,   0 ); break; // Black
        case  41: SetAttrBG( 205,   0,   0 ); break; // Red
        case  42: SetAttrBG(   0, 205,   0 ); break; // Green
        case  43: SetAttrBG( 255, 199,   6 ); break; // Yellow
        case  44: SetAttrBG(   0,   0, 238 ); break; // Blue
        case  45: SetAttrBG( 205,   0, 205 ); break; // Magenta
        case  46: SetAttrBG(   0, 205, 205 ); break; // Cyan
        case  47: SetAttrBG( 232, 232, 232 ); break; // White

        case  49: SetAttrBG(   0,   0,   0 ); break; // Default Background

        case  90: SetAttrFG( 128, 128, 128 ); break; // Bright Black (Gray)
        case  91: SetAttrFG( 255,   0,   0 ); break; // Bright Red
        case  92: SetAttrFG(   0, 255,   0 ); break; // Bright Green
        case  93: SetAttrFG( 255, 255,   0 ); break; // Bright Yellow
        case  94: SetAttrFG(  92,  92, 255 ); break; // Bright Blue
        case  95: SetAttrFG( 255,   0, 255 ); break; // Bright Magenta
        case  96: SetAttrFG(   0, 255, 255 ); break; // Bright Cyan
        case  97: SetAttrFG( 255, 255, 255 ); break; // Bright White

        case 100: SetAttrBG( 128, 128, 128 ); break; // Bright Black (Gray)
        case 101: SetAttrBG( 255,   0,   0 ); break; // Bright Red
        case 102: SetAttrBG(   0, 255,   0 ); break; // Bright Green
        case 103: SetAttrBG( 255, 255,   0 ); break; // Bright Yellow
        case 104: SetAttrBG(  92,  92, 255 ); break; // Bright Blue
        case 105: SetAttrBG( 255,   0, 255 ); break; // Bright Magenta
        case 106: SetAttrBG(   0, 255, 255 ); break; // Bright Cyan
        case 107: SetAttrBG( 255, 255, 255 ); break; // Bright White

        default: LogErr( QString("Ignored SGR attr: %1").arg(n) ); break;
        }
    }

    return(0);

}

int Core::SetCodeCsi(uint c) {

    if( c >= 0x20 && c <= 0x3F ) {
        csi.append((char)c);
        return(0);
    }

    if( c >= 0x40 && c <= 0x7E ) {
        LogMsg( QString("Detected csi %1").arg(((char)c) ) + QString( " csi = ") + csi );

        csi_mode = false;

        if( c == 'K' ) {
            //attr = FLAG_DEFAULT; // Probably not needed...???
            if( csi.length() == 0 ) {
                ClearLine();
            } else {
                LogErr("ClearLine with param not handeled"); // TODO: mode
            }

            return(0);
        }

        if( c == 'L' ) {
            if( csi.length() == 0 ) {
                InsertLine(1);
            } else {
                uint lines = csi.toUInt();
                InsertLine( lines );
            }
            return(0);
        }

        if( c == 'M' ) {
            if( csi.length() == 0 ) {
                DeleteLine(1);
            } else {
                uint lines = csi.toUInt();
                DeleteLine( lines );
            }
            return(0);
        }

        if( c == 'P' ) {
            if( csi.length() == 0 ) {
                DeleteChar(1);
            } else {
                uint nchar = csi.toUInt();
                DeleteChar( nchar );
            }
            return(0);
        }

        if( c == 'A' ) {

            if( csi.length() == 0 ) {
                if( cy > 0 ) cy--;
            } else {
                uint offs = csi.toUInt();
                if( offs > cy ) cy  = 0;
                else            cy -= offs;
            }

            return(0);
        }

        if( c == 'B' ) {

            if( csi.length() == 0 ) {
                cy++;
            } else {
                uint offs = csi.toUInt();
                cy += offs;
            }

            if( cy >= NROW ) cy = NROW - 1;

            return(0);
        }

        if( c == 'C' ) {

            if( csi.length() == 0 ) {
                cx++;
            } else {
                int offs = csi.toInt();
                cx += offs;
            }

            if( cx >= NCOL ) cx = NCOL - 1;
            return(0);
        }

        if( c == 'D' ) {

            if( csi.length() == 0 ) {
                if( cx > 0 )  cx -= 1;
            } else {
                uint offs = csi.toUInt();
                if( offs > cx ) cx  = 0;
                else            cx -= offs;
            }
            return(0);
        }

        if( c == 'G' ) { // Cursor Horizontal Absolute

            if( csi.length() == 0 ) {
                cx = 0;
            } else {
                int offs = csi.toInt();
                cx = offs;
            }

            if( cx >= NCOL ) cx = NCOL - 1;
            return(0);
        }

        if( c == 'H' ) {
            //attr = FLAG_DEFAULT; // Probably not needed....
            QStringList list = csi.split(";");
            if( list.length() <= 1 ) {

                cy = 0;
                cx = 0;

            } else {

                QString s0 = list[0];
                QString s1 = list[1];
                cy = s0.toInt();
                cx = s1.toInt();
                if( cy >= NROW ) cy = NROW;
                if( cx >= NCOL ) cx = NCOL;

                // convert to 0-based from 1-based
                if( cy > 0 ) cy = cy - 1;
                if( cx > 0 ) cx = cx - 1;

            }

            return(0);
        }

        if( c == 'J' ) {
            attr = FLAG_DEFAULT;
            if( csi.length() == 0 ) {
                this->ClearScreenFromCursor();
            } else {

                uint mode = csi.toUInt();

                switch( mode ) {
                case 0: this->ClearScreenFromCursor(); break;
                case 1: this->ClearScreenToCursor(); break;
                case 2: this->ClearScreen(); break;
                default : LogErr(QString("ClearScreen with mode=%1 not handeled").arg(mode));
                }
            }
            return(0);
        }

        if( c == 'm' ) {
            return this->SetCodeCsiSgr(csi);
        }

        if( c == 'h' ) {
            if( csi == QString("?1049") ) {
                this->AltEnter();
                return(0);
            }

            if( csi == QString("?1") ) {
                this->app_cursor = true; // Enter Application key mode
                LogMsg(QString("Enable DECCKM"));
                return(0);
            }
        }

        if( c == 'l' ) {
            if( csi == QString("?1049") ) {
                this->AltExit();
                return(0);
            }

            if( csi == QString("?1") ) {
                this->app_cursor = false; // Exit Application key mode
                LogMsg(QString("Disable DECCKM"));
                return(0);
            }
        }

        if( c == 'n' ) {
            if( csi == QString("6") ) {
                // Need to report cursor position by sending ESC[n;mR where n is the row, m is col
                LogMsg(QString("Current Cursor Pos Row = %1 Col = %2").arg(cy + 1).arg(cx + 1));
                char msg[20];
                sprintf( msg, "\e[%d;%dR", cy + 1, cx + 1 );
                int len = strlen( msg );
                ptylib_send( msg, len );
                return(0);
            }
        }

        LogErr( QString("Unhandled csi %1").arg(((char)c)) + QString( " csi = ") + csi );
        return(0);

    }

    csi_mode = false;
    LogErr( QString("csi char out of range %1").arg(c) );
    return(0);
}

int Core::SetCode( uint c ) {

    if( esc ) {

        esc = false;

        if( c == '[' ) {
            csi_mode = true;
            csi.clear();
            return(0);
        }

        if( c == ']' || c == 'P' || c == 'X' || c == '^' || c == '_' ) {
            LogMsg(QString("SSS detected %1").arg(c));
            sss_mode = c;
            sss_ccnt = 0;
            return(0);
        }

        if( c == '\\' ) {
            LogMsg("ST detected");
            sss_mode = 0;
            return(0);
        }

        LogErr( QString("Unhandled esc sequence %1(%2)").arg( c ).arg( (char) c ));
        return(0);

    }

    //--------------------------------------

    if( c < 0x20 || c == 0x7F ) {

        switch( c )  {

        case 0x00:                        break; // ignore null
        //case 0x03: break; // EOT? returns via ctrl-c where system("stty sane") was not called
        case 0x1b: esc = -1;              break; // ESC
        case 0x0c: this->ClearScreen();   break; // Form Feed FF
        case '\a': LogMsg("BEEP"); if( sss_mode == ']') { sss_mode = 0; /*Do OSC handler...*/ } break; // BEL
        case '\b': if( cx > 0 ) cx -= 1;  break; // backspace
        case '\t': cx += 8 - cx % 8;      break; // tabstop is for now hardcoded as 8
        case '\r': cx = 0;                break; // Carriage Return CR
        case '\n': this->ShiftScreen();   break; // Line Feed LF
        default  : LogErr(QString("Unhandled C0 detected: %1").arg(c)); return(c);
        }

        return(0);

    } else {

        if( csi_mode ) {
            return SetCodeCsi( c );
        }

        if( sss_mode > 0 ) {
            if( sss_ccnt < SSS_CMAX ) sss_data[sss_ccnt++] = c;
            return(0);
        }

        if(this->terminal->isFullWidth( c ) ) {

            if( cx >= NCOL - 1 ) this->ShiftScreen(); // line wrapping TODO: toggle ON/OFF

            if( cx < ( NCOL - 1 ) && cy < NROW ) {
                tbuf[cy][cx]   = FLAG_WIDECHAR;
                tbuf[cy][cx+1] = c | attr;
                cx += 2;
            }
        } else {
            if( cx >= NCOL ) this->ShiftScreen(); // line wrapping TODO: toggle ON/OFF
            if( cx < NCOL && cy < NROW ) tbuf[cy][cx] = c | attr;
            cx++;
        }

        return(0);
    }

    return(-1);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int Core::SetChar( uint c ) {

    if( 0 == crem ) {

        if( ( c & 0x80 ) == 0x0 ) {
            return this->SetCode( c );
        }

        cccc[0] = c;
        ccnt = 1;

        if( ( c & 0xE0 ) == 0xC0 ) {
            crem = 1;
        } else if( ( c & 0xF0 ) == 0xE0 ) {
            crem = 2;
        } else if( ( c & 0xF8 ) == 0xF0 ) {
            crem = 3;
        } else {
            return this->SetCode( 0xFFFD );
        }

    } else {

        cccc[ ccnt++ ] = c;
        crem--;

        if( 0 == crem ) {

            uint codepoint = 0xFFFD;

            if( ccnt == 2 ) codepoint = (( cccc[0] & 0x1F ) <<  6 ) | ( cccc[1] & 0x3F );
            if( ccnt == 3 ) codepoint = (( cccc[0] & 0x0F ) << 12 ) | (( cccc[1] & 0x3F ) <<  6 ) | ( cccc[2] & 0x3F );
            if( ccnt == 4 ) codepoint = (( cccc[0] & 0x07 ) << 18 ) | (( cccc[1] & 0x3F ) << 12 ) | (( cccc[2] & 0x3F ) << 6 ) | ( cccc[3] & 0x3F );

            return this->SetCode( codepoint );
        }
    }

    return(1);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void Core::UpdateCore( char *str, int len ) {

    if( str == NULL ) {
        LogMsg( QString("POS %1").arg(len) );
        return;
    }

    if( len <= 0 ) {
        LogErr( QString("UpdateCore Error %1\n").arg(len) );
    }

    if( capture ) {
        LogErr( QString("UpdateCore %1\n").arg(len) );

        char cap[10000];
        cap[0] = '\0';

        for( int k = 0; k < len; k++ ) {
            if      ( str[k] >= 0x7F ) sprintf( cap + strlen(cap), "%02x ", str[k] ) ;
            else if ( str[k] <  0x20 ) sprintf( cap + strlen(cap), "%02x ", str[k] ) ;
            else if ( str[k] == 0x20 ) sprintf( cap + strlen(cap), "SP" );
            else if ( str[k] >= 0x20 ) sprintf( cap + strlen(cap), " %c ", str[k] );
            if( str[k] == '\n' ) sprintf( cap + strlen(cap), "\n");
            //if( k % 32 == 31 ) printf("\n");
        }
        LogErr( QString::fromLatin1( cap ) );
    }

    int n = 0;
    while( n < len ) {
        int c = str[n++];
        this->SetChar(c);
    }

    delete str;

    emit UpdateTerminal();
}

void Core::SendCore(QString s ) {

    int n = 0;
    bool surrogate = false;
    uint c_prev;
    while( n < s.length() ) {
        uint c = (uint)s[n++].unicode();

        if( QChar(c).isHighSurrogate() ) {
            c_prev = c;
            surrogate = true;
            continue;
        }

        if( surrogate ) {
            c = QChar::surrogateToUcs4( c_prev, c );
            surrogate = false;
        }

        if( echo ) this->SetChar(c);
    }

    //attr = FLAG_DEFAULT; // ?? Probably wrong to do this here...
    ptylib_send( s.toUtf8().data(), s.length());
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

void Core::TestCore( int mode ) {

    char data[1000];
    data[0] = '\0';

    switch ( mode ) {
    case 0: sprintf( data, "\nabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz01234567890123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\ndef\t   あいうえおtest\nxyz\n𩸽 ほっけ\n㍻ 平成\n㋿ 令和\n　←全角スペース\n㍼  昭和\nか  き くけ こ\n"); break;

    case 1:
        sprintf( data, "\n");
        sprintf( data + strlen(data), "あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほ０１２３４５６７８９０１２３４５６７８９０１２３４５６７８９\n");
        sprintf( data + strlen(data), " あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほ０１２３４５６７８９０１２３４５６７８９０１２３４５６７８９\n");
        sprintf( data + strlen(data), "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\n");
        break;

    case 2: sprintf( data, "\nHelloWorld\n\e[01;34mHelloWorld\n\e[0m\e[03mHelloWorld\n\e[04mHelloWorld\n\e[m"); break;
    case 3: sprintf( data, "\n\e[41mHelloWorld\n\e[01mHelloWorld\n\e[0m\e[03;07mHelloWorld \n\e[04mHelloWorld\n\e[m"); break;
    //case 4: sprintf( data, "\e[999;999H\e[6n"); break;
    case 4: sprintf( data, "\e]8;;http://example.com\e\\This is a link\e]8;;\e\\\n"); break;

    case 5:
        sprintf(data, "\n");
        for( int n = 30; n < 38; n++ ) { sprintf( data + strlen(data), "\e[%d;49m %03d\e[m", n, n ); }
        for( int n = 30; n < 38; n++ ) { sprintf( data + strlen(data), "\e[%d;47m %03d\e[m", n, n ); } sprintf( data + strlen(data), "\n");
        for( int n = 40; n < 48; n++ ) { sprintf( data + strlen(data), "\e[%d;39m %03d\e[m", n, n ); }
        for( int n = 40; n < 48; n++ ) { sprintf( data + strlen(data), "\e[%d;30m %03d\e[m", n, n ); } sprintf( data + strlen(data), "\n");
        for( int n = 90; n < 98; n++ ) { sprintf( data + strlen(data), "\e[%dm %03d\e[m", n, n ); }
        for( int n =100; n <108; n++ ) { sprintf( data + strlen(data), "\e[%dm %03d\e[m", n, n ); } sprintf( data + strlen(data), "\n");
        break;

    case 6:
        sprintf(data, "\n");
        for( int n = 30; n < 38; n++ ) { sprintf( data + strlen(data), "\e[%d;49;7m %03d\e[m", n, n ); }
        for( int n = 30; n < 38; n++ ) { sprintf( data + strlen(data), "\e[%d;47;7m %03d\e[m", n, n ); } sprintf( data + strlen(data), "\n");
        for( int n = 40; n < 48; n++ ) { sprintf( data + strlen(data), "\e[%d;39;7m %03d\e[m", n, n ); }
        for( int n = 40; n < 48; n++ ) { sprintf( data + strlen(data), "\e[%d;30;7m %03d\e[m", n, n ); } sprintf( data + strlen(data), "\n");
        for( int n = 90; n < 98; n++ ) { sprintf( data + strlen(data), "\e[%d;7m %03d\e[m", n, n ); }
        for( int n =100; n <108; n++ ) { sprintf( data + strlen(data), "\e[%d;7m %03d\e[m", n, n ); } sprintf( data + strlen(data), "\n");
        break;

        //sprintf( data, "\nNormal\e[07m\nReverse\e[m");

    case 7: sprintf( data, "\e[20B"); break;
    case 8: sprintf( data, "\e[20C"); break;
    //case 7: sprintf( data, "\e[10;10H\e[1J"); break;
    //case 8: sprintf( data, "\e[10;10H\e[2M"); break;
    //case 9: sprintf( data, "\e[10;25H\e[2L"); break;
    //case 7: sprintf( data, "\e[1;1H\e[1J"); break;
    //case 8: sprintf( data, "\e[1;1H\e[2M"); break;
    //case 9: sprintf( data, "\e[2;2H\e[P"); break;

    case 9:
        sprintf(data, "\n");
        for( int n = 0; n < 100; n++ ) { sprintf( data + strlen(data), "%d\n", n ); }
        break;

    case 100: sprintf( data, "\e[?1049h\e[6n" ); break; // Enable  alternative screen buffer
    case 101: sprintf( data, "\e[?1049l" ); break; // Disable alternative screen buffer
    case 102: capture = true; break;
    case 103: capture = false; break;
    default: sprintf( data, "Unknown Test Mode\n"); break;
    }

    int dlen = strlen( data );

    int n = 0;
    while( n < dlen ) {
        int c = data[n++];
        this->SetChar(c);
    }

    emit UpdateTerminal();
}
