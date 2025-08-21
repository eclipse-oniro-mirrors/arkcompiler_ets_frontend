/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as path from 'node:path';
import * as ts from 'typescript';
import * as fs from 'fs';
import type { IsEtsFileCallback } from '../IsEtsFileCallback';
import { FaultID } from '../Problems';
import { ARKTS_IGNORE_DIRS, ARKTS_IGNORE_DIRS_OH_MODULES, ARKTS_IGNORE_FILES } from './consts/ArktsIgnorePaths';
import { ES_VALUE } from './consts/ESObject';
import { EXTENDED_BASE_TYPES } from './consts/ExtendedBaseTypes';
import { SENDABLE_DECORATOR } from './consts/SendableAPI';
import { USE_SHARED } from './consts/SharedModuleAPI';
import { STANDARD_LIBRARIES } from './consts/StandardLibraries';
import {
  ARKTS_COLLECTIONS_D_ETS,
  ARKTS_LANG_D_ETS,
  COLLECTIONS_NAMESPACE,
  ISENDABLE_TYPE,
  LANG_NAMESPACE
} from './consts/SupportedDetsIndexableTypes';
import { TYPED_ARRAYS } from './consts/TypedArrays';
import { TYPED_COLLECTIONS } from './consts/TypedCollections';
import { forEachNodeInSubtree } from './functions/ForEachNodeInSubtree';
import { getScriptKind } from './functions/GetScriptKind';
import { isStdLibrarySymbol, isStdLibraryType } from './functions/IsStdLibrary';
import { isStructDeclaration, isStructDeclarationKind } from './functions/IsStruct';
import type { NameGenerator } from './functions/NameGenerator';
import { srcFilePathContainsDirectory } from './functions/PathHelper';
import { isAssignmentOperator } from './functions/isAssignmentOperator';
import { isIntrinsicObjectType } from './functions/isIntrinsicObjectType';
import type { LinterOptions } from '../LinterOptions';
import { ETS } from './consts/TsSuffix';
import { STRINGLITERAL_NUMBER, STRINGLITERAL_NUMBER_ARRAY } from './consts/StringLiteral';
import { ETS_MODULE, PATH_SEPARATOR, VALID_OHM_COMPONENTS_MODULE_PATH } from './consts/OhmUrl';
import { EXTNAME_ETS, EXTNAME_JS, EXTNAME_D_ETS } from './consts/ExtensionName';
import { CONCAT_ARRAY, STRING_ERROR_LITERAL } from './consts/Literals';
import { INT_MIN, INT_MAX, LARGE_NUMBER_MIN, LARGE_NUMBER_MAX } from './consts/NumericalConstants';
import { IGNORE_TYPE_LIST } from './consts/TypesToBeIgnored';

export const PROMISE_METHODS = new Set(['all', 'race', 'any', 'resolve', 'allSettled']);
export const PROMISE_METHODS_WITH_NO_TUPLE_SUPPORT = new Set(['all', 'race', 'any', 'allSettled']);
export const SYMBOL = 'Symbol';
export const SYMBOL_CONSTRUCTOR = 'SymbolConstructor';
const ITERATOR = 'iterator';

export type CheckType = (this: TsUtils, t: ts.Type) => boolean;

export class TsUtils {
  constructor(
    private readonly tsTypeChecker: ts.TypeChecker,
    private readonly options: LinterOptions
  ) {}

  entityNameToString(name: ts.EntityName): string {
    if (ts.isIdentifier(name)) {
      return name.escapedText.toString();
    }
    return this.entityNameToString(name.left) + this.entityNameToString(name.right);
  }

  isNumberLikeType(tsType: ts.Type): boolean {
    if (this.options.useRtLogic && tsType.isUnion()) {
      for (const tsCompType of tsType.types) {
        if ((tsCompType.flags & ts.TypeFlags.NumberLike) === 0) {
          return false;
        }
      }
      return true;
    }
    return (tsType.getFlags() & ts.TypeFlags.NumberLike) !== 0;
  }

  static isBooleanLikeType(tsType: ts.Type): boolean {
    return (tsType.getFlags() & ts.TypeFlags.BooleanLike) !== 0;
  }

  static isDestructuringAssignmentLHS(tsExpr: ts.ArrayLiteralExpression | ts.ObjectLiteralExpression): boolean {

    /*
     * Check whether given expression is the LHS part of the destructuring
     * assignment (or is a nested element of destructuring pattern).
     */
    let tsParent = tsExpr.parent;
    let tsCurrentExpr: ts.Node = tsExpr;
    while (tsParent) {
      if (
        ts.isBinaryExpression(tsParent) &&
        isAssignmentOperator(tsParent.operatorToken) &&
        tsParent.left === tsCurrentExpr
      ) {
        return true;
      }

      if (
        (ts.isForStatement(tsParent) || ts.isForInStatement(tsParent) || ts.isForOfStatement(tsParent)) &&
        tsParent.initializer &&
        tsParent.initializer === tsCurrentExpr
      ) {
        return true;
      }

      tsCurrentExpr = tsParent;
      tsParent = tsParent.parent;
    }

    return false;
  }

  static isEnumType(tsType: ts.Type, checkUnion?: boolean): boolean {
    // when type equals `typeof <Enum>`, only symbol contains information about it's type.
    const isEnumSymbol = tsType.symbol && this.isEnum(tsType.symbol);
    // otherwise, we should analyze flags of the type itself
    const isEnumType = !!(tsType.flags & ts.TypeFlags.Enum) || !!(tsType.flags & ts.TypeFlags.EnumLiteral);

    if (isEnumSymbol || isEnumType) {
      return true;
    }

    if (checkUnion && tsType.isUnion()) {
      return tsType.types.some((t) => {
        return TsUtils.isEnumType(t);
      });
    }

    return false;
  }

  static isEnum(tsSymbol: ts.Symbol): boolean {
    return !!(tsSymbol.flags & ts.SymbolFlags.Enum);
  }

  static hasModifier(
    tsModifiers: readonly ts.Modifier[] | ts.NodeArray<ts.ModifierLike> | undefined,
    tsModifierKind: number
  ): boolean {
    if (!tsModifiers) {
      return false;
    }

    for (const tsModifier of tsModifiers) {
      if (tsModifier.kind === tsModifierKind) {
        return true;
      }
    }

    return false;
  }

  static unwrapParenthesized(tsExpr: ts.Expression): ts.Expression {
    let unwrappedExpr = tsExpr;
    while (ts.isParenthesizedExpression(unwrappedExpr)) {
      unwrappedExpr = unwrappedExpr.expression;
    }

    return unwrappedExpr;
  }

  followIfAliased(sym: ts.Symbol): ts.Symbol {
    if ((sym.getFlags() & ts.SymbolFlags.Alias) !== 0) {
      return this.tsTypeChecker.getAliasedSymbol(sym);
    }
    return sym;
  }

  private readonly trueSymbolAtLocationCache = new Map<ts.Node, ts.Symbol | null>();

  trueSymbolAtLocation(node: ts.Node): ts.Symbol | undefined {
    const cache = this.trueSymbolAtLocationCache;
    const val = cache.get(node);
    if (val !== undefined) {
      return val !== null ? val : undefined;
    }
    let sym = this.tsTypeChecker.getSymbolAtLocation(node);
    if (sym === undefined) {
      cache.set(node, null);
      return undefined;
    }
    sym = this.followIfAliased(sym);
    cache.set(node, sym);
    return sym;
  }

  private static isTypeDeclSyntaxKind(kind: ts.SyntaxKind): boolean {
    return (
      isStructDeclarationKind(kind) ||
      kind === ts.SyntaxKind.EnumDeclaration ||
      kind === ts.SyntaxKind.ClassDeclaration ||
      kind === ts.SyntaxKind.InterfaceDeclaration ||
      kind === ts.SyntaxKind.TypeAliasDeclaration
    );
  }

  static symbolHasDuplicateName(symbol: ts.Symbol, tsDeclKind: ts.SyntaxKind): boolean {

    /*
     * Type Checker merges all declarations with the same name in one scope into one symbol.
     * Thus, check whether the symbol of certain declaration has any declaration with
     * different syntax kind.
     */
    const symbolDecls = symbol?.getDeclarations();
    if (symbolDecls) {
      for (const symDecl of symbolDecls) {
        const declKind = symDecl.kind;
        // we relax arkts-unique-names for namespace collision with class/interface/enum/type/struct
        const isNamespaceTypeCollision =
          TsUtils.isTypeDeclSyntaxKind(declKind) && tsDeclKind === ts.SyntaxKind.ModuleDeclaration ||
          TsUtils.isTypeDeclSyntaxKind(tsDeclKind) && declKind === ts.SyntaxKind.ModuleDeclaration;

        /*
         * Don't count declarations with 'Identifier' syntax kind as those
         * usually depict declaring an object's property through assignment.
         */
        if (declKind !== ts.SyntaxKind.Identifier && declKind !== tsDeclKind && !isNamespaceTypeCollision) {
          return true;
        }
      }
    }

    return false;
  }

  static isPrimitiveType(type: ts.Type): boolean {
    const f = type.getFlags();
    return (
      (f & ts.TypeFlags.Boolean) !== 0 ||
      (f & ts.TypeFlags.BooleanLiteral) !== 0 ||
      (f & ts.TypeFlags.Number) !== 0 ||
      (f & ts.TypeFlags.NumberLiteral) !== 0

    /*
     *  In ArkTS 'string' is not a primitive type. So for the common subset 'string'
     *  should be considered as a reference type. That is why next line is commented out.
     * (f & ts.TypeFlags.String) != 0 || (f & ts.TypeFlags.StringLiteral) != 0
     */
    );
  }

  checkStatementForErrorClass(stmt: ts.ThrowStatement): boolean {
    const newExpr = stmt.expression;
    if (!ts.isNewExpression(newExpr)) {
      return true;
    }
    const ident = newExpr.expression;
    if (!ts.isIdentifier(ident)) {
      return true;
    }

    if (ident.text === STRING_ERROR_LITERAL) {
      return false;
    }

    const declaration = this.getDeclarationNode(ident);
    if (!declaration || ident.text.includes(STRING_ERROR_LITERAL)) {
      return false;
    }

    if (!declaration || !ident.text.includes(STRING_ERROR_LITERAL)) {
      return true;
    }

    if (!ts.isClassDeclaration(declaration)) {
      return true;
    }

    if (!declaration.heritageClauses) {
      return true;
    }

    return !this.includesErrorClass(declaration.heritageClauses);
  }

  includesErrorClass(hClauses: ts.NodeArray<ts.HeritageClause>): boolean {
    void this;
    let includesErrorClass = false;

    for (const hClause of hClauses) {
      for (const type of hClause.types) {
        if (!ts.isIdentifier(type.expression)) {
          continue;
        }
        if (type.expression.text === 'Error') {
          includesErrorClass = true;
        }
      }
    }

    return includesErrorClass;
  }

  static isPrimitiveLiteralType(type: ts.Type): boolean {
    return !!(
      type.flags &
      (ts.TypeFlags.BooleanLiteral |
        ts.TypeFlags.NumberLiteral |
        ts.TypeFlags.StringLiteral |
        ts.TypeFlags.BigIntLiteral)
    );
  }

  static isPurePrimitiveLiteralType(type: ts.Type): boolean {
    return TsUtils.isPrimitiveLiteralType(type) && !(type.flags & ts.TypeFlags.EnumLiteral);
  }

  static isTypeSymbol(symbol: ts.Symbol | undefined): boolean {
    return (
      !!symbol &&
      !!symbol.flags &&
      ((symbol.flags & ts.SymbolFlags.Class) !== 0 || (symbol.flags & ts.SymbolFlags.Interface) !== 0)
    );
  }

  // Check whether type is generic 'Array<T>' type defined in TypeScript standard library.
  isGenericArrayType(tsType: ts.Type): tsType is ts.TypeReference {
    return (
      !(this.options.arkts2 && !isStdLibraryType(tsType)) &&
      TsUtils.isTypeReference(tsType) &&
      tsType.typeArguments?.length === 1 &&
      tsType.target.typeParameters?.length === 1 &&
      tsType.getSymbol()?.getName() === 'Array'
    );
  }

  isReadonlyArrayType(tsType: ts.Type): boolean {
    return (
      !(this.options.arkts2 && !isStdLibraryType(tsType)) &&
      TsUtils.isTypeReference(tsType) &&
      tsType.typeArguments?.length === 1 &&
      tsType.target.typeParameters?.length === 1 &&
      tsType.getSymbol()?.getName() === 'ReadonlyArray'
    );
  }

  static isConcatArrayType(tsType: ts.Type): boolean {
    return (
      isStdLibraryType(tsType) &&
      TsUtils.isTypeReference(tsType) &&
      tsType.typeArguments?.length === 1 &&
      tsType.target.typeParameters?.length === 1 &&
      tsType.getSymbol()?.getName() === CONCAT_ARRAY
    );
  }

  static isArrayLikeType(tsType: ts.Type): boolean {
    return (
      isStdLibraryType(tsType) &&
      TsUtils.isTypeReference(tsType) &&
      tsType.typeArguments?.length === 1 &&
      tsType.target.typeParameters?.length === 1 &&
      tsType.getSymbol()?.getName() === 'ArrayLike'
    );
  }

  isTypedArray(tsType: ts.Type, allowTypeArrays: string[]): boolean {
    const symbol = tsType.symbol;
    if (!symbol) {
      return false;
    }
    const name = this.tsTypeChecker.getFullyQualifiedName(symbol);
    if (this.isGlobalSymbol(symbol) && allowTypeArrays.includes(name)) {
      return true;
    }
    const decl = TsUtils.getDeclaration(symbol);
    return (
      !!decl &&
      TsUtils.isArkTSCollectionsClassOrInterfaceDeclaration(decl) &&
      allowTypeArrays.includes(symbol.getName())
    );
  }

  isArray(tsType: ts.Type): boolean {
    return (
      this.isGenericArrayType(tsType) || this.isReadonlyArrayType(tsType) || this.isTypedArray(tsType, TYPED_ARRAYS)
    );
  }

  isCollectionArrayType(tsType: ts.Type): boolean {
    return this.isTypedArray(tsType, TYPED_COLLECTIONS);
  }

  isIndexableArray(tsType: ts.Type): boolean {
    return (
      this.isGenericArrayType(tsType) ||
      this.isReadonlyArrayType(tsType) ||
      TsUtils.isConcatArrayType(tsType) ||
      TsUtils.isArrayLikeType(tsType) ||
      this.isTypedArray(tsType, TYPED_ARRAYS) ||
      this.isTypedArray(tsType, TYPED_COLLECTIONS)
    );
  }

  static isTuple(tsType: ts.Type): boolean {
    return TsUtils.isTypeReference(tsType) && !!(tsType.objectFlags & ts.ObjectFlags.Tuple);
  }

  // does something similar to relatedByInheritanceOrIdentical function
  isOrDerivedFrom(tsType: ts.Type, checkType: CheckType, checkedBaseTypes?: Set<ts.Type>): boolean {
    // eslint-disable-next-line no-param-reassign
    tsType = TsUtils.reduceReference(tsType);

    if (checkType.call(this, tsType)) {
      return true;
    }

    if (!tsType.symbol?.declarations) {
      return false;
    }

    // Avoid type recursion in heritage by caching checked types.
    (checkedBaseTypes = checkedBaseTypes || new Set<ts.Type>()).add(tsType);

    for (const tsTypeDecl of tsType.symbol.declarations) {
      const isClassOrInterfaceDecl = ts.isClassDeclaration(tsTypeDecl) || ts.isInterfaceDeclaration(tsTypeDecl);
      const isDerived = isClassOrInterfaceDecl && !!tsTypeDecl.heritageClauses;
      if (!isDerived) {
        continue;
      }
      for (const heritageClause of tsTypeDecl.heritageClauses) {
        if (this.processParentTypesCheck(heritageClause.types, checkType, checkedBaseTypes)) {
          return true;
        }
      }
    }

    return false;
  }

  static isTypeReference(tsType: ts.Type): tsType is ts.TypeReference {
    return (
      (tsType.getFlags() & ts.TypeFlags.Object) !== 0 &&
      ((tsType as ts.ObjectType).objectFlags & ts.ObjectFlags.Reference) !== 0
    );
  }

  static isPrototypeSymbol(symbol: ts.Symbol | undefined): boolean {
    return !!symbol && !!symbol.flags && (symbol.flags & ts.SymbolFlags.Prototype) !== 0;
  }

  static isFunctionSymbol(symbol: ts.Symbol | undefined): boolean {
    return !!symbol && !!symbol.flags && (symbol.flags & ts.SymbolFlags.Function) !== 0;
  }

  static isInterfaceType(tsType: ts.Type | undefined): boolean {
    return (
      !!tsType && !!tsType.symbol && !!tsType.symbol.flags && (tsType.symbol.flags & ts.SymbolFlags.Interface) !== 0
    );
  }

  static isAnyType(tsType: ts.Type): tsType is ts.TypeReference {
    return (tsType.getFlags() & ts.TypeFlags.Any) !== 0;
  }

  static isUnknownType(tsType: ts.Type): boolean {
    return (tsType.getFlags() & ts.TypeFlags.Unknown) !== 0;
  }

  static isUnsupportedType(tsType: ts.Type): boolean {
    return (
      !!tsType.flags &&
      ((tsType.flags & ts.TypeFlags.Any) !== 0 ||
        (tsType.flags & ts.TypeFlags.Unknown) !== 0 ||
        (tsType.flags & ts.TypeFlags.Intersection) !== 0)
    );
  }

