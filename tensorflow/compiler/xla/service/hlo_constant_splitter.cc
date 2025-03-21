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

#include "tensorflow/compiler/xla/service/hlo_constant_splitter.h"

#include <iterator>
#include <utility>
#include <vector>

namespace xla {

namespace {

// Return if this is one of the instructions that we consider a "constant".
bool IsSupportedConstant(const HloInstruction* instruction,
                         bool split_expressions) {
  return instruction->opcode() == HloOpcode::kConstant ||
         (split_expressions && instruction->opcode() == HloOpcode::kIota);
}

// Return if this is one of the constant expressions that we consider for
// duplication.
bool IsSupportedConstantExpression(const HloInstruction* instruction) {
  return instruction->opcode() == HloOpcode::kBroadcast ||
         instruction->IsElementwise();
}

// Perform duplication of a certain constant expression and replace the
// original expression for a specific user.
StatusOr<bool> DuplicateConstantExpressionPerUser(HloComputation* computation,
                                                  HloInstruction* to_clone,
                                                  HloInstruction* user) {
  absl::InlinedVector<std::pair<const HloInstruction*, int>, 8> worklist(
      1, std::make_pair(to_clone, 0));
  absl::InlinedVector<const HloInstruction*, 8> to_clone_vec;
  absl::flat_hash_set<const HloInstruction*> visited;
  bool changed = false;
  VLOG(10) << "Duplicating: " << to_clone->ToString() << " for user "
           << user->ToString();
  // Walk graph in post order to find all elements of the expression and add
  // them to to_clone_vec in post order which is the order in which we want to
  // process them.
  while (!worklist.empty()) {
    auto& [to_clone_i, index] = worklist.back();
    if (index >= to_clone_i->operand_count()) {
      to_clone_vec.push_back(to_clone_i);
      worklist.pop_back();
      continue;
    }
    int64_t prev_idx = index++;
    if (visited.insert(to_clone_i->operands()[prev_idx]).second) {
      VLOG(10) << "Adding operand to worklist: "
               << to_clone_i->operands()[prev_idx]->ToString();
      worklist.push_back(std::make_pair(to_clone_i->operands()[prev_idx], 0));
    }
  }
  absl::flat_hash_map<const HloInstruction*, HloInstruction*>
      cloned_instructions_map;
  // Clone each instruction and replace the instruction operands with the cloned
  // version if existing.
  for (auto* i : to_clone_vec) {
    absl::InlinedVector<HloInstruction*, 4> new_operand_vector;
    for (auto* op : i->operands()) {
      auto it = cloned_instructions_map.find(op);
      CHECK(it != cloned_instructions_map.end())
          << "Expected already cloned instruction for operand: "
          << op->ToString() << " Instruction to clone: " << i->ToString();
      new_operand_vector.push_back(it->second);
    }
    HloInstruction* cloned_instr = computation->AddInstruction(
        i->CloneWithNewOperands(i->shape(), new_operand_vector));
    cloned_instructions_map[i] = cloned_instr;
    if (i == to_clone) {
      TF_RETURN_IF_ERROR(to_clone->ReplaceUseWith(user, cloned_instr));
      changed = true;
    }
  }
  return changed;
}

}  // namespace

StatusOr<bool> HloConstantSplitter::Run(
    HloModule* module,
    const absl::flat_hash_set<absl::string_view>& execution_threads) {
  bool changed = false;
  for (HloComputation* computation : module->computations(execution_threads)) {
    absl::flat_hash_set<HloInstruction*> constants_set;
    std::vector<HloInstruction*> constants_list;
    std::vector<HloInstruction*> worklist;
    for (HloInstruction* instruction :
         computation->MakeInstructionPostOrder()) {
      VLOG(10) << "Considering: " << instruction->ToString();
      if (IsSupportedConstant(instruction, split_expressions_)) {
        VLOG(10) << "Adding to constant list: " << instruction->ToString();
        constants_set.insert(instruction);
        constants_list.push_back(instruction);
      }
    }
    int64_t previous_total_constants = 0;
    // Run algorithm until fixed point to discover all simple constant
    // expressions.
    while (constants_list.size() != previous_total_constants) {
      VLOG(10) << "Previous total: " << previous_total_constants
               << " current constants: " << constants_list.size();
      previous_total_constants = constants_list.size();
      worklist.clear();
      worklist.insert(worklist.end(), constants_list.begin(),
                      constants_list.end());
      while (!worklist.empty()) {
        auto* i = worklist.back();
        worklist.pop_back();
        bool is_constant = true;
        for (auto* ops : i->operands()) {
          if (!constants_set.contains(ops)) {
            is_constant = false;
            break;
          }
        }
        if (is_constant) {
          if (constants_set.insert(i).second) {
            constants_list.push_back(i);
          }
          if (split_expressions_) {
            for (auto* u : i->users()) {
              if (IsSupportedConstantExpression(u) &&
                  !constants_set.contains(u)) {
                worklist.push_back(u);
              }
            }
          }
        }
      }
    }
    if (VLOG_IS_ON(5)) {
      VLOG(5) << "For computation: " << computation->ToString();
      for (HloInstruction* instruction : constants_list) {
        VLOG(5) << "Is a constant: " << instruction->ToString();
      }
    }

    // Perform duplication of the constants/constant expressions.
    for (HloInstruction* instruction : constants_list) {
      if (instruction->user_count() == 0) {
        continue;
      }
      absl::InlinedVector<HloInstruction*, 8> users;
      users.reserve(instruction->user_count());
      // Consider for splitting only leaf expressions (not constants in the
      // middle of a constant expression). Also only split for non-constant
      // users for expressions.
      for (int i = 1; i < instruction->user_count(); ++i) {
        if (instruction->opcode() == HloOpcode::kConstant ||
            !constants_set.contains(instruction->users()[i])) {
          users.push_back(instruction->users()[i]);
        }
      }
      for (auto* u : users) {
        TF_ASSIGN_OR_RETURN(bool duplicated, DuplicateConstantExpressionPerUser(
                                                 computation, instruction, u));
        changed |= duplicated;
      }
    }
  }
  return changed;
}

}  // namespace xla
