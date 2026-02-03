//===-- Cpu0ISelLowering.cpp - Cpu0 DAG Lowering Implementation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Cpu0 uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//
#include "Cpu0ISelLowering.h"

#include "Cpu0MachineFunction.h"
#include "Cpu0Subtarget.h"
#include "Cpu0TargetMachine.h"
#include "Cpu0TargetObjectFile.h"
#include "MCTargetDesc/Cpu0BaseInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "cpu0-lower"

//@3_1 1 {
const char *Cpu0TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case Cpu0ISD::JmpLink:
    return "Cpu0ISD::JmpLink";
  case Cpu0ISD::TailCall:
    return "Cpu0ISD::TailCall";
  case Cpu0ISD::Hi:
    return "Cpu0ISD::Hi";
  case Cpu0ISD::Lo:
    return "Cpu0ISD::Lo";
  case Cpu0ISD::GPRel:
    return "Cpu0ISD::GPRel";
  case Cpu0ISD::Ret:
    return "Cpu0ISD::Ret";
  case Cpu0ISD::EH_RETURN:
    return "Cpu0ISD::EH_RETURN";
  case Cpu0ISD::DivRem:
    return "Cpu0ISD::DivRem";
  case Cpu0ISD::DivRemU:
    return "Cpu0ISD::DivRemU";
  case Cpu0ISD::Wrapper:
    return "Cpu0ISD::Wrapper";
  default:
    return NULL;
  }
}
//@3_1 1 }

//@Cpu0TargetLowering {
Cpu0TargetLowering::Cpu0TargetLowering(const Cpu0TargetMachine &TM,
                                       const Cpu0Subtarget &STI)
    : TargetLowering(TM), Subtarget(STI), ABI(TM.getABI()) {

  // Cpu0 Custom Operations

  // Operations not directly supported by Cpu0.

  //- Set .align 2
  // It will emit .align 2 later
  setMinFunctionAlignment(Align(2));
}

const Cpu0TargetLowering *
Cpu0TargetLowering::create(const Cpu0TargetMachine &TM,
                           const Cpu0Subtarget &STI) {
  return llvm::createCpu0SETargetLowering(TM, STI);
}

//===----------------------------------------------------------------------===//
//  Lower helper functions
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Misc Lower Operation implementation
//===----------------------------------------------------------------------===//

#include "Cpu0GenCallingConv.inc"

//===----------------------------------------------------------------------===//
//@            Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//

//@LowerFormalArguments {
/// LowerFormalArguments - transform physical registers into virtual registers
/// and generate load operations for arguments places on the stack.
SDValue Cpu0TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  Cpu0FunctionInfo *Cpu0FI = MF.getInfo<Cpu0FunctionInfo>();

  Cpu0FI->setVarArgsFrameIndex(0);

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  Cpu0CC Cpu0CCInfo(CallConv, ABI.IsO32(), CCInfo);

  Cpu0FI->setFormalArgInfo(CCInfo.getNextStackOffset(),
                           Cpu0CCInfo.hasByValArg());

  return Chain;
}
// @LowerFormalArguments }

//===----------------------------------------------------------------------===//
//@              Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

/* NOTE(fh):
 * LowerReturn() would build the return DAG correctly
 ? Outs vs OutVals
 * This target lowering must build a selectionDAG sequence that means:
 * Before leaving the function: copy the return values into ABI-mandated
 * physical registers, then emit a target return node that acts as the
 * terminator
 *
 * It returns a SDValue that is the *new chain* node of type Other
 *
 * Chain: enforces ordering of side-effect operations (copies, stores, calls,
 * returns)
 *
 * Flag(glue): enforces tight bundling so scheduler won't separate "copy return
 * value" from "return"
 */

