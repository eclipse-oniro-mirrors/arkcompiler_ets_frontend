/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "lsp/include/class_hierarchy_info.h"
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include <gtest/gtest.h>

using ark::es2panda::lsp::Initializer;

namespace {

class LspGetClassHierarchyInfoTests : public LSPAPITests {};

TEST_F(LspGetClassHierarchyInfoTests, GetClassHierarchyInfo_1)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class Parent {
private privateMethod(): void {
  console.log("Parent method");
}
  public publicMethod(): void {
  console.log("Parent method");
}
  protected action(fileName: string, position: number): number {
  return position;
}
  static staticMethod(): void {
  console.log("Parent static method");
  }
}
  class Child extends Parent {
  public display(): void {
    console.log("need display");
  }
})";

    auto pos = text.find("Child");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("class_hierarchy_info_1.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto classHierarchy = lspApi->getClassHierarchyInfo(context, pos);

    ASSERT_EQ(classHierarchy.size(), 1);
    ASSERT_EQ(classHierarchy[0].GetName(), "Parent");
    auto methods = classHierarchy[0].GetMethodItemList();
    auto it = methods.find("publicMethod(): void");
    ASSERT_TRUE(it != methods.end());
    ASSERT_TRUE(it->second != nullptr);
    ASSERT_EQ(it->second->GetSetterStyle(), ark::es2panda::lsp::SetterStyle::NONE);
    ASSERT_EQ(it->second->GetAccessModifierStyle(), ark::es2panda::lsp::AccessModifierStyle::PUBLIC);
    it = methods.find("action(fileName: string, position: number): number");
    ASSERT_TRUE(it != methods.end());
    ASSERT_TRUE(it->second != nullptr);
    ASSERT_EQ(it->second->GetSetterStyle(), ark::es2panda::lsp::SetterStyle::NONE);
    ASSERT_EQ(it->second->GetAccessModifierStyle(), ark::es2panda::lsp::AccessModifierStyle::PROTECTED);
    initializer.DestroyContext(context);
}

TEST_F(LspGetClassHierarchyInfoTests, GetClassHierarchyInfo_2)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class Animal {
private body_: string = '';

  protected action(): void {
    console.log("need Animal action");
  }
  protected sleep(): void {
    console.log("need Animal sleep");
  }
}

class Bird extends Animal {
  action(): void {
    console.log("need action");
  }

  Drink(): void {
    console.log("need Drink");
  }
}

class Magpie extends Bird {
  public action(): void {}
  Drink(): void {
    console.log("need Drink");
  }
})";

    auto pos = text.find("Magpie");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("class_hierarchy_info_2.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto classHierarchy = lspApi->getClassHierarchyInfo(context, pos);
    ASSERT_EQ(classHierarchy.size(), 1);
    ASSERT_EQ(classHierarchy[0].GetName(), "Animal");

    auto methods = classHierarchy[0].GetMethodItemList();
    auto it = methods.find("sleep(): void");
    ASSERT_TRUE(it != methods.end());
    ASSERT_TRUE(it->second != nullptr);
    ASSERT_EQ(it->second->GetSetterStyle(), ark::es2panda::lsp::SetterStyle::NONE);
    ASSERT_EQ(it->second->GetAccessModifierStyle(), ark::es2panda::lsp::AccessModifierStyle::PROTECTED);
    initializer.DestroyContext(context);
}

TEST_F(LspGetClassHierarchyInfoTests, GetClassHierarchyInfo_3)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class Animal {
  private body_: string = '';

  protected action(): void {
    console.log("need action");
  }
  protected sleep(): void {
    console.log("need sleep");
  }
})";

    auto pos = text.find("Animal");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("class_hierarchy_info_3.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto classHierarchy = lspApi->getClassHierarchyInfo(context, pos);
    ASSERT_TRUE(classHierarchy.empty());
    initializer.DestroyContext(context);
}

