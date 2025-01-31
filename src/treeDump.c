
#include <assert.h>

#include "sema.h"
#include "treeDump.h"
#include "tokens.h"

static int dumpTypeRefImpl(FILE *output, int indent, const TypeRef *type);
static int dumpTypeDescImpl(FILE *output, int indent, const TypeDesc *desc);
static int dumpAstInitializerImpl(FILE *output, int indent, AstInitializer *init, Boolean compund);
static int dumpAstDeclarationImpl(FILE *output, int indent, AstDeclaration *decl);
static int dumpAstExpressionImpl(FILE *output, int indent, AstExpression *expr);
static int dumpAstStatementImpl(FILE *output, int indent, AstStatement *stmt);

static int putIndent(FILE *output, int indent) {
  int result = indent;
  while (indent--) fputc(' ', output);
  return result;
}

static int wrapIfNeeded(FILE *output, ExpressionType topOp, AstExpression *arg, Boolean forced) {
  unsigned topPrior = opPriority(topOp);
  unsigned argPriority = opPriority(arg->op);

  int result = 0;

  Boolean needParens = forced || topPrior > argPriority;

  if (needParens) {
      result += fprintf(output, "(");
  }

  result += dumpAstExpressionImpl(output, 0, arg);

  if (needParens) {
      result += fprintf(output, ")");
  }

  return result;
}

static char *esaceString(const char *s, size_t l) {
  StringBuffer sb = { 0 };

  unsigned i;
  for (i = 0; i < l - 1; ++i) {
      char c = s[i];
      switch (c) {
      case '\0':
          putSymbol(&sb, '\\');
          putSymbol(&sb, '0');
          break;
      case '\n':
          putSymbol(&sb, '\\');
          putSymbol(&sb, 'n');
          break;
      case '\t':
          putSymbol(&sb, '\\');
          putSymbol(&sb, 't');
          break;
      case '\\':
          putSymbol(&sb, '\\');
          putSymbol(&sb, '\\');
          break;
      case '\a':
          putSymbol(&sb, '\\');
          putSymbol(&sb, 'a');
          break;
      case '\b':
          putSymbol(&sb, '\\');
          putSymbol(&sb, 'b');
          break;
      case '\f':
          putSymbol(&sb, '\\');
          putSymbol(&sb, 'f');
          break;
      case '\r':
          putSymbol(&sb, '\\');
          putSymbol(&sb, 'r');
          break;
      case '\v':
          putSymbol(&sb, '\\');
          putSymbol(&sb, 'v');
          break;
      case '?':
          putSymbol(&sb, '\\');
          putSymbol(&sb, '?');
          break;
      default:
          putSymbol(&sb, c);
      }
  }
  putSymbol(&sb, '\0');
  return sb.ptr;
}

