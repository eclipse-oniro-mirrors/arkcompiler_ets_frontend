/**
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
 * @file: Runtime counterpart of generic_method_constraint_class_substitution.ets:
 * a method type parameter constrained by a class type parameter resolves its
 * constraint against the enclosing class instantiation.
 */

class ConstraintBase {
    baseField: number = 1
}

class ConstraintDerived extends ConstraintBase {
    derivedField: number = 2
}

class ConstraintHolder<T extends ConstraintBase> {
    value: T

    constructor(value: T) {
        this.value = value
    }

    then<S extends T>(cb: (x: T) => S): ConstraintHolder<S> {
        return new ConstraintHolder<S>(cb(this.value))
    }

    swap<S extends T>(v: S, w: S): S {
        return v
    }
}

function eraseType(value: Object): Object {
    return value
}

function main(): int {
    let baseHolder = new ConstraintHolder<ConstraintBase>(new ConstraintBase())
    let derivedHolder = new ConstraintHolder<ConstraintDerived>(new ConstraintDerived())

    // Explicit type argument against the substituted constraint.
    let thenExplicit = derivedHolder.then<ConstraintDerived>((x: ConstraintDerived): ConstraintDerived => x)
    let swapped = derivedHolder.swap<ConstraintDerived>(new ConstraintDerived(), new ConstraintDerived())

    // Inferred type argument from the callback.
    let thenInferred = derivedHolder.then((x: ConstraintDerived): ConstraintDerived => x)

    arktest.assertTrue(eraseType(thenExplicit.value) instanceof ConstraintDerived)
    arktest.assertTrue(eraseType(swapped) instanceof ConstraintDerived)
    arktest.assertTrue(eraseType(thenInferred.value) instanceof ConstraintDerived)

    // Constraint is enforced against the substituted type at runtime-relevant boundaries.
    arktest.assertTrue(eraseType(baseHolder.then((x: ConstraintBase): ConstraintDerived => new ConstraintDerived()).value)
        instanceof ConstraintDerived)

    return 0
}
