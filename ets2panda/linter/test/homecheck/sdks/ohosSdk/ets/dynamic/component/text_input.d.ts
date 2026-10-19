/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

declare enum InputType {

    Normal,

    Number,

    PhoneNumber,

    Email,

    Password,

    NUMBER_PASSWORD = 8,
    /*
     * UserName entry mode.
     *
     * @syscap SystemCapability.ArkUI.ArkUI.Full
     * @since 11
     */

    USER_NAME = 10,

    NEW_PASSWORD = 11,

    NUMBER_DECIMAL = 12,

    URL = 13
}

declare enum ContentType {

    USER_NAME = 0,

    PASSWORD = 1,

    NEW_PASSWORD = 2,

    FULL_STREET_ADDRESS = 3,

    HOUSE_NUMBER = 4,

    DISTRICT_ADDRESS = 5,

    CITY_ADDRESS = 6,

    PROVINCE_ADDRESS = 7,

    COUNTRY_ADDRESS = 8,

    PERSON_FULL_NAME = 9,

    PERSON_LAST_NAME = 10,

    PERSON_FIRST_NAME = 11,

    PHONE_NUMBER = 12,

    PHONE_COUNTRY_CODE = 13,

    FULL_PHONE_NUMBER = 14,

    EMAIL_ADDRESS = 15,

    BANK_CARD_NUMBER = 16,

    ID_CARD_NUMBER = 17,

    NICKNAME = 23,

    DETAIL_INFO_WITHOUT_STREET = 24,

    FORMAT_ADDRESS = 25
}

declare enum EnterKeyType {

    Go = 2,

    Search = 3,

    Send = 4,

    Next = 5,

    Done = 6,

    PREVIOUS = 7,

    NEW_LINE = 8
}

declare interface UnderlineColor {

    typing?: ResourceColor | undefined;

    normal?: ResourceColor | undefined;

    error?: ResourceColor | undefined;

    disable?: ResourceColor | undefined;
}

declare interface SubmitEvent {

    keepEditableState(): void;

    text: string;
}

declare class TextInputController extends TextContentControllerBase {

    constructor();

    caretPosition(value: number): void;

    setTextSelection(selectionStart: number, selectionEnd: number, options?: SelectionOptions): void;

    stopEditing(): void;
}

declare interface TextInputOptions {

    placeholder?: ResourceStr;

    text?: ResourceStr;

    controller?: TextInputController;
}

declare enum TextInputStyle {

    Default,

    Inline
}

interface TextInputInterface {

    (value?: TextInputOptions): TextInputAttribute;
}

interface PasswordIcon {

    onIconSrc?: string | Resource;

    offIconSrc?: string | Resource;
}

declare class TextInputAttribute extends CommonMethod<TextInputAttribute> {

    type(value: InputType): TextInputAttribute;

    contentType(value: ContentType): TextInputAttribute;

    placeholderColor(value: ResourceColor): TextInputAttribute;

    textOverflow(value: TextOverflow): TextInputAttribute;

    textIndent(value: Dimension): TextInputAttribute;

    placeholderFont(value?: Font): TextInputAttribute;

    enterKeyType(value: EnterKeyType): TextInputAttribute;

    caretColor(value: ResourceColor): TextInputAttribute;

    onEditChanged(callback: (isEditing: boolean) => void): TextInputAttribute;

    onEditChange(callback: (isEditing: boolean) => void): TextInputAttribute;

    onSubmit(callback: (enterKey: EnterKeyType, event: SubmitEvent) => void): TextInputAttribute;

    onChange(callback: EditableTextOnChangeCallback): TextInputAttribute;

    onTextSelectionChange(callback: (selectionStart: number, selectionEnd: number) => void): TextInputAttribute;

    onContentScroll(callback: (totalOffsetX: number, totalOffsetY: number) => void): TextInputAttribute;

    maxLength(value: number): TextInputAttribute;

    fontColor(value: ResourceColor): TextInputAttribute;

    fontSize(value: Length): TextInputAttribute;

    fontStyle(value: FontStyle): TextInputAttribute;

    fontWeight(value: number | FontWeight | string): TextInputAttribute;

    fontFamily(value: ResourceStr): TextInputAttribute;

    inputFilter(value: ResourceStr, error?: (value: string) => void): TextInputAttribute;

    onCopy(callback: (value: string) => void): TextInputAttribute;

    onCut(callback: (value: string) => void): TextInputAttribute;

    onPaste(callback: (value: string, event: PasteEvent) => void): TextInputAttribute;

    copyOption(value: CopyOptions): TextInputAttribute;

    showPasswordIcon(value: boolean): TextInputAttribute;

    textAlign(value: TextAlign): TextInputAttribute;

    style(value: TextInputStyle | TextContentStyle): TextInputAttribute;

    caretStyle(value: CaretStyle): TextInputAttribute;

    selectedBackgroundColor(value: ResourceColor): TextInputAttribute;

    caretPosition(value: number): TextInputAttribute;

    enableKeyboardOnFocus(value: boolean): TextInputAttribute;

    passwordIcon(value: PasswordIcon): TextInputAttribute;

    showError(value?: ResourceStr | undefined): TextInputAttribute;

    showUnit(value: CustomBuilder): TextInputAttribute;

    showUnderline(value: boolean): TextInputAttribute;

    underlineColor(value: ResourceColor | UnderlineColor | undefined): TextInputAttribute;

    selectionMenuHidden(value: boolean): TextInputAttribute;

    barState(value: BarState): TextInputAttribute;

    maxLines(value: number): TextInputAttribute;

    wordBreak(value: WordBreak): TextInputAttribute;

    lineBreakStrategy(strategy: LineBreakStrategy): TextInputAttribute;

    customKeyboard(value: CustomBuilder, options?: KeyboardOptions): TextInputAttribute;

    showCounter(value: boolean, options?: InputCounterOptions): TextInputAttribute;

    cancelButton(value: {
        style?: CancelButtonStyle;
        icon?: IconOptions;
    }): TextInputAttribute;

    selectAll(value: boolean): TextInputAttribute;

    minFontSize(value: number | string | Resource): TextInputAttribute;

    maxFontSize(value: number | string | Resource): TextInputAttribute;

    heightAdaptivePolicy(value: TextHeightAdaptivePolicy): TextInputAttribute;

    enableAutoFill(value: boolean): TextInputAttribute;

    decoration(value: TextDecorationOptions): TextInputAttribute;

    letterSpacing(value: number | string | Resource): TextInputAttribute;

    lineHeight(value: number | string | Resource): TextInputAttribute;

    passwordRules(value: string): TextInputAttribute;

    fontFeature(value: string): TextInputAttribute;

    showPassword(visible: boolean): TextInputAttribute;

    onSecurityStateChange(callback: Callback<boolean>): TextInputAttribute;

    onWillInsert(callback: Callback<InsertValue, boolean>): TextInputAttribute;

    onDidInsert(callback: Callback<InsertValue>): TextInputAttribute;

    onWillDelete(callback: Callback<DeleteValue, boolean>): TextInputAttribute;

    onDidDelete(callback: Callback<DeleteValue>): TextInputAttribute;

    editMenuOptions(editMenu: EditMenuOptions): TextInputAttribute;

    enablePreviewText(enable: boolean): TextInputAttribute;

    enableHapticFeedback(isEnabled: boolean): TextInputAttribute;
}

declare const TextInput: TextInputInterface;

declare const TextInputInstance: TextInputAttribute;