static int dumpAstExpressionImpl(FILE *output, int indent, AstExpression *expr) {
  int result = putIndent(output, indent);

  ExpressionType op = expr->op;
  ExpressionType op2 = E_ERROR;

  const char *mnemonic;
  switch (op) {
    case E_CONST: {
        AstConst *cnts = &expr->constExpr;
        switch (cnts->op) {
        case CK_INT_CONST: result += fprintf(output, "%lld", cnts->i); break;
        case CK_FLOAT_CONST: result += fprintf(output, "%Lf", cnts->f); break;
        case CK_STRING_LITERAL: {
            char *escaped = esaceString(cnts->l.s, cnts->l.length);
            result += fprintf(output, "\"%s\"", escaped);
            releaseHeap(escaped);
            break;
          }
        }
        break;
    }
    case E_VA_ARG:
      result += fprintf(output, "__builtin_va_arg(");
      result += dumpAstExpressionImpl(output, 0, expr->vaArg.va_list);
      result += fprintf(output, ", ");
      result += dumpTypeRefImpl(output, 0, expr->vaArg.argType);
      result += fprintf(output, ")");
      break;
    case E_ERROR:
      result += fprintf(output, "%s", "ERROR EXPR");
      break;
    case E_NAMEREF:
      result += fprintf(output, "%s", expr->nameRefExpr.s->name);
      break;
    case E_COMPOUND:
      result += fprintf(output, "(");
      result += dumpTypeRefImpl(output, 0, expr->type);
      result += fprintf(output, ")\n");
      result += dumpAstInitializerImpl(output, indent + 2, expr->compound, TRUE);
      break;
    case E_LABEL_REF:
      result += fprintf(output, "&&%s", expr->label);
      break;
    case E_PAREN:
      result += fprintf(output, "(");
      result += dumpAstExpressionImpl(output, 0, expr->parened);
      result += fprintf(output, ")");
      break;
    case E_BLOCK:
      result += fprintf(output, "(");
      result += dumpAstStatementImpl(output, 0, expr->block);
      result += fprintf(output, ")");
      break;
    case E_CALL: {
       AstCallExpression *callExpr = &expr->callExpr;
       result += wrapIfNeeded(output, op, callExpr->callee, FALSE);
       result += fprintf(output, "(");
       int i;
       AstExpressionList *arguments = callExpr->arguments;
       int first = TRUE;
       while (arguments) {
           if (first) {
               first = FALSE;
           } else {
               result += fprintf(output, ", ");
           }
          result += dumpAstExpressionImpl(output, 0, arguments->expression);
          arguments = arguments->next;
       }
       result += fprintf(output, ")");
       break;
    }
    case E_BIT_EXTEND: {
        result += fprintf(output, "(");
        result += fprintf(output, "%d <-- %d # ", expr->extendExpr.isUnsigned ? 0 : 1, expr->extendExpr.w);
        result += wrapIfNeeded(output, op, expr->extendExpr.argument, FALSE);
        result += fprintf(output, ")");
        break;
    }
    case E_CAST: {
        AstCastExpression *castExpr = &expr->castExpr;
        result += fprintf(output, "(");
        result += dumpTypeRefImpl(output, 0, castExpr->type);
        result += fprintf(output, ")");
        result += wrapIfNeeded(output, op, castExpr->argument, FALSE);
        break;
    }
    case E_TERNARY: {
        AstTernaryExpression *trnExpr = &expr->ternaryExpr;
        result += dumpAstExpressionImpl(output, 0, trnExpr->condition);
        result += fprintf(output, " ? ");
        result += dumpAstExpressionImpl(output, 0, trnExpr->ifTrue);
        result += fprintf(output, " : ");
        result += dumpAstExpressionImpl(output, 0, trnExpr->ifFalse);
        break;
    }
    case EF_ARROW:
    case EF_DOT: {
        AstFieldExpression *fieldExpr = &expr->fieldExpr;
        result += wrapIfNeeded(output, op, fieldExpr->recevier, FALSE);
        if (expr->op == EF_ARROW) {
            result += fprintf(output, "->%s", fieldExpr->member->name);
        } else {
            result += fprintf(output, ".%s", fieldExpr->member->name);
        }
        break;
    }
    case EU_PRE_INC: result += fprintf(output, "++"); goto pre;
    case EU_PRE_DEC: result += fprintf(output, "--"); goto pre;
    case EU_DEREF: result += fprintf(output, "*"); goto pre;
    case EU_REF: result += fprintf(output, "&"); goto pre;
    case EU_PLUS: result += fprintf(output, "+"); goto pre;
    case EU_MINUS: result += fprintf(output, "-"); goto pre;
    case EU_TILDA: result += fprintf(output, "~"); goto pre;
    case EU_EXL: result += fprintf(output, "!"); goto pre;
    pre:
        result += wrapIfNeeded(output, op, expr->unaryExpr.argument, FALSE);
        break;
    case EU_POST_INC: mnemonic = "++"; goto post;
    case EU_POST_DEC: mnemonic = "--"; goto post;
    post:
        result += wrapIfNeeded(output, op, expr->unaryExpr.argument, FALSE);
        result += fprintf(output, "%s", mnemonic);
        break;

    case EB_ADD: mnemonic = " +"; goto binary;
    case EB_SUB: mnemonic = " -"; goto binary;
    case EB_MUL: mnemonic = " *"; goto binary;
    case EB_DIV: mnemonic = " /"; goto binary;
    case EB_MOD: mnemonic = " %"; goto binary;
    case EB_LHS: mnemonic = " <<"; goto binary;
    case EB_RHS: mnemonic = " >>"; goto binary;
    case EB_AND: mnemonic = " &"; goto binary;
    case EB_OR:  mnemonic = " |"; goto binary;
    case EB_XOR: mnemonic = " ^"; goto binary;
    case EB_ANDAND:  mnemonic = " &&"; goto binary;
    case EB_OROR:  mnemonic = " ||"; goto binary;
    case EB_EQ:  mnemonic = " =="; goto binary;
    case EB_NE:  mnemonic = " !="; goto binary;
    case EB_LT:  mnemonic = " <"; goto binary;
    case EB_LE:  mnemonic = " <="; goto binary;
    case EB_GT:  mnemonic = " >"; goto binary;
    case EB_GE:  mnemonic = " >="; goto binary;
    case EB_COMMA: mnemonic = ","; goto binary;
    case EB_ASSIGN: mnemonic = " ="; goto binary;
    case EB_ASG_ADD: mnemonic = " +="; goto binary;
    case EB_ASG_SUB: mnemonic = " -="; goto binary;
    case EB_ASG_MUL: mnemonic = " *="; goto binary;
    case EB_ASG_DIV: mnemonic = " /="; goto binary;
    case EB_ASG_MOD: mnemonic = " %="; goto binary;
    case EB_ASG_SHL: mnemonic = " <<="; goto binary;
    case EB_ASG_SHR: mnemonic = " >>="; goto binary;
    case EB_ASG_AND: mnemonic = " &="; goto binary;
    case EB_ASG_OR: mnemonic = " |="; goto binary;
    case EB_ASG_XOR: mnemonic = " ^="; goto binary;
    binary:
      result += wrapIfNeeded(output, op, expr->binaryExpr.left, FALSE);
      result += fprintf(output, "%s ", mnemonic);
      op2 = expr->binaryExpr.right->op;
      // In expressions like x - (a + b) or x - (a - b) rhs has to be enparened
      result += wrapIfNeeded(output, op, expr->binaryExpr.right, op == EB_SUB && (op2 == EB_SUB || op2 == EB_ADD));
      break;
    case EB_A_ACC: /** a[b] */
      result += wrapIfNeeded(output, op, expr->binaryExpr.left, FALSE);
      result += fprintf(output, "[");
      result += dumpAstExpressionImpl(output, 0, expr->binaryExpr.right);
      result += fprintf(output, "]");
      break;
    default:
      break;
  }

  return result;
}

