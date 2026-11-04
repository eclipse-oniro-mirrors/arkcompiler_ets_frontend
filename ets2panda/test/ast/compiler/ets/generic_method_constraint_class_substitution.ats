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
 * @file: A method type parameter constrained by a class type parameter must have its
 * constraint (and default) substituted when the enclosing generic class is instantiated,
 * regardless of the method signature shape.
 */

class Chain<T extends A> {
    public value: T

    constructor(value: T) {
        this.value = value
    }

    // Callback parameter and wrapped return type: constraint of S is not a direct
    // substitution target of the old narrow special case.
    then<S extends T>(cb: (x: T) => S): Chain<S> {
        return new Chain<S>(cb(this.value))
    }

    // Multiple parameters of the type parameter's type.
    swap<S extends T>(v: S, w: S): S {
        return v
    }

    // Composite constraint mentioning the class type parameter.
    pick<S extends T[]>(v: S): S {
        return v
    }

    // Default type mentioning the class type parameter with a non-trivial signature.
    valueOrSelf<U = T>(v: number): U {
        return v as U
    }

    // Interdependent method type parameters: Y's constraint references X.
    pair<X extends T, Y extends X>(x: X, y: Y): void {}

    // Method type parameter used in a rest parameter.
    collect<S extends T>(...rest: S[]): void {}
}

// Overload candidate collection must not drop the default-valued generic overload
// when a same-arity non-generic overload precedes it.
class Overloaded<T extends A> {
    pick(a: string): string { return "s" }
    pick<T1 = T>(a: T1): string { return "t1" }
    pick<T1, T2>(a: T1, b: T2): string { return "t2" }
}

class A {
    x: number = 0
}
class B extends A {
    y: number = 0
}
class Z {
    w: number = 0
}

function main(): void {
    let p: Chain<A> = new Chain<A>(new A())

    // Explicit type argument checks against the substituted constraint A.
    p.then<B>((a: A): B => new B())
    p.swap<B>(new B(), new B())
    p.pick<A[]>([new A()])
    let v: A = p.valueOrSelf<A>(1)

    // Inferred type argument from the callback.
    let r: Chain<B> = p.then((a: A): B => new B())

    // Interdependent constraints resolve through the enclosing instantiation.
    p.pair<B, B>(new B(), new B())

    // Rest parameter type follows the type argument.
    p.collect<B>(new B(), new B())

    // Middle overload stays a resolution candidate after instantiation.
    let o: Overloaded<A> = new Overloaded<A>()
    let r1: string = o.pick(new A())
    let r2: string = o.pick<B>(new B())
    let r3: string = o.pick(10)
    let r4: string = o.pick(10, "abc")

    // Invalid type argument must still be rejected, with the substituted constraint in the message.
    p.then<Z>((a: A): Z => new Z())
    p.swap<Z>(new Z(), new Z())
}

/* @@? 101:5 Error Semantic error ESE0127: No matching call signature for then(((p1: A) => Z)) */
/* @@? 101:5 Error Semantic error ESE0228: Type argument 'Z' should be a subtype of 'A' constraint */
/* @@? 102:5 Error Semantic error ESE0127: No matching call signature for swap(Z, Z) */
/* @@? 102:5 Error Semantic error ESE0228: Type argument 'Z' should be a subtype of 'A' constraint */
