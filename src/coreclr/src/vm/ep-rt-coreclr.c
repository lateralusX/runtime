#include <config.h>

#ifdef ENABLE_PERFTRACING
#include "ep-rt-config.h"
#include "ep-types.h"
#include "ep-rt.h"
#include "ep-stack-contents.h"

ep_rt_lock_handle_t _ep_rt_coreclr_config_lock_handle;
CrstStatic _ep_rt_coreclr_config_lock;

thread_local EventPipeCoreCLRThreadHolderTLS EventPipeCoreCLRThreadHolderTLS::g_threadHolderTLS;

ep_char8_t *_ep_rt_coreclr_diagnostics_cmd_line;

#ifndef TARGET_UNIX
uint32_t *_ep_rt_coreclr_proc_group_offsets;
#endif

/*
 * Forward declares of all static functions.
 */

static
StackWalkAction
rt_coreclr_stack_walk_callback (
	CrawlFrame *frame,
	EventPipeStackContents *stack_contents);

static
StackWalkAction
rt_coreclr_stack_walk_callback (
	CrawlFrame *frame,
	EventPipeStackContents *stack_contents)
{
	CONTRACTL
	{
		NOTHROW;
		GC_NOTRIGGER;
		MODE_ANY;
		PRECONDITION(frame != NULL);
		PRECONDITION(stack_contents != NULL);
	}
	CONTRACTL_END;

	// Get the IP.
	UINT_PTR control_pc = (UINT_PTR)frame->GetRegisterSet ()->ControlPC;
	if (control_pc == NULL) {
		if (ep_stack_contents_get_length (stack_contents) == 0) {
			// This happens for pinvoke stubs on the top of the stack.
			return SWA_CONTINUE;
		}
	}

	EP_ASSERT (control_pc != NULL);

	// Add the IP to the captured stack.
	ep_stack_contents_append (stack_contents, control_pc, frame->GetFunction ());

	// Continue the stack walk.
	return SWA_CONTINUE;
}

bool
ep_rt_coreclr_walk_managed_stack_for_thread (
	ep_rt_thread_handle_t thread,
	EventPipeStackContents *stack_contents)
{
	CONTRACTL
	{
		NOTHROW;
		GC_NOTRIGGER;
		MODE_ANY;
		PRECONDITION(thread != NULL);
		PRECONDITION(stack_contents != NULL);
	}
	CONTRACTL_END;

	// Calling into StackWalkFrames in preemptive mode violates the host contract,
	// but this contract is not used on CoreCLR.
	CONTRACT_VIOLATION (HostViolation);

	// Before we call into StackWalkFrames we need to mark GC_ON_TRANSITIONS as FALSE
	// because under GCStress runs (GCStress=0x3), a GC will be triggered for every transition,
	// which will cause the GC to try to walk the stack while we are in the middle of walking the stack.
	bool gc_on_transitions = GC_ON_TRANSITIONS (FALSE);

	StackWalkAction result = thread->StackWalkFrames (
		(PSTACKWALKFRAMESCALLBACK)rt_coreclr_stack_walk_callback,
		stack_contents,
		ALLOW_ASYNC_STACK_WALK | FUNCTIONSONLY | HANDLESKIPPEDFRAMES | ALLOW_INVALID_OBJECTS);

	GC_ON_TRANSITIONS (gc_on_transitions);
	return ((result == SWA_DONE) || (result == SWA_CONTINUE));
}

#endif /* ENABLE_PERFTRACING */
