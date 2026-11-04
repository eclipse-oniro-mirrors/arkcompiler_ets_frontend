/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class BiConsumer<T, U> {
    accept(t: T, u: U) {
        t;
        u;
    }
}

class SomeObject<T> {}
class SomeType {}
class OtherType {}

class Base { 
    foo_default<T=SomeType, C=T, B=C, A=B, U=A>(): BiConsumer<T, U> { 
        return new BiConsumer<T, U>() 
    }
    foo_implicit<T=SomeType, F=SomeType, U=F>(x: F): BiConsumer<T, U> { 
        return new BiConsumer<T, U>() 
    }
    foo_array<T=SomeType, U=T[]>(): BiConsumer<T, U> { 
        return new BiConsumer<T, U>() 
    }
    foo_array_implicit<X, T=SomeType, U=X[]>(x : X): BiConsumer<T, U> { 
        return new BiConsumer<T, U>() 
    }
    foo_union<T=SomeType, F = OtherType, U= T | F>(): BiConsumer<T, U> { 
        return new BiConsumer<T, U>() 
    }
    foo_object<T=SomeType, U=SomeObject<T>>(): BiConsumer<T, U> { 
        return new BiConsumer<T, U>() 
    }
}

function main(): int {
    new Base().foo_default().accept(new SomeType(), new SomeType())
    new Base().foo_implicit(new OtherType()).accept(new SomeType(), new OtherType())
    new Base().foo_array().accept(new SomeType(), [new SomeType()])
    new Base().foo_array_implicit(new OtherType()).accept(new SomeType(), [new OtherType()])
    new Base().foo_union().accept(new SomeType(), new SomeType())
    new Base().foo_union().accept(new SomeType(), new OtherType())
    new Base().foo_object().accept(new SomeType(), new SomeObject<SomeType>)
    return 0
}
