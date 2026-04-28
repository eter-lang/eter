#pragma once
#include "eter/Base/SourceBuffer.h"
#include "eter/Lexer/Lexer.h"

namespace eter {
    void dumpTokens(SourceBuffer &SB) {
      eter::lexer::Lexer Lex;
      auto Span = eter::Span(0, SB.getBuffer().size());
      auto Items = Lex.lex(SB, Span);

      llvm::outs() << "=== Token Dump ===\n";
      for (const auto &Item : Items) {
        if (std::holds_alternative<lexer::Token>(Item)) {
          auto Tok = std::get<lexer::Token>(Item);
          llvm::StringRef TokText = SB.getBuffer().substr(Tok.TokenSpan.Start, Tok.TokenSpan.End - Tok.TokenSpan.Start);
          llvm::outs() << "\033[1;32m" << lexer::Token::getTokenName(Tok.TokenKind)
                       << "\033[0m: '\033[1;36m" << TokText << "\033[0m'\n";
        } else if (std::holds_alternative<lexer::LexerError>(Item)) {
          auto Err = std::get<lexer::LexerError>(Item);
          llvm::outs() << "\033[1;31mError [" << Err.ErrorSpan.Start << ", " << Err.ErrorSpan.End
                       << "]\033[0m: " << lexer::LexerError::getErrorName(Err.ErrorKind) << "\n";
        }
      }
      llvm::outs() << "==================\n";
    }
} // namespace eter
