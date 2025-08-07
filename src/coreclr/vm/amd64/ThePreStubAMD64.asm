; Licensed to the .NET Foundation under one or more agreements.
; The .NET Foundation licenses this file to you under the MIT license.

include <AsmMacros.inc>
include AsmConstants.inc

        extern  PreStubWorker:proc
        extern  ProcessCLRException:proc

NESTED_ENTRY ThePreStub, _TEXT, ProcessCLRException

        PROLOG_WITH_TRANSITION_BLOCK

        ;
        ; call PreStubWorker
        ;
        lea             rcx, [rsp + __PWTB_TransitionBlock]     ; pTransitionBlock*
        mov             rdx, METHODDESC_REGISTER
        call            PreStubWorker

        EPILOG_WITH_TRANSITION_BLOCK_TAILCALL
        TAILJMP_RAX

NESTED_END ThePreStub, _TEXT

end