static int dumpAstStatementImpl(FILE *output, int indent, AstStatement *stmt) {
  int result = 0;

  switch (stmt->statementKind) {
   case SK_BLOCK: {
      AstBlock *block = &stmt->block;
      int i;
      AstStatementList *stmts = block->stmts;
      Boolean first = TRUE;
      while (stmts) {
          if (first) {
              first = FALSE;
          } else {
              result += fprintf(output, "\n");
          }
         result += dumpAstStatementImpl(output, indent, stmts->stmt);
         stmts = stmts->next;
      }
      result += putIndent(output, indent);
      break;
   }
   case SK_EXPR_STMT:
      result += putIndent(output, indent);
      result += dumpAstExpressionImpl(output, 0, stmt->exprStmt.expression);
      break;
   case SK_LABEL: {
        result += putIndent(output, indent);
        AstLabelStatement *lbl = &stmt->labelStmt;
        switch (lbl->kind) {
        case LK_LABEL: result += fprintf(output, "%s: ", lbl->label); break;
        case LK_DEFAULT: result += fprintf(output, "DEFAULT: "); break;
        case LK_CASE: result += fprintf(output, "CASE %ld: ", lbl->caseConst); break;
        }
        result += dumpAstStatementImpl(output, 0, lbl->body);
        break;
   }
   case SK_DECLARATION:
       result += dumpAstDeclarationImpl(output, indent, stmt->declStmt.declaration);
       break;
   case SK_EMPTY:
       break;
   case SK_IF: {
       AstIfStatement *ifStmt = &stmt->ifStmt;
       result += putIndent(output, indent);
       result += fprintf(output, "IF (");
       result += dumpAstExpressionImpl(output, 0, ifStmt->condition);
       result += fprintf(output, ")\n");
       result += putIndent(output, indent);
       result += fprintf(output, "THEN\n");
       result += dumpAstStatementImpl(output, indent + 2, ifStmt->thenBranch);
       result += fprintf(output, "\n");
       if (ifStmt->elseBranch) {
         result += putIndent(output, indent);
         result += fprintf(output, "ELSE\n");
         result += dumpAstStatementImpl(output, indent + 2, ifStmt->elseBranch);
         result += fprintf(output, "\n");
       }
       result += putIndent(output, indent);
       result += fprintf(output, "END_IF");
       break;
    }
    case SK_SWITCH: {
       AstSwitchStatement *switchStmt = &stmt->switchStmt;
       result += putIndent(output, indent);
       result += fprintf(output, "SWITCH (");
       result += dumpAstExpressionImpl(output, 0, switchStmt->condition);
       result += fprintf(output, ")\n");
       result += dumpAstStatementImpl(output, indent + 2, switchStmt->body);
       result += fprintf(output, "\n");
       result += putIndent(output, indent);
       result += fprintf(output, "END_SWITCH");
       break;
    }
    case SK_WHILE:
    case SK_DO_WHILE: {
       AstLoopStatement *loop = &stmt->loopStmt;
       result += putIndent(output, indent);
       if (stmt->statementKind == SK_WHILE) {
           result += fprintf(output, "WHILE (");
           result += dumpAstExpressionImpl(output, 0, loop->condition);
           result += fprintf(output, ")\n");
       } else {
           result += fprintf(output, "DO\n");
       }
       result += dumpAstStatementImpl(output, indent + 2, loop->body);
       result += fprintf(output, "\n");
       result += putIndent(output, indent);
       if (stmt->statementKind == SK_WHILE) {
           result += fprintf(output, "END_WHILE");
       } else {
           result += fprintf(output, "WHILE (");
           result += dumpAstExpressionImpl(output, 0, loop->condition);
           result += fprintf(output, ")");
       }
       break;
    }
    case SK_FOR: {

       AstForStatement *forLoop = &stmt->forStmt;
       result += putIndent(output, indent);
       result += fprintf(output, "FOR (");
       if (forLoop->initial) {
         AstStatementList *stmts = forLoop->initial;
         // TODO: improve rendering
         for (; stmts; stmts = stmts->next) {
          result += dumpAstStatementImpl(output, 0, stmts->stmt);
          if (stmts->next) {
              result += fprintf(output, ", ");
          }
         }
       }
       result += fprintf(output, "; ");

       if (forLoop->condition) {
         result += dumpAstExpressionImpl(output, 0, forLoop->condition);
         result += fprintf(output, "; ");
       }

       if (forLoop->modifier) {
         result += dumpAstExpressionImpl(output, 0, forLoop->modifier);
       }

       result += fprintf(output, ")\n");
       result += dumpAstStatementImpl(output, indent + 2, forLoop->body);
       result += fprintf(output, "\n");

       result += putIndent(output, indent);
       result += fprintf(output, "END_FOR");

       break;
    }
    case SK_BREAK:
      result += putIndent(output, indent);
      result += fprintf(output, "BREAK");
      break;
    case SK_CONTINUE:
      result += putIndent(output, indent);
      result += fprintf(output, "CONTINUE");
      break;
    case SK_GOTO_L:
      result += putIndent(output, indent);
      result += fprintf(output, "GOTO %s", stmt->jumpStmt.label);
      break;
    case SK_GOTO_P:
      result += putIndent(output, indent);
      result += fprintf(output, "GOTO *");
      result += dumpAstExpressionImpl(output, 0, stmt->jumpStmt.expression);
      break;
    case SK_RETURN:
      result += putIndent(output, indent);
      result += fprintf(output, "RETURN");
      if (stmt->jumpStmt.expression) {
          result += fprintf(output, " ");
          result += dumpAstExpressionImpl(output, 0, stmt->jumpStmt.expression);
      }
      break;
    case SK_ERROR:
      result += putIndent(output, indent);
      result += fprintf(output, "ERROR_STATEMENT");
      break;
   }

  return result;
}


