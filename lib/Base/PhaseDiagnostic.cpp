//===----------------------------------------------------------------------===//
//
// Part of the Eter Project, under the Apache License v2.0 with LLVM Exceptions.
// See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "eter/Base/PhaseDiagnostic.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

#include <string>
#include <utility>

namespace eter::diag {

std::string
renderMessage(llvm::StringRef Template,
              llvm::ArrayRef<std::pair<llvm::StringRef, std::string>> Args) {
  std::string Out;
  Out.reserve(Template.size());

  size_t I = 0;
  while (I < Template.size()) {
    const char C = Template[I];
    if (C != '{') {
      Out.push_back(C);
      ++I;
      continue;
    }

    const size_t Close = Template.find('}', I + 1);
    if (Close == llvm::StringRef::npos) {
      Out.append(Template.substr(I).str());
      break;
    }

    const llvm::StringRef Name = Template.slice(I + 1, Close);
    bool Found = false;
    for (const auto &A : Args) {
      if (A.first == Name) {
        Out.append(A.second);
        Found = true;
        break;
      }
    }
    if (!Found)
      Out.append(Template.slice(I, Close + 1).str());

    I = Close + 1;
  }

  return Out;
}

} // namespace eter::diag
