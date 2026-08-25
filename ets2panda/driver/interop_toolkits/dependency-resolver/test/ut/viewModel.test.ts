/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import * as common from '@interop-toolkits/common';
import type { Context } from '../../src/context';
import { DependencyGraph } from '../../src/resolver';
import { NodeType } from '../../src/resolver/graph';
import { renderGraphHtml } from '../../src/visualize/htmlRenderer';
import {
  DYNAMIC_INTEROP_SDK_MODULE_ID,
  DYNAMIC_SDK_MODULE_ID,
  EXTERNAL_MODULE_ID,
  STATIC_INTEROP_SDK_MODULE_ID,
  STATIC_SDK_MODULE_ID,
  graphToViewModel,
} from '../../src/visualize/viewModel';

describe('graphToViewModel', () => {
  it('uses metadata language and groups SDK files by source', () => {
    const staticSdk = '/sdk/static/api.d.ets';
    const dynamicSdk = '/sdk/dynamic/api.d.ts';
    const staticInteropSdk = '/sdk/static-interop/api.d.ts';
    const dynamicInteropSdk = '/sdk/dynamic-interop/api.d.ets';
    const unknown = '/unknown/file.ets';
    const metadata = new Map<string, common.fileManager.FileMeta>([
      [staticSdk, fileMeta(staticSdk, common.fileUtils.Language.STATIC, common.fileManager.Owner.SDK)],
      [dynamicSdk, fileMeta(dynamicSdk, common.fileUtils.Language.DYNAMIC, common.fileManager.Owner.SDK)],
      [
        staticInteropSdk,
        fileMeta(staticInteropSdk, common.fileUtils.Language.DYNAMIC, common.fileManager.Owner.INTEROP_SDK),
      ],
      [
        dynamicInteropSdk,
        fileMeta(dynamicInteropSdk, common.fileUtils.Language.STATIC, common.fileManager.Owner.INTEROP_SDK),
      ],
    ]);
    const context = {
      fileManager: {
        queryFileMeta: (filePath: string) => metadata.get(filePath),
      },
    } as unknown as Context;
    const graph = new DependencyGraph(
      new Map([
        [staticSdk, node(staticSdk, NodeType.DYNAMIC)],
        [dynamicSdk, node(dynamicSdk, NodeType.STATIC)],
        [staticInteropSdk, node(staticInteropSdk, NodeType.STATIC)],
        [dynamicInteropSdk, node(dynamicInteropSdk, NodeType.DYNAMIC)],
        [unknown, node(unknown, NodeType.DYNAMIC, true)],
      ]),
    );

    const model = graphToViewModel(graph, context);

    expect(fileByPath(model, staticSdk)).toMatchObject({
      kind: 'static',
      isSdk: true,
      moduleId: STATIC_SDK_MODULE_ID,
    });
    expect(fileByPath(model, dynamicSdk)).toMatchObject({
      kind: 'dynamic',
      isSdk: true,
      moduleId: DYNAMIC_SDK_MODULE_ID,
    });
    expect(fileByPath(model, staticInteropSdk)).toMatchObject({
      kind: 'dynamic',
      isSdk: true,
      moduleId: STATIC_INTEROP_SDK_MODULE_ID,
    });
    expect(fileByPath(model, dynamicInteropSdk)).toMatchObject({
      kind: 'static',
      isSdk: true,
      moduleId: DYNAMIC_INTEROP_SDK_MODULE_ID,
    });
    expect(fileByPath(model, unknown)).toMatchObject({ kind: 'dynamic', isSdk: false, moduleId: EXTERNAL_MODULE_ID });
    expect(model.modules).toEqual(
      expect.arrayContaining([
        { id: STATIC_SDK_MODULE_ID, label: STATIC_SDK_MODULE_ID, language: common.fileUtils.Language.STATIC },
        { id: DYNAMIC_SDK_MODULE_ID, label: DYNAMIC_SDK_MODULE_ID, language: common.fileUtils.Language.DYNAMIC },
        {
          id: STATIC_INTEROP_SDK_MODULE_ID,
          label: STATIC_INTEROP_SDK_MODULE_ID,
          language: common.fileUtils.Language.DYNAMIC,
        },
        {
          id: DYNAMIC_INTEROP_SDK_MODULE_ID,
          label: DYNAMIC_INTEROP_SDK_MODULE_ID,
          language: common.fileUtils.Language.STATIC,
        },
      ]),
    );
  });

  it('uses node language when rendering a sentinel without metadata', () => {
    const staticSentinel = '/external/static.ets';
    const dynamicSentinel = '/external/dynamic.ts';
    const graph = new DependencyGraph(
      new Map([
        [staticSentinel, node(staticSentinel, NodeType.STATIC, true)],
        [dynamicSentinel, node(dynamicSentinel, NodeType.DYNAMIC, true)],
      ]),
    );

    const model = graphToViewModel(graph);

    expect(fileByPath(model, staticSentinel)).toMatchObject({ kind: 'static', moduleId: EXTERNAL_MODULE_ID });
    expect(fileByPath(model, dynamicSentinel)).toMatchObject({
      kind: 'dynamic',
      isSentinel: true,
      moduleId: EXTERNAL_MODULE_ID,
    });
    expect(fileByPath(model, staticSentinel)).toMatchObject({ isSentinel: true });
  });

  it('renders sentinel nodes as squares and regular nodes as circles', () => {
    const sentinel = '/external/sentinel.ets';
    const regular = '/project/regular.ts';
    const model = graphToViewModel(
      new DependencyGraph(
        new Map([
          [sentinel, node(sentinel, NodeType.STATIC, true)],
          [regular, node(regular, NodeType.DYNAMIC)],
        ]),
      ),
    );

    const html = renderGraphHtml(model);

    expect(html).toContain('var shape=f.isSentinel');
    expect(html).toContain('?el("rect"');
    expect(html).toContain(':el("circle"');
    expect(html).toContain('querySelectorAll(".node-shape")');
  });

  it('renders only static and dynamic language controls', () => {
    const html = renderGraphHtml({ modules: [], files: [] });

    expect(html).toContain('id="color-static"');
    expect(html).toContain('id="color-dynamic"');
    expect(html).not.toContain('id="color-external"');
  });

  it('renders middle edge arrows and an SDK visibility toggle', () => {
    const html = renderGraphHtml({ modules: [], files: [] });

    expect(html).toContain('"marker-mid":"url(#arrow)"');
    expect(html).not.toContain('"marker-end":"url(#arrow)"');
    expect(html).toContain('id="toggle-sdk"');
    expect(html).toContain('function setSdkHidden(hidden)');
    expect(html).toContain('order.sort(function(a,b){return Number(isSdkGroup(a))-Number(isSdkGroup(b));})');
    expect(html).toContain('layout(hidden);updateLayout();');
  });
});

function fileMeta(
  filePath: string,
  language: common.fileUtils.Language,
  owner: common.fileManager.Owner,
): common.fileManager.FileMeta {
  return {
    fileName: filePath.split('/').at(-1) ?? filePath,
    filePath,
    language,
    owner,
  };
}

function fileByPath(model: ReturnType<typeof graphToViewModel>, filePath: string) {
  return model.files.find((file) => file.path === filePath);
}

function node(fileName: string, type: NodeType, isSentinel = false) {
  return { fileName, type, isSentinel, isResolved: !isSentinel, dependencies: [], dependants: [] };
}
