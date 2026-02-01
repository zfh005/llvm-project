//===-- Cpu0ISelDAGToDAG.cpp - A Dag to Dag Inst Selector for Cpu0 --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the CPU0 target.
//
//===----------------------------------------------------------------------===//

#include "Cpu0ISelDAGToDAG.h"
#include "Cpu0.h"

#include "Cpu0MachineFunction.h"
#include "Cpu0RegisterInfo.h"
#include "Cpu0SEISelDAGToDAG.h"
#include "Cpu0TargetMachine.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;

#define DEBUG_TYPE "cpu0-isel"

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Cpu0DAGToDAGISel - CPU0 specific code to select CPU0 machine
// instructions for SelectionDAG operations.
//===----------------------------------------------------------------------===//

bool Cpu0DAGToDAGISel::runOnMachineFunction(MachineFunction &MF) {
  bool Ret = SelectionDAGISel::runOnMachineFunction(MF);

  return Ret;
}

/* NOTE(fh):
 * In .td:
 * def addr : ComplexPattern<iPTR, 2, "SelectAddr", [frameindex],
 * [SDNPWantParent]>;
 * - Address operands in LD/ST patterns use addr
 * - addr expands into two operands (base, offset)
 * - The C++ "SelectAddr" must fill thouse outputs
 *
 * This is for the selection of "DATA DAG node with addr type"
 * This choose the addressing operands (operand decomposition)
 * The address expression in the DAG can be many shapes:
 * add(base, imm), add(reg, reg), Wrapper(GlobalAddress)
 * But Cpu0 LD/ST instr would only want operands in the form:
 *    LD Base, Offset
 * llvm need a way to take an arbitrary DAG address expression and decompose it
 * into the operands which instruction would expect.
 *
 * PatFrag vs ComplexPattern
 * - PatFrag: helps match the operation node (e.g. aligned store with certain
 *            contraints)
 *            -> a named DAG-op matcher (with optional predicate)
 * - ComplexPattern: helps match/decompose an operand (addr -> base + offset)
 *                   -> a named operand matcher/decomposer implemented in C++
 */
//@SelectAddr {
/// ComplexPattern used on Cpu0InstrInfo
/// Used on Cpu0 Load/Store instructions
bool Cpu0DAGToDAGISel::SelectAddr(SDNode *Parent, SDValue Addr, SDValue &Base,
                                  SDValue &Offset) {
  //@SelectAddr }
  EVT ValTy = Addr.getValueType();
  SDLoc DL(Addr);

  // If Parent is an unaligned f32 load or store, select a (base + index)
  // floating point load/store instruction (luxc1 or suxc1).
  const LSBaseSDNode *LS = 0;

  if (Parent && (LS = dyn_cast<LSBaseSDNode>(Parent))) {
    EVT VT = LS->getMemoryVT();

    if (VT.getSizeInBits() / 8 > LS->getAlignment()) {
      assert(0 && "Unaligned loads/stores not supported for this type.");
      if (VT == MVT::f32)
        return false;
    }
  }

  // if Address is FI, get the TargetFrameIndex.
  if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), ValTy);
    Offset = CurDAG->getTargetConstant(0, DL, ValTy);
    return true;
  }

  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, DL, ValTy);
  return true;
}

/* NOTE(fh):
 ! This is for the selection of "OP code DAG Node"
 * This choose the instruction (opcode-level)
 * Called for every node in SelectionDAG (ISD::ADD, Cpu0ISD::Ret)
 */
//@Select {
/// Select instructions not customized! Used for
/// expanded, promoted and normal instructions
void Cpu0DAGToDAGISel::Select(SDNode *Node) {
  //@Select }
  unsigned Opcode = Node->getOpcode();

  // If we have a custom node, we already have selected!
  if (Node->isMachineOpcode()) {
    LLVM_DEBUG(errs() << "== "; Node->dump(CurDAG); errs() << "\n");
    Node->setNodeId(-1);
    return;
  }

  // See if subclasses can handle this node.
  /* NOTE(fh):
   * trySelect is a pure virtual function, and is overridden by the derived
   * class that handles target specific cases -> subtarget custom hook
   */
  if (trySelect(Node))
    return;

  switch (Opcode) {
  default:
    break;
  }

  /* NOTE(fh):
   * SelectCode is the TableGen-generated matcher
   * - It tries patterns from .td
   * - If none match, it will throw an error
   */
  // Select the default instruction
  SelectCode(Node);
}
