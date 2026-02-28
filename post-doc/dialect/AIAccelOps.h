//===- AIAccelOps.h - AI Accelerator Operations --------*- C++ -*-===//

#ifndef AIACCCEL_OPS_H
#define AIACCCEL_OPS_H

#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OpAsmInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"

//===----------------------------------------------------------------------===//
// AIAccel Operation Includes
//===----------------------------------------------------------------------===//

#define GET_OP_CLASSES
#include "AIAccel/AIAccelOps.h.inc"

#endif // AIACCCEL_OPS_H
