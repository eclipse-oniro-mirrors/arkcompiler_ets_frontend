/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { KBoolean, KInt, KNativePointer, KUInt } from '../common/InteropTypes';
import { unpackString, VariantTypes } from '../common/private';
import { throwError } from '../common/utils';
import { isNullPtr } from '../common/Wrapper';
import { global } from '../common/global';
import { NativePtrDecoder } from '../common/Platform';
import { AstNodeType, NodeInfo } from '../common/types';

enum HierarchyType {
  OTHERS,
  INTERFACE,
  CLASS
}

export enum SetterStyle {
  NONE = 0,
  SETTER,
  GETTER
}

export enum AccessModifierStyle {
  PUBLIC = 0,
  PROTECTED,
  PRIVATE
}

enum ClassRelationKind {
  UNKNOWN,
  INTERFACE,
  CLASS,
  FIELD,
  METHOD,
  PROPERTY
}

export enum ClassDefinitionStyle {
  FIELD = 0,
  METHOD
}

export abstract class LspNode {
  readonly peer: KNativePointer;

  protected constructor(peer: KNativePointer) {
    if (isNullPtr(peer)) {
      throwError('nullptr from peer in lspnode constuctor');
    }
    this.peer = peer;
  }
}

export class LspPosition extends LspNode {
  readonly line: number;
  readonly character: number;
  constructor(peer: KNativePointer) {
    super(peer);
    this.line = global.es2panda._getPosLine(peer);
    this.character = global.es2panda._getPosChar(peer);
  }
}

export class LspRange extends LspNode {
  readonly start: LspPosition;
  readonly end: LspPosition;
  constructor(peer: KNativePointer) {
    super(peer);
    this.start = new LspPosition(global.es2panda._getRangeStart(peer));
    this.end = new LspPosition(global.es2panda._getRangeEnd(peer));
  }
}

export enum LspDiagSeverity {
  Error = 1,
  Warning = 2,
  Information = 3,
  Hint = 4
}

export enum LspDiagTag {
  Unnecessary = 1,
  Deprecated = 2
}

export class LspLocation extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.uri = unpackString(global.es2panda._getLocUri(peer));
    this.range = new LspRange(global.es2panda._getLocRange(peer));
  }
  readonly uri: string;
  readonly range: LspRange;
}

export class LspRelatedInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.message = unpackString(global.es2panda._getRelatedInfoMsg(peer));
    this.location = new LspLocation(global.es2panda._getRelatedInfoLoc(peer));
  }
  readonly message: string;
  readonly location: LspLocation;
}

export class LspCodeDescription extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.href = unpackString(global.es2panda._getCodeDescriptionHref(peer));
  }
  readonly href: string;
}

export class LspDiagnosticNode extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.message = unpackString(global.es2panda._getDiagMsg(peer));
    this.source = unpackString(global.es2panda._getDiagSource(peer));
    this.range = new LspRange(global.es2panda._getDiagRange(peer));
    this.tags = new NativePtrDecoder()
      .decode(global.es2panda._getDiagTags(peer))
      .map((elPeer: KNativePointer) => elPeer as KInt);
    this.relatedInfo = new NativePtrDecoder()
      .decode(global.es2panda._getDiagRelatedInfo(peer))
      .map((elPeer: KNativePointer) => new LspRelatedInfo(elPeer));
    let codeVarPtr = global.es2panda._getDiagCode(peer);
    if (global.interop._getTypeOfVariant(codeVarPtr) === VariantTypes.VARIANT_INT) {
      this.code = global.interop._GetIntFromVariant(codeVarPtr);
    } else {
      this.code = unpackString(global.interop._getStringFromVariant(codeVarPtr));
    }
    let dataPtr = global.es2panda._getDiagData(peer);
    if (global.interop._getTypeOfVariant(dataPtr) === VariantTypes.VARIANT_INT) {
      this.data = global.interop._GetIntFromVariant(dataPtr);
    } else {
      this.data = unpackString(global.interop._getStringFromVariant(dataPtr));
    }
    this.severity = global.es2panda._getDiagSeverity(peer);
    this.codeDescription = new LspCodeDescription(global.es2panda._getDiagCodeDescription(peer));
  }
  readonly source: String;
  readonly message: String;
  readonly range: LspRange;
  readonly codeDescription: LspCodeDescription;
  readonly severity: LspDiagSeverity;
  readonly tags: LspDiagTag[];
  readonly relatedInfo: LspRelatedInfo[];
  readonly code: number | String;
  readonly data: number | string;
}

