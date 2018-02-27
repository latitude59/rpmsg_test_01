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
#include "message_protocol.h"

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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#if 0
int send_packet( int fd, packet_t* packet )
{
    int bytes_written = 0;

    size_t len = sizeof( packet_t ) + ( packet->msg_count * sizeof( message_t ) );
    bytes_written = write( fd, (void*)packet, len );
    if( bytes_written != len )
    {
        printf( "send_packet() write() error: len = %d, bytes_written = %d - %s\r\n", len, bytes_written, STRERR );
        return -1;
    }

    return 0;
}
#endif

int decode_packet( int len, char* data )
{
    if( data == NULL )
        return -1;
    // check packet length
    packet_hdr_t* hdr = (packet_hdr_t*)data;
    pthread_mutex_lock( &mutex );
    printf( "start_byte: 0x%02x\n", hdr->start_byte );
    printf( "ack_req: %u\n", hdr->ack_req );
    printf( "msg_count: %u\n", hdr->msg_count );
    printf( "data_len: %u\n", hdr->data_len );
    for( int msg_num = 0; msg_num < hdr->msg_count; msg_num++ )
    {
        message_t* msg_ptr = (message_t*)( data + sizeof( packet_hdr_t ) + ( sizeof( message_t ) * msg_num ) );
        printf( "message %d:\n", msg_num + 1 );
        printf( "  command: %u\n", msg_ptr->cmd );
        printf( "  address: %u\n", msg_ptr->dev_addr );
        printf( "  ack: %u\n", msg_ptr->ack );
        printf( "  timestamp: %u\n", msg_ptr->timestamp );
        printf( "  value: %u\n", msg_ptr->value );
    }
    // check crc
    uint32_t* crc = (uint32_t*)( data + sizeof( packet_hdr_t ) + ( sizeof( message_t ) * hdr->msg_count ) );
    printf( "crc: 0x%08x\n", *crc );
    pthread_mutex_unlock( &mutex );
    return 0;
}


void* threadfunc( void* parg )
{
    printf( "%s\n", __func__ );
    const int BUF_SZ = 512;
    char data[BUF_SZ];
    int* pfd = (int*)parg;
    int bytes_read = 0;
    int msgs_rxd = 0;
    for( ; ; )
    {
        printf( "here\n" );
        memset( data, 0, BUF_SZ );
        bytes_read = read( *pfd, data, BUF_SZ );
        pthread_mutex_lock( &mutex );
        printf( "got  [%02d]: ", msgs_rxd++ );
        for( int i = 0; i < bytes_read; i++ )
            printf( " %02x", data[i] );
        printf( "\n" );
        pthread_mutex_unlock( &mutex );
        decode_packet( bytes_read, data );
    }
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
    //tc_send( fd, TC_TRANSFER_COUNT, DATA_LEN );

    for( ; ; )
    {
        printf( "enter command: " );
        char c = getchar();
        switch( c )
        {
        case '1':
            printf( "\nsend 1 message\n" );
            break;
        default:
            printf( "\nsend no messages\n" );
            break;
        }
    }

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