/* NOTE(fh):
 * Walk through with example: return i32 0
 *
 * The two problems llvm must prevent
 * When generating:
 * - CopyToReg %V0, 0
 * - Ret ... uses %V0
 * llvm must guarantee:
 * 1. Ordering: the copy happens before the return
 * 2. Adjacency/bundling: the scheduler doesn't float other stuff in between in
 * a way that breaks constraints
 *
 * SelectionDAG uses two separate mechanisms:
 * - Chain = "this has side effects / must be ordered"
 *  - A Chain is a special DAG of type MVT::Other that represents the ordered
 *    sequence of side-effecting operations
 * - Glue = "Keep these nodes stuck toghther / don't let shceduling split them"
 */

SDValue
Cpu0TargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                bool IsVarArg,
                                const SmallVectorImpl<ISD::OutputArg> &Outs,
                                const SmallVectorImpl<SDValue> &OutVals,
                                const SDLoc &DL, SelectionDAG &DAG) const {
  // CCValAssign - represent the assignment of
  // the return value to a location
  /* NOTE(fh):
   * Step A: compute where does the retuen value go
   *
   * Outs describes return values: here is one return value of type i32
   *
   * RVLocs becomes something like: return value #0 goes in V0(i32)...
   * This is driven by RetCC_Cpu0
   * Mental model: RVLocs[0]: value #0 (i32) is returned in V0
   */
  SmallVector<CCValAssign, 16> RVLocs;
  MachineFunction &MF = DAG.getMachineFunction();

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, *DAG.getContext());
  Cpu0CC Cpu0CCInfo(CallConv, ABI.IsO32(), CCInfo);

  // Analyze return values.
  Cpu0CCInfo.analyzeReturn(Outs, Subtarget.abiUsesSoftFloat(),
                           MF.getFunction().getReturnType());

  /* NOTE(fh):
   * Step B: prepare the operand list for the return node
   *
   * RetOps starts with [Chain]
   *
   * Later, it will become something like:
   * [Chain_after_copies, Register(V0), (maybe Register(v1)), (maybe Flag)]
   */
  SDValue Flag;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  /* NOTE(fh):
   * Step C: loop over each return value and emit CopyToReg into the assigned
   * return register
   */
  // Copy the result values into the output registers.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    SDValue Val = OutVals[i];    //* for ret i32 0, Val = Constant 0 node
    CCValAssign &VA = RVLocs[i]; //* VA: loc is reg V0, locVT is i32
    assert(VA.isRegLoc() && "Can only return in registers!");

    if (RVLocs[i].getValVT() != RVLocs[i].getLocVT())
      Val = DAG.getNode(ISD::BITCAST, DL, RVLocs[i].getLocVT(), Val);

    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), Val, Flag);

    // Guarantee that all emitted copies are stuck together with flags.
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
    /* NOTE(fh):
     * The DAG would now look like:
     * t0: ch = EntryToken
     * t3: ch,glue = CopyToReg t0, Register:i32 %V0, Constant:i32<0>
     * 
     * t3 produces a new Chain due to CopyToReg
     * 
     * getCopyToReg() produces a node whose result#0 is the new chain,
     * and whose result #1 is the new glue(Flag)
     */
  }

  /* NOTE(fh):
   * SDNode and SDVlue:
   *  - SDNode: the object
   *  - SDValue: a handle to one particular result produced by that object
   * 
   * SDNode: "an instruction-like node" in the DAG
   *  - Represent an operation like: ADD, CopyToReg, Cpu0ISD...
   *  - One SDNode can produce multiple results
   * 
   * SDValue: "node pointer, result index"
   *  - SDValue(t3, 0) -> the chain result
   *  - SDValue(t3, 1) -> the glue result
   */

  //@Ordinary struct type: 2 {
  // The cpu0 ABIs for returning structs by value requires that we copy
  // the sret argument into $v0 for the return. We saved the argument into
  // a virtual register in the entry block, so now we copy the value out
  // and into $v0.
  if (MF.getFunction().hasStructRetAttr()) {
    Cpu0FunctionInfo *Cpu0FI = MF.getInfo<Cpu0FunctionInfo>();
    unsigned Reg = Cpu0FI->getSRetReturnReg();

    if (!Reg)
      llvm_unreachable("sret virtual register not created in the entry block");
    SDValue Val =
        DAG.getCopyFromReg(Chain, DL, Reg, getPointerTy(DAG.getDataLayout()));
    unsigned V0 = Cpu0::V0;

    Chain = DAG.getCopyToReg(Chain, DL, V0, Val, Flag);
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(V0, getPointerTy(DAG.getDataLayout())));
  }
  //@Ordinary struct type: 2 }

  /* NOTE(fh):
   * Step D: finish the return node with correct operands + optional flag
   */
  RetOps[0] = Chain; // Update chain.

  // Add the flag if we have it.
  if (Flag.getNode())
    RetOps.push_back(Flag);

  // Return on Cpu0 is always a "ret $lr"
  return DAG.getNode(Cpu0ISD::Ret, DL, MVT::Other, RetOps);
  /* NOTE(fh):
   * The return node would become:
   * t4: ch = Cpu0::Ret t3, Register:i32 %V0, t3:1
   * 
   * Ret takes t3 as a chain input so ordering is enforced
   * 
   * t3:1 result#1 of t3(the glue)
   * 
   * It means Return, but only after chain t3 completes, and treat %V0 as a live
   * return register; glue it with t3:1 so it stays to the next copy
   */
}

