// #############################################################################
// Header file: ban.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include "config.h"

#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"

// #############################################################################

#ifndef __ban_h__
#define __ban_h__

// #############################################################################
// type and constant definitionsx
//

struct NNCMS_BAN_INFO
{
    // Ban data
    unsigned int id;
    struct in_addr ip;
    struct in_addr mask;

    // Linked list, required for insert sorting
    struct NNCMS_BAN_INFO *next;
};

// #############################################################################
// function declarations
//

// Modular functions
bool ban_init( struct NNCMS_THREAD_INFO *req );
bool ban_deinit( struct NNCMS_THREAD_INFO *req );
void ban_load( struct NNCMS_THREAD_INFO *req );
void ban_unload( struct NNCMS_THREAD_INFO *req );

// Pages
void ban_add( struct NNCMS_THREAD_INFO *req );
void ban_view( struct NNCMS_THREAD_INFO *req );
void ban_edit( struct NNCMS_THREAD_INFO *req );
void ban_delete( struct NNCMS_THREAD_INFO *req );

// Check if ip address is banned
bool ban_check( struct NNCMS_THREAD_INFO *req, struct in_addr *addr );

// #############################################################################

#endif // __ban_h__

// #############################################################################
