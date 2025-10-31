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

class A {
    private a_priv = 0;
    protected a_prot = 0;
    a_pub = 0;
    public a_public = 0;

    foo_a() { return this.a_priv + this.a_prot + this.a_pub + this.a_public +
                     this.b_priv + this.b_prot + this.b_pub + this.b_public +
                     this.c_priv + this.c_prot + this.c_pub + this.c_public ; }

    foo_a(a: A) { return a.a_priv + a.a_prot + a.a_pub + a.a_public +
                         a.b_priv + a.b_prot + a.b_pub + a.b_public +
                         a.c_priv + a.c_prot + a.c_pub + a.c_public ; }

    foo_a(b: B) { return b.a_priv + b.a_prot + b.a_pub + b.a_public +
                         b.b_priv + b.b_prot + b.b_pub + b.b_public +
                         b.c_priv + b.c_prot + b.c_pub + b.c_public ; }

    foo_a(c: C) { return c.a_priv + c.a_prot + c.a_pub + c.a_public +
                         c.b_priv + c.b_prot + c.b_pub + c.b_public +
                         c.c_priv + c.c_prot + c.c_pub + c.c_public ; }
}

class B extends A {
    private b_priv = 0;
    protected b_prot = 0;
    b_pub = 0;
    public b_public = 0;

    foo_b() { return this.a_priv + this.a_prot + this.a_pub + this.a_public +
                     this.b_priv + this.b_prot + this.b_pub + this.b_public +
                     this.c_priv + this.c_prot + this.c_pub + this.c_public ; }

    foo_b(a: A) { return a.a_priv + a.a_prot + a.a_pub + a.a_public +
                         a.b_priv + a.b_prot + a.b_pub + a.b_public +
                         a.c_priv + a.c_prot + a.c_pub + a.c_public ; }

    foo_b(b: B) { return b.a_priv + b.a_prot + b.a_pub + b.a_public +
                         b.b_priv + b.b_prot + b.b_pub + b.b_public +
                         b.c_priv + b.c_prot + b.c_pub + b.c_public ; }

    foo_b(c: C) { return c.a_priv + c.a_prot + c.a_pub + c.a_public +
                         c.b_priv + c.b_prot + c.b_pub + c.b_public +
                         c.c_priv + c.c_prot + c.c_pub + c.c_public ; }
}

class C extends B {
    private c_priv = 0;
    protected c_prot = 0;
    c_pub = 0;
    public c_public = 0;

    foo_c() { return this.a_priv + this.a_prot + this.a_pub + this.a_public +
                     this.b_priv + this.b_prot + this.b_pub + this.b_public +
                     this.c_priv + this.c_prot + this.c_pub + this.c_public ; }

    foo_c(a: A) { return a.a_priv + a.a_prot + a.a_pub + a.a_public +
                         a.b_priv + a.b_prot + a.b_pub + a.b_public +
                         a.c_priv + a.c_prot + a.c_pub + a.c_public ; }

    foo_c(b: B) { return b.a_priv + b.a_prot + b.a_pub + b.a_public +
                         b.b_priv + b.b_prot + b.b_pub + b.b_public +
                         b.c_priv + b.c_prot + b.c_pub + b.c_public ; }

    foo_c(c: C) { return c.a_priv + c.a_prot + c.a_pub + c.a_public +
                         c.b_priv + c.b_prot + c.b_pub + c.b_public +
                         c.c_priv + c.c_prot + c.c_pub + c.c_public ; }
}

/* errors in foo_a() */
/* @@? 23:27 Error Semantic error ESE0087: Property 'b_priv' does not exist on type 'A' */
/* @@? 23:41 Error Semantic error ESE0087: Property 'b_prot' does not exist on type 'A' */
/* @@? 23:55 Error Semantic error ESE0087: Property 'b_pub' does not exist on type 'A' */
/* @@? 23:68 Error Semantic error ESE0087: Property 'b_public' does not exist on type 'A' */
/* @@? 24:27 Error Semantic error ESE0087: Property 'c_priv' does not exist on type 'A' */
/* @@? 24:41 Error Semantic error ESE0087: Property 'c_prot' does not exist on type 'A' */
/* @@? 24:55 Error Semantic error ESE0087: Property 'c_pub' does not exist on type 'A' */
/* @@? 24:68 Error Semantic error ESE0087: Property 'c_public' does not exist on type 'A' */

/* errors in foo_a(a: A) */
/* @@? 27:28 Error Semantic error ESE0087: Property 'b_priv' does not exist on type 'A' */
/* @@? 27:39 Error Semantic error ESE0087: Property 'b_prot' does not exist on type 'A' */
/* @@? 27:50 Error Semantic error ESE0087: Property 'b_pub' does not exist on type 'A' */
/* @@? 27:60 Error Semantic error ESE0087: Property 'b_public' does not exist on type 'A' */
/* @@? 28:28 Error Semantic error ESE0087: Property 'c_priv' does not exist on type 'A' */
/* @@? 28:39 Error Semantic error ESE0087: Property 'c_prot' does not exist on type 'A' */
/* @@? 28:50 Error Semantic error ESE0087: Property 'c_pub' does not exist on type 'A' */
/* @@? 28:60 Error Semantic error ESE0087: Property 'c_public' does not exist on type 'A' */