export class LspDiagsNode extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.diagnostics = new NativePtrDecoder()
      .decode(global.es2panda._getDiags(this.peer))
      .map((elPeer: KNativePointer) => {
        return new LspDiagnosticNode(elPeer);
      });
  }
  readonly diagnostics: LspDiagnosticNode[];
}

export class LspDefinitionData extends LspNode {
  constructor(peer: KNativePointer, filePath?: string) {
    super(peer);
    this.fileName = filePath ? filePath : unpackString(global.es2panda._GetFileNameFromDef(peer));
    this.start = global.es2panda._GetStartFromDef(peer);
    this.length = global.es2panda._getLengthFromDef(peer);
  }
  readonly fileName: String;
  readonly start: KInt;
  readonly length: KInt;
  nodeInfos?: NodeInfo[];
}

export class LspReferenceData extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getReferenceFileName(peer));
    this.start = global.es2panda._getReferenceStart(peer);
    this.length = global.es2panda._getReferenceLength(peer);
  }
  readonly fileName: String;
  readonly start: KInt;
  readonly length: KInt;
}

export class LspDeclInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getDeclInfoFileName(peer));
    this.fileText = unpackString(global.es2panda._getDeclInfoFileText(peer));
  }
  readonly fileName: String;
  readonly fileText: String;
}

export class LspReferences extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.referenceInfos = new NativePtrDecoder()
      .decode(global.es2panda._getReferenceInfos(this.peer))
      .map((elPeer: KNativePointer) => {
        return new LspReferenceData(elPeer);
      });
  }
  readonly referenceInfos: LspReferenceData[];
}

export class LspTextSpan extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.start = global.es2panda._getTextSpanStart(peer);
    this.length = global.es2panda._getTextSpanLength(peer);
  }
  readonly start: KInt;
  readonly length: KInt;
}

export class LspSourceLocation extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.line = global.es2panda._getSourceLocationLine(peer);
    this.column = global.es2panda._getSourceLocationColumn(peer);
  }
  readonly line: number;
  readonly column: number;
}

export interface TextSpan {
  start: KInt;
  length: KInt;
}

export class LspSymbolDisplayPart extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.text = unpackString(global.es2panda._getDisplayPartsText(peer));
    this.kind = unpackString(global.es2panda._getDisplayPartsKind(peer));
  }
  readonly text: String;
  readonly kind: String;
}

export class LspClassHierarchyItem extends LspNode {
  constructor(peer: KNativePointer, style: ClassDefinitionStyle) {
    super(peer);
    this.style = style;
    this.detail = unpackString(global.es2panda._getDetailFromClassHierarchyItem(this.peer));
    this.accessModifier = global.es2panda._getAccessModifierStyleFromClassHierarchyItem(this.peer);
  }
  readonly detail: string;
  readonly accessModifier: AccessModifierStyle;
  readonly style: ClassDefinitionStyle;
}

export class LspClassMethodItem extends LspClassHierarchyItem {
  constructor(peer: KNativePointer) {
    super(peer, ClassDefinitionStyle.METHOD);
    this.setter = global.es2panda._getSetterStyleFromClassMethodItem(this.peer);
  }
  readonly setter: SetterStyle;
}

export class LspClassPropertyItem extends LspClassHierarchyItem {
  constructor(peer: KNativePointer) {
    super(peer, ClassDefinitionStyle.FIELD);
  }
}

