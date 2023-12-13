#include "os_user_privileges.h"

/*!
 *  This function returns an accessPermission to a certain functionality within the task manager.
 *  Based on the permission request different request arguments are handed to the function.
 *  The permission request represents the page the task manager wants to access. The different
 *  values for this enum can be found in the os_user_privileges.h file. Depending on the permission
 *  request there may be an request argument needed while asking for a specific permission.
 *  One example would be pr = OS_PR_KILL_SELECT, which announces selecting a process to kill
 *  within the task manager. The request argument would then match the pid of the process that has
 *  been selected. With this information one could deny killing processes with certain ids (like the idle task).
 *  The raf announces which member of the request argument has to be accessed. Take a look at
 *  os_user_privileges.h for more information.
 *
 *  \param pr The permission request denouncing the functionality the task manager wants to access.
 *  \param ra This may be 0, but in some kind of request is needed to hand over more specific context
 *            conditions of the permission request.
 *  \param raf This parameter gives information about the type of the request argument.
 *  \param reason A pointer to a pointer of a char, where a reason can be specified which
 *                is displayed when access is denied. ( Return OS_AP_EXPLICIT_DENY to display this).
 *  \return This functions returns whether a certain functionality may be accessed, or not. It can
 *          also be specified, if an explicit reason should be displayed (OS_AP_EXPLICIT_DENY) or not (OS_AP_SILENT_DENY).
 */
AccessPermission os_askPermission(PermissionRequest pr, RequestArgument ra, RequestArgumentFlag raf, const char **reason) {
    switch (pr) {
        case OS_PR_ALWAYS_DENY:
            return OS_AP_EXPLICIT_DENY;
        default:
            return OS_AP_ALLOW;
    }
}