TEST_F(LspGetClassHierarchyInfoTests, GetClassHierarchyInfo_4)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class ii {
  private body_: string = '';

  action(): void {
    console.log("need sleep");
  }

  set Body(value: string) {
    this.body_ = value;
  }
  get Body(): string {
    return this.body_;
  }
}

  class jj extends ii {
  private age_: number = 18;
  public action(): void {
    console.log("need sleep and fly");
  }
})";

    auto pos = text.find("jj");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("class_hierarchy_info_4.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto classHierarchy = lspApi->getClassHierarchyInfo(context, pos);
    ASSERT_EQ(classHierarchy.size(), 1);
    ASSERT_EQ(classHierarchy[0].GetName(), "ii");

    auto methods = classHierarchy[0].GetMethodItemList();
    auto it = methods.find("Body(): string");
    ASSERT_TRUE(it != methods.end());
    ASSERT_TRUE(it->second != nullptr);
    ASSERT_EQ(it->second->GetSetterStyle(), ark::es2panda::lsp::SetterStyle::GETTER);
    ASSERT_EQ(it->second->GetAccessModifierStyle(), ark::es2panda::lsp::AccessModifierStyle::PUBLIC);
    it = methods.find("Body(value: string)");
    ASSERT_TRUE(it != methods.end());
    ASSERT_TRUE(it->second != nullptr);
    ASSERT_EQ(it->second->GetSetterStyle(), ark::es2panda::lsp::SetterStyle::SETTER);
    ASSERT_EQ(it->second->GetAccessModifierStyle(), ark::es2panda::lsp::AccessModifierStyle::PUBLIC);
    initializer.DestroyContext(context);
}

TEST_F(LspGetClassHierarchyInfoTests, GetClassHierarchyInfo_5)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class C<T, M> {
  func1(): void {}
  func2(): string {
    return "1";
  }
  func3(): number {
    return 1;
  }
  func4(): boolean {
    return false;
  }
  func5(): Array<number> {
    return [1, 2];
  }
}

class B<T, M> extends C<T, M> {
  method1(): void {}
  method2(parameter1: string, callBack: () => void): void {}
}

class A extends B<string, number> {/*1*/};)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("class_hierarchy_info_5.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto classHierarchy = lspApi->getClassHierarchyInfo(context, pos);
    size_t expectInfoListSize = 2;
    ASSERT_EQ(classHierarchy.size(), expectInfoListSize);
    ASSERT_EQ(classHierarchy[0].GetName(), "B");
    ASSERT_EQ(classHierarchy[1].GetName(), "C");
    auto classBItems = classHierarchy[0].GetMethodItemList();
    ASSERT_TRUE(classBItems.find("method1(): void") != classBItems.end());
    ASSERT_TRUE(classBItems.find("method2(parameter1: string, callBack: (() => void)): void") != classBItems.end());
    auto classCItems = classHierarchy[1].GetMethodItemList();
    ASSERT_TRUE(classCItems.find("func1(): void") != classCItems.end());
    ASSERT_TRUE(classCItems.find("func2(): string") != classCItems.end());
    ASSERT_TRUE(classCItems.find("func3(): number") != classCItems.end());
    ASSERT_TRUE(classCItems.find("func4(): boolean") != classCItems.end());
    ASSERT_TRUE(classCItems.find("func5(): Array<number>") != classCItems.end());
    initializer.DestroyContext(context);
}

TEST_F(LspGetClassHierarchyInfoTests, GetClassHierarchyInfo_6)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(type parameter = number;

class B {
  public method1(parameter1: number): parameter {
    return 1;
  }
  method2(parameter1: number): number {
    return parameter1 as number;
  }
  async method3(parameter1: string): Promise<string> {
    return '1';
  }
}

class A extends B {/*1*/};)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("class_hierarchy_info_6.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto classHierarchy = lspApi->getClassHierarchyInfo(context, pos);
    ASSERT_FALSE(classHierarchy.empty());
    ASSERT_EQ(classHierarchy[0].GetName(), "B");
    auto classBItems = classHierarchy[0].GetMethodItemList();
    ASSERT_TRUE(classBItems.find("method1(parameter1: number): parameter") != classBItems.end());
    ASSERT_TRUE(classBItems.find("method2(parameter1: number): number") != classBItems.end());
    ASSERT_TRUE(classBItems.find("method3(parameter1: string): Promise<string>") != classBItems.end());
    initializer.DestroyContext(context);
}

TEST_F(LspGetClassHierarchyInfoTests, GetClassHierarchyInfo_7)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class C <T, M> {
  func1(): void {}

  func2(): boolean {
    return false;
  }

  func3(): Array<number> {
    return [1, 2];
  }
};

class B<T, M> extends C<T, M> {
  method1() {}
  func2(): boolean {
    return false;
  }
}

