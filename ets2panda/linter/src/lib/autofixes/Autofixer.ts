/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

import * as ts from 'typescript';
import { TsUtils } from '../utils/TsUtils';
import { scopeContainsThis } from '../utils/functions/ContainsThis';
import { NameGenerator } from '../utils/functions/NameGenerator';
import { isAssignmentOperator } from '../utils/functions/isAssignmentOperator';
import { SymbolCache } from './SymbolCache';
import { SENDABLE_DECORATOR } from '../utils/consts/SendableAPI';
import { ARKTSUTILS_LOCKS_MEMBER } from '../utils/consts/LimitedStdAPI';
import { DEFAULT_MODULE_NAME, PATH_SEPARATOR, SRC_AND_MAIN } from '../utils/consts/OhmUrl';
import {
  DOUBLE_DOLLAR_IDENTIFIER,
  THIS_IDENTIFIER,
  ATTRIBUTE_SUFFIX,
  INSTANCE_IDENTIFIER,
  COMMON_METHOD_IDENTIFIER,
  APPLY_STYLES_IDENTIFIER,
  CustomInterfaceName,
  ARKUI_MODULE,
  VALUE_IDENTIFIER,
  INDENT_STEP,
  ENTRY_DECORATOR_NAME,
  ENTRY_STORAGE_PROPERITY,
  LOCAL_STORAGE_TYPE_NAME,
  GET_LOCAL_STORAGE_FUNC_NAME,
  PROVIDE_DECORATOR_NAME,
  PROVIDE_ALIAS_PROPERTY_NAME,
  PROVIDE_ALLOW_OVERRIDE_PROPERTY_NAME,
  NEW_PROP_DECORATOR_SUFFIX,
  VIRTUAL_SCROLL_IDENTIFIER,
  DISABLE_VIRTUAL_SCROLL_IDENTIFIER,
  USE_STATIC_STATEMENT
} from '../utils/consts/ArkuiConstants';
import { ES_VALUE } from '../utils/consts/ESObject';
import type { IncrementDecrementNodeInfo } from '../utils/consts/InteropAPI';
import {
  GET_PROPERTY,
  SET_PROPERTY,
  ARE_EQUAL,
  ARE_STRICTLY_EQUAL,
  WRAP,
  INSTANTIATE,
  TO_NUMBER,
  TO_PROMISE,
  INVOKE,
  INVOKE_METHOD,
  LENGTH,
  IS_INSTANCE_OF
} from '../utils/consts/InteropAPI';
import path from 'node:path';
import { isStdLibrarySymbol } from '../utils/functions/IsStdLibrary';
import { propertyAccessReplacements, identifierReplacements } from '../utils/consts/DeprecatedApi';
import { ARKTS_COLLECTIONS_MODULE, BIT_VECTOR } from '../utils/consts/CollectionsAPI';

const UNDEFINED_NAME = 'undefined';

const LINE_FEED = '\n';
const CARRIAGE_RETURN_LINE_FEED = '\r\n';

const NEW_LINE_SEARCH_REGEX = /\r\n|\n|\r/;

const GENERATED_OBJECT_LITERAL_INTERFACE_NAME = 'GeneratedObjectLiteralInterface_';
const GENERATED_OBJECT_LITERAL_INTERFACE_TRESHOLD = 1000;

const GENERATED_OBJECT_LITERAL_CLASS_NAME = 'GeneratedObjectLiteralClass_';
const GENERATED_OBJECT_LITERAL_CLASS_TRESHOLD = 1000;

const GENERATED_OBJECT_LITERAL_INIT_INTERFACE_NAME = 'GeneratedObjectLiteralInitInterface_';
const GENERATED_OBJECT_LITERAL_INIT_INTERFACE_TRESHOLD = 1000;

const GENERATED_TYPE_LITERAL_INTERFACE_NAME = 'GeneratedTypeLiteralInterface_';
const GENERATED_TYPE_LITERAL_INTERFACE_TRESHOLD = 1000;

const GENERATED_DESTRUCT_OBJECT_NAME = 'GeneratedDestructObj_';
const GENERATED_DESTRUCT_OBJECT_TRESHOLD = 1000;

const GENERATED_DESTRUCT_ARRAY_NAME = 'GeneratedDestructArray_';
const GENERATED_DESTRUCT_ARRAY_TRESHOLD = 1000;

const GENERATED_TMP_VARIABLE_NAME = 'tmp_';
const GENERATED_TMP_VARIABLE_TRESHOLD = 1000;

const SPECIAL_LIB_NAME = 'specialAutofixLib';

const OBJECT_LITERAL_CLASS_CONSTRUCTOR_PARAM_NAME = 'init';

const METHOD_KEYS = 'keys';

interface CreateClassPropertyForObjectLiteralParams {
  prop: ts.PropertyAssignment | ts.ShorthandPropertyAssignment;
  enclosingStmt: ts.Node;
  classFields: ts.PropertyDeclaration[];
  ctorBodyStmts: ts.Statement[];
  ctorInitProps: ts.PropertyAssignment[];
}

export interface Autofix {
  replacementText: string;
  start: number;
  end: number;
  line?: number;
  column?: number;
  endLine?: number;
  endColumn?: number;
}

export class Autofixer {
  private readonly printer: ts.Printer;

  private readonly nonCommentPrinter: ts.Printer;

  private readonly typeLiteralInterfaceNameGenerator = new NameGenerator(
    GENERATED_TYPE_LITERAL_INTERFACE_NAME,
    GENERATED_TYPE_LITERAL_INTERFACE_TRESHOLD
  );

  private readonly destructObjNameGenerator = new NameGenerator(
    GENERATED_DESTRUCT_OBJECT_NAME,
    GENERATED_DESTRUCT_OBJECT_TRESHOLD
  );

  private readonly destructArrayNameGenerator = new NameGenerator(
    GENERATED_DESTRUCT_ARRAY_NAME,
    GENERATED_DESTRUCT_ARRAY_TRESHOLD
  );

  private readonly objectLiteralInterfaceNameGenerator = new NameGenerator(
    GENERATED_OBJECT_LITERAL_INTERFACE_NAME,
    GENERATED_OBJECT_LITERAL_INTERFACE_TRESHOLD
  );

  private readonly objectLiteralClassNameGenerator = new NameGenerator(
    GENERATED_OBJECT_LITERAL_CLASS_NAME,
    GENERATED_OBJECT_LITERAL_CLASS_TRESHOLD
  );

  private readonly objectLiteralInitInterfaceNameGenerator = new NameGenerator(
    GENERATED_OBJECT_LITERAL_INIT_INTERFACE_NAME,
    GENERATED_OBJECT_LITERAL_INIT_INTERFACE_TRESHOLD
  );

  private readonly tmpVariableNameGenerator = new NameGenerator(
    GENERATED_TMP_VARIABLE_NAME,
    GENERATED_TMP_VARIABLE_TRESHOLD
  );

  private readonly lastImportEndMap = new Map<string, number>();

  private readonly symbolCache: SymbolCache;

  private readonly renameSymbolAsIdentifierCache = new Map<ts.Symbol, Autofix[] | undefined>();

  private readonly enumMergingCache = new Map<ts.Symbol, Autofix[] | undefined>();

  private readonly privateIdentifierCache = new Map<ts.Symbol, Autofix[] | undefined>();

  private readonly sendableDecoratorCache = new Map<ts.Declaration, Autofix[] | undefined>();

  private readonly newLine: string;

  constructor(
    private readonly typeChecker: ts.TypeChecker,
    private readonly utils: TsUtils,
    readonly sourceFile: ts.SourceFile,
    readonly cancellationToken?: ts.CancellationToken
  ) {
    this.symbolCache = new SymbolCache(this.typeChecker, this.utils, sourceFile, cancellationToken);
    this.newLine = Autofixer.getNewLineCharacterFromSrcFile(sourceFile);

    const tsNewLineKind =
      this.newLine === CARRIAGE_RETURN_LINE_FEED ? ts.NewLineKind.CarriageReturnLineFeed : ts.NewLineKind.LineFeed;
    this.printer = ts.createPrinter({
      omitTrailingSemicolon: false,
      removeComments: false,
      newLine: tsNewLineKind
    });
    this.nonCommentPrinter = ts.createPrinter({
      omitTrailingSemicolon: false,
      removeComments: true,
      newLine: tsNewLineKind
    });
  }

  private static getNewLineCharacterFromSrcFile(srcFile: ts.SourceFile): string {
    const match = srcFile.text.match(NEW_LINE_SEARCH_REGEX);
    return match ? match[0] : LINE_FEED;
  }

  private getNewLine(srcFile?: ts.SourceFile): string {
    return srcFile ? Autofixer.getNewLineCharacterFromSrcFile(srcFile) : this.newLine;
  }

  /**
   * Generates the text representation for destructuring elements in an object.
   * @param variableDeclarationMap - Map of property names to variable names.
   * @param newObjectName - Name of the new object to destructure.
   * @param declarationFlags - Flags for the variable declaration.
   * @param printer - TypeScript printer instance.
   * @param sourceFile - Source file from which the nodes are taken.
   * @returns The generated destructuring text.
   */
  private genDestructElementTextForObjDecls(
    variableDeclarationMap: Map<string, string>,
    newObjectName: string,
    declarationFlags: ts.NodeFlags,
    printer: ts.Printer,
    sourceFile: ts.SourceFile
  ): string {
    let destructElementText: string = '';

    variableDeclarationMap.forEach((propertyName, variableName) => {
      // Create a property access expression for the new object
      const propertyAccessExpr = ts.factory.createPropertyAccessExpression(
        ts.factory.createIdentifier(newObjectName),
        ts.factory.createIdentifier(variableName)
      );

      // Create a variable declaration with the property access expression
      const variableDecl = ts.factory.createVariableDeclaration(
        ts.factory.createIdentifier(propertyName),
        undefined,
        undefined,
        propertyAccessExpr
      );

      // Create a variable statement for the variable declaration
      const variableStatement = ts.factory.createVariableStatement(
        undefined,
        ts.factory.createVariableDeclarationList([variableDecl], declarationFlags)
      );

      // Print the variable statement to text and append it
      const text = printer.printNode(ts.EmitHint.Unspecified, variableStatement, sourceFile);
      destructElementText += text + this.getNewLine();
    });

    return destructElementText;
  }

  /**
   * Creates autofix suggestions for destructuring assignment.
   * @param variableDeclaration - The variable declaration to fix.
   * @param newObjectName - Name of the new object to use for destructuring.
   * @param destructElementText - Generated text for destructuring elements.
   * @returns Array of autofix suggestions or undefined.
   */
  private genAutofixForObjDecls(
    variableDeclaration: ts.VariableDeclaration,
    newObjectName: string | undefined,
    destructElementText: string,
    isIdentifier: boolean
  ): Autofix[] | undefined {
    let variableNameReplaceText: Autofix;
    let destructElementReplaceText: Autofix;
    let autofix: Autofix[] | undefined = [];

    // Check if the initializer is a identifier
    if (isIdentifier) {
      destructElementReplaceText = {
        replacementText: destructElementText,
        start: variableDeclaration.parent.getStart(),
        end: variableDeclaration.parent.parent.getEnd()
      };
      autofix = [destructElementReplaceText];
    } else {
      // Create autofix suggestions for both variable name and destructuring
      variableNameReplaceText = {
        replacementText: newObjectName as string,
        start: variableDeclaration.name.getStart(),
        end: variableDeclaration.name.getEnd()
      };
      destructElementReplaceText = {
        replacementText: this.getNewLine() + destructElementText,
        start: variableDeclaration.parent.parent.getEnd(),
        end: variableDeclaration.parent.parent.getEnd()
      };
      autofix = [variableNameReplaceText, destructElementReplaceText];
    }

    return autofix;
  }

  /**
   * Checks if the given variable declaration passes boundary checks for object declarations.
   *
   * @param faultId - The fault ID indicating the type of check to perform (e.g., destructuring parameter).
   * @param variableDeclaration - The variable or parameter declaration to check for boundary conditions.
   * @returns A boolean indicating if the declaration passes the boundary checks.
   */
  private static passBoundaryCheckForObjDecls(variableDeclaration: ts.VariableDeclaration): boolean {
    // Check if the fault ID is for a destructuring parameter or if the declaration has a spread operator
    if (
      TsUtils.destructuringDeclarationHasSpreadOperator(variableDeclaration.name as ts.BindingPattern) ||
      TsUtils.destructuringDeclarationHasDefaultValue(variableDeclaration.name as ts.BindingPattern)
    ) {
      return false;
    }

    // If the initializer is an object literal expression, check its properties
    if (ts.isObjectLiteralExpression(variableDeclaration.initializer as ts.Node)) {
      const len = (variableDeclaration.initializer as ts.ObjectLiteralExpression).properties.length;
      if (len === 0) {
        // Return false if there are no properties
        return false;
      }
    }

    // Return true if all checks are passed
    return true;
  }

  /**
   ** Fixes object binding pattern declarations by generating appropriate autofix suggestions.
   * @param variableDeclaration - The variable declaration to fix.
   * @param faultId - The fault ID indicating the type of check to perform.
   * @returns Array of autofix suggestions or undefined.
   */
  fixObjectBindingPatternDeclarations(variableDeclaration: ts.VariableDeclaration): Autofix[] | undefined {
    if (!Autofixer.passBoundaryCheckForObjDecls(variableDeclaration)) {
      return undefined;
    }
    // Map to hold variable names and their corresponding property names
    const variableDeclarationMap: Map<string, string> = new Map();
    // If the declaration is an object binding pattern, extract names
    if (ts.isObjectBindingPattern(variableDeclaration.name)) {
      variableDeclaration.name.elements.forEach((element) => {
        if (!element.propertyName) {
          variableDeclarationMap.set(element.name.getText(), element.name.getText());
        } else {
          variableDeclarationMap.set((element.propertyName as ts.Identifier).text, element.name.getText());
        }
      });
    }
    const sourceFile = variableDeclaration.getSourceFile();
    let newObjectName: string | undefined;
    const isIdentifier = ts.isIdentifier(variableDeclaration.initializer as ts.Node);
    // If it is identifer, use its text as the new object name; otherwise, generate a unique name
    if (isIdentifier) {
      newObjectName = variableDeclaration.initializer?.getText();
    } else {
      newObjectName = TsUtils.generateUniqueName(this.destructObjNameGenerator, sourceFile);
    }
    if (!newObjectName) {
      return undefined;
    }
    const declarationFlags = ts.getCombinedNodeFlags(variableDeclaration);
    const destructElementText = this.genDestructElementTextForObjDecls(
      variableDeclarationMap,
      newObjectName,
      declarationFlags,
      this.printer,
      sourceFile
    );

    // Generate and return autofix suggestions for the object declarations
    return this.genAutofixForObjDecls(variableDeclaration, newObjectName, destructElementText, isIdentifier);
  }

  /**
   * Generates the text representation for destructuring elements in an array.
   * @param variableNames - Array of variable names corresponding to array elements.
   * @param newArrayName - Name of the new array to destructure.
   * @param declarationFlags - Flags for the variable declaration.
   * @param printer - TypeScript printer instance.
   * @param sourceFile - Source file from which the nodes are taken.
   * @returns The generated destructuring text.
   */
  private genDestructElementTextForArrayDecls(
    variableNames: string[],
    newArrayName: string,
    declarationFlags: ts.NodeFlags,
    printer: ts.Printer,
    sourceFile: ts.SourceFile
  ): string {
    let destructElementText: string = '';
    const length = variableNames.length;

    // Iterate over the array of variable names
    for (let i = 0; i < length; i++) {
      const variableName = variableNames[i];

      // Create an element access expression for the new array
      const elementAccessExpr = ts.factory.createElementAccessExpression(
        ts.factory.createIdentifier(newArrayName),
        ts.factory.createNumericLiteral(i)
      );

      // Create a variable declaration with the element access expression
      const variableDecl = ts.factory.createVariableDeclaration(
        ts.factory.createIdentifier(variableName),
        undefined,
        undefined,
        elementAccessExpr
      );

      // Create a variable statement for the variable declaration
      const variableStatement = ts.factory.createVariableStatement(
        undefined,
        ts.factory.createVariableDeclarationList([variableDecl], declarationFlags)
      );

      // Print the variable statement to text and append it
      const text = printer.printNode(ts.EmitHint.Unspecified, variableStatement, sourceFile);
      destructElementText += text + this.getNewLine();
    }

    return destructElementText;
  }

  /**
   * Creates autofix suggestions for array destructuring assignment.
   * @param variableDeclaration - The variable declaration to fix.
   * @param newArrayName - Name of the new array to use for destructuring.
   * @param destructElementText - Generated text for destructuring elements.
   * @returns Array of autofix suggestions.
   */
  private genAutofixForArrayDecls(
    variableDeclaration: ts.VariableDeclaration,
    newArrayName: string | undefined,
    destructElementText: string,
    isIdentifierOrElementAccess: boolean
  ): Autofix[] {
    let variableNameReplaceText: Autofix;
    let destructElementReplaceText: Autofix;
    let autofix: Autofix[] = [];

    // Check if the initializer is an identifier or element access expression
    if (isIdentifierOrElementAccess) {
      destructElementReplaceText = {
        replacementText: destructElementText,
        start: variableDeclaration.parent.getStart(),
        end: variableDeclaration.parent.parent.getEnd()
      };
      autofix = [destructElementReplaceText];
    } else {
      // Create autofix suggestions for both variable name and destructuring
      variableNameReplaceText = {
        replacementText: newArrayName as string,
        start: variableDeclaration.name.getStart(),
        end: variableDeclaration.name.getEnd()
      };
      destructElementReplaceText = {
        replacementText: this.getNewLine() + destructElementText,
        start: variableDeclaration.parent.parent.getEnd(),
        end: variableDeclaration.parent.parent.getEnd()
      };
      autofix = [variableNameReplaceText, destructElementReplaceText];
    }

    return autofix;
  }

  /**
   * Checks if the given variable declaration passes boundary checks for array or tuple declarations.
   *
   * @param variableDeclaration - A variable declaration or parameter declaration to be checked.
   * @param isArrayOrTuple - A boolean indicating whether the declaration is an array or tuple.
   * @returns A boolean indicating if the declaration passes the boundary checks.
   */
  private static passBoundaryCheckForArrayDecls(
    variableDeclaration: ts.VariableDeclaration,
    isArrayOrTuple: boolean
  ): boolean {
    // If it's not an array/tuple or the declaration has a spread operator in destructuring
    if (
      !isArrayOrTuple ||
      TsUtils.destructuringDeclarationHasSpreadOperator(variableDeclaration.name as ts.BindingPattern) ||
      TsUtils.destructuringDeclarationHasDefaultValue(variableDeclaration.name as ts.BindingPattern)
    ) {
      // Return false if it fails the boundary check
      return false;
    }

    // Check if the array binding pattern has any empty elements
    if (TsUtils.checkArrayBindingHasEmptyElement(variableDeclaration.name as ts.ArrayBindingPattern)) {
      // Return false if it contains empty elements
      return false;
    }

    // Check if the initializer has the same dimension as expected
    if (!TsUtils.isSameDimension(variableDeclaration.initializer as ts.Node)) {
      return false;
    }

    // Return true if all checks are passed
    return true;
  }

  /**
   * Fixes array binding pattern declarations by generating appropriate autofix suggestions.
   *
   * @param variableDeclaration - The variable declaration to fix.
   * @param isArrayOrTuple - Flag indicating if the declaration is for an array or tuple.
   * @returns Array of autofix suggestions or undefined.
   */
  fixArrayBindingPatternDeclarations(
    variableDeclaration: ts.VariableDeclaration,
    isArrayOrTuple: boolean
  ): Autofix[] | undefined {
    if (!Autofixer.passBoundaryCheckForArrayDecls(variableDeclaration, isArrayOrTuple)) {
      return undefined;
    }
    const variableNames: string[] = [];
    // If the declaration is an array binding pattern, extract variable names
    if (ts.isArrayBindingPattern(variableDeclaration.name)) {
      variableDeclaration.name.elements.forEach((element) => {
        variableNames.push(element.getText());
      });
    }

    const sourceFile = variableDeclaration.getSourceFile();
    let newArrayName: string | undefined = '';
    // Check if the initializer is either an identifier or an element access expression
    const isIdentifierOrElementAccess =
      ts.isIdentifier(variableDeclaration.initializer as ts.Node) ||
      ts.isElementAccessExpression(variableDeclaration.initializer as ts.Node);
    // If it is, use its text as the new array name; otherwise, generate a unique name
    if (isIdentifierOrElementAccess) {
      newArrayName = variableDeclaration.initializer?.getText();
    } else {
      newArrayName = TsUtils.generateUniqueName(this.destructArrayNameGenerator, sourceFile);
    }
    if (!newArrayName) {
      return undefined;
    }

    // Get the combined node flags for the variable declaration
    const declarationFlags = ts.getCombinedNodeFlags(variableDeclaration);
    // Generate the destructuring element text for the array declaration
    const destructElementText = this.genDestructElementTextForArrayDecls(
      variableNames,
      newArrayName,
      declarationFlags,
      this.printer,
      sourceFile
    );

    // Generate and return autofix suggestions for the array declarations
    return this.genAutofixForArrayDecls(
      variableDeclaration,
      newArrayName,
      destructElementText,
      isIdentifierOrElementAccess
    );
  }

  /**
   * Generates the text representation for destructuring assignments in an array.
   * @param variableNames - Array of variable names corresponding to array elements.
   * @param newArrayName - Name of the new array to use for destructuring.
   * @param printer - TypeScript printer instance.
   * @param sourceFile - Source file from which the nodes are taken.
   * @returns The generated destructuring assignment text.
   */
  private genDestructElementTextForArrayAssignment(
    variableNames: string[],
    newArrayName: string | undefined,
    printer: ts.Printer,
    sourceFile: ts.SourceFile
  ): string {
    let destructElementText: string = '';
    const length = variableNames.length;

    // Iterate over the array of variable names
    for (let i = 0; i < length; i++) {
      const variableName = variableNames[i];

      // Create an element access expression for the new array
      const elementAccessExpr = ts.factory.createElementAccessExpression(
        ts.factory.createIdentifier(newArrayName as string),
        ts.factory.createNumericLiteral(i)
      );

      // Create a binary expression to assign the array element to the variable
      const assignmentExpr = ts.factory.createBinaryExpression(
        ts.factory.createIdentifier(variableName),
        ts.factory.createToken(ts.SyntaxKind.EqualsToken),
        elementAccessExpr
      );

      // Create an expression statement for the assignment expression
      const expressionStatement = ts.factory.createExpressionStatement(assignmentExpr);

      // Print the expression statement to text and append it
      const text = printer.printNode(ts.EmitHint.Unspecified, expressionStatement, sourceFile);
      destructElementText += text + this.getNewLine();
    }

    return destructElementText;
  }

  /**
   * Creates autofix suggestions for array destructuring assignment.
   * @param assignmentExpr - The binary expression for the assignment.
   * @param newArrayName - Name of the new array to use for destructuring.
   * @param destructElementText - Generated text for destructuring assignments.
   * @returns Array of autofix suggestions.
   */
  private genAutofixForArrayAssignment(
    assignmentExpr: ts.BinaryExpression,
    newArrayName: string | undefined,
    destructElementText: string,
    isIdentifierOrElementAccess: boolean
  ): Autofix[] {
    let arrayNameReplaceText: Autofix;
    let destructElementReplaceText: Autofix;
    let autofix: Autofix[] = [];

    // Check if the right side of the assignment is an identifier or element access expression
    if (isIdentifierOrElementAccess) {
      destructElementReplaceText = {
        replacementText: destructElementText,
        start: assignmentExpr.parent.getStart(),
        end: assignmentExpr.parent.getEnd()
      };
      autofix = [destructElementReplaceText];
    } else {
      // Create autofix suggestions for both array name and destructuring assignments
      const keywordsLet = 'let ';
      arrayNameReplaceText = {
        replacementText: keywordsLet + newArrayName,
        start: assignmentExpr.left.getStart(),
        end: assignmentExpr.left.getEnd()
      };
      destructElementReplaceText = {
        replacementText: this.getNewLine() + destructElementText,
        start: assignmentExpr.parent.getEnd(),
        end: assignmentExpr.parent.getEnd()
      };
      autofix = [arrayNameReplaceText, destructElementReplaceText];
    }

    return autofix;
  }

