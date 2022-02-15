
#ifndef __SEMA_H__
#define __SEMA_H__ 1

#include "common.h"
#include "tree.h"
#include "parser.h"
#include "types.h"

enum {
  POINTER_TYPE_SIZE = 8,
  BYTE_BIT_SIZE = 8,
  UNKNOWN_SIZE = -1
};

int computeTypeSize(TypeRef *type);
int computeSUETypeSize(ParserContext *ctx, AstSUEDeclaration *declaration);


TypeRef *computeArrayAccessExpressionType(ParserContext *ctx, Coordinates *coords, TypeRef *arrayType, TypeRef *indexType);
AstStructDeclarator *computeMemberDeclarator(ParserContext *ctx, Coordinates *coords, TypeRef *receiverType, const char *memberName, ExpressionType op);
TypeRef *computeFunctionReturnType(ParserContext *ctx, Coordinates *coords, TypeRef *calleeType);
TypeRef *computeIncDecType(ParserContext *ctx, Coordinates *coords, TypeRef *argumentType, ExpressionType op);
TypeRef *computeTypeForUnaryOperator(ParserContext *ctx, Coordinates *coords, TypeRef *argumentType, ExpressionType op);
TypeRef *computeBinaryType(ParserContext *ctx, Coordinates *coords, TypeRef* left, TypeRef *right, ExpressionType op);
TypeRef *computeTernaryType(ParserContext *ctx, Coordinates *coords, TypeRef* cond, TypeRef* ifTrue, TypeRef *ifFalse, ExpressionType op);
TypeRef *computeAssignmentTypes(ParserContext *ctx, Coordinates *coords, TypeRef *left, TypeRef *right);
TypeRef *computeTernaryType(ParserContext *ctx, Coordinates *coords, TypeRef* cond, TypeRef* ifTrue, TypeRef *ifFalse, ExpressionType op);
TypeRef *computeFunctionType(ParserContext *ctx, Coordinates *coords, AstFunctionDeclaration *declaration);

AstInitializer *finalizeInitializer(ParserContext *ctx, TypeRef *valueType, AstInitializer *initializer, Boolean isTopLevel);

Boolean verifyValueType(ParserContext *ctx, Coordinates *coords, TypeRef *valueType);
void verifyAndTransformCallAruments(ParserContext *ctx, Coordinates *coords, TypeRef *functionType, AstExpressionList *aruments);

Boolean isErrorType(TypeRef *type);
Boolean isIntegerType(TypeRef *type);
Boolean isVoidType(TypeRef *type);

Boolean isAssignableTypes(ParserContext *ctx, Coordinates *coords, TypeRef *to, TypeRef *from, Boolean init);

void verifyGotoExpression(ParserContext *ctx, AstExpression *expr);

void verifySwitchCases(ParserContext *ctx, AstStatement *switchBody, unsigned caseCount);
void verifyGotoLabels(ParserContext *ctx, AstStatement *body, HashMap *labelSet);

Boolean checkExpressionIsAssignable(ParserContext *ctx, Coordinates *coords, AstExpression *expr, Boolean report);
Boolean checkTypeIsCastable(ParserContext *ctx, Coordinates *coords, TypeRef *to, TypeRef *from, Boolean report);
Boolean checkRefArgument(ParserContext *ctx, Coordinates *coords, AstExpression *arg, Boolean report);

AstExpression *transformBinaryExpression(ParserContext *ctx, AstExpression *expr);
AstExpression *transformTernaryExpression(ParserContext *ctx, AstExpression *expr);
AstExpression *transformAssignExpression(ParserContext *ctx, AstExpression *expr);

void verifyStatementLevelExpression(ParserContext *ctx, AstExpression *expr);

typedef enum _TypeEqualityKind {
  TEK_UNKNOWN,
  TEK_EQUAL,
  TEK_ALMOST_EQUAL, // could be casted implicitly
  TEK_NOT_EXATCLY_EQUAL, // in case of two different enums, warning
  TEK_NOT_EQUAL
} TypeEqualityKind;