static int dumpAstInitializerImpl(FILE *output, int indent, AstInitializer *init, Boolean compund) {
  int result = 0;
  result += putIndent(output, indent);
  if (init->kind == IK_EXPRESSION) {
      if (compund) {
        result += dumpTypeRef(output, init->slotType);
        result += fprintf(output, " #%d <--- ", init->offset);
      }
      result += dumpAstExpressionImpl(output, 0, init->expression);
  } else {
      assert(init->kind == IK_LIST);
      AstInitializerList *nested = init->initializerList;
      Boolean first = TRUE;
      result += fprintf(output, "INIT_BEGIN\n");
      while (nested) {
          if (first) {
              first = FALSE;
          } else {
              result += fprintf(output, "\n");
          }
          result += dumpAstInitializerImpl(output, indent + 2, nested->initializer, TRUE);
          nested = nested->next;
      }
      result += fprintf(output, "\n");
      result += putIndent(output, indent);
      result += fprintf(output, "INIT_END");
  }
  return result;
}

static int dumpAstValueDeclarationImpl(FILE *output, int indent, AstValueDeclaration *value) {
  int result = putIndent(output, indent);

  Boolean hasBits = FALSE;
  if (value->flags.bits.isStatic) {
      result += fprintf(output, "S");
      hasBits = TRUE;
  }
  if (value->flags.bits.isExternal) {
      result += fprintf(output, "E");
      hasBits = TRUE;
  }
  if (value->flags.bits.isRegister) {
      result += fprintf(output, "R");
      hasBits = TRUE;
  }

  if (hasBits) {
    result += fprintf(output, " ");
  }

  if (value->kind == VD_PARAMETER) {
    result += fprintf(output, "#%d: ", value->index);
  }
  result += dumpTypeRefImpl(output, 0, value->type);
  if (value->name) {
      result += fprintf(output, " %s", value->name);
  }

  if (value->kind == VD_VARIABLE) {
      if (value->initializer) {
          result += fprintf(output, " = \\\n");
          result += dumpAstInitializerImpl(output, indent + 2, value->initializer, FALSE);
      }
  }

  return result;
}


