/*
 * Copyright 2016 - 2022  Angelo Matni, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "noelle/tools/DSWP.hpp"

namespace llvm::noelle {

void DSWP::collectLiveInEnvInfo(LoopDependenceInfo *LDI) {

  /*
   * Fetch the environment of the loop
   */
  auto environment = LDI->getEnvironment();
  assert(environment != nullptr);

  /*
   * Fetch the SCC manager
   */
  auto sccManager = LDI->getSCCManager();
  assert(sccManager != nullptr);
  auto sccdag = sccManager->getSCCDAG();
  assert(sccdag != nullptr);

  /*
   * Collect live-in information
   */
  for (auto envID : environment->getEnvIDsOfLiveInVars()) {

    /*
     * Fetch the producer of the live-in value.
     */
    auto producer = environment->getProducer(envID);
    assert(producer != nullptr);

    /*
     * Check the uses of the live-in value.
     */
    for (auto consumer : environment->consumersOf(producer)) {

      /*
       * Clonable consumers must be loaded into every task that uses them
       */
      auto consumerSCC = sccdag->sccOfValue(consumer);
      auto consumerSCCAttrs = sccManager->getSCCAttrs(consumerSCC);
      if (this->canBeCloned(consumerSCCAttrs)) {
        for (auto i = 0; i < tasks.size(); ++i) {
          auto task = (DSWPTask *)tasks[i];
          if (task->clonableSCCs.find(consumerSCC) == task->clonableSCCs.end())
            continue;
          envBuilder->getUser(i)->addLiveIn(envID);
        }

        continue;
      }

      /*
       * If not clonable, one and only task uses the consumer and must load it
       */
      assert(this->sccToStage.find(consumerSCC) != this->sccToStage.end());
      auto task = this->sccToStage.at(consumerSCC);
      auto id = task->getID();
      auto userID = this->fromTaskIDToUserID.at(id);
      envBuilder->getUser(userID)->addLiveIn(envID);
    }
  }

  return;
}

void DSWP::collectLiveOutEnvInfo(LoopDependenceInfo *LDI) {

  /*
   * Fetch the environment of the loop
   */
  auto environment = LDI->getEnvironment();
  assert(environment != nullptr);

  /*
   * Fetch the SCC manager
   */
  auto sccManager = LDI->getSCCManager();
  auto sccdag = sccManager->getSCCDAG();

  /*
   * Collect live-out information
   */
  for (auto envID : environment->getEnvIDsOfLiveOutVars()) {
    auto producer = environment->getProducer(envID);

    /*
     * Clonable producers all produce the same live out value.
     * Arbitrarily choose the first task that clones the producer to store it
     * live out
     */
    auto producerSCC = sccdag->sccOfValue(producer);
    auto producerSCCAttrs = sccManager->getSCCAttrs(producerSCC);
    if (this->canBeCloned(producerSCCAttrs)) {
      for (auto i = 0; i < tasks.size(); ++i) {
        auto task = (DSWPTask *)tasks[i];
        if (task->clonableSCCs.find(producerSCC) == task->clonableSCCs.end())
          continue;
        envBuilder->getUser(i)->addLiveOut(envID);
        break;
      }

      continue;
    }

    /*
     * If not clonable, one and only task produces the value and must store it
     * live out
     */
    assert(this->sccToStage.find(producerSCC) != this->sccToStage.end());
    auto task = this->sccToStage.at(producerSCC);
    auto id = task->getID();
    auto userID = this->fromTaskIDToUserID.at(id);
    envBuilder->getUser(userID)->addLiveOut(envID);
  }
}

} // namespace llvm::noelle