export class LspClassHierarchyInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.className = unpackString(global.es2panda._getClassNameFromClassHierarchyInfo(this.peer));
    this.methodItems = new NativePtrDecoder()
      .decode(global.es2panda._getMethodItemsFromClassHierarchyInfo(this.peer))
      .map((elPeer: KNativePointer) => {
        return new LspClassMethodItem(elPeer);
      });
    this.fieldItems = new NativePtrDecoder()
      .decode(global.es2panda._getPropertyItemsFromClassHierarchyInfo(this.peer))
      .map((elPeer: KNativePointer) => {
        return new LspClassPropertyItem(elPeer);
      });
  }
  readonly className: string;
  readonly methodItems: LspClassMethodItem[];
  readonly fieldItems: LspClassPropertyItem[];
}

export class LspClassHierarchy extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.infos = new NativePtrDecoder()
      .decode(global.es2panda._castToClassHierarchyInfos(this.peer))
      .map((elPeer: KNativePointer) => {
        return new LspClassHierarchyInfo(elPeer);
      });
  }
  readonly infos: LspClassHierarchyInfo[];
}

export class LspClassPropertyInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fieldsInfo = new NativePtrDecoder()
      .decode(global.es2panda._getFieldsInfoFromPropertyInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new FieldsInfo(elPeer);
      });
  }
  readonly fieldsInfo: FieldsInfo[];
}

export class FieldsInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.name = unpackString(global.es2panda._getNameFromPropertyInfo(peer));
    this.properties = new NativePtrDecoder()
      .decode(global.es2panda._getFieldListPropertyFromPropertyInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new FieldListProperty(elPeer);
      });
  }
  readonly name: String;
  readonly properties: FieldListProperty[];
}

export class FieldListProperty extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.kind = unpackString(global.es2panda._getKindFromPropertyInfo(peer));
    this.modifierKinds = new NativePtrDecoder()
      .decode(global.es2panda._getModifierKindsFromPropertyInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new String(unpackString(elPeer));
      });
    this.displayName = unpackString(global.es2panda._getDisplayNameFromPropertyInfo(peer));
    this.start = global.es2panda._getStartFromPropertyInfo(peer);
    this.end = global.es2panda._getEndFromPropertyInfo(peer);
  }
  readonly kind: String;
  readonly modifierKinds: String[];
  readonly displayName: String;
  readonly start: number;
  readonly end: number;
}

export class LspClassHierarchies extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.classHierarchies = new NativePtrDecoder()
      .decode(global.es2panda._getClassHierarchyList(peer))
      .map((elPeer: KNativePointer) => {
        return new ClassHierarchyItemInfo(elPeer);
      });
  }
  readonly classHierarchies: ClassHierarchyItemInfo[];
}

export class ClassHierarchyItemInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.pos = global.es2panda._getPosFromClassHierarchyItemInfo(peer);
    this.kind = global.es2panda._getKindFromClassHierarchyItemInfo(peer);
    this.description = unpackString(global.es2panda._getDescriptionFromClassHierarchyItemInfo(peer));
    this.overridden = new NativePtrDecoder()
      .decode(global.es2panda._getOverriddenFromClassHierarchyItemInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new ClassRelationDetails(elPeer);
      });
    this.overriding = new NativePtrDecoder()
      .decode(global.es2panda._getOverridingFromClassHierarchyItemInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new ClassRelationDetails(elPeer);
      });
    this.implemented = new NativePtrDecoder()
      .decode(global.es2panda._getImplementedFromClassHierarchyItemInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new ClassRelationDetails(elPeer);
      });
    this.implementing = new NativePtrDecoder()
      .decode(global.es2panda._getImplementingFromClassHierarchyItemInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new ClassRelationDetails(elPeer);
      });
  }
  readonly pos: number;
  readonly kind: ClassRelationKind;
  readonly description: String;
  readonly overridden: ClassRelationDetails[];
  readonly overriding: ClassRelationDetails[];
  readonly implemented: ClassRelationDetails[];
  readonly implementing: ClassRelationDetails[];
}

