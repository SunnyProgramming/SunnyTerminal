#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>

#define SHELL "/bin/sh"

int    master, slave;
char   name[100];
struct termios term;
struct winsize win;

int initialized = 0;

int ptylib_init( int cols, int rows ) {

    if(initialized) return(0);

	int rc = openpty( &master, &slave, name, &term, &win );
	if( rc != 0 ) {
		printf("Failed to openpty\n");
		return(-1);
	}

        struct termios tp;
        if( tcgetattr( slave, &tp ) == -1  ) { // works with master as well for some reason
            perror("tcgetattr");
            return(-1);
        }
        printf("tp.c_lflag %08x\n", tp.c_lflag );
        tp.c_lflag |= ECHO;
        //tp.c_lflag &= ~(ICANON);
        printf("tp.c_lflag %08x\n", tp.c_lflag );

        if( tcsetattr( slave, TCSAFLUSH, &tp ) == -1 ) { // works with master as well for some reason
            perror("tcsetattr");
            return(-1);
        }

	printf("%s\n", "-" SHELL );
	printf("name: %s\n",   name );
	printf("master: %d\n", master );
	printf("slave: %d\n",  slave );
	printf("win row: %u\n", win.ws_row );
	printf("win col: %u\n", win.ws_col );
	printf("win xpixel: %u\n", win.ws_xpixel );
	printf("win ypixel: %u\n", win.ws_ypixel );

    printf("term.c_iflag %08x\n", term.c_iflag );
    printf("term.c_oflag %08x\n", term.c_oflag );
    printf("term.c_cflag %08x\n", term.c_cflag );
    printf("term.c_lflag %08x\n", term.c_lflag );
    fflush(stdout);

    struct passwd *pwd;
    pwd = getpwuid((geteuid()));
    printf("pw_name  %s\n", pwd->pw_name );
    printf("pw_dir   %s\n", pwd->pw_dir );
    printf("pw_shell %s\n", pwd->pw_shell );
    fflush(stdout);

    char *env[6];
    char  term[40], logname[200], user[200], home[200], shell[40];

    sprintf( term,    "TERM=ansi");
    sprintf( logname, "LOGNAME=%s", pwd->pw_name );
    sprintf( user,    "USER=%s",    pwd->pw_name );
    sprintf( home,    "HOME=%s",    pwd->pw_dir  );
    sprintf( shell,   "SHELL=%s",   SHELL        );

    env[0] = term;
    env[1] = logname;
    env[2] = user;
    env[3] = home;
    env[4] = shell;
    env[5] = NULL;

    pid_t p = fork();

	if( p == 0 ) {
		printf("child process");
        fflush(stdout);
		close( master );
		setsid();
		if( ioctl( slave, TIOCSCTTY, NULL ) == -1 ) {
			
			perror("ioctl(TIOCSCTTY)");
			return(-1);
		}

        //struct termios tp;
        //if( tcgetattr(0, &tp ) == -1  ) {
        //    perror("tcgetattr");
        //    return(-1);
        //}
        //printf("tp.c_lflag %08x\n", tp.c_lflag );
        //tp.c_lflag |= ECHO;
        //printf("tp.c_lflag %08x\n", tp.c_lflag );

        //if( tcsetattr(0, TCSAFLUSH, &tp ) == -1 ) {
        //    perror("tcsetattr");
        //    return(-1);
        //}

        //fflush(stdout);

		dup2( slave, 0 );
		dup2( slave, 1 );
		dup2( slave, 2 );
		close( slave );

        system("stty sane"); // TODO check if stty sane is successful

        char stty[30];
        sprintf( stty, "stty rows %d cols %d", rows, cols );
        system(stty); // TODO check if stty sane is successful

        initialized = 1;
        execle( SHELL, "-" SHELL, (char*)NULL, env);
        //execle( SHELL, "-/bin/sh", (char*)NULL, env);
		return(-1);
	}

    initialized = 1;
	close( slave );
    return(0);

}

int ptylib_send( char *str, int len ) {
    if(!initialized) return(0);
	write( master, str, len );
    return(0);
}

//--------------------------------------------------------------

/*
static fd_set readable;

int ptylib_recv_select( void ) {

    FD_ZERO(&readable);
    FD_SET( master, &readable );

    if( select( master + 1, &readable, NULL, NULL, NULL ) <= 0 ) {
        printf("error select\n");
        fflush(stdout);
        return(-1);
    }

    return(0);

}

int ptylib_recv_isset( void ) {

    if( FD_ISSET( master, &readable ) ) {
        return(1);
    }
    return(0);
}

int ptylib_recv_read( char *str, int max ) {

    int n;


    if(( n = read( master, str, max )) <= 0 ) {
        // When SHELL is exited, read will return -1
        printf("Nothing to read, SHELL is closed...\n");
        fflush(stdout);
        return(-1);
    }

    return(n);
}
*/

int ptylib_recv( char *str, int max ) {

    if(!initialized) return(0);

    int n;
    fd_set readable;

    FD_ZERO(&readable);
    FD_SET( master, &readable );

    if( select( master + 1, &readable, NULL, NULL, NULL ) <= 0 ) {
        printf("error select\n");
        fflush(stdout);
        return(-1);
    }

    if( FD_ISSET( master, &readable ) ) {

        if(( n = read( master, str, max )) <= 0 ) {
            // When SHELL is exited, read will return -1
            printf("Nothing to read, SHELL is closed...\n");
            fflush(stdout);
            return(-3);
        }

        return(n);
    }

    return(-2);
}

int ptylib_exit( ) {
    if(!initialized) return(0);
	close( master );
	//close( slave );
	return(0);

}

int ptylib_initialized() {
    return initialized;
}
