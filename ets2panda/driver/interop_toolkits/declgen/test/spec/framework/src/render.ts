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

/**
 * Markdown renderer. Walks the canonical model and emits the full spec
 * document; `structure` picks the outer axis (topic vs direction) at render
 * time, independent of the on-disk layout. Output is byte-stable for a given
 * model so that `render --check` can gate CI.
 */

import * as fs from 'fs';

import {
  compatLabel,
  isCase,
  isSection,
  type Lang,
  type SectionNode,
  type SpecModel,
  type SpecNode,
  type Structure,
} from './model';
import { normalizeBody, stripLicense } from './mdutil';

type Format = 'md' | 'rst';

interface RenderContext {
  model: SpecModel;
  lang: Lang;
  structure: Structure;
}

function injectPlaceholders(body: string, node: SpecNode, specDir: string): string {
  return body.replace(/\{\{\s*(input|output|sample:input|sample:output)\s*\}\}/g, (_all, name: string) =>
    codeBlocksFor(name, node, specDir),
  );
}

function readFilesFor(name: string, node: SpecNode, specDir: string): { label: string; content: string }[] {
  const sectionsDir = `${specDir}/sections`.replace(/\\/g, '/');
  if (isCase(node)) {
    switch (name) {
      case 'input':
        return node.inputFiles.map((abs) => ({
          label: abs.replace(node.inputDir + '/', '').replace(/\\/g, '/'),
          content: fs.readFileSync(abs, 'utf-8'),
        }));
      case 'output':
        return readCaseOutputs(node).map((abs) => ({
          label: abs.replace(node.outputDir + '/', '').replace(/\\/g, '/'),
          content: fs.readFileSync(abs, 'utf-8'),
        }));
      case 'sample:input':
        return node.sampleInput ? [{ label: '', content: fs.readFileSync(node.sampleInput, 'utf-8') }] : [];
      case 'sample:output':
        return node.sampleOutput ? [{ label: '', content: fs.readFileSync(node.sampleOutput, 'utf-8') }] : [];
    }
  }
  void sectionsDir;
  return [];
}

function readCaseOutputs(node: SpecNode): string[] {
  if (!isCase(node)) {
    return [];
  }
  const walk = (dir: string): string[] => {
    if (!fs.existsSync(dir)) {
      return [];
    }
    const out: string[] = [];
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
      const full = `${dir}/${entry.name}`;
      if (entry.isDirectory()) {
        out.push(...walk(full));
      } else {
        out.push(full);
      }
    }
    return out.sort();
  };
  return walk(node.outputDir);
}

function codeBlocksFor(name: string, node: SpecNode, specDir: string): string {
  const files = readFilesFor(name, node, specDir);
  if (files.length === 0) {
    return '';
  }
  const blocks = files.map(({ label, content }) => {
    const code = stripLicense(content).trim();
    const prefix =
      files.length > 1 && label !== ''
        ? `*${name === 'input' || name === 'sample:input' ? 'input' : 'output'}/${label}*\n\n`
        : '';
    return `${prefix}\`\`\`typescript\n${code}\n\`\`\``;
  });
  return blocks.join('\n\n');
}

function renderCase(node: SpecNode, ctx: RenderContext): string {
  if (!isCase(node)) {
    return '';
  }
  const parts = [`**${node.desc[ctx.lang].title}**`, compatLabel(node.compat, ctx.lang)];
  const body = normalizeBody(injectPlaceholders(node.desc[ctx.lang].body, node, ctx.model.specDir));
  if (body !== '') {
    parts.push(body);
  }
  return parts.join('\n\n');
}

function renderDeep(node: SpecNode, ctx: RenderContext): string {
  if (isCase(node)) {
    return renderCase(node, ctx);
  }
  const parts: string[] = [`**${node.titles[ctx.lang]}**`];
  const narrative = normalizeBody(node.narrative[ctx.lang]);
  if (narrative !== '') {
    parts.push(narrative);
  }
  const children = node.children.map((child) => renderDeep(child, ctx)).filter((block) => block !== '');
  if (children.length > 0) {
    parts.push(children.join('\n\n'));
  }
  return parts.join('\n\n');
}

function renderTopicChildren(topic: SectionNode, ctx: RenderContext): string {
  const parts: string[] = [];
  const narrative = normalizeBody(topic.narrative[ctx.lang]);
  if (narrative !== '') {
    parts.push(narrative);
  }
  const children = topic.children.map((child) => renderDeep(child, ctx)).filter((block) => block !== '');
  if (children.length > 0) {
    parts.push(children.join('\n\n'));
  }
  return parts.join('\n\n');
}

function directionChildrenOf(direction: SectionNode): SectionNode[] {
  return direction.children.filter(isSection);
}

interface Chapter {
  /** Sort key: first-appearance position in SUMMARY order. */
  key: number;
  title: string;
  body: string;
}