export class ClassRelationDetails extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getFileNameFromClassRelationDetails(peer));
    this.pos = global.es2panda._getPosFromClassRelationDetails(peer);
    this.kind = global.es2panda._getKindFromClassRelationDetails(peer);
  }
  readonly fileName: String;
  readonly pos: number;
  readonly kind: ClassRelationKind;
}

export class LspQuickInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.kind = unpackString(global.es2panda._getQuickInfoKind(peer));
    this.kindModifier = unpackString(global.es2panda._getQuickInfoKindModifier(peer));
    this.textSpan = new LspTextSpan(global.es2panda._getTextSpan(peer));
    this.fileName = unpackString(global.es2panda._getQuickInfoFileName(peer));
    this.displayParts = new NativePtrDecoder()
      .decode(global.es2panda._getSymbolDisplayPart(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSymbolDisplayPart(elPeer);
      });
  }
  readonly kind: String;
  readonly kindModifier: String;
  readonly textSpan: LspTextSpan;
  readonly fileName: String;
  readonly displayParts: LspSymbolDisplayPart[];
}

export enum LspHighlightSpanKind {
  NONE,
  DEFINITION,
  REFERENCE,
  WRITTEN_REFERENCE
}

export class LspHighlightSpan extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getHighlightFileName(peer));
    this.textSpan = new LspTextSpan(global.es2panda._getHighlightTextSpan(peer));
    this.contextSpan = new LspTextSpan(global.es2panda._getHighlightContextSpan(peer));
    this.kind = global.es2panda._getHighlightKind(peer);
  }
  readonly fileName: String;
  readonly textSpan: LspTextSpan;
  readonly contextSpan: LspTextSpan;
  readonly kind: LspHighlightSpanKind;
}

export class LspDocumentHighlights extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getQuickInfoKind(peer));
    this.highlightSpans = new NativePtrDecoder()
      .decode(global.es2panda._getHighlightSpanFromHighlights(peer))
      .map((elPeer: KNativePointer) => {
        return new LspHighlightSpan(elPeer);
      });
  }
  readonly fileName: String;
  readonly highlightSpans: LspHighlightSpan[];
}

export class LspDocumentHighlightsReferences extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.documentHighlights = new NativePtrDecoder()
      .decode(global.es2panda._getDocumentHighlightsFromRef(peer))
      .map((elPeer: KNativePointer) => {
        return new LspDocumentHighlights(elPeer);
      });
  }
  readonly documentHighlights: LspDocumentHighlights[];
}

export enum LspCompletionEntryKind {
  TEXT = 1,
  METHOD = 2,
  FUNCTION = 3,
  CONSTRUCTOR = 4,
  FIELD = 5,
  VARIABLE = 6,
  CLASS = 7,
  INTERFACE = 8,
  MODULE = 9,
  PROPERTY = 10,
  UNIT = 11,
  VALUE = 12,
  ENUM = 13,
  KEYWORD = 14,
  SNIPPET = 15,
  COLOR = 16,
  FILE = 17,
  REFERENCE = 18,
  FOLDER = 19,
  ENUM_MEMBER = 20,
  CONSTANT = 21,
  STRUCT = 22,
  EVENT = 23,
  OPERATOR = 24,
  TYPE_PARAMETER = 25,
  ALIAS_TYPE = 26
}

export enum ResolutionStatus {
  RESOLVED,
  UNRESOLVED
}

export class LspCompletionEntryData extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getFileNameFromEntryData(peer));
    this.namedExport = unpackString(global.es2panda._getNamedExportFromEntryData(peer));
    this.importDeclaration = unpackString(global.es2panda._getImportDeclarationFromEntryData(peer));
    this.status = global.es2panda._getStatusFromEntryData(peer);
  }
  readonly fileName: String;
  readonly namedExport: String;
  readonly importDeclaration: String;
  readonly status: ResolutionStatus;
}

