//===-- AMDILISelDAGToDAG.cpp - A dag to dag inst selector for AMDIL ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the AMDIL target.
//
//===----------------------------------------------------------------------===//
#include "AMDILDevices.h"
#include "AMDILTargetMachine.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// AMDILDAGToDAGISel - AMDIL specific code to select AMDIL machine instructions
// //for SelectionDAG operations.
//
namespace {
class AMDILDAGToDAGISel : public SelectionDAGISel {
  // Subtarget - Keep a pointer to the AMDIL Subtarget around so that we can
  // make the right decision when generating code for different targets.
  const AMDILSubtarget &Subtarget;
public:
  AMDILDAGToDAGISel(AMDILTargetMachine &TM AMDIL_OPT_LEVEL_DECL);
  virtual ~AMDILDAGToDAGISel();
  inline SDValue getSmallIPtrImm(unsigned Imm);

  SDNode *Select(SDNode *N);
  // Complex pattern selectors
  bool SelectADDRParam(SDValue Addr, SDValue& R1, SDValue& R2);
  bool SelectADDR(SDValue N, SDValue &R1, SDValue &R2);
  bool SelectADDR64(SDValue N, SDValue &R1, SDValue &R2);
  static bool isGlobalStore(const StoreSDNode *N);
  static bool isPrivateStore(const StoreSDNode *N);
  static bool isLocalStore(const StoreSDNode *N);
  static bool isRegionStore(const StoreSDNode *N);

  static bool isCPLoad(const LoadSDNode *N);
  static bool isConstantLoad(const LoadSDNode *N, int cbID);
  static bool isGlobalLoad(const LoadSDNode *N);
  static bool isPrivateLoad(const LoadSDNode *N);
  static bool isLocalLoad(const LoadSDNode *N);
  static bool isRegionLoad(const LoadSDNode *N);

  virtual const char *getPassName() const;
private:
  SDNode *xformAtomicInst(SDNode *N);

  // Include the pieces autogenerated from the target description.
#include "AMDILGenDAGISel.inc"
};
}  // end anonymous namespace

// createAMDILISelDag - This pass converts a legalized DAG into a AMDIL-specific
// DAG, ready for instruction scheduling.
//
FunctionPass *llvm::createAMDILISelDag(AMDILTargetMachine &TM
                                        AMDIL_OPT_LEVEL_DECL) {
  return new AMDILDAGToDAGISel(TM AMDIL_OPT_LEVEL_VAR);
}

AMDILDAGToDAGISel::AMDILDAGToDAGISel(AMDILTargetMachine &TM
                                      AMDIL_OPT_LEVEL_DECL)
  : SelectionDAGISel(TM AMDIL_OPT_LEVEL_VAR), Subtarget(TM.getSubtarget<AMDILSubtarget>())
{
}

AMDILDAGToDAGISel::~AMDILDAGToDAGISel() {
}

SDValue AMDILDAGToDAGISel::getSmallIPtrImm(unsigned int Imm) {
  return CurDAG->getTargetConstant(Imm, MVT::i32);
}

bool AMDILDAGToDAGISel::SelectADDRParam(
    SDValue Addr, SDValue& R1, SDValue& R2) {

  if (Addr.getOpcode() == ISD::FrameIndex) {
    if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
      R1 = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
      R2 = CurDAG->getTargetConstant(0, MVT::i32);
    } else {
      R1 = Addr;
      R2 = CurDAG->getTargetConstant(0, MVT::i32);
    }
  } else if (Addr.getOpcode() == ISD::ADD) {
    R1 = Addr.getOperand(0);
    R2 = Addr.getOperand(1);
  } else {
    R1 = Addr;
    R2 = CurDAG->getTargetConstant(0, MVT::i32);
  }
  return true;
}

bool AMDILDAGToDAGISel::SelectADDR(SDValue Addr, SDValue& R1, SDValue& R2) {
  if (Addr.getOpcode() == ISD::TargetExternalSymbol ||
      Addr.getOpcode() == ISD::TargetGlobalAddress) {
    return false;
  }
  return SelectADDRParam(Addr, R1, R2);
}


