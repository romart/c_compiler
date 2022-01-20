
#include <assert.h>

#include "sema.h"
#include "treeDump.h"
#include "parser.h"


size_t computeTypeSize(TypeRef *type) {
  if (type->kind == TR_VALUE) {
      return type->descriptorDesc->size;
  }

  if (type->kind == TR_POINTED) {
      return POINTER_TYPE_SIZE;
  }

  if (type->kind == TR_ARRAY) {
      ArrayTypeDescriptor *atype = &type->arrayTypeDesc;
      return atype->size * computeTypeSize(atype->elementType);
  }

  return POINTER_TYPE_SIZE;
}

int typesEquals(TypeRef *t1, TypeRef *t2) {
  return TRUE;
}

static int stringHashCode(const void *v) {
    const char *s = (const char *)v;
    int result = 0;

    int i = 0;

    while (s[i]) {
        result *= 31;
        result += s[i++];
    }

    return result;
}

static int stringCmp(const void *v1, const void *v2) {
    const char *s1 = (const char *)v1;
    const char *s2 = (const char *)v2;

    return strcmp(s1, s2);
}

Scope *newScope(ParserContext *ctx, Scope *parent) {
  Scope *result = (Scope *)areanAllocate(ctx->typeArena, sizeof (Scope));
  result->parent = parent;
  result->symbols = createHashMap(DEFAULT_MAP_CAPACITY, stringHashCode, stringCmp);
  return result;
}

static Symbol *findSymbolInScope(Scope *scope, const char *name) {
  return (Symbol *)getFromHashMap(scope->symbols, name);
}

Symbol* findSymbol(ParserContext *ctx, const char *name) {
    Scope* s = ctx->currentScope;
    while (s != NULL) {
        Symbol *sb = (Symbol *)getFromHashMap(s->symbols, name);
        if (sb != NULL) return sb;
        s = s->parent;
    }

    return NULL;
}


int isTypeName(ParserContext *ctx, const char* name, struct _Scope* scope) {
    Symbol *s = findSymbol(ctx, name);
    return s && s->kind == TypedefSymbol;
}


Symbol* declareSymbol(ParserContext *ctx, int kind, const char *name) {
    int symbolSize = sizeof(Symbol);
    Symbol *s = (Symbol *)areanAllocate(ctx->typeArena, symbolSize);
    s->kind = kind;
    s->name = name;

    Scope *scope = ctx->currentScope;
    putToHashMap(scope->symbols, name, s);

    return s;
}

Symbol* findOrDeclareSymbol(ParserContext* ctx, int kind, const char* name) {
    Symbol *existed = findSymbol(ctx, name);
    if (existed) return existed;
    return declareSymbol(ctx, kind, name);
}

static int functionsEqual(AstFunctionDeclaration *f1, AstFunctionDeclaration *f2) {
  return TRUE;
}


typedef int (*symbolProcessor)(ParserContext *, Symbol *, void *);

static Symbol *declareGenericSymbol(ParserContext *ctx, int kind, const char *name, void *value, symbolProcessor existed, symbolProcessor new) {

  Symbol *s = findSymbolInScope(ctx->currentScope, name);
  if (s) {
      if (s->kind == kind) {
          existed(ctx, s, value);
      } else {
          parseError(ctx, "redefinition of '%s' as different kind of symbol", name);
      }
  } else {
    s = declareSymbol(ctx, kind, name);
    new(ctx, s, value);
  }

  return s;
}

static int existedTypeDefProcessor(ParserContext *ctx, Symbol *s, void *value) {
  assert(s->kind == TypedefSymbol);
  TypeRef *oldType = s->typeref;
  TypeRef *newType = (TypeRef *)value;
  if (typesEquals(oldType, newType)) {
      // parseWarning(ctx, "redefinition")
  } else {
    char t1[128] = { 0 }, t2[128] = { 0 };

    renderTypeRef(oldType, t1, sizeof t1);
    renderTypeRef(newType, t2, sizeof t2);
    parseError(ctx, "typedef redefinition with different types ('%s' vs '%s')", t1, t2);
  }
}

static int newTypeDefProcessor(ParserContext *ctx, Symbol *s, void *value) {
  assert(s->kind == TypedefSymbol);
  s->typeref = (TypeRef *)value;
}

Symbol *declareTypeDef(ParserContext *ctx, const char *name, TypeRef *type) {
  return declareGenericSymbol(ctx, TypedefSymbol, name, type, existedTypeDefProcessor, newTypeDefProcessor);
}