int renderTypeDesc(const TypeDesc *desc, char *b, int bufferSize) {
  switch (desc->typeId) {
    case T_ENUM:
      return snprintf(b, bufferSize, "ENUM %s", desc->typeDefinition->name);
    case T_UNION:
      return snprintf(b, bufferSize, "UNION %s", desc->typeDefinition->name);
    case T_STRUCT:
      return snprintf(b, bufferSize, "STRUCT %s", desc->typeDefinition->name);
    case T_ERROR:
      return snprintf(b, bufferSize, "ERROR TYPE");
    default:
      return snprintf(b, bufferSize, "%s", desc->name);
  }
}

static int dumpTypeDescImpl(FILE *output, int indent, const TypeDesc *desc) {
  int result = putIndent(output, indent);
  char b[1024] = { 0 };
  renderTypeDesc(desc, b, sizeof b);
  result += fprintf(output, "%s", b);
  return result;
}

int renderTypeRef(const TypeRef *type, char *b, int bufferSize) {
  Boolean hasBits = FALSE;

  char *s = b;
  int l = 0;
  if (type->flags.bits.isConst) {
    l = snprintf(b, bufferSize, "C");
    bufferSize -=l;
    b +=l;
    hasBits = TRUE;
  }

  if (bufferSize <= 0) goto done;

  if (type->flags.bits.isVolatile) {
    l = snprintf(b, bufferSize, "V");
    bufferSize -=l;
    b +=l;
    hasBits = TRUE;
  }

  if (bufferSize <= 0) goto done;

  if (hasBits) {
      l = snprintf(b, bufferSize, " ");
      bufferSize -=l;
      b +=l;
  }

  if (bufferSize <= 0) goto done;

  // TODO: support multi-line
  switch (type->kind) {
  case TR_VALUE:
      b += renderTypeDesc(type->descriptorDesc, b, bufferSize);
      break;
  case TR_POINTED:
      l = snprintf(b, bufferSize, "*"); b += l; bufferSize -= l;
      if (bufferSize <= 0) goto done;
      b += renderTypeRef(type->pointed, b, bufferSize);
      break;
  case TR_ARRAY: {
        const ArrayTypeDescriptor *desc = &type->arrayTypeDesc;
        int wrap = desc->elementType->kind != TR_VALUE ? TRUE : FALSE;
        if (wrap) {
            l = snprintf(b, bufferSize, "("); b += l; bufferSize -= l;
            if (bufferSize <= 0) goto done;
        }

        l = renderTypeRef(desc->elementType, b, bufferSize); b += l; bufferSize -= l;
        if (bufferSize <= 0) goto done;

        if (wrap) {
            l = snprintf(b, bufferSize, ")"); b += l; bufferSize -= l;
            if (bufferSize <= 0) goto done;
        }

        const char *st = desc->isStatic ? "static" : "";

        if (desc->size) {
            l = snprintf(b, bufferSize, "[%s%s%d]", st, st[0] ? " " : "", desc->size);
        } else {
            l = snprintf(b, bufferSize, "[%s]", st);
        }
        b += l; bufferSize -=l;
      }
      break;
  case TR_VLA: {
      const VLADescriptor *desc = &type->vlaDescriptor;
      int wrap = desc->elementType->kind != TR_VALUE ? TRUE : FALSE;
      if (wrap) {
          l = snprintf(b, bufferSize, "("); b += l; bufferSize -= l;
          if (bufferSize <= 0) goto done;
      }

      l = renderTypeRef(desc->elementType, b, bufferSize); b += l; bufferSize -= l;
      if (bufferSize <= 0) goto done;

      if (wrap) {
          l = snprintf(b, bufferSize, ")"); b += l; bufferSize -= l;
          if (bufferSize <= 0) goto done;
      }

      if (desc->sizeSymbol) {
          l = snprintf(b, bufferSize, "[%s]", desc->sizeSymbol->name);
      } else {
          l = snprintf(b, bufferSize, "[*]");
      }
      b += l; bufferSize -=l;
    }
    break;
  case TR_FUNCTION: {
      const FunctionTypeDescriptor *desc = &type->functionTypeDesc;
      l = snprintf(b, bufferSize, "{"); b += l; bufferSize -= l;
      if (bufferSize <= 0) goto done;
      l = renderTypeRef(desc->returnType, b, bufferSize); b += l; bufferSize -= l;
      if (bufferSize <= 0) goto done;
      l = snprintf(b, bufferSize, " ("); b += l; bufferSize -= l;
      if (bufferSize <= 0) goto done;

      int i = 0;

      TypeList *paramList = desc->parameters;

      while (paramList) {
          if (i++)  {
              l = snprintf(b, bufferSize, ", "); b += l; bufferSize -= l;
              if (bufferSize <= 0) goto done;
          }
          l = renderTypeRef(paramList->type, b, bufferSize); b += l; bufferSize -= l;
          if (bufferSize <= 0) goto done;
          paramList = paramList->next;
      }

      if (desc->isVariadic) {
          l = snprintf(b, bufferSize, ", ..."); b += l; bufferSize -= l;
          if (bufferSize <= 0) goto done;
      }

      l = snprintf(b, bufferSize, ")}"); b += l; bufferSize -= l;
      break;
    }
  case TR_BITFIELD:
      l = renderTypeRef(type->bitFieldDesc.storageType, b, bufferSize); b += l; bufferSize -= l;
      if (bufferSize <= 0) goto done;
      l = snprintf(b, bufferSize, ":%u:%u", type->bitFieldDesc.offset, type->bitFieldDesc.width);
      break;
  }

  done:

  return b - s;
}

