//
// Created by PAWEL on 22/10/2026.
// KIR.cpp - KeetScript Intermediate Representation Implementation 🐱
//

#include "KIR.h"

#include <cstring>
#include <iomanip>

namespace Keety {

    // =========================================================================
    // K-IR Type System Implementation
    // =========================================================================

    KIRType KIRType::fromAnnotation(const TypeAnnotation& ann) {
        KIRType t = fromString(ann.baseType);
        t.refKind = ann.refKind;
        t.lifetime = ann.lifetime;
        return t;
    }

    KIRType KIRType::fromString(std::string_view raw) {
        KIRType t;
        std::string s(raw);
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();

        if (s.rfind("&", 0) == 0) {
            t.refKind = ReferenceKind::ImmutableRef;
            s = s.substr(1);
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
            if (!s.empty() && s.front() == '\'') {
                size_t sp = s.find(' ');
                if (sp != std::string::npos) {
                    t.lifetime = s.substr(0, sp);
                    s = s.substr(sp + 1);
                } else {
                    t.lifetime = s;
                    s.clear();
                }
            }
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
            if (s.rfind("mut", 0) == 0 && (s.size() == 3 || std::isspace(static_cast<unsigned char>(s[3])))) {
                t.refKind = ReferenceKind::MutableRef;
                s = s.substr(3);
            }
        }
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());

