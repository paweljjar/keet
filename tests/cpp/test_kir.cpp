//
// tests/cpp/test_kir.cpp - Unit tests for KeetScript Intermediate Representation (K-IR)
//

#include <keet/Testing.h>
#include "KIR.h"
#include "Lexer.h"
#include "Parser.h"

using namespace Keety;

KEET_TEST(KIR, TypeSystem) {
    auto tInt = KIRType::fromString("int");
    KEET_ASSERT_EQ(tInt.base, KIRBaseType::Int32);
    KEET_ASSERT_FALSE(tInt.isReference());
    KEET_ASSERT_EQ(tInt.toString(), "int");

    auto tStrRef = KIRType::fromString("&string");
    KEET_ASSERT_EQ(tStrRef.base, KIRBaseType::String);
    KEET_ASSERT_TRUE(tStrRef.isImmutableRef());
    KEET_ASSERT_FALSE(tStrRef.isMutableRef());
    KEET_ASSERT_EQ(tStrRef.toString(), "&string");

    auto tMutBuf = KIRType::fromString("&mut Buffer");
    KEET_ASSERT_EQ(tMutBuf.base, KIRBaseType::Buffer);
    KEET_ASSERT_TRUE(tMutBuf.isMutableRef());
    KEET_ASSERT_EQ(tMutBuf.toString(), "&mut Buffer");

    auto tLife = KIRType::fromString("&'a mut Buffer");
    KEET_ASSERT_TRUE(tLife.isMutableRef());
    KEET_ASSERT_EQ(tLife.lifetime, "'a");
}

KEET_TEST(KIR, OperandsAndInstructions) {
    auto reg0 = KIROperand::reg(0, KIRType::fromString("int"));
    KEET_ASSERT_EQ(reg0.toString(), "%0");

    auto varX = KIROperand::var("count", KIRType::fromString("int"));
    KEET_ASSERT_EQ(varX.toString(), "%count");

    auto cInt = KIROperand::constInt(42);
    KEET_ASSERT_EQ(cInt.toString(), "42");

    auto cStr = KIROperand::constStr("hello");
    KEET_ASSERT_EQ(cStr.toString(), "\"hello\"");

    // Test Alloca instruction
    KIRInstruction allocaInst;
    allocaInst.opcode = KIROpcode::Alloca;
    allocaInst.dest = varX;
    KEET_ASSERT_EQ(allocaInst.toString(), "%count = alloca int");

    // Test Store instruction
    KIRInstruction storeInst;
    storeInst.opcode = KIROpcode::Store;
    storeInst.dest = varX;
    storeInst.operands.push_back(cInt);
    KEET_ASSERT_EQ(storeInst.toString(), "store 42, %count");

    // Test BorrowRef instruction
    KIRInstruction borrowInst;
    borrowInst.opcode = KIROpcode::BorrowRef;
    borrowInst.dest = reg0;
    borrowInst.operands.push_back(varX);
    KEET_ASSERT_EQ(borrowInst.toString(), "%0 = borrow_ref %count");

    // Test Move instruction
    KIRInstruction moveInst;
    moveInst.opcode = KIROpcode::Move;
    moveInst.dest = reg0;
    moveInst.operands.push_back(varX);
    KEET_ASSERT_EQ(moveInst.toString(), "%0 = move %count");

    // Test Branch instruction
    KIRInstruction brInst;
    brInst.opcode = KIROpcode::Branch;
    brInst.targetBlock = "bb_merge";
    KEET_ASSERT_EQ(brInst.toString(), "branch bb_merge");

    // Test Drop instruction
    KIRInstruction dropInst;
    dropInst.opcode = KIROpcode::Drop;
    dropInst.dest = varX;
    KEET_ASSERT_EQ(dropInst.toString(), "drop %count");
}

KEET_TEST(KIR, BasicBlockAndCFG) {
    KIRBasicBlock bb("entry");
    KEET_ASSERT_FALSE(bb.isTerminated());

    KIRInstruction inst;
    inst.opcode = KIROpcode::Alloca;
    inst.dest = KIROperand::var("x", KIRType::fromString("int"));
    bb.addInstruction(inst);
    KEET_ASSERT_FALSE(bb.isTerminated());

    KIRInstruction ret;
    ret.opcode = KIROpcode::Return;
    bb.addInstruction(ret);
    KEET_ASSERT_TRUE(bb.isTerminated());
}

