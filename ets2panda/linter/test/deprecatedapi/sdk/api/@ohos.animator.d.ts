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

export declare class Animator {
    static createAnimator(options: AnimatorOptions): AnimatorResult;
    static create(options: AnimatorOptions): AnimatorResult;
}

export interface AnimatorOptions {
    duration: number;
    easing: string;
    delay: number;
    fill: "none" | "forwards" | "backwards" | "both";
    direction: "normal" | "reverse" | "alternate" | "alternate-reverse";
    iterations: number;
    begin: number;
    end: number;
}

export interface AnimatorResult {
    update(options: AnimatorOptions): void;
    reset(options: AnimatorOptions): void;
    reset(options: AnimatorOptions | SimpleAnimatorOptions): void;
    play(): void;
    finish(): void;
    pause(): void;
    cancel(): void;
    reverse(): void;
    onframe: (progress: number) => void;
    onFrame: (progress: number) => void;
    onfinish: () => void;
    onFinish: () => void;
    oncancel: () => void;
    onCancel: () => void;
    onrepeat: () => void;
    onRepeat: () => void;
    setExpectedFrameRateRange(rateRange: ExpectedFrameRateRange): void;
}

export declare class SimpleAnimatorOptions {
    duration(duration: number): SimpleAnimatorOptions;
    easing(curve: string): SimpleAnimatorOptions;
    delay(delay: number): SimpleAnimatorOptions;
    fill(fillMode: FillMode): SimpleAnimatorOptions;
    direction(direction: PlayMode): SimpleAnimatorOptions;
    iterations(iterations: number): SimpleAnimatorOptions;
}

declare enum PlayMode {
    Normal,
    Reverse,
    Alternate,
    AlternateReverse
}

declare interface ExpectedFrameRateRange {
    min: number;
    max: number;
    expected: number;
}