/* errors in foo_a(b: B) */
/* @@? 31:28 Error Semantic error ESE0293: Property b_priv is not visible here. */
/* @@? 31:39 Error Semantic error ESE0293: Property b_prot is not visible here. */
/* @@? 32:28 Error Semantic error ESE0087: Property 'c_priv' does not exist on type 'B' */
/* @@? 32:39 Error Semantic error ESE0087: Property 'c_prot' does not exist on type 'B' */
/* @@? 32:50 Error Semantic error ESE0087: Property 'c_pub' does not exist on type 'B' */
/* @@? 32:60 Error Semantic error ESE0087: Property 'c_public' does not exist on type 'B' */

/* errors in foo_a(c: C) */
/* @@? 35:28 Error Semantic error ESE0293: Property b_priv is not visible here. */
/* @@? 35:39 Error Semantic error ESE0293: Property b_prot is not visible here. */
/* @@? 36:28 Error Semantic error ESE0293: Property c_priv is not visible here. */
/* @@? 36:39 Error Semantic error ESE0293: Property c_prot is not visible here. */

/* errors in foo_b() */
/* @@? 45:27 Error Semantic error ESE0293: Property a_priv is not visible here. */
/* @@? 47:27 Error Semantic error ESE0087: Property 'c_priv' does not exist on type 'B' */
/* @@? 47:41 Error Semantic error ESE0087: Property 'c_prot' does not exist on type 'B' */
/* @@? 47:55 Error Semantic error ESE0087: Property 'c_pub' does not exist on type 'B' */
/* @@? 47:68 Error Semantic error ESE0087: Property 'c_public' does not exist on type 'B' */

/* errors in foo_b(a: A) */
/* @@? 49:28 Error Semantic error ESE0293: Property a_priv is not visible here. */
/* @@? 50:28 Error Semantic error ESE0087: Property 'b_priv' does not exist on type 'A' */
/* @@? 50:39 Error Semantic error ESE0087: Property 'b_prot' does not exist on type 'A' */
/* @@? 50:50 Error Semantic error ESE0087: Property 'b_pub' does not exist on type 'A' */
/* @@? 50:60 Error Semantic error ESE0087: Property 'b_public' does not exist on type 'A' */
/* @@? 51:28 Error Semantic error ESE0087: Property 'c_priv' does not exist on type 'A' */
/* @@? 51:39 Error Semantic error ESE0087: Property 'c_prot' does not exist on type 'A' */
/* @@? 51:50 Error Semantic error ESE0087: Property 'c_pub' does not exist on type 'A' */
/* @@? 51:60 Error Semantic error ESE0087: Property 'c_public' does not exist on type 'A' */

/* errors in foo_b(b: B) */
/* @@? 53:28 Error Semantic error ESE0293: Property a_priv is not visible here. */
/* @@? 55:28 Error Semantic error ESE0087: Property 'c_priv' does not exist on type 'B' */
/* @@? 55:39 Error Semantic error ESE0087: Property 'c_prot' does not exist on type 'B' */
/* @@? 55:50 Error Semantic error ESE0087: Property 'c_pub' does not exist on type 'B' */
/* @@? 55:60 Error Semantic error ESE0087: Property 'c_public' does not exist on type 'B' */

/* errors in foo_b(c: C) */
/* @@? 57:28 Error Semantic error ESE0293: Property a_priv is not visible here. */
/* @@? 59:28 Error Semantic error ESE0293: Property c_priv is not visible here. */
/* @@? 59:39 Error Semantic error ESE0293: Property c_prot is not visible here. */

/* errors in foo_c() */
/* @@? 68:27 Error Semantic error ESE0293: Property a_priv is not visible here. */
/* @@? 69:27 Error Semantic error ESE0293: Property b_priv is not visible here. */

/* errors in foo_c(a: A) */
/* @@? 72:28 Error Semantic error ESE0293: Property a_priv is not visible here. */
/* @@? 73:28 Error Semantic error ESE0087: Property 'b_priv' does not exist on type 'A' */
/* @@? 73:39 Error Semantic error ESE0087: Property 'b_prot' does not exist on type 'A' */
/* @@? 73:50 Error Semantic error ESE0087: Property 'b_pub' does not exist on type 'A' */
/* @@? 73:60 Error Semantic error ESE0087: Property 'b_public' does not exist on type 'A' */
/* @@? 74:28 Error Semantic error ESE0087: Property 'c_priv' does not exist on type 'A' */
/* @@? 74:39 Error Semantic error ESE0087: Property 'c_prot' does not exist on type 'A' */
/* @@? 74:50 Error Semantic error ESE0087: Property 'c_pub' does not exist on type 'A' */
/* @@? 74:60 Error Semantic error ESE0087: Property 'c_public' does not exist on type 'A' */

/* errors in foo_c(b: B) */
/* @@? 76:28 Error Semantic error ESE0293: Property a_priv is not visible here. */
/* @@? 77:28 Error Semantic error ESE0293: Property b_priv is not visible here. */
/* @@? 78:28 Error Semantic error ESE0087: Property 'c_priv' does not exist on type 'B' */
/* @@? 78:39 Error Semantic error ESE0087: Property 'c_prot' does not exist on type 'B' */
/* @@? 78:50 Error Semantic error ESE0087: Property 'c_pub' does not exist on type 'B' */
/* @@? 78:60 Error Semantic error ESE0087: Property 'c_public' does not exist on type 'B' */

/* errors in foo_c(c: C) */
/* @@? 80:28 Error Semantic error ESE0293: Property a_priv is not visible here. */
/* @@? 81:28 Error Semantic error ESE0293: Property b_priv is not visible here. */
