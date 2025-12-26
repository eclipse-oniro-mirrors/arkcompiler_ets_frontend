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

@Target([AnnotationTargets.INTERFACE_PROPERTY])
@Retention("SOURCE")
@interface property{ }

@Target([AnnotationTargets.INTERFACE_GETTER])
@Retention("SOURCE")
@interface getter{ }

@Target([AnnotationTargets.INTERFACE_SETTER])
@Retention("SOURCE")
@interface setter{ }

@Target([AnnotationTargets.INTERFACE_SETTER, AnnotationTargets.INTERFACE_GETTER])
@Retention("SOURCE")
@interface gettersetter{ }

interface InterfaceProperty {
    @property
    interfaceProperty: number;

    @property
    get property1(): number;

    @property
    set property1(value: number);

    @property
    get interfaceGetter(): number;

    @property
    set interfaceSetter(value: number);
}

interface InterfaceGetter {
    // INTERFACE_PROPERTY
    @getter
    interfaceProperty: number;
    
    // INTERFACE_GETTER
    @getter
    get interfaceGetter(): number;
    
    // INTERFACE_SETTER
    @getter
    set interfaceSetter(value: number);
}

interface InterfaceSetter {
    // INTERFACE_PROPERTY
    @setter
    interfaceProperty: number;
    
    // INTERFACE_GETTER
    @setter
    get interfaceGetter(): number;
    
    // INTERFACE_SETTER
    @setter
    set interfaceSetter(value: number);
}

interface InterfaceGetterSetter {
    // INTERFACE_PROPERTY
    @gettersetter
    interfaceProperty: number;
    
    // INTERFACE_GETTER
    @gettersetter
    get interfaceGetter(): number;
    
    // INTERFACE_SETTER
    @gettersetter
    set interfaceSetter(value: number);
}

interface InterfaceMix {
    // INTERFACE_PROPERTY
    @gettersetter
    interfaceProperty: number;

    @getter
    @setter
    interfaceProperty1: number;
}
/* @@? 36:6 Error Semantic error ESE495032: Annotation 'property' cannot be used on INTERFACE_GETTER. It is only allowed on: INTERFACE_PROPERTY. */
/* @@? 39:6 Error Semantic error ESE495032: Annotation 'property' cannot be used on INTERFACE_SETTER. It is only allowed on: INTERFACE_PROPERTY. */
/* @@? 42:6 Error Semantic error ESE495032: Annotation 'property' cannot be used on INTERFACE_GETTER. It is only allowed on: INTERFACE_PROPERTY. */
/* @@? 45:6 Error Semantic error ESE495032: Annotation 'property' cannot be used on INTERFACE_SETTER. It is only allowed on: INTERFACE_PROPERTY. */
/* @@? 51:6 Error Semantic error ESE495032: Annotation 'getter' cannot be used on INTERFACE_PROPERTY. It is only allowed on: INTERFACE_GETTER. */
/* @@? 59:6 Error Semantic error ESE495032: Annotation 'getter' cannot be used on INTERFACE_SETTER. It is only allowed on: INTERFACE_GETTER. */
/* @@? 65:6 Error Semantic error ESE495032: Annotation 'setter' cannot be used on INTERFACE_PROPERTY. It is only allowed on: INTERFACE_SETTER. */
/* @@? 69:6 Error Semantic error ESE495032: Annotation 'setter' cannot be used on INTERFACE_GETTER. It is only allowed on: INTERFACE_SETTER. */
/* @@? 79:6 Error Semantic error ESE495032: Annotation 'gettersetter' cannot be used on INTERFACE_PROPERTY. It is only allowed on: INTERFACE_SETTER, INTERFACE_GETTER. */
/* @@? 93:6 Error Semantic error ESE495032: Annotation 'gettersetter' cannot be used on INTERFACE_PROPERTY. It is only allowed on: INTERFACE_SETTER, INTERFACE_GETTER. */
/* @@? 96:6 Error Semantic error ESE495032: Annotation 'getter' cannot be used on INTERFACE_PROPERTY. It is only allowed on: INTERFACE_GETTER. */
/* @@? 97:6 Error Semantic error ESE495032: Annotation 'setter' cannot be used on INTERFACE_PROPERTY. It is only allowed on: INTERFACE_SETTER. */
