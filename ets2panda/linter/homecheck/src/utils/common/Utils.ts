/*
 * Copyright (c) 2024 - 2025 Huawei Device Co., Ltd.
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
import Logger, { LOG_LEVEL, LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { Command, OptionValues } from 'commander';
import { SceneConfig, Sdk } from 'arkanalyzer/lib/Config';
import { NullType, NumberType, Scene, Type, UndefinedType } from 'arkanalyzer';
import { PrimitiveType, UnionType } from 'arkanalyzer/lib';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'Utils');

export class Utils {
    /**
     * 解析命令行选项
     * @param args 命令行参数数组
     * @returns 解析后的选项值
     */
    static parseCliOptions(args: string[]): OptionValues {
        logger.info('Parse cli options.');
        const program = new Command();
        return this.getCliOptions(program, args);
    }

    /**
     * 获取命令行选项
     * @param program Command 对象
     * @param args 命令行参数数组
     * @returns 选项值对象
     */
    static getCliOptions(program: Command, args: string[]): OptionValues {
        program
            .option('--configPath <configPath>', 'rule config path')
            .option('--projectConfigPath <projectConfigPath>', 'project config path')
            .option('--depGraphOutputDir <depGraphOutputDir>', 'output directory of dependency graph')
            .parse(args);
        return program.opts();
    }

    /**
     * 设置日志路径
     * @param logPath 日志路径
     */
    static setLogPath(logPath: string): void {
        Logger.configure(logPath, LOG_LEVEL.INFO, LOG_LEVEL.INFO);
    }

    /**
     * 设置日志信息，包含路径、arkanalyzer日志级别、homecheck日志级别
     */
    static setLogConfig(logPath: string, aaLevel: LOG_LEVEL, hcLevel: LOG_LEVEL): void {
        Logger.configure(logPath, aaLevel, hcLevel);
    }

    /**
     * 获取枚举类型的值
     * @param value - 枚举值，可以是字符串或数字
     * @param enumType - 枚举类型
     * @returns 枚举值对应的枚举类型值
     */
    static getEnumValues(value: string | number, enumType: any): any {
        const key = Object.keys(enumType).find(k => k.toLowerCase() === value || enumType[k as string] === value);
        return enumType[key as string];
    }

    /**
     * 按行号和列号对键值对进行排序
     * @param keyA 格式为 "行号%列号%规则ID" 的字符串
     * @param keyB 格式为 "行号%列号%规则ID" 的字符串
     * @returns 排序比较结果
     */
    public static sortByLineAndColumn(keyA: string, keyB: string): number {
        const [lineA, colA] = keyA.split('%', 2).map(Number);
        const [lineB, colB] = keyB.split('%', 2).map(Number);
        if (lineA !== lineB) {
            return lineA - lineB;
        }
        return colA - colB;
    }

    public static generateSceneForEts2SDK(sdks: Sdk[]): Scene {
        const sceneConfig: SceneConfig = new SceneConfig();
        sceneConfig.buildConfig('ets2SDK', '', sdks);
        sceneConfig.getOptions().enableBuiltIn = false;

        const scene = new Scene();
        scene.buildSceneFromProjectDir(sceneConfig);
        return scene;
    }

    static isUnionTypeContainsType(unionType: UnionType, targetType: Type): boolean {
        for (const t of unionType.getTypes()) {
            if (t === targetType) {
                return true;
            }
        }
        return false;
    }

    /**
     * 检查给出的类型是否是NumberType，或numberType与null、undefined的联合类型
     */
    static isNearlyNumberType(checkType: Type): boolean {
        if (checkType instanceof NumberType) {
            return true;
        }
        if (checkType instanceof UnionType) {
            for (const t of checkType.getTypes()) {
                if (t instanceof NumberType || t instanceof UndefinedType || t instanceof NullType) {
                    continue;
                }
                if (t instanceof UnionType) {
                    if (!this.isNearlyNumberType(t)) {
                        return false;
                    }
                    continue;
                }
                return false;
            }
            return true;
        }
        return false;
    }

    /**
     * 检查给出的类型是否是内置类型，或内置类型与null、undefined的联合类型
     */
    static isNearlyPrimitiveType(checkType: Type): boolean {
        if (checkType instanceof PrimitiveType) {
            return true;
        }
        if (checkType instanceof UnionType) {
            for (const t of checkType.getTypes()) {
                if (t instanceof PrimitiveType || t instanceof UndefinedType || t instanceof NullType) {
                    continue;
                }
                if (t instanceof UnionType) {
                    if (!this.isNearlyPrimitiveType(t)) {
                        return false;
                    }
                    continue;
                }
                return false;
            }
            return true;
        }
        return false;
    }
}

export type WarnInfo = {
    line: number;
    startCol: number;
    endLine?: number;
    endCol: number;
    filePath: string;
};
