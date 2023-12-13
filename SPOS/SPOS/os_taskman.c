/*!
 * \internal
 * \file
 *
 * Big Picture overview of the TM.
 * First we will define a few custom flags for conditional compiling;
 *     The TM is compatible with all minor versions for exercises 2 through 6.
 *     The single defines are not essential for understanding the key functionality.
 *
 * How does it work?
 *     At its core, the TM is basically a generic engine that controls a dynamic graph (!) of pages.
 *     Usually the dynamic graph of pages degenerates to a static tree, rooted at the "root-page".
 *
 * Control flow;
 *     When the TM is invoked, by calling `os_taskManMain`, it will automatically push the root-page to its
 *     display-buffer. The user may then select pages which are pushed on top of this root-page.
 *     When the buffer is empty, the TM terminates and returns control to its caller.
 *     All the work is done in `os_taskManMain` while the pages are specified as functions that can peek on the
 *     call stack to determine their respective context.
 *     Pages **NEVER** block (i.e. no busy waiting). All user interaction is done in `os_taskManMain`.
 *     If you wish to understands the details, there is no way around understanding how `os_taskManMain` works.
 *
 * Page specification;
 *     To allow the single pages to be minimalistic, and quite simple to extend, a lot of helper functions and
 *     macros are implemented which can be used in order to reduce code duplication.
 */

#include "os_taskman.h"

#include "defines.h"
#include "lcd.h"
#include "util.h"

#include <stdint.h>

/* INTERFACE TO SPOS *****************************/

#include "os_input.h"
#include "os_process.h"
#include "os_scheduler.h"
#include "os_user_privileges.h"
#if (VERSUCH >= 3)
#include "os_memory.h"
#endif

#pragma GCC push_options
#pragma GCC optimize("O3")

Process *os_getProcessSlot(ProcessID);
Program *os_getProgramSlot(Program);

/* END OF INTERFACE DECLS ************************/


/*!
 * Specifies to what depth pages of the TM can be nested.
 * This is hard coded, because we have to allocate stack space before calling the pages.
 * If this value is too small, you will not be able to descend beyond the given level.
 */
#define TM_NESTING_DEPTH 6

/*!
 * Does the OS know how to kill a process?
 * This should be implemented in exercise 3.
 */
#define TM_COMPILE_KILL_SUPPORT (VERSUCH >= 3)
/*!
 * Does the OS know what the priority of a process is?
 * This should be implemented in exercise 3.
 */
#define TM_COMPILE_PRIORITY_SUPPORT (VERSUCH >= 3)
/*!
 * Does the OS know how to plug and play different scheduling strategies?
 * This should be implemented in exercise 2, when the scheduler is implemented.
 */
#define TM_COMPILE_SCHEDULING_SUPPORT (VERSUCH >= 2)
/*!
 * Used to deactivate the support for the memory drivers.
 * Set this to 1 if you have implemented the memory part of SPOS.
 */
#define TM_COMPILE_HEAP_SUPPORT (VERSUCH >= 3)

/*!
 * The number of main-pages of the TM. Actually, this is set by
 * the respective page-handler at runtime.
 */
#define TM_MAINPAGES 8

/*!
 * How many heaps should the TM maximally support. This is
 * slightly redundant to os_getMemDriverListLength() of the os_mem_drivers
 * module, but the TM wants to be independent of that other define.
 */
#define TM_HEAP_SUPPORT 3

/*!
 * When dumping the map of any heap, this define specifies how many
 * map-entries should be visible at a time. Be careful when changing this
 * constant, as it may lead to display overruns if set too high (or odd).
 */
#define TM_MAP_ENTRIES_PER_PAGE 20

/*!
 * This is a wrapper for the os_getInput function of the os_input module.
 * It is never used directly but utilizes a macro to use a stack variable as inputBuffer.
 * \param inputBuffer The byte where to store the state of the buttons.
 * \returns The content of the buffer.
 */
static uint8_t updateInputP(uint8_t *inputBuffer) {
    return (*inputBuffer = os_getInput());
}

//! Index of the Escape-Button.
#define ES 3
//! Index of the Up-Button
#define UP 2
//! Index of the Down-Button
#define DN 1
//! Index of the Enter-Button
#define OK 0

/*!
 * Everywhere in the TM, an action must be acknowledged. This is a HCI policy.
 * These variables/macros tell the user whether their action was
 * successful (done) or failed (fail).
 * These specific strings were chosen in order for them to be of the
 * same length. Do not change them to longer strings!
 */
static char PROGMEM const doneStr[] = "done";
static char PROGMEM const failStr[] = "fail";
#define tm_done()                     \
    {                                 \
        lcd_line2();                  \
        lcd_writeProgString(doneStr); \
    }
#define tm_fail()                     \
    {                                 \
        lcd_line2();                  \
        lcd_writeProgString(failStr); \
    }

//! A very convenient constant to pad strings with spaces.
static char PROGMEM const spaces16[] = "                ";

// Forward declarations
// See below for further explanation!
typedef struct ParamStack ParamStack;
typedef struct PageResult PageResult;

/*!
 * This define allows us to create a TM page routine without having
 * to copy the whole parameter string over and over.
 * NOTE: This is the very basic TM page function. There are macros of
 *       much higher abstraction, you should always use THOSE!
 */
#define TM_PAGE(NAME) void NAME(ParamStack const *p, PageResult *result)
/*!
 * The type of TM page routines.
 * A task manager page is a `tm_page*` (i.e. a function pointer).
 */
typedef TM_PAGE(tm_page);