  isUnsupportedTypeArkts2(tsType: ts.Type): boolean {
    const typenode = this.tsTypeChecker.typeToTypeNode(tsType, undefined, ts.NodeBuilderFlags.None);
    return !!typenode && !this.isSupportedType(typenode);
  }

  static isNullableUnionType(type: ts.Type): boolean {
    if (!type.isUnion()) {
      return false;
    }

    for (const t of type.types) {
      if (TsUtils.isNullishType(t)) {
        return true;
      }
    }

    return false;
  }

  /**
   * Returns true if the given type is `null` or `undefined`.
   */
  static isNullishType(t: ts.Type): boolean {
    return !!(t.flags & ts.TypeFlags.Undefined) || !!(t.flags & ts.TypeFlags.Null);
  }

  static isMethodAssignment(tsSymbol: ts.Symbol | undefined): boolean {
    return (
      !!tsSymbol && (tsSymbol.flags & ts.SymbolFlags.Method) !== 0 && (tsSymbol.flags & ts.SymbolFlags.Assignment) !== 0
    );
  }

  static getDeclaration(tsSymbol: ts.Symbol | undefined): ts.Declaration | undefined {
    if (tsSymbol?.declarations && tsSymbol.declarations.length > 0) {
      return tsSymbol.declarations[0];
    }
    return undefined;
  }

  private static isVarDeclaration(tsDecl: ts.Node): boolean {
    return ts.isVariableDeclaration(tsDecl) && ts.isVariableDeclarationList(tsDecl.parent);
  }

  isValidEnumMemberInit(tsExpr: ts.Expression): boolean {
    if (this.isNumberConstantValue(tsExpr.parent as ts.EnumMember)) {
      return true;
    }
    if (this.isStringConstantValue(tsExpr.parent as ts.EnumMember)) {
      return true;
    }
    return this.isCompileTimeExpression(tsExpr);
  }

  private isCompileTimeExpressionHandlePropertyAccess(tsExpr: ts.Expression): boolean {
    if (!ts.isPropertyAccessExpression(tsExpr)) {
      return false;
    }

    /*
     * if enum member is in current enum declaration try to get value
     * if it comes from another enum consider as constant
     */
    const propertyAccess = tsExpr;
    if (this.isNumberConstantValue(propertyAccess)) {
      return true;
    }
    const leftHandSymbol = this.trueSymbolAtLocation(propertyAccess.expression);
    if (!leftHandSymbol) {
      return false;
    }
    const decls = leftHandSymbol.getDeclarations();
    if (!decls || decls.length !== 1) {
      return false;
    }
    return ts.isEnumDeclaration(decls[0]);
  }

  isCompileTimeExpression(tsExpr: ts.Expression): boolean {
    if (
      ts.isParenthesizedExpression(tsExpr) ||
      ts.isAsExpression(tsExpr) && tsExpr.type.kind === ts.SyntaxKind.NumberKeyword
    ) {
      return this.isCompileTimeExpression(tsExpr.expression);
    }

    switch (tsExpr.kind) {
      case ts.SyntaxKind.PrefixUnaryExpression:
        return this.isPrefixUnaryExprValidEnumMemberInit(tsExpr as ts.PrefixUnaryExpression);
      case ts.SyntaxKind.ParenthesizedExpression:
      case ts.SyntaxKind.BinaryExpression:
        return this.isBinaryExprValidEnumMemberInit(tsExpr as ts.BinaryExpression);
      case ts.SyntaxKind.ConditionalExpression:
        return this.isConditionalExprValidEnumMemberInit(tsExpr as ts.ConditionalExpression);
      case ts.SyntaxKind.Identifier:
        return this.isIdentifierValidEnumMemberInit(tsExpr as ts.Identifier);
      case ts.SyntaxKind.NumericLiteral:
        return true;
      case ts.SyntaxKind.StringLiteral:
        return true;
      case ts.SyntaxKind.PropertyAccessExpression:
        return this.isCompileTimeExpressionHandlePropertyAccess(tsExpr);
      default:
        return false;
    }
  }

  private isPrefixUnaryExprValidEnumMemberInit(tsExpr: ts.PrefixUnaryExpression): boolean {
    return TsUtils.isUnaryOpAllowedForEnumMemberInit(tsExpr.operator) && this.isCompileTimeExpression(tsExpr.operand);
  }

  private isBinaryExprValidEnumMemberInit(tsExpr: ts.BinaryExpression): boolean {
    return (
      TsUtils.isBinaryOpAllowedForEnumMemberInit(tsExpr.operatorToken) &&
      this.isCompileTimeExpression(tsExpr.left) &&
      this.isCompileTimeExpression(tsExpr.right)
    );
  }

  private isConditionalExprValidEnumMemberInit(tsExpr: ts.ConditionalExpression): boolean {
    return this.isCompileTimeExpression(tsExpr.whenTrue) && this.isCompileTimeExpression(tsExpr.whenFalse);
  }

  private isIdentifierValidEnumMemberInit(tsExpr: ts.Identifier): boolean {
    const tsSymbol = this.trueSymbolAtLocation(tsExpr);
    const tsDecl = TsUtils.getDeclaration(tsSymbol);
    return (
      !!tsDecl &&
      (TsUtils.isVarDeclaration(tsDecl) && TsUtils.isConst(tsDecl.parent) || tsDecl.kind === ts.SyntaxKind.EnumMember)
    );
  }

  private static isUnaryOpAllowedForEnumMemberInit(tsPrefixUnaryOp: ts.PrefixUnaryOperator): boolean {
    return (
      tsPrefixUnaryOp === ts.SyntaxKind.PlusToken ||
      tsPrefixUnaryOp === ts.SyntaxKind.MinusToken ||
      tsPrefixUnaryOp === ts.SyntaxKind.TildeToken
    );
  }

  private static isBinaryOpAllowedForEnumMemberInit(tsBinaryOp: ts.BinaryOperatorToken): boolean {
    return (
      tsBinaryOp.kind === ts.SyntaxKind.AsteriskToken ||
      tsBinaryOp.kind === ts.SyntaxKind.SlashToken ||
      tsBinaryOp.kind === ts.SyntaxKind.PercentToken ||
      tsBinaryOp.kind === ts.SyntaxKind.MinusToken ||
      tsBinaryOp.kind === ts.SyntaxKind.PlusToken ||
      tsBinaryOp.kind === ts.SyntaxKind.LessThanLessThanToken ||
      tsBinaryOp.kind === ts.SyntaxKind.GreaterThanGreaterThanToken ||
      tsBinaryOp.kind === ts.SyntaxKind.BarBarToken ||
      tsBinaryOp.kind === ts.SyntaxKind.GreaterThanGreaterThanGreaterThanToken ||
      tsBinaryOp.kind === ts.SyntaxKind.AmpersandToken ||
      tsBinaryOp.kind === ts.SyntaxKind.CaretToken ||
      tsBinaryOp.kind === ts.SyntaxKind.BarToken ||
      tsBinaryOp.kind === ts.SyntaxKind.AmpersandAmpersandToken
    );
  }

  static isConst(tsNode: ts.Node): boolean {
    return !!(ts.getCombinedNodeFlags(tsNode) & ts.NodeFlags.Const);
  }

  isNumberConstantValue(
    tsExpr: ts.EnumMember | ts.PropertyAccessExpression | ts.ElementAccessExpression | ts.NumericLiteral
  ): boolean {
    const tsConstValue =
      tsExpr.kind === ts.SyntaxKind.NumericLiteral ?
        Number(tsExpr.getText()) :
        this.tsTypeChecker.getConstantValue(tsExpr);

    return tsConstValue !== undefined && typeof tsConstValue === 'number';
  }

  isIntegerConstantValue(
    tsExpr: ts.EnumMember | ts.PropertyAccessExpression | ts.ElementAccessExpression | ts.NumericLiteral
  ): boolean {
    const tsConstValue =
      tsExpr.kind === ts.SyntaxKind.NumericLiteral ?
        Number(tsExpr.getText()) :
        this.tsTypeChecker.getConstantValue(tsExpr);
    return (
      tsConstValue !== undefined &&
      typeof tsConstValue === 'number' &&
      tsConstValue.toFixed(0) === tsConstValue.toString()
    );
  }

  isStringConstantValue(tsExpr: ts.EnumMember | ts.PropertyAccessExpression | ts.ElementAccessExpression): boolean {
    const tsConstValue = this.tsTypeChecker.getConstantValue(tsExpr);
    return tsConstValue !== undefined && typeof tsConstValue === 'string';
  }

  // Returns true if typeA is a subtype of typeB
  relatedByInheritanceOrIdentical(typeA: ts.Type, typeB: ts.Type): boolean {
    // eslint-disable-next-line no-param-reassign
    typeA = TsUtils.reduceReference(typeA);
    // eslint-disable-next-line no-param-reassign
    typeB = TsUtils.reduceReference(typeB);

    if (typeA === typeB || this.isObject(typeB)) {
      return true;
    }
    if (!typeA.symbol?.declarations) {
      return false;
    }
    const isBISendable = TsUtils.isISendableInterface(typeB);
    for (const typeADecl of typeA.symbol.declarations) {
      if (this.relatedByInheritanceOrIdenticalCheckParentTypes(typeA, typeB, typeADecl, isBISendable)) {
        return true;
      }
    }
    return false;
  }

  private relatedByInheritanceOrIdenticalCheckParentTypes(
    typeA: ts.Type,
    typeB: ts.Type,
    typeADecl: ts.Declaration,
    isBISendable = false
  ): boolean {
    if (isBISendable && ts.isClassDeclaration(typeADecl) && TsUtils.hasSendableDecorator(typeADecl)) {
      return true;
    }
    if (!ts.isClassDeclaration(typeADecl) && !ts.isInterfaceDeclaration(typeADecl)) {
      return false;
    }
    if (this.processExtendedParentTypes(typeA, typeB)) {
      return true;
    }
    if (this.isStdIterableType(typeB) && this.hasSymbolIteratorMethod(typeA)) {
      return true;
    }
    if (!typeADecl.heritageClauses) {
      return false;
    }
    for (const heritageClause of typeADecl.heritageClauses) {
      const processInterfaces = typeA.isClass() ? heritageClause.token !== ts.SyntaxKind.ExtendsKeyword : true;
      if (this.processParentTypes(heritageClause.types, typeB, processInterfaces)) {
        return true;
      }
    }
    return false;
  }

  hasSymbolIteratorMethod(type: ts.Type): boolean {
    const rhsTypeProps = this.tsTypeChecker.getPropertiesOfType(type);
    return rhsTypeProps.some((prop) => {
      const propDecl = TsUtils.getDeclaration(prop);
      return (
        propDecl &&
        (ts.isMethodSignature(propDecl) || ts.isMethodDeclaration(propDecl)) &&
        ts.isComputedPropertyName(propDecl.name) &&
        this.isSymbolIteratorExpression(propDecl.name.expression)
      );
    });
  }

  isStdIterableType(type: ts.Type): boolean {
    void this;
    const sym = type.getSymbol();
    return !!sym && sym.getName() === 'Iterable' && isStdLibrarySymbol(sym);
  }

  static reduceReference(t: ts.Type): ts.Type {
    return TsUtils.isTypeReference(t) && t.target !== t ? t.target : t;
  }

  private needToDeduceStructuralIdentityHandleUnions(
    lhsType: ts.Type,
    rhsType: ts.Type,
    rhsExpr: ts.Expression,
    isStrict: boolean
  ): boolean {
    if (this.needToDeduceStructuralIdentityHandleUnionsIsStrict(lhsType, rhsType, rhsExpr, isStrict)) {
      return true;
    }
    if (rhsType.isUnion()) {
      // Each Class/Interface of the RHS union type must be compatible with LHS type.
      for (const compType of rhsType.types) {
        if (this.needToDeduceStructuralIdentity(lhsType, compType, rhsExpr, isStrict)) {
          return true;
        }
      }
      return false;
    }
    if (lhsType.isUnion() && TsUtils.isTypeReference(rhsType)) {
      let needDeduce = false;
      // RHS type needs to be compatible with at least one type of the LHS union.
      for (const compType of lhsType.types) {
        if (!TsUtils.isTypeReference(compType) && !TsUtils.isISendableInterface(compType)) {
          continue;
        }
        if (this.needToDeduceStructuralIdentity(compType, rhsType, rhsExpr, isStrict)) {
          needDeduce = true;
        } else {
          return false;
        }
      }
      return needDeduce;
    }
    // should be unreachable
    return false;
  }

  needToDeduceStructuralIdentityHandleUnionsIsStrict(
    lhsType: ts.Type,
    rhsType: ts.Type,
    rhsExpr: ts.Expression,
    isStrict: boolean
  ): boolean {
    if (rhsType.isUnion() && lhsType.isUnion()) {
      return rhsType.types.some((compRhsType) => {
        return lhsType.types.every((compLhsType) => {
          return this.needToDeduceStructuralIdentity(compLhsType, compRhsType, rhsExpr, isStrict);
        });
      });
    }
    return false;
  }

  // return true if two class types are not related by inheritance and structural identity check is needed
  needToDeduceStructuralIdentity(
    lhsType: ts.Type,
    rhsType: ts.Type,
    rhsExpr: ts.Expression,
    isStrict: boolean = false
  ): boolean {
    // eslint-disable-next-line no-param-reassign
    lhsType = this.getNonNullableType(lhsType);
    // eslint-disable-next-line no-param-reassign
    rhsType = this.getNonNullableType(rhsType);
    if (this.isLibraryType(lhsType)) {
      return false;
    }
    if (this.isDynamicObjectAssignedToStdType(lhsType, rhsExpr)) {
      return false;
    }
    // #14569: Check for Function type.
    if (this.skipStructuralTypingCheckForFunctionals(lhsType, rhsType)) {
      return false;
    }
    if (this.skipCheckForArrayBufferLike(lhsType, rhsType)) {
      return false;
    }
    if (rhsType.isUnion() || lhsType.isUnion()) {
      return this.needToDeduceStructuralIdentityHandleUnions(lhsType, rhsType, rhsExpr, isStrict);
    }
    if (this.needToDeduceStructuralIdentityAdvancedClassChecks(lhsType, rhsType)) {
      // missing exact rule
      return true;
    }
    // if isStrict, things like generics need to be checked
    if (isStrict) {
      if (TsUtils.isTypeReference(rhsType) && !!(rhsType.objectFlags & ts.ObjectFlags.ArrayLiteral)) {
        // The 'arkts-sendable-obj-init' rule already exists. Wait for the new 'strict type' to be modified.
        return false;
      }
      if (this.needToDeduceStructuralIdentityIsStrict(lhsType, rhsType, rhsExpr, isStrict)) {
        return true;
      }
      // eslint-disable-next-line no-param-reassign
      lhsType = TsUtils.reduceReference(lhsType);
      // eslint-disable-next-line no-param-reassign
      rhsType = TsUtils.reduceReference(rhsType);
    }
    return (
      lhsType.isClassOrInterface() &&
      rhsType.isClassOrInterface() &&
      !this.relatedByInheritanceOrIdentical(rhsType, lhsType)
    );
  }

  needToDeduceStructuralIdentityIsStrict(
    lhsType: ts.Type,
    rhsType: ts.Type,
    rhsExpr: ts.Expression,
    isStrict: boolean = false
  ): boolean {
    if (
      TsUtils.reduceReference(lhsType) !== TsUtils.reduceReference(rhsType) ||
      !TsUtils.isTypeReference(lhsType) ||
      !TsUtils.isTypeReference(rhsType)
    ) {
      return false;
    }
    const lhsArgs = lhsType.typeArguments;
    const rhsArgs = rhsType.typeArguments;
    if (lhsArgs && lhsArgs.length > 0) {
      if (rhsArgs && rhsArgs.length > 0) {
        if (rhsArgs[0] === lhsArgs[0]) {
          return false;
        }
        return this.needToDeduceStructuralIdentity(lhsArgs[0], rhsArgs[0], rhsExpr, isStrict);
      }
      return this.needToDeduceStructuralIdentity(lhsArgs[0], rhsType, rhsExpr, isStrict);
    }
    return false;
  }

  private needToDeduceStructuralIdentityAdvancedClassChecks(lhsType: ts.Type, rhsType: ts.Type): boolean {
    return (
      !!this.options.advancedClassChecks &&
      TsUtils.isClassValueType(rhsType) &&
      lhsType !== rhsType &&
      !TsUtils.isObjectType(lhsType)
    );
  }

  // Does the 'arkts-no-structural-typing' rule need to be strictly enforced to complete previously missed scenarios
  needStrictMatchType(lhsType: ts.Type, rhsType: ts.Type): boolean {
    if (this.options.arkts2) {
      return true;
    }
    if (this.isStrictSendableMatch(lhsType, rhsType)) {
      return true;
    }
    // add other requirements with strict type requirements here
    return false;
  }

  // For compatibility, left must all ClassOrInterface is sendable, right must has non-sendable ClassorInterface
  private isStrictSendableMatch(lhsType: ts.Type, rhsType: ts.Type): boolean {
    let isStrictLhs = false;
    if (lhsType.isUnion()) {
      for (let compType of lhsType.types) {
        compType = TsUtils.reduceReference(compType);
        if (!compType.isClassOrInterface()) {
          continue;
        }
        if (!this.isSendableClassOrInterface(compType)) {
          return false;
        }
        isStrictLhs = true;
      }
    } else {
      isStrictLhs = this.isSendableClassOrInterface(lhsType);
    }
    return isStrictLhs && this.typeContainsNonSendableClassOrInterface(rhsType);
  }