  /**
   * Checks if the given assignment expression passes boundary checks for array assignments.
   *
   * @param assignmentExpr - The assignment expression to check (a binary expression).
   * @param isArrayOrTuple - A boolean indicating if the assignment is for an array or tuple.
   * @returns A boolean indicating if the assignment passes the boundary checks.
   */
  private static passBoundaryCheckForArrayAssignment(
    assignmentExpr: ts.BinaryExpression,
    isArrayOrTuple: boolean
  ): boolean {
    // Return false if the assignment is not for an array or tuple
    if (!isArrayOrTuple) {
      return false;
    }

    // Check if the left side of the assignment is an array literal expression with a spread operator
    if (TsUtils.destructuringAssignmentHasSpreadOperator(assignmentExpr.left as ts.ArrayLiteralExpression)) {
      return false;
    }

    if (TsUtils.destructuringAssignmentHasDefaultValue(assignmentExpr.left as ts.ArrayLiteralExpression)) {
      return false;
    }

    // Check if the left side of the assignment has an empty element
    if (TsUtils.checkArrayLiteralHasEmptyElement(assignmentExpr.left as ts.ArrayLiteralExpression)) {
      return false;
    }

    // Check if the right side of the assignment has the same dimension as the left side
    if (!TsUtils.isSameDimension(assignmentExpr.right)) {
      return false;
    }

    // Return true if all boundary checks are passed
    return true;
  }

  /**
   * Fixes array binding pattern assignments by generating appropriate autofix suggestions.
   * @param assignmentExpr - The binary expression for the assignment.
   * @param isArrayOrTuple - Flag indicating if the assignment is for an array or tuple.
   * @returns Array of autofix suggestions or undefined.
   */
  fixArrayBindingPatternAssignment(
    assignmentExpr: ts.BinaryExpression,
    isArrayOrTuple: boolean
  ): Autofix[] | undefined {
    if (!Autofixer.passBoundaryCheckForArrayAssignment(assignmentExpr, isArrayOrTuple)) {
      return undefined;
    }
    // Collect variable names from array literal expression
    const variableNames: string[] = [];
    if (ts.isArrayLiteralExpression(assignmentExpr.left)) {
      assignmentExpr.left.elements.forEach((element) => {
        variableNames.push(element.getText());
      });
    }

    const sourceFile = assignmentExpr.getSourceFile();
    let newArrayName: string | undefined = '';
    const isIdentifierOrElementAccess =
      ts.isIdentifier(assignmentExpr.right) || ts.isElementAccessExpression(assignmentExpr.right as ts.Node);
    if (isIdentifierOrElementAccess) {
      newArrayName = assignmentExpr.right.getText();
    } else {
      newArrayName = TsUtils.generateUniqueName(this.destructArrayNameGenerator, sourceFile);
    }
    if (!newArrayName) {
      return undefined;
    }

    // Generate the text for destructuring assignments
    const destructElementText = this.genDestructElementTextForArrayAssignment(
      variableNames,
      newArrayName,
      this.printer,
      sourceFile
    );

    return this.genAutofixForArrayAssignment(
      assignmentExpr,
      newArrayName,
      destructElementText,
      isIdentifierOrElementAccess
    );
  }

  /**
   * Creates a mapping of variable declarations and needParentheses for object properties based on the provided binary expression.
   * @param binaryExpr - The binary expression containing the object literal.
   * @returns An object containing the variable declaration map and needParentheses indicating if property initializers are object literals.
   */
  private static genTsVarDeclMapAndFlags(binaryExpr: ts.BinaryExpression): {
    tsVarDeclMap: Map<string, string>;
    needParentheses: boolean[];
  } {
    const tsVarDeclMap: Map<string, string> = new Map();
    const needParentheses: boolean[] = [];

    // Check if the left side of the binary expression is an object literal
    if (ts.isObjectLiteralExpression(binaryExpr.left)) {
      binaryExpr.left.properties.forEach((property) => {
        // Handle property assignments with initializer
        if (ts.isPropertyAssignment(property)) {
          tsVarDeclMap.set(property.name?.getText(), property.initializer.getText());
          needParentheses.push(ts.isObjectLiteralExpression(property.initializer));
        } else if (ts.isShorthandPropertyAssignment(property)) {
          tsVarDeclMap.set(property.name?.getText(), property.name.getText());
          needParentheses.push(false);
        }
      });
    }

    return { tsVarDeclMap, needParentheses };
  }

  /**
   * Generates the text for destructuring assignments based on the variable declaration map and needParentheses.
   * @param tsVarDeclMap - Map of variable names to property names.
   * @param needParentheses - Array of needParentheses indicating if property initializers are object literals.
   * @param newObjName - The name of the new object to use for destructuring.
   * @param binaryExpr - The binary expression representing the destructuring.
   * @param printer - TypeScript printer instance for printing nodes.
   * @returns The generated text for destructuring assignments.
   */
  private genDestructElementTextForObjAssignment(
    tsVarDeclMap: Map<string, string>,
    needParentheses: boolean[],
    newObjName: string,
    binaryExpr: ts.BinaryExpression,
    printer: ts.Printer
  ): string {
    let destructElementText: string = '';
    let index: number = 0;

    // Iterate through the variable declaration map to generate destructuring assignments
    tsVarDeclMap.forEach((propertyName, variableName) => {
      // Create property access expression for the new object
      const propAccessExpr = ts.factory.createPropertyAccessExpression(
        ts.factory.createIdentifier(newObjName),
        ts.factory.createIdentifier(variableName)
      );
      // Create binary expression for assignment
      const assignmentExpr = ts.factory.createBinaryExpression(
        ts.factory.createIdentifier(propertyName),
        ts.factory.createToken(ts.SyntaxKind.EqualsToken),
        propAccessExpr
      );
      // Create statement for the assignment expression, with or without parentheses based on the flag
      const statement = needParentheses[index] ?
        ts.factory.createExpressionStatement(ts.factory.createParenthesizedExpression(assignmentExpr)) :
        ts.factory.createExpressionStatement(assignmentExpr);

      // Append the generated text for the destructuring assignment
      destructElementText +=
        printer.printNode(ts.EmitHint.Unspecified, statement, binaryExpr.getSourceFile()) + this.getNewLine();

      index++;
    });

    return destructElementText;
  }

  /**
   * Creates the replacement text for the variable declaration name.
   * @param binaryExpr - The binary expression containing the object literal or call expression.
   * @param newObjName - The new object name to be used in the replacement.
   * @param printer - TypeScript printer instance for printing nodes.
   * @returns The replacement text for the variable declaration name.
   */
  private static genDeclNameReplaceTextForObjAssignment(
    binaryExpr: ts.BinaryExpression,
    newObjName: string,
    printer: ts.Printer
  ): string {
    let declNameReplaceText = '';

    // create variableDeclList and get declNameReplaceText text
    const variableDecl = ts.factory.createVariableDeclaration(
      ts.factory.createIdentifier(newObjName),
      undefined,
      undefined,
      binaryExpr.right
    );
    const variableDeclList = ts.factory.createVariableDeclarationList([variableDecl], ts.NodeFlags.Let);
    declNameReplaceText = printer.printNode(ts.EmitHint.Unspecified, variableDeclList, binaryExpr.getSourceFile());

    return declNameReplaceText;
  }

  /**
   * Creates autofix suggestions for object literal destructuring assignments.
   * @param binaryExpr - The binary expression containing the destructuring assignment.
   * @param declNameReplaceText - Replacement text for the variable declaration name.
   * @param destructElementText - Generated text for destructuring assignments.
   * @returns Array of autofix suggestions or undefined if no fixes are needed.
   */
  private createAutofixForObjAssignment(
    binaryExpr: ts.BinaryExpression,
    declNameReplaceText: string,
    destructElementText: string,
    isIdentifier: boolean
  ): Autofix[] | undefined {
    let declNameReplaceTextAutofix: Autofix;
    let destructElementReplaceTextAutofix: Autofix;
    let autofix: Autofix[] | undefined;

    // Check if the right side of the binary expression is identifer
    if (isIdentifier) {
      destructElementReplaceTextAutofix = {
        replacementText: destructElementText,
        start: binaryExpr.parent.parent.getStart(),
        end: binaryExpr.parent.parent.getEnd()
      };
      autofix = [destructElementReplaceTextAutofix];
    } else {
      declNameReplaceTextAutofix = {
        replacementText: declNameReplaceText,
        start: binaryExpr.parent.getStart(),
        end: binaryExpr.parent.getEnd()
      };
      destructElementReplaceTextAutofix = {
        replacementText: this.getNewLine() + destructElementText,
        start: binaryExpr.parent.parent.getEnd(),
        end: binaryExpr.parent.parent.getEnd()
      };
      autofix = [declNameReplaceTextAutofix, destructElementReplaceTextAutofix];
    }

    return autofix;
  }

  /**
   * Checks if the given assignment expression passes boundary checks for object assignments.
   *
   * @param binaryExpr - The binary expression representing the assignment to check.
   * @returns A boolean indicating if the assignment passes the boundary checks.
   */
  private static passBoundaryCheckForObjAssignment(binaryExpr: ts.BinaryExpression): boolean {
    // Check for spread operator in destructuring assignment on the left side
    if (TsUtils.destructuringAssignmentHasSpreadOperator(binaryExpr.left as ts.ObjectLiteralExpression)) {
      return false;
    }

    if (TsUtils.destructuringAssignmentHasDefaultValue(binaryExpr.left as ts.ObjectLiteralExpression)) {
      return false;
    }

    // Check if the right side is an object literal expression with no properties
    if (ts.isObjectLiteralExpression(binaryExpr.right) && binaryExpr.right.properties.length === 0) {
      return false;
    }

    // Return true if all boundary checks are passed
    return true;
  }

  /**
   * Fixes object literal expression destructuring assignments by generating autofix suggestions.
   * @param binaryExpr - The binary expression representing the destructuring assignment.
   * @returns Array of autofix suggestions or undefined if no fixes are needed.
   */
  fixObjectLiteralExpressionDestructAssignment(binaryExpr: ts.BinaryExpression): Autofix[] | undefined {
    if (!Autofixer.passBoundaryCheckForObjAssignment(binaryExpr)) {
      return undefined;
    }
    // Create a mapping of variable declarations and needParentheses
    const { tsVarDeclMap, needParentheses } = Autofixer.genTsVarDeclMapAndFlags(binaryExpr);

    const sourceFile = binaryExpr.getSourceFile();
    let newObjName: string | undefined = '';
    // Generate a unique name if right expression is not identifer
    const isIdentifier = ts.isIdentifier(binaryExpr.right);
    if (isIdentifier) {
      newObjName = binaryExpr.right?.getText();
    } else {
      newObjName = TsUtils.generateUniqueName(this.destructObjNameGenerator, sourceFile);
    }
    if (!newObjName) {
      return undefined;
    }
    // Create the text for destructuring elements
    const destructElementText = this.genDestructElementTextForObjAssignment(
      tsVarDeclMap,
      needParentheses,
      newObjName,
      binaryExpr,
      this.printer
    );

    // Create the replacement text for the variable declaration name
    const declNameReplaceText = Autofixer.genDeclNameReplaceTextForObjAssignment(binaryExpr, newObjName, this.printer);

    // Generate autofix suggestions
    return this.createAutofixForObjAssignment(binaryExpr, declNameReplaceText, destructElementText, isIdentifier);
  }

  fixLiteralAsPropertyNamePropertyAssignment(node: ts.PropertyAssignment): Autofix[] | undefined {
    const contextualType = this.typeChecker.getContextualType(node.parent);
    if (contextualType === undefined) {
      return undefined;
    }

    const symbol = this.utils.getPropertySymbol(contextualType, node);
    if (symbol === undefined) {
      return undefined;
    }

    return this.renameSymbolAsIdentifier(symbol);
  }

  fixLiteralAsPropertyNamePropertyName(node: ts.PropertyName, enumMember?: ts.EnumMember): Autofix[] | undefined {
    const symbol = this.typeChecker.getSymbolAtLocation(node);
    if (symbol === undefined) {
      return undefined;
    }

    return this.renameSymbolAsIdentifier(symbol, enumMember);
  }

  renameAsObjectElementAccessExpression(node: ts.ElementAccessExpression): Autofix[] | undefined {
    const parenExpr = ts.isParenthesizedExpression(node.expression) ? node.expression : undefined;
    const asExpr = parenExpr && ts.isAsExpression(parenExpr.expression) ? parenExpr.expression : undefined;
    if (!asExpr) {
      return undefined;
    }

    const argument = node.argumentExpression;
    const propertyName = ts.isStringLiteral(argument) ? argument.text : undefined;
    if (!propertyName) {
      return undefined;
    }

    const propertyChain: string[] = [propertyName];
    let current: ts.Node = node;

    while (current.parent && ts.isElementAccessExpression(current.parent)) {
      const parentArg = current.parent.argumentExpression;
      if (ts.isStringLiteral(parentArg)) {
        propertyChain.push(parentArg.text);
      }
      current = current.parent;
    }
    const realObj = asExpr.expression;
    const type = this.typeChecker.getTypeAtLocation(realObj);

    let currentType = type;
    for (const prop of propertyChain) {
      const property = this.typeChecker.getPropertyOfType(currentType, prop);
      if (!property) {
        return undefined;
      }
      currentType = this.typeChecker.getTypeOfSymbolAtLocation(property, node);
    }
    const replacementText = realObj.getText() + '.' + propertyChain.join('.');

    return [
      {
        replacementText,
        start: node.getStart(),
        end: current.getEnd()
      }
    ];
  }

  fixPropertyAccessByIndex(node: ts.ElementAccessExpression): Autofix[] | undefined {
    if (ts.isParenthesizedExpression(node.expression) && ts.isAsExpression(node.expression.expression)) {
      const assertedType = this.typeChecker.getTypeAtLocation(node.expression.expression.type);
      if (this.typeChecker.typeToString(assertedType) === 'object') {
        return this.renameAsObjectElementAccessExpression(node);
      }
    }

    const symbol = this.typeChecker.getSymbolAtLocation(node.argumentExpression);
    if (symbol === undefined) {
      return undefined;
    }

    return this.renameSymbolAsIdentifier(symbol);
  }

  private renameSymbolAsIdentifier(symbol: ts.Symbol, enumMember?: ts.EnumMember): Autofix[] | undefined {
    if (this.renameSymbolAsIdentifierCache.has(symbol)) {
      return this.renameSymbolAsIdentifierCache.get(symbol);
    }
    if (
      !TsUtils.isPropertyOfInternalClassOrInterface(symbol) &&
      !(enumMember?.parent && ts.isEnumDeclaration(enumMember.parent))
    ) {
      this.renameSymbolAsIdentifierCache.set(symbol, undefined);
      return undefined;
    }

    const newName = this.utils.findIdentifierNameForSymbol(symbol, enumMember);
    if (newName === undefined) {
      this.renameSymbolAsIdentifierCache.set(symbol, undefined);
      return undefined;
    }

    let result: Autofix[] | undefined = [];
    this.symbolCache.getReferences(symbol).forEach((node) => {
      if (result === undefined) {
        return;
      }

      let autofix: Autofix[] | undefined;
      if (
        ts.isPropertyDeclaration(node) ||
        ts.isPropertyAssignment(node) ||
        ts.isPropertySignature(node) ||
        ts.isEnumMember(node)
      ) {
        autofix = Autofixer.renamePropertyName(node.name, newName);
      } else if (ts.isElementAccessExpression(node)) {
        autofix = Autofixer.renameElementAccessExpression(node, newName);
      }

      if (autofix === undefined) {
        result = undefined;
        return;
      }

      result.push(...autofix);
    });
    if (!result?.length) {
      result = undefined;
    }

    this.renameSymbolAsIdentifierCache.set(symbol, result);
    return result;
  }

  addDefaultModuleToPath(parts: string[], importDeclNode: ts.ImportDeclaration): Autofix[] | undefined {
    void this;
    const moduleSpecifier = importDeclNode.moduleSpecifier;

    /*
     * check the current file's path
     * get the parent directory name of the "src" directory
     */

    const moduleName = TsUtils.getModuleName(importDeclNode);
    const newPathParts = [moduleName ?? DEFAULT_MODULE_NAME, SRC_AND_MAIN, ...parts];
    const newPath = newPathParts.join(PATH_SEPARATOR);
    const newPathString = '\'' + newPath + '\'';

    return [{ start: moduleSpecifier.getStart(), end: moduleSpecifier.getEnd(), replacementText: newPathString }];
  }

  fixImportPath(parts: string[], index: number, importDeclNode: ts.ImportDeclaration): Autofix[] | undefined {
    void this;
    const moduleSpecifier = importDeclNode.moduleSpecifier;

    const beforeEts = parts.slice(0, index);
    const afterEts = parts.slice(index, parts.length);
    const newPathParts = [...beforeEts, SRC_AND_MAIN, ...afterEts];

    const newPath = newPathParts.join(PATH_SEPARATOR);
    const newPathString = '\'' + newPath + '\'';

    return [{ start: moduleSpecifier.getStart(), end: moduleSpecifier.getEnd(), replacementText: newPathString }];
  }

  private static renamePropertyName(node: ts.PropertyName, newName: string): Autofix[] | undefined {
    if (ts.isComputedPropertyName(node)) {
      return undefined;
    }

    if (ts.isMemberName(node)) {
      if (ts.idText(node) !== newName) {
        return undefined;
      }

      return [];
    }

    return [{ replacementText: newName, start: node.getStart(), end: node.getEnd() }];
  }

  private static renameElementAccessExpression(
    node: ts.ElementAccessExpression,
    newName: string
  ): Autofix[] | undefined {
    const argExprKind = node.argumentExpression.kind;
    if (argExprKind !== ts.SyntaxKind.NumericLiteral && argExprKind !== ts.SyntaxKind.StringLiteral) {
      return undefined;
    }

    return [
      {
        replacementText: node.expression.getText() + '.' + newName,
        start: node.getStart(),
        end: node.getEnd()
      }
    ];
  }

  fixFunctionExpression(
    funcExpr: ts.FunctionExpression,
    // eslint-disable-next-line default-param-last
    retType: ts.TypeNode | undefined = funcExpr.type,
    modifiers: readonly ts.Modifier[] | undefined,
    isGenerator: boolean,
    hasUnfixableReturnType: boolean
  ): Autofix[] | undefined {
    const hasThisKeyword = scopeContainsThis(funcExpr.body);
    const isCalledRecursively = this.utils.isFunctionCalledRecursively(funcExpr);
    if (isGenerator || hasThisKeyword || isCalledRecursively || hasUnfixableReturnType) {
      return undefined;
    }

    let arrowFunc: ts.Expression = ts.factory.createArrowFunction(
      modifiers,
      funcExpr.typeParameters,
      funcExpr.parameters,
      retType,
      ts.factory.createToken(ts.SyntaxKind.EqualsGreaterThanToken),
      funcExpr.body
    );
    if (Autofixer.needsParentheses(funcExpr)) {
      arrowFunc = ts.factory.createParenthesizedExpression(arrowFunc);
    }
    const text = this.printer.printNode(ts.EmitHint.Unspecified, arrowFunc, funcExpr.getSourceFile());
    return [{ start: funcExpr.getStart(), end: funcExpr.getEnd(), replacementText: text }];
  }

  private static isNodeInWhileOrIf(node: ts.Node): boolean {
    return (
      node.kind === ts.SyntaxKind.WhileStatement ||
      node.kind === ts.SyntaxKind.DoStatement ||
      node.kind === ts.SyntaxKind.IfStatement
    );
  }

  private static isNodeInForLoop(node: ts.Node): boolean {
    return (
      node.kind === ts.SyntaxKind.ForInStatement ||
      node.kind === ts.SyntaxKind.ForOfStatement ||
      node.kind === ts.SyntaxKind.ForStatement
    );
  }

  private static parentInFor(node: ts.Node): ts.Node | undefined {
    let parentNode = node.parent;
    while (parentNode) {
      if (Autofixer.isNodeInForLoop(parentNode)) {
        return parentNode;
      }
      parentNode = parentNode.parent;
    }
    return undefined;
  }

  private static parentInCaseOrWhile(varDeclList: ts.VariableDeclarationList): boolean {
    let parentNode: ts.Node = varDeclList.parent;
    while (parentNode) {
      if (parentNode.kind === ts.SyntaxKind.CaseClause || Autofixer.isNodeInWhileOrIf(parentNode)) {
        return false;
      }
      parentNode = parentNode.parent;
    }
    return true;
  }

  private static isFunctionLikeDeclarationKind(node: ts.Node): boolean {
    switch (node.kind) {
      case ts.SyntaxKind.FunctionDeclaration:
      case ts.SyntaxKind.MethodDeclaration:
      case ts.SyntaxKind.Constructor:
      case ts.SyntaxKind.GetAccessor:
      case ts.SyntaxKind.SetAccessor:
      case ts.SyntaxKind.FunctionExpression:
      case ts.SyntaxKind.ArrowFunction:
        return true;
      default:
        return false;
    }
  }

  private static findVarScope(node: ts.Node): ts.Node {
    while (node !== undefined) {
      if (node.kind === ts.SyntaxKind.Block || node.kind === ts.SyntaxKind.SourceFile) {
        break;
      }
      // eslint-disable-next-line no-param-reassign
      node = node.parent;
    }
    return node;
  }

  private static varHasScope(node: ts.Node, scope: ts.Node): boolean {
    while (node !== undefined) {
      if (node === scope) {
        return true;
      }
      // eslint-disable-next-line no-param-reassign
      node = node.parent;
    }
    return false;
  }

  private static varInFunctionForScope(node: ts.Node, scope: ts.Node): boolean {
    while (node !== undefined) {
      if (Autofixer.isFunctionLikeDeclarationKind(node)) {
        break;
      }
      // eslint-disable-next-line no-param-reassign
      node = node.parent;
    }
    // node now Function like declaration

    // node need to check that function like declaration is in scope
    if (Autofixer.varHasScope(node, scope)) {
      // var use is in function scope, which is in for scope
      return true;
    }
    return false;
  }

  private static selfDeclared(decl: ts.Node, ident: ts.Node): boolean {
    // Do not check the same node
    if (ident === decl) {
      return false;
    }

    while (ident !== undefined) {
      if (ident.kind === ts.SyntaxKind.VariableDeclaration) {
        const declName = (ident as ts.VariableDeclaration).name;
        if (declName === decl) {
          return true;
        }
      }
      // eslint-disable-next-line no-param-reassign
      ident = ident.parent;
    }
    return false;
  }

  private static analizeTDZ(decl: ts.VariableDeclaration, identifiers: ts.Node[]): boolean {
    for (const ident of identifiers) {
      if (Autofixer.selfDeclared(decl.name, ident)) {
        return false;
      }
      if (ident.pos < decl.pos) {
        return false;
      }
    }
    return true;
  }

  private static analizeScope(decl: ts.VariableDeclaration, identifiers: ts.Node[]): boolean {
    const scope = Autofixer.findVarScope(decl);
    if (scope === undefined) {
      return false;
    } else if (scope.kind === ts.SyntaxKind.Block) {
      for (const ident of identifiers) {
        if (!Autofixer.varHasScope(ident, scope)) {
          return false;
        }
      }
    } else if (scope.kind === ts.SyntaxKind.SourceFile) {
      // Do nothing
    } else {
      // Unreachable, but check it
      return false;
    }
    return true;
  }

  private static analizeFor(decl: ts.VariableDeclaration, identifiers: ts.Node[]): boolean {
    const forNode = Autofixer.parentInFor(decl);
    if (forNode) {
      // analize that var is initialized
      if (forNode.kind === ts.SyntaxKind.ForInStatement || forNode.kind === ts.SyntaxKind.ForOfStatement) {
        const typedForNode = forNode as ts.ForInOrOfStatement;
        const forVarDeclarations = (typedForNode.initializer as ts.VariableDeclarationList).declarations;
        if (forVarDeclarations.length !== 1) {
          return false;
        }
        const forVarDecl = forVarDeclarations[0];

        // our goal to skip declarations in for of/in initializer
        if (forVarDecl !== decl && decl.initializer === undefined) {
          return false;
        }
      } else if (decl.initializer === undefined) {
        return false;
      }

      // analize that var uses are only in function block
      for (const ident of identifiers) {
        if (ident !== decl && !Autofixer.varHasScope(ident, forNode)) {
          return false;
        }
      }

      // analize that var is not in function
      for (const ident of identifiers) {
        if (ident !== decl && Autofixer.varInFunctionForScope(ident, forNode)) {
          return false;
        }
      }
    }
    return true;
  }

  private checkVarDeclarations(varDeclList: ts.VariableDeclarationList): boolean {
    for (const decl of varDeclList.declarations) {
      const symbol = this.typeChecker.getSymbolAtLocation(decl.name);
      if (!symbol) {
        return false;
      }

      const identifiers = this.symbolCache.getReferences(symbol);

      const declLength = symbol.declarations?.length;
      if (!declLength || declLength >= 2) {
        return false;
      }

      // Check for var use in tdz oe self declaration
      if (!Autofixer.analizeTDZ(decl, identifiers)) {
        return false;
      }

      // Has use outside scope of declaration?
      if (!Autofixer.analizeScope(decl, identifiers)) {
        return false;
      }

      // For analisys
      if (!Autofixer.analizeFor(decl, identifiers)) {
        return false;
      }

      if (symbol.getName() === 'let') {
        return false;
      }
    }
    return true;
  }