/*!
 * This type holds the state information of a specific page. Pages have
 * generally in common, that they have an index which is currently displayed and
 * a range where this index can move. Note that indices may be invalid.
 */
typedef struct PageState {
    //! What is the handler for this page.
    tm_page *call;
    //! Which index is currently displayed.
    uint16_t param;
    //! What is the valid range for this page (e.g. \#PROCS).
    uint16_t range;
} PageState;

/*!
 * Since the TM is built to save redundancy at all costs, we have a main TM
 * core function that calls all pages respecting what subpage was chosen by
 * the previous page.
 * However, due to this mechanism, the native call-stack only holds one function
 * call, but is oblivious to the parameters of parent pages.
 * Therefore we have to construct or own call-stack, stored in a variable of
 * this type.
 */
struct ParamStack {
    /*!
     * The hierarchy of called pages. Here new pages are 'pushed' and old
     * pages are 'popped'.
     */
    PageState pages[TM_NESTING_DEPTH];
    /*!
     * Which is the top-most page? Note that this stack grows from high
     * addresses to low addresses. Hence, if 'top' reaches 0, the stack is full.
     */
    uint8_t top;
    /*!
     * The total number of available slots. This is redundant to the length
     * of the 'pages' array, but it is easier to access, which is the rationale
     * for the redundancy. Note how it is immutable.
     */
    const uint8_t size;
};

/*!
 * This is essentially what a page returns to the main TM function (via
 * a reference in its parameter-list).
 * It is not returned directly to increase automation (see below).
 */
struct PageResult {
    /*!
     * What page to call if the user chooses to descent.
     * If there is no child page, this will be a special null-page (NOT NULL).
     */
    PageState child;
    /*!
     * Was the page successful in displaying the requested index, or does the
     * TM function has to continue searching for a valid argument?
     * The rationale for this goes really deep, so please consult the
     * documentation of os_taskManMain.
     */
    bool success;
};

/*!
 * This is the root page that is automatically called by os_taskManMain.
 */
static tm_page tm_rootpage;

/*!
 * This variable is true if the TaskManager is open
 */
static bool tm_open;

/*!
 * \return True if the taskmanager is currently open.
 */
bool os_taskManOpen() {
    return tm_open;
}


/*!
 * This is the main entry point for the TM, as invoked e.g. from the
 * scheduler.
 * This function will dynamically build the call graph (!) of sub-pages
 * during runtime and react on the user input, selecting the
 * appropriate page.
 * The key idea is to keep all (sub-)pages AS SIMPLE AS POSSIBLE.
 * Specifically, all pages have a very important property: they return
 * IMMEDIATELY. That means that all the looping and looking for the right
 * sub-page to display must be done in os_taskManMain, as no page contains a
 * loop (except very small and fast ones).
 * A page only displays the contents that go on the LCD at one time, without
 * attempting to figure out the right index to display.
 * This has a bunch of advantages. For one thing, we do not have to reimplement
 * the wheel over and over again. Also, the pages that implement a certain
 * functionality stay simple, such that we have time to think about the stuff
 * that really needs our attention ("How do I access a processes priority?" as
 * opposed to "How do I poll which page to display next?").
 * If a page is not able to display a given index (e.g. because the process to
 * that index does not exists) it will return false, and the main-loop will
 * attempt to find another index to display, until no possibilities are left,
 * and the page is forcefully popped from the virtual call-stack.
 *
 * \hidecallgraph
 */