bool AMDILDAGToDAGISel::SelectADDR64(SDValue Addr, SDValue& R1, SDValue& R2) {
  if (Addr.getOpcode() == ISD::TargetExternalSymbol ||
      Addr.getOpcode() == ISD::TargetGlobalAddress) {
    return false;
  }

  if (Addr.getOpcode() == ISD::FrameIndex) {
    if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
      R1 = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i64);
      R2 = CurDAG->getTargetConstant(0, MVT::i64);
    } else {
      R1 = Addr;
      R2 = CurDAG->getTargetConstant(0, MVT::i64);
    }
  } else if (Addr.getOpcode() == ISD::ADD) {
    R1 = Addr.getOperand(0);
    R2 = Addr.getOperand(1);
  } else {
    R1 = Addr;
    R2 = CurDAG->getTargetConstant(0, MVT::i64);
  }
  return true;
}

SDNode *AMDILDAGToDAGISel::Select(SDNode *N) {
  unsigned int Opc = N->getOpcode();
  if (N->isMachineOpcode()) {
    return NULL;   // Already selected.
  }
  switch (Opc) {
  default: break;
  case ISD::FrameIndex:
    {
      if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(N)) {
        unsigned int FI = FIN->getIndex();
        EVT OpVT = N->getValueType(0);
        unsigned int NewOpc = AMDIL::MOVE_i32;
        SDValue TFI = CurDAG->getTargetFrameIndex(FI, MVT::i32);
        return CurDAG->SelectNodeTo(N, NewOpc, OpVT, TFI);
      }
    }
    break;
  }
  // For all atomic instructions, we need to add a constant
  // operand that stores the resource ID in the instruction
  if (Opc > AMDILISD::ADDADDR && Opc < AMDILISD::APPEND_ALLOC) {
    N = xformAtomicInst(N);
  }
  return SelectCode(N);
}

bool AMDILDAGToDAGISel::isGlobalStore(const StoreSDNode *N) {
  return check_type(N->getSrcValue(), AMDILAS::GLOBAL_ADDRESS);
}

bool AMDILDAGToDAGISel::isPrivateStore(const StoreSDNode *N) {
  return (!check_type(N->getSrcValue(), AMDILAS::LOCAL_ADDRESS)
          && !check_type(N->getSrcValue(), AMDILAS::GLOBAL_ADDRESS)
          && !check_type(N->getSrcValue(), AMDILAS::REGION_ADDRESS));
}

bool AMDILDAGToDAGISel::isLocalStore(const StoreSDNode *N) {
  return check_type(N->getSrcValue(), AMDILAS::LOCAL_ADDRESS);
}

bool AMDILDAGToDAGISel::isRegionStore(const StoreSDNode *N) {
  return check_type(N->getSrcValue(), AMDILAS::REGION_ADDRESS);
}

bool AMDILDAGToDAGISel::isConstantLoad(const LoadSDNode *N, int cbID) {
  if (check_type(N->getSrcValue(), AMDILAS::CONSTANT_ADDRESS)) {
    return true;
  }
  MachineMemOperand *MMO = N->getMemOperand();
  const Value *V = MMO->getValue();
  const Value *BV = getBasePointerValue(V);
  if (MMO
      && MMO->getValue()
      && ((V && dyn_cast<GlobalValue>(V))
          || (BV && dyn_cast<GlobalValue>(
                        getBasePointerValue(MMO->getValue()))))) {
    return check_type(N->getSrcValue(), AMDILAS::PRIVATE_ADDRESS);
  } else {
    return false;
  }
}

bool AMDILDAGToDAGISel::isGlobalLoad(const LoadSDNode *N) {
  return check_type(N->getSrcValue(), AMDILAS::GLOBAL_ADDRESS);
}

bool AMDILDAGToDAGISel::isLocalLoad(const  LoadSDNode *N) {
  return check_type(N->getSrcValue(), AMDILAS::LOCAL_ADDRESS);
}

bool AMDILDAGToDAGISel::isRegionLoad(const  LoadSDNode *N) {
  return check_type(N->getSrcValue(), AMDILAS::REGION_ADDRESS);
}

bool AMDILDAGToDAGISel::isCPLoad(const LoadSDNode *N) {
  MachineMemOperand *MMO = N->getMemOperand();
  if (check_type(N->getSrcValue(), AMDILAS::PRIVATE_ADDRESS)) {
    if (MMO) {
      const Value *V = MMO->getValue();
      const PseudoSourceValue *PSV = dyn_cast<PseudoSourceValue>(V);
      if (PSV && PSV == PseudoSourceValue::getConstantPool()) {
        return true;
      }
    }
  }
  return false;
}

