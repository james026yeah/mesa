//===-- AMDGPURegisterInfo.h - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TODO: Add full description
//
//===----------------------------------------------------------------------===//

#ifndef AMDGPUREGISTERINFO_H_
#define AMDGPUREGISTERINFO_H_

#include "AMDILRegisterInfo.h"

namespace llvm {

  class AMDGPUTargetMachine;
  class TargetInstrInfo;

  struct AMDGPURegisterInfo : public AMDILRegisterInfo
  {
    AMDGPUTargetMachine &TM;
    const TargetInstrInfo &TII;

    AMDGPURegisterInfo(AMDGPUTargetMachine &tm, const TargetInstrInfo &tii);

    virtual BitVector getReservedRegs(const MachineFunction &MF) const = 0;

    virtual const TargetRegisterClass *
    getISARegClass(const TargetRegisterClass * rc) const = 0;
  };
} // End namespace llvm

#endif // AMDIDSAREGISTERINFO_H_