static int dumpTypeRefImpl(FILE *output, int indent, const TypeRef *type) {
  int result = putIndent(output, indent);
  char b[1024] = { 0 };

  renderTypeRef(type, b, sizeof b);
  result += fprintf(output, "%s", b);
  return result;
}

static int dumpAstFuntionDeclarationImpl(FILE *output, int indent, AstFunctionDeclaration *decl) {
  int result = putIndent(output, indent);

  Boolean hasBits = FALSE;
  if (decl->flags.bits.isStatic) {
      result += fprintf(output, "S");
      hasBits = TRUE;
  }
  if (decl->flags.bits.isExternal) {
      result += fprintf(output, "E");
      hasBits = TRUE;
  }

  if (hasBits) {
    result += fprintf(output, " ");
  }

  result += fprintf(output, "FUN ");
  result += dumpTypeRefImpl(output, 0, decl->returnType);
  result += fprintf(output, " ");
  result += fprintf(output, "%s ", decl->name);
//  result += fprintf(output, "PARAM COUNT %d", decl->parameterCount);
  int i;

  AstValueDeclaration *parameter = decl->parameters;

  while (parameter) {
    result += fprintf(output, "\n");
    result += dumpAstValueDeclarationImpl(output, indent + 2, parameter);
    parameter = parameter->next;
  }

  if (decl->isVariadic) {
      result += fprintf(output, "\n");
      result += putIndent(output, indent + 2);
      result += fprintf(output, "## ...");
  }

  return result;
}