bool AMDILDAGToDAGISel::isPrivateLoad(const LoadSDNode *N) {
  if (check_type(N->getSrcValue(), AMDILAS::PRIVATE_ADDRESS)) {
    // Check to make sure we are not a constant pool load or a constant load
    // that is marked as a private load
    if (isCPLoad(N) || isConstantLoad(N, -1)) {
      return false;
    }
  }
  if (!check_type(N->getSrcValue(), AMDILAS::LOCAL_ADDRESS)
      && !check_type(N->getSrcValue(), AMDILAS::GLOBAL_ADDRESS)
      && !check_type(N->getSrcValue(), AMDILAS::REGION_ADDRESS)
      && !check_type(N->getSrcValue(), AMDILAS::CONSTANT_ADDRESS)
      && !check_type(N->getSrcValue(), AMDILAS::PARAM_D_ADDRESS)
      && !check_type(N->getSrcValue(), AMDILAS::PARAM_I_ADDRESS))
  {
    return true;
  }
  return false;
}

const char *AMDILDAGToDAGISel::getPassName() const {
  return "AMDIL DAG->DAG Pattern Instruction Selection";
}

SDNode*
AMDILDAGToDAGISel::xformAtomicInst(SDNode *N)
{
  uint32_t addVal = 1;
  bool addOne = false;
  // bool bitCastToInt = (N->getValueType(0) == MVT::f32);
  unsigned opc = N->getOpcode();
  switch (opc) {
    default: return N;
    case AMDILISD::ATOM_G_ADD:
    case AMDILISD::ATOM_G_AND:
    case AMDILISD::ATOM_G_MAX:
    case AMDILISD::ATOM_G_UMAX:
    case AMDILISD::ATOM_G_MIN:
    case AMDILISD::ATOM_G_UMIN:
    case AMDILISD::ATOM_G_OR:
    case AMDILISD::ATOM_G_SUB:
    case AMDILISD::ATOM_G_RSUB:
    case AMDILISD::ATOM_G_XCHG:
    case AMDILISD::ATOM_G_XOR:
    case AMDILISD::ATOM_G_ADD_NORET:
    case AMDILISD::ATOM_G_AND_NORET:
    case AMDILISD::ATOM_G_MAX_NORET:
    case AMDILISD::ATOM_G_UMAX_NORET:
    case AMDILISD::ATOM_G_MIN_NORET:
    case AMDILISD::ATOM_G_UMIN_NORET:
    case AMDILISD::ATOM_G_OR_NORET:
    case AMDILISD::ATOM_G_SUB_NORET:
    case AMDILISD::ATOM_G_RSUB_NORET:
    case AMDILISD::ATOM_G_XCHG_NORET:
    case AMDILISD::ATOM_G_XOR_NORET:
    case AMDILISD::ATOM_L_ADD:
    case AMDILISD::ATOM_L_AND:
    case AMDILISD::ATOM_L_MAX:
    case AMDILISD::ATOM_L_UMAX:
    case AMDILISD::ATOM_L_MIN:
    case AMDILISD::ATOM_L_UMIN:
    case AMDILISD::ATOM_L_OR:
    case AMDILISD::ATOM_L_SUB:
    case AMDILISD::ATOM_L_RSUB:
    case AMDILISD::ATOM_L_XCHG:
    case AMDILISD::ATOM_L_XOR:
    case AMDILISD::ATOM_L_ADD_NORET:
    case AMDILISD::ATOM_L_AND_NORET:
    case AMDILISD::ATOM_L_MAX_NORET:
    case AMDILISD::ATOM_L_UMAX_NORET:
    case AMDILISD::ATOM_L_MIN_NORET:
    case AMDILISD::ATOM_L_UMIN_NORET:
    case AMDILISD::ATOM_L_OR_NORET:
    case AMDILISD::ATOM_L_SUB_NORET:
    case AMDILISD::ATOM_L_RSUB_NORET:
    case AMDILISD::ATOM_L_XCHG_NORET:
    case AMDILISD::ATOM_L_XOR_NORET:
    case AMDILISD::ATOM_R_ADD:
    case AMDILISD::ATOM_R_AND:
    case AMDILISD::ATOM_R_MAX:
    case AMDILISD::ATOM_R_UMAX:
    case AMDILISD::ATOM_R_MIN:
    case AMDILISD::ATOM_R_UMIN:
    case AMDILISD::ATOM_R_OR:
    case AMDILISD::ATOM_R_SUB:
    case AMDILISD::ATOM_R_RSUB:
    case AMDILISD::ATOM_R_XCHG:
    case AMDILISD::ATOM_R_XOR:
    case AMDILISD::ATOM_R_ADD_NORET:
    case AMDILISD::ATOM_R_AND_NORET:
    case AMDILISD::ATOM_R_MAX_NORET:
    case AMDILISD::ATOM_R_UMAX_NORET:
    case AMDILISD::ATOM_R_MIN_NORET:
    case AMDILISD::ATOM_R_UMIN_NORET:
    case AMDILISD::ATOM_R_OR_NORET:
    case AMDILISD::ATOM_R_SUB_NORET:
    case AMDILISD::ATOM_R_RSUB_NORET:
    case AMDILISD::ATOM_R_XCHG_NORET:
    case AMDILISD::ATOM_R_XOR_NORET:
    case AMDILISD::ATOM_G_CMPXCHG:
    case AMDILISD::ATOM_G_CMPXCHG_NORET:
    case AMDILISD::ATOM_L_CMPXCHG:
    case AMDILISD::ATOM_L_CMPXCHG_NORET:
    case AMDILISD::ATOM_R_CMPXCHG:
    case AMDILISD::ATOM_R_CMPXCHG_NORET:
             break;
    case AMDILISD::ATOM_G_DEC:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_G_SUB;
             }
             break;
    case AMDILISD::ATOM_G_INC:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_G_ADD;
             }
             break;
    case AMDILISD::ATOM_G_DEC_NORET:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_G_SUB_NORET;
             }
             break;
    case AMDILISD::ATOM_G_INC_NORET:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_G_ADD_NORET;
             }
             break;
    case AMDILISD::ATOM_L_DEC:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_L_SUB;
             }
             break;
    case AMDILISD::ATOM_L_INC:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_L_ADD;
             }
             break;
    case AMDILISD::ATOM_L_DEC_NORET:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_L_SUB_NORET;
             }
             break;
    case AMDILISD::ATOM_L_INC_NORET:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_L_ADD_NORET;
             }
             break;
    case AMDILISD::ATOM_R_DEC:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_R_SUB;
             }
             break;
    case AMDILISD::ATOM_R_INC:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_R_ADD;
             }
             break;
    case AMDILISD::ATOM_R_DEC_NORET:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_R_SUB;
             }
             break;
    case AMDILISD::ATOM_R_INC_NORET:
             addOne = true;
             if (Subtarget.calVersion() >= CAL_VERSION_SC_136) {
               addVal = (uint32_t)-1;
             } else {
               opc = AMDILISD::ATOM_R_ADD_NORET;
             }
             break;
  }
  // The largest we can have is a cmpxchg w/ a return value and an output chain.
  // The cmpxchg function has 3 inputs and a single output along with an
  // output change and a target constant, giving a total of 6.
  SDValue Ops[12];
  unsigned x = 0;
  unsigned y = N->getNumOperands();
  for (x = 0; x < y; ++x) {
    Ops[x] = N->getOperand(x);
  }
  if (addOne) {
    Ops[x++] = SDValue(SelectCode(CurDAG->getConstant(addVal, MVT::i32).getNode()), 0);
  }
  Ops[x++] = CurDAG->getTargetConstant(0, MVT::i32);
  SDVTList Tys = N->getVTList();
  MemSDNode *MemNode = dyn_cast<MemSDNode>(N);
  assert(MemNode && "Atomic should be of MemSDNode type!");
  N = CurDAG->getMemIntrinsicNode(opc, N->getDebugLoc(), Tys, Ops, x,
      MemNode->getMemoryVT(), MemNode->getMemOperand()).getNode();
  return N;
}

#ifdef DEBUGTMP
#undef INT64_C
#endif
#undef DEBUGTMP
