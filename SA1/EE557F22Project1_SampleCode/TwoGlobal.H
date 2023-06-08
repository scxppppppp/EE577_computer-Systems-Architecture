/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2012 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
class TWOGLOBAL
{
  public:
    TWOGLOBAL();
    UINT64 References() const { return _references;};
    UINT64 Predicts() const { return _predicts;};
    UINT64 Mispredicts() const { return _references - _predicts;};
    VOID Activate()
    {
        INS_AddInstrumentFunction(Instruction, this);
    }

  private:
    enum 
    {
        TABLESIZE = 1,
        BUNDLESIZE = 1,
        MAXTHREADS = 100
    };

    static VOID Instruction(INS ins, VOID *v);
    static VOID CondBranch(TWOGLOBAL *tg, INT8 * branchHistory, VOID * ip,  BOOL taken);
    inline INT8 * HistAddress(INS ins);

    INT8 _branchHistory[TABLESIZE][BUNDLESIZE];
    UINT64 _references, _predicts;
};

inline TWOGLOBAL::TWOGLOBAL()
{
    _references = 0;
    _predicts = 0;
}

inline VOID TWOGLOBAL::Instruction(INS ins, VOID *v)
{
    TWOGLOBAL *tg = static_cast<TWOGLOBAL*>(v);
    if (INS_IsBranchOrCall(ins) && INS_HasFallThrough(ins))
        {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, 
                           (AFUNPTR)CondBranch, IARG_PTR, (void *)tg, IARG_PTR, (void *)tg->HistAddress(ins), IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_END);
        }
}

inline INT8 * TWOGLOBAL::HistAddress(INS ins)
{
    ADDRINT ip = INS_Address(ins);
    return &(_branchHistory[(ip / BUNDLESIZE) % TABLESIZE][ip % BUNDLESIZE]);
}

inline VOID TWOGLOBAL::CondBranch(TWOGLOBAL *tg, INT8 * branchHistory, 
VOID * ip, BOOL taken)
{
    INT8 history;

    tg->_references++;
  
    history = *branchHistory & 0x3;
    tg->_predicts += (taken && history >= 2);
    tg->_predicts += (!taken && history <= 1);

    INT8 delta = 0;
    if (taken && history < 3)
    {
        delta = 1;
    }
    if (!taken && history > 0)
    {
        delta = -1;
    }
  
    *branchHistory += delta;
}