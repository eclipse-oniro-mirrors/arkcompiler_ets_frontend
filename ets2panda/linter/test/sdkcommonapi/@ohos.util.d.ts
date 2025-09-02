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

declare namespace util {
    class Base64 {
        /**
         * Constructor for creating base64 encoding and decoding
         *
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.Base64Helper.constructor
         */
        constructor();
        /**
         * Encodes all bytes from the specified u8 array into a newly-allocated u8 array using the Base64 encoding scheme.
         *
         * @param { Uint8Array } src - A Uint8Array value
         * @returns { Uint8Array } Return the encoded new Uint8Array.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.Base64Helper.encodeSync
         */
        encodeSync(src: Uint8Array): Uint8Array;
        /**
         * Encodes the specified byte array into a String using the Base64 encoding scheme.
         *
         * @param { Uint8Array } src - A Uint8Array value
         * @returns { string } Return the encoded string.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.Base64Helper.encodeToStringSync
         */
        encodeToStringSync(src: Uint8Array): string;
        /**
         * Decodes a Base64 encoded String or input u8 array into a newly-allocated u8 array using the Base64 encoding scheme.
         *
         * @param { Uint8Array | string } src - A Uint8Array value or value A string value
         * @returns { Uint8Array } Return the decoded Uint8Array.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.Base64Helper.decodeSync
         */
        decodeSync(src: Uint8Array | string): Uint8Array;
        /**
         * Asynchronously encodes all bytes in the specified u8 array into the newly allocated u8 array using the Base64 encoding scheme.
         *
         * @param { Uint8Array } src - A Uint8Array value
         * @returns { Promise<Uint8Array> } Return the encodes asynchronous new Uint8Array.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.Base64Helper.encode
         */
        encode(src: Uint8Array): Promise<Uint8Array>;
        /**
         * Asynchronously encodes the specified byte array into a String using the Base64 encoding scheme.
         *
         * @param { Uint8Array } src - A Uint8Array value
         * @returns { Promise<string> } Returns the encoded asynchronous string.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.Base64Helper.encodeToString
         */
        encodeToString(src: Uint8Array): Promise<string>;
        /**
         * Use the Base64 encoding scheme to asynchronously decode a Base64-encoded string or input u8 array into a newly allocated u8 array.
         *
         * @param { Uint8Array | string } src - A Uint8Array value or value A string value
         * @returns { Promise<Uint8Array> } Return the decoded asynchronous Uint8Array.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.Base64Helper.decode
         */
        decode(src: Uint8Array | string): Promise<Uint8Array>;
    }
    class LruBuffer<K, V> {
        /**
         * Default constructor used to create a new LruBuffer instance with the default capacity of 64.
         *
         * @param { number } capacity - Indicates the capacity to customize for the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.constructor
         */
        constructor(capacity?: number);
        /**
         * Updates the buffer capacity to a specified capacity.
         *
         * @param { number } newCapacity - Indicates the new capacity to set.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.updateCapacity
         */
        updateCapacity(newCapacity: number): void;
        /**
         * Returns a string representation of the object.
         *
         * @returns { string } Returns the string representation of the object and outputs the string representation of the object.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.toString
         */
        toString(): string;
        /**
         * Obtains a list of all values in the current buffer.
         *
         * @type { number }
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.length
         */
        length: number;
        /**
         * Obtains the capacity of the current buffer.
         *
         * @returns { number } Returns the capacity of the current buffer.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.getCapacity
         */
        getCapacity(): number;
        /**
         * Clears key-value pairs from the current buffer.
         *
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.clear
         */
        clear(): void;
        /**
         * Obtains the number of times createDefault(Object) returned a value.
         *
         * @returns { number } Returns the number of times createDefault(java.lang.Object) returned a value.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.getCreateCount
         */
        getCreateCount(): number;
        /**
         * Obtains the number of times that the queried values are not matched.
         *
         * @returns { number } Returns the number of times that the queried values are not matched.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.getMissCount
         */
        getMissCount(): number;
        /**
         * Obtains the number of times that values are evicted from the buffer.
         *
         * @returns { number } Returns the number of times that values are evicted from the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.getRemovalCount
         */
        getRemovalCount(): number;
        /**
         * Obtains the number of times that the queried values are successfully matched.
         *
         * @returns { number } Returns the number of times that the queried values are successfully matched.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.getMatchCount
         */
        getMatchCount(): number;
        /**
         * Obtains the number of times that values are added to the buffer.
         *
         * @returns { number } Returns the number of times that values are added to the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.getPutCount
         */
        getPutCount(): number;
        /**
         * Checks whether the current buffer is empty.
         *
         * @returns { boolean } Returns true if the current buffer contains no value.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.isEmpty
         */
        isEmpty(): boolean;
        /**
         * Obtains the value associated with a specified key.
         *
         * @param { K } key - Indicates the key to query.
         * @returns { V | undefined } Returns the value associated with the key if the specified key is present in the buffer; returns null otherwise.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.get
         */
        get(key: K): V | undefined;
        /**
         * Adds a key-value pair to the buffer.
         *
         * @param { K } key - Indicates the key to add.
         * @param { V } value - Indicates the value associated with the key to add.
         * @returns { V } Returns the value associated with the added key; returns the original value if the key to add already exists.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.put
         */
        put(key: K, value: V): V;
        /**
         * Obtains a list of all values in the current buffer.
         *
         * @returns { V[] } Returns the list of all values in the current buffer in ascending order, from the most recently accessed to least recently accessed.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.values
         */
        values(): V[];
        /**
         * Obtains a list of keys for the values in the current buffer.
         *
         * @returns { K[] } Returns a list of keys sorted from most recently accessed to least recently accessed.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.keys
         */
        keys(): K[];
        /**
         * Deletes a specified key and its associated value from the current buffer.
         *
         * @param { K } key - Indicates the key to delete.
         * @returns { V | undefined } Returns an Optional object containing the deleted key-value pair; returns an empty Optional object if the key does not exist.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.remove
         */
        remove(key: K): V | undefined;
        /**
         * Executes subsequent operations after a value is deleted.
         *
         * @param { boolean } isEvict - The parameter value is true if this method is called due to insufficient capacity,
         * and the parameter value is false in other cases.
         * @param { K } key - Indicates the deleted key.
         * @param { V } value - Indicates the deleted value.
         * @param { V } newValue - The parameter value is the new value associated if the put(java.lang.Object,java.lang.Object)
         * method is called and the key to add already exists. The parameter value is null in other cases.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.afterRemoval
         */
        afterRemoval(isEvict: boolean, key: K, value: V, newValue: V): void;
        /**
         * Checks whether the current buffer contains a specified key.
         *
         * @param { K } key - Indicates the key to check.
         * @returns { boolean } Returns true if the buffer contains the specified key.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.contains
         */
        contains(key: K): boolean;
        /**
         * Called after a cache miss to compute a value for the corresponding key.
         *
         * @param { K } key - Indicates the missed key.
         * @returns { V } Returns the value associated with the key.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.createDefault
         */
        createDefault(key: K): V;
        /**
         * Returns an array of key-value pairs of enumeratable properties of a given object.
         *
         * @returns { IterableIterator<[K, V]> } Returns an array of key-value pairs for the enumeratable properties of the given object itself.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.entries
         */
        entries(): IterableIterator<[
            K,
            V
        ]>;
        /**
         * Specifies the default iterator for an object.
         * @returns { IterableIterator<[K, V]> } Returns a two - dimensional array in the form of key - value pairs.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.LRUCache.[Symbol.iterator]
         */
        [Symbol.iterator](): IterableIterator<[
            K,
            V
        ]>;
    }
    class LRUCache<K, V> {
        /**
         * Default constructor used to create a new LruBuffer instance with the default capacity of 64.
         *
         * @param { number } [capacity] - Indicates the capacity to customize for the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Default constructor used to create a new LruBuffer instance with the default capacity of 64.
         *
         * @param { number } [capacity] - Indicates the capacity to customize for the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Default constructor used to create a new LruBuffer instance with the default capacity of 64.
         *
         * @param { number } [capacity] - Indicates the capacity to customize for the buffer.
         * @throws { BusinessError } 401 - Parameter error. Possible causes: 1.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        constructor(capacity?: number);
        /**
         * Updates the buffer capacity to a specified capacity.
         *
         * @param { number } newCapacity - Indicates the new capacity to set.
         * @throws { BusinessError } 401 - Parameter error. Possible causes: 1.Mandatory parameters are left unspecified.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Updates the buffer capacity to a specified capacity.
         *
         * @param { number } newCapacity - Indicates the new capacity to set.
         * @throws { BusinessError } 401 - Parameter error. Possible causes: 1.Mandatory parameters are left unspecified.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Updates the buffer capacity to a specified capacity.
         *
         * @param { number } newCapacity - Indicates the new capacity to set.
         * @throws { BusinessError } 401 - Parameter error. Possible causes: 1.Mandatory parameters are left unspecified.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        updateCapacity(newCapacity: number): void;
        /**
         * Returns a string representation of the object.
         *
         * @returns { string } Returns the string representation of the object and outputs the string representation of the object.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Returns a string representation of the object.
         *
         * @returns { string } Returns the string representation of the object and outputs the string representation of the object.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Returns a string representation of the object.
         *
         * @returns { string } Returns the string representation of the object and outputs the string representation of the object.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        toString(): string;
        /**
         * Obtains a list of all values in the current buffer.
         *
         * @type { number }
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains a list of all values in the current buffer.
         *
         * @type { number }
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains a list of all values in the current buffer.
         *
         * @type { number }
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        length: number;
        /**
         * Obtains the capacity of the current buffer.
         *
         * @returns { number } Returns the capacity of the current buffer.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains the capacity of the current buffer.
         *
         * @returns { number } Returns the capacity of the current buffer.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains the capacity of the current buffer.
         *
         * @returns { number } Returns the capacity of the current buffer.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        getCapacity(): number;
        /**
         * Clears key-value pairs from the current buffer.
         *
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Clears key-value pairs from the current buffer.
         *
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Clears key-value pairs from the current buffer.
         *
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        clear(): void;
        /**
         * Obtains the number of times createDefault(Object) returned a value.
         *
         * @returns { number } Returns the number of times createDefault(java.lang.Object) returned a value.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains the number of times createDefault(Object) returned a value.
         *
         * @returns { number } Returns the number of times createDefault(java.lang.Object) returned a value.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains the number of times createDefault(Object) returned a value.
         *
         * @returns { number } Returns the number of times createDefault(java.lang.Object) returned a value.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        getCreateCount(): number;
        /**
         * Obtains the number of times that the queried values are not matched.
         *
         * @returns { number } Returns the number of times that the queried values are not matched.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains the number of times that the queried values are not matched.
         *
         * @returns { number } Returns the number of times that the queried values are not matched.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains the number of times that the queried values are not matched.
         *
         * @returns { number } Returns the number of times that the queried values are not matched.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        getMissCount(): number;
        /**
         * Obtains the number of times that values are evicted from the buffer.
         *
         * @returns { number } Returns the number of times that values are evicted from the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains the number of times that values are evicted from the buffer.
         *
         * @returns { number } Returns the number of times that values are evicted from the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains the number of times that values are evicted from the buffer.
         *
         * @returns { number } Returns the number of times that values are evicted from the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        getRemovalCount(): number;
        /**
         * Obtains the number of times that the queried values are successfully matched.
         *
         * @returns { number } Returns the number of times that the queried values are successfully matched.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains the number of times that the queried values are successfully matched.
         *
         * @returns { number } Returns the number of times that the queried values are successfully matched.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains the number of times that the queried values are successfully matched.
         *
         * @returns { number } Returns the number of times that the queried values are successfully matched.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        getMatchCount(): number;
        /**
         * Obtains the number of times that values are added to the buffer.
         *
         * @returns { number } Returns the number of times that values are added to the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains the number of times that values are added to the buffer.
         *
         * @returns { number } Returns the number of times that values are added to the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains the number of times that values are added to the buffer.
         *
         * @returns { number } Returns the number of times that values are added to the buffer.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        getPutCount(): number;
        /**
         * Checks whether the current buffer is empty.
         *
         * @returns { boolean } Returns true if the current buffer contains no value.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Checks whether the current buffer is empty.
         *
         * @returns { boolean } Returns true if the current buffer contains no value.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Checks whether the current buffer is empty.
         *
         * @returns { boolean } Returns true if the current buffer contains no value.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        isEmpty(): boolean;
        /**
         * Obtains the value associated with a specified key.
         *
         * @param { K } key - Indicates the key to query.
         * @returns { V | undefined } Returns the value associated with the key if the specified key is present in the buffer; returns null otherwise.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains the value associated with a specified key.
         *
         * @param { K } key - Indicates the key to query.
         * @returns { V | undefined } Returns the value associated with the key if the specified key is present in the buffer; returns null otherwise.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains the value associated with a specified key.
         *
         * @param { K } key - Indicates the key to query.
         * @returns { V | undefined } Returns the value associated with the key if the specified key is present in the buffer; returns null otherwise.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        get(key: K): V | undefined;
        /**
         * Adds a key-value pair to the buffer.
         *
         * @param { K } key - Indicates the key to add.
         * @param { V } value - Indicates the value associated with the key to add.
         * @returns { V } Returns the value associated with the added key; returns the original value if the key to add already exists.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Adds a key-value pair to the buffer.
         *
         * @param { K } key - Indicates the key to add.
         * @param { V } value - Indicates the value associated with the key to add.
         * @returns { V } Returns the value associated with the added key; returns the original value if the key to add already exists.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Adds a key-value pair to the buffer.
         *
         * @param { K } key - Indicates the key to add.
         * @param { V } value - Indicates the value associated with the key to add.
         * @returns { V } Returns the value associated with the added key; returns the original value if the key to add already exists.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        put(key: K, value: V): V;
        /**
         * Obtains a list of all values in the current buffer.
         *
         * @returns { V[] } Returns the list of all values in the current buffer in ascending order, from the most recently accessed to least recently accessed.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains a list of all values in the current buffer.
         *
         * @returns { V[] } Returns the list of all values in the current buffer in ascending order, from the most recently accessed to least recently accessed.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains a list of all values in the current buffer.
         *
         * @returns { V[] } Returns the list of all values in the current buffer in ascending order, from the most recently accessed to least recently accessed.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        values(): V[];
        /**
         * Obtains a list of keys for the values in the current buffer.
         * since 9
         *
         * @returns { K[] } Returns a list of keys sorted from most recently accessed to least recently accessed.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Obtains a list of keys for the values in the current buffer.
         * since 9
         *
         * @returns { K[] } Returns a list of keys sorted from most recently accessed to least recently accessed.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Obtains a list of keys for the values in the current buffer.
         * since 9
         *
         * @returns { K[] } Returns a list of keys sorted from most recently accessed to least recently accessed.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        keys(): K[];
        /**
         * Deletes a specified key and its associated value from the current buffer.
         *
         * @param { K } key - Indicates the key to delete.
         * @returns { V | undefined } Returns an Optional object containing the deleted key-value pair; returns an empty Optional object if the key does not exist.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Deletes a specified key and its associated value from the current buffer.
         *
         * @param { K } key - Indicates the key to delete.
         * @returns { V | undefined } Returns an Optional object containing the deleted key-value pair; returns an empty Optional object if the key does not exist.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Deletes a specified key and its associated value from the current buffer.
         *
         * @param { K } key - Indicates the key to delete.
         * @returns { V | undefined } Returns an Optional object containing the deleted key-value pair; returns an empty Optional object if the key does not exist.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        remove(key: K): V | undefined;
        /**
         * Executes subsequent operations after a value is deleted.
         *
         * @param { boolean } isEvict - The parameter value is true if this method is called due to insufficient capacity,
         * and the parameter value is false in other cases.
         * @param { K } key - Indicates the deleted key.
         * @param { V } value - Indicates the deleted value.
         * @param { V } newValue - The parameter value is the new value associated if the put(java.lang.Object,java.lang.Object)
         * method is called and the key to add already exists. The parameter value is null in other cases.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Executes subsequent operations after a value is deleted.
         *
         * @param { boolean } isEvict - The parameter value is true if this method is called due to insufficient capacity,
         * and the parameter value is false in other cases.
         * @param { K } key - Indicates the deleted key.
         * @param { V } value - Indicates the deleted value.
         * @param { V } newValue - The parameter value is the new value associated if the put(java.lang.Object,java.lang.Object)
         * method is called and the key to add already exists. The parameter value is null in other cases.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Executes subsequent operations after a value is deleted.
         *
         * @param { boolean } isEvict - The parameter value is true if this method is called due to insufficient capacity,
         * and the parameter value is false in other cases.
         * @param { K } key - Indicates the deleted key.
         * @param { V } value - Indicates the deleted value.
         * @param { V } newValue - The parameter value is the new value associated if the put(java.lang.Object,java.lang.Object)
         * method is called and the key to add already exists. The parameter value is null in other cases.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        afterRemoval(isEvict: boolean, key: K, value: V, newValue: V): void;
        /**
         * Checks whether the current buffer contains a specified key.
         *
         * @param { K } key - Indicates the key to check.
         * @returns { boolean } Returns true if the buffer contains the specified key.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Checks whether the current buffer contains a specified key.
         *
         * @param { K } key - Indicates the key to check.
         * @returns { boolean } Returns true if the buffer contains the specified key.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Checks whether the current buffer contains a specified key.
         *
         * @param { K } key - Indicates the key to check.
         * @returns { boolean } Returns true if the buffer contains the specified key.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        contains(key: K): boolean;
        /**
         * Executes subsequent operations if miss to compute a value for the specific key.
         *
         * @param { K } key - Indicates the missed key.
         * @returns { V } Returns the value associated with the key.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Executes subsequent operations if miss to compute a value for the specific key.
         *
         * @param { K } key - Indicates the missed key.
         * @returns { V } Returns the value associated with the key.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Executes subsequent operations if miss to compute a value for the specific key.
         *
         * @param { K } key - Indicates the missed key.
         * @returns { V } Returns the value associated with the key.
         * @throws { BusinessError } 401 - Parameter error. Possible causes:
         * 1.Mandatory parameters are left unspecified;
         * 2.Incorrect parameter types.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        createDefault(key: K): V;
        /**
         * Returns an array of key-value pairs of enumeratable properties of a given object.
         *
         * @returns { IterableIterator<[K, V]> } Returns an array of key-value pairs for the enumeratable properties of the given object itself.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Returns an array of key-value pairs of enumeratable properties of a given object.
         *
         * @returns { IterableIterator<[K, V]> } Returns an array of key-value pairs for the enumeratable properties of the given object itself.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Returns an array of key-value pairs of enumeratable properties of a given object.
         *
         * @returns { IterableIterator<[K, V]> } Returns an array of key-value pairs for the enumeratable properties of the given object itself.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        entries(): IterableIterator<[
            K,
            V
        ]>;
        /**
         * Specifies the default iterator for an object.
         *
         * @returns { IterableIterator<[K, V]> } Returns a two - dimensional array in the form of key - value pairs.
         * @syscap SystemCapability.Utils.Lang
         * @since 9
         */
        /**
         * Specifies the default iterator for an object.
         *
         * @returns { IterableIterator<[K, V]> } Returns a two - dimensional array in the form of key - value pairs.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * Specifies the default iterator for an object.
         *
         * @returns { IterableIterator<[K, V]> } Returns a two - dimensional array in the form of key - value pairs.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        [Symbol.iterator](): IterableIterator<[
            K,
            V
        ]>;
    }
    class Scope {
        /**
         * A constructor used to create a Scope instance with the lower and upper bounds specified.
         *
         * @param { ScopeType } lowerObj - A ScopeType value
         * @param { ScopeType } upperObj - A ScopeType value
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.constructor
         */
        constructor(lowerObj: ScopeType, upperObj: ScopeType);
        /**
         * Obtains a string representation of the current range.
         *
         * @returns { string } Returns a string representation of the current range object.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.toString
         */
        toString(): string;
        /**
         * Returns the intersection of a given range and the current range.
         *
         * @param { Scope } range - A Scope range object
         * @returns { Scope } Returns the intersection of a given range and the current range.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.intersect
         */
        intersect(range: Scope): Scope;
        /**
         * Returns the intersection of the current range and the range specified by the given lower and upper bounds.
         *
         * @param { ScopeType } lowerObj - A ScopeType value
         * @param { ScopeType } upperObj - A ScopeType value
         * @returns { Scope } Returns the intersection of the current range and the range specified by the given lower and upper bounds.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.intersect
         */
        intersect(lowerObj: ScopeType, upperObj: ScopeType): Scope;
        /**
         * Obtains the upper bound of the current range.
         *
         * @returns { ScopeType } Returns the upper bound of the current range.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.getUpper
         */
        getUpper(): ScopeType;
        /**
         * Obtains the lower bound of the current range.
         *
         * @returns { ScopeType } Returns the lower bound of the current range.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.getLower
         */
        getLower(): ScopeType;
        /**
         * Creates the smallest range that includes the current range and the given lower and upper bounds.
         *
         * @param { ScopeType } lowerObj - A ScopeType value
         * @param { ScopeType } upperObj - A ScopeType value
         * @returns { Scope } Returns the smallest range that includes the current range and the given lower and upper bounds.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.expand
         */
        expand(lowerObj: ScopeType, upperObj: ScopeType): Scope;
        /**
         * Creates the smallest range that includes the current range and a given range.
         *
         * @param { Scope } range - A Scope range object
         * @returns { Scope } Returns the smallest range that includes the current range and a given range.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.expand
         */
        expand(range: Scope): Scope;
        /**
         * Creates the smallest range that includes the current range and a given value.
         *
         * @param { ScopeType } value - A ScopeType value
         * @returns { Scope } Returns the smallest range that includes the current range and a given value.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.expand
         */
        expand(value: ScopeType): Scope;
        /**
         * Checks whether a given value is within the current range.
         *
         * @param { ScopeType } value - A ScopeType value
         * @returns { boolean } If the value is within the current range return true,otherwise return false.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.contains
         */
        contains(value: ScopeType): boolean;
        /**
         * Checks whether a given range is within the current range.
         *
         * @param { Scope } range - A Scope range
         * @returns { boolean } If the current range is within the given range return true,otherwise return false.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.contains
         */
        contains(range: Scope): boolean;
        /**
         * Clamps a given value to the current range.
         *
         * @param { ScopeType } value - A ScopeType value
         * @returns { ScopeType } Returns a ScopeType object that a given value is clamped to the current range..
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         * @deprecated since 9
         * @useinstead ohos.util.ScopeHelper.clamp
         */
        clamp(value: ScopeType): ScopeType;
    }
    interface ScopeComparable {
        /**
         * The comparison function is used by the scope.
         *
         * @param { ScopeComparable } other - Other
         * @returns { boolean } Returns whether the current object is greater than or equal to the input object.
         * @syscap SystemCapability.Utils.Lang
         * @since 8
         */
        /**
         * The comparison function is used by the scope.
         *
         * @param { ScopeComparable } other - Other
         * @returns { boolean } Returns whether the current object is greater than or equal to the input object.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @since 10
         */
        /**
         * The comparison function is used by the scope.
         *
         * @param { ScopeComparable } other - Other
         * @returns { boolean } Returns whether the current object is greater than or equal to the input object.
         * @syscap SystemCapability.Utils.Lang
         * @crossplatform
         * @atomicservice
         * @since 12
         */
        compareTo(other: ScopeComparable): boolean;
    }
    class types {
        isArgumentsObject(value: Object): boolean;
        isGeneratorFunction(value: Object): boolean;
        isGeneratorObject(value: Object): boolean;
        isModuleNamespaceObject(value: Object): boolean;
        isProxy(value: Object): boolean;
        isSymbolObject(value: Object): boolean;
    }
    class Aspect {
        static addBefore(targetClass: Object, methodName: string, isStatic: boolean, before: Function): void;
        static addAfter(targetClass: Object, methodName: string, isStatic: boolean, after: Function): void;
        static replace(targetClass: Object, methodName: string, isStatic: boolean, instead: Function): void;
    }
    class Base64Helper {
        encodeToStringSync(src: Uint8Array, options?: Type): string;
    }
    function getErrorString(errno: number): string;
    function printf(format: string, ...args: Object[]): string;
    function promiseWrapper(original: (err: Object, value: Object) => void): Object;
    interface DecodeWithStreamOptions {
        /**
        * Does the call follow additional data blocks. The default value is false.
        * @type { ?boolean }
        * @syscap SystemCapability.Utils.Lang
        * @crossplatform
        * @atomicservice
        * @since 11
        */
        stream?: boolean;
    }
    class TextDecoder {
        constructor(encoding?: string, options?: {
            fatal?: boolean;
            ignoreBOM?: boolean;
        });
        decode(input: Uint8Array, options?: {
            stream?: false;
        }): string;
        decodeWithStream(input: Uint8Array, options?: DecodeWithStreamOptions): string;
    }
    class TextEncoder {
        encode(input?: string): Uint8Array;
        encodeInto(input: string, dest: Uint8Array): {
            read: number;
            written: number;
        };
    }
}
export default util;
