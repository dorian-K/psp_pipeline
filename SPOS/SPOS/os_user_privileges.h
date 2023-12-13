/*! \file
 *  \brief User privileges specification used in part by the task manager.
 *
 *  Contains the task manager interface functionality, specifying the
 *  respective rights of the current user.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef _OS_USER_PRIVILEGES_H
#define _OS_USER_PRIVILEGES_H

#include "defines.h"
#include "os_scheduler.h"

#include <avr/pgmspace.h>
#include <stdint.h>
#if (VERSUCH >= 3)
#include "os_memory.h"
#endif

//! The response value that answers a permission request.
typedef enum {
    //! Explicitly tell the user that he may not access this functionality.
    OS_AP_EXPLICIT_DENY,

    //! Do not tell the user that he has no access rights for this functionality.
    OS_AP_SILENT_DENY,

    //! Allow the user to access this functionality.
    OS_AP_ALLOW
} AccessPermission;

/*!
 *  The request serialization.
 */
typedef enum {
    OS_PR_OPEN_TASKMAN,      //!< Requests permission to open the task manager.
    OS_PR_ALWAYS_DENY,       //!< Default request that is always to be denied.
    OS_PR_ALWAYS_ALLOW,      //!< Default request that is always to be allowed.
    OS_PR_FRONTPAGE,         //!< Request to show the front page showing how many processes. are running and which one is currently active.
    OS_PR_START_PROG_SELECT, //!< Request to show the program execution selection.
    OS_PR_START_PROG,        //!< Request to start the chosen program.
    OS_PR_KILL_SELECT,       //!< Request to show the process kill selection.
    OS_PR_KILL,              //!< Request to kill the selected program.
    OS_PR_PRIORITY_SELECT,   //!< Request to show the page in which a process can be selected whose priority should be changed.
    OS_PR_PRIORITY_SHOW,     //!< Request to show the page on which the new priority of the selected process can be chosen.
    OS_PR_PRIORITY,          //!< Request to set the priority of the selected process to the chosen value.
    OS_PR_SCHEDULING_SELECT, //!< Request to show the scheduling strategy selection.
    OS_PR_SCHEDULING,        //!< Request to set the scheduling strategy to the selected.
    OS_PR_ALLOCATION_SELECT, //!< Request to show the allocation strategy selection for the previously selected heap.
    OS_PR_ALLOCATION,        //!< Request to set the allocation strategy of the selected heap to the newly chosen.
    OS_PR_SHOW_HEAP,         //!< Request to open the heap sub menu for the selected heap.
    OS_PR_ERASE_HEAP         //!< Request to completely erase the contents (map and use) of the selected heap.
} PermissionRequest;

//! The argument of the request.
typedef union {
    char null;
    Program *prog;
    ProcessID pid;
    SchedulingStrategy ss;
    uint8_t heapId;
#if (VERSUCH >= 3)
    AllocStrategy as;
#else
    uint8_t as;
#endif
} RequestArgument;

//! Which member of the RequestArgument is set.
typedef enum {
    OS_RAF_null,
    OS_RAF_prog,
    OS_RAF_pid,
    OS_RAF_ss,
    OS_RAF_heapId,
    OS_RAF_as
} RequestArgumentFlag;

//! Ask if a certain functionality may be accessed by the current user.
//! The given Process ID may be irrelevant for a given request.
AccessPermission os_askPermission(PermissionRequest ap, RequestArgument ra, RequestArgumentFlag raf, const char **reason);

#endif
