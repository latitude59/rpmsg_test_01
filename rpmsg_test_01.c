/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define STRERR strerror( errno )

const int TC_TRANSFER_COUNT = 12;
const int DATA_LEN = 40;
const char RPMSGDEV[] = "/dev/ttyRPMSG";

struct termios ti;

// open and initialise RPMSG device
// returns file descriptor or -1 if error
int init( void )
{
    int fd;

    fd = open( RPMSGDEV, O_RDWR | O_NOCTTY );
    if( fd < 0 )
    {
        printf( "Unable to open %s: %s\n", RPMSGDEV, STRERR );
        return -1;
    }
    printf( "%s opened: fd %d\n", RPMSGDEV, fd );

    tcflush( fd, TCIOFLUSH );       // flush data received but not read, and data written but not transmitted

    if( tcgetattr( fd, &ti ) < 0 )  // obtain terminal parameters
    {
        printf( "tcgetattr error: %s\n", STRERR );
        return -1;
    }

    cfmakeraw( &ti );               // input is available character by character, echoing is disabled, and all special processing of terminal input and output characters is disabled
    cfsetospeed( &ti, B115200 );    // set output baud rate to 115,200
    cfsetispeed( &ti, B115200 );    // set input baud rate to 115,200
    if( tcsetattr( fd, TCSANOW, &ti ) < 0 )     // make above changes immediately
    {
        printf( "tcsetattr error: %s\n", STRERR );
        return -1;
    }
    return fd;
}

// close RPMSG device
void deinit( int fd )
{
    if( close( fd ) != 0 )
        printf( "close() error: %s\n", STRERR );
    else
        printf( "%s closed\n" , RPMSGDEV );
}

// compare received data with expected
int pattern_cmp( char *buffer, char pattern, int len )
{
    int i;

    for( int i = 0; i < len; i++ )
        if( buffer[i] != pattern )
            return -1;
    return 0;
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// send <data_len> bytes <transfer_count> times
int tc_send( int fd, int transfer_count, int data_len )
{
    int bytesWritten = 0;
    char data[data_len];

    memset( data, -1, data_len );

    for( int i = 0; i < transfer_count; i++ )
    {
        memset( data, i, data_len );
        bytesWritten = write( fd, data, data_len );
        if( bytesWritten != data_len )
        {
            printf( "tc_send() error: %s\n", STRERR );
            return -1;
        }

        pthread_mutex_lock( &mutex );
        printf( "sent [%02d]: ", i );
        for( int j = 0; j < data_len; j++ )
            printf( " %02x", data[j] );
        printf( "\n" );
        pthread_mutex_unlock( &mutex );
    }
    return 0;
}

// receive <data_len> bytes <transfer_count> times and compare with expected
int tc_receive( int fd, int transfer_count, int data_len )
{
    int bytesRead = 0;
    char data[data_len];

    memset( data, -1, data_len );

    for( int i = 0; i < transfer_count; i++ )
    {
        bytesRead = read( fd, data, data_len );
        if( bytesRead != data_len )
        {
            printf( "tc_receive() error: %s\n", STRERR );
            return -1;
        }

        pthread_mutex_lock( &mutex );
        printf( "got  [%02d]: ", i );
        for( int j = 0; j < data_len; j++ )
            printf( " %02x", data[j] );
        printf( "\n" );
        pthread_mutex_unlock( &mutex );

        int result = pattern_cmp( data, i, data_len );
        if( result == -1 )
        {
            printf( "pattern_cmp() error\n" );
            return result;
        }
    }
    return 0;
}


void* threadfunc( void* parg )
{
    printf( "%s\n", __func__ );
    int* pfd = (int*)parg;
    tc_receive( *pfd, TC_TRANSFER_COUNT, DATA_LEN );
    return NULL;
}


int main( int argc, char* argv[] )
{
    int fd, retval;
    pthread_t thread;
    int arg;
    fd = init();
    if( fd < 0 )
        return fd;

    printf( "%s successfully initialised\n", RPMSGDEV );

    printf( "creating read thread\n" );
    if( pthread_create( &thread, NULL, threadfunc, &fd ) != 0 )
    {
        printf( "pthread_create() error: %s\n", STRERR );
        deinit( fd );
        return -1;
    }

    printf( "read thread created\n" );
    tc_send( fd, TC_TRANSFER_COUNT, DATA_LEN );

    // wait for thread to finish
    if( pthread_join( thread, NULL ) != 0 )
    {
        printf( "pthread_join() error: %s\n", STRERR );
        deinit( fd );
        return -1;
    }

    printf( "thread terminated\n" );

//    if( tc_send( fd, TC_TRANSFER_COUNT, DATA_LEN ) == 0 )
//    {
//        printf( "creating thread\n" );
//        if( pthread_create( &thread, NULL, threadfunc, &fd ) == 0 )
//        {
//            printf( "thread created\n" );
//            // wait for thread to finish
//            if( pthread_join( thread, NULL ) != 0 )
//            {
//                printf( "pthread_join() error: %s\n", STRERR );
//                deinit( fd );
//                return -1;
//            }
//            printf( "thread terminated\n" );
//        }
//        else
//        {
//            printf( "pthread_create() error: %s\n", STRERR );
//            deinit( fd );
//            return -1;
//        }
//    }

    //tc_receive( fd, TC_TRANSFER_COUNT, DATA_LEN );
    deinit( fd );
    return 0;
}