static int existedFunctionProcessor(ParserContext *ctx, Symbol *s, void *value) {
  assert(s->kind == FunctionSymbol);
  AstFunctionDeclaration *oldDeclaration = s->function;
  AstFunctionDeclaration *newDeclaration = (AstFunctionDeclaration *)value;
  if (functionsEqual(oldDeclaration, newDeclaration)) {
      // TODO: link them into list
  } else {
    parseError(ctx, "conflicting types for '%s'", oldDeclaration->name);
  }
}

static int newFunctionProcessor(ParserContext *ctx, Symbol *s, void *value) {
  assert(s->kind == FunctionSymbol);
  s->function = (AstFunctionDeclaration *)value;
}

Symbol *declareFunctionSymbol(ParserContext *ctx, const char *name, AstFunctionDeclaration *declaration) {
  return declareGenericSymbol(ctx, FunctionSymbol, name, declaration, existedFunctionProcessor, newFunctionProcessor);
}

static int existedValueProcessor(ParserContext *ctx, Symbol *s, void *value) {
  assert(s->kind == ValueSymbol);

  AstValueDeclaration *oldValue = s->variableDesc;
  AstValueDeclaration *newValue = (AstValueDeclaration *)value;

  TypeRef *oldType = oldValue->type;
  TypeRef *newType = newValue->type;

  if (typesEquals(oldType, newType)) {
      // TODO: link declarations to list
  } else {
    char t1[128] = { 0 }, t2[128] = { 0 };

    renderTypeRef(oldType, t1, sizeof t1);
    renderTypeRef(newType, t2, sizeof t2);
    parseError(ctx, "typedef redefinition with different types ('%s' vs '%s')", t1, t2);
  }
}

static int newValueProcessor(ParserContext *ctx, Symbol *s, void *value) {
  assert(s->kind == ValueSymbol);
  s->variableDesc = (AstValueDeclaration *)value;
}

Symbol *declareValueSymbol(ParserContext *ctx, const char *name, AstValueDeclaration *declaration) {
  return declareGenericSymbol(ctx, ValueSymbol, name, declaration, existedValueProcessor, newValueProcessor);
}

Symbol *declareSUESymbol(ParserContext *ctx, int symbolKind, int typeId, const char *symbolName, AstSUEDeclaration *declaration, Symbol **ss) {
  Symbol *s = findSymbolInScope(ctx->currentScope, symbolName);
  Symbol *old = s;
  const char *name = declaration->name;

  TypeDesc *typeDescriptor;
  if (!s) {
      s = declareSymbol(ctx, symbolKind, symbolName);
      // TODO: compute size
      typeDescriptor = s->typeDescriptor = createTypeDescriptor(ctx, typeId, name, -1);
      typeDescriptor->structInfo = declaration;
  } else {
      if (s->kind != symbolKind) {
          parseError(ctx, "use of '%s' with tag type that does not match previous declaration", name);
          // also point to already defined one
          // TODO: recovery
      } else {
          typeDescriptor = s->typeDescriptor;
          AstSUEDeclaration *existedDeclaration = typeDescriptor->structInfo;
          if (declaration->members) {
            if (existedDeclaration->members) {
                parseError(ctx, "redefinition of '%s'", name);
                // also point to already defined one
                // TODO recovery
            } else {
                typeDescriptor->structInfo = declaration;
            }
          }
      }
  }
  *ss = s;
  return old;
}

Symbol *declareEnumConstantSymbol(ParserContext *ctx, EnumConstant *enumerator) {
  Symbol *s = findSymbolInScope(ctx->currentScope, enumerator->name);
  if (s) {
      const char *suffix = s->kind == EnumConstSymbol ? "of enumerator " : "";
      parseError(ctx, "redefinition %s'%s'", suffix, enumerator->name);
      return NULL; // or 's'?
  }

  s = declareSymbol(ctx, EnumConstSymbol, enumerator->name);
  s->enumerator = enumerator;
  return s;
}

// Types

static TypeDesc errorTypeImpl = { T_ERROR, "<error>", -1, NULL };

TypeDesc *errorTypeDescriptor = &errorTypeImpl;