static int dumpTypeDefinitionImpl(FILE *output, int indent, TypeDefiniton *definition) {
  int result = putIndent(output, indent);

  enum TypeDefinitionKind kind = definition->kind;

  if (kind == TDK_TYPEDEF) {
      result += fprintf(output, "TYPEDF %s = ", definition->name ? definition->name : "<no_name>");
      result += dumpTypeRefImpl(output, 0, definition->type);
      return result;
  }

  int isEnum = kind == TDK_ENUM;
  const char *prefix = kind == TDK_STRUCT ? "STRUCT" : isEnum ? "ENUM" : "UNION";
  result += fprintf(output, "%s", prefix);

  if (definition->name) {
    result += fprintf(output, " %s", definition->name);
  }

  if (isEnum) {
      EnumConstant *enumerator = definition->enumerators;
      if (enumerator) result += fprintf(output, "\n");
      for (; enumerator; enumerator = enumerator->next) {
          result += putIndent(output, indent + 2);
          result += fprintf(output, "%s = %d\n", enumerator->name, enumerator->value);
      }
      if (definition->enumerators) {
          result += putIndent(output, indent);
          result += fprintf(output, "ENUM_END");
      }
  } else {
      StructualMember *member = definition->members;
      if (member) result += fprintf(output, "\n");
      for (; member; member = member->next) {
          result += dumpTypeRefImpl(output, indent + 2, member->type);
          result += fprintf(output, " %s #%u\n", member->name, member->offset);
      }
      if (definition->members) {
        result += putIndent(output, indent);
        result += fprintf(output, "%s_END", prefix);
      }
  }

  return result;
}

