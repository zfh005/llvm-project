# LLVM Cpu0 Backend (Ch 3-1) — Big Picture Wiring Notes

This chapter section is mostly **plumbing**: it wires CPU0-specific components into LLVM’s generic codegen pipeline so LLVM can ask the backend “target questions” (registers, stack frame, instruction behavior, lowering rules, object emission policy).

---

## 1) Core mental model

LLVM codegen = **generic pipeline** + **target bundle of answers**.

LLVM repeatedly needs answers to questions like:
- What registers exist? Which are caller/callee-saved?
- How do I lower IR ops/calls into target instructions?
- How do I build stack frames (prologue/epilogue)?
- How do globals/sections/relocations get emitted into object files?
- What endianness / data layout rules apply?

CPU0 provides these answers via a handful of target objects.

---

## 2) Who owns what (the wiring diagram)

### `Cpu0TargetMachine` — top-level configuration
**Role:** Represents “compile for CPU0 with this triple/CPU/features/opts/endianness”.

**Owns / provides:**
- `DefaultSubtarget` (fallback subtarget)
- `TLOF` (TargetLoweringObjectFile): object-file emission policy
- `ABI` (Cpu0ABIInfo): calling convention + ABI rules
- can provide **per-function subtargets** via a SubtargetMap (if function attrs differ)

**Where used:**
- created by `llc -march=cpu0` / Clang target selection
- used to build pass pipeline: `createPassConfig(...)`

---

### `Cpu0Subtarget` — the feature-specific hub (“answer sheet”)
**Role:** Central object in backend wiring. Given CPU + feature bits, it returns target component objects.

`Cpu0Subtarget` exposes:
- `getInstrInfo()`
- `getRegisterInfo()`
- `getFrameLowering()`
- `getTargetLowering()`
- `getSelectionDAGInfo()`
- `getDataLayout()`

**Key idea:** Many passes and lowering code start with:
> “Ask the Subtarget for the target-specific piece you need.”

---

## 3) Component objects and what they answer

### `Cpu0InstrInfo`
Instruction-level facts + utilities:
- instruction properties (branch, call, terminator, etc.)
- inserting copies / loads / stores for spills
- branch analysis and branch fixups

Sometimes specialized by sub-variant (e.g., `Cpu0SEInstrInfo`).

---

### `Cpu0RegisterInfo`
Register-file + ABI register conventions:
- physical registers list + register classes
- caller/callee-saved sets
- reserved regs (SP, FP, zero reg, etc.)
- constraints affecting register allocation

---

### `Cpu0FrameLowering`
Stack frame mechanics:
- stack growth direction
- prologue/epilogue emission
- stack object layout and alignment
- spill/restore of callee-saved registers

---

### `Cpu0TargetLowering` (inherits `TargetLowering`)
**SelectionDAG lowering brain**:
- how LLVM IR ops become target DAG nodes / instructions
- calling convention lowering (args/returns)
- custom lowering hooks for target-specific operations (shifts, compares, selects, etc.)

---

### `Cpu0TargetObjectFile` (inherits `TargetLoweringObjectFile`)
Object emission policy:
- which sections globals go to (`.text`, `.data`, `.rodata`, `.bss`)
- relocation / symbol / section choices (often ELF-oriented)

---

## 4) How the pipeline uses these objects

### 1) Instruction Selection (SelectionDAG)
- uses `Subtarget.getTargetLowering()` for IR/DAG lowering decisions
- uses `Subtarget.getInstrInfo()` / `getRegisterInfo()` to construct legal machine instructions

### 2) Machine passes + Register Allocation
- heavily consult `RegisterInfo` and `InstrInfo`

### 3) Prologue/Epilogue insertion
- uses `FrameLowering` + ABI rules (`Cpu0ABIInfo`)

### 4) Asm/Object emission
- uses instruction + MC layer
- uses `TargetObjectFile` for sections/relocations policy

---

## 5) Why Ch 3-1 feels like “too much code”
It’s mostly constructors/getters and object ownership:
- not the “algorithm”
- but the *plumbing that lets LLVM find your algorithm* later

Once this wiring exists, later chapters can say:
> “call `Subtarget.getTargetLowering()`”
and it works.

---

## 6) Ultra-short cheat sheet

- **TargetMachine**: compile configuration + pass pipeline entry
- **Subtarget**: feature-specific hub returning target components
- **InstrInfo**: instruction behavior utilities
- **RegisterInfo**: registers + ABI register rules
- **FrameLowering**: stack frame + prologue/epilogue
- **TargetLowering**: IR/DAG lowering rules
- **TargetObjectFile**: section/relocation policy

---
