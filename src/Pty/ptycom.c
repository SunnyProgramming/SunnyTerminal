#include <stdio.h>
#include <string.h>
#include <windows.h>

static int initialized = 0;
static HANDLE hcom = INVALID_HANDLE_VALUE; // HANDLE is basically a void *

int ptylib_init( int cols, int rows ) {

    if( initialized ) return(0);

    hcom = CreateFileA( // CreateFile is macro choooses CreateFileA or CreateFileW
            "COM1",                          // COM10 or above must be in "\\\\.\\COM10"
            GENERIC_READ | GENERIC_WRITE,    // fdwAccess? read write mode
            0,                               // shared mode? multiple open is not allowed
            NULL,                            // address of security pointer?
            OPEN_EXISTING,                   // how to create
            0,                               // file attribute, FILE_ATTRIBUTE_NORMAL?
            NULL                             // handle of file with attribute to copy
        );

    if( hcom == INVALID_HANDLE_VALUE ) {
        printf("could not open com port\n");
        CloseHandle(hcom);
        return(-1);
    }

        if( TRUE != SetupComm( hcom, 4096, 4096 )) {
        printf("Failed SetupComm\n");
        return(-1);
    }

    if( TRUE != PurgeComm( hcom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ) {
        printf("Failed PurgeComm\n");
        return(-1);
    }

    //---------------------------------------

    DCB dcb; // Device Control Block...
    GetCommState( hcom, &dcb );

    printf("Current DCB state\n");
    printf("DCBlength = %ld\n", (DWORD)dcb.DCBlength );
    printf("BaudRate = %ld\n", (DWORD)dcb.BaudRate );
    printf("fBinary = %ld\n", (DWORD)dcb.fBinary );
    printf("fParity = %ld\n", (DWORD)dcb.fParity );
    printf("fOutxCtsFlow = %ld\n", (DWORD)dcb.fOutxCtsFlow );
    printf("fOutxDsrFlow = %ld\n", (DWORD)dcb.fOutxDsrFlow );
    printf("fDtrControl = %ld\n", (DWORD)dcb.fDtrControl );
    printf("fDsrSensitivity = %ld\n", (DWORD)dcb.fDsrSensitivity );
    printf("fTXContinueOnXoff = %ld\n", (DWORD)dcb.fTXContinueOnXoff );
    printf("fOutX = %ld\n", (DWORD)dcb.fOutX );
    printf("fInX = %ld\n", (DWORD)dcb.fInX );
    printf("fErrorChar = %ld\n", (DWORD)dcb.fErrorChar );
    printf("fRtsControl = %ld\n", (DWORD)dcb.fRtsControl );
    printf("fAbortOnError = %ld\n", (DWORD)dcb.fAbortOnError );
    printf("XonLim = %ld\n", (DWORD)dcb.XonLim );
    printf("XoffLim = %ld\n", (DWORD)dcb.XoffLim );
    printf("ByteSize = %ld\n", (DWORD)dcb.ByteSize );
    printf("Parity = %ld\n", (DWORD)dcb.Parity );
    printf("StopBits = %ld\n", (DWORD)dcb.StopBits );
    printf("XonChar = %ld\n", (DWORD)dcb.XonChar );
    printf("XoffChar = %ld\n", (DWORD)dcb.XoffChar );
    printf("ErrorChar = %ld\n", (DWORD)dcb.ErrorChar );
    printf("EofChar = %ld\n", (DWORD)dcb.EofChar );
    printf("EvtChar = %ld\n", (DWORD)dcb.EvtChar );
    fflush(stdout);

    dcb.BaudRate = 115200;
    dcb.fBinary  = TRUE;
    dcb.ByteSize = 8;
    dcb.fParity  = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    if( TRUE != SetCommState( hcom, &dcb ) ) {
        printf("Failed SetCommSate\n");
        return(-1);
    }

    //---------------------------------------

    COMMTIMEOUTS timeout;
    GetCommTimeouts( hcom, &timeout );

    printf("ReadIntervalTimeout = %ld\n", timeout.ReadIntervalTimeout );
    printf("ReadTotalTimeoutMultiplier = %ld\n", timeout.ReadTotalTimeoutMultiplier );
    printf("ReadTotalTimeoutConstant = %ld\n", timeout.ReadTotalTimeoutConstant );
    printf("WriteTotalTimeoutMultiplier = %ld\n", timeout.WriteTotalTimeoutMultiplier );
    printf("WriteTotalTimeoutConstant = %ld\n", timeout.WriteTotalTimeoutConstant );
    fflush(stdout);

    initialized = 1;
    return(0);
}

int ptylib_send( char *str, int len ) {

    if(!initialized) {
        return(0); // ignored
    }

    if( hcom == INVALID_HANDLE_VALUE ) {
        return(-1);
    }

    DWORD writeSize;
    WriteFile(hcom, str, (DWORD)len, &writeSize, NULL);
    return((int)writeSize);
}

/*
int ptylib_recv_select() {
    return(0);
}

int ptylib_recv_isset() {
    return(1);
}

int ptylib_recv_read( char *str, int max ) {
    int n;
    int len = strlen( data );
    if( len >= max ) len = max;
    for( n = 0; n < len; n++ ) str[n] = data[n];
    data[0] = '\0';
    return( len );
}
*/

int ptylib_recv( char *str, int max ) {

    if(!initialized) {
        return(0); // ignored
    }

    if( hcom == INVALID_HANDLE_VALUE ) {
        return(-1);
    }

    DWORD len;
    if( FALSE != ReadFile( hcom, str, max, &len, NULL ) ) {
        return((int)len);
    }

    return( 0 );
}

int ptylib_exit( ) {

    printf("ptylib_exit called\n"); fflush(stdout);

    if( hcom != INVALID_HANDLE_VALUE ) {
        CloseHandle(hcom);
    }

    return(0);
}

int ptylib_initialized() {
    return initialized;
}