  private canAutofixNoVar(varDeclList: ts.VariableDeclarationList): boolean {
    if (!Autofixer.parentInCaseOrWhile(varDeclList)) {
      return false;
    }

    if (!this.checkVarDeclarations(varDeclList)) {
      return false;
    }

    return true;
  }

  fixVarDeclaration(node: ts.VariableDeclarationList): Autofix[] | undefined {
    const newNode = ts.factory.createVariableDeclarationList(node.declarations, ts.NodeFlags.Let);
    const text = this.printer.printNode(ts.EmitHint.Unspecified, newNode, node.getSourceFile());
    return this.canAutofixNoVar(node) ?
      [{ start: node.getStart(), end: node.getEnd(), replacementText: text }] :
      undefined;
  }

  private getFixReturnTypeArrowFunction(funcLikeDecl: ts.FunctionLikeDeclaration, typeNode: ts.TypeNode): string {
    if (!funcLikeDecl.body) {
      return '';
    }
    const node = ts.factory.createArrowFunction(
      undefined,
      funcLikeDecl.typeParameters,
      funcLikeDecl.parameters,
      typeNode,
      ts.factory.createToken(ts.SyntaxKind.EqualsGreaterThanToken),
      funcLikeDecl.body
    );
    return this.printer.printNode(ts.EmitHint.Unspecified, node, funcLikeDecl.getSourceFile());
  }

  fixMissingReturnType(funcLikeDecl: ts.FunctionLikeDeclaration, typeNode: ts.TypeNode): Autofix[] {
    if (ts.isArrowFunction(funcLikeDecl)) {
      const text = this.getFixReturnTypeArrowFunction(funcLikeDecl, typeNode);
      const startPos = funcLikeDecl.getStart();
      const endPos = funcLikeDecl.getEnd();

      return [{ start: startPos, end: endPos, replacementText: text }];
    }
    const text = ': ' + this.printer.printNode(ts.EmitHint.Unspecified, typeNode, funcLikeDecl.getSourceFile());
    const pos = Autofixer.getReturnTypePosition(funcLikeDecl);
    return [{ start: pos, end: pos, replacementText: text }];
  }

  dropTypeOnVarDecl(varDecl: ts.VariableDeclaration): Autofix[] {
    const newVarDecl = ts.factory.createVariableDeclaration(varDecl.name, undefined, undefined, undefined);
    const text = this.printer.printNode(ts.EmitHint.Unspecified, newVarDecl, varDecl.getSourceFile());
    return [{ start: varDecl.getStart(), end: varDecl.getEnd(), replacementText: text }];
  }

  fixTypeAssertion(typeAssertion: ts.TypeAssertion): Autofix[] {
    const asExpr = ts.factory.createAsExpression(typeAssertion.expression, typeAssertion.type);
    const text = this.nonCommentPrinter.printNode(ts.EmitHint.Unspecified, asExpr, typeAssertion.getSourceFile());
    return [{ start: typeAssertion.getStart(), end: typeAssertion.getEnd(), replacementText: text }];
  }

  fixCommaOperator(tsNode: ts.Node): Autofix[] {
    const tsExprNode = tsNode as ts.BinaryExpression;
    const text = this.recursiveCommaOperator(tsExprNode);
    return [{ start: tsExprNode.parent.getFullStart(), end: tsExprNode.parent.getEnd(), replacementText: text }];
  }

  private recursiveCommaOperator(tsExprNode: ts.BinaryExpression): string {
    let text = '';
    if (tsExprNode.operatorToken.kind !== ts.SyntaxKind.CommaToken) {
      return tsExprNode.getFullText() + ';';
    }

    if (tsExprNode.left.kind === ts.SyntaxKind.BinaryExpression) {
      text += this.recursiveCommaOperator(tsExprNode.left as ts.BinaryExpression);
      text += this.getNewLine() + tsExprNode.right.getFullText() + ';';
    } else {
      const leftText = tsExprNode.left.getFullText();
      const rightText = tsExprNode.right.getFullText();
      text = leftText + ';' + this.getNewLine() + rightText + ';';
    }

    return text;
  }

  private getEnumMembers(node: ts.Node, enumDeclsInFile: ts.Declaration[], result: Autofix[] | undefined): void {
    if (result === undefined || !ts.isEnumDeclaration(node)) {
      return;
    }

    if (result.length) {
      result.push({ start: node.getStart(), end: node.getEnd(), replacementText: '' });
      return;
    }

    const members: ts.EnumMember[] = [];
    for (const decl of enumDeclsInFile) {
      for (const member of (decl as ts.EnumDeclaration).members) {
        if (
          member.initializer &&
          member.initializer.kind !== ts.SyntaxKind.NumericLiteral &&
          member.initializer.kind !== ts.SyntaxKind.StringLiteral
        ) {
          result = undefined;
          return;
        }
      }
      members.push(...(decl as ts.EnumDeclaration).members);
    }

    const fullEnum = ts.factory.createEnumDeclaration(node.modifiers, node.name, members);
    const fullText = this.printer.printNode(ts.EmitHint.Unspecified, fullEnum, node.getSourceFile());
    result.push({ start: node.getStart(), end: node.getEnd(), replacementText: fullText });
  }

  fixEnumMerging(enumSymbol: ts.Symbol, enumDeclsInFile: ts.Declaration[]): Autofix[] | undefined {
    if (this.enumMergingCache.has(enumSymbol)) {
      return this.enumMergingCache.get(enumSymbol);
    }

    if (enumDeclsInFile.length <= 1) {
      this.enumMergingCache.set(enumSymbol, undefined);
      return undefined;
    }

    let result: Autofix[] | undefined = [];
    this.symbolCache.getReferences(enumSymbol).forEach((node) => {
      this.getEnumMembers(node, enumDeclsInFile, result);
    });
    if (!result?.length) {
      result = undefined;
    }

    this.enumMergingCache.set(enumSymbol, result);
    return result;
  }

  private static getReturnTypePosition(funcLikeDecl: ts.FunctionLikeDeclaration): number {
    if (funcLikeDecl.body) {

      /*
       * Find position of the first node or token that follows parameters.
       * After that, iterate over child nodes in reverse order, until found
       * first closing parenthesis.
       */
      const postParametersPosition = ts.isArrowFunction(funcLikeDecl) ?
        funcLikeDecl.equalsGreaterThanToken.getStart() :
        funcLikeDecl.body.getStart();

      const children = funcLikeDecl.getChildren();
      for (let i = children.length - 1; i >= 0; i--) {
        const child = children[i];
        if (child.kind === ts.SyntaxKind.CloseParenToken && child.getEnd() <= postParametersPosition) {
          return child.getEnd();
        }
      }
    }

    // Shouldn't get here.
    return -1;
  }

  private static needsParentheses(node: ts.FunctionExpression): boolean {
    const parent = node.parent;
    return (
      ts.isPrefixUnaryExpression(parent) ||
      ts.isPostfixUnaryExpression(parent) ||
      ts.isPropertyAccessExpression(parent) ||
      ts.isElementAccessExpression(parent) ||
      ts.isTypeOfExpression(parent) ||
      ts.isVoidExpression(parent) ||
      ts.isAwaitExpression(parent) ||
      ts.isCallExpression(parent) && node === parent.expression ||
      ts.isBinaryExpression(parent) && !isAssignmentOperator(parent.operatorToken)
    );
  }

  fixCtorParameterProperties(
    ctorDecl: ts.ConstructorDeclaration,
    paramTypes: ts.TypeNode[] | undefined
  ): Autofix[] | undefined {
    if (paramTypes === undefined) {
      return undefined;
    }

    const fieldInitStmts: ts.Statement[] = [];
    const newFieldPos = ctorDecl.getStart();
    const autofixes: Autofix[] = [{ start: newFieldPos, end: newFieldPos, replacementText: '' }];

    for (let i = 0; i < ctorDecl.parameters.length; i++) {
      this.fixCtorParameterPropertiesProcessParam(ctorDecl.parameters[i], paramTypes[i], fieldInitStmts, autofixes);
    }

    // Note: Bodyless ctors can't have parameter properties.
    if (ctorDecl.body) {
      const beforeFieldStmts: ts.Statement[] = [];
      const afterFieldStmts: ts.Statement[] = [];
      const hasSuperExpressionStatement: boolean = this.hasSuperExpression(
        ctorDecl.body,
        beforeFieldStmts,
        afterFieldStmts
      );
      let finalStmts: ts.Statement[] = [];
      if (hasSuperExpressionStatement) {
        finalStmts = beforeFieldStmts.concat(fieldInitStmts).concat(afterFieldStmts);
      } else {
        finalStmts = fieldInitStmts.concat(ctorDecl.body.statements);
      }
      const newBody = ts.factory.createBlock(finalStmts, true);
      const newBodyText = this.printer.printNode(ts.EmitHint.Unspecified, newBody, ctorDecl.getSourceFile());
      autofixes.push({ start: ctorDecl.body.getStart(), end: ctorDecl.body.getEnd(), replacementText: newBodyText });
    }

    return autofixes;
  }

  private hasSuperExpression(
    body: ts.Block,
    beforeFieldStmts: ts.Statement[],
    afterFieldStmts: ts.Statement[]
  ): boolean {
    void this;
    let hasSuperExpressionStatement = false;
    ts.forEachChild(body, (node) => {
      if (this.isSuperCallStmt(node as ts.Statement)) {
        hasSuperExpressionStatement = true;
        beforeFieldStmts.push(node as ts.Statement);
      } else if (hasSuperExpressionStatement) {
        afterFieldStmts.push(node as ts.Statement);
      } else {
        beforeFieldStmts.push(node as ts.Statement);
      }
    });
    return hasSuperExpressionStatement;
  }

  private isSuperCallStmt(node: ts.Statement): boolean {
    void this;
    if (ts.isExpressionStatement(node) && ts.isCallExpression(node.expression)) {
      const expr = node.expression.expression;
      return expr.kind === ts.SyntaxKind.SuperKeyword;
    }
    return false;
  }

  private fixCtorParameterPropertiesProcessParam(
    param: ts.ParameterDeclaration,
    paramType: ts.TypeNode,
    fieldInitStmts: ts.Statement[],
    autofixes: Autofix[]
  ): void {
    // Parameter property can not be a destructuring parameter.
    if (!ts.isIdentifier(param.name)) {
      return;
    }

    if (this.utils.hasAccessModifier(param)) {
      const propIdent = ts.factory.createIdentifier(param.name.text);
      const modifiers = ts.getModifiers(param);
      const paramModifiers = modifiers?.filter((x) => {
        return x.kind !== ts.SyntaxKind.OverrideKeyword;
      });

      const newFieldNode = ts.factory.createPropertyDeclaration(
        paramModifiers,
        propIdent,
        param.questionToken,
        paramType,
        undefined
      );
      const newFieldText =
        this.printer.printNode(ts.EmitHint.Unspecified, newFieldNode, param.getSourceFile()) + this.getNewLine();
      autofixes[0].replacementText += newFieldText;

      const newParamDecl = ts.factory.createParameterDeclaration(
        undefined,
        undefined,
        param.name,
        param.questionToken,
        param.type,
        param.initializer
      );
      const newParamText = this.printer.printNode(ts.EmitHint.Unspecified, newParamDecl, param.getSourceFile());
      autofixes.push({ start: param.getStart(), end: param.getEnd(), replacementText: newParamText });

      fieldInitStmts.push(
        ts.factory.createExpressionStatement(
          ts.factory.createAssignment(
            ts.factory.createPropertyAccessExpression(ts.factory.createThis(), propIdent),
            propIdent
          )
        )
      );
    }
  }

  fixPrivateIdentifier(node: ts.PrivateIdentifier): Autofix[] | undefined {
    const classMember = this.typeChecker.getSymbolAtLocation(node);
    if (!classMember || (classMember.getFlags() & ts.SymbolFlags.ClassMember) === 0 || !classMember.valueDeclaration) {
      return undefined;
    }

    if (this.privateIdentifierCache.has(classMember)) {
      return this.privateIdentifierCache.get(classMember);
    }

    const memberDecl = classMember.valueDeclaration as ts.ClassElement;
    const parentDecl = memberDecl.parent;
    if (!ts.isClassLike(parentDecl) || this.utils.classMemberHasDuplicateName(memberDecl, parentDecl, true)) {
      this.privateIdentifierCache.set(classMember, undefined);
      return undefined;
    }

    let result: Autofix[] | undefined = [];
    this.symbolCache.getReferences(classMember).forEach((ident) => {
      if (ts.isPrivateIdentifier(ident)) {
        result!.push(this.fixSinglePrivateIdentifier(ident));
      }
    });
    if (!result.length) {
      result = undefined;
    }

    this.privateIdentifierCache.set(classMember, result);
    return result;
  }

  private isFunctionDeclarationFirst(tsFunctionDeclaration: ts.FunctionDeclaration): boolean {
    if (tsFunctionDeclaration.name === undefined) {
      return false;
    }

    const symbol = this.typeChecker.getSymbolAtLocation(tsFunctionDeclaration.name);
    if (symbol === undefined) {
      return false;
    }

    let minPos = tsFunctionDeclaration.pos;
    this.symbolCache.getReferences(symbol).forEach((ident) => {
      if (ident.pos < minPos) {
        minPos = ident.pos;
      }
    });

    return minPos >= tsFunctionDeclaration.pos;
  }

  fixNestedFunction(tsFunctionDeclaration: ts.FunctionDeclaration): Autofix[] | undefined {
    const isGenerator = tsFunctionDeclaration.asteriskToken !== undefined;
    const hasThisKeyword =
      tsFunctionDeclaration.body === undefined ? false : scopeContainsThis(tsFunctionDeclaration.body);
    const canBeFixed = !isGenerator && !hasThisKeyword;
    if (!canBeFixed) {
      return undefined;
    }

    const name = tsFunctionDeclaration.name?.escapedText;
    const type = tsFunctionDeclaration.type;
    const body = tsFunctionDeclaration.body;
    if (!name || !type || !body) {
      return undefined;
    }

    // Check only illegal decorators, cause all decorators for function declaration are illegal
    if (ts.getIllegalDecorators(tsFunctionDeclaration)) {
      return undefined;
    }

    if (!this.isFunctionDeclarationFirst(tsFunctionDeclaration)) {
      return undefined;
    }

    const typeParameters = tsFunctionDeclaration.typeParameters;
    const parameters = tsFunctionDeclaration.parameters;
    const modifiers = ts.getModifiers(tsFunctionDeclaration);

    const token = ts.factory.createToken(ts.SyntaxKind.EqualsGreaterThanToken);
    const typeDecl = ts.factory.createFunctionTypeNode(typeParameters, parameters, type);
    const arrowFunc = ts.factory.createArrowFunction(modifiers, typeParameters, parameters, type, token, body);

    const declaration: ts.VariableDeclaration = ts.factory.createVariableDeclaration(
      name,
      undefined,
      typeDecl,
      arrowFunc
    );
    const list: ts.VariableDeclarationList = ts.factory.createVariableDeclarationList([declaration], ts.NodeFlags.Let);

    const statement = ts.factory.createVariableStatement(modifiers, list);
    const text = this.printer.printNode(ts.EmitHint.Unspecified, statement, tsFunctionDeclaration.getSourceFile());
    return [{ start: tsFunctionDeclaration.getStart(), end: tsFunctionDeclaration.getEnd(), replacementText: text }];
  }

  fixMultipleStaticBlocks(nodes: ts.Node[]): Autofix[] | undefined {
    const autofix: Autofix[] | undefined = [];
    let body = (nodes[0] as ts.ClassStaticBlockDeclaration).body;
    let bodyStatements: ts.Statement[] = [];
    bodyStatements = bodyStatements.concat(body.statements);
    for (let i = 1; i < nodes.length; i++) {
      bodyStatements = bodyStatements.concat((nodes[i] as ts.ClassStaticBlockDeclaration).body.statements);
      autofix[i] = { start: nodes[i].getStart(), end: nodes[i].getEnd(), replacementText: '' };
    }
    body = ts.factory.createBlock(bodyStatements, true);
    // static blocks shouldn't have modifiers
    const statickBlock = ts.factory.createClassStaticBlockDeclaration(body);
    const text = this.printer.printNode(ts.EmitHint.Unspecified, statickBlock, nodes[0].getSourceFile());
    autofix[0] = { start: nodes[0].getStart(), end: nodes[0].getEnd(), replacementText: text };
    return autofix;
  }

  private fixSinglePrivateIdentifier(ident: ts.PrivateIdentifier): Autofix {
    if (
      ts.isPropertyDeclaration(ident.parent) ||
      ts.isMethodDeclaration(ident.parent) ||
      ts.isGetAccessorDeclaration(ident.parent) ||
      ts.isSetAccessorDeclaration(ident.parent)
    ) {
      // Note: 'private' modifier should always be first.
      const mods = ts.getModifiers(ident.parent);
      const newMods: ts.Modifier[] = [ts.factory.createModifier(ts.SyntaxKind.PrivateKeyword)];
      if (mods) {
        for (const mod of mods) {
          newMods.push(ts.factory.createModifier(mod.kind));
        }
      }

      const newName = ident.text.slice(1, ident.text.length);
      const newDecl = Autofixer.replacePrivateIdentInDeclarationName(newMods, newName, ident.parent);
      const text = this.printer.printNode(ts.EmitHint.Unspecified, newDecl, ident.getSourceFile());
      return { start: ident.parent.getStart(), end: ident.parent.getEnd(), replacementText: text };
    }

    return {
      start: ident.getStart(),
      end: ident.getEnd(),
      replacementText: ident.text.slice(1, ident.text.length)
    };
  }

  private static replacePrivateIdentInDeclarationName(
    mods: ts.Modifier[],
    name: string,
    oldDecl: ts.PropertyDeclaration | ts.MethodDeclaration | ts.GetAccessorDeclaration | ts.SetAccessorDeclaration
  ): ts.Declaration {
    if (ts.isPropertyDeclaration(oldDecl)) {
      return ts.factory.createPropertyDeclaration(
        mods,
        ts.factory.createIdentifier(name),
        oldDecl.questionToken ?? oldDecl.exclamationToken,
        oldDecl.type,
        oldDecl.initializer
      );
    } else if (ts.isMethodDeclaration(oldDecl)) {
      return ts.factory.createMethodDeclaration(
        mods,
        oldDecl.asteriskToken,
        ts.factory.createIdentifier(name),
        oldDecl.questionToken,
        oldDecl.typeParameters,
        oldDecl.parameters,
        oldDecl.type,
        oldDecl.body
      );
    } else if (ts.isGetAccessorDeclaration(oldDecl)) {
      return ts.factory.createGetAccessorDeclaration(
        mods,
        ts.factory.createIdentifier(name),
        oldDecl.parameters,
        oldDecl.type,
        oldDecl.body
      );
    }
    return ts.factory.createSetAccessorDeclaration(
      mods,
      ts.factory.createIdentifier(name),
      oldDecl.parameters,
      oldDecl.body
    );
  }

  fixRecordObjectLiteral(objectLiteralExpr: ts.ObjectLiteralExpression): Autofix[] | undefined {
    const autofix: Autofix[] = [];

    for (const prop of objectLiteralExpr.properties) {
      if (!prop.name) {
        return undefined;
      }
      if (this.utils.isValidRecordObjectLiteralKey(prop.name)) {
        // Skip property with a valid property key.
        continue;
      }
      if (!ts.isIdentifier(prop.name)) {
        // Can only fix identifier name.
        return undefined;
      }

      const stringLiteralName = ts.factory.createStringLiteralFromNode(prop.name, true);
      const text = this.printer.printNode(ts.EmitHint.Unspecified, stringLiteralName, prop.name.getSourceFile());
      autofix.push({ start: prop.name.getStart(), end: prop.name.getEnd(), replacementText: text });
    }

    return autofix;
  }

  fixUntypedObjectLiteral(
    objectLiteralExpr: ts.ObjectLiteralExpression,
    objectLiteralType: ts.Type | undefined
  ): Autofix[] | undefined {
    if (objectLiteralType) {

      /*
       * Special case for object literal of Record type: fix object's property names
       * by replacing identifiers with string literals.
       */
      if (this.utils.isStdRecordType(this.utils.getNonNullableType(objectLiteralType))) {
        return this.fixRecordObjectLiteral(objectLiteralExpr);
      }

      // Here, we only fix object literal that doesn't have a contextual type.
      return undefined;
    }

    if (Autofixer.hasUnfixableProperty(objectLiteralExpr)) {
      return undefined;
    }

    const enclosingStmt = TsUtils.getEnclosingTopLevelStatement(objectLiteralExpr);
    if (!enclosingStmt) {
      return undefined;
    }

    if (Autofixer.hasMethodsOrAccessors(objectLiteralExpr)) {
      return this.fixObjectLiteralAsClass(objectLiteralExpr, undefined, enclosingStmt);
    }
    return this.fixUntypedObjectLiteralAsInterface(objectLiteralExpr, enclosingStmt);
  }

  private static hasUnfixableProperty(objectLiteralExpr: ts.ObjectLiteralExpression): boolean {
    return objectLiteralExpr.properties.some((prop) => {
      return ts.isSpreadAssignment(prop) || !ts.isIdentifier(prop.name);
    });
  }

  private static hasMethodsOrAccessors(objectLiteralExpr: ts.ObjectLiteralExpression): boolean {
    return objectLiteralExpr.properties.some((prop) => {
      return ts.isMethodDeclaration(prop) || ts.isAccessor(prop);
    });
  }

  private fixUntypedObjectLiteralAsInterface(
    objectLiteralExpr: ts.ObjectLiteralExpression,
    enclosingStmt: ts.Node
  ): Autofix[] | undefined {
    const newInterfaceProps = this.getInterfacePropertiesFromObjectLiteral(objectLiteralExpr, enclosingStmt);
    if (!newInterfaceProps) {
      return undefined;
    }

    const srcFile = objectLiteralExpr.getSourceFile();
    const newInterfaceName = TsUtils.generateUniqueName(this.objectLiteralInterfaceNameGenerator, srcFile);
    if (!newInterfaceName) {
      return undefined;
    }

    return [
      this.createNewInterfaceForObjectLiteral(srcFile, newInterfaceName, newInterfaceProps, enclosingStmt.getStart()),
      this.fixObjectLiteralExpression(srcFile, newInterfaceName, objectLiteralExpr)
    ];
  }

  private getInterfacePropertiesFromObjectLiteral(
    objectLiteralExpr: ts.ObjectLiteralExpression,
    enclosingStmt: ts.Node
  ): ts.PropertySignature[] | undefined {
    const interfaceProps: ts.PropertySignature[] = [];
    for (const prop of objectLiteralExpr.properties) {
      const interfaceProp = this.getInterfacePropertyFromObjectLiteralElement(prop, enclosingStmt);
      if (!interfaceProp) {
        return undefined;
      }
      interfaceProps.push(interfaceProp);
    }
    return interfaceProps;
  }

  private getInterfacePropertyFromObjectLiteralElement(
    prop: ts.ObjectLiteralElementLike,
    enclosingStmt: ts.Node
  ): ts.PropertySignature | undefined {
    // Can't fix if property is not a key-value pair, or the property name is a computed value.
    if (!ts.isPropertyAssignment(prop) || ts.isComputedPropertyName(prop.name)) {
      return undefined;
    }

    const propType = this.typeChecker.getTypeAtLocation(prop);

    // Can't capture generic type parameters of enclosing declarations.
    if (this.utils.hasGenericTypeParameter(propType)) {
      return undefined;
    }

    if (TsUtils.typeIsCapturedFromEnclosingLocalScope(propType, enclosingStmt)) {
      return undefined;
    }

    const propTypeNode = this.typeChecker.typeToTypeNode(propType, undefined, ts.NodeBuilderFlags.None);
    if (!propTypeNode || !this.utils.isSupportedType(propTypeNode)) {
      return undefined;
    }

    const newProp: ts.PropertySignature = ts.factory.createPropertySignature(
      undefined,
      prop.name,
      undefined,
      propTypeNode
    );
    return newProp;
  }

  private createNewInterfaceForObjectLiteral(
    srcFile: ts.SourceFile,
    interfaceName: string,
    members: ts.TypeElement[],
    pos: number
  ): Autofix {
    const newInterfaceDecl = ts.factory.createInterfaceDeclaration(
      undefined,
      interfaceName,
      undefined,
      undefined,
      members
    );
    const text = this.printer.printNode(ts.EmitHint.Unspecified, newInterfaceDecl, srcFile) + this.getNewLine();
    return { start: pos, end: pos, replacementText: text };
  }

