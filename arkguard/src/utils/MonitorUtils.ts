/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as ts from 'typescript';

export enum MonitorDiagnosticCode {
  UNSUPPORTED_EXPRESSION = 'MONITOR_UNSUPPORTED_EXPRESSION',
  INVALID_PATH = 'MONITOR_INVALID_PATH',
  INVALID_OPTIONS = 'MONITOR_INVALID_OPTIONS',
  MISSING_PATH = 'MONITOR_MISSING_PATH'
}

export interface MonitorDiagnostic {
  node: ts.Node;
  code: MonitorDiagnosticCode;
  message: string;
}

export interface MonitorProperties {
  decorator: string;
  properties: Set<string>;
  diagnostics: MonitorDiagnostic[];
}

// Set by the build integration and cleared with the project's global state.
// Standalone callers without a logger still receive warnings on the console.
let monitorDiagnosticLogger: ((message: string) => void) | undefined;

export function setMonitorDiagnosticLogger(logger: ((message: string) => void) | undefined): void {
  monitorDiagnosticLogger = logger;
}

// This evaluator deliberately does not resolve identifiers or execute user code.
// Only string concatenation is supported; literal numbers/booleans/null are
// converted to strings only in template substitutions, where JS does so as well.
function evaluateString(node: ts.Expression, depth: number = 0): string | undefined {
  if (depth > 64) {
    return undefined;
  }
  if (ts.isStringLiteralLike(node)) {
    return node.text;
  }
  if (ts.isParenthesizedExpression(node)) {
    return evaluateString(node.expression, depth + 1);
  }
  if (ts.isBinaryExpression(node) && node.operatorToken.kind === ts.SyntaxKind.PlusToken) {
    const left = evaluateString(node.left, depth + 1);
    const right = evaluateString(node.right, depth + 1);
    return left === undefined || right === undefined ? undefined : left + right;
  }
  if (ts.isTemplateExpression(node)) {
    let value = node.head.text;
    for (const span of node.templateSpans) {
      const substitution = evaluateTemplateValue(span.expression, depth + 1);
      if (substitution === undefined) {
        return undefined;
      }
      value += substitution + span.literal.text;
    }
    return value;
  }
  return undefined;
}

function evaluateTemplateValue(node: ts.Expression, depth: number): string | undefined {
  if (depth > 64) {
    return undefined;
  }
  if (ts.isParenthesizedExpression(node)) {
    return evaluateTemplateValue(node.expression, depth + 1);
  }
  if (ts.isNumericLiteral(node)) {
    return String(Number(node.text));
  }
  switch (node.kind) {
    case ts.SyntaxKind.TrueKeyword:
      return 'true';
    case ts.SyntaxKind.FalseKeyword:
      return 'false';
    case ts.SyntaxKind.NullKeyword:
      return 'null';
    default:
      return evaluateString(node, depth + 1);
  }
}

function unwrapParentheses(node: ts.Expression): ts.Expression {
  while (ts.isParenthesizedExpression(node)) {
    node = node.expression;
  }
  return node;
}

function readWildcardOption(node: ts.ObjectLiteralExpression): boolean | undefined {
  if (node.properties.length === 0) {
    return true;
  }
  if (node.properties.length !== 1) {
    return undefined;
  }
  const option = node.properties[0];
  if (!ts.isPropertyAssignment(option) ||
      !(ts.isIdentifier(option.name) || ts.isStringLiteral(option.name)) ||
      option.name.text !== 'enableWildcard') {
    return undefined;
  }
  const value = unwrapParentheses(option.initializer);
  if (value.kind === ts.SyntaxKind.TrueKeyword) {
    return true;
  }
  return value.kind === ts.SyntaxKind.FalseKeyword ? false : undefined;
}

function isPropertySegment(segment: string): boolean {
  const characters = Array.from(segment);
  return characters.length > 0 && ts.isIdentifierStart(characters[0].codePointAt(0)!, ts.ScriptTarget.Latest) &&
    characters.slice(1).every(character => ts.isIdentifierPart(character.codePointAt(0)!, ts.ScriptTarget.Latest));
}

function parsePath(path: string, wildcardEnabled: boolean | undefined): string[] | undefined {
  const segments = path.split('.');
  const properties: string[] = [];
  for (let index = 0; index < segments.length; index++) {
    const segment = segments[index];
    if (segment === '*' && index > 0 && index === segments.length - 1 && wildcardEnabled !== false) {
      continue;
    }
    if (/^\d+$/.test(segment)) {
      continue;
    }
    if (!isPropertySegment(segment)) {
      return undefined;
    }
    properties.push(segment);
  }
  return properties;
}

function bindingContains(name: ts.BindingName, decorator: string): boolean {
  if (ts.isIdentifier(name)) {
    return name.text === decorator;
  }
  return name.elements.some(element => ts.isBindingElement(element) && bindingContains(element.name, decorator));
}