TypeDesc builtInTypeDescriptors[] = {
    { T_VOID, "void", 0, NULL },

    { T_S1, "signed char", 1, NULL },
    { T_S2, "signed short", 2, NULL },
    { T_S4, "signed int", 4, NULL },
    { T_S8, "signed long", 8, NULL },

    { T_U1, "unsigned char", 1, NULL },
    { T_U2, "unsigned short", 2, NULL },
    { T_U4, "unsigned int", 4, NULL },
    { T_U8, "unsigned long", 8, NULL },

    { T_F4, "float", 4, NULL },
    { T_F8, "double", 8, NULL }
};

TypeRef *makeBasicType(ParserContext *ctx, TypeDesc *descriptor, unsigned flags) {
  TypeRef *ref = (TypeRef *)areanAllocate(ctx->typeArena, sizeof(TypeRef));

  ref->kind = TR_VALUE;
  ref->flags.storage = flags;
  ref->descriptorDesc = descriptor;
}

TypeRef* makePointedType(ParserContext *ctx, SpecifierFlags flags, TypeRef *pointedTo) {
    TypeRef *result = (TypeRef *)areanAllocate(ctx->typeArena, sizeof(TypeRef));
    result->kind = TR_POINTED;
    result->flags.storage = flags.storage;
    result->pointedTo = pointedTo;
    return result;
}

TypeRef *makeArrayType(ParserContext *ctx, int size, TypeRef *elementType) {
    TypeRef *result = (TypeRef *)areanAllocate(ctx->typeArena, sizeof(TypeRef));
    result->kind = TR_ARRAY;
    result->arrayTypeDesc.size = size;
    result->arrayTypeDesc.elementType = elementType;
    return result;
}

TypeRef *makeFunctionType(ParserContext *ctx, TypeRef *returnType, FunctionParams *params) {
    TypeRef *result = (TypeRef *)areanAllocate(ctx->typeArena, sizeof(TypeRef));
    result->kind = TR_FUNCTION;
    result->functionTypeDesc.isVariadic = params->isVariadic;
    result->functionTypeDesc.returnType = returnType;

    Vector *parameters = params->parameters;

    if (parameters) {
        int parameterCount = parameters->size;
        TypeRef **paramStorage = (TypeRef**)areanAllocate(ctx->typeArena, sizeof (TypeRef*) * parameterCount);

        void** paramVector = parameters->storage;

        unsigned i;
        for (i = 0; i < parameterCount; ++i) {
            AstValueDeclaration *parami = (AstValueDeclaration *)paramVector[i];
            paramStorage[i] = parami->type;
        }

        releaseVector(parameters);
        params->parameters = NULL;
        result->functionTypeDesc.parameterCount = parameterCount;
        result->functionTypeDesc.parameters = paramStorage;
    } else {
        result->functionTypeDesc.parameterCount = 0;
        result->functionTypeDesc.parameters = NULL;
    }

    return result;
}

TypeRef *makeFunctionReturnType(ParserContext *ctx, DeclarationSpecifiers *specifiers, Declarator *declarator) {

    TypeRef *type = specifiers->basicType;

    int i;
    for (i = declarator->partsCounter - 1; i >= 0; --i) {
        DeclaratorPart *part = &declarator->declaratorParts[i];
        switch (part->kind) {
        case DPK_POINTER:
            type = makePointedType(ctx, part->flags, type);
            break;
        case DPK_ARRAY:
            type = makeArrayType(ctx, part->arraySize, type);
            break;
        case DPK_FUNCTION:
            return type;
        case DPK_NONE:
        default:
            unreachable("UNKNOWN Declarator Part");
        }
    }

    parseError(ctx, "Expected function declarator here");
    return NULL; // return error type
}


TypeRef *makeTypeRef(ParserContext *ctx, DeclarationSpecifiers *specifiers, Declarator *declarator) {

    TypeRef *type = specifiers->basicType;

    int i;
    for (i = declarator->partsCounter - 1; i >= 0; --i) {
        DeclaratorPart *part = &declarator->declaratorParts[i];
        switch (part->kind) {
        case DPK_POINTER:
            type = makePointedType(ctx, part->flags, type);
            break;
        case DPK_ARRAY:
            type = makeArrayType(ctx, part->arraySize, type);
            break;
        case DPK_FUNCTION:
            type = makeFunctionType(ctx, type, &part->parameters);
            break;
        case DPK_NONE:
        default:
            unreachable("UNKNOWN Declarator Part");
        }
    }

    return type;
}

TypeRef *makeErrorRef(ParserContext *ctx) {
  return makeBasicType(ctx, errorTypeDescriptor, 0);
}
