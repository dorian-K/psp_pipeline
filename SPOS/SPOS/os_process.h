/*! \file
 *  \brief Struct specifying a process.
 *
 *  Contains the struct that bundles all properties of a running process.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef _OS_PROCESS_H
#define _OS_PROCESS_H

#include <stdbool.h>
#include <stdint.h>

//! The type for the ID of a running process.
#error IMPLEMENT STH. HERE
typedef ? ProcessID;

//! This is the type of a program function (not the pointer to one!).
#error IMPLEMENT STH. HERE
typedef ? Program(?);

//! The type of the priority of a process.
typedef uint8_t Priority;

//! The age of a process (scheduler specific property).
typedef uint16_t Age;

//! The type for the checksum used to check stack consistency.
typedef uint8_t StackChecksum;

//! Type for the state a specific process is currently in.
typedef enum ProcessState {
    OS_PS_UNUSED,
    OS_PS_READY,
    OS_PS_RUNNING,
    OS_PS_BLOCKED
} ProcessState;

//! A union that holds the current stack pointer of a given process.
//! We use a union so we can reduce the number of explicit casts.
typedef union StackPointer {
    uint16_t as_int;
    uint8_t *as_ptr;
} StackPointer;

/*!
 *  The struct that holds all information for a process.
 *  Note that additional scheduling information (such as the current time-slice)
 *  are stored by the module that implements the actual scheduling strategies.
 */
#error IMPLEMENT STH. HERE
typedef struct {
    ?
} Process;

/*!
 *  This structure can be used to form a singly linked list
 *  containing program pointers.
 *  Iteration is possible by following the pointer in the next field.
 */
struct program_linked_list_node {
    Program *program;
    struct program_linked_list_node *next;
};

/*!
 *  Declaration for the variable which points to the head
 *  of the linked list where autostart programs are registered.
 */
extern struct program_linked_list_node *autostart_head;

/*!
 *  Prepends the passed program function to the autostart linked list.
 *
 *  This will break if you pass a non-trivial expression as the function pointer.
 *  For example REGISTER_AUTOSTART(some_struct.func_ptr) will not have the desired effect,
 *  but will in fact not even compile.
 *
 *  Use this macro in this fashion:
 *
 *    REGISTER_AUTOSTART(foobar);
 *    void foobar(void) {
 *      foo();
 *      bar();
 *      ...
 *    }
 */
#define REGISTER_AUTOSTART(PROGRAM_FUNCTION)                                         \
    Program PROGRAM_FUNCTION;                                                        \
    void __attribute__((constructor)) register_autostart_##PROGRAM_FUNCTION(void) {  \
        static struct program_linked_list_node node = {.program = PROGRAM_FUNCTION}; \
        node.next = autostart_head;                                                  \
        autostart_head = &node;                                                      \
    }

//! Returns whether the passed process can be selected to run.
bool os_isRunnable(const Process *process);

#endif