export class LspCompletionEntry extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.name = unpackString(global.es2panda._getNameFromEntry(peer));
    this.sortText = unpackString(global.es2panda._getSortTextFromEntry(peer));
    this.insertText = unpackString(global.es2panda._getInsertTextFromEntry(peer));
    this.kind = global.es2panda._getKindFromEntry(peer);
    this.data = this.getCompletionEntryData(peer);
  }
  readonly name: String;
  readonly sortText: String;
  readonly insertText: String;
  readonly kind: LspCompletionEntryKind;
  readonly data: LspCompletionEntryData | null;
  private getCompletionEntryData(peer: KNativePointer): LspCompletionEntryData | null {
    const dataPtr = global.es2panda._getDataFromEntry(peer);
    if (dataPtr) {
      return new LspCompletionEntryData(dataPtr);
    } else {
      return null;
    }
  }
}

export class LspCompletionInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.entries = new NativePtrDecoder()
      .decode(global.es2panda._getEntriesFromCompletionInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new LspCompletionEntry(elPeer);
      });
  }
  readonly entries: LspCompletionEntry[];
}

export enum AccessKind {
  READ,
  WRITE,
  READWRITE
}

export class LspReferenceLocation extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.uri = unpackString(global.es2panda._getUriFromLocation(peer));
    this.start = global.es2panda._getStartFromLocation(peer);
    this.end = global.es2panda._getEndFromLocation(peer);
    this.accessKind = global.es2panda._getAccessKindFromLocation(peer);
  }
  readonly uri: String;
  readonly start: KInt;
  readonly end: KInt;
  readonly accessKind: AccessKind;
}

export class LspReferenceLocationList extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.entries = new NativePtrDecoder()
      .decode(global.es2panda._getLocationFromList(peer))
      .map((elPeer: KNativePointer) => {
        return new LspReferenceLocation(elPeer);
      });
  }
  readonly entries: LspReferenceLocation[];
}

export class LspLineAndCharacter extends LspNode {
  readonly line: number;
  readonly character: number;
  constructor(peer: KNativePointer) {
    super(peer);
    this.line = global.es2panda._getLine(peer);
    this.character = global.es2panda._getChar(peer);
  }
}

export class LspClassConstructorInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.constructorInfoFileTextChanges = new NativePtrDecoder()
      .decode(global.es2panda._getFileTextChangesFromConstructorInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new ConstructorInfoFileTextChanges(elPeer);
      });
  }
  readonly constructorInfoFileTextChanges: ConstructorInfoFileTextChanges[];
}

export class ConstructorInfoFileTextChanges extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getFileNameFromConstructorInfo(peer));
    this.constructorInfoTextChanges = new NativePtrDecoder()
      .decode(global.es2panda._getTextChangeFromConstructorInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new ConstructorInfoTextChange(elPeer);
      });
  }
  readonly fileName: String;
  readonly constructorInfoTextChanges: ConstructorInfoTextChange[];
}

export class ConstructorInfoTextChange extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.span_ = new LspTextSpan(global.es2panda._getTextSpanFromConstructorInfo(peer));
    this.newText_ = unpackString(global.es2panda._getNewTextFromConstructorInfo(peer));
  }
  readonly span_: LspTextSpan;
  readonly newText_: String;
}

export class CompletionEntryDetails extends LspNode {
  readonly name: String;
  readonly kind: String;
  readonly kindModifier: String;
  readonly fileName: String;
  readonly displayParts: LspSymbolDisplayPart[];
  constructor(peer: KNativePointer) {
    super(peer);
    this.name = unpackString(global.es2panda._getCompletionEntryDetailsEntryName(peer));
    this.kind = unpackString(global.es2panda._getCompletionEntryDetailsKind(peer));
    this.kindModifier = unpackString(global.es2panda._getCompletionEntryDetailsKindModifier(peer));
    this.fileName = unpackString(global.es2panda._getCompletionEntryDetailsFileName(peer));
    this.displayParts = new NativePtrDecoder()
      .decode(global.es2panda._getCompletionEntryDetailsSymbolDisplayPart(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSymbolDisplayPart(elPeer);
      });
  }
}

export class TextChange extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.span = new LspTextSpan(global.es2panda._getTextSpanFromTextChange(peer));
    this.newText = unpackString(global.es2panda._getNewTextFromTextChange(peer));
  }
  readonly span: LspTextSpan;
  readonly newText: String;
}

