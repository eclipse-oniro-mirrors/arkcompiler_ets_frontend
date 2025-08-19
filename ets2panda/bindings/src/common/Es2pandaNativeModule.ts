/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as fs from 'fs';
import * as path from 'path';
import {
  KNativePointer as KPtr,
  KInt,
  KBoolean,
  KNativePointer,
  KDouble,
  KUInt,
  KStringPtr,
  KInt32ArrayPtr
} from './InteropTypes';
import { Es2pandaNativeModule as GeneratedEs2pandaNativeModule } from '../generated/Es2pandaNativeModule';
import { loadNativeModuleLibrary, registerNativeModuleLibraryName } from './loadLibraries';
import { throwError } from './utils';

export type KPtrArray = BigUint64Array;

export class Es2pandaNativeModule {
  _ClassDefinitionSuper(context: KPtr, node: KPtr): KPtr {
    throw new Error('Not implemented');
  }
  _CreateTSInterfaceDeclaration(
    _context: KPtr,
    _extends: KPtrArray,
    _extendsLen: KInt,
    _id: KPtr,
    _typeParams: KPtr,
    _body: KPtr,
    _isStatic: KBoolean,
    _isExternal: KBoolean
  ): KPtr {
    throw new Error('Not implemented');
  }

  _ContextState(context: KPtr): KInt {
    throw new Error('Not implemented');
  }
  _ContextErrorMessage(context: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _GetAllErrorMessages(context: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _AstNodeDumpModifiers(context: KPtr, node: KPtr): KPtr {
    throw new Error('Not implemented');
  }
  _CreateAstDumper(context: KPtr, node: KPtr, source: String): KPtr {
    throw new Error('Not implemented');
  }

  _CreateConfig(argc: number, argv: string[] | Uint8Array, pandaLibPath: KStringPtr): KPtr {
    throw new Error('Not implemented');
  }
  _DestroyConfig(config: KPtr): void {
    throw new Error('Not implemented');
  }
  _CreateContextFromString(config: KPtr, source: String, filename: String): KPtr {
    throw new Error('Not implemented');
  }
  _CreateContextFromStringWithHistory(config: KPtr, source: String, filename: String): KPtr {
    throw new Error('Not implemented');
  }
  _GenerateTsDeclarationsFromContext(
    config: KPtr,
    outputDeclEts: String,
    outputEts: String,
    exportAll: KBoolean,
    isolated: KBoolean,
    recordFile: String
  ): KPtr {
    throw new Error('Not implemented');
  }
  _CreateContextFromFile(config: KPtr, filename: String): KPtr {
    throw new Error('Not implemented');
  }
  _DestroyContext(context: KPtr): void {
    throw new Error('Not implemented');
  }
  _ProceedToState(context: KPtr, state: number): void {
    throw new Error('Not implemented');
  }
  _ContextProgram(context: KPtr): KPtr {
    throw new Error('Not implemented');
  }
  _ProgramAst(program: KPtr): KPtr {
    throw new Error('Not implemented');
  }
  _CheckerStartChecker(context: KPtr): KBoolean {
    throw new Error('Not implemented');
  }

  _IsProgram(context: KPtr, node: KPtr): KBoolean {
    throw new Error('Not implemented');
  }
  _AstNodeDumpJsonConst(context: KPtr, node: KPtr): KPtr {
    throw new Error('Not implemented');
  }
  _AstNodeDumpEtsSrcConst(context: KPtr, node: KPtr): KPtr {
    throw new Error('Not implemented');
  }
  _AstNodeUpdateChildren(context: KPtr, node: KPtr): void {
    throw new Error('Not implemented');
  }
  _AstNodeUpdateAll(context: KPtr, node: KPtr): void {
    throw new Error('Not implemented');
  }
  _AstNodeSetOriginalNode(context: KPtr, ast: KPtr, originalNode: KPtr): void {
    throw new Error('Not implemented');
  }
  _AstNodeOriginalNodeConst(context: KPtr, ast: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _VarBinderSetProgram(context: KPtr): void {
    throw new Error('Not implemented');
  }
  _VarBinderSetContext(context: KPtr): void {
    throw new Error('Not implemented');
  }

  _getCurrentTokenValue(context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getSemanticDiagnostics(context: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getSyntacticDiagnostics(context: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDiags(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDiagMsg(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDiagSource(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDefinitionAtPosition(context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getTypeHierarchies(searchContext: KNativePointer, context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getFileNameFromTypeHierarchiesInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getNameFromTypeHierarchiesInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getTypeFromTypeHierarchiesInfo(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getPositionFromTypeHierarchiesInfo(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getSuperFromTypeHierarchiesInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getSubFromTypeHierarchiesInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getFileNameFromTypeHierarchies(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getNameFromTypeHierarchies(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getTypeFromTypeHierarchies(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getPosFromTypeHierarchies(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getSubOrSuper(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getClassHierarchyInfo(context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _castToClassHierarchyInfos(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getClassNameFromClassHierarchyInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getMethodItemsFromClassHierarchyInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getPropertyItemsFromClassHierarchyInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDetailFromClassHierarchyItem(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getAccessModifierStyleFromClassHierarchyItem(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getSetterStyleFromClassMethodItem(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getAliasScriptElementKind(ptr: KNativePointer, position: KInt): KInt {
    throw new Error('Not implemented');
  }

  _getClassPropertyInfo(context: KNativePointer, position: KInt, shouldCollectInherited: boolean): KPtr {
    throw new Error('Not implemented');
  }

  _getFieldsInfoFromPropertyInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getNameFromPropertyInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getFieldListPropertyFromPropertyInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getModifierKindsFromPropertyInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDisplayNameFromPropertyInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getStartFromPropertyInfo(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getEndFromPropertyInfo(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getKindFromPropertyInfo(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _organizeImports(context: KNativePointer, filename: String): KPtr {
    throw new Error('Not implemented');
  }

  _getFileTextChanges(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getTextChangesFromFileTextChanges(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getFileNameFromFileTextChanges(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getTextSpanFromTextChange(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getNewTextFromTextChange(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _findSafeDeleteLocation(context: KNativePointer, declInfo: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getSafeDeleteLocations(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getSafeDeleteLocationUri(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getSafeDeleteLocationStart(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getSafeDeleteLocationLength(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getCompletionEntryDetails(entryName: String, filename: String, context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getCompletionEntryDetailsEntryName(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getCompletionEntryDetailsKind(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getCompletionEntryDetailsKindModifier(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getCompletionEntryDetailsFileName(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getCompletionEntryDetailsSymbolDisplayPart(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getRefactorActionName(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getRefactorActionDescription(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getRefactorActionKind(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getApplicableRefactorAction(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getApplicableRefactorName(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getApplicableRefactorDescription(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getApplicableRefactors(context: KNativePointer, kind: String, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getApplicableRefactorInfoList(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getClassConstructorInfo(context: KNativePointer, position: number, strArryPtr: string[] | Uint8Array): KPtr {
    throw new Error('Not implemented');
  }

  _getFileTextChangesFromConstructorInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getFileNameFromConstructorInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getTextChangeFromConstructorInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getNewTextFromConstructorInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getTextSpanFromConstructorInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getClassHierarchies(context: KNativePointer, fileName: String, position: number): KPtr {
    throw new Error('Not implemented');
  }

  _pushBackToNativeContextVector(context: KNativePointer, contextList: KNativePointer, isNew: KBoolean): KPtr {
    throw new Error('Not implemented');
  }

  _getClassHierarchyList(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getPosFromClassHierarchyItemInfo(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getKindFromClassHierarchyItemInfo(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getDescriptionFromClassHierarchyItemInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getOverriddenFromClassHierarchyItemInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getOverridingFromClassHierarchyItemInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getImplementedFromClassHierarchyItemInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getImplementingFromClassHierarchyItemInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getFileNameFromClassRelationDetails(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getPosFromClassRelationDetails(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getKindFromClassRelationDetails(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _GetFileNameFromDef(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _GetStartFromDef(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getLengthFromDef(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getDiagRange(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRangeEnd(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRangeStart(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }
  _getPosLine(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getPosChar(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getDiagSeverity(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getDiagCode(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getDiagCodeDescription(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getCodeDescriptionHref(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getDiagTags(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getDiagData(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getDiagRelatedInfo(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRelatedInfoMsg(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRelatedInfoLoc(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getLocUri(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getLocRange(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getImplementationAtPosition(context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getReferenceStart(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getReferenceLength(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getReferenceFileName(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getReferencesFromRefs(ptr: KNativePointer): KPtr[] {
    throw new Error('Not implemented');
  }

  _getReferenceInfos(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDeclInfo(context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getDeclInfoFileText(declInfo: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDeclInfoFileName(declInfo: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getReferencesAtPosition(context: KNativePointer, declInfo: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _isPackageModule(context: KNativePointer): boolean {
    throw new Error('Not implemented');
  }

  _getFileReferences(filename: String, context: KNativePointer, isPackageModule: boolean): KPtr {
    throw new Error('Not implemented');
  }

  _getSpanOfEnclosingComment(context: KNativePointer, position: KInt, onlyMultiLine: boolean): KPtr {
    throw new Error('Not implemented');
  }

  _getSuggestionDiagnostics(context: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getQuickInfoAtPosition(filename: String, context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getDisplayPartsText(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDisplayPartsKind(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getQuickInfoKind(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getQuickInfoKindModifier(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getQuickInfoFileName(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getSymbolDisplayPart(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getTextSpanStart(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getTextSpanLength(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getTextSpan(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getSourceLocationLine(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getSourceLocationColumn(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getHighlightTextSpan(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getHighlightContextSpan(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getHighlightFileName(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getHighlightIsInString(ptr: KNativePointer): boolean {
    throw new Error('Not implemented');
  }

  _getHighlightKind(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getHighlightSpanFromHighlights(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDocumentHighlightsFromRef(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDocumentHighlights(context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getCompletionAtPosition(context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getFileNameFromEntryData(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getNamedExportFromEntryData(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getImportDeclarationFromEntryData(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getStatusFromEntryData(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getNameFromEntry(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getSortTextFromEntry(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getInsertTextFromEntry(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getKindFromEntry(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getDataFromEntry(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getEntriesFromCompletionInfo(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getUriFromLocation(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getStartFromLocation(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getEndFromLocation(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getIsDefinitionFromLocation(ptr: KNativePointer): boolean {
    throw new Error('Not implemented');
  }

  _getIsImportFromLocation(ptr: KNativePointer): boolean {
    throw new Error('Not implemented');
  }

  _getAccessKindFromLocation(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getLocationFromList(ptr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getLine(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _getChar(ptr: KNativePointer): KInt {
    throw new Error('Not implemented');
  }

  _toLineColumnOffset(context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getSafeDeleteInfo(context: KNativePointer, position: KInt): boolean {
    throw new Error('Not implemented');
  }

  _getCodeFixesAtPosition(
    context: KNativePointer,
    startPosition: KInt,
    endPosition: KInt,
    errorCodesPtr: KInt32ArrayPtr,
    codeLength: KInt
  ): KPtr {
    throw new Error('Not implemented');
  }

  _getCodeFixActionInfos(infoPtr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getFileTextChangesFromCodeActionInfo(infoPtr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getDescriptionFromCodeActionInfo(infoPtr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getFixNameFromCodeFixActionInfo(infoPtr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getFixIdFromCodeFixActionInfo(infoPtr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getFixAllDescriptionFromCodeFixActionInfo(infoPtr: KNativePointer): KPtr {
    throw new Error('Not implemented');
  }

  _getInlayHintText(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getInlayHintNumber(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getInlayHintKind(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getInlayHintWhitespaceBefore(ptr: KPtr): KBoolean {
    throw new Error('Not implemented');
  }

  _getInlayHintWhitespaceAfter(ptr: KPtr): KBoolean {
    throw new Error('Not implemented');
  }

  _getInlayHintList(context: KPtr, span: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getInlayHints(context: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameLocationFileName(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameLocationStart(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getRenameLocationEnd(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getRenameLocationLine(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _renameLocationHasPrefixText(ptr: KPtr): KBoolean {
    throw new Error('Not implemented');
  }

  _getRenameLocationPrefixText(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _renameLocationHasSuffixText(ptr: KPtr): KBoolean {
    throw new Error('Not implemented');
  }

  _getRenameLocationSuffixText(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _findRenameLocations(argc: KInt, fileContexts: Uint8Array, context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _findRenameLocationsInCurrentFile(context: KNativePointer, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _needsCrossFileRename(context: KNativePointer, position: KInt): KBoolean {
    throw new Error('Not implemented');
  }

  _getRenameSuccessFileName(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameSuccessKind(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getRenameSuccessDisplayName(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameSuccessFullDisplayName(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameSuccessKindModifiers(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameSuccessTriggerSpan(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameFailureLocalizedErrorMessage(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameInfoIsSuccess(ptr: KPtr): KBoolean {
    throw new Error('Not implemented');
  }

  _getRenameInfoSuccess(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameInfoFailure(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getRenameInfo(context: KNativePointer, position: KInt, pandaLibPath: String): KPtr {
    throw new Error('Not implemented');
  }

  _createTextSpan(start: KInt, length: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getSignatureHelpParameterDocumentation(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getSignatureHelpParameterDisplayParts(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getSignatureHelpParameterName(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getSignatureHelpItemPrefix(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getSignatureHelpItemSuffix(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getSignatureHelpItemSeparator(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getSignatureHelpItemParameter(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getSignatureHelpItemDocumentation(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getSignatureHelpItem(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getApplicableSpan(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getSelectedItemIndex(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getArgumentIndex(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getArgumentCount(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getSignatureHelpItems(context: KPtr, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getOffsetByColAndLine(sourceCode: String, line: KInt, column: KInt): KInt {
    throw new Error('Not implemented');
  }

  _getColAndLineByOffset(sourceCode: String, offset: KInt): KInt {
    throw new Error('Not implemented');
  }

  _MemInitialize(pandaLibPath: KStringPtr): void {
    throw new Error('Not implemented');
  }

  _MemFinalize(): void {
    throw new Error('Not implemented');
  }

  _CreateGlobalContext(configPtr: KNativePointer, externalFileList: string[] | Uint8Array, fileNum: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _DestroyGlobalContext(contextPtr: KPtr): void {
    throw new Error('Not implemented');
  }

  _CreateCacheContextFromString(
    config: KPtr,
    source: String,
    filename: String,
    globalContext: KPtr,
    isExternal: boolean
  ): KPtr {
    throw new Error('Not implemented');
  }

  _RemoveFileCache(globalContextPtr: KPtr, filename: String): void {
    throw new Error('Not implemented');
  }

  _AddFileCache(globalContextPtr: KPtr, filename: String): void {
    throw new Error('Not implemented');
  }

  _InvalidateFileCache(globalContextPtr: KPtr, filename: String): void {
    throw new Error('Not implemented');
  }

  _getNameByNodeInfo(nodeInfo: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getKindByNodeInfo(nodeInfo: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getNodeInfosByDefinitionData(context: KPtr, position: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _CreateNodeInfoPtr(nodeName: String, nodeKind: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getProgramAst(context: KPtr): KPtr {
    throw new Error('Not implemented');
  }

  _getClassDefinition(astNode: KPtr, nodeName: String): KPtr {
    throw new Error('Not implemented');
  }

  _getDefinitionDataFromNode(context: KPtr, nodeInfoPtrs: Uint8Array, arraySize: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getIdentifier(astNode: KPtr, nodeName: String): KPtr {
    throw new Error('Not implemented');
  }
}

export function initEs2panda(): Es2pandaNativeModule {
  let libPath = process.env.BINDINGS_PATH;
  if (libPath === undefined) {
    libPath = path.resolve(__dirname, '../../ts_bindings.node');
  } else {
    libPath = path.join(libPath, 'ts_bindings.node');
  }
  if (!fs.existsSync(libPath)) {
    throwError(`Cannot find lib path ${libPath}`);
  }
  registerNativeModuleLibraryName('NativeModule', libPath);
  const instance = new Es2pandaNativeModule();
  loadNativeModuleLibrary('NativeModule', instance);
  return instance;
}

export function initGeneratedEs2panda(): GeneratedEs2pandaNativeModule {
  let libPath = process.env.BINDINGS_PATH;
  if (libPath === undefined) {
    libPath = path.resolve(__dirname, '../../ts_bindings.node');
  } else {
    libPath = path.join(libPath, 'ts_bindings.node');
  }
  if (!fs.existsSync(libPath)) {
    throwError(`Cannot find lib path ${libPath}`);
  }
  registerNativeModuleLibraryName('NativeModule', libPath);
  const instance = new GeneratedEs2pandaNativeModule();
  loadNativeModuleLibrary('NativeModule', instance);
  return instance;
}

export function initPublicEs2panda(): Es2pandaNativeModule {
  let libPath = process.env.BINDINGS_PATH;
  if (libPath === undefined) {
    libPath = path.resolve(__dirname, '../../public.node');
  } else {
    libPath = path.join(libPath, 'public.node');
  }
  if (!fs.existsSync(libPath)) {
    throwError(`Cannot find lib path ${libPath}`);
  }
  registerNativeModuleLibraryName('NativeModule', libPath);
  const instance = new Es2pandaNativeModule();
  loadNativeModuleLibrary('NativeModule', instance);
  return instance;
}

export function initPublicGeneratedEs2panda(): GeneratedEs2pandaNativeModule {
  let libPath = process.env.BINDINGS_PATH;
  if (libPath === undefined) {
    libPath = path.resolve(__dirname, '../../public.node');
  } else {
    libPath = path.join(libPath, 'public.node');
  }
  if (!fs.existsSync(libPath)) {
    throwError(`Cannot find lib path ${libPath}`);
  }
  registerNativeModuleLibraryName('NativeModule', libPath);
  const instance = new GeneratedEs2pandaNativeModule();
  loadNativeModuleLibrary('NativeModule', instance);
  return instance;
}
