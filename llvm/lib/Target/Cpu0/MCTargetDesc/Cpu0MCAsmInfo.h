//===-- Cpu0MCAsmInfo.h - Cpu0 Asm Info ------------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the Cpu0MCAsmInfo class.
//
//===----------------------------------------------------------------------===//

/* NOTE(fh):
 * Defines assembler syntax and general asm "policy" for the target
 * These are global asm formatting rules
 */

#ifndef LLVM_LIB_TARGET_CPU0_MCTARGETDESC_CPU0MCASMINFO_H
#define LLVM_LIB_TARGET_CPU0_MCTARGETDESC_CPU0MCASMINFO_H

#include "Cpu0Config.h"

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
  class Triple;

  class Cpu0MCAsmInfo : public MCAsmInfoELF {
    void anchor() override;
  public:
    explicit Cpu0MCAsmInfo(const Triple &TheTriple);
  };

} // namespace llvm

#endif

