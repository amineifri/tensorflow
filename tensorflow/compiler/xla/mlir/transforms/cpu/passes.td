/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_COMPILER_XLA_MLIR_TRANSFORMS_CPU_PASSES_TD_
#define TENSORFLOW_COMPILER_XLA_MLIR_TRANSFORMS_CPU_PASSES_TD_

include "mlir/Pass/PassBase.td"

//===----------------------------------------------------------------------===//
// Auxiliary passes for lowering to XLA Cpu runtime.
//===----------------------------------------------------------------------===//

def ConvertLmhloToCpuRuntimePass :
    Pass<"xla-lmhlo-to-cpu-runtime", "mlir::ModuleOp"> {
  let summary = "Converts lmhlo operations to XLA Cpu runtime custom calls";

  let description = [{
      Converts lmhlo dialect operations to XLA Cpu runtime custom calls.
  }];

  let constructor = "createConvertLmhloToCpuRuntimePass()";
}

#endif  // TENSORFLOW_COMPILER_XLA_MLIR_TRANSFORMS_CPU_PASSES_TD_
