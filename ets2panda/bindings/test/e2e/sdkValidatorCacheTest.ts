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

import fs from 'fs';
import path from 'path';
import { LspDiagnosticNode } from '../../src/lsp';
import { getLsp, getLspWithUi, getRealPath } from '../utils';

interface UnpublishedExports {
    trimmedNames: string[];
    trimmedOverloadComponents: string[];
}

interface ExportDeclarationMatch {
    name: string;
    isOverload: boolean;
}

const FALLBACK_TRIMMED_NAMES = ['NavDestinationModuleInfo', 'NavigationModuleInfo'];

const FALLBACK_TRIMMED_OVERLOAD_COMPONENTS = [
    'AlphabetIndexer', 'ArcAlphabetIndexer', 'ArcList', 'ArcListItem',
    'ArcScrollBar', 'ArcSwiper', 'Badge', 'Blank',
    'Button', 'CalendarPicker', 'Canvas', 'Checkbox',
    'CheckboxGroup', 'Circle', 'Column', 'ColumnSplit',
    'ContainerReader', 'ContainerSpan', 'ContentSlot', 'Counter',
    'DataPanel', 'DatePicker', 'DistortionComponent', 'Divider',
    'DynamicComponent', 'EffectComponent', 'Ellipse', 'EmbeddedComponent',
    'Flex', 'FlowItem', 'FolderStack', 'ForEach',
    'Gauge', 'Grid', 'GridCol', 'GridItem',
    'GridRow', 'Hyperlink', 'If', 'Image',
    'ImageAnimator', 'ImageSpan', 'IndicatorComponent', 'LazyColumnLayout',
    'LazyDynamicLayout', 'LazyForEach', 'LazyVGridLayout', 'LazyVWaterFlowLayout',
    'Line', 'List', 'ListItem', 'ListItemGroup',
    'LoadingProgress', 'Marquee', 'Menu', 'MenuItem',
    'MenuItemGroup', 'NavDestination', 'Navigation', 'NodeContainer',
    'Particle', 'Path', 'PatternLock', 'PluginComponent',
    'Polygon', 'Polyline', 'Progress', 'QRCode',
    'Radio', 'Rating', 'Rect', 'Refresh',
    'RelativeContainer', 'Repeat', 'RichEditor', 'Row',
    'RowSplit', 'Scroll', 'ScrollBar', 'Search',
    'SecurityUIExtensionComponent', 'Select', 'Shape', 'SideBarContainer',
    'Slider', 'Span', 'Stack', 'Swiper',
    'SymbolGlyph', 'SymbolSpan', 'TabContent', 'Tabs',
    'Text', 'TextArea', 'TextClock', 'TextInput',
    'TextPicker', 'TextTimer', 'TimePicker', 'Toggle',
    'ToolBarItem', 'UIExtensionComponent', 'UIPickerComponent', 'UnionEffectContainer',
    'Video', 'WaterFlow', 'WithTheme', 'XComponent'
];

/**
 * Locate the SDK api root (`code/interface/sdk-js/api`) by walking up from the
 * test file. Returns `null` when not in a full-repo environment.
 */
function findSdkApiRoot(testFilePath: string): string | null {
    let dir = path.dirname(testFilePath);
    for (let i = 0; i < 10; i++) {
        const candidate = path.join(dir, 'code', 'interface', 'sdk-js', 'api');
        if (fs.existsSync(candidate)) {
            return candidate;
        }
        const parent = path.dirname(dir);
        if (parent === dir) {
            break;
        }
        dir = parent;
    }
    return null;
}

/**
 * Map an import specifier from the test `.ets` file to its SDK declaration
 * file path under the api root. Returns `null` for specifiers that do not map
 * to a static `.d.ets` declaration (e.g. std / escompat / arkruntime).
 */
function specifierToSdkFile(specifier: string, apiRoot: string): string | null {
    if (specifier.startsWith('arkui.component.')) {
        const name = specifier.slice('arkui.component.'.length);
        return path.join(apiRoot, 'arkui', 'component', `${name}.static.d.ets`);
    }
    if (specifier.startsWith('@ohos.arkui.')) {
        return path.join(apiRoot, `${specifier}.static.d.ets`);
    }
    return null;
}

