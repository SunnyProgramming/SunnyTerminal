#ifndef _PTYLIB_H_
#define _PTYLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

int ptylib_init( int, int );
int ptylib_send( char*, int );
int ptylib_recv( char*, int );
//int ptylib_recv_select();
//int ptylib_recv_isset();
//int ptylib_recv_read( char*, int );
int ptylib_exit();
int ptylib_initialized();

#ifdef __cplusplus
}
#endif

#endif
