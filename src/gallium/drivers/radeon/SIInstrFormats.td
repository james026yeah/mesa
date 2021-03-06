//===-- SIInstrFormats.td - TODO: Add brief description -------===//
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


class VOP3_32 <bits<9> op, string opName, list<dag> pattern>
  : VOP3 <op, (outs VReg_32:$dst), (ins AllReg_32:$src0, AllReg_32:$src1, AllReg_32:$src2, i32imm:$src3, i32imm:$src4, i32imm:$src5, i32imm:$src6), opName, pattern>;

class VOP3_64 <bits<9> op, string opName, list<dag> pattern>
  : VOP3 <op, (outs VReg_64:$dst), (ins AllReg_64:$src0, AllReg_64:$src1, AllReg_64:$src2, i32imm:$src3, i32imm:$src4, i32imm:$src5, i32imm:$src6), opName, pattern>;


class SOP1_32 <bits<8> op, string opName, list<dag> pattern>
  : SOP1 <op, (outs SReg_32:$dst), (ins SReg_32:$src0), opName, pattern>;

class SOP1_64 <bits<8> op, string opName, list<dag> pattern>
  : SOP1 <op, (outs SReg_64:$dst), (ins SReg_64:$src0), opName, pattern>;

class SOP2_32 <bits<7> op, string opName, list<dag> pattern>
  : SOP2 <op, (outs SReg_32:$dst), (ins SReg_32:$src0, SReg_32:$src1), opName, pattern>;

class SOP2_64 <bits<7> op, string opName, list<dag> pattern>
  : SOP2 <op, (outs SReg_64:$dst), (ins SReg_64:$src0, SReg_64:$src1), opName, pattern>;

class VOP1_Helper <bits<8> op, RegisterClass vrc, RegisterClass arc,
                   string opName, list<dag> pattern> : 
  VOP1 <
    op, (outs vrc:$dst), (ins arc:$src0), opName, pattern
  >;

multiclass VOP1_32 <bits<8> op, string opName, list<dag> pattern,
                    bits<16> amdil = AMDILInst.NONE> {

  let AMDILOp = amdil in {
    def _e32: VOP1_Helper <op, VReg_32, AllReg_32, opName, pattern>;
  }

  def _e64 : VOP3_32 <
    {1, 1, op{6}, op{5}, op{4}, op{3}, op{2}, op{1}, op{0}},
    opName, []
  >;
}

multiclass VOP1_64 <bits<8> op, string opName, list<dag> pattern> {

  def _e32 : VOP1_Helper <op, VReg_64, AllReg_64, opName, pattern>;

  def _e64 : VOP3_64 <
    {1, 1, op{6}, op{5}, op{4}, op{3}, op{2}, op{1}, op{0}},
    opName, []
  >;
}

class VOP2_Helper <bits<6> op, RegisterClass vrc, RegisterClass arc,
                   string opName, list<dag> pattern> :
  VOP2 <
    op, (outs vrc:$dst), (ins arc:$src0, vrc:$src1), opName, pattern
  >;

multiclass VOP2_32 <bits<6> op, string opName, list<dag> pattern,
                    bits<16> amdil = AMDILInst.NONE> {

  let AMDILOp = amdil in {
    def _e32 : VOP2_Helper <op, VReg_32, AllReg_32, opName, pattern>;
  }

  def _e64 : VOP3_32 <
    {1, 0, 0, op{5}, op{4}, op{3}, op{2}, op{1}, op{0}},
    opName, []
  >;
}

multiclass VOP2_64 <bits<6> op, string opName, list<dag> pattern> {
  def _e32: VOP2_Helper <op, VReg_64, AllReg_64, opName, pattern>;

  def _e64 : VOP3_64 <
    {1, 0, 0, op{5}, op{4}, op{3}, op{2}, op{1}, op{0}},
    opName, []
  >;
}

class SOPK_32 <bits<5> op, string opName, list<dag> pattern>
  : SOPK <op, (outs SReg_32:$dst), (ins i16imm:$src0), opName, pattern>;

class SOPK_64 <bits<5> op, string opName, list<dag> pattern>
  : SOPK <op, (outs SReg_64:$dst), (ins i16imm:$src0), opName, pattern>;

class VOPC_Helper <bits<8> op, RegisterClass vrc, RegisterClass arc,
                 string opName, list<dag> pattern> :
  VOPC <
    op, (outs), (ins arc:$src0, vrc:$src1), opName, pattern
  >;

multiclass VOPC_32 <bits<8> op, string opName, list<dag> pattern> {

  def _e32 : VOPC_Helper <op, VReg_32, AllReg_32, opName, pattern>;

  def _e64 : VOP3_32 <
    {0, op{7}, op{6}, op{5}, op{4}, op{3}, op{2}, op{1}, op{0}},
    opName, []
  >;
}

multiclass VOPC_64 <bits<8> op, string opName, list<dag> pattern> {

  def _e32 : VOPC_Helper <op, VReg_64, AllReg_64, opName, pattern>;

  def _e64 : VOP3_64 <
    {0, op{7}, op{6}, op{5}, op{4}, op{3}, op{2}, op{1}, op{0}},
    opName, []
  >;
}

class SOPC_32 <bits<7> op, string opName, list<dag> pattern>
  : SOPC <op, (outs CCReg:$dst), (ins SReg_32:$src0, SReg_32:$src1), opName, pattern>;

class SOPC_64 <bits<7> op, string opName, list<dag> pattern>
  : SOPC <op, (outs CCReg:$dst), (ins SReg_64:$src0, SReg_64:$src1), opName, pattern>;