export class FileTextChanges extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getFileNameFromFileTextChanges(peer));
    this.textChanges = new NativePtrDecoder()
      .decode(global.es2panda._getTextChangesFromFileTextChanges(peer))
      .map((elPeer: KNativePointer) => {
        return new TextChange(elPeer);
      });
  }
  readonly fileName: String;
  readonly textChanges: TextChange[];
}

export class CodeActionInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.changes = new NativePtrDecoder()
      .decode(global.es2panda._getFileTextChangesFromCodeActionInfo(peer))
      .map((elPeer: KNativePointer) => {
        return new FileTextChanges(elPeer);
      });
    this.description = unpackString(global.es2panda._getDescriptionFromCodeActionInfo(peer));
  }
  readonly changes: FileTextChanges[];
  readonly description: String;
}

export class CodeFixActionInfo extends CodeActionInfo {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fixName = unpackString(global.es2panda._getFixNameFromCodeFixActionInfo(peer));
    this.fixId_ = unpackString(global.es2panda._getFixIdFromCodeFixActionInfo(peer));
    this.fixAllDescription_ = unpackString(global.es2panda._getFixAllDescriptionFromCodeFixActionInfo(peer));
  }
  readonly fixName: String;
  readonly fixId_: String;
  readonly fixAllDescription_: String;
}

export class CodeFixActionInfoList extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.codeFixActionInfos = new NativePtrDecoder()
      .decode(global.es2panda._getCodeFixActionInfos(peer))
      .map((elPeer: KNativePointer) => {
        return new CodeFixActionInfo(elPeer);
      });
  }
  readonly codeFixActionInfos: CodeFixActionInfo[];
}

export class LspFileTextChanges extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileTextChanges = new NativePtrDecoder()
      .decode(global.es2panda._getFileTextChanges(peer))
      .map((elPeer: KNativePointer) => {
        return new FileTextChanges(elPeer);
      });
  }
  readonly fileTextChanges: FileTextChanges[];
}

export class LspSafeDeleteLocationInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.uri = unpackString(global.es2panda._getSafeDeleteLocationUri(peer));
    this.start = global.es2panda._getSafeDeleteLocationStart(peer);
    this.length = global.es2panda._getSafeDeleteLocationLength(peer);
  }
  readonly uri: String;
  readonly start: KInt;
  readonly length: KInt;
}

export class LspSafeDeleteLocation extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.safeDeleteLocationInfos = new NativePtrDecoder()
      .decode(global.es2panda._getSafeDeleteLocations(this.peer))
      .map((elPeer: KNativePointer) => {
        return new LspSafeDeleteLocationInfo(elPeer);
      });
  }
  readonly safeDeleteLocationInfos: LspSafeDeleteLocationInfo[];
}

export class RefactorAction extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.name = unpackString(global.es2panda._getRefactorActionName(peer));
    this.description = unpackString(global.es2panda._getRefactorActionDescription(peer));
    this.kind = unpackString(global.es2panda._getRefactorActionKind(peer));
  }
  readonly name: String;
  readonly description: String;
  readonly kind: String;
}

export class ApplicableRefactorItemInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.name = unpackString(global.es2panda._getApplicableRefactorName(peer));
    this.description = unpackString(global.es2panda._getApplicableRefactorDescription(peer));
    this.action = new RefactorAction(global.es2panda._getApplicableRefactorAction(peer));
  }

  readonly name: String;
  readonly description: String;
  readonly action: RefactorAction;
}

export class LspApplicableRefactorInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.applicableRefactorInfo = new NativePtrDecoder()
      .decode(global.es2panda._getApplicableRefactorInfoList(peer))
      .map((elPeer: KNativePointer) => {
        return new ApplicableRefactorItemInfo(elPeer);
      });
  }

  readonly applicableRefactorInfo: ApplicableRefactorItemInfo[];
}