        if (s == "void") t.base = KIRBaseType::Void;
        else if (s == "int" || s == "int32") t.base = KIRBaseType::Int32;
        else if (s == "int64" || s == "long") t.base = KIRBaseType::Int64;
        else if (s == "float" || s == "float32") t.base = KIRBaseType::Float32;
        else if (s == "double" || s == "float64") t.base = KIRBaseType::Float64;
        else if (s == "bool" || s == "boolean") t.base = KIRBaseType::Bool;
        else if (s == "string") t.base = KIRBaseType::String;
        else if (s == "Buffer" || s == "io::Buffer") t.base = KIRBaseType::Buffer;
        else if (s == "auto" || s.empty()) t.base = KIRBaseType::Auto;
        else {
            t.base = KIRBaseType::Custom;
            t.customName = s;
        }
        return t;
    }

    std::string KIRType::toString() const {
        std::string s;
        if (refKind == ReferenceKind::ImmutableRef) {
            s += "&";
            if (!lifetime.empty()) s += lifetime + " ";
        } else if (refKind == ReferenceKind::MutableRef) {
            s += "&mut ";
            if (!lifetime.empty()) s += lifetime + " ";
        }

        switch (base) {
            case KIRBaseType::Void: s += "void"; break;
            case KIRBaseType::Int32: s += "int"; break;
            case KIRBaseType::Int64: s += "int64"; break;
            case KIRBaseType::Float32: s += "float"; break;
            case KIRBaseType::Float64: s += "double"; break;
            case KIRBaseType::Bool: s += "bool"; break;
            case KIRBaseType::String: s += "string"; break;
            case KIRBaseType::Buffer: s += "Buffer"; break;
            case KIRBaseType::Auto: s += "auto"; break;
            case KIRBaseType::Custom: s += customName; break;
        }
        return s;
    }

    // =========================================================================
    // K-IR Operand Implementation
    // =========================================================================

    KIROperand KIROperand::reg(int id, KIRType t) {
        KIROperand op;
        op.kind = KIROperandKind::Register;
        op.name = "%" + std::to_string(id);
        op.type = std::move(t);
        return op;
    }

    KIROperand KIROperand::var(std::string name, KIRType t) {
        KIROperand op;
        op.kind = KIROperandKind::Variable;
        op.name = "%" + name;
        op.type = std::move(t);
        return op;
    }

    KIROperand KIROperand::constInt(int64_t v) {
        KIROperand op;
        op.kind = KIROperandKind::ConstantInt;
        op.intVal = v;
        op.type.base = KIRBaseType::Int32;
        return op;
    }

    KIROperand KIROperand::constFloat(double v) {
        KIROperand op;
        op.kind = KIROperandKind::ConstantFloat;
        op.floatVal = v;
        op.type.base = KIRBaseType::Float32;
        return op;
    }

    KIROperand KIROperand::constStr(std::string s) {
        KIROperand op;
        op.kind = KIROperandKind::ConstantString;
        op.strVal = std::move(s);
        op.type.base = KIRBaseType::String;
        return op;
    }

    KIROperand KIROperand::constBool(bool b) {
        KIROperand op;
        op.kind = KIROperandKind::ConstantBool;
        op.boolVal = b;
        op.type.base = KIRBaseType::Bool;
        return op;
    }

    std::string KIROperand::toString() const {
        switch (kind) {
            case KIROperandKind::Variable:
            case KIROperandKind::Register:
                return name;
            case KIROperandKind::ConstantInt:
                return std::to_string(intVal);
            case KIROperandKind::ConstantFloat:
                return std::to_string(floatVal);
            case KIROperandKind::ConstantString:
                return "\"" + strVal + "\"";
            case KIROperandKind::ConstantBool:
                return boolVal ? "true" : "false";
        }
        return "%unknown";
    }

    bool KIROperand::operator==(const KIROperand& o) const {
        if (kind != o.kind) return false;
        switch (kind) {
            case KIROperandKind::Variable:
            case KIROperandKind::Register:
                return name == o.name && type == o.type;
            case KIROperandKind::ConstantInt:
                return intVal == o.intVal;
            case KIROperandKind::ConstantFloat:
                return floatVal == o.floatVal;
            case KIROperandKind::ConstantString:
                return strVal == o.strVal;
            case KIROperandKind::ConstantBool:
                return boolVal == o.boolVal;
        }
        return false;
    }

    // =========================================================================
    // K-IR Instruction Implementation
    // =========================================================================

    std::string KIRInstruction::toString() const {
        std::ostringstream os;
        switch (opcode) {
            case KIROpcode::Alloca:
                os << dest.toString() << " = alloca " << dest.type.toString();
                break;
            case KIROpcode::Store:
                os << "store ";
                if (!operands.empty()) os << operands[0].toString();
                os << ", " << dest.toString();
                break;
            case KIROpcode::Load:
                os << dest.toString() << " = load " << (operands.empty() ? "" : operands[0].toString());
                break;
            case KIROpcode::BorrowRef:
                os << dest.toString() << " = borrow_ref " << (operands.empty() ? "" : operands[0].toString());
                break;
            case KIROpcode::BorrowMut:
                os << dest.toString() << " = borrow_mut " << (operands.empty() ? "" : operands[0].toString());
                break;
            case KIROpcode::Move:
                os << dest.toString() << " = move " << (operands.empty() ? "" : operands[0].toString());
                break;
            case KIROpcode::Call:
                if (!dest.name.empty()) {
                    os << dest.toString() << " = ";
                }
                os << "call @" << opStr << "(";
                for (size_t i = 0; i < operands.size(); ++i) {
                    if (i > 0) os << ", ";
                    os << operands[i].toString();
                }
                os << ")";
                break;
            case KIROpcode::BinaryOp:
                os << dest.toString() << " = " << opStr << " ";
                if (operands.size() >= 1) os << operands[0].toString();
                if (operands.size() >= 2) os << ", " << operands[1].toString();
                break;
            case KIROpcode::Branch:
                os << "branch " << targetBlock;
                break;
            case KIROpcode::BranchCond:
                os << "branch_cond " << (operands.empty() ? "%cond" : operands[0].toString())
                   << ", " << targetBlock << ", " << falseBlock;
                break;
            case KIROpcode::Return:
                os << "return";
                if (!operands.empty()) os << " " << operands[0].toString();
                break;
            case KIROpcode::Drop:
                os << "drop " << (operands.empty() ? dest.toString() : operands[0].toString());
                break;
            case KIROpcode::Zonk:
                os << dest.toString() << " = zonk ";
                if (operands.size() >= 1) os << operands[0].toString();
                if (operands.size() >= 2) os << ", " << operands[1].toString();
                break;
        }
        return os.str();
    }

    // =========================================================================
    // K-IR Binary Serialization & Deserialization
    // =========================================================================

    namespace {
        void writeU32(std::ostream& os, uint32_t val) {
            os.write(reinterpret_cast<const char*>(&val), sizeof(val));
        }

        uint32_t readU32(std::istream& is) {
            uint32_t val = 0;
            is.read(reinterpret_cast<char*>(&val), sizeof(val));
            return val;
        }

        void writeStr(std::ostream& os, const std::string& str) {
            writeU32(os, static_cast<uint32_t>(str.size()));
            if (!str.empty()) os.write(str.data(), str.size());
        }

        std::string readStr(std::istream& is) {
            uint32_t len = readU32(is);
            if (len == 0) return "";
            std::string s(len, '\0');
            is.read(&s[0], len);
            return s;
        }

        void writeKIRType(std::ostream& os, const KIRType& t) {
            writeU32(os, static_cast<uint32_t>(t.base));
            writeStr(os, t.customName);
            writeU32(os, static_cast<uint32_t>(t.refKind));
            writeStr(os, t.lifetime);
        }

        KIRType readKIRType(std::istream& is) {
            KIRType t;
            t.base = static_cast<KIRBaseType>(readU32(is));
            t.customName = readStr(is);
            t.refKind = static_cast<ReferenceKind>(readU32(is));
            t.lifetime = readStr(is);
            return t;
        }

        void writeKIROperand(std::ostream& os, const KIROperand& op) {
            writeU32(os, static_cast<uint32_t>(op.kind));
            writeStr(os, op.name);
            os.write(reinterpret_cast<const char*>(&op.intVal), sizeof(op.intVal));
            os.write(reinterpret_cast<const char*>(&op.floatVal), sizeof(op.floatVal));
            writeStr(os, op.strVal);
            uint8_t b = op.boolVal ? 1 : 0;
            os.write(reinterpret_cast<const char*>(&b), sizeof(b));
            writeKIRType(os, op.type);
        }

        KIROperand readKIROperand(std::istream& is) {
            KIROperand op;
            op.kind = static_cast<KIROperandKind>(readU32(is));
            op.name = readStr(is);
            is.read(reinterpret_cast<char*>(&op.intVal), sizeof(op.intVal));
            is.read(reinterpret_cast<char*>(&op.floatVal), sizeof(op.floatVal));
            op.strVal = readStr(is);
            uint8_t b = 0;
            is.read(reinterpret_cast<char*>(&b), sizeof(b));
            op.boolVal = (b != 0);
            op.type = readKIRType(is);
            return op;
        }
    } // namespace

    void KIRModule::serialize(std::ostream& os) const {
        // Header magic: 'K', 'I', 'R', 0x01
        const char magic[4] = {'K', 'I', 'R', 0x01};
        os.write(magic, 4);

        writeStr(os, name);

        // Imports
        writeU32(os, static_cast<uint32_t>(imports.size()));
        for (const auto& imp : imports) writeStr(os, imp);

        // Structs
        writeU32(os, static_cast<uint32_t>(structs.size()));
        for (const auto& st : structs) {
            writeStr(os, st.name);
            uint8_t exp = st.isExported ? 1 : 0;
            os.write(reinterpret_cast<const char*>(&exp), sizeof(exp));
            writeU32(os, static_cast<uint32_t>(st.fields.size()));
            for (const auto& f : st.fields) {
                writeStr(os, f.name);
                writeKIRType(os, f.type);
                writeStr(os, f.defaultExpr);
            }
        }

        // Functions
        writeU32(os, static_cast<uint32_t>(functions.size()));
        for (const auto& fn : functions) {
            writeStr(os, fn->name);
            writeKIRType(os, fn->returnType);
            uint8_t exp = fn->isExported ? 1 : 0;
            uint8_t mainFlag = fn->isMain ? 1 : 0;
            os.write(reinterpret_cast<const char*>(&exp), sizeof(exp));
            os.write(reinterpret_cast<const char*>(&mainFlag), sizeof(mainFlag));

            // Parameters
            writeU32(os, static_cast<uint32_t>(fn->parameters.size()));
            for (const auto& param : fn->parameters) {
                writeStr(os, param.name);
                writeKIRType(os, param.type);
                uint8_t isM = param.isMove ? 1 : 0;
                os.write(reinterpret_cast<const char*>(&isM), sizeof(isM));
            }

            // Blocks
            writeU32(os, static_cast<uint32_t>(fn->blocks.size()));
            for (const auto& bb : fn->blocks) {
                writeStr(os, bb->label);
                writeU32(os, static_cast<uint32_t>(bb->instructions.size()));
                for (const auto& inst : bb->instructions) {
                    writeU32(os, static_cast<uint32_t>(inst.opcode));
                    writeStr(os, inst.opStr);
                    writeKIROperand(os, inst.dest);
                    writeU32(os, static_cast<uint32_t>(inst.operands.size()));
                    for (const auto& op : inst.operands) {
                        writeKIROperand(os, op);
                    }
                    writeStr(os, inst.targetBlock);
                    writeStr(os, inst.falseBlock);
                }
            }
        }
    }

    std::unique_ptr<KIRModule> KIRModule::deserialize(std::istream& is) {
        char magic[4] = {0};
        is.read(magic, 4);
        if (magic[0] != 'K' || magic[1] != 'I' || magic[2] != 'R' || magic[3] != 0x01) {
            return nullptr;
        }

        auto mod = std::make_unique<KIRModule>();
        mod->name = readStr(is);

        uint32_t importCount = readU32(is);
        mod->imports.reserve(importCount);
        for (uint32_t i = 0; i < importCount; ++i) {
            mod->imports.push_back(readStr(is));
        }

        uint32_t structCount = readU32(is);
        mod->structs.reserve(structCount);
        for (uint32_t i = 0; i < structCount; ++i) {
            KIRStruct st;
            st.name = readStr(is);
            uint8_t exp = 0;
            is.read(reinterpret_cast<char*>(&exp), sizeof(exp));
            st.isExported = (exp != 0);

            uint32_t fldCount = readU32(is);
            st.fields.reserve(fldCount);
            for (uint32_t j = 0; j < fldCount; ++j) {
                KIRStructField f;
                f.name = readStr(is);
                f.type = readKIRType(is);
                f.defaultExpr = readStr(is);
                st.fields.push_back(std::move(f));
            }
            mod->structs.push_back(std::move(st));
        }

        uint32_t funcCount = readU32(is);
        mod->functions.reserve(funcCount);
        for (uint32_t i = 0; i < funcCount; ++i) {
            auto fn = std::make_shared<KIRFunction>();
            fn->name = readStr(is);
            fn->returnType = readKIRType(is);
            uint8_t exp = 0, mainFlag = 0;
            is.read(reinterpret_cast<char*>(&exp), sizeof(exp));
            is.read(reinterpret_cast<char*>(&mainFlag), sizeof(mainFlag));
            fn->isExported = (exp != 0);
            fn->isMain = (mainFlag != 0);

            uint32_t paramCount = readU32(is);
            fn->parameters.reserve(paramCount);
            for (uint32_t p = 0; p < paramCount; ++p) {
                KIRParameter param;
                param.name = readStr(is);
                param.type = readKIRType(is);
                uint8_t isM = 0;
                is.read(reinterpret_cast<char*>(&isM), sizeof(isM));
                param.isMove = (isM != 0);
                fn->parameters.push_back(std::move(param));
            }

            uint32_t blockCount = readU32(is);
            fn->blocks.reserve(blockCount);
            for (uint32_t b = 0; b < blockCount; ++b) {
                auto bb = std::make_shared<KIRBasicBlock>(readStr(is));
                uint32_t instCount = readU32(is);
                bb->instructions.reserve(instCount);
                for (uint32_t k = 0; k < instCount; ++k) {
                    KIRInstruction inst;
                    inst.opcode = static_cast<KIROpcode>(readU32(is));
                    inst.opStr = readStr(is);
                    inst.dest = readKIROperand(is);
                    uint32_t opCount = readU32(is);
                    inst.operands.reserve(opCount);
                    for (uint32_t m = 0; m < opCount; ++m) {
                        inst.operands.push_back(readKIROperand(is));
                    }
                    inst.targetBlock = readStr(is);
                    inst.falseBlock = readStr(is);
                    bb->instructions.push_back(std::move(inst));
                }
                fn->blocks.push_back(bb);
            }
            mod->functions.push_back(fn);
        }

        return mod;
    }

    std::vector<uint8_t> KIRModule::toBinary() const {
        std::ostringstream ss(std::ios::binary);
        serialize(ss);
        std::string s = ss.str();
        return std::vector<uint8_t>(s.begin(), s.end());
    }

    std::unique_ptr<KIRModule> KIRModule::fromBinary(const std::vector<uint8_t>& bytes) {
        std::string s(bytes.begin(), bytes.end());
        std::istringstream ss(s, std::ios::binary);
        return deserialize(ss);
    }

    std::string KIRModule::print() const {
        return KIRPrinter::print(*this);
    }

    // =========================================================================
    // K-IR Printer Implementation
    // =========================================================================

    std::string KIRPrinter::print(const KIRModule& mod) {
        std::ostringstream os;
        os << "; =====================================================================\n";
        os << "; K-IR Module: " << mod.name << "\n";
        os << "; =====================================================================\n\n";

        for (const auto& imp : mod.imports) {
            os << "import " << imp << ";\n";
        }
        if (!mod.imports.empty()) os << "\n";

        for (const auto& st : mod.structs) {
            if (st.isExported) os << "export ";
            os << "struct " << st.name << " {\n";
            for (const auto& f : st.fields) {
                os << "  " << f.name << ": " << f.type.toString();
                if (!f.defaultExpr.empty()) os << " = " << f.defaultExpr;
                os << "\n";
            }
            os << "}\n\n";
        }

        for (const auto& fn : mod.functions) {
            os << printFunction(*fn) << "\n";
        }

        return os.str();
    }

    std::string KIRPrinter::printFunction(const KIRFunction& fn) {
        std::ostringstream os;
        if (fn.isExported) os << "export ";
        os << "fn " << fn.name << "(";
        for (size_t i = 0; i < fn.parameters.size(); ++i) {
            if (i > 0) os << ", ";
            if (fn.parameters[i].isMove) os << "move ";
            os << "%" << fn.parameters[i].name << ": " << fn.parameters[i].type.toString();
        }
        os << "): " << fn.returnType.toString() << " {\n";

        for (const auto& bb : fn.blocks) {
            os << printBlock(*bb);
        }
        os << "}\n";
        return os.str();
    }

    std::string KIRPrinter::printBlock(const KIRBasicBlock& bb) {
        std::ostringstream os;
        os << bb.label << ":\n";
        for (const auto& inst : bb.instructions) {
            os << "  " << printInstruction(inst) << "\n";
        }
        return os.str();
    }

    std::string KIRPrinter::printInstruction(const KIRInstruction& inst) {
        return inst.toString();
    }

    // =========================================================================
    // K-IR Builder Implementation
    // =========================================================================

    KIROperand KIRBuilder::_nextReg(KIRType t) {
        return KIROperand::reg(_regCounter++, std::move(t));
    }

    std::string KIRBuilder::_nextBlockLabel(const std::string& prefix) {
        return prefix + "_" + std::to_string(_blockCounter++);
    }

    std::unique_ptr<KIRModule> KIRBuilder::build(const SourceFile& ast) {
        auto mod = std::make_unique<KIRModule>();
        mod->name = ast.componentName;

        if (ast.isPureModule && ast.moduleBlock) {
            if (!ast.moduleBlock->moduleName.empty()) {
                mod->name = ast.moduleBlock->moduleName;
            }

            for (const auto& imp : ast.moduleBlock->imports) {
                mod->imports.push_back(imp->headerPath.empty() ? imp->asAlias : imp->headerPath);
            }

            for (const auto& st : ast.moduleBlock->structs) {
                KIRStruct s;
                s.name = st->name;
                s.isExported = st->isExported;
                for (const auto& fld : st->fields) {
                    KIRStructField kf;
                    kf.name = fld->name;
                    kf.type = KIRType::fromString(fld->typeName);
                    kf.defaultExpr = fld->defaultValueExpr;
                    s.fields.push_back(std::move(kf));
                }
                mod->structs.push_back(std::move(s));
            }

            for (const auto& fn : ast.moduleBlock->functions) {
                mod->addFunction(buildFunction(fn->name, fn->parameters, fn->returnType, fn->bodyTokens, fn->loc, fn->isExported, fn->isMain));
            }

            for (const auto& cls : ast.moduleBlock->classes) {
                for (const auto& ctor : cls->constructors) {
                    mod->addFunction(buildFunction(cls->name + "::" + ctor->name, ctor->parameters, "void", ctor->bodyTokens, ctor->loc, cls->isExported, false));
                }
                for (const auto& m : cls->methods) {
                    mod->addFunction(buildFunction(cls->name + "::" + m->name, m->parameters, m->returnType, m->bodyTokens, m->loc, cls->isExported, false));
                }
            }
        } else if (ast.script) {
            for (const auto& fn : ast.script->functions) {
                mod->addFunction(buildFunction(fn->name, fn->parameters, fn->returnType, fn->bodyTokens, fn->loc, false, false));
            }
        }

        return mod;
    }

    std::shared_ptr<KIRFunction> KIRBuilder::buildFunction(
        const std::string& name,
        const std::vector<ParameterDecl>& params,
        const std::string& returnType,
        const std::vector<Token>& bodyTokens,
        const SourceLocation& loc,
        bool isExported,
        bool isMain) {

        _regCounter = 0;
        _blockCounter = 0;

        auto fn = std::make_shared<KIRFunction>();
        fn->name = name;
        fn->returnType = KIRType::fromString(returnType);
        fn->isExported = isExported;
        fn->isMain = isMain;
        fn->loc = loc;

        for (const auto& p : params) {
            KIRParameter kp;
            kp.name = p.name;
            kp.type = KIRType::fromString(p.typeName);
            kp.type.refKind = p.refKind;
            kp.type.lifetime = p.lifetime;
            kp.isMove = p.isMove;
            fn->parameters.push_back(std::move(kp));
        }

        auto currentBlock = fn->entryBlock();

        // Scope stack tracking allocated variables for Drop insertion
        std::vector<std::vector<std::string>> scopeVars;
        scopeVars.push_back({});

        auto emitDropVars = [&](std::vector<std::string>& vars) {
            // Drop in reverse order of declaration (LIFO / RAII)
            for (auto it = vars.rbegin(); it != vars.rend(); ++it) {
                KIRInstruction dropInst;
                dropInst.opcode = KIROpcode::Drop;
                dropInst.dest = KIROperand::var(*it);
                currentBlock->addInstruction(dropInst);
            }
            vars.clear();
        };

        // Expression lowering helper
        auto lowerExpression = [&](size_t& cur, const std::string& stopDelim = ";") -> KIROperand {
            int depth = 0;
            std::vector<Token> exprToks;
            while (cur < bodyTokens.size()) {
                if (depth == 0) {
                    if (bodyTokens[cur].text == stopDelim || bodyTokens[cur].is(TokenKind::Semicolon) ||
                        bodyTokens[cur].is(TokenKind::RightBrace)) {
                        break;
                    }
                }
                if (bodyTokens[cur].is(TokenKind::LeftParen) || bodyTokens[cur].is(TokenKind::LeftBracket) ||
                    bodyTokens[cur].is(TokenKind::LeftBrace)) {
                    depth++;
                } else if (bodyTokens[cur].is(TokenKind::RightParen) || bodyTokens[cur].is(TokenKind::RightBracket) ||
                           bodyTokens[cur].is(TokenKind::RightBrace)) {
                    if (depth > 0) depth--;
                }
                exprToks.push_back(bodyTokens[cur++]);
            }

            if (exprToks.empty()) return KIROperand::constInt(0);

            // 1. Move operand: move <ident>
            if (exprToks.size() == 2 && exprToks[0].is(TokenKind::KwMove) && exprToks[1].is(TokenKind::Identifier)) {
                KIROperand reg = _nextReg();
                KIRInstruction inst;
                inst.opcode = KIROpcode::Move;
                inst.dest = reg;
                inst.operands.push_back(KIROperand::var(exprToks[1].text));
                currentBlock->addInstruction(inst);
                return reg;
            }

            // 2. Borrow reference: &<ident> or &mut <ident>
            if (exprToks.size() >= 2 && exprToks[0].is(TokenKind::Ampersand)) {
                bool isMut = (exprToks[1].is(TokenKind::KwMut));
                std::string varName = isMut ? (exprToks.size() >= 3 ? exprToks[2].text : "") : exprToks[1].text;
                KIROperand reg = _nextReg();
                KIRInstruction inst;
                inst.opcode = isMut ? KIROpcode::BorrowMut : KIROpcode::BorrowRef;
                inst.dest = reg;
                inst.operands.push_back(KIROperand::var(varName));
                currentBlock->addInstruction(inst);
                return reg;
            }

            // 3. Binary operator: <lhs> <op> <rhs>
            for (size_t i = 1; i + 1 < exprToks.size(); ++i) {
                std::string op = exprToks[i].text;
                if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
                    op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
                    KIROperand lhs = exprToks[i - 1].is(TokenKind::Identifier) ?
                                     KIROperand::var(exprToks[i - 1].text) :
                                     (exprToks[i - 1].is(TokenKind::NumberLiteral) ?
                                      KIROperand::constInt(std::stoll(exprToks[i - 1].text)) :
                                      KIROperand::constStr(exprToks[i - 1].text));
                    KIROperand rhs = exprToks[i + 1].is(TokenKind::Identifier) ?
                                     KIROperand::var(exprToks[i + 1].text) :
                                     (exprToks[i + 1].is(TokenKind::NumberLiteral) ?
                                      KIROperand::constInt(std::stoll(exprToks[i + 1].text)) :
                                      KIROperand::constStr(exprToks[i + 1].text));
                    KIROperand reg = _nextReg();
                    KIRInstruction inst;
                    inst.opcode = KIROpcode::BinaryOp;
                    inst.opStr = op;
                    inst.dest = reg;
                    inst.operands.push_back(lhs);
                    inst.operands.push_back(rhs);
                    currentBlock->addInstruction(inst);
                    return reg;
                }
            }

            // 4. Function call: func(...)
            if (exprToks.size() >= 3 && exprToks[0].is(TokenKind::Identifier) && exprToks[1].is(TokenKind::LeftParen)) {
                std::string fnName = exprToks[0].text;
                std::vector<KIROperand> args;
                for (size_t i = 2; i < exprToks.size(); ++i) {
                    if (exprToks[i].is(TokenKind::RightParen)) break;
                    if (exprToks[i].is(TokenKind::Comma)) continue;
                    if (exprToks[i].is(TokenKind::Ampersand)) {
                        bool isMut = (i + 1 < exprToks.size() && exprToks[i + 1].is(TokenKind::KwMut));
                        std::string target = isMut ? (i + 2 < exprToks.size() ? exprToks[i + 2].text : "") :
                                                    (i + 1 < exprToks.size() ? exprToks[i + 1].text : "");
                        KIROperand bReg = _nextReg();
                        KIRInstruction bInst;
                        bInst.opcode = isMut ? KIROpcode::BorrowMut : KIROpcode::BorrowRef;
                        bInst.dest = bReg;
                        bInst.operands.push_back(KIROperand::var(target));
                        currentBlock->addInstruction(bInst);
                        args.push_back(bReg);
                        i += (isMut ? 2 : 1);
                    } else if (exprToks[i].is(TokenKind::KwMove)) {
                        std::string target = (i + 1 < exprToks.size()) ? exprToks[i + 1].text : "";
                        KIROperand mReg = _nextReg();
                        KIRInstruction mInst;
                        mInst.opcode = KIROpcode::Move;
                        mInst.dest = mReg;
                        mInst.operands.push_back(KIROperand::var(target));
                        currentBlock->addInstruction(mInst);
                        args.push_back(mReg);
                        i++;
                    } else if (exprToks[i].is(TokenKind::Identifier)) {
                        args.push_back(KIROperand::var(exprToks[i].text));
                    } else if (exprToks[i].is(TokenKind::NumberLiteral)) {
                        args.push_back(KIROperand::constInt(std::stoll(exprToks[i].text)));
                    } else if (exprToks[i].is(TokenKind::StringLiteral)) {
                        args.push_back(KIROperand::constStr(exprToks[i].text));
                    }
                }
                KIROperand reg = _nextReg();
                KIRInstruction inst;
                inst.opcode = KIROpcode::Call;
                inst.opStr = fnName;
                inst.dest = reg;
                inst.operands = std::move(args);
                currentBlock->addInstruction(inst);
                return reg;
            }

            // 5. Single operand / literal
            if (exprToks.size() == 1) {
                if (exprToks[0].is(TokenKind::NumberLiteral)) return KIROperand::constInt(std::stoll(exprToks[0].text));
                if (exprToks[0].is(TokenKind::StringLiteral)) return KIROperand::constStr(exprToks[0].text);
                if (exprToks[0].text == "true") return KIROperand::constBool(true);
                if (exprToks[0].text == "false") return KIROperand::constBool(false);
                if (exprToks[0].is(TokenKind::Identifier)) return KIROperand::var(exprToks[0].text);
            }

            // Default fallback
            return KIROperand::constStr(exprToks[0].text);
        };

        size_t cur = 0;
        if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::LeftBrace)) {
            cur++;
        }

        while (cur < bodyTokens.size()) {
            // Block enter
            if (bodyTokens[cur].is(TokenKind::LeftBrace)) {
                scopeVars.push_back({});
                cur++;
                continue;
            }

            // Block exit
            if (bodyTokens[cur].is(TokenKind::RightBrace)) {
                if (scopeVars.size() > 1) {
                    emitDropVars(scopeVars.back());
                    scopeVars.pop_back();
                    cur++;
                    continue;
                } else {
                    cur++;
                    break;
                }
            }

            if (bodyTokens[cur].is(TokenKind::Semicolon)) {
                cur++;
                continue;
            }

            // Variable declaration: let [mut] <name> [: <type>] [= <expr>];
            if (bodyTokens[cur].is(TokenKind::KwLet) || bodyTokens[cur].is(TokenKind::KwConst)) {
                cur++;
                if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::KwMut)) cur++;
                if (cur >= bodyTokens.size() || !bodyTokens[cur].is(TokenKind::Identifier)) {
                    cur++;
                    continue;
                }
                std::string varName = bodyTokens[cur++].text;
                KIRType vType;
                vType.base = KIRBaseType::Auto;

                if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::Colon)) {
                    cur++;
                    std::string rawType;
                    while (cur < bodyTokens.size() && !bodyTokens[cur].is(TokenKind::Assign) && !bodyTokens[cur].is(TokenKind::Semicolon)) {
                        if (!rawType.empty() && rawType.back() != '&') rawType += " ";
                        rawType += bodyTokens[cur++].text;
                    }
                    vType = KIRType::fromString(rawType);
                }

                // Emit Alloca
                KIRInstruction allocaInst;
                allocaInst.opcode = KIROpcode::Alloca;
                allocaInst.dest = KIROperand::var(varName, vType);
                currentBlock->addInstruction(allocaInst);
                scopeVars.back().push_back(varName);

                if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::Assign)) {
                    cur++; // skip '='
                    KIROperand val = lowerExpression(cur, ";");
                    KIRInstruction storeInst;
                    storeInst.opcode = KIROpcode::Store;
                    storeInst.dest = KIROperand::var(varName, vType);
                    storeInst.operands.push_back(val);
                    currentBlock->addInstruction(storeInst);
                }
                continue;
            }

            // Return statement: return [<expr>];
            if (bodyTokens[cur].is(TokenKind::KwReturn)) {
                cur++;
                if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::Semicolon)) {
                    cur++;
                    // Emit drops for all active scopes before returning
                    for (auto it = scopeVars.rbegin(); it != scopeVars.rend(); ++it) {
                        for (auto vit = it->rbegin(); vit != it->rend(); ++vit) {
                            KIRInstruction dropInst;
                            dropInst.opcode = KIROpcode::Drop;
                            dropInst.dest = KIROperand::var(*vit);
                            currentBlock->addInstruction(dropInst);
                        }
                    }
                    KIRInstruction retInst;
                    retInst.opcode = KIROpcode::Return;
                    currentBlock->addInstruction(retInst);
                } else {
                    KIROperand val = lowerExpression(cur, ";");
                    // Emit drops for all active scopes before returning
                    for (auto it = scopeVars.rbegin(); it != scopeVars.rend(); ++it) {
                        for (auto vit = it->rbegin(); vit != it->rend(); ++vit) {
                            KIRInstruction dropInst;
                            dropInst.opcode = KIROpcode::Drop;
                            dropInst.dest = KIROperand::var(*vit);
                            currentBlock->addInstruction(dropInst);
                        }
                    }
                    KIRInstruction retInst;
                    retInst.opcode = KIROpcode::Return;
                    retInst.operands.push_back(val);
                    currentBlock->addInstruction(retInst);
                }
                continue;
            }

            // Assignment statement: <ident> = <expr>;
            if (bodyTokens[cur].is(TokenKind::Identifier) && cur + 1 < bodyTokens.size() &&
                bodyTokens[cur + 1].is(TokenKind::Assign)) {
                std::string varName = bodyTokens[cur].text;
                cur += 2; // skip name and '='
                KIROperand val = lowerExpression(cur, ";");
                KIRInstruction storeInst;
                storeInst.opcode = KIROpcode::Store;
                storeInst.dest = KIROperand::var(varName);
                storeInst.operands.push_back(val);
                currentBlock->addInstruction(storeInst);
                continue;
            }

            // Conditional statement: if <cond> { ... } [else { ... }]
            if (bodyTokens[cur].is(TokenKind::KwIf)) {
                cur++; // skip 'if'
                bool hasParen = (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::LeftParen));
                if (hasParen) cur++;
                KIROperand cond = lowerExpression(cur, hasParen ? ")" : "{");
                if (hasParen && cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::RightParen)) cur++;

                auto thenBlock = fn->createBlock(_nextBlockLabel("then"));
                auto elseBlock = fn->createBlock(_nextBlockLabel("else"));
                auto mergeBlock = fn->createBlock(_nextBlockLabel("merge"));

                KIRInstruction brCond;
                brCond.opcode = KIROpcode::BranchCond;
                brCond.operands.push_back(cond);
                brCond.targetBlock = thenBlock->label;
                brCond.falseBlock = elseBlock->label;
                currentBlock->addInstruction(brCond);

                currentBlock->successors.push_back(thenBlock->label);
                currentBlock->successors.push_back(elseBlock->label);
                thenBlock->predecessors.push_back(currentBlock->label);
                elseBlock->predecessors.push_back(currentBlock->label);

                // Lower Then Block
                if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::LeftBrace)) {
                    cur++;
                    currentBlock = thenBlock;
                    scopeVars.push_back({});
                    while (cur < bodyTokens.size() && !bodyTokens[cur].is(TokenKind::RightBrace)) {
                        if (bodyTokens[cur].is(TokenKind::Identifier) && cur + 1 < bodyTokens.size() &&
                            bodyTokens[cur + 1].is(TokenKind::Assign)) {
                            std::string v = bodyTokens[cur].text;
                            cur += 2;
                            KIROperand val = lowerExpression(cur, ";");
                            KIRInstruction st;
                            st.opcode = KIROpcode::Store;
                            st.dest = KIROperand::var(v);
                            st.operands.push_back(val);
                            currentBlock->addInstruction(st);
                        } else {
                            cur++;
                        }
                    }
                    if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::RightBrace)) {
                        emitDropVars(scopeVars.back());
                        scopeVars.pop_back();
                        cur++;
                    }
                    if (!thenBlock->isTerminated()) {
                        KIRInstruction br;
                        br.opcode = KIROpcode::Branch;
                        br.targetBlock = mergeBlock->label;
                        thenBlock->addInstruction(br);
                        thenBlock->successors.push_back(mergeBlock->label);
                        mergeBlock->predecessors.push_back(thenBlock->label);
                    }
                }

                // Lower Else Block (if present)
                currentBlock = elseBlock;
                if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::KwElse)) {
                    cur++; // skip 'else'
                    if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::LeftBrace)) {
                        cur++;
                        scopeVars.push_back({});
                        while (cur < bodyTokens.size() && !bodyTokens[cur].is(TokenKind::RightBrace)) {
                            if (bodyTokens[cur].is(TokenKind::Identifier) && cur + 1 < bodyTokens.size() &&
                                bodyTokens[cur + 1].is(TokenKind::Assign)) {
                                std::string v = bodyTokens[cur].text;
                                cur += 2;
                                KIROperand val = lowerExpression(cur, ";");
                                KIRInstruction st;
                                st.opcode = KIROpcode::Store;
                                st.dest = KIROperand::var(v);
                                st.operands.push_back(val);
                                currentBlock->addInstruction(st);
                            } else {
                                cur++;
                            }
                        }
                        if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::RightBrace)) {
                            emitDropVars(scopeVars.back());
                            scopeVars.pop_back();
                            cur++;
                        }
                    }
                }
                if (!elseBlock->isTerminated()) {
                    KIRInstruction br;
                    br.opcode = KIROpcode::Branch;
                    br.targetBlock = mergeBlock->label;
                    elseBlock->addInstruction(br);
                    elseBlock->successors.push_back(mergeBlock->label);
                    mergeBlock->predecessors.push_back(elseBlock->label);
                }

                currentBlock = mergeBlock;
                continue;
            }

            // General expression statement / call
            lowerExpression(cur, ";");
        }

        // Emit final drops if block is not terminated
        if (!currentBlock->isTerminated()) {
            for (auto it = scopeVars.rbegin(); it != scopeVars.rend(); ++it) {
                emitDropVars(*it);
            }
            if (fn->returnType.base == KIRBaseType::Void) {
                KIRInstruction retInst;
                retInst.opcode = KIROpcode::Return;
                currentBlock->addInstruction(retInst);
            }
        }

        return fn;
    }

} // namespace Keety