function generalChapters(ctx: RenderContext, general: SectionNode[]): Chapter[] {
  return general.map((node) => ({
    key: ctx.model.roots.indexOf(node),
    title: node.titles[ctx.lang],
    body: renderDeep(node, ctx),
  }));
}

function directionChapter(ctx: RenderContext, direction: SectionNode): Chapter {
  const parts: string[] = [];
  const narrative = normalizeBody(direction.narrative[ctx.lang]);
  if (narrative !== '') {
    parts.push(narrative);
  }
  for (const topic of directionChildrenOf(direction)) {
    const body = renderTopicChildren(topic, ctx);
    parts.push(`### ${topic.titles[ctx.lang]}\n\n${body}`.trimEnd());
  }
  return {
    key: ctx.model.roots.indexOf(direction),
    title: direction.titles[ctx.lang],
    body: parts.join('\n\n'),
  };
}

function collectDirectionMajor(ctx: RenderContext, general: SectionNode[], directions: SectionNode[]): Chapter[] {
  const chapters = generalChapters(ctx, general);
  directions.forEach((direction) => {
    chapters.push(directionChapter(ctx, direction));
  });
  return chapters.sort((a, b) => a.key - b.key);
}

/** Topic order = first appearance across direction groups (SUMMARY pre-order). */
function topicOrderOf(directions: SectionNode[]): SectionNode[] {
  const firstBySlug = new Map<string, SectionNode>();
  for (const direction of directions) {
    for (const topic of directionChildrenOf(direction)) {
      if (!firstBySlug.has(topic.slug)) {
        firstBySlug.set(topic.slug, topic);
      }
    }
  }
  return [...firstBySlug.values()];
}

/** Each topic's sort key = earliest position among the direction roots. */
function topicKeysOf(ctx: RenderContext, directions: SectionNode[]): Map<string, number> {
  const topicKeys = new Map<string, number>();
  directions.forEach((direction) => {
    const rootKey = ctx.model.roots.indexOf(direction);
    for (const topic of directionChildrenOf(direction)) {
      const known = topicKeys.get(topic.slug);
      topicKeys.set(topic.slug, known === undefined ? rootKey : Math.min(known, rootKey));
    }
  });
  return topicKeys;
}

function renderDirectionTopicPart(ctx: RenderContext, direction: SectionNode, topic: SectionNode): string {
  const body = renderTopicChildren(topic, ctx);
  return `### ${direction.titles[ctx.lang]}\n\n${body}`.trimEnd();
}

function topicChapterBody(ctx: RenderContext, topic: SectionNode, directions: SectionNode[]): string {
  const parts: string[] = [];
  for (const direction of directions) {
    const counterpart = directionChildrenOf(direction).find((child) => child.slug === topic.slug);
    if (counterpart) {
      parts.push(renderDirectionTopicPart(ctx, direction, counterpart));
    }
  }
  return parts.join('\n\n');
}

function collectTopicMajor(ctx: RenderContext, general: SectionNode[], directions: SectionNode[]): Chapter[] {
  const chapters = generalChapters(ctx, general);
  const topicKeys = topicKeysOf(ctx, directions);
  for (const topic of topicOrderOf(directions)) {
    const body = topicChapterBody(ctx, topic, directions);
    chapters.push({ key: topicKeys.get(topic.slug)!, title: topic.titles[ctx.lang], body });
  }
  return chapters.sort((a, b) => a.key - b.key);
}

function collectChapters(ctx: RenderContext): Chapter[] {
  const roots = ctx.model.roots.filter(isSection);
  const general = roots.filter((node) => !node.direction);
  const directions = roots.filter((node) => node.direction);
  if (ctx.structure === 'direction-major') {
    return collectDirectionMajor(ctx, general, directions);
  }
  return collectTopicMajor(ctx, general, directions);
}

export function renderDoc(model: SpecModel, lang: Lang, format: Format, structure?: Structure): string {
  if (format !== 'md') {
    throw new Error(`format "${format}" is not implemented yet (scheduled in M3)`);
  }
  const ctx: RenderContext = { model, lang, structure: structure ?? model.config.structure };
  const lines: string[] = [];
  for (const fm of model.config.frontMatter) {
    lines.push(fm);
  }
  if (model.config.frontMatter.length > 0) {
    lines.push('');
  }
  lines.push(`# ${model.config.title[lang]}`);
  const chapters = collectChapters(ctx);
  for (const chapter of chapters) {
    lines.push('', `## ${chapter.title}`, '', chapter.body);
  }
  return (
    lines
      .join('\n')
      .replace(/\n{3,}/g, '\n\n')
      .trimEnd() + '\n'
  );
}

export function outputDocName(docId: string, lang: Lang, format: Format): string {
  return `${docId}.${lang}.${format}`;
}