export class LspTypeHierarchies extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getFileNameFromTypeHierarchies(peer));
    this.name = unpackString(global.es2panda._getNameFromTypeHierarchies(peer));
    this.type = global.es2panda._getTypeFromTypeHierarchies(peer);
    this.pos = global.es2panda._getPosFromTypeHierarchies(peer);
    this.subOrSuper = new NativePtrDecoder()
      .decode(global.es2panda._getSubOrSuper(peer))
      .map((elPeer: KNativePointer) => {
        return new LspTypeHierarchies(elPeer);
      });
  }
  readonly fileName: String;
  readonly name: String;
  readonly type: HierarchyType;
  readonly pos: KInt;
  subOrSuper: LspTypeHierarchies[];
}

export class LspTypeHierarchiesInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getFileNameFromTypeHierarchiesInfo(peer));
    this.name = unpackString(global.es2panda._getNameFromTypeHierarchiesInfo(peer));
    this.type = global.es2panda._getTypeFromTypeHierarchiesInfo(peer);
    this.pos = global.es2panda._getPositionFromTypeHierarchiesInfo(peer);
    this.superHierarchies = new LspTypeHierarchies(global.es2panda._getSuperFromTypeHierarchiesInfo(peer));
    this.subHierarchies = new LspTypeHierarchies(global.es2panda._getSubFromTypeHierarchiesInfo(peer));
  }
  readonly fileName: String;
  readonly name: String;
  readonly type: HierarchyType;
  readonly pos: KInt;
  readonly superHierarchies: LspTypeHierarchies;
  readonly subHierarchies: LspTypeHierarchies;
}

enum LspInlayHintKind {
  TYPE,
  PARAMETER,
  ENUM
}

export class LspInlayHint extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.text = unpackString(global.es2panda._getInlayHintText(peer));
    this.number = global.es2panda._getInlayHintNumber(peer);
    this.kind = global.es2panda._getInlayHintKind(peer);
    this.whitespaceBefore = global.es2panda._getInlayHintWhitespaceBefore(peer);
    this.whitespaceAfter = global.es2panda._getInlayHintWhitespaceAfter(peer);
  }
  readonly text: string;
  readonly number: number;
  readonly kind: LspInlayHintKind;
  readonly whitespaceBefore: KBoolean;
  readonly whitespaceAfter: KBoolean;
}

export class LspInlayHintList extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.inlayHints = new NativePtrDecoder()
      .decode(global.es2panda._getInlayHints(peer))
      .map((elPeer: KNativePointer) => {
        return new LspInlayHint(elPeer);
      });
  }
  readonly inlayHints: LspInlayHint[];
}

export class LspSignatureHelpParameter extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.name = unpackString(global.es2panda._getSignatureHelpParameterName(peer));
    this.documentation = new NativePtrDecoder()
      .decode(global.es2panda._getSignatureHelpParameterDocumentation(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSymbolDisplayPart(elPeer);
      });
    this.displayParts = new NativePtrDecoder()
      .decode(global.es2panda._getSignatureHelpParameterDisplayParts(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSymbolDisplayPart(elPeer);
      });
  }
  readonly name: string;
  readonly documentation: LspSymbolDisplayPart[];
  readonly displayParts: LspSymbolDisplayPart[];
}

export class LspSignatureHelpItem extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.prefixDisplayParts = new NativePtrDecoder()
      .decode(global.es2panda._getSignatureHelpItemPrefix(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSymbolDisplayPart(elPeer);
      });
    this.suffixDisplayParts = new NativePtrDecoder()
      .decode(global.es2panda._getSignatureHelpItemSuffix(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSymbolDisplayPart(elPeer);
      });
    this.separatorDisplayParts = new NativePtrDecoder()
      .decode(global.es2panda._getSignatureHelpItemSeparator(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSymbolDisplayPart(elPeer);
      });
    this.parameters = new NativePtrDecoder()
      .decode(global.es2panda._getSignatureHelpItemParameter(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSignatureHelpParameter(elPeer);
      });
    this.documentation = new NativePtrDecoder()
      .decode(global.es2panda._getSignatureHelpItemDocumentation(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSymbolDisplayPart(elPeer);
      });
  }
  readonly prefixDisplayParts: LspSymbolDisplayPart[];
  readonly suffixDisplayParts: LspSymbolDisplayPart[];
  readonly separatorDisplayParts: LspSymbolDisplayPart[];
  readonly parameters: LspSignatureHelpParameter[];
  readonly documentation: LspSymbolDisplayPart[];
}