  skipCheckForArrayBufferLike(lhsType: string | ts.Type, rhsType: string | ts.Type): boolean {
    const lhsIsArrayBufferLike = TsUtils.isArrayBufferType(
      typeof lhsType === 'string' ? lhsType : this.tsTypeChecker.typeToString(lhsType)
    );
    const rhsIsArrayBufferLike = TsUtils.isArrayBufferType(
      typeof rhsType === 'string' ? rhsType : this.tsTypeChecker.typeToString(rhsType)
    );
    return !!this.options.arkts2 && lhsIsArrayBufferLike && rhsIsArrayBufferLike;
  }

  private processExtendedParentTypes(typeA: ts.Type, typeB: ts.Type): boolean {

    /*
     * Most standard types in TS Stdlib do not use explicit inheritance and rely on
     * structural compatibility. In contrast, the type definitions in ArkTS stdlib
     * use inheritance with explicit base types. We check the inheritance hierarchy
     * for such types according to how they are defined in ArkTS Stdlib.
     */

    if (!this.options.arkts2) {
      return false;
    }
    if (!isStdLibrarySymbol(typeA.symbol) && !isStdLibrarySymbol(typeB.symbol)) {
      return false;
    }
    return this.checkExtendedParentTypes(typeA.symbol.name, typeB.symbol.name);
  }

  private checkExtendedParentTypes(typeA: string, typeB: string): boolean {
    if (typeA === typeB) {
      return true;
    }
    const extBaseTypes = EXTENDED_BASE_TYPES.get(typeA);
    if (!extBaseTypes) {
      return false;
    }
    for (const extBaseType of extBaseTypes) {
      if (this.checkExtendedParentTypes(extBaseType, typeB)) {
        return true;
      }
    }
    return false;
  }

  private processParentTypes(
    parentTypes: ts.NodeArray<ts.Expression>,
    typeB: ts.Type,
    processInterfaces: boolean
  ): boolean {
    for (const baseTypeExpr of parentTypes) {
      const baseType = TsUtils.reduceReference(this.tsTypeChecker.getTypeAtLocation(baseTypeExpr));
      if (
        baseType &&
        baseType.isClass() !== processInterfaces &&
        this.relatedByInheritanceOrIdentical(baseType, typeB)
      ) {
        return true;
      }
    }
    return false;
  }

  private processParentTypesCheck(
    parentTypes: ts.NodeArray<ts.Expression>,
    checkType: CheckType,
    checkedBaseTypes: Set<ts.Type>
  ): boolean {
    for (const baseTypeExpr of parentTypes) {
      const baseType = TsUtils.reduceReference(this.tsTypeChecker.getTypeAtLocation(baseTypeExpr));
      if (baseType && !checkedBaseTypes.has(baseType) && this.isOrDerivedFrom(baseType, checkType, checkedBaseTypes)) {
        return true;
      }
    }
    return false;
  }

  isObject(tsType: ts.Type): boolean {
    if (!tsType) {
      return false;
    }
    if (tsType.symbol && tsType.isClassOrInterface() && tsType.symbol.name === 'Object') {
      return true;
    }
    const node = this.tsTypeChecker.typeToTypeNode(tsType, undefined, undefined);
    return node !== undefined && node.kind === ts.SyntaxKind.ObjectKeyword;
  }

  isCallToFunctionWithOmittedReturnType(tsExpr: ts.Expression): boolean {
    if (ts.isCallExpression(tsExpr)) {
      const tsCallSignature = this.tsTypeChecker.getResolvedSignature(tsExpr);
      if (tsCallSignature) {
        const tsSignDecl = tsCallSignature.getDeclaration();
        // `tsSignDecl` is undefined when `getResolvedSignature` returns `unknownSignature`
        if (!tsSignDecl?.type) {
          return true;
        }
      }
    }

    return false;
  }

  private static hasReadonlyFields(type: ts.Type): boolean {
    // No members -> no readonly fields
    if (type.symbol.members === undefined) {
      return false;
    }

    let result: boolean = false;

    type.symbol.members.forEach((value) => {
      if (
        value.declarations !== undefined &&
        value.declarations.length > 0 &&
        ts.isPropertyDeclaration(value.declarations[0])
      ) {
        const propmMods = ts.getModifiers(value.declarations[0]);
        if (TsUtils.hasModifier(propmMods, ts.SyntaxKind.ReadonlyKeyword)) {
          result = true;
        }
      }
    });

    return result;
  }

  private hasDefaultCtor(type: ts.Type): boolean {
    const checkBaseTypes = (type: ts.Type): boolean => {
      if (!this.options.arkts2) {
        return true;
      }
      const baseTypes = type.getBaseTypes()?.filter((baseType) => {
        return baseType.isClass();
      });
      if (!baseTypes || baseTypes.length === 0) {
        return true;
      }
      return baseTypes.some((baseType: ts.Type) => {
        return this.hasDefaultCtor(baseType);
      });
    };

    // No members -> no explicit constructors -> there is default ctor
    if (type.symbol.members === undefined) {
      return checkBaseTypes(type);
    }

    // has any constructor
    let hasCtor: boolean = false;
    // has default constructor
    let hasDefaultCtor: boolean = false;

    type.symbol.members.forEach((value) => {
      if ((value.flags & ts.SymbolFlags.Constructor) === 0) {
        return;
      }
      hasCtor = true;

      if (value.declarations === undefined || value.declarations.length <= 0) {
        return;
      }

      const declCtor = value.declarations[0] as ts.ConstructorDeclaration;
      if (declCtor.parameters.length === 0) {
        hasDefaultCtor = true;
      }
    });

    // Has no any explicit constructor -> has implicit default constructor.
    if (!hasCtor) {
      return checkBaseTypes(type);
    }

    return hasDefaultCtor;
  }

  private static isAbstractClass(type: ts.Type): boolean {
    if (type.isClass() && type.symbol.declarations && type.symbol.declarations.length > 0) {
      const declClass = type.symbol.declarations[0] as ts.ClassDeclaration;
      const classMods = ts.getModifiers(declClass);
      if (TsUtils.hasModifier(classMods, ts.SyntaxKind.AbstractKeyword)) {
        return true;
      }
    }

    return false;
  }

  isAbstractMethodInAbstractClass(node: ts.Node): boolean {
    const type = this.tsTypeChecker.getTypeAtLocation(node);
    const funcDeclParentType = this.tsTypeChecker.getTypeAtLocation(node.parent);
    if (
      TsUtils.isAbstractClass(funcDeclParentType) &&
      type.symbol?.declarations &&
      type.symbol.declarations.length > 0
    ) {
      const declClass = type.symbol.declarations[0] as ts.MethodDeclaration;
      const classMods = ts.getModifiers(declClass);
      if (TsUtils.hasModifier(classMods, ts.SyntaxKind.AbstractKeyword)) {
        return true;
      }
    }
    return false;
  }

  validateObjectLiteralType(type: ts.Type | undefined): boolean {
    if (!type) {
      return false;
    }
    // eslint-disable-next-line no-param-reassign
    type = TsUtils.reduceReference(type);
    return (
      type.isClassOrInterface() &&
      this.hasDefaultCtor(type) &&
      !TsUtils.hasReadonlyFields(type) &&
      !TsUtils.isAbstractClass(type)
    );
  }

  hasMethods(type: ts.Type): boolean {
    const properties = this.tsTypeChecker.getPropertiesOfType(type);
    if (properties?.length) {
      for (const prop of properties) {
        if (prop.getFlags() & ts.SymbolFlags.Method) {
          return true;
        }
      }
    }
    return false;
  }

  findProperty(type: ts.Type, name: string): ts.Symbol | undefined {
    const properties = this.tsTypeChecker.getPropertiesOfType(type);
    if (properties?.length) {
      for (const prop of properties) {
        if (prop.name === name) {
          return prop;
        }
      }
    }

    return undefined;
  }

  checkTypeSet(typeSet: ts.Type, predicate: CheckType): boolean {
    if (!typeSet.isUnionOrIntersection()) {
      return predicate.call(this, typeSet);
    }
    for (const elemType of typeSet.types) {
      if (this.checkTypeSet(elemType, predicate)) {
        return true;
      }
    }
    return false;
  }

  getNonNullableType(t: ts.Type): ts.Type {
    const isNullableUnionType = this.options.useRtLogic ? TsUtils.isNullableUnionType(t) : t.isUnion();
    if (isNullableUnionType) {
      return t.getNonNullableType();
    }
    return t;
  }

  private isObjectLiteralAssignableToUnion(lhsType: ts.UnionType, rhsExpr: ts.ObjectLiteralExpression): boolean {
    for (const compType of lhsType.types) {
      if (this.isObjectLiteralAssignable(compType, rhsExpr)) {
        return true;
      }
    }
    return false;
  }

  isObjectLiteralAssignable(lhsType: ts.Type | undefined, rhsExpr: ts.ObjectLiteralExpression): boolean {
    if (lhsType === undefined) {
      return false;
    }
    // Always check with the non-nullable variant of lhs type.
    // eslint-disable-next-line no-param-reassign
    lhsType = this.getNonNullableType(lhsType);
    if (lhsType.isUnion() && this.isObjectLiteralAssignableToUnion(lhsType, rhsExpr)) {
      return true;
    }

    /*
     * Allow initializing with anything when the type
     * originates from the library.
     */
    if (TsUtils.isAnyType(lhsType) || this.isLibraryType(lhsType)) {
      return true;
    }

    /*
     * issue 13412:
     * Allow initializing with a dynamic object when the LHS type
     * is primitive or defined in standard library.
     */
    if (this.isDynamicObjectAssignedToStdType(lhsType, rhsExpr)) {
      return true;
    }
    // For Partial<T>, Required<T>, Readonly<T> types, validate their argument type.
    if (this.isStdPartialType(lhsType) || this.isStdRequiredType(lhsType) || this.isStdReadonlyType(lhsType)) {
      if (lhsType.aliasTypeArguments && lhsType.aliasTypeArguments.length === 1) {
        // eslint-disable-next-line no-param-reassign
        lhsType = lhsType.aliasTypeArguments[0];
      } else {
        return false;
      }
    }

    /*
     * Allow initializing Record objects with object initializer.
     * Record supports any type for a its value, but the key value
     * must be either a string or number literal.
     */
    if (this.isStdRecordType(lhsType)) {
      return this.validateRecordObjectKeys(rhsExpr);
    }
    return (
      this.validateObjectLiteralType(lhsType) && !this.hasMethods(lhsType) && this.validateFields(lhsType, rhsExpr)
    );
  }

  private isDynamicObjectAssignedToStdType(lhsType: ts.Type, rhsExpr: ts.Expression): boolean {
    if (isStdLibraryType(lhsType) || TsUtils.isPrimitiveType(lhsType)) {
      // eslint-disable-next-line no-nested-ternary
      const rhsSym = ts.isCallExpression(rhsExpr) ?
        this.getSymbolOfCallExpression(rhsExpr) :
        this.options.useRtLogic ?
          this.trueSymbolAtLocation(rhsExpr) :
          this.tsTypeChecker.getSymbolAtLocation(rhsExpr);
      if (rhsSym && this.isLibrarySymbol(rhsSym)) {
        return true;
      }
    }
    return false;
  }

  validateFields(objectType: ts.Type, objectLiteral: ts.ObjectLiteralExpression): boolean {
    for (const prop of objectLiteral.properties) {
      if (ts.isPropertyAssignment(prop)) {
        if (!this.validateField(objectType, prop)) {
          return false;
        }
      }
    }

    return true;
  }

  getPropertySymbol(type: ts.Type, prop: ts.PropertyAssignment): ts.Symbol | undefined {
    const propNameSymbol = this.tsTypeChecker.getSymbolAtLocation(prop.name);
    // eslint-disable-next-line no-nested-ternary
    const propName = propNameSymbol ?
      ts.symbolName(propNameSymbol) :
      ts.isMemberName(prop.name) ?
        ts.idText(prop.name) :
        prop.name.getText();
    const propSym = this.findProperty(type, propName);
    return propSym;
  }

  private validateField(type: ts.Type, prop: ts.PropertyAssignment): boolean {
    // Issue 15497: Use unescaped property name to find correpsponding property.
    const propSym = this.getPropertySymbol(type, prop);
    if (!propSym?.declarations?.length) {
      return false;
    }

    const propType = this.tsTypeChecker.getTypeOfSymbolAtLocation(propSym, propSym.declarations[0]);
    const initExpr = TsUtils.unwrapParenthesized(prop.initializer);
    const rhsType = this.tsTypeChecker.getTypeAtLocation(initExpr);
    if (ts.isObjectLiteralExpression(initExpr)) {
      if (!this.isObjectLiteralAssignable(propType, initExpr)) {
        return false;
      }
    } else {
      // Only check for structural sub-typing.
      if (
        this.needToDeduceStructuralIdentity(propType, rhsType, initExpr, this.needStrictMatchType(propType, rhsType))
      ) {
        return false;
      }
      if (this.isWrongSendableFunctionAssignment(propType, rhsType)) {
        return false;
      }
    }

    return true;
  }

  validateRecordObjectKeys(objectLiteral: ts.ObjectLiteralExpression): boolean {
    for (const prop of objectLiteral.properties) {
      if (!prop.name || !this.isValidRecordObjectLiteralKey(prop.name)) {
        return false;
      }
    }
    return true;
  }

  isValidRecordObjectLiteralKey(propName: ts.PropertyName): boolean {
    if (ts.isComputedPropertyName(propName)) {
      return this.isValidComputedPropertyName(propName, true);
    }
    return ts.isStringLiteral(propName) || ts.isNumericLiteral(propName);
  }

  private static isSupportedTypeNodeKind(kind: ts.SyntaxKind): boolean {
    return (
      kind !== ts.SyntaxKind.AnyKeyword &&
      kind !== ts.SyntaxKind.UnknownKeyword &&
      kind !== ts.SyntaxKind.SymbolKeyword &&
      kind !== ts.SyntaxKind.IndexedAccessType &&
      kind !== ts.SyntaxKind.ConditionalType &&
      kind !== ts.SyntaxKind.MappedType &&
      kind !== ts.SyntaxKind.InferType
    );
  }

  private isSupportedTypeHandleUnionTypeNode(typeNode: ts.UnionTypeNode): boolean {
    for (const unionTypeElem of typeNode.types) {
      if (!this.isSupportedType(unionTypeElem)) {
        return false;
      }
    }
    return true;
  }

  private isSupportedTypeHandleTupleTypeNode(typeNode: ts.TupleTypeNode): boolean {
    for (const elem of typeNode.elements) {
      if (ts.isTypeNode(elem) && !this.isSupportedType(elem)) {
        return false;
      }
      if (ts.isNamedTupleMember(elem) && !this.isSupportedType(elem.type)) {
        return false;
      }
    }
    return true;
  }

  isSupportedType(typeNode: ts.TypeNode): boolean {
    if (ts.isParenthesizedTypeNode(typeNode)) {
      return this.isSupportedType(typeNode.type);
    }

    if (ts.isArrayTypeNode(typeNode)) {
      return this.isSupportedType(typeNode.elementType);
    }

    if (ts.isTypeReferenceNode(typeNode) && typeNode.typeArguments) {
      for (const typeArg of typeNode.typeArguments) {
        if (!this.isSupportedType(typeArg)) {
          return false;
        }
      }
      return true;
    }

    if (ts.isUnionTypeNode(typeNode)) {
      return this.isSupportedTypeHandleUnionTypeNode(typeNode);
    }

    if (ts.isTupleTypeNode(typeNode)) {
      return this.isSupportedTypeHandleTupleTypeNode(typeNode);
    }

    return (
      !ts.isTypeLiteralNode(typeNode) &&
      (this.options.advancedClassChecks || !ts.isTypeQueryNode(typeNode)) &&
      !ts.isIntersectionTypeNode(typeNode) &&
      TsUtils.isSupportedTypeNodeKind(typeNode.kind)
    );
  }

  isStructObjectInitializer(objectLiteral: ts.ObjectLiteralExpression): boolean {
    if (ts.isCallLikeExpression(objectLiteral.parent)) {
      const signature = this.tsTypeChecker.getResolvedSignature(objectLiteral.parent);
      const signDecl = signature?.declaration;
      return !!signDecl && ts.isConstructorDeclaration(signDecl) && isStructDeclaration(signDecl.parent);
    }
    return false;
  }

  parentSymbolCache = new Map<ts.Symbol, string | undefined>();

