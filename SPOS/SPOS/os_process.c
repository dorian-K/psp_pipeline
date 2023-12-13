#include "os_process.h"

#include <stddef.h>

struct program_linked_list_node *autostart_head = NULL;

/*!
 *  Checks whether given process is runnable
 *
 *  \param process A pointer on the process
 *  \return The state of the process, 1 if it is ready or running, 0 otherwise
 */
bool os_isRunnable(const Process *process) {
    if (process && (process->state == OS_PS_READY || process->state == OS_PS_RUNNING)) {
        return true;
    }

    return false;
}