  private fixObjectLiteralExpression(
    srcFile: ts.SourceFile,
    newInterfaceName: string,
    objectLiteralExpr: ts.ObjectLiteralExpression
  ): Autofix {

    /*
     * If object literal is initializing a variable or property,
     * then simply add new 'contextual' type to the declaration.
     * Otherwise, cast object literal to newly created interface type.
     */
    if (
      (ts.isVariableDeclaration(objectLiteralExpr.parent) ||
        ts.isPropertyDeclaration(objectLiteralExpr.parent) ||
        ts.isParameter(objectLiteralExpr.parent)) &&
      !objectLiteralExpr.parent.type
    ) {
      const text = ': ' + newInterfaceName;
      const pos = Autofixer.getDeclarationTypePosition(objectLiteralExpr.parent);
      return { start: pos, end: pos, replacementText: text };
    }

    const newTypeRef = ts.factory.createTypeReferenceNode(newInterfaceName);
    let newExpr: ts.Expression = ts.factory.createAsExpression(
      ts.factory.createObjectLiteralExpression(objectLiteralExpr.properties),
      newTypeRef
    );
    if (!ts.isParenthesizedExpression(objectLiteralExpr.parent)) {
      newExpr = ts.factory.createParenthesizedExpression(newExpr);
    }
    const text = this.printer.printNode(ts.EmitHint.Unspecified, newExpr, srcFile);
    return { start: objectLiteralExpr.getStart(), end: objectLiteralExpr.getEnd(), replacementText: text };
  }

  private static getDeclarationTypePosition(
    decl: ts.VariableDeclaration | ts.PropertyDeclaration | ts.ParameterDeclaration
  ): number {
    if (ts.isPropertyDeclaration(decl)) {
      return (decl.questionToken || decl.exclamationToken || decl.name).getEnd();
    } else if (ts.isParameter(decl)) {
      return (decl.questionToken || decl.name).getEnd();
    }
    return (decl.exclamationToken || decl.name).getEnd();
  }

  private fixObjectLiteralAsClass(
    objectLiteralExpr: ts.ObjectLiteralExpression,
    typeDecl: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined,
    enclosingStmt: ts.Node,
    typeNode?: ts.TypeReferenceNode
  ): Autofix[] | undefined {
    if (this.utils.nodeCapturesValueFromEnclosingLocalScope(objectLiteralExpr, enclosingStmt)) {
      return undefined;
    }

    const srcFile = objectLiteralExpr.getSourceFile();
    const newClassName = TsUtils.generateUniqueName(this.objectLiteralClassNameGenerator, srcFile);
    if (!newClassName) {
      return undefined;
    }
    const newInitInterfaceName = TsUtils.generateUniqueName(this.objectLiteralInitInterfaceNameGenerator, srcFile);
    if (!newInitInterfaceName) {
      return undefined;
    }

    const classDeclAndCtorInitProps = this.createClassDeclForObjectLiteral(
      objectLiteralExpr,
      enclosingStmt,
      { className: newClassName, initInterfaceName: newInitInterfaceName },
      typeDecl,
      typeNode
    );
    if (!classDeclAndCtorInitProps) {
      return undefined;
    }
    const { classDecl, ctorInitProps } = classDeclAndCtorInitProps;
    let classDeclText =
      this.printer.printNode(ts.EmitHint.Unspecified, classDecl, srcFile) + this.getNewLine() + this.getNewLine();

    const ctorArgs: ts.Expression[] = [];
    if (ctorInitProps.length) {
      classDeclText += this.createInitInterfaceForObjectLiteral(srcFile, newInitInterfaceName, classDecl);
      classDeclText += this.getNewLine() + this.getNewLine();
      ctorArgs.push(ts.factory.createObjectLiteralExpression(ctorInitProps, ctorInitProps.length > 1));
    }
    const newExpr = ts.factory.createNewExpression(ts.factory.createIdentifier(newClassName), undefined, ctorArgs);
    const newExprText = this.printer.printNode(ts.EmitHint.Unspecified, newExpr, srcFile);
    const ctorCallAutofix = {
      start: objectLiteralExpr.getStart(),
      end: objectLiteralExpr.getEnd(),
      replacementText: newExprText
    };
    const classDeclPos = enclosingStmt.getStart();
    return [{ start: classDeclPos, end: classDeclPos, replacementText: classDeclText }, ctorCallAutofix];
  }

  private createClassDeclForObjectLiteral(
    objectLiteralExpr: ts.ObjectLiteralExpression,
    enclosingStmt: ts.Node,
    names: { className: string; initInterfaceName: string },
    typeDecl: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined,
    typeNode?: ts.TypeReferenceNode
  ): { classDecl: ts.ClassDeclaration; ctorInitProps: ts.PropertyAssignment[] } | undefined {
    const { className, initInterfaceName } = names;
    const classFields: ts.PropertyDeclaration[] = [];
    const classMethods: (ts.MethodDeclaration | ts.AccessorDeclaration)[] = [];
    const ctorBodyStmts: ts.Statement[] = [];
    const ctorInitProps: ts.PropertyAssignment[] = [];

    Autofixer.addSuperCallToObjectLiteralConstructor(typeDecl, ctorBodyStmts);

    for (const prop of objectLiteralExpr.properties) {
      if (ts.isSpreadAssignment(prop) || !ts.isIdentifier(prop.name)) {
        return undefined;
      }
      if (ts.isMethodDeclaration(prop) || ts.isAccessor(prop)) {
        classMethods.push(prop);
        continue;
      }
      const created = this.createClassPropertyForObjectLiteral({
        prop,
        enclosingStmt,
        classFields,
        ctorBodyStmts,
        ctorInitProps
      });
      if (!created) {
        return undefined;
      }
    }

    const classElements: ts.ClassElement[] = [...classFields];
    if (ctorInitProps.length) {
      classElements.push(Autofixer.createClassConstructorForObjectLiteral(initInterfaceName, ctorBodyStmts));
    }
    classElements.push(...classMethods);

    const heritageClauses = Autofixer.createHeritageClausesForObjectLiteralClass(typeDecl, typeNode);

    return {
      classDecl: ts.factory.createClassDeclaration(undefined, className, undefined, heritageClauses, classElements),
      ctorInitProps
    };
  }

  private static addSuperCallToObjectLiteralConstructor(
    typeDecl: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined,
    ctorBodyStmts: ts.Statement[]
  ): void {
    if (typeDecl && ts.isClassDeclaration(typeDecl)) {
      const superCall = ts.factory.createExpressionStatement(
        ts.factory.createCallExpression(ts.factory.createSuper(), undefined, [])
      );
      ctorBodyStmts.push(superCall);
    }
  }

  private createClassPropertyForObjectLiteral(
    createClassPropParams: CreateClassPropertyForObjectLiteralParams
  ): boolean {
    const { prop, enclosingStmt, classFields, ctorBodyStmts, ctorInitProps } = createClassPropParams;
    if (!ts.isIdentifier(prop.name)) {
      return false;
    }
    const propType = this.typeChecker.getTypeAtLocation(prop);

    // Can't capture generic type parameters of enclosing declarations.
    if (this.utils.hasGenericTypeParameter(propType)) {
      return false;
    }

    if (TsUtils.typeIsCapturedFromEnclosingLocalScope(propType, enclosingStmt)) {
      return false;
    }

    const propTypeNode = this.typeChecker.typeToTypeNode(propType, undefined, ts.NodeBuilderFlags.None);
    if (!propTypeNode || !this.utils.isSupportedType(propTypeNode)) {
      return false;
    }

    const propName = ts.factory.createIdentifier(prop.name.text);
    classFields.push(ts.factory.createPropertyDeclaration(undefined, propName, undefined, propTypeNode, undefined));
    ctorBodyStmts.push(
      ts.factory.createExpressionStatement(
        ts.factory.createBinaryExpression(
          ts.factory.createPropertyAccessExpression(ts.factory.createThis(), propName),
          ts.factory.createToken(ts.SyntaxKind.EqualsToken),
          ts.factory.createPropertyAccessExpression(
            ts.factory.createIdentifier(OBJECT_LITERAL_CLASS_CONSTRUCTOR_PARAM_NAME),
            propName
          )
        )
      )
    );
    ctorInitProps.push(ts.isPropertyAssignment(prop) ? prop : ts.factory.createPropertyAssignment(prop.name, propName));
    return true;
  }

  private static createHeritageClausesForObjectLiteralClass(
    typeDecl: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined,
    typeNode?: ts.TypeReferenceNode
  ): ts.HeritageClause[] | undefined {
    if (!typeDecl?.name) {
      return undefined;
    }

    const heritageTypeExpression = typeNode ?
      Autofixer.entityNameToExpression(typeNode.typeName) :
      ts.factory.createIdentifier(typeDecl.name.text);

    return [
      ts.factory.createHeritageClause(
        ts.isClassDeclaration(typeDecl) ? ts.SyntaxKind.ExtendsKeyword : ts.SyntaxKind.ImplementsKeyword,
        [ts.factory.createExpressionWithTypeArguments(heritageTypeExpression, undefined)]
      )
    ];
  }

  private static entityNameToExpression(name: ts.EntityName): ts.Expression {
    if (ts.isQualifiedName(name)) {
      return ts.factory.createPropertyAccessExpression(Autofixer.entityNameToExpression(name.left), name.right);
    }
    return ts.factory.createIdentifier(name.text);
  }

  private static createClassConstructorForObjectLiteral(
    newInitInterfaceName: string,
    ctorBodyStmts: ts.Statement[]
  ): ts.ConstructorDeclaration {
    const ctorParams: ts.ParameterDeclaration[] = [];
    ctorParams.push(
      ts.factory.createParameterDeclaration(
        undefined,
        undefined,
        OBJECT_LITERAL_CLASS_CONSTRUCTOR_PARAM_NAME,
        undefined,
        ts.factory.createTypeReferenceNode(newInitInterfaceName)
      )
    );
    return ts.factory.createConstructorDeclaration(undefined, ctorParams, ts.factory.createBlock(ctorBodyStmts, true));
  }

  private createInitInterfaceForObjectLiteral(
    srcFile: ts.SourceFile,
    interfaceName: string,
    newClassDecl: ts.ClassDeclaration
  ): string {
    const props: ts.PropertySignature[] = [];
    newClassDecl.members.forEach((prop) => {
      if (ts.isPropertyDeclaration(prop)) {
        props.push(ts.factory.createPropertySignature(undefined, prop.name, undefined, prop.type));
      }
    });
    const newInterfaceDecl = ts.factory.createInterfaceDeclaration(
      undefined,
      interfaceName,
      undefined,
      undefined,
      props
    );
    return this.printer.printNode(ts.EmitHint.Unspecified, newInterfaceDecl, srcFile);
  }

  fixTypedObjectLiteral(
    objectLiteralExpr: ts.ObjectLiteralExpression,
    objectLiteralType: ts.Type | undefined
  ): Autofix[] | undefined {
    // Here we only try to fix typed object literal. Other case is handled by 'fixUntypedObjectLiteral' method.

    if (!objectLiteralType || !this.utils.validateObjectLiteralType(objectLiteralType)) {
      return undefined;
    }

    const typeDecl = TsUtils.getDeclaration(objectLiteralType.getSymbol());
    if (!typeDecl || !ts.isClassDeclaration(typeDecl) && !ts.isInterfaceDeclaration(typeDecl) || !typeDecl.name) {
      return undefined;
    }

    if (Autofixer.hasUnfixableProperty(objectLiteralExpr)) {
      return undefined;
    }

    if (this.hasMethodOverridingProperty(objectLiteralExpr, objectLiteralType)) {
      return undefined;
    }

    const enclosingStmt = TsUtils.getEnclosingTopLevelStatement(objectLiteralExpr);
    if (!enclosingStmt) {
      return undefined;
    }

    const typeNode = (objectLiteralExpr.parent as ts.VariableDeclaration).type as ts.TypeReferenceNode | undefined;
    return this.fixObjectLiteralAsClass(objectLiteralExpr, typeDecl, enclosingStmt, typeNode);
  }

  private hasMethodOverridingProperty(
    objectLiteralExpr: ts.ObjectLiteralExpression,
    objectLiteralType: ts.Type
  ): boolean {
    const typeProps = this.typeChecker.getPropertiesOfType(objectLiteralType);
    for (const objProp of objectLiteralExpr.properties) {
      if (
        ts.isPropertyAssignment(objProp) &&
        typeProps.some((typeProp) => {
          const typePropDecl = TsUtils.getDeclaration(typeProp);
          return (
            !!typePropDecl &&
            (ts.isMethodSignature(typePropDecl) || ts.isMethodDeclaration(typePropDecl)) &&
            typePropDecl.name === objProp.name
          );
        })
      ) {
        return true;
      }

      if (
        ts.isMethodDeclaration(objProp) &&
        typeProps.some((typeProp) => {
          const typePropDecl = TsUtils.getDeclaration(typeProp);
          return (
            !!typePropDecl &&
            (ts.isPropertyDeclaration(typePropDecl) || ts.isPropertySignature(typePropDecl)) &&
            typePropDecl.name.getText() === objProp.name.getText()
          );
        })
      ) {
        return true;
      }
    }

    return false;
  }

