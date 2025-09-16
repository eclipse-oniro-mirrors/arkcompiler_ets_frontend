/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { ArkAssignStmt, ArkMethod, CallGraph, CallGraphBuilder, Local, LOG_MODULE_TYPE, Logger, Scene, Stmt, Value } from 'arkanalyzer/lib';
import { WarnInfo } from '../../utils/common/Utils';
import { Language } from 'arkanalyzer/lib/core/model/ArkFile';
import { DVFG, DVFGNode } from 'arkanalyzer/lib/VFG/DVFG';
import { DVFGBuilder } from 'arkanalyzer/lib/VFG/builder/DVFGBuilder';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'Utils');

export const CALL_DEPTH_LIMIT = 2;
export class CallGraphHelper {
    private static cgInstance: CallGraph | null = null;

    public static dispose(): void {
        this.cgInstance = null;
    }

    public static getCGInstance(scene: Scene): CallGraph {
        if (!this.cgInstance) {
            this.cgInstance = new CallGraph(scene);
        }
        return this.cgInstance;
    }
}

export class GlobalCallGraphHelper {
    private static cgInstance: CallGraph | null = null;

    public static dispose(): void {
        this.cgInstance = null;
    }

    public static getCGInstance(scene: Scene): CallGraph {
        if (!this.cgInstance) {
            this.cgInstance = new CallGraph(scene);
            let cgBuilder = new CallGraphBuilder(this.cgInstance, scene);
            cgBuilder.buildCHA4WholeProject(true);
        }
        return this.cgInstance;
    }
}

export class DVFGHelper {
    private static dvfgInstance: DVFG;
    private static dvfgBuilder: DVFGBuilder;
    private static built: Set<ArkMethod> = new Set();

    public static dispose(): void {
        // @ts-ignore
        this.dvfgInstance = null;
        // @ts-ignore
        this.dvfgBuilder = null;
        this.built.clear();
    }

    private static createDVFGInstance(scene: Scene): void {
        if (!this.dvfgInstance) {
            this.dvfgInstance = new DVFG(GlobalCallGraphHelper.getCGInstance(scene));
            this.dvfgBuilder = new DVFGBuilder(this.dvfgInstance, scene);
        }
    }

    public static buildSingleDVFG(method: ArkMethod, scene: Scene): void {
        if (!this.dvfgInstance) {
            this.createDVFGInstance(scene);
        }
        if (!this.built.has(method)) {
            this.dvfgBuilder.buildForSingleMethod(method);
            this.built.add(method);
        }
    }

    public static getOrNewDVFGNode(stmt: Stmt, scene: Scene): DVFGNode {
        if (!this.dvfgInstance) {
            this.createDVFGInstance(scene);
        }
        return this.dvfgInstance!.getOrNewDVFGNode(stmt);
    }
}

export const CALLBACK_METHOD_NAME: string[] = [
    'onClick', // 点击事件，当用户点击组件时触发
    'onTouch', // 触摸事件，当手指在组件上按下、滑动、抬起时触发
    'onAppear', // 组件挂载显示时触发
    'onDisAppear', // 组件卸载消失时触发
    'onDragStart', // 拖拽开始事件，当组件被长按后开始拖拽时触发
    'onDragEnter', // 拖拽进入组件范围时触发
    'onDragMove', // 拖拽在组件范围内移动时触发
    'onDragLeave', // 拖拽离开组件范围内时触发
    'onDrop', // 拖拽释放目标，当在本组件范围内停止拖拽行为时触发
    'onKeyEvent', // 按键事件，当组件获焦后，按键动作触发
    'onFocus', // 焦点事件，当组件获取焦点时触发
    'onBlur', // 当组件失去焦点时触发的回调
    'onHover', // 鼠标悬浮事件，鼠标进入或退出组件时触发
    'onMouse', // 鼠标事件，当鼠标按键点击或在组件上移动时触发
    'onAreaChange', // 组件区域变化事件，组件尺寸、位置变化时触发
    'onVisibleAreaChange', // 组件可见区域变化事件，组件在屏幕中的显示区域面积变化时触发
    'onChange',
];

export function getLanguageStr(language: Language): string {
    let targetLan: string = '';
    switch (language) {
        case Language.JAVASCRIPT:
            targetLan = 'javascript';
            break;
        case Language.TYPESCRIPT:
            targetLan = 'typescript';
            break;
        case Language.ARKTS1_1:
            targetLan = 'arkts1.1';
            break;
        case Language.ARKTS1_2:
            targetLan = 'arkts1.2';
            break;
        default:
            break;
    }
    return targetLan;
}

export function getLineAndColumn(stmt: Stmt, operand: Value, exactEndInfo: boolean = false): WarnInfo {
    const arkFile = stmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
    const originPosition = stmt.getOperandOriginalPosition(operand);
    if (arkFile && originPosition) {
        const originPath = arkFile.getFilePath();
        const line = originPosition.getFirstLine();
        const startCol = originPosition.getFirstCol();
        if (exactEndInfo) {
            const endLine = originPosition.getLastLine();
            const endCol = originPosition.getLastCol();
            return { line, startCol, endLine: endLine, endCol: endCol, filePath: originPath };
        } else {
            return { line, startCol, endCol: startCol, filePath: originPath };
        }
    } else {
        logger.debug('ArkFile or operand position is null.');
    }
    return { line: -1, startCol: -1, endCol: -1, filePath: '' };
}

export function getGlobalsDefineInDefaultMethod(defaultMethod: ArkMethod): Map<string, Stmt[]> {
    const globalVarMap: Map<string, Stmt[]> = new Map();
    const stmts = defaultMethod.getBody()?.getCfg().getStmts() ?? [];
    for (const stmt of stmts) {
        if (!(stmt instanceof ArkAssignStmt)) {
            continue;
        }
        const leftOp = stmt.getLeftOp();
        if (!(leftOp instanceof Local)) {
            continue;
        }
        const name = leftOp.getName();
        if (name.startsWith('%') || name === 'this') {
            continue;
        }
        globalVarMap.set(name, [...(globalVarMap.get(name) ?? []), stmt]);
    }
    return globalVarMap;
}

export function getGlobalLocalsInDefaultMethod(defaultMethod: ArkMethod): Map<string, Local> {
    const globals: Map<string, Local> = new Map<string, Local>();
    const stmts = defaultMethod.getBody()?.getCfg().getStmts() ?? [];
    for (const stmt of stmts) {
        if (!(stmt instanceof ArkAssignStmt)) {
            continue;
        }
        const leftOp = stmt.getLeftOp();
        if (!(leftOp instanceof Local)) {
            continue;
        }
        const name = leftOp.getName();
        if (name.startsWith('%') || name === 'this') {
            continue;
        }
        globals.set(leftOp.getName(), leftOp);
    }
    return globals;
}