void os_taskManMain(void) {
    // Ask for permission to be opened
    RequestArgument ra;
    const char *reason;
    switch (os_askPermission(OS_PR_OPEN_TASKMAN, ra, OS_RAF_null, &reason)) {
        case OS_AP_SILENT_DENY:
            return;

        case OS_AP_EXPLICIT_DENY:
            lcd_clear();
            lcd_writeProgString(PSTR("Feature denied"));
            if (!reason) {
                lcd_writeChar('.');
            } else {
                lcd_writeChar(':');
                char reason16[17];
                strlcpy_P(reason16, reason, 17);
                lcd_line2();
                lcd_writeString(reason16);
            }
            // Wait for confirmation (OK+ES)
            while (os_getInput() != (1 | (1 << 3))) continue;
            os_waitForNoInput();
            return;

        default:
            break;
    }

    tm_open = true;

// Small shorthand to poll for user-input.
#define updateInput() updateInputP(&buttonInput)

/*
 * Convenience macro to get the state of a specific button.
 * E.g. READ_BTN(DN) will check the buttonInput buffer and evaluate
 * to 1 if the Down-Button was pressed.
 */
#define READ_BTN(BTN) ((buttonInput >> (BTN)) & 1)

    // Here, only the low nibble is relevant.
    uint8_t buttonInput;

    /*
     * This variable will later be used to store the information in which
     * direction the user wants to navigate. Note that this variable is
     * an element of {x, 0, 1}, while x === -1 regarding the range of the
     * current page. This effects that we can "loop through" at both sides.
     * Note: If you just "fixed" the "typo" above; see xkcd.com/326 and revert it.
     */
    uint16_t direction = 0;

    /*
     * This is our main call-stack. Note that (in contrast to previous versions)
     * the size of the call-stack is fixed and determined at design-time. That
     * means that the longest path in the call-graph is fixed as well. A virtual
     * stack overflow is caught below, which will throw a runtime error.
     */
    ParamStack stack = {
        .top = TM_NESTING_DEPTH,
        .size = TM_NESTING_DEPTH};
    stack.top--;
    stack.pages[stack.top].call = tm_rootpage;
    stack.pages[stack.top].param = 0;
    stack.pages[stack.top].range = TM_MAINPAGES;

    /*
     * This is where all pages will write their result to. It sits on the stack
     * as opposed to the heap, as all pages are called while the stack-frame you
     * are currently looking at exists.
     */
    PageResult pageResult;

    /*
     * This is a loop that is left when the stack underflows (i.e. when there is
     * no page left to display). This is not an error condition but simply occurs
     * when the user leaves the root-page.
     */
    while (stack.top < stack.size) {
        /*
         * 'run' specifies the number of attempts we will make to get the current
         * page to display an index. Obviously, if it fails more that 'range' times,
         * there is NO index that will work, which we will respond to by forcefully
         * popping the current page (see below).
         */
        uint16_t run = stack.pages[stack.top].range + 1;
        do {
            stack.pages[stack.top].param += direction;
            stack.pages[stack.top].param %= stack.pages[stack.top].range;
            lcd_clear();

            /*
             * The page is supposed to display nothing if it fails.
             * Observe, that return success=false does not mean that the page could
             * not execute a command by the user.
             */
            stack.pages[stack.top].call(&stack, &pageResult);

            /*
             * If the user did not actually want to move (e.g. he just entered this
             * page), we have to force any movement (forward is more intuitive), in
             * order to find a displayable index.
             */
            direction += !direction;
        } while (run-- && !pageResult.success);
        direction = 0;
        bool newInput;

        // After displaying the page (or failing at it) we have to handle whether
        // to change the page, or obey the user-input (which we will have to poll).
        do {
            // We only change the page (i.e. render a new one) if either we could not
            // get the current one to succeed or if the user wants us to change it.
            if (pageResult.success) {
                /*
                 * Note how the user is not even bothered in case the current page failed.
                 * After polling here, the poll-result that was read here, will be used
                 * in the remainder of this iteration.
                 * So even if the user manages to release a button faster than we can
                 * process it, we will still know it was pressed (updateInput() has
                 * heavy side effects, as it is a macro).
                 */
                while (!updateInput()) continue;
            }
            newInput = true;
            if (READ_BTN(ES) || !pageResult.success) {
                /*
                 * If either the user wants us to leave the current page, or the page
                 * was unsuccessful (not able to display a single index), we have to
                 * POP THE CURRENT FRAME.
                 */
                stack.top++;
            } else if (READ_BTN(OK) && (pageResult.child.call) && stack.top) { // Only descend if the nesting level has not been reached!
                /*
                 * If the user wants us to descend in the page-tree AND the current
                 * page has a valid sub-page (child), then we will
                 * PUSH A NEW FRAME.
                 */
                stack.pages[--stack.top] = pageResult.child;
            } else if ((READ_BTN(DN) || READ_BTN(UP)) && (stack.pages[stack.top].range > 1)) {
                // If the user wants us to move around, we will modify the 'direction'
                // variable to indicate whether to go up or down in the next iteration.
                direction += READ_BTN(DN) + (READ_BTN(UP) * (stack.pages[stack.top].range - 1));
            } else {
                /*
                 * 'newInput' actually means "input that we have to react on". So, if
                 * the input of the user would e.g. only repaint the display with the
                 * precise same content, we wait for other input, as repaints are ugly.
                 */
                newInput = false;
            }
            while (updateInput()) continue;
        } while (!newInput);
        // This can occur if our design-time estimate of the stack size was too small.
        // { stack.top + 1 != 0 }
    }
    tm_open = false;
    lcd_clear();
#undef updateInput
#undef READ_BTN
}

//! The strings that are displayed in the root-page.
char PROGMEM const mainLabels[] =
    // 123456789abcdef0123456789ABCDEF0
    "-~= TaskMan =~-                \0"
    "Kill Process                   \0"
    "Change Priority                \0"
    "Change Scheduling Strategy     \0"
    "Heap(s)                        \0";

// Forward declarations for the sub-pages of the root-page.
static tm_page tm_frontpage;

#if TM_COMPILE_KILL_SUPPORT
static tm_page tm_killProc;
#endif

#if TM_COMPILE_PRIORITY_SUPPORT
static tm_page tm_priority;
#endif

#if TM_COMPILE_SCHEDULING_SUPPORT
static tm_page tm_scheduling;
#endif

#if TM_COMPILE_HEAP_SUPPORT
static tm_page tm_heap;
#endif

static tm_page tm_null;

// A convenience macro to access the stack-history.
#define peekStack(GOBACK) (p->pages[p->top + (GOBACK)])

// XXX this could probably be improved ...
#define MAX2(Xa, Xb) (((Xa) > (Xb)) ? (Xa) : (Xb))
#define MAX3(Xa, X2...) (MAX2(Xa, (MAX2(X2))))
#define MAX4(Xa, X3...) (MAX2(Xa, (MAX3(X3))))
#define MAX5(Xa, X4...) (MAX2(Xa, (MAX4(X4))))
#define MAX6(Xa, X5...) (MAX2(Xa, (MAX5(X5))))

#if TM_COMPILE_SCHEDULING_SUPPORT
#if VERSUCH >= 5
#define SS_MAX_COUNT (MAX6(OS_SS_RUN_TO_COMPLETION, OS_SS_RANDOM, OS_SS_EVEN, OS_SS_ROUND_ROBIN, OS_SS_INACTIVE_AGING, OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE) + 1)
#else
#define SS_MAX_COUNT (MAX5(OS_SS_RUN_TO_COMPLETION, OS_SS_RANDOM, OS_SS_EVEN, OS_SS_ROUND_ROBIN, OS_SS_INACTIVE_AGING) + 1)
#endif