class A extends B<string, number> {
  method1() {}
  func3(): Array<number> {
    return [1, 2];
  }/*1*/
})";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("class_hierarchy_info_7.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto classHierarchy = lspApi->getClassHierarchyInfo(context, pos);
    size_t expectInfoListSize = 2;
    ASSERT_EQ(classHierarchy.size(), expectInfoListSize);
    ASSERT_EQ(classHierarchy[0].GetName(), "B");
    ASSERT_EQ(classHierarchy[1].GetName(), "C");
    auto classBItems = classHierarchy[0].GetMethodItemList();
    ASSERT_EQ(classBItems.size(), 1);
    ASSERT_TRUE(classBItems.find("func2(): boolean") != classBItems.end());
    auto classCItems = classHierarchy[1].GetMethodItemList();
    ASSERT_EQ(classCItems.size(), 1);
    ASSERT_TRUE(classCItems.find("func1(): void") != classCItems.end());
    initializer.DestroyContext(context);
}

TEST_F(LspGetClassHierarchyInfoTests, GetClassHierarchyInfo_8)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class Parent {
  public property1: number = 1;
  protected property2: string = '1';
  private property3: boolean = true;
  readonly property4: number = 1;
  static property5: number = 1;
}

class Son extends Parent {
  property1: number = 2;
  ChildExtraProperty1: number = 2;
  ChildExtraProperty2: number = 2;
}

class GrandSon extends Son {/*1*/
  public property2: string = '2';
  ChildExtraProperty1: number = 3;
})";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("class_hierarchy_info_8.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto classHierarchy = lspApi->getClassHierarchyInfo(context, pos);
    size_t expectInfoListSize = 2;
    ASSERT_EQ(classHierarchy.size(), expectInfoListSize);
    ASSERT_EQ(classHierarchy[0].GetName(), "Son");
    ASSERT_EQ(classHierarchy[1].GetName(), "Parent");
    auto sonItems = classHierarchy[0].GetPropertyItemList();
    size_t expectPropertyListSize = 2;
    ASSERT_EQ(sonItems.size(), expectPropertyListSize);
    ASSERT_TRUE(sonItems.find("property1: number") != sonItems.end());
    ASSERT_TRUE(sonItems.find("ChildExtraProperty2: number") != sonItems.end());
    auto parentItems = classHierarchy[1].GetPropertyItemList();
    ASSERT_EQ(parentItems.size(), 1);
    ASSERT_TRUE(parentItems.find("property4: number") != parentItems.end());
    initializer.DestroyContext(context);
}

TEST_F(LspGetClassHierarchyInfoTests, GetClassHierarchyInfo_9)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class Parent {
private privateMethod(): void {
  console.log("Parent method");
}
  public publicMethod(): void {
  console.log("Parent method");
}
  protected action(fileName: string, position: number): number {
  return position;
}
  static staticMethod(): void {
  console.log("Parent static method");
  }
}
  //中文测试
  class Child extends Parent {
  public display(): void {
    console.log("need display");
  }
})";

    // NOLINTNEXTLINE(readability-magic-numbers)
    auto pos = 327;
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("class_hierarchy_info_9.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto classHierarchy = lspApi->getClassHierarchyInfo(context, pos);

    ASSERT_EQ(classHierarchy.size(), 1);
    ASSERT_EQ(classHierarchy[0].GetName(), "Parent");
    auto methods = classHierarchy[0].GetMethodItemList();
    auto it = methods.find("publicMethod(): void");
    ASSERT_TRUE(it != methods.end());
    ASSERT_TRUE(it->second != nullptr);
    ASSERT_EQ(it->second->GetSetterStyle(), ark::es2panda::lsp::SetterStyle::NONE);
    ASSERT_EQ(it->second->GetAccessModifierStyle(), ark::es2panda::lsp::AccessModifierStyle::PUBLIC);
    it = methods.find("action(fileName: string, position: number): number");
    ASSERT_TRUE(it != methods.end());
    ASSERT_TRUE(it->second != nullptr);
    ASSERT_EQ(it->second->GetSetterStyle(), ark::es2panda::lsp::SetterStyle::NONE);
    ASSERT_EQ(it->second->GetAccessModifierStyle(), ark::es2panda::lsp::AccessModifierStyle::PROTECTED);
    initializer.DestroyContext(context);
}
}  // namespace