Cpu0TargetLowering::Cpu0CC::Cpu0CC(
    CallingConv::ID CC, bool IsO32_, CCState &Info,
    Cpu0CC::SpecialCallingConvType SpecialCallingConv_)
    : CCInfo(Info), CallConv(CC), IsO32(IsO32_) {
  // Pre-allocate reserved argument area.
  CCInfo.AllocateStack(reservedArgArea(), Align(1));
}

template <typename Ty>
void Cpu0TargetLowering::Cpu0CC::analyzeReturn(
    const SmallVectorImpl<Ty> &RetVals, bool IsSoftFloat,
    const SDNode *CallNode, const Type *RetTy) const {
  CCAssignFn *Fn;

  /* NOTE(fh):
   * Runtime call to the calling-conv assignment function generated/connected from .td
   */

  Fn = RetCC_Cpu0;

  for (unsigned I = 0, E = RetVals.size(); I < E; ++I) {
    MVT VT = RetVals[I].VT;
    ISD::ArgFlagsTy Flags = RetVals[I].Flags;
    MVT RegVT = this->getRegVT(VT, IsSoftFloat);

    if (Fn(I, VT, RegVT, CCValAssign::Full, Flags, this->CCInfo)) {
#ifndef NDEBUG
      dbgs() << "Call result #" << I << " has unhandled type "
             << EVT(VT).getEVTString() << '\n';
#endif
      llvm_unreachable(nullptr);
    }
  }
}

void Cpu0TargetLowering::Cpu0CC::analyzeCallResult(
    const SmallVectorImpl<ISD::InputArg> &Ins, bool IsSoftFloat,
    const SDNode *CallNode, const Type *RetTy) const {
  analyzeReturn(Ins, IsSoftFloat, CallNode, RetTy);
}

void Cpu0TargetLowering::Cpu0CC::analyzeReturn(
    const SmallVectorImpl<ISD::OutputArg> &Outs, bool IsSoftFloat,
    const Type *RetTy) const {
  analyzeReturn(Outs, IsSoftFloat, nullptr, RetTy);
}

unsigned Cpu0TargetLowering::Cpu0CC::reservedArgArea() const {
  return (IsO32 && (CallConv != CallingConv::Fast)) ? 8 : 0;
}

MVT Cpu0TargetLowering::Cpu0CC::getRegVT(MVT VT, bool IsSoftFloat) const {
  if (IsSoftFloat || IsO32)
    return VT;

  return VT;
}
