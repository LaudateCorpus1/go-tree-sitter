#include "parser.h"
#include <wctype.h>

enum TokenType {
  AUTOMATIC_SEMICOLON,
  TEMPLATE_CHARS
};

static void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static bool scan_whitespace_and_comments(TSLexer *lexer) {
  for (;;) {
    while (iswspace(lexer->lookahead)) {
      advance(lexer);
    }

    if (lexer->lookahead == '/') {
      advance(lexer);

      if (lexer->lookahead == '/') {
        advance(lexer);
        while (lexer->lookahead != 0 && lexer->lookahead != '\n') {
          advance(lexer);
        }
      } else if (lexer->lookahead == '*') {
        advance(lexer);
        while (lexer->lookahead != 0) {
          if (lexer->lookahead == '*') {
            advance(lexer);
            if (lexer->lookahead == '/') {
              advance(lexer);
              break;
            }
          } else {
            advance(lexer);
          }
        }
      } else {
        return false;
      }
    } else {
      return true;
    }
  }
}

static inline bool external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  if (valid_symbols[TEMPLATE_CHARS]) {
    if (valid_symbols[AUTOMATIC_SEMICOLON]) return false;
    lexer->result_symbol = TEMPLATE_CHARS;
    for (bool notfirst = false;; notfirst = true) {
      lexer->mark_end(lexer);
      switch (lexer->lookahead) {
        case '`':
          return notfirst;
        case '\0':
          return false;
        case '$':
          advance(lexer);
          if (lexer->lookahead == '{') return notfirst;
          break;
        case '\\':
          advance(lexer);
          advance(lexer);
          break;
        default:
          advance(lexer);
      }
    }
  } else {
    lexer->result_symbol = AUTOMATIC_SEMICOLON;
    lexer->mark_end(lexer);

    for (;;) {
      if (lexer->lookahead == 0) return true;
      if (lexer->lookahead == '}') return true;
      if (!iswspace(lexer->lookahead)) return false;
      if (lexer->lookahead == '\n') break;
      advance(lexer);
    }

    advance(lexer);

    if (!scan_whitespace_and_comments(lexer)) return false;

    switch (lexer->lookahead) {
      case ',':
      case '.':
      case ';':
      case '*':
      case '%':
      case '>':
      case '<':
      case '=':
      case '[':
      case '(':
      case '?':
      case '^':
      case '|':
      case '&':
      case '/':
        return false;

      // Insert a semicolon before `--` and `++`, but not before binary `+` or `-`.
      case '+':
        advance(lexer);
        return lexer->lookahead == '+';
      case '-':
        advance(lexer);
        return lexer->lookahead == '-';

      // Don't insert a semicolon before `!=`, but do insert one before a unary `!`.
      case '!':
        advance(lexer);
        return lexer->lookahead != '=';

      // Don't insert a semicolon before `in` or `instanceof`, but do insert one
      // before an identifier.
      case 'i':
        advance(lexer);

        if (lexer->lookahead != 'n') return true;
        advance(lexer);

        if (!iswalpha(lexer->lookahead)) return false;

        for (unsigned i = 0; i < 8; i++) {
          if (lexer->lookahead != "stanceof"[i]) return true;
          advance(lexer);
        }

        if (!iswalpha(lexer->lookahead)) return false;
        break;
    }

    return true;
  }
}
