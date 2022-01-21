
#ifndef __PARSER_H__
#define __PARSER_H__ 1

#include "tokens.h"
#include "lex.h"
#include "tree.h"
#include "mem.h"
#include "diagnostics.h"

AstFile* parseFile(FILE* file, const char* fileName);

AstConst* eval(ParserContext *ctx, AstExpression* expression);

#define TokenTextCacheSize 1024

typedef struct _Token {
    Coordinates coordinates;
    int code;
    int rawCode;
    const char *text;
    union {
      long long iv;
      double dv;
    } value;
    struct _Token *next;
} Token;

struct _Scope;

typedef struct _ParserContext {
    AstFile* parsedFile;

    yyscan_t scanner;

    struct _Scope* rootScope;
    struct _Scope* currentScope;

    Token *firstToken;
    Token *token;

    struct {
      Arena *tokenArena;
      Arena *astArena;
      Arena *typeArena;
      Arena *stringArena;
      Arena *diagnosticsArena;
    } memory;

    int anonSymbolsCounter;

    Diagnostics diagnostics;

    struct {
      YYLTYPE position;
      unsigned lineno;
      unsigned lineCount;
      unsigned *linesPos;
    } locationInfo;
} ParserContext;

#endif // __PARSER_H__