static int dumpAstDeclarationImpl(FILE *output, int indent, AstDeclaration *decl) {
  int result = 0;

  switch (decl->kind) {
  case DK_PROTOTYPE:
      result += dumpAstFuntionDeclarationImpl(output, indent, decl->functionProrotype);
      return result;
  case DK_VAR:
      result += dumpAstValueDeclarationImpl(output, indent, decl->variableDeclaration);
      return result;
  }

  unreachable("Declaration node corruption, unknown declaration kind");
}

static int dumpAstFunctionDefinitionImpl(FILE *output, int indent, AstFunctionDefinition *definition) {
  int result = 0;

  dumpAstFuntionDeclarationImpl(output, indent, definition->declaration);
  result += fprintf(output, "\n");

  result += putIndent(output, indent);
  result += fprintf(output, "BEGIN\n");
  dumpAstStatementImpl(output, indent + 2, definition->body);
  result += fprintf(output, "\n");
  result += putIndent(output, indent);
  result += fprintf(output, "END");


  return result;
}

static int dumpTranslationUnitImpl(FILE *output, int indent, AstTranslationUnit *unit) {
  if (unit->kind == TU_DECLARATION) {
     return dumpAstDeclarationImpl(output, indent, unit->declaration);
  } else {
     return dumpAstFunctionDefinitionImpl(output, indent, unit->definition);
  }
}


static int dumpTypeDefinitions(FILE *output, int indent, TypeDefiniton *typeDefinitions) {

  if (typeDefinitions == NULL) return 0;

  int result = dumpTypeDefinitions(output, indent, typeDefinitions->next);

  result += dumpTypeDefinitionImpl(output, indent, typeDefinitions);
  result += fprintf(output, "\n----\n");

  return result;
}

int dumpAstFile(FILE *output, AstFile *file, TypeDefiniton *typeDefinitions) {
  int r = 0;
  r += fprintf(output, "FILE %s\n", file->fileName);
  r += dumpTypeDefinitions(output, 2, typeDefinitions);
  int i = 0;
  AstTranslationUnit *unit = file->units;
  while (unit) {
      if (i++) r += fprintf(output, "\n----\n");
      r += dumpTranslationUnitImpl(output, 2, unit);
      unit = unit->next;
  }
  return r;
}

void dumpLocation(FILE *output, AstExpression *t) {
  const char *file = NULL;
  unsigned line = 0;

  fileAndLine(t->coordinates.left, &line, &file);

  fprintf(output, "Token location: %s:%u\n", file, line);
}

int dumpAstExpression(FILE *output, AstExpression *expr) {
  return dumpAstExpressionImpl(output, 0, expr);
}

int dumpAstStatement(FILE *output, AstStatement *stmt) {
  return dumpAstStatementImpl(output, 0, stmt);
}

int dumpAstDeclaration(FILE *output, AstDeclaration *declaration) {
  return dumpAstDeclarationImpl(output, 0, declaration);
}

int dumpTypeRef(FILE *output, const TypeRef *type) {
  return dumpTypeRefImpl(output, 0, type);
}

int dumpTypeDesc(FILE *output, const TypeDesc *desc) {
  return dumpTypeDescImpl(output, 0, desc);
}

int dumpAstValueDeclaration(FILE *output, AstValueDeclaration *param) {
  return dumpAstValueDeclarationImpl(output, 0, param);
}

int dumpAstInitializer(FILE *outout, AstInitializer *init) {
  return dumpAstInitializerImpl(outout, 0, init, FALSE);
}
