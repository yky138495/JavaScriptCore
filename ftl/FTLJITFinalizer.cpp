/*
 * Copyright (C) 2013-2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "FTLJITFinalizer.h"

#if ENABLE(FTL_JIT)

#include "CodeBlockWithJITType.h"
#include "DFGPlan.h"
#include "FTLState.h"
#include "FTLThunks.h"
#include "JSCInlines.h"
#include "ProfilerDatabase.h"

namespace JSC { namespace FTL {

JITFinalizer::JITFinalizer(DFG::Plan& plan)
    : Finalizer(plan)
{
}

JITFinalizer::~JITFinalizer()
{
}

size_t JITFinalizer::codeSize()
{
    size_t result = 0;

    if (b3CodeLinkBuffer)
        result += b3CodeLinkBuffer->size();
    
    if (entrypointLinkBuffer)
        result += entrypointLinkBuffer->size();
    
    return result;
}

bool JITFinalizer::finalize()
{
    return finalizeCommon();
}

bool JITFinalizer::finalizeFunction()
{
    return finalizeCommon();
}

bool JITFinalizer::finalizeCommon()
{
    bool dumpDisassembly = shouldDumpDisassembly() || Options::asyncDisassembly();
    
    MacroAssemblerCodeRef<JSEntryPtrTag> b3CodeRef =
        FINALIZE_CODE_IF(dumpDisassembly, *b3CodeLinkBuffer, JSEntryPtrTag,
            "FTL B3 code for %s", toCString(CodeBlockWithJITType(m_plan.codeBlock, JITCode::FTLJIT)).data());

    MacroAssemblerCodeRef<JSEntryPtrTag> arityCheckCodeRef = entrypointLinkBuffer
        ? FINALIZE_CODE_IF(dumpDisassembly, *entrypointLinkBuffer, JSEntryPtrTag,
            "FTL entrypoint thunk for %s with B3 generated code at %p", toCString(CodeBlockWithJITType(m_plan.codeBlock, JITCode::FTLJIT)).data(), function)
        : MacroAssemblerCodeRef<JSEntryPtrTag>::createSelfManagedCodeRef(b3CodeRef.code());

    jitCode->initializeB3Code(b3CodeRef);
    jitCode->initializeArityCheckEntrypoint(arityCheckCodeRef);

    m_plan.codeBlock->setJITCode(*jitCode);

    if (UNLIKELY(m_plan.compilation))
        m_plan.vm->m_perBytecodeProfiler->addCompilation(m_plan.codeBlock, *m_plan.compilation);
    
    return true;
}

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

