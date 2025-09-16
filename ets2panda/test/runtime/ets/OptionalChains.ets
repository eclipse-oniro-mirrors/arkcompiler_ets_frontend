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

function assert_n(v: Object | null | undefined) { arktest.assertTrue(v === null); }
function assert_u(v: Object | null | undefined) { arktest.assertTrue(v === undefined); }
function assert_o(v: Object | null | undefined) { arktest.assertTrue(v !== null && v !== undefined); }
function assert_npe(f: () => void) {
    try {
        f();
    } catch (e: NullPointerError) {
        return;
    }
    arktest.assertTrue(false,  "npe was not thrown")
}

class Link {
    m(): Link { return this; }
    f: Link = this;
    a: Link[] = [(this)];
    c: () => Link = () => this

    om(): Link | null { return this.m() }
    of: Link | null = this.f;
    oa: Link[] | null = this.a;
    oc: (() => Link) | null = this.c;

    nm(): Link | null { return null }
    nf: Link | null = null;
    na: Link[] | null = null;
    nc: (() => Link) | null = null;

    static noevalFlag = true;
    noeval(): Link { if (Link.noevalFlag) { throw new Error("never evaluated"); } return this; }
}

function test1(l: Link | null, nl: Link | null) {
    assert_o(l?.m());
    assert_o(l?.f);
    assert_o(l?.a[0]);
    assert_o(l?.c());
    assert_o(l?.of!.f);

    assert_u(nl?.m());
    assert_u(nl?.f);
    assert_u(nl?.a[0]);
    assert_u(nl?.c());
    assert_u(nl?.of!.f);
    assert_u(nl?.nf!.f);

    nl?.m().noeval();
    nl?.f.noeval();
    nl?.a[0].noeval();
    nl?.c().noeval();
    nl?.of!.f.noeval();
    assert_npe(() => { nl?.of!.f! });
}

function test2(l: Link | null, nl: Link | null) {
    assert_o(l?.m().f.a[0].c());
    assert_o(l?.f.m().c().a[0]);
    assert_o(l?.a[0].c().f.m());
    assert_o(l?.c().m().a[0].f);
    assert_o(l?.c().m().of!.a[0].oc!().f);

    assert_u(nl?.m().f.a[0].c());
    assert_u(nl?.f.m().c().a[0]);
    assert_u(nl?.a[0].c().f.m());
    assert_u(nl?.c().m().a[0].f);
    assert_u(nl?.c().m().of!.a[0].oc!().f);

    nl?.m().f.a[0].c().noeval();
    nl?.f.m().c().a[0].noeval();
    nl?.a[0].c().f.m().noeval();
    nl?.c().m().a[0].f.noeval();
    nl?.c().m().of!.a[0].oc!().f.noeval();
}

function test3(l: Link | null, nl: Link | null) {
    assert_o(l?.om()?.of?.oa?.[0].oc?.());
    assert_o(l?.of?.om()?.oc?.().oa?.[0]);
    assert_o(l?.oa?.[0]?.oc?.().of?.om());
    assert_o(l?.oc?.().om()?.oa?.[0].of);
    assert_o(l?.oc?.().om()?.of!.oa?.[0].oc!().of);

    assert_u(nl?.om()?.of?.oa?.[0].oc?.());
    assert_u(nl?.of?.om()?.oc?.().oa?.[0]);
    assert_u(nl?.oa?.[0]?.oc?.().of?.om());
    assert_u(nl?.oc?.().om()?.oa?.[0].of);
    assert_u(nl?.oc!().om()?.of!.oa![0].oc!().of);

    nl?.om()?.of?.oa?.[0].oc?.().noeval();
    nl?.of?.om()?.oc?.().oa?.[0].noeval();
    nl?.oa?.[0]?.oc?.().of?.om()?.noeval();
    nl?.oc?.().om()?.oa?.[0].of?.noeval();
    nl?.oc?.().om()?.of!.oa?.[0].oc!().of?.noeval();
}

function test4(l: Link | null, nl: Link | null) {
    assert_npe(() => { nl?.of! });
    nl?.of!.f;
}

function test5(l: Link | null, nl: Link | null) {
    l?.f.a[0]?.f.c();
    nl?.f.a[0]?.f.c().noeval();
    assert_npe(() => { nl?.f.a[0]?.f.c()! });
    assert_npe(() => { (nl?.f?.a)?.[0].f! });
    assert_u(l?.f.a[0].nf?.a[0].noeval()?.m());

    let u: Link | undefined = l?.f.oc?.().na?.[0].noeval().f?.oa?.[0];
}

function main() {
    test1(new Link(), null)
    test2(new Link(), null)
    test3(new Link(), null)
    test4(new Link(), null)
    test5(new Link(), null)
}