/**
 * Extract the JSDoc block immediately preceding a line index in `lines`.
 * Returns the block text (including `/** ... *\/`), or empty string if none.
 */
function precedingJsDoc(lines: string[], lineIndex: number): string {
    let i = lineIndex - 1;
    // Skip decorators (e.g. @Builder, @ComponentBuilder) and blank lines.
    while (i >= 0 && (lines[i].trim() === '' || lines[i].trim().startsWith('@'))) {
        i--;
    }
    if (i < 0 || !lines[i].includes('*/')) {
        return '';
    }
    const end = i;
    while (i >= 0 && !lines[i].includes('/**')) {
        i--;
    }
    if (i < 0) {
        return '';
    }
    return lines.slice(i, end + 1).join('\n');
}

/**
 * Extract all import specifiers (`from '...'`) from a source file, deduplicated.
 */
function collectImportSpecifiers(source: string): string[] {
    const importPattern = /from\s+['"]([^'"]+)['"]/g;
    const specifiers = new Set<string>();
    let match: RegExpExecArray | null;
    while ((match = importPattern.exec(source)) !== null) {
        specifiers.add(match[1]);
    }
    return [...specifiers];
}

/**
 * Match a single line against `export declare function/interface/type`.
 * Returns the exported name and whether it is an overload-probed function.
 */