function declaresDecorator(statement: ts.Statement, decorator: string): boolean {
  if (ts.isVariableStatement(statement)) {
    return statement.declarationList.declarations.some(declaration => bindingContains(declaration.name, decorator));
  }
  if (ts.isFunctionDeclaration(statement) || ts.isClassDeclaration(statement) ||
      ts.isEnumDeclaration(statement) || ts.isModuleDeclaration(statement) || ts.isImportEqualsDeclaration(statement)) {
    return statement.name?.text === decorator;
  }
  if (ts.isImportDeclaration(statement)) {
    const clause = statement.importClause;
    const bindings = clause?.namedBindings;
    return clause?.name?.text === decorator || !!(bindings && (ts.isNamespaceImport(bindings) ?
      bindings.name.text === decorator : bindings.elements.some(element => element.name.text === decorator)));
  }
  return false;
}

// Source files are freshly parsed for each scan. Weak keys avoid retaining ASTs
// across builds, and each statement list is inspected only once per decorator.
const localBindings = new WeakMap<ts.Node, Map<string, boolean>>();

// Exclude explicit local/imported bindings without a Program/TypeChecker or
// resolving aliases. Global built-ins are the supported entry points.
function hasLocalBinding(node: ts.Node, decorator: string): boolean {
  for (let scope = node.parent; scope; scope = scope.parent) {
    if (ts.isSourceFile(scope) || ts.isBlock(scope) || ts.isModuleBlock(scope)) {
      let bindings = localBindings.get(scope);
      if (!bindings) {
        bindings = new Map();
        localBindings.set(scope, bindings);
      }
      if (!bindings.has(decorator)) {
        bindings.set(decorator, scope.statements.some(statement => declaresDecorator(statement, decorator)));
      }
      if (bindings.get(decorator)) {
        return true;
      }
    }
    if (ts.isFunctionLike(scope) && scope.parameters.some(parameter => bindingContains(parameter.name, decorator))) {
      return true;
    }
    if (ts.isCatchClause(scope) && scope.variableDeclaration &&
        bindingContains(scope.variableDeclaration.name, decorator)) {
      return true;
    }
  }
  return false;
}

/**
 * Parse direct Monitor/SyncMonitor calls. The caller controls the PROJECT scan
 * and property-obfuscation gates. No AST nodes or property white lists are mutated.
 */
export function collectMonitorProperties(node: ts.Node): MonitorProperties | undefined {
  if (!ts.isDecorator(node) || !ts.isCallExpression(node.expression) ||
      !ts.isIdentifier(node.expression.expression)) {
    return undefined;
  }
  const decorator = node.expression.expression.text;
  if ((decorator !== 'Monitor' && decorator !== 'SyncMonitor') || hasLocalBinding(node, decorator)) {
    return undefined;
  }
  const result: MonitorProperties = { decorator, properties: new Set(), diagnostics: [] };
  const args = node.expression.arguments;
  let firstPath = 0;
  let wildcardEnabled: boolean | undefined = decorator === 'SyncMonitor';
  if (decorator === 'Monitor' && args.length > 0) {
    const first = unwrapParentheses(args[0]);
    if (ts.isObjectLiteralExpression(first)) {
      firstPath = 1;
      wildcardEnabled = readWildcardOption(first);
      if (wildcardEnabled === undefined) {
        result.diagnostics.push({ node: args[0], code: MonitorDiagnosticCode.INVALID_OPTIONS,
          message: 'Expected {} or { enableWildcard: true/false } as Monitor options.' });
      }
    } else if (evaluateString(first) === undefined) {
      // An unknown first argument may be options. Diagnose it below, but still
      // preserve concrete names from subsequent paths without guessing its value.
      wildcardEnabled = undefined;
    }
  }
  if (firstPath === args.length) {
    result.diagnostics.push({ node, code: MonitorDiagnosticCode.MISSING_PATH,
      message: 'Expected at least one monitored path.' });
  }
  for (let index = firstPath; index < args.length; index++) {
    const argument = args[index];
    const path = evaluateString(argument);
    if (path === undefined) {
      result.diagnostics.push({ node: argument, code: MonitorDiagnosticCode.UNSUPPORTED_EXPRESSION,
        message: 'Cannot determine the monitored path from a literal-only expression.' });
      continue;
    }
    const properties = parsePath(path, wildcardEnabled);
    if (properties === undefined) {
      result.diagnostics.push({ node: argument, code: MonitorDiagnosticCode.INVALID_PATH,
        message: 'Expected a dot-separated property path, with numeric indices or a supported final wildcard.' });
      continue;
    }
    properties.forEach(property => result.properties.add(property));
  }
  return result;
}

export function reportMonitorDiagnostic(decorator: string, diagnostic: MonitorDiagnostic): void {
  const sourceFile = diagnostic.node.getSourceFile();
  const position = sourceFile.getLineAndCharacterOfPosition(diagnostic.node.getStart(sourceFile));
  const argument = JSON.stringify(diagnostic.node.getText(sourceFile));
  const message = `[ArkGuard:${diagnostic.code}] ${sourceFile.fileName}:${position.line + 1}:${position.character + 1} ` +
    `@${decorator} ${argument}: ${diagnostic.message} ` +
    'Check the decorator arguments. For unresolved paths, use -keep-property-name to retain every property ' +
    'in all possible runtime paths; keeping the path variable name is insufficient.';
  if (monitorDiagnosticLogger) {
    monitorDiagnosticLogger(message);
  } else {
    console.warn(message);
  }
}