  fixShorthandPropertyAssignment(prop: ts.ShorthandPropertyAssignment): Autofix[] {
    const newName = ts.factory.createIdentifier(prop.name.text);
    const newProp = ts.factory.createPropertyAssignment(newName, newName);
    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, newProp, prop.getSourceFile());
    return [{ start: prop.getStart(), end: prop.getEnd(), replacementText }];
  }

  /*
   * In case of type alias initialized with type literal, replace
   * entire type alias with identical interface declaration.
   */
  private proceedTypeAliasDeclaration(typeLiteral: ts.TypeLiteralNode): Autofix[] | undefined {
    if (ts.isTypeAliasDeclaration(typeLiteral.parent)) {
      const typeAlias = typeLiteral.parent;
      const newInterfaceDecl = ts.factory.createInterfaceDeclaration(
        typeAlias.modifiers,
        typeAlias.name,
        typeAlias.typeParameters,
        undefined,
        typeLiteral.members
      );
      const text = this.printer.printNode(ts.EmitHint.Unspecified, newInterfaceDecl, typeLiteral.getSourceFile());
      return [{ start: typeAlias.getStart(), end: typeAlias.getEnd(), replacementText: text }];
    }
    return undefined;
  }

  fixTypeliteral(typeLiteral: ts.TypeLiteralNode): Autofix[] | undefined {
    const typeAliasAutofix = this.proceedTypeAliasDeclaration(typeLiteral);
    if (typeAliasAutofix) {
      return typeAliasAutofix;
    }

    /*
     * Create new interface declaration with members of type literal
     * and put the interface name in place of the type literal.
     */
    const srcFile = typeLiteral.getSourceFile();
    const enclosingStmt = TsUtils.getEnclosingTopLevelStatement(typeLiteral);
    if (!enclosingStmt) {
      return undefined;
    }

    if (this.utils.nodeCapturesValueFromEnclosingLocalScope(typeLiteral, enclosingStmt)) {
      return undefined;
    }

    const newInterfaceName = TsUtils.generateUniqueName(this.typeLiteralInterfaceNameGenerator, srcFile);
    if (!newInterfaceName) {
      return undefined;
    }
    const newInterfacePos = enclosingStmt.getStart();
    const newInterfaceDecl = ts.factory.createInterfaceDeclaration(
      undefined,
      newInterfaceName,
      undefined,
      undefined,
      typeLiteral.members
    );
    const interfaceText =
      this.printer.printNode(ts.EmitHint.Unspecified, newInterfaceDecl, srcFile) + this.getNewLine();

    return [
      { start: newInterfacePos, end: newInterfacePos, replacementText: interfaceText },
      { start: typeLiteral.getStart(), end: typeLiteral.getEnd(), replacementText: newInterfaceName }
    ];
  }

  removeNode(node: ts.Node): Autofix[] {
    void this;
    return [{ start: node.getStart(), end: node.getEnd(), replacementText: '' }];
  }

  replaceNode(node: ts.Node, replacementText: string): Autofix[] {
    void this;
    return [{ start: node.getStart(), end: node.getEnd(), replacementText }];
  }

  removeImportSpecifier(specToRemove: ts.ImportSpecifier, importDecl: ts.ImportDeclaration): Autofix[] | undefined {
    const importClause = importDecl.importClause;
    const namedBindings = importClause?.namedBindings;
    if (!importClause || !namedBindings || !ts.isNamedImports(namedBindings)) {
      return undefined;
    }

    const fixes: Autofix[] = [];
    const allSpecifiers = namedBindings.elements;
    const remainingSpecifiers = allSpecifiers.filter((spec) => {
      return spec !== specToRemove;
    });

    // If none are valid, remove all named imports.
    if (remainingSpecifiers.length === 0) {
      fixes.push({
        start: importClause.name ? importClause.name.end : importDecl.getStart(),
        end: importClause.name ? namedBindings.end : importDecl.getEnd(),
        replacementText: ''
      });
    } else {
      const specIndex = allSpecifiers.findIndex((spec) => {
        return spec === specToRemove;
      });
      const isLast = specIndex === allSpecifiers.length - 1;
      const isFirst = specIndex === 0;

      let start = specToRemove.getStart();
      let end = specToRemove.getEnd();

      if (!isLast) {
        end = allSpecifiers[specIndex + 1].getStart();
      } else if (!isFirst) {
        const prev = allSpecifiers[specIndex - 1];
        start = prev.getEnd();
      }
      fixes.push({ start: start, end: end, replacementText: '' });
    }

    const alias = specToRemove.name;
    const original = specToRemove.propertyName;
    if (original) {
      const replacements = this.replaceIdentifierUsages(alias, original.getText());
      fixes.push(...replacements);
    }
    return fixes;
  }

  removeDefaultImport(
    importDecl: ts.ImportDeclaration,
    defaultImport: ts.Identifier,
    replacementName: string
  ): Autofix[] | undefined {
    const importClause = importDecl.importClause;
    if (!importClause || !defaultImport) {
      return undefined;
    }
    const fixes: Autofix[] = [];
    const namedBindings = importClause.namedBindings;
    fixes.push({
      start: namedBindings ? defaultImport.getStart() : importDecl.getStart(),
      end: namedBindings ? namedBindings.getStart() : importDecl.getEnd(),
      replacementText: ''
    });

    const replacements = this.replaceIdentifierUsages(defaultImport, replacementName);
    fixes.push(...replacements);

    return fixes;
  }

  replaceIdentifierUsages(importedIdentifier: ts.Identifier, replacementName: string): Autofix[] {
    const fixes: Autofix[] = [];
    const file = importedIdentifier.getSourceFile();
    const originalSymbol = this.typeChecker.getSymbolAtLocation(importedIdentifier);
    if (!originalSymbol) {
      return fixes;
    }

    const visit = (node: ts.Node): void => {
      if (ts.isIdentifier(node) && !ts.isImportClause(node.parent) && !ts.isImportSpecifier(node.parent)) {
        const nodeSymbol = this.typeChecker.getSymbolAtLocation(node);
        if (nodeSymbol === originalSymbol) {
          // Skip any identifier that is part of a 'locks' qualification
          const parent = node.parent;
          if (
            ts.isQualifiedName(parent) && parent.right.text === ARKTSUTILS_LOCKS_MEMBER ||
            ts.isPropertyAccessExpression(parent) && parent.name.text === ARKTSUTILS_LOCKS_MEMBER
          ) {
            return;
          }
          fixes.push({
            start: node.getStart(),
            end: node.getEnd(),
            replacementText: replacementName
          });
        }
      }
      ts.forEachChild(node, visit);
    };

    visit(file);
    return fixes;
  }

  fixSendableExplicitFieldType(node: ts.PropertyDeclaration): Autofix[] | undefined {
    const initializer = node.initializer;
    if (initializer === undefined) {
      return undefined;
    }

    const propType = this.typeChecker.getTypeAtLocation(node);
    const propTypeNode = this.typeChecker.typeToTypeNode(propType, undefined, ts.NodeBuilderFlags.None);
    if (!propTypeNode || !this.utils.isSupportedType(propTypeNode)) {
      return undefined;
    }

    const pos = (node.questionToken || node.exclamationToken || node.name).getEnd();
    const text = ': ' + this.printer.printNode(ts.EmitHint.Unspecified, propTypeNode, node.getSourceFile());
    return [{ start: pos, end: pos, replacementText: text }];
  }

  addClassSendableDecorator(
    hClause: ts.HeritageClause,
    typeExpr: ts.ExpressionWithTypeArguments
  ): Autofix[] | undefined {
    const decl = hClause.parent;
    if (this.sendableDecoratorCache.has(decl)) {
      return this.sendableDecoratorCache.get(decl);
    }
    if (hClause.token === ts.SyntaxKind.ExtendsKeyword && !this.utils.isValidSendableClassExtends(typeExpr)) {
      return undefined;
    }
    const result = this.addSendableDecorator(decl);
    this.sendableDecoratorCache.set(decl, result);
    return result;
  }

  addSendableDecorator(node: ts.Node): Autofix[] {
    void this;
    const text = '@' + SENDABLE_DECORATOR + this.getNewLine();
    const pos = node.getStart();
    return [{ start: pos, end: pos, replacementText: text }];
  }

  fixGlobalThisSet(decl: ts.BinaryExpression): Autofix[] | undefined {
    void this;
    const left = decl.left;
    const right = decl.right;
    if (
      ts.isPropertyAccessExpression(left) &&
      ts.isIdentifier(left.expression) &&
      left.expression.text === 'globalThis'
    ) {
      const propertyName = left.name.text;
      const value = right.getText();
      const replacementText = `${SPECIAL_LIB_NAME}.globalThis.set("${propertyName}", ${value})`;
      return [{ start: decl.getStart(), end: decl.getEnd(), replacementText: replacementText }];
    }
    return undefined;
  }

  fixVoidOperator(voidExpr: ts.VoidExpression): Autofix[] {
    let newExpr = voidExpr.expression;

    if (Autofixer.needParenthesesForVoidOperator(newExpr)) {
      newExpr = ts.factory.createParenthesizedExpression(newExpr);
    }

    const funcBody = ts.factory.createBlock(
      [
        ts.factory.createExpressionStatement(newExpr),
        ts.factory.createReturnStatement(ts.factory.createIdentifier(UNDEFINED_NAME))
      ],
      true
    );

    const arrowFunc = ts.factory.createArrowFunction(
      undefined,
      undefined,
      [],
      undefined,
      ts.factory.createToken(ts.SyntaxKind.EqualsGreaterThanToken),
      funcBody
    );

    const callExpr = ts.factory.createCallExpression(
      ts.factory.createParenthesizedExpression(arrowFunc),
      undefined,
      undefined
    );

    const text = this.printer.printNode(ts.EmitHint.Unspecified, callExpr, voidExpr.getSourceFile());
    return [{ start: voidExpr.getStart(), end: voidExpr.getEnd(), replacementText: text }];
  }

  private static needParenthesesForVoidOperator(expr: ts.Expression): boolean {
    return ts.isObjectLiteralExpression(expr) || ts.isFunctionExpression(expr) || ts.isClassExpression(expr);
  }

  fixRegularExpressionLiteral(node: ts.RegularExpressionLiteral | ts.CallExpression): Autofix[] | undefined {
    const srcFile = node.getSourceFile();
    let patternNode: ts.Expression | undefined;
    let flag: string | undefined;

    if (ts.isRegularExpressionLiteral(node)) {
      const literalText = node.getText();
      const parts = Autofixer.extractRegexParts(literalText);
      patternNode = ts.factory.createStringLiteral(parts.pattern);
      flag = parts.flag;
    } else if (ts.isCallExpression(node)) {
      const args = node.arguments;
      if (args.length === 0 || args.length > 2) {
        return undefined;
      }
      const patternArg = args[0];
      if (!this.isStaticStringExpression(patternArg)) {
        return undefined;
      }
      patternNode = patternArg;

      if (args.length > 1) {
        const flagArg = args[1];
        if (ts.isStringLiteral(flagArg)) {
          flag = flagArg.text;
        } else {
          return undefined;
        }
      }
    } else {
      return undefined;
    }

    const newArgs: ts.Expression[] = [patternNode];
    if (flag !== undefined) {
      newArgs.push(ts.factory.createStringLiteral(flag));
    }
    const newExpression = ts.factory.createNewExpression(ts.factory.createIdentifier('RegExp'), undefined, newArgs);

    const text = this.printer.printNode(ts.EmitHint.Unspecified, newExpression, srcFile);
    return [
      {
        start: node.getStart(),
        end: node.getEnd(),
        replacementText: text
      }
    ];
  }

  private isStaticStringExpression(node: ts.Node): boolean {
    if (ts.isStringLiteral(node)) {
      return true;
    }
    if (ts.isBinaryExpression(node)) {
      return (
        node.operatorToken.kind === ts.SyntaxKind.PlusToken &&
        this.isStaticStringExpression(node.left) &&
        this.isStaticStringExpression(node.right)
      );
    }
    if (ts.isCallExpression(node)) {
      const expression = node.expression;
      if (
        ts.isPropertyAccessExpression(expression) &&
        expression.name.text === 'concat' &&
        this.isStaticStringExpression(expression.expression)
      ) {
        return node.arguments.every((arg) => {
          return this.isStaticStringExpression(arg);
        });
      }
    }
    return false;
  }

  private static extractRegexParts(literalText: string): {
    pattern: string;
    flag: string | undefined;
  } {
    let pattern: string = '';
    let flag: string | undefined;
    const lastSlashIndex = literalText.lastIndexOf('/');
    const afterLastSlash = literalText.slice(lastSlashIndex + 1);
    if (afterLastSlash !== '') {
      pattern = literalText.slice(1, lastSlashIndex);
      flag = afterLastSlash;
    } else {
      pattern = literalText.slice(1, lastSlashIndex);
    }
    return { pattern, flag };
  }

  /*
   * "unsafe" (result is not common subset) autofixes
   */

  // to use special lib functions it's need to import it
  static SPECIAL_LIB_NAME = 'specialAutofixLib';

  // autofix for '**', '**=' operations and 'math.pow()' call
  fixExponent(exponentNode: ts.Node): Autofix[] | undefined {
    let autofix: Autofix[] = [];
    let replaceText: Autofix = { replacementText: '', start: 0, end: 0 };

    // ts.BinaryExpression
    let callArgs: ts.Expression[] | undefined;
    if (exponentNode.kind === ts.SyntaxKind.CallExpression) {
      callArgs = [...(exponentNode as ts.CallExpression).arguments];
    } else if (exponentNode.kind === ts.SyntaxKind.BinaryExpression) {
      callArgs = [(exponentNode as ts.BinaryExpression).left, (exponentNode as ts.BinaryExpression).right];
    } else {
      // if we get here - it was an error!
      return undefined;
    }

    const newCall = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(
        ts.factory.createIdentifier('Math'),
        ts.factory.createIdentifier('pow')
      ),
      undefined,
      callArgs
    );

    if (
      exponentNode.kind === ts.SyntaxKind.BinaryExpression &&
      (exponentNode as ts.BinaryExpression).operatorToken.kind === ts.SyntaxKind.AsteriskAsteriskEqualsToken
    ) {
      const newAssignment = ts.factory.createAssignment((exponentNode as ts.BinaryExpression).left, newCall);
      replaceText = {
        replacementText: this.printer.printNode(ts.EmitHint.Unspecified, newAssignment, exponentNode.getSourceFile()),
        start: exponentNode.getStart(),
        end: exponentNode.getEnd()
      };
    } else {
      replaceText = {
        replacementText: this.printer.printNode(ts.EmitHint.Unspecified, newCall, exponentNode.getSourceFile()),
        start: exponentNode.getStart(),
        end: exponentNode.getEnd()
      };
    }

    autofix = [replaceText];
    return autofix;
  }

  fixNativeBidirectionalBinding(
    expr: ts.NonNullExpression,
    interfacesNeedToImport: Set<string>
  ): Autofix[] | undefined {
    if (!ts.isNonNullExpression(expr.expression)) {
      return undefined;
    }
    const originalExpr = expr.expression.expression;
    const doubleDollarIdentifier = ts.factory.createIdentifier(DOUBLE_DOLLAR_IDENTIFIER);
    interfacesNeedToImport.add(DOUBLE_DOLLAR_IDENTIFIER);
    const callExpr = ts.factory.createCallExpression(doubleDollarIdentifier, undefined, [originalExpr]);
    const text = this.printer.printNode(ts.EmitHint.Unspecified, callExpr, expr.getSourceFile());
    return [{ start: expr.getStart(), end: expr.getEnd(), replacementText: text }];
  }

  fixCustomBidirectionalBinding(
    originalExpr: ts.ObjectLiteralExpression,
    type: ts.TypeNode | undefined,
    currentParam: ts.Identifier,
    customParam: ts.Identifier
  ): Autofix[] | undefined {
    const assignment1 = ts.factory.createPropertyAssignment(
      customParam,
      ts.factory.createPropertyAccessExpression(ts.factory.createThis(), currentParam)
    );
    const value = ts.factory.createIdentifier(VALUE_IDENTIFIER);
    const block = ts.factory.createBlock(
      [
        ts.factory.createExpressionStatement(
          ts.factory.createBinaryExpression(
            ts.factory.createPropertyAccessExpression(ts.factory.createThis(), currentParam),
            ts.factory.createToken(ts.SyntaxKind.EqualsToken),
            value
          )
        )
      ],
      true
    );
    const parameter = ts.factory.createParameterDeclaration(undefined, undefined, value, undefined, type, undefined);
    const arrowFunc = ts.factory.createArrowFunction(
      undefined,
      undefined,
      [parameter],
      undefined,
      ts.factory.createToken(ts.SyntaxKind.EqualsGreaterThanToken),
      block
    );
    const assignment2 = ts.factory.createPropertyAssignment(
      ts.factory.createIdentifier('$' + customParam.getText()),
      arrowFunc
    );
    const newExpr = ts.factory.createObjectLiteralExpression([assignment1, assignment2], true);
    let text = this.printer.printNode(ts.EmitHint.Unspecified, newExpr, originalExpr.getSourceFile());
    const startPos = this.sourceFile.getLineAndCharacterOfPosition(originalExpr.parent.getStart()).character;
    text = this.adjustIndentation(text, startPos);
    return [{ start: originalExpr.getStart(), end: originalExpr.getEnd(), replacementText: text }];
  }

  fixDoubleDollar(dollarExpr: ts.PropertyAccessExpression, interfacesNeedToImport: Set<string>): Autofix[] {
    const dollarValue = dollarExpr.name.escapedText as string;
    const dollarValueExpr = ts.factory.createPropertyAccessExpression(
      ts.factory.createThis(),
      ts.factory.createIdentifier(dollarValue)
    );
    const doubleDollarIdentifier = ts.factory.createIdentifier(DOUBLE_DOLLAR_IDENTIFIER);
    interfacesNeedToImport.add(DOUBLE_DOLLAR_IDENTIFIER);
    const callExpr = ts.factory.createCallExpression(doubleDollarIdentifier, undefined, [dollarValueExpr]);
    const text = this.printer.printNode(ts.EmitHint.Unspecified, callExpr, dollarExpr.getSourceFile());
    return [{ start: dollarExpr.getStart(), end: dollarExpr.getEnd(), replacementText: text }];
  }

  fixDollarBind(property: ts.PropertyAssignment): Autofix[] {
    const identifier = property.initializer;
    const paramName = identifier.getText().substring(1);
    const newPropInit = ts.factory.createPropertyAccessExpression(
      ts.factory.createThis(),
      ts.factory.createIdentifier(paramName)
    );
    const text = this.printer.printNode(ts.EmitHint.Unspecified, newPropInit, property.getSourceFile());
    return [{ start: identifier.getStart(), end: identifier.getEnd(), replacementText: text }];
  }

  fixExtendDecorator(
    extendDecorator: ts.Decorator,
    preserveDecorator: boolean,
    interfacesNeedToImport: Set<string>
  ): Autofix[] | undefined {
    if (!ts.isCallExpression(extendDecorator.expression)) {
      return undefined;
    }
    const funcDecl = extendDecorator.parent;
    if (!ts.isFunctionDeclaration(funcDecl)) {
      return undefined;
    }
    const block = funcDecl.body;
    const parameters: ts.MemberName[] = [];
    const values: ts.Expression[][] = [];
    const statements = block?.statements;
    Autofixer.getParamsAndValues(statements, parameters, values);
    const returnStatement = ts.factory.createReturnStatement(ts.factory.createThis());
    const newBlock = Autofixer.createBlock(parameters, values, ts.factory.createThis(), returnStatement);
    const componentName = extendDecorator.expression.arguments[0]?.getText();
    if (!componentName) {
      return undefined;
    }
    const typeName = componentName + ATTRIBUTE_SUFFIX;
    interfacesNeedToImport.add(typeName);
    const parameDecl = ts.factory.createParameterDeclaration(
      undefined,
      undefined,
      ts.factory.createIdentifier(THIS_IDENTIFIER),
      undefined,
      ts.factory.createTypeReferenceNode(ts.factory.createIdentifier(typeName), undefined),
      undefined
    );
    const returnType = ts.factory.createTypeReferenceNode(ts.factory.createIdentifier(THIS_IDENTIFIER), undefined);
    const newFuncDecl = Autofixer.createFunctionDeclaration(funcDecl, undefined, parameDecl, returnType, newBlock);
    let text = this.printer.printNode(ts.EmitHint.Unspecified, newFuncDecl, funcDecl.getSourceFile());
    if (preserveDecorator) {
      text = '@' + CustomInterfaceName.AnimatableExtend + this.getNewLine() + text;
    }
    return [{ start: funcDecl.getStart(), end: funcDecl.getEnd(), replacementText: text }];
  }

  fixEntryDecorator(entryDecorator: ts.Decorator): Autofix[] | undefined {
    if (!ts.isCallExpression(entryDecorator.expression)) {
      return undefined;
    }

    const args = entryDecorator.expression.arguments;
    if (args.length !== 1) {
      return undefined;
    }

    const parentNode = entryDecorator.parent;
    const arg = args[0];
    let getLocalStorageStatement: ts.VariableStatement | undefined;

    if (ts.isIdentifier(arg) || ts.isNewExpression(arg) || ts.isCallExpression(arg)) {
      getLocalStorageStatement = Autofixer.createGetLocalStorageLambdaStatement(arg);
    } else if (ts.isObjectLiteralExpression(arg)) {
      getLocalStorageStatement = Autofixer.processEntryAnnotationObjectLiteralExpression(arg);
    }

    if (getLocalStorageStatement !== undefined) {
      let text = this.printer.printNode(ts.EmitHint.Unspecified, getLocalStorageStatement, parentNode.getSourceFile());
      const fixedEntryDecorator = Autofixer.createFixedEntryDecorator();
      const fixedEntryDecoratorText = this.printer.printNode(
        ts.EmitHint.Unspecified,
        fixedEntryDecorator,
        parentNode.getSourceFile()
      );
      text = text + this.getNewLine() + fixedEntryDecoratorText;
      return [{ start: entryDecorator.getStart(), end: entryDecorator.getEnd(), replacementText: text }];
    }
    return undefined;
  }

  private static createFixedEntryDecorator(): ts.Decorator {
    const storageProperty = ts.factory.createPropertyAssignment(
      ts.factory.createIdentifier(ENTRY_STORAGE_PROPERITY),
      ts.factory.createStringLiteral(GET_LOCAL_STORAGE_FUNC_NAME)
    );
    const objectLiteralExpr = ts.factory.createObjectLiteralExpression([storageProperty], false);
    const callExpr = ts.factory.createCallExpression(ts.factory.createIdentifier(ENTRY_DECORATOR_NAME), undefined, [
      objectLiteralExpr
    ]);
    return ts.factory.createDecorator(callExpr);
  }

  private static processEntryAnnotationObjectLiteralExpression(
    expression: ts.ObjectLiteralExpression
  ): ts.VariableStatement | undefined {
    const objectProperties = expression.properties;
    if (objectProperties.length !== 1) {
      return undefined;
    }
    const objectProperty = objectProperties[0];
    if (!ts.isPropertyAssignment(objectProperty)) {
      return undefined;
    }
    if (ts.isIdentifier(objectProperty.name)) {
      if (objectProperty.name.escapedText !== ENTRY_STORAGE_PROPERITY) {
        return undefined;
      }
      const properityInitializer = objectProperty.initializer;
      if (
        ts.isIdentifier(properityInitializer) ||
        ts.isNewExpression(properityInitializer) ||
        ts.isCallExpression(properityInitializer) ||
        ts.isPropertyAccessExpression(properityInitializer)
      ) {
        return Autofixer.createGetLocalStorageLambdaStatement(properityInitializer);
      }
    }
    return undefined;
  }

  private static createGetLocalStorageLambdaStatement(expression: ts.Expression): ts.VariableStatement {
    const variable = ts.factory.createVariableDeclaration(
      ts.factory.createIdentifier(GET_LOCAL_STORAGE_FUNC_NAME),
      undefined,
      undefined,
      ts.factory.createArrowFunction(
        undefined,
        undefined,
        [],
        ts.factory.createTypeReferenceNode(ts.factory.createIdentifier(LOCAL_STORAGE_TYPE_NAME), undefined),
        ts.factory.createToken(ts.SyntaxKind.EqualsGreaterThanToken),
        expression
      )
    );
    const declarationList = ts.factory.createVariableDeclarationList([variable], ts.NodeFlags.Const);
    return ts.factory.createVariableStatement(undefined, declarationList);
  }

  fixProvideDecorator(provideDecorator: ts.Decorator): Autofix[] | undefined {
    const callExpr = provideDecorator.expression as ts.CallExpression;
    const args = callExpr.arguments;
    const parentNode = provideDecorator.parent;
    const arg = args[0];
    let provideAnnotationFixed: ts.Decorator | undefined;
    if (ts.isStringLiteral(arg)) {
      provideAnnotationFixed = Autofixer.createProvideDecorator(arg);
    }
    if (ts.isObjectLiteralExpression(arg)) {
      const properties = arg.properties;
      const property = properties[0] as ts.PropertyAssignment;
      const propertyInitializer = property.initializer as ts.StringLiteral;
      provideAnnotationFixed = Autofixer.createProvideDecorator(propertyInitializer, true);
    }
    if (provideAnnotationFixed !== undefined) {
      const text = this.printer.printNode(ts.EmitHint.Unspecified, provideAnnotationFixed, parentNode.getSourceFile());
      return [{ start: provideDecorator.getStart(), end: provideDecorator.getEnd(), replacementText: text }];
    }
    return undefined;
  }

  private static createProvideDecorator(
    alias: ts.StringLiteral,
    allowOverride: boolean | undefined = undefined
  ): ts.Decorator {
    const properties: ts.PropertyAssignment[] = [];
    properties.push(
      ts.factory.createPropertyAssignment(ts.factory.createIdentifier(PROVIDE_ALIAS_PROPERTY_NAME), alias)
    );
    if (allowOverride !== undefined && allowOverride) {
      properties.push(
        ts.factory.createPropertyAssignment(
          ts.factory.createIdentifier(PROVIDE_ALLOW_OVERRIDE_PROPERTY_NAME),
          ts.factory.createTrue()
        )
      );
    }
    const objectLiteralExpr = ts.factory.createObjectLiteralExpression(properties, false);
    const callExpr = ts.factory.createCallExpression(ts.factory.createIdentifier(PROVIDE_DECORATOR_NAME), undefined, [
      objectLiteralExpr
    ]);
    return ts.factory.createDecorator(callExpr);
  }

  private static createFunctionDeclaration(
    funcDecl: ts.FunctionDeclaration,
    typeParameters: ts.TypeParameterDeclaration[] | undefined,
    paramDecl: ts.ParameterDeclaration,
    returnType: ts.TypeNode,
    block: ts.Block
  ): ts.FunctionDeclaration {
    return ts.factory.createFunctionDeclaration(
      undefined,
      undefined,
      funcDecl.name,
      typeParameters,
      [paramDecl, ...funcDecl.parameters],
      returnType,
      block
    );
  }

  private static createBlock(
    parameters: ts.MemberName[],
    values: ts.Expression[][],
    arg: ts.Expression,
    returnStatement?: ts.Statement
  ): ts.Block {
    const statements: ts.Statement[] = [];
    for (let i = 0; i < parameters.length; i++) {
      const parameter = parameters[i];
      const value = values[i];
      const statement = ts.factory.createExpressionStatement(
        ts.factory.createCallExpression(ts.factory.createPropertyAccessExpression(arg, parameter), undefined, value)
      );
      statements.push(statement);
    }
    if (returnStatement) {
      statements.push(returnStatement);
    }
    const block = ts.factory.createBlock(statements, true);
    return block;
  }

  private static traverseNodes(node: ts.Node, parameters: ts.MemberName[], values: ts.Expression[][]): void {
    const callExpressions: ts.CallExpression[] = Autofixer.extractCallExpressions(node);
    callExpressions.forEach((callExpression) => {
      if (ts.isPropertyAccessExpression(callExpression.expression)) {
        const propertyAccess = callExpression.expression;
        parameters.push(propertyAccess.name);
      } else if (ts.isIdentifier(callExpression.expression)) {
        parameters.push(callExpression.expression);
      }
      values.push(Array.from(callExpression.arguments));
    });
  }

  private static extractCallExpressions(node: ts.Node): ts.CallExpression[] {
    const callExpressions: ts.CallExpression[] = [];
    let current: ts.Node | undefined;
    if (ts.isExpressionStatement(node)) {
      if (ts.isCallExpression(node.expression)) {
        current = node.expression;
      }
    }

    if (ts.isPropertyAssignment(node)) {
      if (ts.isCallExpression(node.initializer)) {
        current = node.initializer;
      }
    }

    while (current) {
      if (ts.isCallExpression(current)) {
        if (
          (ts.isPropertyAccessExpression(current.parent) ||
            ts.isExpressionStatement(current.parent) ||
            ts.isPropertyAssignment(current.parent)) &&
          (ts.isPropertyAccessExpression(current.expression) || ts.isIdentifier(current.expression))
        ) {
          callExpressions.push(current);
        }
      }

      if (ts.isCallExpression(current) || ts.isPropertyAccessExpression(current)) {
        current = current.expression;
      } else {
        break;
      }
    }
    return callExpressions;
  }

  private static getParamsAndValues(
    statements: ts.NodeArray<ts.Statement> | undefined,
    parameters: ts.MemberName[],
    values: ts.Expression[][]
  ): void {
    if (!statements) {
      return;
    }
    for (let i = 0; i < statements.length; i++) {
      const statement = statements[i];
      const tempParas: ts.MemberName[] = [];
      const tempVals: ts.Expression[][] = [];
      Autofixer.traverseNodes(statement, tempParas, tempVals);
      if (
        ts.isExpressionStatement(statement) &&
        ts.isCallExpression(statement.expression) &&
        ts.isPropertyAccessExpression(statement.expression.expression)
      ) {
        tempParas.reverse();
        tempVals.reverse();
      }
      for (let j = 0; j < tempParas.length; j++) {
        parameters.push(tempParas[j]);
        values.push(tempVals[j]);
      }
    }
  }

  private getVariableName(node: ts.Node): string | undefined {
    let variableName: string | undefined;

    switch (node.kind) {
      case ts.SyntaxKind.BinaryExpression: {
        const binaryExpr = node as ts.BinaryExpression;
        if (binaryExpr.operatorToken.kind !== ts.SyntaxKind.EqualsToken) {
          return undefined;
        }

        variableName = binaryExpr.left.getText();
        break;
      }
      case ts.SyntaxKind.VariableDeclaration: {
        const variableDecl = node as ts.VariableDeclaration;
        variableName = variableDecl.name.getText();
        break;
      }
      case ts.SyntaxKind.ExpressionStatement: {
        variableName = TsUtils.generateUniqueName(this.tmpVariableNameGenerator, this.sourceFile);
        break;
      }
      default: {
        return undefined;
      }
    }

    return variableName;
  }

  private getNewNodesForIncrDecr(variableName: string, operator: number): IncrementDecrementNodeInfo | undefined {
    let update: string | undefined;
    let updateNode: ts.BinaryExpression | undefined;

    switch (operator) {
      case ts.SyntaxKind.MinusMinusToken: {
        const { varAssignText, addOrDecrOperation } = this.createNewIncrDecrNodes(
          variableName,
          ts.SyntaxKind.MinusToken
        );
        update = varAssignText;
        updateNode = addOrDecrOperation;
        break;
      }
      case ts.SyntaxKind.PlusPlusToken: {
        const { varAssignText, addOrDecrOperation } = this.createNewIncrDecrNodes(
          variableName,
          ts.SyntaxKind.PlusToken
        );
        update = varAssignText;
        updateNode = addOrDecrOperation;
        break;
      }
      default:
        return undefined;
    }

    return { varAssignText: update, addOrDecrOperation: updateNode };
  }

  fixUnaryIncrDecr(
    node: ts.PrefixUnaryExpression | ts.PostfixUnaryExpression,
    pan: ts.PropertyAccessExpression
  ): Autofix[] | undefined {
    const parent = node.parent;
    const grandParent = parent.parent;

    const { expression, name } = pan;
    const { operator } = node;
    const isVariableDeclaration = ts.isVariableDeclaration(node.parent);

    const variableName = this.getVariableName(node.parent);

    if (!variableName) {
      return undefined;
    }

    const updateNodes = this.getNewNodesForIncrDecr(variableName, operator);

    if (!updateNodes?.varAssignText || !updateNodes.addOrDecrOperation) {
      return undefined;
    }

    const replacementText = this.getReplacementTextForPrefixAndPostfixUnary(
      node,
      updateNodes,
      expression,
      name,
      variableName
    );

    if (!replacementText) {
      return undefined;
    }

    if (isVariableDeclaration) {
      const start = grandParent.getStart();
      const end = grandParent.getEnd();
      return [{ replacementText, start, end }];
    }

    const start = parent.getStart();
    const end = parent.getEnd();
    return [{ replacementText, start, end }];
  }

  private getReplacementTextForPrefixAndPostfixUnary(
    node: ts.Node,
    updateNodes: IncrementDecrementNodeInfo,
    expression: ts.LeftHandSideExpression,
    name: ts.MemberName,
    variableName: string
  ): string | undefined {
    const { varAssignText, addOrDecrOperation } = updateNodes;
    const converted: ts.Node = this.createGetPropertyForIncrDecr(expression.getText(), name.text);
    let convertedAssigned = '';
    if (ts.isBinaryExpression(node.parent) && node.parent.operatorToken.kind === ts.SyntaxKind.EqualsToken) {
      convertedAssigned = this.wrapPropertyAccessInBinaryExpr(variableName, converted);
    } else {
      convertedAssigned = this.wrapPropertyAccessInVariableDeclaration(variableName, converted);
    }
    let replacementText = '';

    switch (node.kind) {
      case ts.SyntaxKind.PrefixUnaryExpression: {
        const assign = this.createSetProperty(
          expression.getText(),
          name.text,
          ts.factory.createIdentifier(variableName)
        );
        replacementText = `${convertedAssigned}\n${varAssignText}\n${assign}\n`;
        break;
      }
      case ts.SyntaxKind.PostfixUnaryExpression: {
        const assign = this.createSetProperty(expression.getText(), name.text, addOrDecrOperation as ts.Expression);
        replacementText = `${convertedAssigned}\n${assign}\n${varAssignText}\n`;
        break;
      }
      default: {
        return undefined;
      }
    }

    return replacementText;
  }

  private wrapPropertyAccessInVariableDeclaration(variableName: string, wrappedNode: ts.Node): string {
    const node = ts.factory.createVariableDeclarationList(
      [
        ts.factory.createVariableDeclaration(
          ts.factory.createIdentifier(variableName),
          undefined,
          undefined,
          wrappedNode as ts.Expression
        )
      ],
      ts.NodeFlags.Let
    );

    return this.printer.printNode(ts.EmitHint.Unspecified, node, this.sourceFile);
  }

  private wrapPropertyAccessInBinaryExpr(variableName: string, wrappedNode: ts.Node): string {
    const node = ts.factory.createBinaryExpression(
      ts.factory.createIdentifier(variableName),
      ts.SyntaxKind.EqualsToken,
      wrappedNode as ts.Expression
    );

    return this.printer.printNode(ts.EmitHint.Unspecified, node, this.sourceFile);
  }

  private createGetPropertyForIncrDecr(expression: string, name: string): ts.Node {
    void this;
    return ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(
        ts.factory.createCallExpression(
          ts.factory.createPropertyAccessExpression(
            ts.factory.createIdentifier(expression),
            ts.factory.createIdentifier(GET_PROPERTY)
          ),
          undefined,
          [ts.factory.createStringLiteral(name)]
        ),
        ts.factory.createIdentifier(TO_NUMBER)
      ),
      undefined,
      []
    );
  }

  private createNewIncrDecrNodes(variableName: string, token: number): IncrementDecrementNodeInfo {
    const update = ts.factory.createBinaryExpression(
      ts.factory.createIdentifier(variableName),
      ts.factory.createToken(token),
      ts.factory.createNumericLiteral('1')
    );

    const node = ts.factory.createBinaryExpression(
      ts.factory.createIdentifier(variableName),
      ts.factory.createToken(ts.SyntaxKind.EqualsToken),
      update
    );

    return {
      addOrDecrOperation: update,
      varAssignText: this.printer.printNode(ts.EmitHint.Unspecified, node, this.sourceFile)
    };
  }

  private createSetProperty(expression: string, field: string, value: ts.Expression): string {
    const node = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(
        ts.factory.createIdentifier(expression),
        ts.factory.createIdentifier(SET_PROPERTY)
      ),
      undefined,
      [ts.factory.createIdentifier(field), value]
    );

    return this.printer.printNode(ts.EmitHint.Unspecified, node, this.sourceFile);
  }

  fixNumericSemanticsForDeclaration(
    node: ts.VariableDeclaration | ts.PropertyDeclaration | ts.ParameterDeclaration,
    type: ts.Type
  ): Autofix[] | undefined {
    const typeNode = this.getTypeForNumericSemantics(type);
    const text = ': ' + this.printer.printNode(ts.EmitHint.Unspecified, typeNode, node.getSourceFile());
    const pos = Autofixer.getDeclarationTypePosition(node);
    return [{ start: pos, end: pos, replacementText: text }];
  }

  private getTypeForNumericSemantics(type: ts.Type): ts.TypeNode {
    if (this.utils.isGenericArrayType(type)) {
      return ts.factory.createArrayTypeNode(ts.factory.createKeywordTypeNode(ts.SyntaxKind.NumberKeyword));
    }
    return ts.factory.createKeywordTypeNode(ts.SyntaxKind.NumberKeyword);
  }

  /**
   * Transforms a call expression invoking an imported function or method into its interop equivalent.
   * - For direct calls like foo() or bar(123), transforms to foo.invoke() or bar.invoke(ESValue.wrap(123))
   * - For property access calls like foo.bar(123), transforms to foo.invokeMethod('bar', ESValue.wrap(123))
   * @param expression The call expression node to transform.
   * @returns Autofix array or undefined.
   */
  fixInteropInvokeExpression(expression: ts.CallExpression): Autofix[] | undefined {
    const callee = expression.expression;
    const args = this.createArgs(expression.arguments);

    let replacement: ts.CallExpression;

    if (ts.isPropertyAccessExpression(callee)) {
      // For expressions like foo.bar(123) => foo.invokeMethod('bar', ...)
      replacement = ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(callee.expression, ts.factory.createIdentifier(INVOKE_METHOD)),
        undefined,
        [ts.factory.createStringLiteral(callee.name.getText()), ...args || []]
      );
    } else if (ts.isIdentifier(callee)) {
      // For expressions like foo() or bar(123) => foo.invoke(...) or bar.invoke(...)
      replacement = ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(callee, ts.factory.createIdentifier(INVOKE)),
        undefined,
        args
      );
    } else {
      return undefined;
    }

    const replacementText = this.nonCommentPrinter.printNode(
      ts.EmitHint.Unspecified,
      replacement,
      expression.getSourceFile()
    );
    return [{ start: expression.getStart(), end: expression.getEnd(), replacementText }];
  }

  /**
   * Convert a JS-imported `for...of` over an array into an indexed `for` loop.
   *
   * @param forOf - The `ForOfStatement` node to transform.
   */
  applyForOfJsArrayFix(forOf: ts.ForOfStatement): Autofix[] | undefined {
    const loopDecl = (forOf.initializer as ts.VariableDeclarationList).declarations[0];
    const elementName = (loopDecl.name as ts.Identifier).text;

    const indexName = TsUtils.generateUniqueName(this.tmpVariableNameGenerator, forOf.getSourceFile()) ?? '_i';

    const fixes: Autofix[] = [];
    fixes.push(this.buildForOfHeaderFix(forOf, indexName));
    fixes.push(...this.buildForOfBodyFixes(forOf, elementName, indexName));
    return fixes.length > 0 ? fixes : undefined;
  }

  /**
   * Build an Autofix for replacing the for-of loop header.
   */
  private buildForOfHeaderFix(forOf: ts.ForOfStatement, indexName: string): Autofix {
    const arrText = forOf.expression.getText();

    const initializer = ts.factory.createVariableDeclarationList(
      [
        ts.factory.createVariableDeclaration(
          ts.factory.createIdentifier(indexName),
          undefined,
          undefined,
          ts.factory.createNumericLiteral('0')
        )
      ],
      ts.NodeFlags.Let
    );

    // condition: i < arr.getProperty('length').toNumber()
    const condition = ts.factory.createBinaryExpression(
      ts.factory.createIdentifier(indexName),
      ts.SyntaxKind.LessThanToken,
      ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(
          ts.factory.createCallExpression(
            ts.factory.createPropertyAccessExpression(
              ts.factory.createIdentifier(arrText),
              ts.factory.createIdentifier(GET_PROPERTY)
            ),
            undefined,
            [ts.factory.createStringLiteral(LENGTH, true)]
          ),
          ts.factory.createIdentifier('toNumber')
        ),
        undefined,
        []
      )
    );

    const incrementor = ts.factory.createPrefixUnaryExpression(
      ts.SyntaxKind.PlusPlusToken,
      ts.factory.createIdentifier(indexName)
    );

    return this.createReplacementTextForOfHeader(forOf, initializer, condition, incrementor);
  }

  private createReplacementTextForOfHeader(
    forOf: ts.ForOfStatement,
    initializer: ts.VariableDeclarationList,
    condition: ts.BinaryExpression,
    incrementor: ts.PrefixUnaryExpression
  ): Autofix {
    // Render just the "(initializer; condition; incrementor)" text:
    const replacementText = [
      this.printer.printNode(ts.EmitHint.Unspecified, initializer, forOf.getSourceFile()),
      '; ',
      this.printer.printNode(ts.EmitHint.Unspecified, condition, forOf.getSourceFile()),
      '; ',
      this.printer.printNode(ts.EmitHint.Unspecified, incrementor, forOf.getSourceFile())
    ].join('');

    return { start: forOf.initializer.getStart(), end: forOf.expression.getEnd(), replacementText };
  }

  /**
   * Build fixes for replacing loop-variable references inside the for-of body.
   */
  private buildForOfBodyFixes(forOf: ts.ForOfStatement, elementName: string, indexName: string): Autofix[] {
    const fixes: Autofix[] = [];
    const arrExpr = forOf.expression;

    const visit = (node: ts.Node): void => {
      if (ts.isIdentifier(node) && node.text === elementName) {
        const callExpr = ts.factory.createCallExpression(
          ts.factory.createPropertyAccessExpression(
            ts.factory.createIdentifier(arrExpr.getText()),
            ts.factory.createIdentifier(GET_PROPERTY)
          ),
          undefined,
          [ts.factory.createIdentifier(indexName)]
        );

        // Print and append proper conversion suffix
        const printed = this.printer.printNode(ts.EmitHint.Unspecified, callExpr, forOf.getSourceFile());
        const replacementText = printed + this.utils.findTypeOfNodeForConversion(node);

        fixes.push({ start: node.getStart(), end: node.getEnd(), replacementText });
      }
      ts.forEachChild(node, visit);
    };

    visit(forOf.statement);
    return fixes;
  }

  fixInteropInstantiateExpression(
    express: ts.NewExpression,
    args: ts.NodeArray<ts.Expression> | undefined
  ): Autofix[] | undefined {
    const statements = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(
        ts.factory.createIdentifier(express.expression.getText()),
        ts.factory.createIdentifier(INSTANTIATE)
      ),
      undefined,
      this.createArgs(args)
    );
    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, statements, express.getSourceFile());
    return [{ start: express.getStart(), end: express.getEnd(), replacementText: replacementText }];
  }

  createArgs(args: ts.NodeArray<ts.Expression> | undefined): ts.Expression[] | undefined {
    void this;
    if (args && args.length > 0) {
      return args.map((arg) => {
        return ts.factory.createCallExpression(
          ts.factory.createPropertyAccessExpression(
            ts.factory.createIdentifier(ES_VALUE),
            ts.factory.createIdentifier(WRAP)
          ),
          undefined,
          [ts.factory.createIdentifier(arg.getText())]
        );
      });
    }
    return undefined;
  }

  fixFunctionDeclarationly(
    callExpr: ts.CallExpression,
    resolvedTypeArgs: ts.NodeArray<ts.TypeNode>
  ): Autofix[] | undefined {
    if (callExpr.typeArguments && callExpr.typeArguments.length > 0) {
      return undefined;
    }
    const newCallExpr = ts.factory.createCallExpression(callExpr.expression, resolvedTypeArgs, callExpr.arguments);
    const text = this.printer.printNode(ts.EmitHint.Unspecified, newCallExpr, callExpr.getSourceFile());
    return [
      {
        start: callExpr.getStart(),
        end: callExpr.getEnd(),
        replacementText: text
      }
    ];
  }

  checkEnumMemberNameConflict(tsEnumMember: ts.EnumMember, autofix: Autofix[] | undefined): void {
    if (!autofix?.length) {
      return;
    }

    const parentEnum = tsEnumMember.parent;
    if (!this.hasNameConflict(parentEnum, tsEnumMember, autofix)) {
      return;
    }

    const existingNames = this.collectExistingNames(parentEnum, tsEnumMember);
    this.adjustAutofixNames(autofix, existingNames);
  }

  hasNameConflict(parentEnum: ts.EnumDeclaration, tsEnumMember: ts.EnumMember, autofix: Autofix[]): boolean {
    void this;
    return parentEnum.members.some((member) => {
      return (
        member !== tsEnumMember &&
        (ts.isStringLiteral(member.name) || member.name.getText() === autofix[0].replacementText)
      );
    });
  }

  collectExistingNames(parentEnum: ts.EnumDeclaration, tsEnumMember: ts.EnumMember): Set<string> {
    void this;
    return new Set(
      parentEnum.members.
        filter((m) => {
          return m !== tsEnumMember;
        }).
        map((m) => {
          const nameNode = m.name;
          if (ts.isStringLiteral(nameNode)) {
            const fix = this.fixLiteralAsPropertyNamePropertyName(nameNode);
            return fix?.[0]?.replacementText || nameNode.text;
          }
          return nameNode.getText();
        })
    );
  }

  adjustAutofixNames(autofix: Autofix[], existingNames: Set<string>): void {
    void this;
    const baseName = autofix[0].replacementText;
    let newName = baseName;
    let counter = 1;

    while (existingNames.has(newName)) {
      newName = `${baseName}_${counter++}`;
    }

    autofix.forEach((fix) => {
      fix.replacementText = newName;
    });
  }

  removeImport(ident: ts.Identifier, importSpecifier: ts.ImportSpecifier): Autofix[] | undefined {
    const namedImports = importSpecifier.parent;
    const importClause = namedImports.parent;
    const importDeclaration = importClause.parent;
    if (!importDeclaration || !importClause) {
      return undefined;
    }

    if (namedImports.elements.length === 1 && !importClause.name) {
      return this.removeNode(importDeclaration);
    }

    if (namedImports.elements.length <= 0) {
      return undefined;
    }

    const specifiers = namedImports.elements.filter((specifier) => {
      return specifier.name.text !== ident.text;
    });

    const newClause = ts.factory.createImportClause(
      importClause.isTypeOnly,
      importClause.name,
      ts.factory.createNamedImports(specifiers)
    );

    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, newClause, ident.getSourceFile());
    return [{ start: importClause.getStart(), end: importClause.getEnd(), replacementText }];
  }

  fixInterfaceImport(
    interfacesNeedToImport: Set<string>,
    interfacesAlreadyImported: Set<string>,
    file: ts.SourceFile
  ): Autofix[] {
    const importSpecifiers: ts.ImportSpecifier[] = [];
    interfacesNeedToImport.forEach((interfaceName) => {
      if (interfacesAlreadyImported.has(interfaceName)) {
        return;
      }
      const identifier = ts.factory.createIdentifier(interfaceName);
      importSpecifiers.push(ts.factory.createImportSpecifier(false, undefined, identifier));
    });
    const importDeclaration = ts.factory.createImportDeclaration(
      undefined,
      ts.factory.createImportClause(false, undefined, ts.factory.createNamedImports(importSpecifiers)),
      ts.factory.createStringLiteral(ARKUI_MODULE, true),
      undefined
    );

    const leadingComments = ts.getLeadingCommentRanges(file.getFullText(), 0);
    let annotationEndLine = 0;
    let annotationEndPos = 0;
    if (leadingComments && leadingComments.length > 0) {
      annotationEndPos = leadingComments[leadingComments.length - 1].end;
      annotationEndLine = file.getLineAndCharacterOfPosition(annotationEndPos).line;
    }

    const stmt = file?.statements[0];
    const isUseStaticAtStart = stmt ? Autofixer.checkUseStaticAtStart(stmt) : false;
    annotationEndLine = isUseStaticAtStart ? file.getLineAndCharacterOfPosition(stmt.end).line : annotationEndLine;
    annotationEndPos = isUseStaticAtStart ? stmt.end : annotationEndPos;

    let text = Autofixer.formatImportStatement(
      this.printer.printNode(ts.EmitHint.Unspecified, importDeclaration, file)
    );
    if (annotationEndPos !== 0) {
      text = this.getNewLine() + (isUseStaticAtStart ? '' : this.getNewLine()) + text;
    }

    const codeStartLine = isUseStaticAtStart ?
      annotationEndLine + 1 :
      file.getLineAndCharacterOfPosition(file.getStart()).line;
    for (let i = 2; i > codeStartLine - annotationEndLine; i--) {
      text = text + this.getNewLine();
    }

    return [{ start: annotationEndPos, end: annotationEndPos, replacementText: text }];
  }

  private static formatImportStatement(stmt: string): string {
    return stmt.replace(/\{([^}]+)\}/, (match, importList) => {
      const items = importList.split(',').map((item) => {
        return item.trim();
      });

      if (items.length > 1) {
        const formattedList = items.
          map((item) => {
            return `  ${item.trim()},`;
          }).
          join('\n');
        return `{\n${formattedList}\n}`;
      }
      return `{${importList}}`;
    });
  }

  private static checkUseStaticAtStart(stmt: ts.Statement): boolean {
    return stmt.getText().trim().
      replace(/^'|'$/g, '').
      endsWith(USE_STATIC_STATEMENT);
  }

  fixStylesDecoratorGlobal(
    funcDecl: ts.FunctionDeclaration,
    calls: ts.Identifier[],
    needImport: Set<string>
  ): Autofix[] | undefined {
    const block = funcDecl.body;
    const parameters: ts.MemberName[] = [];
    const values: ts.Expression[][] = [];
    const statements = block?.statements;
    Autofixer.getParamsAndValues(statements, parameters, values);
    const newBlock = Autofixer.createBlock(parameters, values, ts.factory.createIdentifier(INSTANCE_IDENTIFIER));
    const parameDecl = ts.factory.createParameterDeclaration(
      undefined,
      undefined,
      ts.factory.createIdentifier(INSTANCE_IDENTIFIER),
      undefined,
      ts.factory.createTypeReferenceNode(ts.factory.createIdentifier(COMMON_METHOD_IDENTIFIER), undefined),
      undefined
    );
    const returnType = ts.factory.createKeywordTypeNode(ts.SyntaxKind.VoidKeyword);
    const newFuncDecl = Autofixer.createFunctionDeclaration(funcDecl, undefined, parameDecl, returnType, newBlock);
    needImport.add(COMMON_METHOD_IDENTIFIER);
    const text = this.printer.printNode(ts.EmitHint.Unspecified, newFuncDecl, funcDecl.getSourceFile());
    const autofix = [{ start: funcDecl.getStart(), end: funcDecl.getEnd(), replacementText: text }];
    this.addAutofixFromCalls(calls, autofix, funcDecl.name as ts.Identifier);
    return autofix;
  }

  fixStylesDecoratorStruct(
    methodDecl: ts.MethodDeclaration,
    calls: ts.Identifier[],
    needImport: Set<string>
  ): Autofix[] | undefined {
    const block = methodDecl.body;
    const parameters: ts.MemberName[] = [];
    const values: ts.Expression[][] = [];
    const statements = block?.statements;
    const type = ts.factory.createTypeReferenceNode(
      ts.factory.createIdentifier(CustomInterfaceName.CustomStyles),
      undefined
    );
    Autofixer.getParamsAndValues(statements, parameters, values);
    const newBlock = Autofixer.createBlock(parameters, values, ts.factory.createIdentifier(INSTANCE_IDENTIFIER));
    const parameDecl = ts.factory.createParameterDeclaration(
      undefined,
      undefined,
      ts.factory.createIdentifier(INSTANCE_IDENTIFIER),
      undefined,
      ts.factory.createTypeReferenceNode(ts.factory.createIdentifier(COMMON_METHOD_IDENTIFIER), undefined),
      undefined
    );
    const arrowFunc = ts.factory.createArrowFunction(
      undefined,
      undefined,
      [parameDecl],
      ts.factory.createKeywordTypeNode(ts.SyntaxKind.VoidKeyword),
      ts.factory.createToken(ts.SyntaxKind.EqualsGreaterThanToken),
      newBlock
    );
    const newModifiers = ts.getModifiers(methodDecl)?.filter((modifier) => {
      return !(ts.isDecorator(modifier) && TsUtils.getDecoratorName(modifier) === CustomInterfaceName.Styles);
    });
    const expr = ts.factory.createPropertyDeclaration(newModifiers, methodDecl.name, undefined, type, arrowFunc);
    needImport.add(COMMON_METHOD_IDENTIFIER);
    let text = this.printer.printNode(ts.EmitHint.Unspecified, expr, methodDecl.getSourceFile());
    const startPos = this.sourceFile.getLineAndCharacterOfPosition(methodDecl.getStart()).character;
    text = this.adjustIndentation(text, startPos);
    const autofix = [{ start: methodDecl.getStart(), end: methodDecl.getEnd(), replacementText: text }];
    const argument = ts.factory.createPropertyAccessExpression(
      ts.factory.createThis(),
      methodDecl.name as ts.Identifier
    );
    this.addAutofixFromCalls(calls, autofix, argument);
    return autofix;
  }

  private adjustIndentation(text: string, startPos: number): string {
    const lines = text.split(this.getNewLine());
    if (lines.length <= 1) {
      return text;
    }

    const firstLine = lines[0];
    const secondLine = lines[1];
    const currentIndent = secondLine.match(/^\s*/)?.[0].length || 0;
    const indentBase = startPos - (currentIndent - INDENT_STEP);

    const middleLines = lines.slice(1, -1).map((line) => {
      if (indentBase > 0) {
        return ' '.repeat(indentBase) + line;
      }
      return line;
    });

    const lastLine = ' '.repeat(startPos) + lines[lines.length - 1];
    return [firstLine, ...middleLines, lastLine].join(this.getNewLine());
  }

  private addAutofixFromCalls(calls: ts.Identifier[], autofix: Autofix[], argument: ts.Expression): void {
    calls.forEach((call) => {
      const callExpr = ts.factory.createCallExpression(
        ts.factory.createIdentifier(APPLY_STYLES_IDENTIFIER),
        undefined,
        [argument]
      );

      const start: number = call.getStart();
      let end: number = 0;
      const expr = call.parent;
      if (ts.isCallExpression(expr)) {
        end = expr.getEnd();
      }
      if (ts.isPropertyAccessExpression(expr) && ts.isCallExpression(expr.parent)) {
        end = expr.parent.getEnd();
      }
      if (end === 0) {
        return;
      }
      const text = this.printer.printNode(ts.EmitHint.Unspecified, callExpr, call.getSourceFile());
      autofix.push({ start: start, end: end, replacementText: text });
    });
  }

  fixStateStyles(
    object: ts.ObjectLiteralExpression,
    startNode: ts.Node,
    needImport: Set<string>
  ): Autofix[] | undefined {
    const properties = object.properties;
    const assignments: ts.PropertyAssignment[] = [];
    for (let i = 0; i < properties.length; i++) {
      const property = properties[i];
      const stateStyle = property.name;
      if (!stateStyle || !ts.isIdentifier(stateStyle) || !ts.isPropertyAssignment(property)) {
        return undefined;
      }
      if (!ts.isObjectLiteralExpression(property.initializer)) {
        assignments.push(property);
        continue;
      }
      const propAssignments = property.initializer.properties;
      const parameters: ts.MemberName[] = [];
      const values: ts.Expression[][] = [];
      for (let j = 0; j < propAssignments.length; j++) {
        const propAssignment = propAssignments[j];
        const tempParas: ts.MemberName[] = [];
        const tempVals: ts.Expression[][] = [];
        Autofixer.traverseNodes(propAssignment, tempParas, tempVals);
        if (
          ts.isPropertyAssignment(propAssignment) &&
          ts.isCallExpression(propAssignment.initializer) &&
          ts.isPropertyAccessExpression(propAssignment.initializer.expression) &&
          ts.isCallExpression(propAssignment.initializer.expression.expression)
        ) {
          tempParas.reverse();
          tempVals.reverse();
        }
        for (let k = 0; k < tempParas.length; k++) {
          parameters.push(tempParas[k]);
          values.push(tempVals[k]);
        }
      }
      const assignment = Autofixer.createPropertyAssignment(parameters, values, stateStyle);
      assignments.push(assignment);
    }
    needImport.add(COMMON_METHOD_IDENTIFIER);
    const newExpr = ts.factory.createObjectLiteralExpression(assignments, true);
    let text = this.printer.printNode(ts.EmitHint.Unspecified, newExpr, object.getSourceFile());
    const startPos = this.sourceFile.getLineAndCharacterOfPosition(startNode.getStart()).character - 1;
    text = this.adjustIndentation(text, startPos);
    return [{ start: object.getStart(), end: object.getEnd(), replacementText: text }];
  }

  private static createPropertyAssignment(
    stateParam: ts.MemberName[],
    sateValue: ts.Expression[][],
    stateStyle: ts.Identifier
  ): ts.PropertyAssignment {
    const block = Autofixer.createBlock(stateParam, sateValue, ts.factory.createIdentifier(INSTANCE_IDENTIFIER));
    const parameterDecl = ts.factory.createParameterDeclaration(
      undefined,
      undefined,
      ts.factory.createIdentifier(INSTANCE_IDENTIFIER),
      undefined,
      ts.factory.createTypeReferenceNode(ts.factory.createIdentifier(COMMON_METHOD_IDENTIFIER), undefined),
      undefined
    );
    const voidToken = ts.factory.createToken(ts.SyntaxKind.VoidKeyword);
    const arrowToken = ts.factory.createToken(ts.SyntaxKind.EqualsGreaterThanToken);
    const property = ts.factory.createPropertyAssignment(
      stateStyle,
      ts.factory.createArrowFunction(undefined, undefined, [parameterDecl], voidToken, arrowToken, block)
    );
    return property;
  }

  fixDataObservation(classDecls: ts.ClassDeclaration[]): Autofix[] | undefined {
    const autofixes: Autofix[] = [];
    classDecls.forEach((classDecl) => {
      const observedDecorator = ts.factory.createDecorator(ts.factory.createIdentifier(CustomInterfaceName.Observed));
      const sourceFile = classDecl.getSourceFile();
      const text = this.printer.printNode(ts.EmitHint.Unspecified, observedDecorator, sourceFile) + this.getNewLine();
      const autofix = { start: classDecl.getStart(), end: classDecl.getStart(), replacementText: text };
      autofixes.push(autofix);
    });
    return autofixes.length !== 0 ? autofixes : undefined;
  }

  fixInteropTsType(
    binaryExpr: ts.BinaryExpression,
    lhs: ts.PropertyAccessExpression,
    rhs: ts.Expression
  ): Autofix[] | undefined {
    void this;
    const base = lhs.expression.getText();
    const prop = lhs.name.text;
    const replacementText = `${base}.setProperty('${prop}',ESValue.wrap(${rhs.getText()}))`;

    return [{ start: binaryExpr.getStart(), end: binaryExpr.getEnd(), replacementText }];
  }

  /**
   * Autofix for `foo instanceof Foo` → `foo.isInstanceOf(Foo)`.
   *
   * @param node The binary `instanceof` expression node.
   * @returns A single Autofix replacing the entire `foo instanceof Foo` text.
   */
  fixInteropJsInstanceOfExpression(node: ts.BinaryExpression): Autofix[] {
    // left-hand and right-hand operands of the `instanceof`
    const leftExpr = node.left;
    const rightExpr = node.right;

    // build: leftExpr.isInstanceOf(rightExpr)
    const callExpr = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(leftExpr, ts.factory.createIdentifier(IS_INSTANCE_OF)),
      undefined,
      [rightExpr]
    );

    // render back to source text
    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, callExpr, node.getSourceFile());

    return [{ replacementText, start: node.getStart(), end: node.getEnd() }];
  }

  createReplacementForJsIndirectImportPropertyAccessExpression(node: ts.PropertyAccessExpression): Autofix[] {
    // Bypass eslint-check
    void this;

    const objName = node.expression.getText();
    const propName = node.name.getText();

    let start = node.getStart();
    let end = node.getEnd();
    let replacementText = `${objName}.${GET_PROPERTY}('${propName}')`;

    // Check if there is an "as number" type assertion in the statement
    if (ts.isAsExpression(node.parent) && node.parent.type.kind === ts.SyntaxKind.NumberKeyword) {
      replacementText += '.toNumber()';
      start = node.parent.getStart();
      end = node.parent.getEnd();
    }

    return [{ replacementText, start, end }];
  }

  createReplacementForJsImportPropertyAccessExpression(node: ts.PropertyAccessExpression): Autofix[] {
    const objName = node.expression.getText();
    const propName = node.name.getText();

    const start = node.getStart();
    const end = node.getEnd();

    const typeTag = this.utils.findTypeOfNodeForConversion(node);
    const replacement = `${objName}.${GET_PROPERTY}('${propName}')${typeTag}`;

    return [{ replacementText: replacement, start, end }];
  }

  /**
   * Converts a JS element access (e.g. `arr[index]`) into the corresponding
   * interop call:
   *   - On assignment (`arr[index] = value`), emits `arr.setProperty(index, ESValue.wrap(value))`
   *   - On read, emits `arr.getProperty(index)` plus any type conversion suffix
   *
   * @param elementAccessExpr The original `ElementAccessExpression` node.
   * @returns An array with a single `Autofix` describing the replacement range and text.
   */
  fixJsImportElementAccessExpression(elementAccessExpr: ts.ElementAccessExpression): Autofix[] {
    const parent = elementAccessExpr.parent;

    const isAssignment =
      parent !== undefined && ts.isBinaryExpression(parent) && parent.operatorToken.kind === ts.SyntaxKind.EqualsToken;

    // array identifier (e.g. "arr")
    const identifierNode = elementAccessExpr.expression as ts.Identifier;

    let replacementText: string;
    if (isAssignment) {
      // arr.setProperty(index, ESValue.wrap(value))
      const wrapped = ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(
          ts.factory.createIdentifier(ES_VALUE),
          ts.factory.createIdentifier(WRAP)
        ),
        undefined,
        [parent.right]
      );

      const callExpr = ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(identifierNode, ts.factory.createIdentifier(SET_PROPERTY)),
        undefined,
        [elementAccessExpr.argumentExpression, wrapped]
      );

      replacementText = this.printer.printNode(ts.EmitHint.Unspecified, callExpr, elementAccessExpr.getSourceFile());
    } else {
      // arr.getProperty(index) plus conversion
      const callExpr = ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(identifierNode, ts.factory.createIdentifier(GET_PROPERTY)),
        undefined,
        [elementAccessExpr.argumentExpression]
      );

      replacementText =
        this.printer.printNode(ts.EmitHint.Unspecified, callExpr, elementAccessExpr.getSourceFile()) +
        this.utils.findTypeOfNodeForConversion(elementAccessExpr);
    }

    const start = isAssignment ? (parent as ts.Node).getStart() : elementAccessExpr.getStart();
    const end = isAssignment ? (parent as ts.Node).getEnd() : elementAccessExpr.getEnd();

    return [{ replacementText, start, end }];
  }

  fixConcurrencyLock(locksProp: ts.PropertyAccessExpression): Autofix[] | undefined {
    // 1) Ensure the next property is `.AsyncLock`
    const asyncLockProp = locksProp.parent;
    if (!ts.isPropertyAccessExpression(asyncLockProp)) {
      return undefined;
    }

    const className = asyncLockProp.name.getText();

    // 2a) Handle static method calls on AsyncLock (e.g. AsyncLock.request, AsyncLock.acquire, etc.)
    const methodProp = asyncLockProp.parent;
    if (ts.isPropertyAccessExpression(methodProp)) {
      const methodName = methodProp.name.getText();
      const callExpr = methodProp.parent;
      if (!ts.isCallExpression(callExpr)) {
        return undefined;
      }

      const fixes: Autofix[] = [];
      // Type fix for variable declaration, if necessary
      const varDecl = Autofixer.findParentVariableDeclaration(callExpr);
      const typeFix = varDecl ? this.buildConcurrencyLockTypeFix(varDecl, className) : undefined;
      if (typeFix) {
        fixes.push(typeFix);
      }

      // build AsyncLock.<methodName>(...)
      const fixCall = ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(
          ts.factory.createIdentifier(className),
          ts.factory.createIdentifier(methodName)
        ),
        undefined,
        callExpr.arguments
      );
      const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, fixCall, callExpr.getSourceFile());
      fixes.push({ start: callExpr.getStart(), end: callExpr.getEnd(), replacementText });
      return fixes;
    }

    // 2b) Handle `new ArkTSUtils.locks.AsyncLock()`
    const newExpr = asyncLockProp.parent;
    if (!ts.isNewExpression(newExpr)) {
      return undefined;
    }
    // build `new AsyncLock()`
    const replacement = ts.factory.createNewExpression(ts.factory.createIdentifier(className), undefined, []);
    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, replacement, newExpr.getSourceFile());
    return [{ start: newExpr.getStart(), end: newExpr.getEnd(), replacementText }];
  }

  /**
   * If the variable declaration has a qualified type `*.locks.ClassName`, build a fix to simplify it to `ClassName`.
   */
  private buildConcurrencyLockTypeFix(varDecl: ts.VariableDeclaration, className: string): Autofix | undefined {
    const typeNode = varDecl.type;

    if (!typeNode || !ts.isTypeReferenceNode(typeNode) || !ts.isQualifiedName(typeNode.typeName)) {
      return undefined;
    }

    const qn = typeNode.typeName;
    const left = qn.left;
    const right = qn.right;

    // ensure it's of form *.locks.ClassName
    if (ts.isQualifiedName(left) && left.right.text === ARKTSUTILS_LOCKS_MEMBER && right.text === className) {
      const simple = ts.factory.createTypeReferenceNode(ts.factory.createIdentifier(className), undefined);
      const text = this.printer.printNode(ts.EmitHint.Unspecified, simple, varDecl.getSourceFile());
      return { start: typeNode.getStart(), end: typeNode.getEnd(), replacementText: text };
    }

    return undefined;
  }

  private static findParentVariableDeclaration(node: ts.Node): ts.VariableDeclaration | undefined {
    while (node) {
      if (ts.isVariableDeclaration(node)) {
        return node;
      }
      node = node.parent;
    }
    return undefined;
  }

  fixAppStorageCallExpression(varDecl: ts.VariableDeclaration): Autofix[] | undefined {
    const typeNode = ts.factory.createKeywordTypeNode(ts.SyntaxKind.NumberKeyword);
    const text = ': ' + this.printer.printNode(ts.EmitHint.Unspecified, typeNode, varDecl.getSourceFile());
    const pos = Autofixer.getDeclarationTypePosition(varDecl);
    return [{ start: pos, end: pos, replacementText: text }];
  }

  private fixInterOpImportJsProcessNode(node: ts.Node): string | undefined {
    if (ts.isIdentifier(node)) {
      return node.text;
    } else if (ts.isCallExpression(node)) {
      const newArgs = this.createArgs(node.arguments);
      const callee = node.expression;
      switch (callee.kind) {
        case ts.SyntaxKind.PropertyAccessExpression: {
          const propertyAccessExpr = node.expression as ts.PropertyAccessExpression;
          const newCallExpr = this.createJSInvokeCallExpression(propertyAccessExpr.expression, INVOKE_METHOD, [
            ts.factory.createStringLiteral(propertyAccessExpr.name.text),
            ...newArgs || []
          ]);

          if (!newCallExpr) {
            return undefined;
          }
          return this.printer.printNode(ts.EmitHint.Unspecified, newCallExpr, node.getSourceFile());
        }
        default: {
          const callExpr = this.createJSInvokeCallExpression(node.expression, INVOKE, [...newArgs || []]);

          if (!callExpr) {
            return undefined;
          }

          return this.printer.printNode(ts.EmitHint.Unspecified, callExpr, node.getSourceFile());
        }
      }
    } else if (ts.isPropertyAccessExpression(node)) {
      const base = this.fixInterOpImportJsProcessNode(node.expression);
      if (!base) {
        return undefined;
      }
      const propName = node.name.text;
      return `${base}.${GET_PROPERTY}('${propName}')`;
    } else if (ts.isNewExpression(node)) {
      const newArgs = this.createArgs(node.arguments);
      const newCallExpr = this.createJSInvokeCallExpression(node.expression, INSTANTIATE, [...newArgs || []]);

      if (!newCallExpr) {
        return undefined;
      }
      return this.printer.printNode(ts.EmitHint.Unspecified, newCallExpr, node.getSourceFile());
    }
    return undefined;
  }

  fixInteropPropertyAccessExpression(express: ts.PropertyAccessExpression): Autofix[] | undefined {
    let text: string = '';
    const statements = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(express.expression, ts.factory.createIdentifier(GET_PROPERTY)),
      undefined,
      [ts.factory.createStringLiteral(express.name.getText())]
    );
    text = this.printer.printNode(ts.EmitHint.Unspecified, statements, express.getSourceFile());
    return [{ start: express.getStart(), end: express.getEnd(), replacementText: text }];
  }

  fixInteropBinaryExpression(express: ts.BinaryExpression): Autofix[] | undefined {
    const left = express.left;
    const right = express.right;
    let objectName = '';
    let propertyName = '';
    if (ts.isPropertyAccessExpression(left)) {
      objectName = left.expression.getText();
      propertyName = left.name.text;
    } else {
      return undefined;
    }
    const statements = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(
        ts.factory.createIdentifier(objectName),
        ts.factory.createIdentifier(SET_PROPERTY)
      ),
      undefined,
      [
        ts.factory.createStringLiteral(propertyName),
        ts.factory.createCallExpression(
          ts.factory.createPropertyAccessExpression(
            ts.factory.createIdentifier(ES_VALUE),
            ts.factory.createIdentifier(WRAP)
          ),
          undefined,
          [ts.factory.createIdentifier(right.getText())]
        )
      ]
    );
    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, statements, express.getSourceFile());
    return [{ start: express.getStart(), end: express.getEnd(), replacementText: replacementText }];
  }

  fixInteropAsExpression(expression: ts.AsExpression): Autofix[] | undefined {
    const castMap: Partial<Record<ts.SyntaxKind, string>> = {
      [ts.SyntaxKind.StringKeyword]: 'toString',
      [ts.SyntaxKind.NumberKeyword]: 'toNumber',
      [ts.SyntaxKind.BooleanKeyword]: 'toBoolean',
      [ts.SyntaxKind.BigIntKeyword]: 'toBigInt'
    };

    const castMethod = castMap[expression.type.kind];
    if (!castMethod) {
      return undefined;
    }
    const express = expression.expression;
    if (!ts.isPropertyAccessExpression(express)) {
      return undefined;
    }

    const propertyAccess = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(express.expression, ts.factory.createIdentifier(GET_PROPERTY)),
      undefined,
      [ts.factory.createStringLiteral(express.name.getText())]
    );

    const finalCall = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(propertyAccess, ts.factory.createIdentifier(castMethod)),
      undefined,
      []
    );

    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, finalCall, expression.getSourceFile());

    return [
      {
        start: expression.getStart(),
        end: expression.getEnd(),
        replacementText
      }
    ];
  }

  fixInteropOperators(expr: ts.Expression): Autofix[] | undefined {
    if (ts.isPropertyAccessExpression(expr)) {
      return this.fixPropertyAccessToNumber(expr);
    }

    if (ts.isIdentifier(expr)) {
      const symbol = this.utils.trueSymbolAtLocation(expr);

      if (this.utils.isJsImport(expr)) {
        const toNumberCall = ts.factory.createCallExpression(
          ts.factory.createPropertyAccessExpression(expr, ts.factory.createIdentifier(TO_NUMBER)),
          undefined,
          []
        );

        const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, toNumberCall, expr.getSourceFile());

        return [
          {
            start: expr.getStart(),
            end: expr.getEnd(),
            replacementText
          }
        ];
      }

      const decl = symbol?.declarations?.find(ts.isVariableDeclaration);
      if (decl?.initializer && ts.isPropertyAccessExpression(decl.initializer)) {
        return this.fixPropertyAccessToNumber(decl.initializer);
      }
    }

    return undefined;
  }

  private fixPropertyAccessToNumber(expr: ts.PropertyAccessExpression): Autofix[] {
    const getPropCall = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(expr.expression, ts.factory.createIdentifier(GET_PROPERTY)),
      undefined,
      [ts.factory.createStringLiteral(expr.name.getText())]
    );

    const toNumberCall = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(getPropCall, ts.factory.createIdentifier(TO_NUMBER)),
      undefined,
      []
    );

    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, toNumberCall, expr.getSourceFile());

    return [
      {
        start: expr.getStart(),
        end: expr.getEnd(),
        replacementText
      }
    ];
  }

  fixInteropArrayElementAccessExpression(express: ts.ElementAccessExpression): Autofix[] | undefined {
    const statements = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(express.expression, ts.factory.createIdentifier(GET_PROPERTY)),
      undefined,
      [express.argumentExpression]
    );
    const text = this.printer.printNode(ts.EmitHint.Unspecified, statements, express.getSourceFile());
    return [{ start: express.getStart(), end: express.getEnd(), replacementText: text }];
  }

  fixInteropArrayBinaryExpression(express: ts.BinaryExpression): Autofix[] | undefined {
    const left = express.left as ts.ElementAccessExpression;
    const right = express.right;
    const statements = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(
        ts.factory.createIdentifier(left.expression.getText()),
        ts.factory.createIdentifier(SET_PROPERTY)
      ),
      undefined,
      [
        left.argumentExpression,
        ts.factory.createCallExpression(
          ts.factory.createPropertyAccessExpression(
            ts.factory.createIdentifier(ES_VALUE),
            ts.factory.createIdentifier(WRAP)
          ),
          undefined,
          [ts.factory.createIdentifier(right.getText())]
        )
      ]
    );
    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, statements, express.getSourceFile());
    return [{ start: express.getStart(), end: express.getEnd(), replacementText: replacementText }];
  }

  fixInterOpImportJsOnTypeOf(typeofExpress: ts.TypeOfExpression): Autofix[] | undefined {
    const node = typeofExpress.expression;
    const start = typeofExpress.getStart();
    const end = typeofExpress.getEnd();
    const processed = this.fixInterOpImportJsProcessNode(node);
    if (!processed) {
      return undefined;
    }
    const replacementText = `${processed}.typeOf()`;
    return [{ start, end, replacementText }];
  }

  fixInteropInterfaceConvertNum(express: ts.PrefixUnaryExpression): Autofix[] | undefined {
    const createConversionExpression = (propertyAccess: ts.PropertyAccessExpression): ts.Expression => {
      const getPropertyCall = ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(
          ts.factory.createIdentifier(propertyAccess.expression.getText()),
          ts.factory.createIdentifier(GET_PROPERTY)
        ),
        undefined,
        [ts.factory.createStringLiteral(propertyAccess.name.getText())]
      );

      return ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(getPropertyCall, ts.factory.createIdentifier(TO_NUMBER)),
        undefined,
        []
      );
    };

    let replacementExpression: ts.Expression | undefined;
    if (ts.isPropertyAccessExpression(express.operand)) {
      replacementExpression = createConversionExpression(express.operand);
    } else if (
      ts.isParenthesizedExpression(express.operand) &&
      ts.isPropertyAccessExpression(express.operand.expression)
    ) {
      replacementExpression = ts.factory.createParenthesizedExpression(
        createConversionExpression(express.operand.expression)
      );
    }

    if (!replacementExpression) {
      return undefined;
    }

    const text = this.printer.printNode(ts.EmitHint.Unspecified, replacementExpression, express.getSourceFile());

    return [{ start: express.operand.getStart(), end: express.operand.getEnd(), replacementText: text }];
  }

  fixImportClause(tsImportClause: ts.ImportClause): Autofix[] {
    void this;
    const replacementText = tsImportClause.getText().replace(/\blazy\b\s*/, '');
    return [{ start: tsImportClause.getStart(), end: tsImportClause.getEnd(), replacementText }];
  }

  fixInteropEqualityOperator(
    tsBinaryExpr: ts.BinaryExpression,
    binaryOperator: ts.BinaryOperator
  ): Autofix[] | undefined {
    const text = this.replaceInteropEqualityOperator(tsBinaryExpr, binaryOperator);
    if (text) {
      return [{ start: tsBinaryExpr.getStart(), end: tsBinaryExpr.getEnd(), replacementText: text }];
    }
    return undefined;
  }

  createInteropPropertyAccess(exp: ts.Expression): string {
    let text: string;
    if (!ts.isPropertyAccessExpression(exp)) {
      text = exp.getText();
      return text;
    }
    const statements = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(exp.expression, ts.factory.createIdentifier(GET_PROPERTY)),
      undefined,
      [ts.factory.createStringLiteral(exp.name.getText())]
    );
    text = this.printer.printNode(ts.EmitHint.Unspecified, statements, exp.getSourceFile());
    return text;
  }

  replaceInteropEqualityOperator(
    tsBinaryExpr: ts.BinaryExpression,
    binaryOperator: ts.BinaryOperator
  ): string | undefined {
    const info = this.getInteropEqualityReplacementInfo(binaryOperator);
    if (!info) {
      return undefined;
    }

    const tsLhsExpr = tsBinaryExpr.left;
    const left = this.createInteropPropertyAccess(tsLhsExpr);
    const tsRhsExpr = tsBinaryExpr.right;
    const right = this.createInteropPropertyAccess(tsRhsExpr);
    if (!left || !right) {
      return undefined;
    }

    const callExpression = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(
        ts.factory.createIdentifier(left),
        ts.factory.createIdentifier(info.functionName)
      ),
      undefined,
      [ts.factory.createIdentifier(right)]
    );

    let text = this.printer.printNode(ts.EmitHint.Unspecified, callExpression, tsBinaryExpr.getSourceFile());
    if (info.isNegative) {
      text = '!' + text;
    }
    return text;
  }

  getInteropEqualityReplacementInfo(
    binaryOperator: ts.BinaryOperator
  ): { functionName: string; isNegative: boolean } | undefined {
    void this;
    switch (binaryOperator) {
      case ts.SyntaxKind.EqualsEqualsToken:
        return { functionName: ARE_EQUAL, isNegative: false };
      case ts.SyntaxKind.ExclamationEqualsToken:
        return { functionName: ARE_EQUAL, isNegative: true };
      case ts.SyntaxKind.EqualsEqualsEqualsToken:
        return { functionName: ARE_STRICTLY_EQUAL, isNegative: false };
      case ts.SyntaxKind.ExclamationEqualsEqualsToken:
        return { functionName: ARE_STRICTLY_EQUAL, isNegative: true };
      default:
        return undefined;
    }
  }

  fixArrayIndexExprType(argExpr: ts.Expression): Autofix[] | undefined {
    void this;
    if (ts.isAsExpression(argExpr)) {
      const innerExpr = argExpr.expression;
      return [
        {
          start: argExpr.getStart(),
          end: argExpr.getEnd(),
          replacementText: `${innerExpr ? innerExpr.getText() : ''} as int`
        }
      ];
    }

    if (ts.isBinaryExpression(argExpr)) {
      return [{ start: argExpr.getStart(), end: argExpr.getEnd(), replacementText: `(${argExpr.getText()}) as int` }];
    }

    return [{ start: argExpr.getStart(), end: argExpr.getEnd(), replacementText: `${argExpr.getText()} as int` }];
  }

  fixNoTsLikeFunctionCall(callExpr: ts.CallExpression): Autofix[] {
    void this;
    const expr = callExpr.expression;
    const hasOptionalChain = !!callExpr.questionDotToken;

    const replacementText = hasOptionalChain ?
      `${expr.getText()}${callExpr.questionDotToken.getText()}unsafeCall` :
      `${expr.getText()}.unsafeCall`;

    return [
      {
        start: expr.getStart(),
        end: hasOptionalChain ? callExpr.questionDotToken.getEnd() : expr.getEnd(),
        replacementText
      }
    ];
  }

  private static createBuiltInTypeInitializer(type: ts.TypeReferenceNode): ts.Expression | undefined {
    const typeName = type.typeName.getText();

    switch (typeName) {
      case 'Date':
        return ts.factory.createNewExpression(ts.factory.createIdentifier('Date'), undefined, []);
      case 'Map':
      case 'Set':
        return ts.factory.createNewExpression(ts.factory.createIdentifier(typeName), type.typeArguments, []);
      case 'Promise':
        return ts.factory.createIdentifier('undefined');
      default:
        return this.isNullableType(type) ? ts.factory.createIdentifier('undefined') : undefined;
    }
  }

  private static isNullableType(type: ts.TypeNode): boolean {
    if (type.kind === ts.SyntaxKind.UndefinedKeyword) {
      return true;
    }

    if (ts.isUnionTypeNode(type)) {
      return type.types.some((t) => {
        return t.kind === ts.SyntaxKind.UndefinedKeyword;
      });
    }

    return false;
  }

  private static createExactObjectInitializer(type: ts.TypeLiteralNode): ts.ObjectLiteralExpression {
    const properties = type.members.
      filter((member): member is ts.PropertySignature => {
        return ts.isPropertySignature(member);
      }).
      map((member) => {
        const initializer = Autofixer.createInitializerForPropertySignature(member);
        if (initializer) {
          return ts.factory.createPropertyAssignment(member.name, initializer);
        }
        return null;
      }).
      filter((property): property is ts.PropertyAssignment => {
        return property !== null;
      });

    return ts.factory.createObjectLiteralExpression(properties, true);
  }

  private static createInitializerForPropertySignature(member: ts.PropertySignature): ts.Expression | undefined {
    return member.type ? Autofixer.createTypeBasedInitializer(member.type) : undefined;
  }

  private static createTypeBasedInitializer(type?: ts.TypeNode): ts.Expression | undefined {
    if (!type) {
      return undefined;
    }

    switch (type.kind) {
      case ts.SyntaxKind.StringKeyword:
        return ts.factory.createStringLiteral('');
      case ts.SyntaxKind.TypeLiteral:
        return Autofixer.createExactObjectInitializer(type as ts.TypeLiteralNode);
      case ts.SyntaxKind.ArrayType:
        return ts.factory.createArrayLiteralExpression([], false);
      case ts.SyntaxKind.TypeReference:
        return Autofixer.createBuiltInTypeInitializer(type as ts.TypeReferenceNode);
      default:
        return this.isNullableType(type) ? ts.factory.createIdentifier('undefined') : undefined;
    }
  }

  private static isUserDefinedClass(type: ts.TypeReferenceNode): boolean {
    const builtInTypes = new Set(['Date', 'Array', 'Map', 'Set', 'Promise', 'RegExp', 'Function']);
    return !builtInTypes.has(type.typeName.getText());
  }

  /**
   * Fixes function declarations/expressions that return `void` as part of a union.
   * Replaces `void` with `undefined` in the return type,
   * replaces `return;` with `return undefined;`,
   * and adds `return undefined;` if the function has no returns.
   */
  fixLimitedVoidTypeFunction(fn: ts.FunctionLikeDeclaration): Autofix[] | undefined {
    const fixes: Autofix[] = [];
    const returnType = fn.type;
    if (!returnType || !ts.isUnionTypeNode(returnType) || !TsUtils.typeContainsVoid(returnType)) {
      return undefined;
    }

    const updatedTypes = returnType.types.map((t) => {
      return t.kind === ts.SyntaxKind.VoidKeyword ? ts.factory.createTypeReferenceNode(UNDEFINED_NAME) : t;
    });

    const newType = ts.factory.createUnionTypeNode(updatedTypes);
    fixes.push({
      start: returnType.getStart(),
      end: returnType.getEnd(),
      replacementText: this.printer.printNode(ts.EmitHint.Unspecified, newType, fn.getSourceFile())
    });

    let hasReturn = false;
    function visit(node: ts.Node): void {
      if (ts.isReturnStatement(node)) {
        hasReturn = true;
        if (!node.expression) {
          fixes.push({
            start: node.getStart(),
            end: node.getEnd(),
            replacementText: 'return undefined;'
          });
        }
      }
      ts.forEachChild(node, visit);
    }
    if (fn.body) {
      visit(fn.body);
      if (ts.isBlock(fn.body)) {
        const statements = fn.body.statements;
        const lastExpr = statements.length > 0 ? statements[statements.length - 1] : undefined;
        if (hasReturn && lastExpr && !ts.isReturnStatement(lastExpr) || !hasReturn) {
          const text = this.createUndefinedReturnStatement(fn, lastExpr);
          fixes.push({
            start: lastExpr ? lastExpr.getEnd() : fn.body.getEnd() - 1,
            end: lastExpr ? lastExpr.getEnd() : fn.body.getEnd() - 1,
            replacementText: text
          });
        }
      }
    }
    return fixes;
  }

  private createUndefinedReturnStatement(fn: ts.FunctionLikeDeclaration, lastExpr: ts.Statement | undefined): string {
    const returnStatement = ts.factory.createReturnStatement(ts.factory.createIdentifier(UNDEFINED_NAME));
    if (lastExpr) {
      const startPos = lastExpr.getStart();
      const lineAndCharacter = this.sourceFile.getLineAndCharacterOfPosition(startPos);
      const indent = lineAndCharacter.character;
      return (
        this.getNewLine() +
        ' '.repeat(indent) +
        this.printer.printNode(ts.EmitHint.Unspecified, returnStatement, fn.getSourceFile())
      );
    }
    const lineAndCharacter = this.sourceFile.getLineAndCharacterOfPosition(fn.getStart());
    const indent = lineAndCharacter.character + INDENT_STEP;
    return (
      this.getNewLine() +
      ' '.repeat(indent) +
      this.printer.printNode(ts.EmitHint.Unspecified, returnStatement, fn.getSourceFile()) +
      this.getNewLine() +
      ' '.repeat(lineAndCharacter.character)
    );
  }

  private fixGenericCallNoTypeArgsWithContextualType(
    node: ts.NewExpression | ts.CallExpression
  ): Autofix[] | undefined {
    const contextualType = this.typeChecker.getContextualType(node);
    if (!contextualType) {
      return undefined;
    }

    const typeArgs = this.getTypeArgumentsFromType(contextualType);
    if (typeArgs.length === 0) {
      return undefined;
    }
    const reference = typeArgs.map((arg) => {
      return ts.factory.createTypeReferenceNode(this.typeChecker.typeToString(arg));
    });
    return this.generateGenericTypeArgumentsAutofix(node, reference);
  }

  fixGenericCallNoTypeArgs(node: ts.NewExpression | ts.CallExpression): Autofix[] | undefined {
    const typeNode = this.getTypeNodeForNewExpression(node);
    if (!typeNode) {
      return this.fixGenericCallNoTypeArgsWithContextualType(node);
    }
    let nodeExpressionText = node.expression.getText();
    if (ts.isCallExpression(node)) {
      const returnTypeText = this.getReturnTypeFromMethodDeclaration(node);
      if (!returnTypeText) {
        return undefined;
      }
      nodeExpressionText = returnTypeText;
    }
    if (ts.isUnionTypeNode(typeNode)) {
      return this.fixGenericCallNoTypeArgsForUnionType(node, typeNode, nodeExpressionText);
    }
    if (ts.isArrayTypeNode(typeNode)) {
      return this.fixGenericCallNoTypeArgsForArrayType(node, typeNode);
    }
    if (!ts.isTypeReferenceNode(typeNode) || typeNode.typeName.getText() !== nodeExpressionText) {
      return undefined;
    }

    const srcFile = node.getSourceFile();
    const typeArgsText = this.printGenericCallTypeArgs(srcFile, typeNode.typeArguments);
    if (!typeArgsText) {
      return undefined;
    }
    const insertPos = node.expression.getEnd();
    return [{ start: insertPos, end: insertPos, replacementText: typeArgsText }];
  }

  fixGenericCallNoTypeArgsForUnknown(node: ts.CallExpression): Autofix[] | undefined {
    if (ts.isPropertyAccessExpression(node.parent)) {
      const insertPos = node.expression.getEnd();
      return [{ start: insertPos, end: insertPos, replacementText: '<Any>' }];
    }

    const typeNode = this.getTypeNodeForNewExpression(node);
    if (!typeNode) {
      return undefined;
    }

    const nodeExpressionText = ts.isVariableDeclaration(node.parent) ?
      this.getReturnTypeFromMethodDeclaration(node) ?? node.expression.getText() :
      node.expression.getText();

    if (ts.isUnionTypeNode(typeNode)) {
      const targetTypeRef = typeNode.types.find((t): t is ts.TypeReferenceNode => {
        return ts.isTypeReferenceNode(t) && t.typeName.getText() === nodeExpressionText;
      });
      if (!targetTypeRef) {
        return undefined;
      }
      return this.createFixWithTypeArgs(node, targetTypeRef.typeArguments);
    }

    if (ts.isTypeReferenceNode(typeNode) && typeNode.typeName.getText() === nodeExpressionText) {
      return this.createFixWithTypeArgs(node, typeNode.typeArguments);
    }

    return undefined;
  }

  private createFixWithTypeArgs(
    node: ts.CallExpression,
    typeArguments: ts.NodeArray<ts.TypeNode> | undefined
  ): Autofix[] | undefined {
    const typeArgsText = this.printGenericCallTypeArgs(node.getSourceFile(), typeArguments);
    return typeArgsText ?
      [
        {
          start: node.expression.getEnd(),
          end: node.expression.getEnd(),
          replacementText: typeArgsText
        }
      ] :
      undefined;
  }

  getReturnTypeFromMethodDeclaration(node: ts.CallExpression): string | undefined {
    if (ts.isPropertyAccessExpression(node.expression)) {
      const propertyAccess = node.expression;
      const methodName = propertyAccess.name.text;
      const receiver = propertyAccess.expression;
      const receiverType = this.typeChecker.getTypeAtLocation(receiver);
      const methodSymbol = this.typeChecker.getPropertyOfType(receiverType, methodName);

      if (!methodSymbol) {
        return undefined;
      }
      const methodDecl = methodSymbol.declarations?.[0] as ts.MethodSignature;
      if (!methodDecl.type || !ts.isTypeReferenceNode(methodDecl.type) && !ts.isUnionTypeNode(methodDecl.type)) {
        return undefined;
      }
      if (ts.isTypeReferenceNode(methodDecl.type)) {
        return methodDecl.type.typeName.getText();
      }
      if (ts.isUnionTypeNode(methodDecl.type)) {
        const typeNode = this.getTypeNodeForNewExpression(node);
        if (!typeNode) {
          return undefined;
        }
        const targetTypeName = this.getTargetTypeName(typeNode, methodDecl.type);

        if (ts.isUnionTypeNode(typeNode)) {
          return targetTypeName;
        }
        const targetTypeNode = methodDecl.type.types.find((type) => {
          return ts.isTypeReferenceNode(type) && type.typeName.getText() === targetTypeName;
        }) as ts.TypeReferenceNode;
        if (!targetTypeNode) {
          return undefined;
        }
        return targetTypeNode.typeName.getText();
      }
    }
    return undefined;
  }

  private getTargetTypeName(typeNode: ts.TypeNode, methodDeclType: ts.UnionTypeNode): string | undefined {
    if (ts.isTypeReferenceNode(typeNode)) {
      return typeNode.typeName.getText();
    }

    if (!ts.isUnionTypeNode(typeNode)) {
      return undefined;
    }

    const hasGenericType = methodDeclType.types.some((type) => {
      return this.isGenericTypeParameter(type);
    });
    const typeNames = new Set(typeNode.types.map(Autofixer.getTypeName).filter(Boolean));

    const candidateNames = hasGenericType ?
      typeNode.types.map(Autofixer.getTypeName) :
      methodDeclType.types.map(Autofixer.getTypeName);
    const targetTypeName = candidateNames.find((name) => {
      return name && name !== 'undefined' && (hasGenericType || typeNames.has(name));
    });
    return targetTypeName;
  }

  private isGenericTypeParameter(type: ts.TypeNode): boolean {
    const getType = this.typeChecker.getTypeFromTypeNode(type);
    return getType.isTypeParameter();
  }

  private static getTypeName(type: ts.TypeNode): string {
    if (ts.isTypeReferenceNode(type)) {
      return type.typeName.getText();
    }
    if (ts.isLiteralTypeNode(type)) {
      return type.literal.getText();
    }
    const typeNameMap = {
      [ts.SyntaxKind.StringKeyword]: 'string',
      [ts.SyntaxKind.NumberKeyword]: 'number',
      [ts.SyntaxKind.BooleanKeyword]: 'boolean',
      [ts.SyntaxKind.UndefinedKeyword]: 'undefined',
      [ts.SyntaxKind.NullKeyword]: 'null'
    };
    return typeNameMap[type.kind] || '';
  }

  private fixGenericCallNoTypeArgsForArrayType(
    node: ts.NewExpression | ts.CallExpression,
    arrayTypeNode: ts.ArrayTypeNode
  ): Autofix[] | undefined {
    const elementTypeNode = arrayTypeNode.elementType;
    const srcFile = node.getSourceFile();
    const typeArgsText = this.printGenericCallTypeArgs(srcFile, elementTypeNode);
    if (!typeArgsText) {
      return undefined;
    }
    const insertPos = node.expression.getEnd();
    return [{ start: insertPos, end: insertPos, replacementText: typeArgsText }];
  }

  private fixGenericCallNoTypeArgsForUnionType(
    node: ts.NewExpression | ts.CallExpression,
    unionType: ts.UnionTypeNode,
    nodeExpressionText: string
  ): Autofix[] | undefined {
    const matchingTypes = unionType.types.filter((type) => {
      return Autofixer.getTypeName(type) === nodeExpressionText;
    });

    if (matchingTypes.length === 1) {
      const matchingType = matchingTypes[0];
      if (ts.isTypeReferenceNode(matchingType)) {
        if (matchingType.typeArguments) {
          const srcFile = node.getSourceFile();
          const typeArgsText = this.printGenericCallTypeArgs(srcFile, matchingType.typeArguments);
          if (!typeArgsText) {
            return undefined;
          }
          const insertPos = node.expression.getEnd();
          return [{ start: insertPos, end: insertPos, replacementText: typeArgsText }];
        }
      } else {
        const insertPos = node.expression.getEnd();
        return [{ start: insertPos, end: insertPos, replacementText: `<${nodeExpressionText}>` }];
      }
    }
    return undefined;
  }

  private printGenericCallTypeArgs(
    sourceFile: ts.SourceFile,
    typeArg: ts.TypeNode | readonly ts.Node[] | undefined
  ): string | undefined {
    if (Array.isArray(typeArg)) {
      if (
        typeArg.some((arg) => {
          return !this.isTypeArgumentAccessible(sourceFile, arg as ts.TypeNode);
        })
      ) {
        return undefined;
      }
      return `<${typeArg.
        map((arg) => {
          ts.setEmitFlags(arg, ts.EmitFlags.SingleLine);
          return this.nonCommentPrinter.printNode(ts.EmitHint.Unspecified, arg, sourceFile);
        }).
        join(', ')}>`;
    }
    if (!typeArg || !this.isTypeArgumentAccessible(sourceFile, typeArg as ts.TypeNode)) {
      return undefined;
    }
    ts.setEmitFlags(typeArg as ts.TypeNode, ts.EmitFlags.SingleLine);
    return `<${this.nonCommentPrinter.printNode(ts.EmitHint.Unspecified, typeArg as ts.TypeNode, sourceFile)}>`;
  }

  private isTypeArgumentAccessible(sourceFile: ts.SourceFile, arg: ts.TypeNode): boolean {
    const type = this.typeChecker.getTypeFromTypeNode(arg);
    const symbol = type.symbol || type.aliasSymbol;
    const decl = TsUtils.getDeclaration(symbol);
    const fileName = decl?.getSourceFile().fileName;
    const isStdLib = isStdLibrarySymbol(symbol);
    const isMatchPathOnSourceAndDecl = decl && path.normalize(sourceFile.fileName) === path.normalize(fileName!);
    if (!decl || isMatchPathOnSourceAndDecl || isStdLib) {
      return true;
    }
    let result = false;
    ts.forEachChild(sourceFile, (node) => {
      if (ts.isImportDeclaration(node)) {
        if (node.importClause?.namedBindings && ts.isNamedImports(node.importClause.namedBindings)) {
          node.importClause.namedBindings.elements.some((element) => {
            result = this.isTypeArgImport(arg, element.name, symbol);
            return result;
          });
        }
        if (node.importClause?.name) {
          result = this.isTypeArgImport(arg, node.importClause.name, symbol);
        }
      }
      return result;
    });
    return result;
  }

  private isTypeArgImport(arg: ts.Node, importName: ts.Identifier, typeArgSym: ts.Symbol): boolean {
    if (arg.getText() !== importName.text) {
      return false;
    }
    const importSym = this.utils.trueSymbolAtLocation(importName);
    return importSym === typeArgSym;
  }

  private generateGenericTypeArgumentsAutofix(
    node: ts.NewExpression | ts.CallExpression,
    typeArgs: ts.TypeReferenceNode[]
  ): Autofix[] | undefined {
    const srcFile = node.getSourceFile();
    const identifier = node.expression;
    const hasValidArgs = typeArgs.some((arg) => {
      return arg?.typeName && ts.isIdentifier(arg.typeName);
    });
    if (!hasValidArgs) {
      return undefined;
    }
    const hasAnyType = typeArgs.some((arg) => {
      return ts.isIdentifier(arg?.typeName) && arg.typeName.text === 'any';
    });
    if (hasAnyType) {
      return undefined;
    }
    const typeArgsText = this.printGenericCallTypeArgs(srcFile, typeArgs);
    if (!typeArgsText) {
      return undefined;
    }
    return [{ start: identifier.getEnd(), end: identifier.getEnd(), replacementText: typeArgsText }];
  }

  private getTypeArgumentsFromType(type: ts.Type): ts.Type[] {
    const typeReference = this.utils.getNonNullableType(type) as ts.TypeReference;
    if (typeReference.typeArguments) {
      return [...typeReference.typeArguments];
    }
    return [];
  }

  private getTypeNodeForNewExpression(node: ts.NewExpression | ts.CallExpression): ts.TypeNode | undefined {
    if (ts.isVariableDeclaration(node.parent) || ts.isPropertyDeclaration(node.parent)) {
      return node.parent.type;
    } else if (ts.isBinaryExpression(node.parent)) {
      return this.utils.getDeclarationTypeNode(node.parent.left);
    } else if (ts.isReturnStatement(node.parent) && ts.isBlock(node.parent.parent)) {
      const funcNode = node.parent.parent.parent;
      const isFunc = ts.isFunctionDeclaration(funcNode) || ts.isMethodDeclaration(funcNode);
      if (!isFunc || !funcNode.type) {
        return undefined;
      }

      const isAsync = TsUtils.hasModifier(funcNode.modifiers, ts.SyntaxKind.AsyncKeyword);
      if (isAsync) {
        if (ts.isTypeReferenceNode(funcNode.type) && funcNode.type.typeName.getText() === 'Promise') {
          return funcNode.type?.typeArguments?.[0];
        }
      }
      return funcNode.type;
    }
    return undefined;
  }

  private createJSInvokeCallExpression(
    ident: ts.Expression,
    method: string,
    args: ts.Expression[] | undefined
  ): ts.CallExpression | undefined {
    if (ts.isNewExpression(ident)) {
      const instantiatedClass = this.createJSInvokeCallExpression(
        ident.expression,
        INSTANTIATE,
        this.createArgs(ident.arguments)
      );
      if (!instantiatedClass) {
        return undefined;
      }
      return this.createJSInvokeCallExpression(instantiatedClass, method, args);
    }
    return ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(ident, ts.factory.createIdentifier(method)),
      undefined,
      args
    );
  }

  fixAwaitJsCallExpression(ident: ts.Identifier, args: ts.NodeArray<ts.Expression> | undefined): Autofix[] | undefined {
    const newArgs = this.createArgs(args);

    const newCallExpr = this.createJSInvokeCallExpression(ident, INVOKE, newArgs);
    if (!newCallExpr) {
      return undefined;
    }

    const replacedNode = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(newCallExpr, ts.factory.createIdentifier(TO_PROMISE)),
      undefined,
      undefined
    );

    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, replacedNode, ident.getSourceFile());
    return [{ start: ident.parent.getStart(), end: ident.parent.getEnd(), replacementText }];
  }

  fixAwaitJsMethodCallExpression(
    ident: ts.Identifier,
    args: ts.NodeArray<ts.Expression> | undefined
  ): Autofix[] | undefined {
    const propertyAccessExpr = ident.parent as ts.PropertyAccessExpression;
    const accessedProperty = propertyAccessExpr.expression;
    const newArgs = this.createArgs(args);

    const newCallExpr = this.createJSInvokeCallExpression(accessedProperty, INVOKE_METHOD, [
      ts.factory.createStringLiteral(ident.text),
      ...newArgs || []
    ]);

    if (!newCallExpr) {
      return undefined;
    }

    const replacedNode = ts.factory.createCallExpression(
      ts.factory.createPropertyAccessExpression(newCallExpr, ts.factory.createIdentifier(TO_PROMISE)),
      undefined,
      undefined
    );

    const replacementText = this.printer.printNode(ts.EmitHint.Unspecified, replacedNode, ident.getSourceFile());
    return [{ start: propertyAccessExpr.parent.getStart(), end: propertyAccessExpr.parent.getEnd(), replacementText }];
  }

  fixAwaitJsPromise(ident: ts.Identifier): Autofix[] {
    void this;
    const replacementText = `${ident.text}.toPromise()`;
    return [{ start: ident.getStart(), end: ident.getEnd(), replacementText }];
  }

  fixMissingAttribute(node: ts.PropertyAccessExpression): Autofix[] {
    const exprName = node.expression.getText();
    const propertyAccessExpr = ts.factory.createPropertyAccessExpression(
      ts.factory.createIdentifier(exprName),
      ts.factory.createIdentifier(METHOD_KEYS)
    );
    const replacement = this.printer.printNode(ts.EmitHint.Unspecified, propertyAccessExpr, node.getSourceFile());
    return [{ start: node.getStart(), end: node.getEnd(), replacementText: replacement }];
  }

  fixBuilderDecorators(decorator: ts.Decorator): Autofix[] | undefined {
    const newDecorator = ts.factory.createDecorator(ts.factory.createIdentifier('Builder'));
    const text = this.printer.printNode(ts.EmitHint.Unspecified, newDecorator, decorator.getSourceFile());
    return [{ start: decorator.getStart(), end: decorator.getEnd(), replacementText: text }];
  }

  fixCustomLayout(node: ts.StructDeclaration): Autofix[] {
    const startPos = Autofixer.getStartPositionWithoutDecorators(node);
    const decorator = ts.factory.createDecorator(ts.factory.createIdentifier(CustomInterfaceName.CustomLayout));

    const text = this.getNewLine() + this.printer.printNode(ts.EmitHint.Unspecified, decorator, node.getSourceFile());
    return [{ start: startPos, end: startPos, replacementText: text }];
  }

  private static getStartPositionWithoutDecorators(node: ts.StructDeclaration): number {
    const decorators = ts.getDecorators(node);
    if (!decorators || decorators.length === 0) {
      return node.getStart();
    }

    return decorators[decorators.length - 1].getEnd();
  }

  fixNumericLiteralIntToNumber(node: ts.NumericLiteral): Autofix[] | undefined {
    void this;
    let replacementText = node.getText();
    const parent = node.parent;

    if (ts.isPrefixUnaryExpression(parent) && parent.operator === ts.SyntaxKind.MinusToken) {
      replacementText = `-${replacementText}.0`;
      return [
        {
          start: parent.getStart(),
          end: node.getEnd(),
          replacementText
        }
      ];
    }

    return [
      {
        start: node.getStart(),
        end: node.getEnd(),
        replacementText: `${replacementText}.0`
      }
    ];
  }

  fixPropDecorator(node: ts.Identifier, decoratorName: string): Autofix[] {
    const newDecorator = ts.factory.createIdentifier(decoratorName + NEW_PROP_DECORATOR_SUFFIX);

    const text = this.printer.printNode(ts.EmitHint.Unspecified, newDecorator, node.getSourceFile());
    return [{ start: node.getStart(), end: node.getEnd(), replacementText: text }];
  }

  importBitVector(node: ts.Node, lastImportDeclaration: ts.Node | undefined): Autofix | undefined {
    const importDeclaration = ts.factory.createImportDeclaration(
      undefined,
      ts.factory.createImportClause(
        false,
        undefined,
        ts.factory.createNamedImports([
          ts.factory.createImportSpecifier(false, undefined, ts.factory.createIdentifier(BIT_VECTOR))
        ])
      ),
      ts.factory.createStringLiteral(ARKTS_COLLECTIONS_MODULE),
      undefined
    );
    // find the last import statement;

    let replacementText: string = this.nonCommentPrinter.printNode(
      ts.EmitHint.Unspecified,
      importDeclaration,
      node.getSourceFile()
    );
    let start: number = 0;
    let end: number = 0;

    if (lastImportDeclaration) {
      start = end = lastImportDeclaration.getEnd();
      replacementText = this.getNewLine() + replacementText + this.getNewLine();
    }

    return { start, end, replacementText };
  }

  fixRepeat(stmt: ts.ExpressionStatement): Autofix[] {
    const newExpr = ts.factory.createCallExpression(ts.factory.createIdentifier(VIRTUAL_SCROLL_IDENTIFIER), undefined, [
      ts.factory.createObjectLiteralExpression(
        [
          ts.factory.createPropertyAssignment(
            ts.factory.createIdentifier(DISABLE_VIRTUAL_SCROLL_IDENTIFIER),
            ts.factory.createTrue()
          )
        ],
        false
      )
    ]);

    let identifier: ts.Identifier | undefined;
    const expression = stmt.expression;
    if (
      ts.isCallExpression(expression) &&
      ts.isPropertyAccessExpression(expression.expression) &&
      ts.isIdentifier(expression.expression.name)
    ) {
      identifier = expression.expression.name;
    }

    const startPos = identifier ? identifier.getStart() : stmt.getStart();
    const lineAndCharacter = this.sourceFile.getLineAndCharacterOfPosition(startPos);
    const indent = identifier ? lineAndCharacter.character - 1 : lineAndCharacter.character + INDENT_STEP;
    const text =
      this.getNewLine() +
      ' '.repeat(indent) +
      '.' +
      this.printer.printNode(ts.EmitHint.Unspecified, newExpr, stmt.getSourceFile());
    return [{ start: stmt.getEnd(), end: stmt.getEnd(), replacementText: text }];
  }

  fixDeprecatedApiForCallExpression(callExpr: ts.CallExpression): Autofix[] | undefined {
    const createUIContextAccess = (methodName: string): ts.Node => {
      return ts.factory.createPropertyAccessExpression(
        ts.factory.createCallExpression(ts.factory.createIdentifier('getUIContext'), undefined, []),
        ts.factory.createIdentifier(methodName)
      );
    };

    if (ts.isPropertyAccessExpression(callExpr.expression)) {
      const fullName = `${callExpr.expression.expression.getText()}.${callExpr.expression.name.getText()}`;
      const methodName = propertyAccessReplacements.get(fullName);
      if (methodName) {
        const newExpression = createUIContextAccess(methodName);
        const newText = this.printer.printNode(ts.EmitHint.Unspecified, newExpression, callExpr.getSourceFile());
        return [
          {
            start: callExpr.expression.getStart(),
            end: callExpr.expression.getEnd(),
            replacementText: newText
          }
        ];
      }
    }

    if (ts.isIdentifier(callExpr.expression)) {
      const identifierText = callExpr.expression.getText();
      const methodName = identifierReplacements.get(identifierText);

      if (methodName) {
        const newExpression = createUIContextAccess(methodName);
        const newText = this.printer.printNode(ts.EmitHint.Unspecified, newExpression, callExpr.getSourceFile());
        return [
          {
            start: callExpr.expression.getStart(),
            end: callExpr.expression.getEnd(),
            replacementText: newText
          }
        ];
      }
    }

    return undefined;
  }

  fixSpecialDeprecatedApiForCallExpression(callExpr: ts.CallExpression, name: ts.Identifier): Autofix[] | undefined {
    if (name.getText() !== 'clip' || !ts.isNewExpression(callExpr.arguments[0])) {
      return undefined;
    }

    const shapeMap = {
      Circle: 'CircleShape',
      Path: 'PathShape',
      Ellipse: 'EllipseShape',
      Rect: 'RectShape'
    } as const;

    const argsExpr = callExpr.arguments[0].arguments;
    const constructorName = callExpr.arguments[0].expression.getText();

    if (
      ts.isPropertyAccessExpression(callExpr.expression) &&
      callExpr.expression.name.text === 'clip' &&
      constructorName in shapeMap
    ) {
      const shapeType = shapeMap[constructorName as keyof typeof shapeMap];
      const newExpression = ts.factory.createCallExpression(
        ts.factory.createPropertyAccessExpression(
          callExpr.expression.expression,
          ts.factory.createIdentifier('clipShape')
        ),
        undefined,
        [ts.factory.createNewExpression(ts.factory.createIdentifier(shapeType), undefined, argsExpr)]
      );

      const newText = this.printer.printNode(ts.EmitHint.Unspecified, newExpression, callExpr.getSourceFile());
      return [
        {
          start: callExpr.getStart(),
          end: callExpr.getEnd(),
          replacementText: newText
        }
      ];
    }

    return undefined;
  }

  fixSideEffectImport(importDeclNode: ts.ImportDeclaration): Autofix[] {
    const initModuleCall = ts.factory.createCallExpression(ts.factory.createIdentifier('initModule'), undefined, [
      importDeclNode.moduleSpecifier
    ]);
    const expressionStatement = ts.factory.createExpressionStatement(initModuleCall);
    const replacedText = this.printer.printNode(
      ts.EmitHint.Unspecified,
      expressionStatement,
      importDeclNode.getSourceFile()
    );

    return [
      {
        start: importDeclNode.getStart(),
        end: importDeclNode.getEnd(),
        replacementText: replacedText
      }
    ];
  }

  fixNumericLiteralToFloat(node: ts.NumericLiteral): Autofix[] {
    void this;
    return [
      {
        start: node.getStart(),
        end: node.getEnd(),
        replacementText: `${node.text}.0`
      }
    ];
  }
}