function matchExportDeclaration(line: string): ExportDeclarationMatch | null {
    const functionRe = /^\s*export\s+declare\s+function\s+([A-Za-z_$][\w$]*)\s*(?:<[^>]*>)?\s*\(/;
    const interfaceRe = /^\s*export\s+declare\s+interface\s+([A-Za-z_$][\w$]*)/;
    const typeRe = /^\s*export\s+declare\s+type\s+([A-Za-z_$][\w$]*)/;
    const fnMatch = line.match(functionRe);
    if (fnMatch !== null) {
        return { name: fnMatch[1], isOverload: true };
    }
    const ifaceMatch = line.match(interfaceRe);
    if (ifaceMatch !== null) {
        return { name: ifaceMatch[1], isOverload: false };
    }
    const typeMatch = line.match(typeRe);
    if (typeMatch !== null) {
        return { name: typeMatch[1], isOverload: false };
    }
    return null;
}

/**
 * Scan one SDK declaration file for exports whose preceding JSDoc contains
 * `@unpublished`. Returns the unpublished overload functions and names found.
 */
function scanSdkDeclarationFile(file: string): { overloads: string[]; names: string[] } {
    const overloads: string[] = [];
    const names: string[] = [];
    const lines = fs.readFileSync(file, 'utf-8').split('\n');
    for (let i = 0; i < lines.length; i++) {
        const declaration = matchExportDeclaration(lines[i]);
        if (declaration === null) {
            continue;
        }
        if (!precedingJsDoc(lines, i).includes('@unpublished')) {
            continue;
        }
        if (declaration.isOverload) {
            overloads.push(declaration.name);
        } else {
            names.push(declaration.name);
        }
    }
    return { overloads, names };
}

/**
 * Scan the SDK declarations imported by `testEtsPath` for `@unpublished`
 * exports. Returns the dynamically built lists, or `null` when the SDK api
 * root cannot be located (non-full-repo environment).
 */
function scanUnpublishedExports(testEtsPath: string): UnpublishedExports | null {
    const apiRoot = findSdkApiRoot(testEtsPath);
    if (apiRoot === null) {
        return null;
    }

    let testSource: string;
    try {
        testSource = fs.readFileSync(testEtsPath, 'utf-8');
    } catch {
        return null;
    }

    const overloadComponents = new Set<string>();
    const names = new Set<string>();
    collectImportSpecifiers(testSource).forEach((spec) => {
        const file = specifierToSdkFile(spec, apiRoot);
        if (file === null || !fs.existsSync(file)) {
            return;
        }
        const unpublished = scanSdkDeclarationFile(file);
        unpublished.overloads.forEach((name) => overloadComponents.add(name));
        unpublished.names.forEach((name) => names.add(name));
    });

    if (overloadComponents.size === 0 && names.size === 0) {
        return null;
    }

    return {
        trimmedNames: [...names].sort(),
        trimmedOverloadComponents: [...overloadComponents].sort()
    };
}

describe('sdkValidatorCacheTest', () => {
    const moduleName = 'sdkValidatorCache';
    const testEtsPath = getRealPath(moduleName, 'unpublishedSdkTest.ets');

    const scanned = scanUnpublishedExports(testEtsPath);
    const trimmedNames = scanned?.trimmedNames ?? FALLBACK_TRIMMED_NAMES;
    const trimmedOverloadComponents = scanned?.trimmedOverloadComponents ?? FALLBACK_TRIMMED_OVERLOAD_COMPONENTS;

    const absentApiPatterns = [
        /^Unresolved reference [A-Za-z_$][\w$]*$/,
        /^Cannot find type '.*'\.$/,
        /^Cannot find imported element '.*'$/,
        /^Imported element not exported '.*'$/,
        /^Can't find prefix for '.*' in .*arktsconfig\.json$/,
        /^Not supported path: /,
        /^Cannot find import: /
    ];

    function expectTrimmedSdkDiagnostics(diagnostics: LspDiagnosticNode[]): void {
        const errors = diagnostics
            .filter((diagnostic) => diagnostic.severity === 1)
            .map((diagnostic) => ({
                line: diagnostic.range.start.line,
                message: diagnostic.message.toString()
            }));
        const messages = errors.map((error) => error.message);

        const callSignaturePattern = /^No matching call signature for ([A-Za-z_$][\w$]*)\(/;
        const arityPattern = /^Expected \d+ arguments, got \d+\.$/;
        const missingElementPattern = /^(?:Cannot find imported element|Imported element not exported) '([A-Za-z_$][\w$]*)'$/;
        const unresolvedPattern = /^Unresolved reference ([A-Za-z_$][\w$]*)$/;

        const probedOverloads = [
            ...new Set(messages.map((message) => message.match(callSignaturePattern)?.[1]).filter(Boolean) as string[])
        ].sort();
        expect(probedOverloads.filter((name) => !trimmedOverloadComponents.includes(name))).toEqual([]);

        const arityErrors = errors.filter((error) => arityPattern.test(error.message));
        if (arityErrors.length > 0) {
            const testLines = fs.readFileSync(testEtsPath, 'utf-8').split('\n');
            const callStartRe = /^\s*([A-Za-z_$][\w$]*)\s*\(/;
            const arityComponents = arityErrors
                .map((error) => testLines[error.line]?.match(callStartRe)?.[1])
                .filter((name): name is string => name !== undefined);
            expect(arityComponents.filter((name) => !trimmedOverloadComponents.includes(name))).toEqual([]);
        }

        const absentAccounted = new Set<string>();
        messages.forEach((message) => {
            const absentName = message.match(missingElementPattern)?.[1] ?? message.match(unresolvedPattern)?.[1];
            if (absentName !== undefined) {
                absentAccounted.add(absentName);
            }
        });

        expect(
            trimmedOverloadComponents.filter(
                (name) => !probedOverloads.includes(name) && !absentAccounted.has(name)
            )
        ).toEqual([]);

        trimmedNames.forEach((name) => {
            expect(
                messages.some(
                    (message) =>
                        message === `Cannot find imported element '${name}'` ||
                        message === `Imported element not exported '${name}'`
                )
            ).toBe(true);
        });

        expect(
            messages.filter(
                (message) =>
                    !callSignaturePattern.test(message) &&
                    !arityPattern.test(message) &&
                    !missingElementPattern.test(message) &&
                    !absentApiPatterns.some((pattern) => pattern.test(message))
            )
        ).toEqual([]);
    }

    describe('With UI Plugins', () => {
        const getUiLsp = (): ReturnType<typeof getLspWithUi> => getLspWithUi(moduleName);

        (process.env.SKIP_UI_PLUGINS ? test.skip : test)('unpublishedSdkUiTest', () => {
            const res = getUiLsp().getSemanticDiagnostics(getRealPath(moduleName, 'unpublishedSdkTest.ets'));
            expectTrimmedSdkDiagnostics(res?.diagnostics ?? []);
        });
    });

});
