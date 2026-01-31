//===-- Cpu0SEISelLowering.cpp - Cpu0SE DAG Lowering Interface --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Subclass of Cpu0TargetLowering specialized for cpu032.
//
//===----------------------------------------------------------------------===//
#include "Cpu0SEISelLowering.h"
#include "Cpu0MachineFunction.h"

#include "Cpu0RegisterInfo.h"
#include "Cpu0TargetMachine.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "cpu0-isel"

static cl::opt<bool> EnableCpu0TailCalls("enable-cpu0-tail-calls", cl::Hidden,
                                         cl::desc("CPU0: Enable tail calls."),
                                         cl::init(false));

/* NOTE(fh):
 ! SE (standard edition) constructor configures legality + register classes
 * 
 * Runs the base class constructor fisrt ": Cpu0TargetLowering(TM, STI)"
 * 
 */
//@Cpu0SETargetLowering {
Cpu0SETargetLowering::Cpu0SETargetLowering(const Cpu0TargetMachine &TM,
                                           const Cpu0Subtarget &STI)
    : Cpu0TargetLowering(TM, STI) {
  //@Cpu0SETargetLowering body {
  // Set up the register classes
  /* NOTE(fh):
   * tells llvm: Values of value type i32 can live in register class RC on target
   *
   * Cpu0::CPURegsRegClass is TableGen-generated
   */
  addRegisterClass(MVT::i32, &Cpu0::CPURegsRegClass);

  // must, computeRegisterProperties - Once all of the register classes are
  //  added, this allows us to compute derived properties we expose.
  computeRegisterProperties(Subtarget.getRegisterInfo());
}

SDValue Cpu0SETargetLowering::LowerOperation(SDValue Op,
                                             SelectionDAG &DAG) const {

  return Cpu0TargetLowering::LowerOperation(Op, DAG);
}

/* NOTE(fh):
 * For now, Cpu0's factory always returns the "SE" variant
 *
 * In the Cpu0SubTarget ctor: TLInfo(Cpu0TargetLowering::create(TM, *this))
 *
 * The construction chain right now is:
 * Cpu0Subtarget 
 * → Cpu0TargetLowering::create 
 * → createCpu0SETargetLowering 
 * → new Cpu0SETargetLowering
 */
const Cpu0TargetLowering *
llvm::createCpu0SETargetLowering(const Cpu0TargetMachine &TM,
                                 const Cpu0Subtarget &STI) {
  return new Cpu0SETargetLowering(TM, STI);
}