#endif
#if TM_COMPILE_HEAP_SUPPORT
#define MS_MAX_COUNT (MAX4(OS_MEM_FIRST, OS_MEM_NEXT, OS_MEM_BEST, OS_MEM_WORST) + 1)
#endif

/*!
 * This is the root-page that shows the top-level pages of the TM.
 */
static TM_PAGE(tm_rootpage) {
    uint16_t page = peekStack(0).param;
    switch (page) {
        /*
         * One case branch for each sub-page.
         * INDEX: the index of the sub-page.
         * CALL: the function handler of the sub-page.
         * PARAM: initial index to invoke the sub-page with.
         * RANGE: the range of possible indices for the sub-page.
         */
#define SUBP(INDEX, CALL, PARAM, RANGE)          \
    case INDEX: {                                \
        result->child.call = CALL;               \
        result->child.param = (uint16_t)(PARAM); \
        result->child.range = (uint16_t)(RANGE); \
        break;                                   \
    }
        SUBP(0, tm_frontpage, 0, 1)
#if TM_COMPILE_KILL_SUPPORT
        SUBP(1, tm_killProc, os_getCurrentProc(), MAX_NUMBER_OF_PROCESSES)
#endif
#if TM_COMPILE_PRIORITY_SUPPORT
        SUBP(2, tm_priority, os_getCurrentProc(), MAX_NUMBER_OF_PROCESSES)
#endif
#if TM_COMPILE_SCHEDULING_SUPPORT
        SUBP(3, tm_scheduling, os_getSchedulingStrategy(), SS_MAX_COUNT)
#endif
#if TM_COMPILE_HEAP_SUPPORT
        SUBP(4, tm_heap, 0, TM_HEAP_SUPPORT)
#endif
#undef SUBP
        default:
            result->child.call = tm_null;
    }
    result->success = result->child.call != tm_null;
    if (result->success) {
        lcd_writeProgString(mainLabels + 32 * page);
        if (!page) {
            // The start-page has an extra information which process
            // is currently running.
            lcd_line2();
            lcd_writeProgString(PSTR("Current Proc: "));
            lcd_writeDec(os_getCurrentProc());
        }
    }
}

/*!
 * Dummy null-page, to be specified by pages that have no sub-page.
 */
static TM_PAGE(tm_null) {
}

/*!
 * Okay, this is a bit tricky :)
 * We want to encapsulate the repetitive work that has to be done in
 * each page. Also we want to make the definition of the result in
 * the page-code as usable as possible.
 * Thus, the actual page-function (TM_PAGE(NAME)) prepares a few variables
 * and then invokes the actual implementation which has a different signature
 * (NAME\#\#_user). That one will then be specified by the user of this macro.
 * \param NAME the literal name of the page-handler to create.
 * \param CHILD the sub-page of this page.
 * \param CHILDARG the initial index to call the sub-page with.
 * \param CHILDRANGE the range of the sub-page.
 * \param REQ ??
 * \param REQFLAG ??
 * \param ARG ??
 */
#define MAKE_PAGEHANDLER(NAME, CHILD, CHILDARG, CHILDRANGE, REQ, REQFLAG, ARG)                                              \
    static tm_page CHILD;                                                                                                   \
    static bool NAME##_user(ParamStack const *, PageState *);                                                               \
    static TM_PAGE(NAME) {                                                                                                  \
        RequestArgument ra = {.REQFLAG = ARG};                                                                              \
        pageHandlerWrapper(                                                                                                 \
            p, result, NAME##_user, ((CHILD) == tm_null) ? 0 : (CHILD), (CHILDARG), (CHILDRANGE), REQ, ra, OS_RAF_##REQFLAG \
        );                                                                                                                  \
    }                                                                                                                       \
    static bool NAME##_user(ParamStack const *p, PageState *result)

/*!
 * Helper function for the `MAKE_PAGEHANDLER` helper macro.
 * \param p The stack passed through from the os_taskManMain function.
 * \param result The information that the page is supposed to return to the os_taskManMain function.
 * \param pageHnd The custom user function, actually implementing the page's functionality.
 * \param childHnd Which child page should be returned by default? This can still be overridden in the custom function.
 * \param childArg What would be the initial argument to our child page?
 * \param childRange What parameter range does our child page have?
 * \param pr The permission request specifier for this page's feature. This has to do with user privileges.
 * \param ra The request argument union.
 * \param raf A flag, indicating which union member to extract from ra.
 */
