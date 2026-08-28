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

import { LspDiagnosticNode } from '../../src/lsp';
import { getLsp, getLspWithUi, getRealPath } from '../utils';

describe('sdkValidatorCacheTest', () => {
    const moduleName = 'sdkValidatorCache';

    const trimmedNames = ['NavDestinationModuleInfo', 'NavigationModuleInfo'];

    const trimmedOverloadComponents = [
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

    function expectTrimmedSdkDiagnostics(diagnostics: LspDiagnosticNode[]): void {
        const errors = diagnostics.filter((diagnostic) => diagnostic.severity === 1);
        const messages = errors.map((diagnostic) => diagnostic.message.toString());

        const callSignaturePattern = /^No matching call signature for ([A-Za-z_$][\w$]*)\(/;
        const probedOverloads = [
            ...new Set(messages.map((message) => message.match(callSignaturePattern)?.[1]).filter(Boolean))
        ].sort();
        expect(probedOverloads).toEqual(trimmedOverloadComponents);

        trimmedNames.forEach((name) => {
            expect(
                messages.some((message) =>
                    message === `Cannot find imported element '${name}'` ||
                    message === `Imported element not exported '${name}'`)
            ).toBe(true);
        });

        const arityErrors = errors
            .filter((diagnostic) => /^Expected \d+ arguments, got \d+\.$/.test(diagnostic.message.toString()))
            .map((diagnostic) => ({ line: diagnostic.range.start.line, message: diagnostic.message.toString() }));
        expect(arityErrors).toEqual([
            { line: 135, message: 'Expected 2 arguments, got 1.' },
            { line: 155, message: 'Expected 0 arguments, got 1.' },
            { line: 163, message: 'Expected 2 arguments, got 1.' },
            { line: 170, message: 'Expected 2 arguments, got 1.' },
            { line: 172, message: 'Expected 0 arguments, got 1.' },
            { line: 175, message: 'Expected 2 arguments, got 1.' },
            { line: 181, message: 'Expected 0 arguments, got 1.' },
            { line: 183, message: 'Expected 0 arguments, got 2.' }
        ]);

        expect(messages).toHaveLength(
            trimmedOverloadComponents.length + trimmedNames.length + arityErrors.length
        );
    }

    describe('With UI Plugins', () => {
        const getUiLsp = (): ReturnType<typeof getLspWithUi> => getLspWithUi(moduleName);

        (process.env.SKIP_UI_PLUGINS ? test.skip : test)('unpublishedSdkUiTest', () => {
            const res = getUiLsp().getSemanticDiagnostics(getRealPath(moduleName, 'unpublishedSdkTest.ets'));
            expectTrimmedSdkDiagnostics(res?.diagnostics ?? []);
        });
    });

});