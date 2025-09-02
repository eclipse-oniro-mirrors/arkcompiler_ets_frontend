/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "debuginfoDumper.h"

#include "util/es2pandaMacros.h"

namespace ark::es2panda::debuginfo {

DebugInfoDumper::DebugInfoDumper(const pandasm::Program *prog) : prog_(prog) {}

static const char *PutComma(bool comma)
{
    return comma ? "," : "";
}

template <typename T>
void DebugInfoDumper::WrapArray(const char *name, const std::vector<T> &array, bool comma)
{
    ss_ << std::endl;
    Indent();
    ss_ << "\"" << name << "\": "
        << "[";

    if (array.empty()) {
        ss_ << "]" << PutComma(comma);
        return;
    }

    ss_ << "\n";
    indent_++;
    // dump VariableDebugInfo in reverse order to match ts2panda
    // NOLINTNEXTLINE
    if constexpr (std::is_same_v<T, pandasm::debuginfo::LocalVariable>) {
        typename std::vector<T>::const_reverse_iterator elem;
        for (elem = array.rbegin(); elem != array.rend(); ++elem) {
            Indent();
            WriteVariableInfo(*elem);
            (std::next(elem) == array.rend()) ? ss_ << "" : ss_ << ",";
            ss_ << "\n";
        }
        // NOLINTNEXTLINE
    } else {
        typename std::vector<T>::const_iterator elem;
        for (elem = array.begin(); elem != array.end(); ++elem) {
            Indent();
            // NOLINTNEXTLINE
            if constexpr (std::is_same_v<T, pandasm::Ins>) {
                WriteIns(*elem);
                // NOLINTNEXTLINE
            } else if constexpr (std::is_same_v<T, pandasm::Function::Parameter>) {
                ss_ << "\"" << (*elem).type.GetName() << "\"";
                // NOLINTNEXTLINE
            } else if constexpr (std::is_same_v<T, std::string>) {
                ss_ << "\"" << *elem << "\"";
                // NOLINTNEXTLINE
            } else if constexpr (std::is_same_v<T, std::variant<int64_t, double>>) {
                ss_ << (std::holds_alternative<int64_t>(*elem) ? std::to_string(std::get<int64_t>(*elem))
                                                               : std::to_string(std::get<double>(*elem)));
                // NOLINTNEXTLINE
            } else {
                ss_ << std::to_string(*elem);
            }

            (std::next(elem) == array.end()) ? ss_ << "" : ss_ << ",";
            ss_ << "\n";
        }
    }

    indent_--;
    Indent();

    ss_ << "]" << PutComma(comma);
}

void DebugInfoDumper::WrapRegArray(ark::pandasm::Ins const &ins, bool comma)
{
    ss_ << std::endl;
    Indent();
    ss_ << "\"regs\": "
        << "[";

    auto const size = ins.RegSize();
    if (size == 0U) {
        ss_ << "]" << PutComma(comma);
        return;
    }

    ss_ << "\n";
    ++indent_;

    for (std::size_t i = 0U; i < size;) {
        Indent();
        ss_ << ins.GetReg(i);
        if (++i < size) {
            ss_ << ',';
        }
        ss_ << '\n';
    }

    --indent_;
    Indent();

    ss_ << "]" << PutComma(comma);
}

void DebugInfoDumper::WrapIDArray(ark::pandasm::Ins const &ins, bool comma)
{
    ss_ << std::endl;
    Indent();
    ss_ << "\"ids\": "
        << "[";

    auto const size = ins.IDSize();
    if (size == 0U) {
        ss_ << "]" << PutComma(comma);
        return;
    }

    ss_ << "\n";
    ++indent_;

    for (std::size_t i = 0U; i < size;) {
        Indent();
        ss_ << ins.GetID(i);
        if (++i < size) {
            ss_ << ',';
        }
        ss_ << '\n';
    }

    --indent_;
    Indent();

    ss_ << "]" << PutComma(comma);
}

void DebugInfoDumper::WrapImmArray(ark::pandasm::Ins const &ins, bool comma)
{
    ss_ << std::endl;
    Indent();
    ss_ << "\"imms\": "
        << "[";

    auto const size = ins.ImmSize();
    if (size == 0U) {
        ss_ << "]" << PutComma(comma);
        return;
    }

    ss_ << "\n";
    ++indent_;

    for (std::size_t i = 0U; i < size;) {
        Indent();
        auto value = ins.GetImm(i);
        ss_ << (std::holds_alternative<int64_t>(value) ? std::to_string(std::get<int64_t>(value))
                                                       : std::to_string(std::get<double>(value)));
        if (++i < size) {
            ss_ << ',';
        }
        ss_ << '\n';
    }

    --indent_;
    Indent();

    ss_ << "]" << PutComma(comma);
}

void DebugInfoDumper::WriteIns(const pandasm::Ins &ins)
{
    ss_ << "{";
    WriteProperty("opcode", ins.ToString());
    indent_++;
    WrapRegArray(ins);
    WrapIDArray(ins);
    WrapImmArray(ins);
    ss_ << std::endl;
    Indent();
    ss_ << "\"label\": "
        << "\"" << (ins.HasLabel() ? ins.Label().c_str() : "") << "\",";
    WritePosInfo(ins.insDebug);
    indent_--;
    Indent();
    ss_ << "}";
}

void DebugInfoDumper::WriteMetaData(const std::vector<pandasm::AnnotationData> &metaData)
{
    for (const auto &it : metaData) {
        for (const auto &elem : it.GetElements()) {
            pandasm::ScalarValue *value = elem.GetValue()->GetAsScalar();
            if (value->GetType() == pandasm::Value::Type::STRING) {
                WriteProperty(elem.GetName().c_str(), value->GetValue<std::string>(), false);
            } else if (value->GetType() == pandasm::Value::Type::U32) {
                WriteProperty(elem.GetName().c_str(), value->GetValue<size_t>());
            }
        }
    }
}

void DebugInfoDumper::WritePosInfo(const pandasm::debuginfo::Ins &posInfo)
{
    ss_ << std::endl;
    Indent();
    ss_ << "\"debug_pos_info\": {";
    WriteProperty("boundLeft", posInfo.BoundLeft());
    WriteProperty("boundRight", posInfo.BoundRight());
    WriteProperty("sourceLineNum", posInfo.LineNumber());
    WriteProperty("wholeLine", posInfo.WholeLine(), false);
    Indent();
    ss_ << "}" << std::endl;
}

void DebugInfoDumper::WriteVariableInfo(const pandasm::debuginfo::LocalVariable &localVariableDebug)
{
    ss_ << "{";
    WriteProperty("name", localVariableDebug.name);
    WriteProperty("signature", localVariableDebug.signature);
    WriteProperty("signatureType", localVariableDebug.signatureType);
    WriteProperty("reg", localVariableDebug.reg);
    WriteProperty("start", static_cast<size_t>(localVariableDebug.start));
    WriteProperty("length", static_cast<size_t>(localVariableDebug.length), false);
    Indent();
    ss_ << "}";
}

void DebugInfoDumper::DumpFuncBody(std::string const &name, const pandasm::Function &func)
{
    indent_++;
    Indent();
    ss_ << "{";
    WriteProperty("name", name);
    ss_ << std::endl;

    indent_++;
    Indent();
    ss_ << "\"signature\": {";
    WriteProperty("retType", func.returnType.GetName());
    indent_++;
    WrapArray("params", func.params, false);
    indent_ -= 2U;
    ss_ << std::endl;
    Indent();
    ss_ << "},";

    WrapArray("ins", func.ins);
    WrapArray("variables", func.localVariableDebug);
    WriteProperty("sourceFile", func.sourceFile);
    WriteProperty("sourceCode", func.sourceCode);
    // icSize - parameterLength - funcName
    WriteMetaData(func.metadata->GetAnnotations());

    indent_--;
    Indent();
    ss_ << "}";
}

void DebugInfoDumper::DumpFunctions(const std::map<std::string, pandasm::Function> &table)
{
    auto iter = table.begin();

    for (; iter != table.end(); ++iter) {
        DumpFuncBody(iter->first, iter->second);

        if (std::next(iter) != table.end()) {
            ss_ << ",";
        }

        ss_ << std::endl;
    }
}

void DebugInfoDumper::Dump()
{
    ss_ << "{\n";
    indent_++;
    Indent();
    ss_ << "\"functions\": [" << std::endl;

    DumpFunctions(prog_->functionStaticTable);
    DumpFunctions(prog_->functionInstanceTable);

    indent_--;
    Indent();
    ss_ << "]" << std::endl;
    ss_ << "}";
    ss_ << std::endl;
    std::cout << ss_.str();
}

template <typename T>
void DebugInfoDumper::WriteProperty(const char *key, T &&value, bool comma)
{
    static_assert(std::is_arithmetic_v<std::decay_t<T>> || std::is_same_v<std::string, std::decay_t<T>>,
                  "Invalid data type.");
    ss_ << std::endl;
    indent_++;
    Indent();
    ss_ << "\"" << key << "\": ";
    if constexpr (std::is_arithmetic_v<std::decay_t<T>>) {
        ss_ << std::to_string(std::forward<T>(value));
    } else {
        ss_ << "\"" << std::forward<T>(value) << "\"";
    }

    comma ? ss_ << "," : ss_ << std::endl;
    indent_--;
}

void DebugInfoDumper::Indent()
{
    for (int32_t i = 0; i <= indent_; i++) {
        ss_ << "  ";
    }
}

}  // namespace ark::es2panda::debuginfo