Boolean typesEquals(TypeRef *t1, TypeRef *t2);
TypeEqualityKind typeEquality(TypeRef *t1, TypeRef *t2);
int isTypeName(ParserContext *ctx, const char* name, struct _Scope* scope);

typedef enum _TypeCastabilityKind {
  TCK_UNKNOWN,
  TCK_NO_CAST, // int -> int
  TCK_IMPLICIT_CAST, // char -> int, could be casted implicitly
  TCK_EXPLICIT_CAST // struct S -> struct D
} TypeCastabilityKind;

TypeCastabilityKind typeCastability(TypeRef *to, TypeRef *from);

typedef enum _SymbolKind {
    FunctionSymbol = 1,
    UnionSymbol,
    StructSymbol,
    TypedefSymbol,
    ValueSymbol,
    EnumSymbol, /** TODO: not sure about it */
    EnumConstSymbol
} SymbolKind;

typedef struct _Symbol {
    SymbolKind kind;
    const char* name; /** struct/union/enum is referenced via "$$name" or "|$name" or "#$enum"*/
    union {
        struct _AstFunctionDeclaration *function; // FunctionSymbol
        TypeDesc *typeDescriptor; // StructSymbol | UnionSymbol | EnumSymbol, struct S;
        TypeRef * typeref; // TypedefSymbol, typedef struct TS* ts_t;
        struct _AstValueDeclaration *variableDesc; // ValueSymbol, int a = 10 | int foo(int value)
        struct _EnumConstant *enumerator; // EnumConstSymbol
    };
} Symbol;

typedef struct _Scope {
    struct _Scope* parent;
    HashMap* symbols;

    struct _Scope *next;
} Scope;

void verifyFunctionReturnType(ParserContext *ctx, Declarator *declarator, TypeRef *returnType);


Symbol* findSymbol(ParserContext *ctx, const char *name);
Symbol* declareSymbol(ParserContext *ctx, SymbolKind kind, const char *name);
Symbol* findOrDeclareSymbol(ParserContext* ctx, SymbolKind kind, const char* name);

Symbol *declareTypeDef(ParserContext *ctx, const char *name, TypeRef *type);
Symbol *declareValueSymbol(ParserContext *ctx, const char *name, AstValueDeclaration *declaration);
Symbol *declareFunctionSymbol(ParserContext *ctx, const char *name, AstFunctionDeclaration *declaration);
Symbol *declareSUESymbol(ParserContext *ctx, SymbolKind symbolKind, TypeId typeId, const char *name, AstSUEDeclaration *declaration);
Symbol *declareEnumConstantSymbol(ParserContext *ctx, EnumConstant *enumerator);

Scope *newScope(ParserContext *ctx, Scope *parent);

TypeRef *makePrimitiveType(ParserContext *ctx, TypeId id, unsigned flags);
TypeRef *makeBasicType(ParserContext *ctx, TypeDesc *descriptor, unsigned flags);
TypeRef* makePointedType(ParserContext *ctx, SpecifierFlags flags, TypeRef *pointedTo);
TypeRef *makeArrayType(ParserContext *ctx, int size, TypeRef *elementType);
TypeRef *makeFunctionType(ParserContext *ctx, TypeRef *returnType, FunctionParams *params);
TypeRef *makeFunctionReturnType(ParserContext *ctx, DeclarationSpecifiers *specifiers, Declarator *declarator);
TypeRef *makeTypeRef(ParserContext *ctx, DeclarationSpecifiers *specifiers, Declarator *declarator);
TypeRef *makeBitFieldType(ParserContext *ctx, TypeRef *storage, unsigned offset, unsigned width);
TypeRef *makeErrorRef(ParserContext *ctx);


int stringHashCode(const void *v);
int stringCmp(const void *v1, const void *v2);

#endif // __SEMA_H__