export class LspSignatureHelpItems extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.items = new NativePtrDecoder()
      .decode(global.es2panda._getSignatureHelpItem(peer))
      .map((elPeer: KNativePointer) => {
        return new LspSignatureHelpItem(elPeer);
      });
    this.applicableSpan = new LspTextSpan(global.es2panda._getApplicableSpan(peer));
    this.selectedItemIndex = global.es2panda._getSelectedItemIndex(peer);
    this.argumentIndex = global.es2panda._getArgumentIndex(peer);
    this.argumentCount = global.es2panda._getArgumentCount(peer);
  }
  readonly items: LspSignatureHelpItem[];
  readonly applicableSpan: LspTextSpan;
  readonly selectedItemIndex: number;
  readonly argumentIndex: number;
  readonly argumentCount: number;
}

export class LspRenameInfoSuccess extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.canRenameSuccess = true;
    this.fileToRename = unpackString(global.es2panda._getRenameSuccessFileName(peer));
    this.kind = unpackString(global.es2panda._getRenameSuccessKind(peer));
    this.displayName = unpackString(global.es2panda._getRenameSuccessDisplayName(peer));
    this.fullDisplayName = unpackString(global.es2panda._getRenameSuccessFullDisplayName(peer));
    this.kindModifiers = unpackString(global.es2panda._getRenameSuccessKindModifiers(peer));
    this.triggerSpan = new LspTextSpan(global.es2panda._getRenameSuccessTriggerSpan(peer));
  }
  readonly canRenameSuccess: boolean;
  readonly fileToRename: string;
  readonly kind: string;
  readonly displayName: string;
  readonly fullDisplayName: string;
  readonly kindModifiers: string;
  readonly triggerSpan: LspTextSpan;
}

export class LspRenameInfoFailure extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.canRenameFailure = false;
    this.localizedErrorMessage = unpackString(global.es2panda._getRenameFailureLocalizedErrorMessage(peer));
  }
  readonly canRenameFailure: boolean;
  readonly localizedErrorMessage: string;
}

export type LspRenameInfoType = LspRenameInfoSuccess | LspRenameInfoFailure;

export class LspRenameLocation extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.fileName = unpackString(global.es2panda._getRenameLocationFileName(peer));
    this.start = global.es2panda._getRenameLocationStart(peer);
    this.end = global.es2panda._getRenameLocationEnd(peer);
    this.line = global.es2panda._getRenameLocationLine(peer);
    this.prefixText = global.es2panda._renameLocationHasPrefixText(peer)
      ? unpackString(global.es2panda._getRenameLocationPrefixText(peer))
      : undefined;
    this.suffixText = global.es2panda._renameLocationHasSuffixText(peer)
      ? unpackString(global.es2panda._getRenameLocationSuffixText(peer))
      : undefined;
  }
  readonly fileName: string;
  readonly start: number;
  readonly end: number;
  readonly line: number;
  readonly prefixText?: string;
  readonly suffixText?: string;
}

export function toAstNodeType(str: string) {
  switch (str) {
    case 'Identifier': {
      return AstNodeType.IDENTIFIER;
    }
    case 'ClassDefinition': {
      return AstNodeType.CLASS_DEFINITION;
    }
    default:
      return AstNodeType.UNKNOWN;
  }
}

export class LspNodeInfo extends LspNode {
  constructor(peer: KNativePointer) {
    super(peer);
    this.name = unpackString(global.es2panda._getNameByNodeInfo(peer));
    this.kind = toAstNodeType(unpackString(global.es2panda._getKindByNodeInfo(peer)));
  }
  readonly name: string;
  readonly kind: AstNodeType;
}
