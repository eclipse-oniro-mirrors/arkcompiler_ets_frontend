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
import convertxml from './@ohos.convertxml';
let xml1 = '';
let conv = new convertxml.ConvertXML(); //error
let options: convertxml.ConvertOptions = { //error
  trim: false, //error
  declarationKey: '', //error
  instructionKey: '', //error
  attributesKey: '', //error
  textKey: '', //error
  cdataKey: '', //error
  doctypeKey: '', //error
  commentKey: '', //error
  parentKey: '', //error
  typeKey: '', //error
  nameKey: '', //error
  elementsKey: '' //error
}
let result = JSON.stringify(conv.fastConvertToJSObject(xml1, options)); //error
let options2: convertxml.ConvertOptions = { //error
  trim: false, //error
  ignoreDeclaration: false, //error
  ignoreInstruction: false, //error
  ignoreAttributes: false, //error
  ignoreComment: false, //error
  ignoreCDATA: false, //error
  ignoreDoctype: false, //error
  ignoreText: false, //error
  declarationKey: '', //error
  instructionKey: '', //error
  attributesKey: '', //error
  textKey: '', //error
  cdataKey: '', //error
  doctypeKey: '', //error
  commentKey: '', //error
  parentKey: '', //error
  typeKey: '', //error
  nameKey: '', //error
  elementsKey: '' //error
}
let opt:convertxml.ConvertXML; //error

class Demo extends  convertxml.ConvertXML implements  convertxml.ConvertOptions{ //error*2
  parentKey: string; //error
  typeKey: string; //error
  nameKey: string; //error
  elementsKey: string; //error
  convert(xml: string, options?: convertxml.ConvertOptions | undefined): Object { //error*2
  }
}
//sum :44