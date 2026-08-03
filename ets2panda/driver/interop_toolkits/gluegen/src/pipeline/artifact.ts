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

declare const ARTIFACT_VALUE: unique symbol;

/** A named, typed value produced and consumed by pipeline stages. */
export interface Artifact<Name extends string, Value> {
  readonly name: Name;
  readonly [ARTIFACT_VALUE]?: Value;
}

export type AnyArtifact = Artifact<string, unknown>;
export type ArtifactList = readonly AnyArtifact[];

export type ArtifactValue<Current extends AnyArtifact> = Current extends Artifact<string, infer Value> ? Value : never;

export type ArtifactRecord<Artifacts extends ArtifactList> = {
  readonly [Current in Artifacts[number] as Current['name']]: ArtifactValue<Current>;
};

/**
 * Binds artifact names to their value types once, then creates strongly typed
 * artifacts with a single call.
 */
export function createArtifactFactory<ArtifactTypes extends object>(): <
  const Name extends Extract<keyof ArtifactTypes, string>,
>(
  name: Name,
) => Artifact<Name, ArtifactTypes[Name]> {
  return <const Name extends Extract<keyof ArtifactTypes, string>>(
    name: Name,
  ): Artifact<Name, ArtifactTypes[Name]> => ({ name });
}
