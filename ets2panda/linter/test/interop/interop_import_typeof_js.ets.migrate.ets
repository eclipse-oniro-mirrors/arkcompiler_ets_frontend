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
 
import myAaa,{ClassA,Dog,Person,Wiki} from "./interop_import_js_js";
import { Dog as Doge } from './interop_import_js_js';
import { Wiki as wiki } from './interop_import_js_js';

myAaa.invoke().typeOf(); //error
let fun = myAaa.invoke();
typeof fun;
Dog.typeOf(); //error
Dog.invoke(ESValue.wrap('doge')).typeOf(); //error
Doge.invoke(ESValue.wrap('doge')).typeOf(); //error
Wiki.typeOf() //error
Wiki.getProperty('name').typeOf() //error
wiki.typeOf() //error
let val = wiki.getProperty("name")
typeof val;
const aClass:ClassA = ClassA.instantiate()
ClassA.instantiate().typeOf() //error
typeof aClass;
let person:Person = Person.instantiate();
let name =person.getProperty("name")
let name2 =person.invokeMethod("getName")
function getPersonInfo(){
  typeof person;
  typeof person.getName();
  typeof name2;
  typeof name;
  typeof person.setAge(111);
  typeof person;
  Person.instantiate().typeOf(); //error
}

const  age = typeof person.setAge(111);
let person2 = typeof person
class Object {
  code: string = "www"
  location: string = typeof ('123')
  getLocation(){
    console.log(`nameType=${ typeof code} `);
    return typeof this.location;
  }
  setLocation(location: string){
    this.location = location;
    typeof location;
  }
  tips(){
    wiki.getProperty('name').typeOf(); //error
    typeof age;
    typeof person2;
    typeof fun;
    console.log(`ClassA=${ ClassA.instantiate().typeOf()} `); //error
  }
}

myAaa.typeOf(); //error
Person.instantiate().getProperty('name').typeOf()  //error
Person.instantiate().invokeMethod("getName").typeOf() //error
Person.instantiate().invokeMethod("setAge", ESValue.wrap(22)).typeOf() //error