static void pageHandlerWrapper(const ParamStack *p, PageResult *result, bool (*pageHnd)(const ParamStack *, PageState *), tm_page *childHnd, uint16_t childArg, uint16_t childRange, PermissionRequest pr, RequestArgument ra, RequestArgumentFlag raf) {
    result->child.call = childHnd;
    result->child.param = childArg;
    result->child.range = childRange;
    const char *reason = NULL;
    switch (os_askPermission(pr, ra, raf, &reason)) {
        case OS_AP_EXPLICIT_DENY:
            lcd_writeProgString(PSTR("Feature denied"));
            if (!reason) {
                lcd_writeChar('.');
            } else {
                lcd_writeChar(':');
                char reason16[17];
                strlcpy_P(reason16, reason, 17);
                lcd_line2();
                lcd_writeString(reason16);
            }
            result->success = true;
            break;
        case OS_AP_SILENT_DENY:
            result->success = false;
            break;
        case OS_AP_ALLOW:
            result->success = pageHnd(p, &(result->child));
            break;
    }
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  THIS IS WHERE THE ENGINE ENDS AND THE USER SPACE STARTS                   //
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


static uint8_t getNumberOfActiveProcs(void) {
    uint8_t num = 0;
    for (ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++) {
        if (os_getProcessSlot(pid)->state != OS_PS_UNUSED) num++;
    }
    return num;
}

/*!
 * Front page. Tells you the running process and the number of slots
 * occupied and the total number of slots.
 * Always returns true.
 */
MAKE_PAGEHANDLER(tm_frontpage, tm_null, 0, 0, OS_PR_FRONTPAGE, null, 0) {
    lcd_writeProgString(PSTR("Running: #"));
    lcd_writeDec(os_getCurrentProc());
    lcd_line2();
    lcd_writeProgString(PSTR("Total: "));
    lcd_writeDec(getNumberOfActiveProcs());
    lcd_writeChar('/');
    lcd_writeDec(MAX_NUMBER_OF_PROCESSES);
    return true;
}

// XXX slightly ugly
#define uniqState(state) (((uint32_t)1) << (state))


// procMutator and procMutatorConfirm is only compiled if it is used, thus a warning is avoided
#if (TM_COMPILE_KILL_SUPPORT || TM_COMPILE_PRIORITY_SUPPORT)

/*!
 * This is a convenience routine, as we have several pages that share the task
 * of choosing a process (depending on some condition).
 * Actually 'procMutator' is a slight misnomer, as it is called by pages that
 * potentially may mutate a process, but this will not modify any state.
 * It will merely print the properties of the specified process.
 * \param p This is the parameter stack from which we will derive the process
 *          to display.
 * \param note An extra message to display (usually it indicates what will
 *             happen to the process after the user selects it (e.g. "Kill").
 * \param stateCondition A bit set specifying which state must be fulfilled by
 *                       the run-state of the process.
 * \returns Whether the selected process could be displayed.
 */
static bool procMutator(const ParamStack *p, const char *note, uint32_t stateCondition) {
    const uint16_t page = peekStack(0).param;
    if (!(uniqState(os_getProcessSlot(page)->state) & stateCondition)) {
        return false;
    }
    lcd_writeProgString(note);
    lcd_writeProgString(PSTR(" proc #"));
    lcd_writeDec(page);
    lcd_writeChar('/');
    lcd_writeDec(getNumberOfActiveProcs());
    lcd_line2();
    lcd_writeProgString(PSTR("(prog @ 0x"));
    lcd_writeHexWord((uint16_t)os_getProcessSlot(page)->program);
    lcd_writeChar(')');
    return true;
}

/*!
 * This is the counter-part of procMutator. Actually, the mutation of the
 * process takes place here and not in procMutator.
 * The function will check whether the process can be modified in the
 * specified way, and then calls the callback-handler passed as last argument.
 * \param p The parameter stack from which to derive the process id.
 * \param note1 A note, specifying what is about to be done with the process.
 * \param noteIdle A note, stating that the respective action could not be done
 *                 with the idle process (e.g. you cannot kill process 0).
 *                 Pass NULL as idleNote, in order to allow the operation for
 *                 the idle process.
 * \param action This is the callback function that will be invoked if all
 *               checks are successful. It receives the process id and returns
 *               whether the action could be applied to that process.
 * \returns true.
 */
static bool procMutatorConfirm(const ParamStack *p, const char *note1, const char *noteIdle, bool (*action)(ProcessID)) {
    const uint16_t proc = peekStack(1).param;
    if (proc || !noteIdle) {
        lcd_writeProgString(note1);
        lcd_writeProgString(PSTR(" #"));
        lcd_writeDec(proc);
        if (action(proc)) {
            tm_done();
        } else {
            tm_fail();
        }
    } else {
        lcd_writeProgString(noteIdle);
        tm_fail();
    }
    return true;
}

#endif

#if TM_COMPILE_KILL_SUPPORT

/*!
 * The page to select a process to kill.
 */
MAKE_PAGEHANDLER(tm_killProc, tm_killProc_kill, 0, 1, OS_PR_KILL_SELECT, pid, peekStack(0).param) {
    // We can kill a process if it is not unused.
    return procMutator(p, PSTR("Kill"), ~uniqState(OS_PS_UNUSED));
}

extern ProcessID currentProc;
bool internalKill(ProcessID pid) {
    ProcessID tmp = currentProc;
    currentProc = INVALID_PROCESS;
    bool result = os_kill(pid);
    currentProc = tmp;
    return result;
}

/*!
 * The page to kill a previously selected process.
 */
MAKE_PAGEHANDLER(tm_killProc_kill, tm_null, 0, 0, OS_PR_KILL, pid, peekStack(1).param) {
    return procMutatorConfirm(p, PSTR("Killing"), PSTR("Cannot kill #0"), internalKill);
}

#endif

#if TM_COMPILE_PRIORITY_SUPPORT

/*!
 * The page to select a process to change its priority.
 */
MAKE_PAGEHANDLER(tm_priority, tm_priority_show, 0, 1, OS_PR_PRIORITY_SELECT, pid, peekStack(0).param) {
    // We can change priority of a process if it is not unused.
    return procMutator(p, PSTR("Prty"), ~uniqState(OS_PS_UNUSED));
}

/*!
 * A convenience routine to render the priority of a process, including
 * a mark to indicate that the priority is being changed.
 * This routine guarantees that this will fit precisely on the two lines
 * of the LCD.
 * \param proc The process id for which to render the priority.
 * \param mod Whether to print the modification indicator or not.
 */
static void priorityConstText(uint16_t proc, bool mod) {
    lcd_writeProgString(PSTR("Priority of #"));
    lcd_writeDec(proc);
    lcd_line2();
    if (mod) {
        lcd_writeProgString(spaces16 + (16 - 2));
        lcd_writeChar('*');
        lcd_writeProgString(spaces16 + (16 - 2));
    } else {
        lcd_writeProgString(spaces16 + (16 - 5));
    }
}

/*!
 * The page to display the priority of a previously selected process.
 * The priority cannot be changed in this page.
 */
MAKE_PAGEHANDLER(tm_priority_show, tm_priority_changeH, 0, 16, OS_PR_PRIORITY_SHOW, pid, peekStack(1).param) {
    const uint16_t proc = peekStack(1).param;
    // Index to start the sub-page with.
    result->param = os_getProcessSlot(proc)->priority >> 4;
    priorityConstText(proc, false);
    lcd_writeChar('(');
    lcd_writeHexByte(os_getProcessSlot(proc)->priority);
    lcd_writeChar(')');
    return true;
}

/*!
 * The page to change the high nibble of the priority of a
 * previously selected process.
 */
MAKE_PAGEHANDLER(tm_priority_changeH, tm_priority_changeL, 0, 16, OS_PR_PRIORITY, null, 0) {
    const uint16_t proc = peekStack(2).param;
    // Index to start the sub-page with.
    result->param = os_getProcessSlot(proc)->priority & 0xF;
    priorityConstText(proc, true);
    lcd_writeChar('[');
    lcd_writeHexNibble(peekStack(0).param);
    lcd_writeChar(']');
    lcd_writeHexNibble(os_getProcessSlot(proc)->priority);
    return true;
}

/*!
 * The page to change the low nibble of the priority of a
 * previously selected process.
 * Note that the high nibble is taken from the parameter stack!
 */
MAKE_PAGEHANDLER(tm_priority_changeL, tm_priority_set, 0, 1, OS_PR_PRIORITY, null, 0) {
    const uint16_t proc = peekStack(3).param;
    priorityConstText(proc, true);
    lcd_writeHexNibble(peekStack(1).param);
    lcd_writeChar('[');
    lcd_writeHexNibble(peekStack(0).param);
    lcd_writeChar(']');
    return true;
}

/*!
 * The page to commit (i.e. set) the priority for a previously selected
 * process. The new priority was selected in the previous two pages. Therefore
 * it lies on the parameter stack.
 */
MAKE_PAGEHANDLER(tm_priority_set, tm_null, 0, 0, OS_PR_PRIORITY, pid, peekStack(4).param) {
    lcd_writeProgString(PSTR("Setting priority"));
    os_getProcessSlot(peekStack(4).param)->priority = ((peekStack(2).param & 0xF) << 4) + ((peekStack(1).param & 0xF));
    tm_done();
    lcd_writeProgString(PSTR(", now: "));
    lcd_writeHexByte(os_getProcessSlot(peekStack(4).param)->priority);
    return true;
}

#endif

/*!
 * The prototype to determine the name of any strategy item.
 * If you implement this interface, make sure to conform to its parameter specification
 * \param index Strategy for which to lookup a name. Handling undefined indices is mandatory.
 * \param stringLengthOut Pointer to a location where to store the length of the returned string.
 *                  If NULL is passed, you are to ignore the parameter.
 * \returns A pointer to the prog string that contains the name and is zero-terminated.
 */
typedef const char *StrategyNameLookup(uint16_t index, uint16_t *stringLengthOut);

/*!
 * This is a generic function creator for StrategyNameLookup functions.
 * Provide only the name and all strategy options there are. Like so:
 *   makeStrategyNameLookup(myLookupFunction, 4, PSTR("???"), {ST_ONE, PSTR("one")}, {ST_TWO, PSTR("two")})
 * \param NAME The name of the function you wish to create.
 * \param STRINGLEN The length of the individual strings.
 * \param UNDEFSTRING The string to be returned if the strategy is unknown.
 * \param STRATEGIES Varying list of strategy options.
 */
#define makeStrategyNameLookup(NAME, STRINGLEN, UNDEFSTRING, STRATEGIES...)                           \
    static StrategyNameLookup NAME;                                                                   \
    static char const *NAME(uint16_t ss, uint16_t *stringLengthOut) {                                 \
        struct {                                                                                      \
            size_t strategy;                                                                          \
            char const *name;                                                                         \
        } const index[] = {                                                                           \
            STRATEGIES{ss, UNDEFSTRING}  /* <--- sentinel element (guarantees the loop terminates!) */ \
        };                                                                                            \
        if (stringLengthOut)                                                                          \
            *stringLengthOut = STRINGLEN;                                                             \
        size_t i;                                                                                     \
        /* poor man's associative array ... */                                                        \
        for (i = 0; i < sizeof(index) / sizeof(*index); i++)                                          \
            if (index[i].strategy == ss)                                                              \
                return index[i].name;                                                                 \
        /* { 0 } */                                                                                   \
        return 0; /* unreachable, but the compiler doesn't understand that */                         \
    }

// Only compiled if the function is used, thus avoiding warning
#if TM_COMPILE_SCHEDULING_SUPPORT

/*!
 * This is a generic function that allows us to select a strategy from
 * a list of available strategies. For instance this is used to select the
 * scheduling strategy, but this function has no idea what the semantic of
 * the selected index is. It merely displays it using the passed list of names.
 * \param p Parameter stack to infer the selected strategy index from.
 * \param names A "list" of strings that describe the selected strategy.
 *              This is one string, that contains several sub-strings that are
 *              all of the same length and zero-terminated.
 *              Thus the sub-strings can be accessed in O(1).
 * \param curr The currently "active" strategy. For all others the user will
 *             be invited to hit enter to change the strategy.
 * \returns true.
 */
static bool strategySelector(const ParamStack *p, StrategyNameLookup *names, uint16_t curr) {
    const uint16_t select = peekStack(0).param;
    uint16_t length;
    const char *const name = names(select, &length);
    if (!name) {
        return false;
    }
    lcd_writeProgString(name);
    if (select != curr) {
        lcd_writeProgString(spaces16 + ((length > 16) ? (length - 16) : 0) + 4 - 1);
        lcd_writeProgString(PSTR("set?"));
    }
    return true;
}

/*!
 * Convenience function to actually change a strategy (as opposed to the
 * strategySelector that allows the user to select a strategy).
 * \param p See strategySelector.
 * \param names See strategySelector.
 * \param curr See strategySelector.
 * \param set Callback function that will execute the respective modification.
 *            It receives a void* (See param:passThrough) and a uint16_t, which
 *            carries the information to which value the strategy is to be
 *            changed to.
 * \param passThrough Any POD that is to be passed through to the callback
 *                    function 'set'. This can be required to provide the
 *                    callback with additional information.
 *                    'strategyChanger' will never look at the value, let alone
 *                    dereference it, so feel free to pass NULL, if you do not
 *                    need extra information.
 * \returns true.
 */
static bool strategyChanger(const ParamStack *p, StrategyNameLookup *names, uint16_t curr, void (*set)(void *, uint16_t), void *passThrough) {
    const uint16_t select = peekStack(1).param;
    if (select == curr) {
        lcd_writeProgString(PSTR("no change"));
        tm_fail();
    } else {
        lcd_writeProgString(PSTR("setting "));
        lcd_writeProgString(names(select, 0));
        lcd_goto(1, 16 - 2);
        lcd_writeProgString(PSTR("..."));
        lcd_writeProgString(spaces16);
        set(passThrough, select);
        tm_done();
    }
    return true;
}

#endif

#if TM_COMPILE_SCHEDULING_SUPPORT

makeStrategyNameLookup(getSchedulingStratNames, 0x18, 0,
                       // 123456789abcdef0123456789ABCDEF0
                       {OS_SS_RUN_TO_COMPLETION, PSTR("<Run To Completion>    ")}, {OS_SS_RANDOM, PSTR("<Random>               ")}, {OS_SS_EVEN, PSTR("<Even>                 ")}, {OS_SS_ROUND_ROBIN, PSTR("<Round Robin>          ")}, {OS_SS_INACTIVE_AGING, PSTR("<Inactive Aging>       ")},
#if VERSUCH >= 5
                       {OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE, PSTR("<MLFQ>                 ")},
#endif
)

    /*!
     * The page to select a scheduling strategy to set.
     */
    MAKE_PAGEHANDLER(tm_scheduling, tm_scheduling_set, 0, 1, OS_PR_SCHEDULING_SELECT, ss, peekStack(0).param) {
    return strategySelector(p, getSchedulingStratNames, os_getSchedulingStrategy());
}

void setSS(void *passThrough, uint16_t select) {
    os_setSchedulingStrategy(select);
}

/*!
 * The page to set a previously selected scheduling strategy.
 */
MAKE_PAGEHANDLER(tm_scheduling_set, tm_null, 0, 0, OS_PR_SCHEDULING, ss, peekStack(1).param) {
    return strategyChanger(p, getSchedulingStratNames, os_getSchedulingStrategy(), setSS, 0);
}

#endif

#if TM_COMPILE_HEAP_SUPPORT

static const char *getHeapName(uint8_t ram) {
    Heap *const heap = os_lookupHeap(ram);
    return heap ? (heap->name) : "unnamed";
}

/*!
 * The page to select which heap to inspect. Supports NULL-heaps.
 */
MAKE_PAGEHANDLER(tm_heap, tm_heap2, 0, 4, OS_PR_SHOW_HEAP, heapId, peekStack(0).param) {
    const uint16_t ram = peekStack(0).param;
    if (ram >= os_getHeapListLength() || !os_lookupHeap(ram)) {
        return false;
    }
    lcd_writeProgString(PSTR("Open "));
    lcd_writeString(getHeapName(ram));
    lcd_writeProgString(PSTR(" Heap"));
    return true;
}

// Forward declarations
static tm_page tm_heap_strategy;
static tm_page tm_heap_contents;
static tm_page tm_heap_chunks;
static tm_page tm_heap_erase;

/*!
 * The page to select what to do with a previously selected heap.
 * The current options are:
 *  - select a strategy
 *  - dump the map
 *  - browse chunks
 *  - erase everything
 */
MAKE_PAGEHANDLER(tm_heap2, tm_heap_strategy, 0, MS_MAX_COUNT, OS_PR_ALWAYS_ALLOW, null, 0) {
    Heap *const heap = os_lookupHeap(peekStack(1).param);
    switch (peekStack(0).param) {
        case 0: {
            lcd_writeProgString(PSTR("Alloc. strategy"));
            result->param = os_getAllocationStrategy(heap);
            break;
        }
        case 1: {
            lcd_writeProgString(PSTR("Map content dump"));
            result->call = tm_heap_contents;
            result->param = 0;
            result->range = (os_getUseSize(heap) + TM_MAP_ENTRIES_PER_PAGE - 1) / TM_MAP_ENTRIES_PER_PAGE;
            break;
        }
        case 2: {
            lcd_writeProgString(PSTR("Chunk browser"));
            result->call = tm_heap_chunks;
            result->param = 0;
            result->range = os_getUseSize(heap);
            break;
        }
        case 3: {
            lcd_writeProgString(PSTR("Erase everything"));
            result->call = tm_heap_erase;
            result->param = 0;
            result->range = 1;
            break;
        }
        default:
            return false;
    }
    return true;
}

makeStrategyNameLookup(getAllocationStratNames, 0x10, 0,
                       // 123456789abcdef0123456789ABCDEF0
                       {OS_MEM_FIRST, PSTR("<First Fit>    ")}, {OS_MEM_NEXT, PSTR("<Next Fit>     ")}, {OS_MEM_BEST, PSTR("<Best Fit>     ")}, {OS_MEM_WORST, PSTR("<Worst Fit>    ")}, )

    /*!
     * The page to select an allocation strategy for the previously selected heap.
     */
    MAKE_PAGEHANDLER(tm_heap_strategy, tm_heap_strategy_set, 0, 1, OS_PR_ALLOCATION_SELECT, as, peekStack(0).param) {
    Heap *const heap = os_lookupHeap(peekStack(2).param);
    return strategySelector(p, getAllocationStratNames, os_getAllocationStrategy(heap));
}

void setAS(void *passThrough, uint16_t select) {
    os_setAllocationStrategy((Heap *)passThrough, select);
}

/*!
 * The page to commit (i.e. set) a previously selected allocation strategy
 * for the earlier selected heap.
 */
MAKE_PAGEHANDLER(tm_heap_strategy_set, tm_null, 0, 0, OS_PR_ALLOCATION, as, peekStack(1).param) {
    Heap *const heap = os_lookupHeap(peekStack(3).param);
    return strategyChanger(p, getAllocationStratNames, os_getAllocationStrategy(heap), setAS, heap);
}

static MemValue derefMap(const Heap *heap, MemAddr usePtr) {
    const uint16_t uOff = usePtr - os_getUseStart(heap);
    return (heap->driver->read(os_getMapStart(heap) + uOff / 2) >> (((~uOff) & 1) << 2)) & 0xF;
}

/*!
 * The page to dump the map entries of the previously selected heap.
 */
MAKE_PAGEHANDLER(tm_heap_contents, tm_null, 0, 0, OS_PR_SHOW_HEAP, null, 0) {
    Heap *const heap = os_lookupHeap(peekStack(2).param);
    uint16_t addr = os_getUseStart(heap) + peekStack(0).param * TM_MAP_ENTRIES_PER_PAGE;
    uint8_t i, j;
    for (i = 0; i < 2; i++) {
        lcd_writeHexWord(addr);
        lcd_writeProgString(PSTR(": "));
        for (j = 0; j < 16 - 6; j++) {
            if (addr < os_getUseStart(heap) + os_getUseSize(heap)) {
                lcd_writeHexNibble(derefMap(heap, addr++));
            } else {
                i = j = 16;
            }
        }
    }
    return true;
}

/*!
 * The page to display distinct chunks of the heap.
 * This page will not work correctly with different heap-map specifications
 * you are free to remove or modify this in case the heap-map semantics are
 * changed.
 */
MAKE_PAGEHANDLER(tm_heap_chunks, tm_null, 0, 0, OS_PR_SHOW_HEAP, null, 0) {
    Heap *const heap = os_lookupHeap(peekStack(2).param);
    const uint16_t addr = os_getUseStart(heap) + peekStack(0).param;
    const MemValue owner = derefMap(heap, addr);
    if (owner == 0 || owner == 0xF) {
        return false;
    }
    lcd_writeProgString(PSTR("Chunk @"));
    lcd_writeHexWord(addr);
    lcd_writeProgString(PSTR(" ("));
    lcd_writeChar((owner < MAX_NUMBER_OF_PROCESSES) ? '#' : '*');
    lcd_writeHexNibble(owner);
    lcd_writeChar(')');
    lcd_line2();
    lcd_writeProgString(PSTR("Length: ..."));
    const uint16_t length = os_getChunkSize(heap, addr);
    lcd_goto(2, 9);
    lcd_writeProgString(spaces16 + (16 - 3));
    lcd_goto(2, 9);
    lcd_writeDec(length);
    return true;
}

MAKE_PAGEHANDLER(tm_heap_erase, tm_heap_erase2, 0, 1, OS_PR_ERASE_HEAP, heapId, peekStack(2).param) {
    lcd_writeProgString(PSTR("Erase map+dat of"));
    lcd_writeString(getHeapName(peekStack(2).param));
    lcd_writeChar('?');
    return true;
}

MAKE_PAGEHANDLER(tm_heap_erase2, tm_null, 0, 0, OS_PR_ERASE_HEAP, heapId, peekStack(3).param) {
    lcd_writeProgString(PSTR("Erasing "));
    lcd_writeString(getHeapName(peekStack(3).param));
    lcd_writeProgString(PSTR("..."));
    Heap *const heap = os_lookupHeap(peekStack(3).param);
    MemAddr start = os_getMapStart(heap);
    MemAddr end = os_getMapStart(heap) + os_getMapSize(heap);
    const MemAddr mapEnd = end;
    MemAddr ptr;
    uint8_t lastProgress = 0;
    for (ptr = start; ptr < end; ptr++) {
        heap->driver->write(ptr, 0);
        uint8_t progress = (100ul * (uint16_t)(ptr - start)) / (uint16_t)(end - start);
        if (((uint16_t)progress * 32ul) / 100ul != ((uint16_t)lastProgress * 32ul) / 100ul) {
            lcd_drawBar((lastProgress = progress));
        }
        if (ptr + 1 == mapEnd) {
            ptr = (start = os_getUseStart(heap)) - 1;
            end = os_getUseStart(heap) + os_getUseSize(heap);
        }
    }
    tm_done();
    return true;
}

#endif

#pragma GCC pop_options
