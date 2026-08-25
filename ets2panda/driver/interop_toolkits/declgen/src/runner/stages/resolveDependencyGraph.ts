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

import { promises as fs } from 'node:fs';
import * as path from 'node:path';
import * as common from '@interop-toolkits/common';
import * as dependencyResolver from 'dependency-resolver';
import * as predefine from '../../predefine';
import * as utils from '../../utils';
import type { Context } from '../context';
import { ARKTS_CONFIG_ARTIFACT, DEPENDENCY_GRAPH_ARTIFACT } from './stageArtifacts';

const SHOW_FULL_DEPENDENCY_GRAPH = process.env.DECLGEN_SHOW_FULL_DEPENDENCY_GRAPH === 'true';

export function createResolveDependencyGraphStage(): common.framework.pipeline.ProvidedStage<
  Context,
  readonly [typeof ARKTS_CONFIG_ARTIFACT],
  typeof DEPENDENCY_GRAPH_ARTIFACT
> {
  return common.framework.pipeline.Stage.start<Context>('resolve-dependency-graph')
    .requires(ARKTS_CONFIG_ARTIFACT)
    .use('resolve-dependency-graph', {
      inputs: [],
      run: async (scope): Promise<dependencyResolver.DependencyGraph> => {
        const arktsconfigPath = scope.get(ARKTS_CONFIG_ARTIFACT);
        const resolver = await createDependencyResolver(scope.context, arktsconfigPath);
        const dependencyGraph = resolver.resolve();
        if (SHOW_FULL_DEPENDENCY_GRAPH) {
          await writeFullDependencyGraph(
            dependencyGraph,
            resolver.context,
            path.join(scope.context.buildConfig.cachePath, predefine.DEPENDENCY_GRAPH_FILE_NAME),
          );
        }
        return dependencyGraph;
      },
    })
    .provides(DEPENDENCY_GRAPH_ARTIFACT, {
      build: (_scope, outputs): dependencyResolver.DependencyGraph => outputs['resolve-dependency-graph'],
    });
}

async function createDependencyResolver(
  context: Context,
  arktsconfigPath: string,
): Promise<dependencyResolver.CrossLanguageResolver> {
  const cachePath = path.join(context.buildConfig.cachePath, 'dependency-resolver');
  await fs.mkdir(cachePath, { recursive: true });
  const resolverContext: dependencyResolver.Context = {
    fileManager: context.fileManager,
    cachePath,
  };
  const depAnalyzerPath = utils.getDepAnalyzerPath(context.buildConfig.pandaSdkPath!);
  const staticResolver = new dependencyResolver.StaticResolver(depAnalyzerPath, arktsconfigPath);
  const dynamicResolver = new dependencyResolver.DynamicResolver(
    context.buildConfig.projectRootPath,
    utils.loadTsCompilerOptions(context.tsconfigPath),
  );
  return new dependencyResolver.CrossLanguageResolver(resolverContext, dynamicResolver, staticResolver);
}

async function writeFullDependencyGraph(
  dependencyGraph: dependencyResolver.DependencyGraph,
  context: dependencyResolver.Context,
  savePath: string,
): Promise<void> {
  const viewModel = dependencyResolver.graphToViewModel(dependencyGraph, context);
  await fs.writeFile(savePath, dependencyResolver.renderGraphHtml(viewModel), 'utf-8');
}