KEET_TEST(KIR, BuilderFromKeetScriptAST) {
    std::string code = R"(
module MathKIR;

export struct Point {
    x: int,
    y: int
}

export fn add(a: int, b: int): int {
    return a + b;
}

fn testBorrow(): void {
    let mut b: Buffer = Buffer{};
    {
        let r: &Buffer = &b;
    }
    let rmut: &mut Buffer = &mut b;
    let movedB = move b;
}

fn testBranch(score: int): string {
    let mut grade: string = "F";
    if score >= 90 {
        grade = "A";
    } else {
        grade = "B";
    }
    return grade;
}
)";

    Lexer lexer(code, "MathKIR.keet");
    auto tokens = lexer.tokenize();
    Parser parser(tokens, "MathKIR");
    auto ast = parser.parse();

    KIRBuilder builder;
    auto mod = builder.build(*ast);
    KEET_ASSERT_TRUE(mod != nullptr);
    KEET_ASSERT_EQ(mod->name, "MathKIR");
    KEET_ASSERT_EQ(mod->structs.size(), 1);
    KEET_ASSERT_EQ(mod->structs[0].name, "Point");
    KEET_ASSERT_EQ(mod->functions.size(), 3);

    // Verify add function
    auto fnAdd = mod->findFunction("add");
    KEET_ASSERT_TRUE(fnAdd != nullptr);
    KEET_ASSERT_EQ(fnAdd->parameters.size(), 2);
    KEET_ASSERT_TRUE(fnAdd->isExported);
    KEET_ASSERT_FALSE(fnAdd->blocks.empty());

    // Verify testBorrow function has Alloca, BorrowRef, BorrowMut, Move, Drop
    auto fnBorrow = mod->findFunction("testBorrow");
    KEET_ASSERT_TRUE(fnBorrow != nullptr);
    bool hasAlloca = false;
    bool hasBorrowRef = false;
    bool hasBorrowMut = false;
    bool hasMove = false;
    bool hasDrop = false;

    for (const auto& bb : fnBorrow->blocks) {
        for (const auto& inst : bb->instructions) {
            if (inst.opcode == KIROpcode::Alloca) hasAlloca = true;
            if (inst.opcode == KIROpcode::BorrowRef) hasBorrowRef = true;
            if (inst.opcode == KIROpcode::BorrowMut) hasBorrowMut = true;
            if (inst.opcode == KIROpcode::Move) hasMove = true;
            if (inst.opcode == KIROpcode::Drop) hasDrop = true;
        }
    }
    KEET_ASSERT_TRUE(hasAlloca);
    KEET_ASSERT_TRUE(hasBorrowRef);
    KEET_ASSERT_TRUE(hasBorrowMut);
    KEET_ASSERT_TRUE(hasMove);
    KEET_ASSERT_TRUE(hasDrop);

    // Verify testBranch function has BranchCond and multiple blocks
    auto fnBranch = mod->findFunction("testBranch");
    KEET_ASSERT_TRUE(fnBranch != nullptr);
    KEET_ASSERT_TRUE(fnBranch->blocks.size() >= 3);
    bool hasBranchCond = false;
    for (const auto& bb : fnBranch->blocks) {
        for (const auto& inst : bb->instructions) {
            if (inst.opcode == KIROpcode::BranchCond) hasBranchCond = true;
        }
    }
    KEET_ASSERT_TRUE(hasBranchCond);
}

KEET_TEST(KIR, TextPrinter) {
    std::string code = R"(
module PrintTest;

fn compute(x: int): int {
    let mut y: int = x;
    return y;
}
)";
    Lexer lexer(code, "PrintTest.keet");
    auto tokens = lexer.tokenize();
    Parser parser(tokens, "PrintTest");
    auto ast = parser.parse();

    KIRBuilder builder;
    auto mod = builder.build(*ast);
    std::string text = mod->print();

    KEET_ASSERT_TRUE(text.find("K-IR Module: PrintTest") != std::string::npos);
    KEET_ASSERT_TRUE(text.find("fn compute(%x: int): int") != std::string::npos);
    KEET_ASSERT_TRUE(text.find("entry:") != std::string::npos);
    KEET_ASSERT_TRUE(text.find("alloca") != std::string::npos);
    KEET_ASSERT_TRUE(text.find("return") != std::string::npos);
}

KEET_TEST(KIR, BinarySerializationRoundTrip) {
    std::string code = R"(
module BinaryRoundTrip;

export struct Item {
    id: int,
    title: string
}

export fn transform(a: int, b: int): int {
    let mut sum: int = a + b;
    return sum;
}
)";
    Lexer lexer(code, "BinaryRoundTrip.keet");
    auto tokens = lexer.tokenize();
    Parser parser(tokens, "BinaryRoundTrip");
    auto ast = parser.parse();

    KIRBuilder builder;
    auto originalMod = builder.build(*ast);

    // Serialize to binary bytes
    auto bytes = originalMod->toBinary();
    KEET_ASSERT_FALSE(bytes.empty());

    // Deserialize back from binary bytes
    auto restoredMod = KIRModule::fromBinary(bytes);
    KEET_ASSERT_TRUE(restoredMod != nullptr);
    KEET_ASSERT_EQ(restoredMod->name, originalMod->name);
    KEET_ASSERT_EQ(restoredMod->structs.size(), originalMod->structs.size());
    KEET_ASSERT_EQ(restoredMod->functions.size(), originalMod->functions.size());

    // Assert exact textual identity of printed representation
    std::string originalText = originalMod->print();
    std::string restoredText = restoredMod->print();
    KEET_ASSERT_EQ(originalText, restoredText);
}

KEET_TEST_MAIN()
