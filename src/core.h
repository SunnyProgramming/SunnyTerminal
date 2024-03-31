#ifndef CORE_H
#define CORE_H

#include <QObject>

#define DEF_NCOL 80
#define DEF_NROW 24

#define MAX_NCOL 300
#define MAX_NROW 100

#define SSS_CMAX 256

#define SCROLL_MAX 300

// Unicode is defined between 0x0 - 0x0010FFFF
// Therefore Top 11 bits can be used for color, bold and other 2 attributes
// Also 0xFFFFFFFF is used as WideChar Flag

// Unicode   21bit
// Attribute 7bits?
// Forecolor 18bit
// Backcolor 18bit

#define FLAG_WIDECHAR          0xFFFFFFFFFFFFFFFFULL
#define FLAG_UNICODE           0x00000000001FFFFFULL

#define FLAG_UNDERLINE         0x0000000000200000ULL
#define FLAG_ITALIC            0x0000000000400000ULL
#define FLAG_BOLD              0x0000000000800000ULL
#define FLAG_REVERSE           0x0000000001000000ULL

//#define FLAG_BLINK           0x0000000002000000ULL
//#define FLAG_STRIKEOUT       0x0000000004000000ULL
//#define FLAG_TRUECOLOR       0x0000000008000000ULL

//RGB666 shift 28bits
#define ATTR_FG_ALL            0x00003FFFF0000000ULL
#define BITS_FG_RGB            28
#define BITS_FG_R              40
#define BITS_FG_G              34
#define BITS_FG_B              28

//RGB666 shift 46bits
#define ATTR_BG_ALL            0xFFFFC00000000000ULL
#define BITS_BG_RGB            46
#define BITS_BG_R              58
#define BITS_BG_G              52
#define BITS_BG_B              46

#define FLAG_DEFAULT           0x00003AEBA0000000ULL  // FG_RGB(232,232,232)

#define COLOR_BLACK             0
#define COLOR_RED               1
#define COLOR_GREEN             2
#define COLOR_YELLOW            3
#define COLOR_BLUE              4
#define COLOR_MAGENTA           5
#define COLOR_CYAN              6
#define COLOR_WHITE             7

class Terminal;

class Core : public QObject {

    Q_OBJECT

public:
    explicit Core(QObject *parent = 0);

    uint NROW;
    uint NCOL;

    uint64_t GetCode( uint, uint );
    uint cx;
    uint cy;
    Terminal *terminal;

    bool    capture;
    bool    echo;

    bool    app_cursor;

    bool    ScrollUp();
    bool    ScrollDown();
    bool    ScrollHome();
    bool    ScrollEnd();
    bool    ScrollSave();

private:

    int      ncol;
    int      nrow;

    uint64_t tbuf[MAX_NROW][MAX_NCOL];
    uint64_t attr;

    void     ScrollAppend();
    void     ScrollAppendScreen();
    bool     scroll_mode;
    uint     scroll_next;
    uint     scroll_last;
    uint     scroll_user;
    uint64_t scroll_tbuf[SCROLL_MAX][MAX_NCOL];

    void     AltEnter();
    void     AltExit();
    bool     alt_mode;
    uint64_t alt_tbuf[MAX_NROW][MAX_NCOL];
    uint64_t alt_attr;
    uint     alt_cx;
    uint     alt_cy;

    int      SetChar ( uint c );
    uint     crem;
    uint     ccnt;
    uint     cccc[4];

    uint     sss_mode; // shared string sequnece mode
    uint     sss_ccnt;
    uint     sss_data[SSS_CMAX];

    bool     esc;

    bool     csi_mode;
    QString  csi;
    void     SetAttrFG( uint, uint, uint );
    void     SetAttrBG( uint, uint, uint );
    int      SetCode    ( uint c );
    int      SetCodeCsi ( uint c );
    int      SetCodeCsiSgr ( QString );
    void     ClearScreen();
    void     ClearScreenToCursor();
    void     ClearScreenFromCursor();
    void     ShiftScreen();
    void     ClearLine();
    void     DeleteLine( uint );
    void     InsertLine( uint );
    void     DeleteChar( uint );

signals:
    void     UpdateTerminal();
    void     LogMsg( QString );
    void     LogErr( QString );

public slots:
    void     UpdateCore( char *, int );
    void     SendCore( QString );
    void     TestCore( int n );

};

#endif // CORE_H