  getParentSymbolName(symbol: ts.Symbol | undefined): string | undefined {
    if (!symbol) {
      return undefined;
    }
    const cached = this.parentSymbolCache.get(symbol);
    if (cached) {
      return cached;
    }

    const fullName = this.tsTypeChecker.getFullyQualifiedName(symbol);
    const match = fullName.match(/['"](.*)['"]\.(.*)/);
    const name = match ? match[2] : fullName;
    const dotPosition = name.lastIndexOf('.');
    const result = dotPosition === -1 ? undefined : name.substring(0, dotPosition);
    this.parentSymbolCache.set(symbol, result);
    return result;
  }

  isGlobalSymbol(symbol: ts.Symbol): boolean {
    const parentName = this.getParentSymbolName(symbol);
    return !parentName || parentName === 'global';
  }

  isStdSymbol(symbol: ts.Symbol): boolean {
    const name = this.tsTypeChecker.getFullyQualifiedName(symbol);
    return name === SYMBOL || name === SYMBOL_CONSTRUCTOR;
  }

  isStdSymbolAPI(symbol: ts.Symbol): boolean {
    const parentName = this.getParentSymbolName(symbol);
    if (!this.options.useRtLogic) {
      const name = parentName ? parentName : symbol.escapedName;
      return name === SYMBOL || name === SYMBOL_CONSTRUCTOR;
    }
    return !!parentName && (parentName === SYMBOL || parentName === SYMBOL_CONSTRUCTOR);
  }

  isSymbolIterator(symbol: ts.Symbol): boolean {
    if (!this.options.useRtLogic) {
      const name = symbol.name;
      const parName = this.getParentSymbolName(symbol);
      return (parName === SYMBOL || parName === SYMBOL_CONSTRUCTOR) && name === ITERATOR;
    }
    return this.isStdSymbolAPI(symbol) && symbol.name === ITERATOR;
  }

  isSymbolIteratorExpression(expr: ts.Expression): boolean {
    const symbol = this.trueSymbolAtLocation(expr);
    return !!symbol && this.isSymbolIterator(symbol);
  }

  static isDefaultImport(importSpec: ts.ImportSpecifier): boolean {
    return importSpec?.propertyName?.text === 'default';
  }

  static getStartPos(nodeOrComment: ts.Node | ts.CommentRange): number {
    return nodeOrComment.kind === ts.SyntaxKind.SingleLineCommentTrivia ||
      nodeOrComment.kind === ts.SyntaxKind.MultiLineCommentTrivia ?
      (nodeOrComment as ts.CommentRange).pos :
      (nodeOrComment as ts.Node).getStart();
  }

  static getEndPos(nodeOrComment: ts.Node | ts.CommentRange): number {
    return nodeOrComment.kind === ts.SyntaxKind.SingleLineCommentTrivia ||
      nodeOrComment.kind === ts.SyntaxKind.MultiLineCommentTrivia ?
      (nodeOrComment as ts.CommentRange).end :
      (nodeOrComment as ts.Node).getEnd();
  }

  static getHighlightRange(nodeOrComment: ts.Node | ts.CommentRange, faultId: number): [number, number] {
    return (
      this.highlightRangeHandlers.get(faultId)?.call(this, nodeOrComment) ?? [
        this.getStartPos(nodeOrComment),
        this.getEndPos(nodeOrComment)
      ]
    );
  }

  static highlightRangeHandlers = new Map([
    [FaultID.VarDeclaration, TsUtils.getVarDeclarationHighlightRange],
    [FaultID.CatchWithUnsupportedType, TsUtils.getCatchWithUnsupportedTypeHighlightRange],
    [FaultID.ForInStatement, TsUtils.getForInStatementHighlightRange],
    [FaultID.WithStatement, TsUtils.getWithStatementHighlightRange],
    [FaultID.DeleteOperator, TsUtils.getDeleteOperatorHighlightRange],
    [FaultID.TypeQuery, TsUtils.getTypeQueryHighlightRange],
    [FaultID.InstanceofUnsupported, TsUtils.getInstanceofUnsupportedHighlightRange],
    [FaultID.ConstAssertion, TsUtils.getConstAssertionHighlightRange],
    [FaultID.LimitedReturnTypeInference, TsUtils.getLimitedReturnTypeInferenceHighlightRange],
    [FaultID.LocalFunction, TsUtils.getLocalFunctionHighlightRange],
    [FaultID.FunctionBind, TsUtils.getFunctionApplyCallHighlightRange],
    [FaultID.FunctionBindError, TsUtils.getFunctionApplyCallHighlightRange],
    [FaultID.FunctionApplyCall, TsUtils.getFunctionApplyCallHighlightRange],
    [FaultID.DeclWithDuplicateName, TsUtils.getDeclWithDuplicateNameHighlightRange],
    [FaultID.ObjectLiteralNoContextType, TsUtils.getObjectLiteralNoContextTypeHighlightRange],
    [FaultID.ClassExpression, TsUtils.getClassExpressionHighlightRange],
    [FaultID.MultipleStaticBlocks, TsUtils.getMultipleStaticBlocksHighlightRange],
    [FaultID.ParameterProperties, TsUtils.getParameterPropertiesHighlightRange],
    [FaultID.SendableDefiniteAssignment, TsUtils.getSendableDefiniteAssignmentHighlightRange],
    [FaultID.ObjectTypeLiteral, TsUtils.getObjectTypeLiteralHighlightRange],
    [FaultID.StructuralIdentity, TsUtils.getStructuralIdentityHighlightRange],
    [FaultID.VoidOperator, TsUtils.getVoidOperatorHighlightRange],
    [FaultID.ImportLazyIdentifier, TsUtils.getImportLazyHighlightRange],
    [FaultID.IncompationbleFunctionType, TsUtils.getIncompationbleFunctionTypeHighlightRange]
  ]);

  static getKeywordHighlightRange(nodeOrComment: ts.Node | ts.CommentRange, keyword: string): [number, number] {
    const start = this.getStartPos(nodeOrComment);
    return [start, start + keyword.length];
  }

  static getVarDeclarationHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, 'var');
  }

  static getCatchWithUnsupportedTypeHighlightRange(
    nodeOrComment: ts.Node | ts.CommentRange
  ): [number, number] | undefined {
    const catchClauseNode = (nodeOrComment as ts.CatchClause).variableDeclaration;
    if (catchClauseNode !== undefined) {
      return [catchClauseNode.getStart(), catchClauseNode.getEnd()];
    }

    return undefined;
  }

