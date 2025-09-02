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
import {
  NameAndEmail,
  WithoutEmail,
  PrimaryColors,
  PrimitiveValues,
  CleanData,
  AddParams,
  PerParams,
  UserType,
  PointInstance,
  identity,
  thisParameterType,
  WithoutThis,
  ClassThisType,
  ShoutGreeting,
  QuietGreeting,
  CapitalizedWord,
  UncapitalizedWord,
  getCapitalize
} from './ignore_files/unique_types2';

//tools
const res :ShoutGreeting = 'HELLO';  //error

const withoutThis: WithoutThis = {name: 'aaa'};  //error
class AA{
    private email:WithoutEmail={name:'1',age:10};  //error
    set(color:PrimaryColors){  //error
    }
    get():PrimitiveValues|undefined{  //error
      this.email as thisParameterType  //error
      const cc:ClassThisType = {}  //error
      return ;
    }
}
function test(user:UserType,point:PointInstance,per:PerParams){}  //error*3
@Entry
@Component
struct MyComponent {
  // 1. Pick/Omit 示例
  @State user: NameAndEmail = { name: 'Alice', email: 'alice@example.com' };  //error
  @State partialUser: WithoutEmail = { name: 'Bob', age: 30 };  //error

  // 2. Exclude/Extract 示例
  @State primaryColor: PrimaryColors = 'red';  //error 
  @State primitiveValue: PrimitiveValues = true;  //error

  // 3. NonNullable 示例
  @State validData: CleanData = 'valid';   //error

  // 4. Parameters/ConstructorParameters 示例
  private handleParams = (params: AddParams) => {  //error
  }

  // 5. ReturnType/InstanceType 示例
  @State userData: UserType = { id: 1, name: 'Charlie' };  //error
  private point: PointInstance = { x: 10, y: 20 };  //error

  // 6. NoInfer 示例
  private useIdentity = () => {
    const numResult = identity(123, 456);   //error
    // const errorResult = identity(123, 'abc');   //error
    console.log('Identity result:', numResult);
  }

  // 7. ThisParameterType/OmitThisParameter 示例
  private thisContext: thisParameterType = { name: 'Context' };  //error
  private noThisFunc: WithoutThis = (x) => {  //error
    console.log('Without this:', x);
  }

  // 8. 字符串工具类型示例
  @State shout: ShoutGreeting = 'HELLO';  //error
  @State quiet: QuietGreeting = 'hello';  //error
  @State capitalized: CapitalizedWord = 'Hello';  //error
  @State uncapitalized: UncapitalizedWord = 'hello';  //error

  // 9. getCapitalize 函数
  computedWord= getCapitalize();  //error

  build() {
    Column() {
      // 显示用户信息
      Text(`Name: ${this.user.name}`)
      Text(`Email: ${this.user.email}`)

      // 显示颜色和值
      Text(`Primary Color: ${this.primaryColor}`)
      Text(`Primitive Value: ${this.primitiveValue}`)

      // 显示字符串转换结果
      Text(`Shout: ${this.shout}`)
      Text(`Quiet: ${this.quiet}`)
      Text(`Capitalized: ${this.capitalized}`)
      Text(`Computed: ${this.computedWord}`)

      // 按钮触发函数
      Button('Call Identity')
        .onClick(this.useIdentity)

      Button('Submit Parameters')
        .onClick(() => this.handleParams([123, 'test']))
    }
    .width('100%')
  }
}