  static getForInStatementHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return [
      this.getEndPos((nodeOrComment as ts.ForInStatement).initializer) + 1,
      this.getStartPos((nodeOrComment as ts.ForInStatement).expression) - 1
    ];
  }

  static getWithStatementHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return [this.getStartPos(nodeOrComment), (nodeOrComment as ts.WithStatement).statement.getStart() - 1];
  }

  static getDeleteOperatorHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, 'delete');
  }

  static getTypeQueryHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, 'typeof');
  }

  static getInstanceofUnsupportedHighlightRange(
    nodeOrComment: ts.Node | ts.CommentRange
  ): [number, number] | undefined {
    return this.getKeywordHighlightRange((nodeOrComment as ts.BinaryExpression).operatorToken, 'instanceof');
  }

  static getConstAssertionHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    if (nodeOrComment.kind === ts.SyntaxKind.AsExpression) {
      return [
        (nodeOrComment as ts.AsExpression).expression.getEnd() + 1,
        (nodeOrComment as ts.AsExpression).type.getStart() - 1
      ];
    }
    return [
      (nodeOrComment as ts.TypeAssertion).expression.getEnd() + 1,
      (nodeOrComment as ts.TypeAssertion).type.getEnd() + 1
    ];
  }

  static getLimitedReturnTypeInferenceHighlightRange(
    nodeOrComment: ts.Node | ts.CommentRange
  ): [number, number] | undefined {
    let node: ts.Node | undefined;
    if (nodeOrComment.kind === ts.SyntaxKind.FunctionExpression) {
      // we got error about return type so it should be present
      node = (nodeOrComment as ts.FunctionExpression).type;
    } else if (nodeOrComment.kind === ts.SyntaxKind.FunctionDeclaration) {
      node = (nodeOrComment as ts.FunctionDeclaration).name;
    } else if (nodeOrComment.kind === ts.SyntaxKind.MethodDeclaration) {
      node = (nodeOrComment as ts.MethodDeclaration).name;
    }

    if (node !== undefined) {
      return [node.getStart(), node.getEnd()];
    }

    return undefined;
  }

  static getLocalFunctionHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, 'function');
  }

  static getFunctionApplyCallHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    const pointPos = (nodeOrComment as ts.Node).getText().lastIndexOf('.');
    return [this.getStartPos(nodeOrComment) + pointPos + 1, this.getEndPos(nodeOrComment)];
  }

  static getDeclWithDuplicateNameHighlightRange(
    nodeOrComment: ts.Node | ts.CommentRange
  ): [number, number] | undefined {
    // in case of private identifier no range update is needed
    const nameNode: ts.Node | undefined = (nodeOrComment as ts.NamedDeclaration).name;
    if (nameNode !== undefined) {
      return [nameNode.getStart(), nameNode.getEnd()];
    }

    return undefined;
  }

  static getObjectLiteralNoContextTypeHighlightRange(
    nodeOrComment: ts.Node | ts.CommentRange
  ): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, '{');
  }

  static getClassExpressionHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, 'class');
  }

  static getMultipleStaticBlocksHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, 'static');
  }

  static getParameterPropertiesHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    const param = nodeOrComment as ts.ParameterDeclaration;
    const modifier = TsUtils.getAccessModifier(ts.getModifiers(param));
    if (modifier !== undefined) {
      return [modifier.getStart(), modifier.getEnd()];
    }
    return undefined;
  }

  static getObjectTypeLiteralHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, '{');
  }

  // highlight ranges for Sendable rules

  static getSendableDefiniteAssignmentHighlightRange(
    nodeOrComment: ts.Node | ts.CommentRange
  ): [number, number] | undefined {
    const name = (nodeOrComment as ts.PropertyDeclaration).name;
    const exclamationToken = (nodeOrComment as ts.PropertyDeclaration).exclamationToken;
    return [name.getStart(), exclamationToken ? exclamationToken.getEnd() : name.getEnd()];
  }

  static getStructuralIdentityHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    let node: ts.Node | undefined;
    if (nodeOrComment.kind === ts.SyntaxKind.ReturnStatement) {
      node = (nodeOrComment as ts.ReturnStatement).expression;
    } else if (nodeOrComment.kind === ts.SyntaxKind.PropertyDeclaration) {
      node = (nodeOrComment as ts.PropertyDeclaration).name;
    }

    if (node !== undefined) {
      return [node.getStart(), node.getEnd()];
    }

    return undefined;
  }

  static getIncompationbleFunctionTypeHighlightRange(
    nodeOrComment: ts.Node | ts.CommentRange
  ): [number, number] | undefined {
    const node = nodeOrComment as ts.Node;
    if (ts.isArrowFunction(node)) {
      const parameters = node.parameters;
      if (parameters.length > 0) {
        const firstParamStart = parameters[0].getStart();
        const lastParamEnd = parameters[parameters.length - 1].getEnd();
        return [firstParamStart, lastParamEnd];
      }
    }
    return [node.getStart(), node.getEnd()];
  }

  static getVoidOperatorHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, 'void');
  }

  static getImportLazyHighlightRange(nodeOrComment: ts.Node | ts.CommentRange): [number, number] | undefined {
    return this.getKeywordHighlightRange(nodeOrComment, 'lazy');
  }

  isStdRecordType(type: ts.Type): boolean {

    /*
     * In TypeScript, 'Record<K, T>' is defined as type alias to a mapped type.
     * Thus, it should have 'aliasSymbol' and 'target' properties. The 'target'
     * in this case will resolve to origin 'Record' symbol.
     */
    if (type.aliasSymbol) {
      const target = (type as ts.TypeReference).target;
      if (target) {
        const sym = target.aliasSymbol;
        return !!sym && sym.getName() === 'Record' && this.isGlobalSymbol(sym);
      }
    }

    return false;
  }

  isStdLongType(type: ts.Type | undefined): boolean {
    if (!type) {
      return false;
    }
    const sym = type.aliasSymbol;
    return !!sym && sym.getName() === 'long' && this.isGlobalSymbol(sym);
  }

  static ifLargerThanInt(node: ts.NumericLiteral, isPrefix: boolean): boolean {
    const raw = node.getText();
    const value = isPrefix ? Number(raw) * -1 : Number(raw);

    return value < INT_MIN || value > INT_MAX;
  }

  static isLargeNumericLiteral(node: ts.NumericLiteral, isPrefix: boolean): boolean {
    const raw = node.getText();
    const value = isPrefix ? Number(raw) * -1 : Number(raw);

    return value < LARGE_NUMBER_MIN || value > LARGE_NUMBER_MAX;
  }

  isStdErrorType(type: ts.Type): boolean {
    const symbol = type.symbol;
    if (!symbol) {
      return false;
    }
    const name = this.tsTypeChecker.getFullyQualifiedName(symbol);
    return name === 'Error' && this.isGlobalSymbol(symbol);
  }

  isStdPartialType(type: ts.Type): boolean {
    const sym = type.aliasSymbol;
    return !!sym && sym.getName() === 'Partial' && this.isGlobalSymbol(sym);
  }

  isStdRequiredType(type: ts.Type): boolean {
    const sym = type.aliasSymbol;
    return !!sym && sym.getName() === 'Required' && this.isGlobalSymbol(sym);
  }

  isStdReadonlyType(type: ts.Type): boolean {
    const sym = type.aliasSymbol;
    return !!sym && sym.getName() === 'Readonly' && this.isGlobalSymbol(sym);
  }

  isLibraryType(type: ts.Type): boolean {
    const nonNullableType = type.getNonNullableType();
    if (nonNullableType.isUnion()) {
      for (const componentType of nonNullableType.types) {
        if (!this.isLibraryType(componentType)) {
          return false;
        }
      }
      return true;
    }
    return this.isLibrarySymbol(nonNullableType.aliasSymbol ?? nonNullableType.getSymbol());
  }

  hasLibraryType(node: ts.Node): boolean {
    return this.isLibraryType(this.tsTypeChecker.getTypeAtLocation(node));
  }

  isLibrarySymbol(sym: ts.Symbol | undefined): boolean {
    if (sym?.declarations && sym.declarations.length > 0) {
      const srcFile = sym.declarations[0].getSourceFile();
      if (!srcFile) {
        return false;
      }
      const fileName = srcFile.fileName;

      /*
       * Symbols from both *.ts and *.d.ts files should obey interop rules.
       * We disable such behavior for *.ts files in the test mode due to lack of 'ets'
       * extension support.
       */
      const ext = path.extname(fileName).toLowerCase();
      const isThirdPartyCode =
        ARKTS_IGNORE_DIRS.some((ignore) => {
          return srcFilePathContainsDirectory(srcFile, ignore);
        }) ||
        ARKTS_IGNORE_FILES.some((ignore) => {
          return path.basename(fileName) === ignore;
        });
      const isEts = ext === '.ets';
      const isTs = ext === '.ts' && !srcFile.isDeclarationFile;
      const isStatic = (isEts || isTs && this.options.checkTsAsSource) && !isThirdPartyCode;
      const isStdLib = STANDARD_LIBRARIES.includes(path.basename(fileName).toLowerCase());

      /*
       * We still need to confirm support for certain API from the
       * TypeScript standard library in ArkTS. Thus, for now do not
       * count standard library modules as dynamic.
       */
      return !isStatic && !isStdLib;
    }
    return false;
  }

  static isOhModulesEtsSymbol(sym: ts.Symbol | undefined): boolean {
    const sourceFile = sym?.declarations?.[0]?.getSourceFile();
    return (
      !!sourceFile &&
      path.extname(sourceFile.fileName).toLowerCase() === ETS &&
      srcFilePathContainsDirectory(sourceFile, ARKTS_IGNORE_DIRS_OH_MODULES)
    );
  }

  isDynamicType(type: ts.Type | undefined): boolean | undefined {
    if (type === undefined) {
      return false;
    }

    /*
     * Return 'true' if it is an object of library type initialization, otherwise
     * return 'false' if it is not an object of standard library type one.
     * In the case of standard library type we need to determine context.
     */

    /*
     * Check the non-nullable version of type to eliminate 'undefined' type
     * from the union type elements.
     */
    // eslint-disable-next-line no-param-reassign
    type = type.getNonNullableType();

    if (type.isUnion()) {
      for (const compType of type.types) {
        const isDynamic = this.isDynamicType(compType);
        if (isDynamic || isDynamic === undefined) {
          return isDynamic;
        }
      }
      return false;
    }

    if (this.isLibraryType(type)) {
      return true;
    }

    if (!isStdLibraryType(type) && !isIntrinsicObjectType(type) && !TsUtils.isAnyType(type)) {
      return false;
    }

    return undefined;
  }

  static isObjectType(type: ts.Type): type is ts.ObjectType {
    return !!(type.flags & ts.TypeFlags.Object);
  }

  private static isAnonymous(type: ts.Type): boolean {
    if (TsUtils.isObjectType(type)) {
      return !!(type.objectFlags & ts.ObjectFlags.Anonymous);
    }
    return false;
  }

  private isDynamicLiteralInitializerHandleCallExpression(callExpr: ts.CallExpression): boolean {
    const type = this.tsTypeChecker.getTypeAtLocation(callExpr.expression);

    if (TsUtils.isAnyType(type)) {
      return true;
    }

    let sym: ts.Symbol | undefined = type.symbol;
    if (this.isLibrarySymbol(sym)) {
      return true;
    }

    /*
     * #13483:
     * x.foo({ ... }), where 'x' is exported from some library:
     */
    if (ts.isPropertyAccessExpression(callExpr.expression)) {
      sym = this.trueSymbolAtLocation(callExpr.expression.expression);
      if (sym && this.isLibrarySymbol(sym)) {
        return true;
      }
    }

    return false;
  }

  isDynamicLiteralInitializer(expr: ts.Expression): boolean {
    if (!ts.isObjectLiteralExpression(expr) && !ts.isArrayLiteralExpression(expr)) {
      return false;
    }

    /*
     * Handle nested literals:
     * { f: { ... } }
     */
    let curNode: ts.Node = expr;
    while (ts.isObjectLiteralExpression(curNode) || ts.isArrayLiteralExpression(curNode)) {
      const exprType = this.tsTypeChecker.getContextualType(curNode);
      if (exprType !== undefined && !TsUtils.isAnonymous(exprType)) {
        const res = this.isDynamicType(exprType);
        if (res !== undefined) {
          return res;
        }
      }

      curNode = curNode.parent;
      if (ts.isPropertyAssignment(curNode)) {
        curNode = curNode.parent;
      }
    }

    /*
     * Handle calls with literals:
     * foo({ ... })
     */
    if (ts.isCallExpression(curNode) && this.isDynamicLiteralInitializerHandleCallExpression(curNode)) {
      return true;
    }

    /*
     * Handle property assignments with literals:
     * obj.f = { ... }
     */
    if (ts.isBinaryExpression(curNode)) {
      const binExpr = curNode;
      if (ts.isPropertyAccessExpression(binExpr.left)) {
        const propAccessExpr = binExpr.left;
        const type = this.tsTypeChecker.getTypeAtLocation(propAccessExpr.expression);
        return this.isLibrarySymbol(type.symbol);
      }
    }

    return false;
  }

  static isEsValueType(typeNode: ts.TypeNode | undefined): boolean {
    return (
      !!typeNode &&
      ts.isTypeReferenceNode(typeNode) &&
      ts.isIdentifier(typeNode.typeName) &&
      typeNode.typeName.text === ES_VALUE
    );
  }

  static isInsideBlock(node: ts.Node): boolean {
    let par = node.parent;
    while (par) {
      if (ts.isBlock(par)) {
        return true;
      }
      par = par.parent;
    }
    return false;
  }

  static isEsValuePossiblyAllowed(typeRef: ts.TypeReferenceNode): boolean {
    return ts.isVariableDeclaration(typeRef.parent);
  }

  isValueAssignableToESValue(node: ts.Node): boolean {
    if (ts.isArrayLiteralExpression(node) || ts.isObjectLiteralExpression(node)) {
      return false;
    }
    const valueType = this.tsTypeChecker.getTypeAtLocation(node);
    return TsUtils.isUnsupportedType(valueType) || TsUtils.isAnonymousType(valueType);
  }

  getVariableDeclarationTypeNode(node: ts.Node): ts.TypeNode | undefined {
    const sym = this.trueSymbolAtLocation(node);
    if (sym === undefined) {
      return undefined;
    }
    return TsUtils.getVariableSymbolDeclarationTypeNode(sym);
  }

  static getVariableSymbolDeclarationTypeNode(sym: ts.Symbol): ts.TypeNode | undefined {
    const decl = TsUtils.getDeclaration(sym);
    if (!!decl && ts.isVariableDeclaration(decl)) {
      return decl.type;
    }
    return undefined;
  }

  getDeclarationTypeNode(node: ts.Node): ts.TypeNode | undefined {
    const sym = this.trueSymbolAtLocation(node);
    if (sym === undefined) {
      return undefined;
    }
    return TsUtils.getSymbolDeclarationTypeNode(sym);
  }

  static getSymbolDeclarationTypeNode(sym: ts.Symbol): ts.TypeNode | undefined {
    const decl = TsUtils.getDeclaration(sym);
    if (!!decl && (ts.isVariableDeclaration(decl) || ts.isPropertyDeclaration(decl))) {
      return decl.type;
    }
    return undefined;
  }

  hasEsObjectType(node: ts.Node): boolean {
    const typeNode = this.getVariableDeclarationTypeNode(node);
    return typeNode !== undefined && TsUtils.isEsValueType(typeNode);
  }

  static symbolHasEsObjectType(sym: ts.Symbol): boolean {
    const typeNode = TsUtils.getVariableSymbolDeclarationTypeNode(sym);
    return typeNode !== undefined && TsUtils.isEsValueType(typeNode);
  }

  static isEsObjectSymbol(sym: ts.Symbol): boolean {
    const decl = TsUtils.getDeclaration(sym);
    return (
      !!decl &&
      ts.isTypeAliasDeclaration(decl) &&
      decl.name.escapedText === ES_VALUE &&
      decl.type.kind === ts.SyntaxKind.AnyKeyword
    );
  }

  static isAnonymousType(type: ts.Type): boolean {
    if (type.isUnionOrIntersection()) {
      for (const compType of type.types) {
        if (TsUtils.isAnonymousType(compType)) {
          return true;
        }
      }
      return false;
    }

    return (
      (type.flags & ts.TypeFlags.Object) !== 0 && ((type as ts.ObjectType).objectFlags & ts.ObjectFlags.Anonymous) !== 0
    );
  }

  getSymbolOfCallExpression(callExpr: ts.CallExpression): ts.Symbol | undefined {
    const signature = this.tsTypeChecker.getResolvedSignature(callExpr);
    const signDecl = signature?.getDeclaration();
    if (signDecl?.name) {
      return this.trueSymbolAtLocation(signDecl.name);
    }
    return undefined;
  }

  static isClassValueType(type: ts.Type): boolean {
    if (
      (type.flags & ts.TypeFlags.Object) === 0 ||
      ((type as ts.ObjectType).objectFlags & ts.ObjectFlags.Anonymous) === 0
    ) {
      return false;
    }
    return type.symbol && (type.symbol.flags & ts.SymbolFlags.Class) !== 0;
  }

  isClassObjectExpression(expr: ts.Expression): boolean {
    if (!TsUtils.isClassValueType(this.tsTypeChecker.getTypeAtLocation(expr))) {
      return false;
    }
    const symbol = this.trueSymbolAtLocation(expr);
    return !symbol || (symbol.flags & ts.SymbolFlags.Class) === 0;
  }

  isClassTypeExpression(expr: ts.Expression): boolean {
    const sym = this.trueSymbolAtLocation(expr);
    return sym !== undefined && (sym.flags & ts.SymbolFlags.Class) !== 0;
  }

  isFunctionCalledRecursively(funcExpr: ts.FunctionExpression): boolean {
    if (!funcExpr.name) {
      return false;
    }

    const sym = this.tsTypeChecker.getSymbolAtLocation(funcExpr.name);
    if (!sym) {
      return false;
    }

    let found = false;
    const callback = (node: ts.Node): void => {
      if (ts.isCallExpression(node) && ts.isIdentifier(node.expression)) {
        const callSym = this.tsTypeChecker.getSymbolAtLocation(node.expression);
        if (callSym && callSym === sym) {
          found = true;
        }
      }
    };

    const stopCondition = (node: ts.Node): boolean => {
      void node;
      return found;
    };

    forEachNodeInSubtree(funcExpr, callback, stopCondition);
    return found;
  }

  getTypeOrTypeConstraintAtLocation(expr: ts.Expression): ts.Type {
    const type = this.tsTypeChecker.getTypeAtLocation(expr);
    if (type.isTypeParameter()) {
      const constraint = type.getConstraint();
      if (constraint) {
        return constraint;
      }
    }
    return type;
  }

  private skipStructuralTypingCheckForFunctionals(lhsType: ts.Type, rhsType: ts.Type): boolean {
    return (
      (this.isStdFunctionType(lhsType) || TsUtils.isFunctionalType(lhsType)) &&
      (this.isStdFunctionType(rhsType) || TsUtils.isFunctionalType(rhsType))
    );
  }

  areCompatibleFunctionalTypes(lhsType: ts.Type, rhsType: ts.Type): boolean {
    if (lhsType.isUnion()) {
      for (let i = 0; i < lhsType.types.length; i++) {
        if (this.areCompatibleFunctionalTypes(lhsType.types[i], rhsType)) {
          return true;
        }
      }
      return false;
    }

    const lhsSignature = TsUtils.getFunctionalTypeSignature(lhsType);
    const rhsSignature = TsUtils.getFunctionalTypeSignature(rhsType);
    if (!lhsSignature || !rhsSignature) {
      return true;
    }

    if (lhsSignature.parameters.length < rhsSignature.parameters.length) {
      return false;
    }

    const lhsReturnType = lhsSignature.getReturnType();
    const rhsReturnType = rhsSignature.getReturnType();
    if (lhsReturnType && rhsReturnType) {
      return !(TsUtils.isVoidType(lhsReturnType) && !TsUtils.isVoidType(rhsReturnType));
    }

    return true;
  }

  static isVoidType(tsType: ts.Type): boolean {
    return (tsType.getFlags() & ts.TypeFlags.Void) !== 0;
  }

  static isFunctionalType(type: ts.Type): boolean {
    const callSigns = type.getCallSignatures();
    return callSigns && callSigns.length > 0;
  }

  static isArrayBufferType(typeStr: string): boolean {
    const arrayBufferLikeArr = ['ArrayBuffer', 'ArrayBufferLike'];
    return arrayBufferLikeArr.includes(typeStr);
  }

  static getFunctionalTypeSignature(type: ts.Type): ts.Signature | undefined {
    const callSigns = type.getCallSignatures();
    if (callSigns.length > 0) {
      return callSigns[0];
    }
    return undefined;
  }

  static getFunctionReturnType(type: ts.Type): ts.Type | null {
    const signatures = type.getCallSignatures();
    if (signatures.length === 0) {
      return null;
    }
    return signatures[0].getReturnType();
  }

  isStdFunctionType(type: ts.Type): boolean {
    const sym = type.getSymbol();
    return !!sym && sym.getName() === 'Function' && this.isGlobalSymbol(sym);
  }

  isStdBigIntType(type: ts.Type): boolean {
    const sym = type.symbol;
    return !!sym && sym.getName() === 'BigInt' && this.isGlobalSymbol(sym);
  }

  isStdNumberType(type: ts.Type): boolean {
    const sym = type.symbol;
    return !!sym && sym.getName() === 'Number' && this.isGlobalSymbol(sym);
  }

  isStdBooleanType(type: ts.Type): boolean {
    const sym = type.symbol;
    return !!sym && sym.getName() === 'Boolean' && this.isGlobalSymbol(sym);
  }

  isEnumStringLiteral(expr: ts.Expression): boolean {
    const symbol = this.trueSymbolAtLocation(expr);
    const isEnumMember = !!symbol && !!(symbol.flags & ts.SymbolFlags.EnumMember);
    const type = this.tsTypeChecker.getTypeAtLocation(expr);
    const isStringEnumLiteral = TsUtils.isEnumType(type) && !!(type.flags & ts.TypeFlags.StringLiteral);
    return isEnumMember && isStringEnumLiteral;
  }

  isValidComputedPropertyName(computedProperty: ts.ComputedPropertyName, isRecordObjectInitializer = false): boolean {
    const expr = computedProperty.expression;
    if (!isRecordObjectInitializer) {
      if (this.isSymbolIteratorExpression(expr)) {
        return true;
      }
    }
    // In ArkTS 1.0, the computed property names are allowed if expression is string literal or string Enum member.
    return (
      !this.options.arkts2 && (ts.isStringLiteralLike(expr) || this.isEnumStringLiteral(computedProperty.expression))
    );
  }

  skipPropertyInferredTypeCheck(
    decl: ts.PropertyDeclaration,
    sourceFile: ts.SourceFile | undefined,
    isEtsFileCb: IsEtsFileCallback | undefined
  ): boolean {
    if (!sourceFile) {
      return false;
    }

    const isEts = this.options.useRtLogic ?
      !!isEtsFileCb && isEtsFileCb(sourceFile) :
      getScriptKind(sourceFile) === ts.ScriptKind.ETS;
    return (
      isEts &&
      sourceFile.isDeclarationFile &&
      !!decl.modifiers?.some((m) => {
        return m.kind === ts.SyntaxKind.PrivateKeyword;
      })
    );
  }

  hasAccessModifier(decl: ts.HasModifiers): boolean {
    const modifiers = ts.getModifiers(decl);
    return (
      !!modifiers &&
      (!this.options.useRtLogic && TsUtils.hasModifier(modifiers, ts.SyntaxKind.ReadonlyKeyword) ||
        TsUtils.hasModifier(modifiers, ts.SyntaxKind.PublicKeyword) ||
        TsUtils.hasModifier(modifiers, ts.SyntaxKind.ProtectedKeyword) ||
        TsUtils.hasModifier(modifiers, ts.SyntaxKind.PrivateKeyword) ||
        !!this.options.arkts2 &&
          (TsUtils.hasModifier(modifiers, ts.SyntaxKind.ReadonlyKeyword) ||
            TsUtils.hasModifier(modifiers, ts.SyntaxKind.OverrideKeyword)))
    );
  }

  static getModifier(
    modifiers: readonly ts.Modifier[] | undefined,
    modifierKind: ts.SyntaxKind
  ): ts.Modifier | undefined {
    if (!modifiers) {
      return undefined;
    }
    return modifiers.find((x) => {
      return x.kind === modifierKind;
    });
  }

  static getAccessModifier(modifiers: readonly ts.Modifier[] | undefined): ts.Modifier | undefined {
    return (
      TsUtils.getModifier(modifiers, ts.SyntaxKind.PublicKeyword) ??
      TsUtils.getModifier(modifiers, ts.SyntaxKind.ProtectedKeyword) ??
      TsUtils.getModifier(modifiers, ts.SyntaxKind.PrivateKeyword)
    );
  }

  static getBaseClassType(type: ts.Type): ts.InterfaceType | undefined {
    const baseTypes = type.getBaseTypes();
    if (baseTypes) {
      for (const baseType of baseTypes) {
        if (baseType.isClass()) {
          return baseType;
        }
      }
    }

    return undefined;
  }

  static destructuringAssignmentHasSpreadOperator(node: ts.AssignmentPattern): boolean {
    if (ts.isArrayLiteralExpression(node)) {
      return node.elements.some((x) => {
        if (ts.isSpreadElement(x)) {
          return true;
        }
        if (ts.isObjectLiteralExpression(x) || ts.isArrayLiteralExpression(x)) {
          return TsUtils.destructuringAssignmentHasSpreadOperator(x);
        }
        return false;
      });
    }

    return node.properties.some((x) => {
      if (ts.isSpreadAssignment(x)) {
        return true;
      }
      if (
        ts.isPropertyAssignment(x) &&
        (ts.isObjectLiteralExpression(x.initializer) || ts.isArrayLiteralExpression(x.initializer))
      ) {
        return TsUtils.destructuringAssignmentHasSpreadOperator(x.initializer);
      }
      return false;
    });
  }

  static destructuringDeclarationHasSpreadOperator(node: ts.BindingPattern): boolean {
    return node.elements.some((x) => {
      if (ts.isBindingElement(x)) {
        if (x.dotDotDotToken) {
          return true;
        }
        if (ts.isArrayBindingPattern(x.name) || ts.isObjectBindingPattern(x.name)) {
          return TsUtils.destructuringDeclarationHasSpreadOperator(x.name);
        }
      }
      return false;
    });
  }

  // Check if the destructuring assignment has default values
  static destructuringAssignmentHasDefaultValue(node: ts.AssignmentPattern): boolean {
    if (ts.isArrayLiteralExpression(node)) {
      return node.elements.some((x) => {
        if (ts.isBinaryExpression(x) && x.right) {
          return true;
        }
        if (ts.isObjectLiteralExpression(x) || ts.isArrayLiteralExpression(x)) {
          return TsUtils.destructuringAssignmentHasDefaultValue(x);
        }
        return false;
      });
    }

    return node.properties.some((x) => {
      if (ts.isShorthandPropertyAssignment(x) && x.equalsToken) {
        return true;
      }
      if (
        ts.isPropertyAssignment(x) &&
        (ts.isObjectLiteralExpression(x.initializer) || ts.isArrayLiteralExpression(x.initializer))
      ) {
        return TsUtils.destructuringAssignmentHasDefaultValue(x.initializer);
      }
      return false;
    });
  }

  // Check if the destructuring declaration has default values
  static destructuringDeclarationHasDefaultValue(node: ts.BindingPattern): boolean {
    return node.elements.some((x) => {
      if (ts.isBindingElement(x)) {
        if (x.initializer) {
          return true;
        }
        if (ts.isArrayBindingPattern(x.name) || ts.isObjectBindingPattern(x.name)) {
          return TsUtils.destructuringDeclarationHasDefaultValue(x.name);
        }
      }
      return false;
    });
  }

  // Check that the dimensions of the destruct array are the same
  static isSameDimension(arr: ts.Node): boolean | undefined {
    if (!ts.isArrayLiteralExpression(arr)) {
      return true;
    }
    let hasOneDimensional: boolean = false;
    let hasMultipleDimensions: boolean = false;
    const arrElements = arr.elements;
    let maxlen: number = 0;
    let minlen: number = 1000;
    for (let i = 0; i < arrElements.length; i++) {
      const e = arrElements[i] as ts.Node;
      if (!ts.isArrayLiteralExpression(e)) {
        hasOneDimensional = true;
      } else {
        hasMultipleDimensions = true;
        if (hasOneDimensional && hasMultipleDimensions) {
          return false;
        }
        if (!this.isSameDimension(e)) {
          return false;
        }
        maxlen = Math.max(e.elements.length, maxlen);
        minlen = Math.min(e.elements.length, minlen);
      }
    }
    if (hasOneDimensional && hasMultipleDimensions) {
      return false;
    } else if (hasOneDimensional && !hasMultipleDimensions) {
      return true;
    } else if (!hasOneDimensional && hasMultipleDimensions && maxlen === minlen) {
      return true;
    }
    return false;
  }

  // if ArrayLiteralExpression has empty element, will be marked as not fixable
  static checkArrayLiteralHasEmptyElement(node: ts.ArrayLiteralExpression): boolean {
    return node.elements.some((x) => {
      if (ts.isOmittedExpression(x)) {
        return true;
      }
      if (ts.isArrayLiteralExpression(x)) {
        if (x.elements.length === 0) {
          return true;
        }
        return this.checkArrayLiteralHasEmptyElement(x);
      }
      return false;
    });
  }

  // if ArrayBindingPattern has empty element, will be marked as not fixable
  static checkArrayBindingHasEmptyElement(node: ts.ArrayBindingPattern): boolean {
    return node.elements.some((x) => {
      if (ts.isOmittedExpression(x)) {
        return true;
      }
      if (ts.isBindingElement(x) && ts.isArrayBindingPattern(x.name)) {
        if (x.name.elements.length === 0) {
          return true;
        }
        return this.checkArrayBindingHasEmptyElement(x.name);
      }
      return false;
    });
  }

  static hasNestedObjectDestructuring(node: ts.ArrayBindingOrAssignmentPattern): boolean {
    if (ts.isArrayLiteralExpression(node)) {
      return node.elements.some((x) => {
        const elem = ts.isSpreadElement(x) ? x.expression : x;
        if (ts.isArrayLiteralExpression(elem)) {
          return TsUtils.hasNestedObjectDestructuring(elem);
        }
        return ts.isObjectLiteralExpression(elem);
      });
    }

    return node.elements.some((x) => {
      if (ts.isBindingElement(x)) {
        if (ts.isArrayBindingPattern(x.name)) {
          return TsUtils.hasNestedObjectDestructuring(x.name);
        }
        return ts.isObjectBindingPattern(x.name);
      }
      return false;
    });
  }

  static getDecoratorName(decorator: ts.Decorator): string {
    let decoratorName = '';
    if (ts.isIdentifier(decorator.expression)) {
      decoratorName = decorator.expression.text;
    } else if (ts.isCallExpression(decorator.expression) && ts.isIdentifier(decorator.expression.expression)) {
      decoratorName = decorator.expression.expression.text;
    }
    return decoratorName;
  }

  static unwrapParenthesizedTypeNode(typeNode: ts.TypeNode): ts.TypeNode {
    let unwrappedTypeNode = typeNode;
    while (ts.isParenthesizedTypeNode(unwrappedTypeNode)) {
      unwrappedTypeNode = unwrappedTypeNode.type;
    }

    return unwrappedTypeNode;
  }

  isSendableTypeNode(typeNode: ts.TypeNode, isShared: boolean = false): boolean {

    /*
     * In order to correctly identify the usage of the enum member or
     * const enum in type annotation, we need to handle union type and
     * type alias cases by processing the type node and checking the
     * symbol in case of type reference node.
     */

    // eslint-disable-next-line no-param-reassign
    typeNode = TsUtils.unwrapParenthesizedTypeNode(typeNode);

    // Only a sendable union type is supported
    if (ts.isUnionTypeNode(typeNode)) {
      return typeNode.types.every((elemType) => {
        return this.isSendableTypeNode(elemType, isShared);
      });
    }

    const sym = ts.isTypeReferenceNode(typeNode) ? this.trueSymbolAtLocation(typeNode.typeName) : undefined;

    if (sym && sym.getFlags() & ts.SymbolFlags.TypeAlias) {
      const typeDecl = TsUtils.getDeclaration(sym);
      if (typeDecl && ts.isTypeAliasDeclaration(typeDecl)) {
        const typeArgs = (typeNode as ts.TypeReferenceNode).typeArguments;
        if (
          typeArgs &&
          !typeArgs.every((typeArg) => {
            return this.isSendableTypeNode(typeArg);
          })
        ) {
          return false;
        }
        return this.isSendableTypeNode(typeDecl.type, isShared);
      }
    }

    // Const enum type is supported
    if (TsUtils.isConstEnum(sym)) {
      return true;
    }
    const type: ts.Type = this.tsTypeChecker.getTypeFromTypeNode(typeNode);

    // In shared module, literal forms of primitive data types can be exported
    if (isShared && TsUtils.isPurePrimitiveLiteralType(type)) {
      return true;
    }

    return this.isSendableType(type);
  }

  isSendableType(type: ts.Type): boolean {
    if (
      (type.flags &
        (ts.TypeFlags.Boolean |
          ts.TypeFlags.Number |
          ts.TypeFlags.String |
          ts.TypeFlags.BigInt |
          ts.TypeFlags.Null |
          ts.TypeFlags.Undefined |
          ts.TypeFlags.TypeParameter)) !==
      0
    ) {
      return true;
    }
    if (this.isSendableTypeAlias(type)) {
      return true;
    }
    if (TsUtils.isSendableFunction(type)) {
      return true;
    }

    return this.isSendableClassOrInterface(type);
  }

  isShareableType(tsType: ts.Type): boolean {
    const sym = tsType.getSymbol();
    if (TsUtils.isConstEnum(sym)) {
      return true;
    }

    if (tsType.isUnion()) {
      return tsType.types.every((elemType) => {
        return this.isShareableType(elemType);
      });
    }

    if (TsUtils.isPurePrimitiveLiteralType(tsType)) {
      return true;
    }

    return this.isSendableType(tsType);
  }

  isSendableClassOrInterface(type: ts.Type): boolean {
    const sym = type.getSymbol();
    if (!sym) {
      return false;
    }

    const targetType = TsUtils.reduceReference(type);

    // class with @Sendable decorator
    if (targetType.isClass()) {
      if (sym.declarations?.length) {
        const decl = sym.declarations[0];
        if (ts.isClassDeclaration(decl)) {
          return TsUtils.hasSendableDecorator(decl);
        }
      }
    }
    // ISendable interface, or a class/interface that implements/extends ISendable interface
    return this.isOrDerivedFrom(type, TsUtils.isISendableInterface);
  }

  typeContainsSendableClassOrInterface(type: ts.Type): boolean {
    // Only check type contains sendable class / interface
    if ((type.flags & ts.TypeFlags.Union) !== 0) {
      return !!(type as ts.UnionType)?.types?.some((type) => {
        return this.typeContainsSendableClassOrInterface(type);
      });
    }

    return this.isSendableClassOrInterface(type);
  }

  typeContainsNonSendableClassOrInterface(type: ts.Type): boolean {
    if (type.isUnion()) {
      return type.types.some((compType) => {
        return this.typeContainsNonSendableClassOrInterface(compType);
      });
    }
    // eslint-disable-next-line no-param-reassign
    type = TsUtils.reduceReference(type);
    return type.isClassOrInterface() && !this.isSendableClassOrInterface(type);
  }

  static isConstEnum(sym: ts.Symbol | undefined): boolean {
    return !!sym && sym.flags === ts.SymbolFlags.ConstEnum;
  }

  isSendableUnionType(type: ts.UnionType): boolean {
    const types = type?.types;
    if (!types) {
      return false;
    }

    return types.every((type) => {
      return this.isSendableType(type);
    });
  }

  static hasSendableDecorator(decl: ts.ClassDeclaration | ts.FunctionDeclaration | ts.TypeAliasDeclaration): boolean {
    return !!TsUtils.getSendableDecorator(decl);
  }

  static getNonSendableDecorators(
    decl: ts.ClassDeclaration | ts.FunctionDeclaration | ts.TypeAliasDeclaration
  ): ts.Decorator[] | undefined {
    const decorators = ts.getAllDecorators(decl);
    return decorators?.filter((x) => {
      return TsUtils.getDecoratorName(x) !== SENDABLE_DECORATOR;
    });
  }

  static getSendableDecorator(
    decl: ts.ClassDeclaration | ts.FunctionDeclaration | ts.TypeAliasDeclaration
  ): ts.Decorator | undefined {
    const decorators = ts.getAllDecorators(decl);
    return decorators?.find((x) => {
      return TsUtils.getDecoratorName(x) === SENDABLE_DECORATOR;
    });
  }

  static getDecoratorsIfInSendableClass(declaration: ts.HasDecorators): readonly ts.Decorator[] | undefined {
    const classNode = TsUtils.getClassNodeFromDeclaration(declaration);
    if (classNode === undefined || !TsUtils.hasSendableDecorator(classNode)) {
      return undefined;
    }
    return ts.getDecorators(declaration);
  }

  private static getClassNodeFromDeclaration(declaration: ts.HasDecorators): ts.ClassDeclaration | undefined {
    if (declaration.kind === ts.SyntaxKind.Parameter) {
      return ts.isClassDeclaration(declaration.parent.parent) ? declaration.parent.parent : undefined;
    }
    return ts.isClassDeclaration(declaration.parent) ? declaration.parent : undefined;
  }

  static isISendableInterface(type: ts.Type): boolean {
    const symbol = type.aliasSymbol ?? type.getSymbol();
    if (symbol?.declarations === undefined || symbol.declarations.length < 1) {
      return false;
    }

    return TsUtils.isArkTSISendableDeclaration(symbol.declarations[0]);
  }

  private static isArkTSISendableDeclaration(decl: ts.Declaration): boolean {
    if (!ts.isInterfaceDeclaration(decl) || !decl.name || decl.name.text !== ISENDABLE_TYPE) {
      return false;
    }

    if (!ts.isModuleBlock(decl.parent) || decl.parent.parent.name.text !== LANG_NAMESPACE) {
      return false;
    }

    if (path.basename(decl.getSourceFile().fileName).toLowerCase() !== ARKTS_LANG_D_ETS) {
      return false;
    }

    return true;
  }

  isAllowedIndexSignature(node: ts.IndexSignatureDeclaration): boolean {

    /*
     * For now, relax index signature only for specific array-like types
     * with the following signature: 'collections.Array<T>.[_: number]: T'.
     */

    if (node.parameters.length !== 1) {
      return false;
    }

    const paramType = this.tsTypeChecker.getTypeAtLocation(node.parameters[0]);
    if ((paramType.flags & ts.TypeFlags.Number) === 0) {
      return false;
    }

    return this.isArkTSCollectionsArrayLikeDeclaration(node.parent);
  }

  isArkTSCollectionsArrayLikeType(type: ts.Type): boolean {
    const symbol = type.aliasSymbol ?? type.getSymbol();
    if (symbol?.declarations === undefined || symbol.declarations.length < 1) {
      return false;
    }

    return this.isArkTSCollectionsArrayLikeDeclaration(symbol.declarations[0]);
  }

  private isArkTSCollectionsArrayLikeDeclaration(decl: ts.Declaration): boolean {
    if (!TsUtils.isArkTSCollectionsClassOrInterfaceDeclaration(decl)) {
      return false;
    }
    if (!this.tsTypeChecker.getTypeAtLocation(decl).getNumberIndexType()) {
      return false;
    }
    return true;
  }

  static isArkTSCollectionsClassOrInterfaceDeclaration(decl: ts.Node): boolean {
    if (!ts.isClassDeclaration(decl) && !ts.isInterfaceDeclaration(decl) || !decl.name) {
      return false;
    }
    if (!ts.isModuleBlock(decl.parent) || decl.parent.parent.name.text !== COLLECTIONS_NAMESPACE) {
      return false;
    }
    if (path.basename(decl.getSourceFile().fileName).toLowerCase() !== ARKTS_COLLECTIONS_D_ETS) {
      return false;
    }
    return true;
  }

  private proceedConstructorDeclaration(
    isFromPrivateIdentifierOrSdk: boolean,
    targetMember: ts.ClassElement,
    classMember: ts.ClassElement,
    isFromPrivateIdentifier: boolean
  ): boolean | undefined {
    if (
      isFromPrivateIdentifierOrSdk &&
      ts.isConstructorDeclaration(classMember) &&
      classMember.parameters.some((x) => {
        return (
          ts.isIdentifier(x.name) &&
          this.hasAccessModifier(x) &&
          this.isPrivateIdentifierDuplicateOfIdentifier(
            targetMember.name as ts.Identifier,
            x.name,
            isFromPrivateIdentifier
          )
        );
      })
    ) {
      return true;
    }
    return undefined;
  }

  private proceedClassType(
    targetMember: ts.ClassElement,
    classType: ts.Type,
    isFromPrivateIdentifier: boolean
  ): boolean | undefined {
    if (classType) {
      const baseType = TsUtils.getBaseClassType(classType);
      if (baseType) {
        const baseDecl = baseType.getSymbol()?.valueDeclaration as ts.ClassLikeDeclaration;
        if (baseDecl) {
          return this.classMemberHasDuplicateName(targetMember, baseDecl, isFromPrivateIdentifier);
        }
      }
    }
    return undefined;
  }

  classMemberHasDuplicateName(
    targetMember: ts.ClassElement,
    tsClassLikeDecl: ts.ClassLikeDeclaration,
    isFromPrivateIdentifier: boolean,
    classType?: ts.Type
  ): boolean {

    /*
     * If two class members have the same name where one is a private identifer,
     * then such members are considered to have duplicate names.
     */
    if (!TsUtils.isIdentifierOrPrivateIdentifier(targetMember.name)) {
      return false;
    }

    const isFromPrivateIdentifierOrSdk = this.isFromPrivateIdentifierOrSdk(isFromPrivateIdentifier);
    for (const classMember of tsClassLikeDecl.members) {
      if (targetMember === classMember) {
        continue;
      }

      // Check constructor parameter properties.
      const constructorDeclarationProceedResult = this.proceedConstructorDeclaration(
        isFromPrivateIdentifierOrSdk,
        targetMember,
        classMember,
        isFromPrivateIdentifier
      );
      if (constructorDeclarationProceedResult) {
        return constructorDeclarationProceedResult;
      }
      if (!TsUtils.isIdentifierOrPrivateIdentifier(classMember.name)) {
        continue;
      }
      if (this.isPrivateIdentifierDuplicateOfIdentifier(targetMember.name, classMember.name, isFromPrivateIdentifier)) {
        return true;
      }
    }

    if (isFromPrivateIdentifierOrSdk) {
      // eslint-disable-next-line no-param-reassign
      classType = classType ?? this.tsTypeChecker.getTypeAtLocation(tsClassLikeDecl);
      const proceedClassTypeResult = this.proceedClassType(targetMember, classType, isFromPrivateIdentifier);
      if (proceedClassTypeResult) {
        return proceedClassTypeResult;
      }
    }

    return false;
  }

  private isFromPrivateIdentifierOrSdk(isFromPrivateIdentifier: boolean): boolean {
    return this.options.useRtLogic || isFromPrivateIdentifier;
  }

  private static isIdentifierOrPrivateIdentifier(node?: ts.PropertyName): node is ts.Identifier | ts.PrivateIdentifier {
    if (!node) {
      return false;
    }
    return ts.isIdentifier(node) || ts.isPrivateIdentifier(node);
  }

  private isPrivateIdentifierDuplicateOfIdentifier(
    ident1: ts.Identifier | ts.PrivateIdentifier,
    ident2: ts.Identifier | ts.PrivateIdentifier,
    isFromPrivateIdentifier: boolean
  ): boolean {
    if (ts.isIdentifier(ident1) && ts.isPrivateIdentifier(ident2)) {
      return ident1.text === ident2.text.substring(1);
    }
    if (ts.isIdentifier(ident2) && ts.isPrivateIdentifier(ident1)) {
      return ident2.text === ident1.text.substring(1);
    }
    if (
      this.isFromPrivateIdentifierOrSdk(isFromPrivateIdentifier) &&
      ts.isPrivateIdentifier(ident1) &&
      ts.isPrivateIdentifier(ident2)
    ) {
      return ident1.text.substring(1) === ident2.text.substring(1);
    }
    return false;
  }

  findIdentifierNameForSymbol(symbol: ts.Symbol, enumMember?: ts.EnumMember): string | undefined {
    let name = TsUtils.getIdentifierNameFromString(symbol.name);
    if (name === undefined || name === symbol.name) {
      return name;
    }

    const enumExpress = enumMember?.parent;
    const parentType = this.getTypeByProperty(symbol);
    if (parentType === undefined) {
      return undefined;
    }
    if (enumExpress) {
      while (this.findEnumMember(enumExpress, name)) {
        name = '_' + name;
      }
    }
    while (this.findProperty(parentType, name) !== undefined) {
      name = '_' + name;
    }

    return name;
  }

  private static getIdentifierNameFromString(str: string): string | undefined {
    if (str === '') {
      return '__empty';
    }
    let result: string = '';

    let offset = 0;
    while (offset < str.length) {
      const codePoint = str.codePointAt(offset);
      if (!codePoint) {
        return undefined;
      }

      const charSize = TsUtils.charSize(codePoint);

      if (offset === 0 && !ts.isIdentifierStart(codePoint, undefined)) {
        result = '__';
      }

      if (!ts.isIdentifierPart(codePoint, undefined)) {
        if (codePoint === 0x20) {
          result += '_';
        } else {
          result += 'x' + codePoint.toString(16);
        }
      } else {
        for (let i = 0; i < charSize; i++) {
          result += str.charAt(offset + i);
        }
      }

      offset += charSize;
    }

    return result;
  }

  private static charSize(codePoint: number): number {
    return codePoint >= 0x10000 ? 2 : 1;
  }

  private getTypeByProperty(symbol: ts.Symbol): ts.Type | undefined {
    if (symbol.declarations === undefined) {
      return undefined;
    }

    for (const propDecl of symbol.declarations) {
      if (
        !ts.isPropertyDeclaration(propDecl) &&
        !ts.isPropertyAssignment(propDecl) &&
        !ts.isPropertySignature(propDecl) &&
        !ts.isEnumMember(propDecl)
      ) {
        return undefined;
      }

      const type = this.tsTypeChecker.getTypeAtLocation(propDecl.parent);
      if (type !== undefined) {
        return type;
      }
    }

    return undefined;
  }

  static isPropertyOfInternalClassOrInterface(symbol: ts.Symbol): boolean {
    if (symbol.declarations === undefined) {
      return false;
    }

    for (const propDecl of symbol.declarations) {
      if (!ts.isPropertyDeclaration(propDecl) && !ts.isPropertySignature(propDecl) && !ts.isEnumMember(propDecl)) {
        return false;
      }

      if (
        !ts.isClassDeclaration(propDecl.parent) &&
        !ts.isInterfaceDeclaration(propDecl.parent) &&
        !ts.isEnumDeclaration(propDecl.parent)
      ) {
        return false;
      }

      if (TsUtils.hasModifier(ts.getModifiers(propDecl.parent), ts.SyntaxKind.ExportKeyword)) {
        return false;
      }
    }

    return true;
  }

  static isIntrinsicObjectType(type: ts.Type): boolean {
    return !!(type.flags & ts.TypeFlags.NonPrimitive);
  }

  isStringType(tsType: ts.Type): boolean {
    if ((tsType.getFlags() & ts.TypeFlags.String) !== 0) {
      return true;
    }

    if (!TsUtils.isTypeReference(tsType)) {
      return false;
    }

    const symbol = tsType.symbol;
    if (symbol === undefined) {
      return false;
    }
    const name = this.tsTypeChecker.getFullyQualifiedName(symbol);
    return name === 'String' && this.isGlobalSymbol(symbol);
  }

  isStdMapType(type: ts.Type): boolean {
    const sym = type.symbol;
    return !!sym && sym.getName() === 'Map' && this.isGlobalSymbol(sym);
  }

  hasGenericTypeParameter(type: ts.Type): boolean {
    if (type.isUnionOrIntersection()) {
      return type.types.some((x) => {
        return this.hasGenericTypeParameter(x);
      });
    }
    if (TsUtils.isTypeReference(type)) {
      const typeArgs = this.tsTypeChecker.getTypeArguments(type);
      return typeArgs.some((x) => {
        return this.hasGenericTypeParameter(x);
      });
    }
    return type.isTypeParameter();
  }

  static getEnclosingTopLevelStatement(node: ts.Node): ts.Node | undefined {
    return ts.findAncestor(node, (ancestor) => {
      return ts.isSourceFile(ancestor.parent);
    });
  }

  static isDeclarationStatement(node: ts.Node): node is ts.DeclarationStatement {
    const kind = node.kind;
    return (
      kind === ts.SyntaxKind.FunctionDeclaration ||
      kind === ts.SyntaxKind.ModuleDeclaration ||
      kind === ts.SyntaxKind.ClassDeclaration ||
      kind === ts.SyntaxKind.StructDeclaration ||
      kind === ts.SyntaxKind.TypeAliasDeclaration ||
      kind === ts.SyntaxKind.InterfaceDeclaration ||
      kind === ts.SyntaxKind.EnumDeclaration ||
      kind === ts.SyntaxKind.MissingDeclaration ||
      kind === ts.SyntaxKind.ImportEqualsDeclaration ||
      kind === ts.SyntaxKind.ImportDeclaration ||
      kind === ts.SyntaxKind.NamespaceExportDeclaration
    );
  }

  static declarationNameExists(srcFile: ts.SourceFile, name: string): boolean {
    return srcFile.statements.some((stmt) => {
      return (
        TsUtils.checkDeclarationInBlockStatement(stmt, name) ||
        TsUtils.checkImportDeclaration(stmt, name) ||
        TsUtils.checkDeclarationInLoopStatement(stmt, name) ||
        TsUtils.checkDeclarationInVariableStatement(stmt, name) ||
        TsUtils.checkGeneralDeclaration(stmt, name)
      );
    });
  }

  static checkDeclarationInBlockStatement(stmt: ts.Statement, name: string): boolean {
    if (ts.isBlock(stmt)) {
      return stmt.statements.some((innerStmt) => {
        return (
          TsUtils.checkDeclarationInVariableStatement(innerStmt, name) ||
          TsUtils.checkGeneralDeclaration(innerStmt, name)
        );
      });
    }
    return false;
  }

  static checkImportDeclaration(stmt: ts.Statement, name: string): boolean {
    if (ts.isImportDeclaration(stmt)) {
      if (!stmt.importClause) {
        return false;
      }
      if (stmt.importClause.namedBindings) {
        if (ts.isNamespaceImport(stmt.importClause.namedBindings)) {
          return stmt.importClause.namedBindings.name.text === name;
        }
        return stmt.importClause.namedBindings.elements.some((x) => {
          return x.name.text === name;
        });
      }
      return stmt.importClause.name?.text === name;
    }
    return false;
  }

  static checkDeclarationInLoopStatement(stmt: ts.Statement, name: string): boolean {
    if (
      ts.isForStatement(stmt) ||
      ts.isWhileStatement(stmt) ||
      ts.isDoStatement(stmt) ||
      ts.isForOfStatement(stmt) ||
      ts.isForInStatement(stmt)
    ) {
      if (ts.isBlock(stmt.statement)) {
        return stmt.statement.statements.some((innerStmt) => {
          return (
            TsUtils.checkDeclarationInVariableStatement(innerStmt, name) ||
            TsUtils.checkGeneralDeclaration(innerStmt, name)
          );
        });
      }
    }
    return false;
  }

  static checkDeclarationInVariableStatement(stmt: ts.Statement, name: string): boolean {
    if (ts.isVariableStatement(stmt)) {
      return stmt.declarationList.declarations.some((decl) => {
        return decl.name.getText() === name;
      });
    }
    return false;
  }

  static checkGeneralDeclaration(stmt: ts.Statement, name: string): boolean {
    if (ts.isFunctionDeclaration(stmt) && stmt.body !== undefined) {
      return (
        stmt.body.statements.some((innerStmt) => {
          return TsUtils.checkDeclarationInVariableStatement(innerStmt, name);
        }) ||
        stmt.name !== undefined && ts.isIdentifier(stmt.name) && stmt.name.text === name
      );
    }
    return (
      TsUtils.isDeclarationStatement(stmt) &&
      stmt.name !== undefined &&
      ts.isIdentifier(stmt.name) &&
      stmt.name.text === name
    );
  }

  static generateUniqueName(nameGenerator: NameGenerator, srcFile: ts.SourceFile): string | undefined {
    let newName: string | undefined;

    do {
      newName = nameGenerator.getName();
      if (newName !== undefined && TsUtils.declarationNameExists(srcFile, newName)) {
        continue;
      }
      break;
    } while (newName !== undefined);

    return newName;
  }

  static isSharedModule(sourceFile: ts.SourceFile): boolean {
    const statements = sourceFile.statements;
    for (const statement of statements) {
      if (ts.isImportDeclaration(statement)) {
        continue;
      }

      return (
        ts.isExpressionStatement(statement) &&
        ts.isStringLiteral(statement.expression) &&
        statement.expression.text === USE_SHARED
      );
    }
    return false;
  }

  getDeclarationNode(node: ts.Node): ts.Declaration | undefined {
    const sym = this.trueSymbolAtLocation(node);
    return TsUtils.getDeclaration(sym);
  }

  static isFunctionLikeDeclaration(node: ts.Declaration): boolean {
    return (
      ts.isFunctionDeclaration(node) ||
      ts.isMethodDeclaration(node) ||
      ts.isGetAccessorDeclaration(node) ||
      ts.isSetAccessorDeclaration(node) ||
      ts.isConstructorDeclaration(node) ||
      ts.isFunctionExpression(node) ||
      ts.isArrowFunction(node)
    );
  }

  isShareableEntity(node: ts.Node): boolean {
    const decl = this.getDeclarationNode(node);
    // CC-OFFNXT(no_explicit_any) std lib
    const typeNode = (decl as any)?.type;
    return typeNode && !TsUtils.isFunctionLikeDeclaration(decl!) ?
      this.isSendableTypeNode(typeNode, true) :
      this.isShareableType(this.tsTypeChecker.getTypeAtLocation(decl ? decl : node));
  }

  isSendableClassOrInterfaceEntity(node: ts.Node): boolean {
    const decl = this.getDeclarationNode(node);
    if (!decl) {
      return false;
    }
    if (ts.isClassDeclaration(decl)) {
      return TsUtils.hasSendableDecorator(decl);
    }
    if (ts.isInterfaceDeclaration(decl)) {
      return this.isOrDerivedFrom(this.tsTypeChecker.getTypeAtLocation(decl), TsUtils.isISendableInterface);
    }
    return false;
  }

  static isInImportWhiteList(resolvedModule: ts.ResolvedModuleFull): boolean {
    if (
      !resolvedModule.resolvedFileName ||
      path.basename(resolvedModule.resolvedFileName).toLowerCase() !== ARKTS_LANG_D_ETS &&
        path.basename(resolvedModule.resolvedFileName).toLowerCase() !== ARKTS_COLLECTIONS_D_ETS
    ) {
      return false;
    }
    return true;
  }

  // If it is an overloaded function, all declarations for that function are found
  static hasSendableDecoratorFunctionOverload(decl: ts.FunctionDeclaration): boolean {
    const decorators = TsUtils.getFunctionOverloadDecorators(decl);
    return !!decorators?.some((x) => {
      return TsUtils.getDecoratorName(x) === SENDABLE_DECORATOR;
    });
  }

  static getFunctionOverloadDecorators(funcDecl: ts.FunctionDeclaration): readonly ts.Decorator[] | undefined {
    const decls = funcDecl.symbol.getDeclarations();
    if (!decls?.length) {
      return undefined;
    }
    let result: ts.Decorator[] = [];
    decls.forEach((decl) => {
      if (!ts.isFunctionDeclaration(decl)) {
        return;
      }
      const decorators = ts.getAllDecorators(decl);
      if (decorators?.length) {
        result = result.concat(decorators);
      }
    });
    return result.length ? result : undefined;
  }

  static isSendableFunction(type: ts.Type): boolean {
    const callSigns = type.getCallSignatures();
    if (!callSigns?.length) {
      return false;
    }
    const decl = callSigns[0].declaration;
    if (!decl || !ts.isFunctionDeclaration(decl)) {
      return false;
    }
    return TsUtils.hasSendableDecoratorFunctionOverload(decl);
  }

  isSendableTypeAlias(type: ts.Type): boolean {
    const decl = this.getTypsAliasOriginalDecl(type);
    return !!decl && TsUtils.hasSendableDecorator(decl);
  }

  hasSendableTypeAlias(type: ts.Type): boolean {
    if (type.isUnion()) {
      return type.types.some((compType) => {
        return this.hasSendableTypeAlias(compType);
      });
    }
    return this.isSendableTypeAlias(type);
  }

  isNonSendableFunctionTypeAlias(type: ts.Type): boolean {
    const decl = this.getTypsAliasOriginalDecl(type);
    return !!decl && ts.isFunctionTypeNode(decl.type) && !TsUtils.hasSendableDecorator(decl);
  }

  // If the alias refers to another alias, the search continues
  private getTypsAliasOriginalDecl(type: ts.Type): ts.TypeAliasDeclaration | undefined {
    if (!type.aliasSymbol) {
      return undefined;
    }
    const decl = TsUtils.getDeclaration(type.aliasSymbol);
    if (!decl || !ts.isTypeAliasDeclaration(decl)) {
      return undefined;
    }
    if (ts.isTypeReferenceNode(decl.type)) {
      const targetType = this.tsTypeChecker.getTypeAtLocation(decl.type.typeName);
      if (targetType.aliasSymbol && targetType.aliasSymbol.getFlags() & ts.SymbolFlags.TypeAlias) {
        return this.getTypsAliasOriginalDecl(targetType);
      }
    }
    return decl;
  }

  // not allow 'lhsType' contains 'sendable typeAlias' && 'rhsType' contains 'non-sendable function/non-sendable function typeAlias'
  isWrongSendableFunctionAssignment(lhsType: ts.Type, rhsType: ts.Type): boolean {
    // eslint-disable-next-line no-param-reassign
    lhsType = this.getNonNullableType(lhsType);
    // eslint-disable-next-line no-param-reassign
    rhsType = this.getNonNullableType(rhsType);
    if (!this.hasSendableTypeAlias(lhsType)) {
      return false;
    }

    if (rhsType.isUnion()) {
      return rhsType.types.some((compType) => {
        return this.isInvalidSendableFunctionAssignmentType(compType);
      });
    }
    return this.isInvalidSendableFunctionAssignmentType(rhsType);
  }

  private isInvalidSendableFunctionAssignmentType(type: ts.Type): boolean {
    if (type.aliasSymbol) {
      return this.isNonSendableFunctionTypeAlias(type);
    }
    if (TsUtils.isFunctionalType(type)) {
      return !TsUtils.isSendableFunction(type);
    }
    return false;
  }

  static isSetExpression(accessExpr: ts.ElementAccessExpression): boolean {
    if (!ts.isBinaryExpression(accessExpr.parent)) {
      return false;
    }
    const binaryExpr = accessExpr.parent;
    return binaryExpr.operatorToken.kind === ts.SyntaxKind.EqualsToken && binaryExpr.left === accessExpr;
  }

  haveSameBaseType(type1: ts.Type, type2: ts.Type): boolean {
    return this.tsTypeChecker.getBaseTypeOfLiteralType(type1) === this.tsTypeChecker.getBaseTypeOfLiteralType(type2);
  }

  isGetIndexableType(type: ts.Type, indexType: ts.Type): boolean {
    const getDecls = type.getProperty('$_get')?.getDeclarations();
    if (getDecls?.length !== 1 || getDecls[0].kind !== ts.SyntaxKind.MethodDeclaration) {
      return false;
    }
    const getMethodDecl = getDecls[0] as ts.MethodDeclaration;
    const getParams = getMethodDecl.parameters;
    if (getMethodDecl.type === undefined || getParams.length !== 1 || getParams[0].type === undefined) {
      return false;
    }

    return this.haveSameBaseType(this.tsTypeChecker.getTypeFromTypeNode(getParams[0].type), indexType);
  }

  isSetIndexableType(type: ts.Type, indexType: ts.Type, valueType: ts.Type): boolean {
    const setProp = type.getProperty('$_set');
    const setDecls = setProp?.getDeclarations();
    if (setDecls?.length !== 1 || setDecls[0].kind !== ts.SyntaxKind.MethodDeclaration) {
      return false;
    }
    const setMethodDecl = setDecls[0] as ts.MethodDeclaration;
    const setParams = setMethodDecl.parameters;
    if (
      setMethodDecl.type !== undefined ||
      setParams.length !== 2 ||
      setParams[0].type === undefined ||
      setParams[1].type === undefined
    ) {
      return false;
    }

    return (
      this.haveSameBaseType(this.tsTypeChecker.getTypeFromTypeNode(setParams[0].type), indexType) &&
      this.haveSameBaseType(this.tsTypeChecker.getTypeFromTypeNode(setParams[1].type), valueType)
    );
  }

  // Search for and save the exported declaration in the specified file, re-exporting another module will not be included.
  searchFileExportDecl(sourceFile: ts.SourceFile, targetDecls?: ts.SyntaxKind[]): Set<ts.Node> {
    const exportDeclSet = new Set<ts.Node>();
    const appendDecl = (decl: ts.Node | undefined): void => {
      if (!decl || targetDecls && !targetDecls.includes(decl.kind)) {
        return;
      }
      exportDeclSet.add(decl);
    };

    sourceFile.statements.forEach((statement: ts.Statement) => {
      if (ts.isExportAssignment(statement)) {
        // handle the case:"export default declName;"
        if (statement.isExportEquals) {
          return;
        }
        appendDecl(this.getDeclarationNode(statement.expression));
      } else if (ts.isExportDeclaration(statement)) {
        // handle the case:"export { declName1, declName2 };"
        if (!statement.exportClause || !ts.isNamedExports(statement.exportClause)) {
          return;
        }
        statement.exportClause.elements.forEach((specifier) => {
          appendDecl(this.getDeclarationNode(specifier.propertyName ?? specifier.name));
        });
      } else if (ts.canHaveModifiers(statement)) {
        // handle the case:"export const/class/function... decalName;"
        if (!TsUtils.hasModifier(ts.getModifiers(statement), ts.SyntaxKind.ExportKeyword)) {
          return;
        }
        if (!ts.isVariableStatement(statement)) {
          appendDecl(statement);
          return;
        }
        for (const exportDecl of statement.declarationList.declarations) {
          appendDecl(exportDecl);
        }
      }
    });
    return exportDeclSet;
  }

  static isAmbientNode(node: ts.Node): boolean {

    /* CC-OFFNXT(no_explicit_any) std lib */
    // Ambient flag is not exposed, so we apply dirty hack to make it visible
    return !!(node.flags & (ts.NodeFlags as any).Ambient);
  }

  isValidSendableClassExtends(tsTypeExpr: ts.ExpressionWithTypeArguments): boolean {
    const expr = tsTypeExpr.expression;
    const sym = this.tsTypeChecker.getSymbolAtLocation(expr);
    if (sym && (sym.flags & ts.SymbolFlags.Class) === 0) {
      // handle non-class situation(local / import)
      if ((sym.flags & ts.SymbolFlags.Alias) !== 0) {

        /*
         * Sendable class can not extends imported sendable class variable
         * Sendable class can extends imported sendable class
         */
        const realSym = this.tsTypeChecker.getAliasedSymbol(sym);
        if (realSym && (realSym.flags & ts.SymbolFlags.Class) === 0) {
          return false;
        }
        return true;
      }
      return false;
    }
    return true;
  }

  isBuiltinClassHeritageClause(node: ts.HeritageClause): boolean {
    const typeNode = node.types[0].expression;
    if (!typeNode) {
      return false;
    }

    const symbol = this.tsTypeChecker.getSymbolAtLocation(typeNode);
    return !!symbol && !isStdLibrarySymbol(symbol);
  }

  isIntegerVariable(sym: ts.Symbol | undefined): boolean {
    if (!sym) {
      return false;
    }
    const decl = TsUtils.getDeclaration(sym);
    if (!decl || !ts.isVariableDeclaration(decl) && !ts.isPropertyDeclaration(decl) || decl.type) {
      return false;
    }
    const init = decl.initializer;
    return !!init && ts.isNumericLiteral(init) && this.isIntegerConstantValue(init);
  }

  isIntegerValue(expr: ts.Expression): boolean {
    if (ts.isParenthesizedExpression(expr)) {
      return this.isIntegerValue(expr.expression);
    } else if (ts.isBinaryExpression(expr) && TsUtils.isArithmeticOperator(expr.operatorToken)) {
      return this.isIntegerValue(expr.left) && this.isIntegerValue(expr.right);
    } else if (
      (ts.isPrefixUnaryExpression(expr) || ts.isPostfixUnaryExpression(expr)) &&
      TsUtils.isUnaryArithmeticOperator(expr.operator)
    ) {
      return this.isIntegerValue(expr.operand);
    }

    return (
      ts.isNumericLiteral(expr) && this.isIntegerConstantValue(expr) ||
      this.isIntegerVariable(this.tsTypeChecker.getSymbolAtLocation(expr))
    );
  }

  static isAppStorageAccess(tsCallExpr: ts.CallExpression): boolean {
    const propertyAccessExpr = tsCallExpr.expression;
    if (!ts.isPropertyAccessExpression(propertyAccessExpr)) {
      return false;
    }
    const accessedExpr = propertyAccessExpr.expression;
    if (!ts.isIdentifier(accessedExpr)) {
      return false;
    }

    if (accessedExpr.text !== 'AppStorage') {
      return false;
    }

    return true;
  }

  static isArithmeticOperator(op: ts.BinaryOperatorToken): boolean {
    switch (op.kind) {
      case ts.SyntaxKind.PlusToken:
      case ts.SyntaxKind.MinusToken:
      case ts.SyntaxKind.AsteriskToken:
      case ts.SyntaxKind.SlashToken:
        return true;
      default:
        return false;
    }
  }

  static isUnaryArithmeticOperator(op: ts.SyntaxKind): boolean {
    switch (op) {
      case ts.SyntaxKind.PlusToken:
      case ts.SyntaxKind.MinusToken:
      case ts.SyntaxKind.PlusPlusToken:
      case ts.SyntaxKind.MinusMinusToken:
      case ts.SyntaxKind.TildeToken:
        return true;
      default:
        return false;
    }
  }

  isNumberLike(type: ts.Type): boolean {
    const typeFlags = type.flags;
    const typeText = this.tsTypeChecker.typeToString(type);

    const isNumberLike =
      typeText === STRINGLITERAL_NUMBER ||
      typeText === STRINGLITERAL_NUMBER_ARRAY ||
      (typeFlags & ts.TypeFlags.NumberLiteral) !== 0 ||
      this.isNumericEnumType(type);
    return isNumberLike;
  }

  isNumericEnumType(type: ts.Type): boolean {
    if (!TsUtils.isEnumType(type, true)) {
      return false;
    }
    const declarations = type.symbol?.getDeclarations() || [];
    const enumMemberDecl = declarations.find(ts.isEnumMember);
    if (enumMemberDecl) {
      const value = this.tsTypeChecker.getConstantValue(enumMemberDecl);
      return typeof value === STRINGLITERAL_NUMBER;
    }

    const enumDecl = declarations.find(ts.isEnumDeclaration);
    if (enumDecl) {
      return enumDecl.members.every((member) => {
        const memberType = this.tsTypeChecker.getTypeAtLocation(member.name);
        return (memberType.flags & ts.TypeFlags.NumberLike) !== 0;
      });
    }
    return false;
  }

  static getModuleName(node: ts.Node): string | undefined {
    const currentFilePath = node.getSourceFile().fileName;
    if (!currentFilePath.includes('src')) {
      return undefined;
    }

    /*
     * Assuming we are working with a path like "entry/src/main/ets/pages/test1.ets"
     * we are splitting at "/src" and get the first item in the list
     * in order to capture the "entry" group
     * we could have worked with an absolute path like
     * "home/user/projects/oh_project/entry/src/main/ets/pages/test1.sts"
     * in that case, after the fist split we would have
     * "home/user/projects/entry"
     * so we split once again with "/" to separate out the directories
     *
     * and get the last item which is entry or our module
     */
    const getPossibleModule = currentFilePath.split('/src')[0];
    if (getPossibleModule.length === 0) {
      return undefined;
    }
    const sanitizedDirectories = getPossibleModule.split('/');
    return sanitizedDirectories[sanitizedDirectories.length - 1];
  }

  static checkFileExists(
    importIncludesModule: boolean,
    currentNode: ts.Node,
    importFilePath: string,
    projectPath: string,
    extension: string = EXTNAME_ETS
  ): boolean {
    const currentModule = TsUtils.getModuleName(currentNode);

    /*
     * some imports are like this
     * ets/pages/test1
     * in this case, they are in the same module as the file we are trying to import to
     * so we add the current module back in
     */
    if (!importIncludesModule) {
      if (!currentModule) {
        return false;
      }

      projectPath.concat(PATH_SEPARATOR + currentModule);
    }

    const importedFile = path.resolve(projectPath, importFilePath + extension);

    return fs.existsSync(importedFile);
  }

  isImportedFromJS(identifier: ts.Identifier): boolean {
    const sym = this.trueSymbolAtLocation(identifier);
    const declaration = sym?.declarations?.[0];
    return !!declaration?.getSourceFile().fileName.endsWith(EXTNAME_JS);
  }

  isPossiblyImportedFromJS(node: ts.Node): boolean {
    const identifier = this.findIdentifierInExpression(node);
    if (!identifier) {
      return false;
    }

    // Direct import check
    if (this.isImportedFromJS(identifier)) {
      return true;
    }

    // Indirect import check
    const originalIdentifier = this.findOriginalIdentifier(identifier);
    return !!originalIdentifier && this.isImportedFromJS(originalIdentifier);
  }

  static isClassOrInterfaceDeclaration(d: ts.Declaration): d is ts.ClassDeclaration | ts.InterfaceDeclaration {
    return ts.isClassDeclaration(d) || ts.isInterfaceDeclaration(d);
  }

  /**
   * Extracts the Identifier from the given node. returns undefined if no Identifier is found.
   *
   * Direct Identifier (foo) → Returns it.
   * Binary Expressions (foo + bar) → Recursively checks left and right.
   * Property Access (obj.foo) → Extracts obj.
   * Function Calls (foo()) → Extracts foo.
   * Array Access (arr[foo]) → Extracts foo.
   * Variable Declaration (let foo = ...) → Extracts foo.
   * Assignments (foo = ...) → Extracts foo.
   */
  findIdentifierInExpression(node: ts.Node): ts.Identifier | undefined {
    if (ts.isIdentifier(node)) {
      return node;
    } else if (ts.isBinaryExpression(node)) {
      return this.findIdentifierInExpression(node.left) || this.findIdentifierInExpression(node.right);
    } else if (ts.isPropertyAccessExpression(node)) {
      return this.findIdentifierInExpression(node.expression);
    } else if (ts.isCallExpression(node) || ts.isNewExpression(node)) {
      return this.findIdentifierInExpression(node.expression);
    } else if (ts.isElementAccessExpression(node)) {
      return this.findIdentifierInExpression(node.expression);
    } else if (ts.isAsExpression(node)) {
      return this.findIdentifierInExpression(node.expression);
    } else if (ts.isVariableDeclaration(node) && ts.isIdentifier(node.name)) {
      return node.name;
    }
    return undefined;
  }

  // Helper function: Find the variable declaration of an identifier -> let arr = foo.arr -> returns foo
  findVariableDeclaration(identifier: ts.Identifier): ts.VariableDeclaration | undefined {
    const sym = this.tsTypeChecker.getSymbolAtLocation(identifier);
    const decl = TsUtils.getDeclaration(sym);
    if (decl && ts.isVariableDeclaration(decl)) {
      return decl;
    }
    return undefined;
  }

  findOriginalIdentifier(identifier: ts.Identifier): ts.Identifier | undefined {
    const variableDeclaration = this.findVariableDeclaration(identifier);
    if (!variableDeclaration?.initializer) {
      return undefined;
    }

    const originalIdentifier = ts.isPropertyAccessExpression(variableDeclaration.initializer) ?
      (variableDeclaration.initializer.expression as ts.Identifier) :
      undefined;

    if (!originalIdentifier) {
      return undefined;
    }

    return originalIdentifier;
  }

  findTypeOfNodeForConversion(node: ts.Node): string {
    // Get type of the property
    const type = this.tsTypeChecker.getTypeAtLocation(node);
    // Default
    let conversionMethod = '';

    if (this.tsTypeChecker.typeToString(type) === 'boolean') {
      conversionMethod = '.toBoolean()';
    } else if (this.tsTypeChecker.typeToString(type) === 'number') {
      conversionMethod = '.toNumber()';
    } else if (this.tsTypeChecker.typeToString(type) === 'string') {
      conversionMethod = '.toString()';
    }

    return conversionMethod;
  }

  static isInsideIfCondition(node: ts.Node): boolean {
    let current: ts.Node | undefined = node;
    while (current) {
      if (ts.isIfStatement(current)) {
        if (
          ts.isBinaryExpression(node.parent) &&
          node.parent.operatorToken.kind === ts.SyntaxKind.EqualsEqualsEqualsToken
        ) {
          return false;
        }

        return true;
      }
      current = current.parent;
    }
    return false;
  }

  static isOhModule(path: string): boolean {
    return path.includes(ETS_MODULE);
  }

  static isValidOhModulePath(path: string): boolean {
    return path.includes(VALID_OHM_COMPONENTS_MODULE_PATH);
  }

  findEnumMember(enumDecl: ts.EnumDeclaration | undefined, name: string): ts.Symbol | undefined {
    if (!enumDecl) {
      return undefined;
    }

    const syms: ts.Symbol[] = [];
    for (const enumMember of enumDecl.members) {
      const sym = this.trueSymbolAtLocation(enumMember.name);
      if (sym) {
        syms.push(sym);
      }
    }

    for (const sym of syms) {
      if (sym.name === name) {
        return sym;
      }
    }

    return undefined;
  }

  isArkts12File(sourceFile: ts.SourceFile): boolean {
    if (!sourceFile?.fileName) {
      return false;
    }
    if (sourceFile.fileName.endsWith(EXTNAME_D_ETS)) {
      return true;
    }
    return !!this.options.inputFiles?.includes(path.normalize(sourceFile.fileName));
  }

  static removeOrReplaceQuotes(str: string, isReplace: boolean): string {
    if (isReplace) {
      const charArray = new Array(str.length);
      for (let i = 0; i < str.length; i++) {
        charArray[i] = str[i] === '"' ? '\'' : str[i];
      }
      return charArray.join('');
    }
    if (str.startsWith('\'') && str.endsWith('\'') || str.startsWith('"') && str.endsWith('"')) {
      return str.slice(1, -1);
    }
    return str;
  }

  isJsImport(node: ts.Node): boolean {
    const symbol = this.trueSymbolAtLocation(node);
    if (symbol) {
      const declaration = symbol.declarations?.[0];
      if (declaration) {
        const sourceFile = declaration.getSourceFile();
        return sourceFile.fileName.endsWith(EXTNAME_JS);
      }
    }
    return false;
  }

  static typeIsCapturedFromEnclosingLocalScope(type: ts.Type, enclosingStmt: ts.Node): boolean {
    let symNode: ts.Node | undefined = TsUtils.getDeclaration(type.getSymbol());

    while (symNode) {
      if (symNode === enclosingStmt) {
        return true;
      }
      symNode = symNode.parent;
    }

    return false;
  }

  nodeCapturesValueFromEnclosingLocalScope(targetNode: ts.Node, enclosingStmt: ts.Node): boolean {
    let found = false;

    const callback = (node: ts.Node): void => {
      if (!ts.isIdentifier(node)) {
        return;
      }
      const sym = this.tsTypeChecker.getSymbolAtLocation(node);
      let symNode: ts.Node | undefined = TsUtils.getDeclaration(sym);
      while (symNode) {
        if (symNode === targetNode) {
          // Symbol originated from the target node. Skip and continue to search
          return;
        }
        if (symNode === enclosingStmt) {
          found = true;
          return;
        }
        symNode = symNode.parent;
      }
    };

    const stopCondition = (node: ts.Node): boolean => {
      void node;
      return found;
    };

    forEachNodeInSubtree(targetNode, callback, stopCondition);
    return found;
  }

  static typeContainsVoid(typeNode: ts.TypeNode): boolean {
    if (ts.isUnionTypeNode(typeNode)) {
      return typeNode.types.some((t) => {
        return t.kind === ts.SyntaxKind.VoidKeyword;
      });
    }
    return typeNode.kind === ts.SyntaxKind.VoidKeyword;
  }

  static isStdPromiseType(type: ts.Type): boolean {
    const sym = type.getSymbol();
    return !!sym && sym.getName() === 'Promise' && isStdLibrarySymbol(sym);
  }

  static isStdPromiseLikeType(type: ts.Type): boolean {
    const sym = type.getSymbol();
    return !!sym && sym.getName() === 'PromiseLike' && isStdLibrarySymbol(sym);
  }

  static checkStmtHasTargetIdentifier(stmt: ts.ExpressionStatement, target: string): boolean {
    let current: ts.Node | undefined = stmt.expression;

    while (current) {
      if (ts.isCallExpression(current)) {
        current = current.expression;
        continue;
      }

      if (ts.isPropertyAccessExpression(current)) {
        if (current.name.getText() === target) {
          return true;
        }
        current = current.expression;
        continue;
      }

      break;
    }

    return false;
  }

  collectPropertiesFromClass(
    classDecl: ts.ClassDeclaration,
    result: {
      staticProps: Map<string, ts.Type>;
      instanceProps: Map<string, ts.Type>;
    },
    isBase: boolean = false
  ): void {
    classDecl.members.forEach((member) => {
      if (!ts.isPropertyDeclaration(member) || !member.name || !ts.isIdentifier(member.name)) {
        return;
      }

      const propName = member.name.text;

      const isPrivate = member.modifiers?.some((m) => {
        return m.kind === ts.SyntaxKind.PrivateKeyword;
      });
      if (isBase && isPrivate) {
        return;
      }

      const propType = this.tsTypeChecker.getTypeAtLocation(member);
      const isStatic = member.modifiers?.some((m) => {
        return m.kind === ts.SyntaxKind.StaticKeyword;
      });

      if (isStatic) {
        result.staticProps.set(propName, propType);
      } else {
        result.instanceProps.set(propName, propType);
      }
    });

    const heritage = classDecl.heritageClauses?.find((h) => {
      return h.token === ts.SyntaxKind.ExtendsKeyword;
    });
    if (heritage) {
      const baseTypeNode = heritage?.types[0];
      if (baseTypeNode) {
        const baseType = this.tsTypeChecker.getTypeAtLocation(baseTypeNode);
        const baseSymbol = baseType.getSymbol();
        const declarations = baseSymbol?.getDeclarations();
        const baseClassDecl = declarations?.find(ts.isClassDeclaration);
        if (baseClassDecl) {
          this.collectPropertiesFromClass(baseClassDecl, result, true);
        }
      }
    }
  }

  static isNegativeNumericLiteral(expr: ts.Expression): boolean {
    return (
      ts.isPrefixUnaryExpression(expr) &&
      expr.operator === ts.SyntaxKind.MinusToken &&
      ts.isNumericLiteral(expr.operand)
    );
  }

  isNumberArrayType(type: ts.Type): boolean {
    if (!type.symbol || !this.isGenericArrayType(type)) {
      return false;
    }

    const typeArguments = this.tsTypeChecker.getTypeArguments(type);
    if (!typeArguments || typeArguments.length === 0) {
      return false;
    }

    return (typeArguments[0].flags & ts.TypeFlags.Number) !== 0;
  }

  static isIgnoredTypeForParameterType(typeString: string, type: ts.Type): boolean {
    if (TsUtils.isAnyType(type)) {
      return true;
    }

    return (
      IGNORE_TYPE_LIST.findIndex((ignored_type) => {
        return typeString.includes(ignored_type);
      }) !== -1
    );
  }
}
