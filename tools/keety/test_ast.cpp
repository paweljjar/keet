//
// Created by PAWEL on 19/10/2026.
// Phase 1 AST and Grammar Verification Test
//

#include <iostream>
#include <fstream>
#include <cassert>
#include <filesystem>
#include "Token.h"
#include "AST.h"
#include "ASTPrinter.h"
#include "BorrowChecker.h"
#include "Diagnostic.h"
#include "IncrementalCache.h"
#include "KIR.h"
#include "Lexer.h"
#include "Parser.h"
#include "Codegen.h"

int main() {
    using namespace Keety;

    // Build a sample SourceFile AST manually to verify Phase 1 grammar definition
    auto file = std::make_unique<SourceFile>("CounterDashboard");

    // 1.3 External C++ bridge
    auto script = std::make_unique<ScriptBlock>();
    auto import1 = std::make_shared<ImportCppDecl>();
    import1->kind = ImportCppDecl::ImportKind::ModuleAlias;
    import1->headerPath = "services/Analytics.h";
    import1->asAlias = "Analytics";
    script->imports.push_back(import1);

    auto import2 = std::make_shared<ImportCppDecl>();
    import2->kind = ImportCppDecl::ImportKind::SelectiveSymbols;
    import2->headerPath = "utils/Math.h";
    import2->symbols = {"clamp", "lerp"};
    script->imports.push_back(import2);

    // 1.1 State and Let declarations
    auto stateCount = std::make_shared<StateDecl>();
    stateCount->name = "count";
    stateCount->typeName = "int";
    stateCount->initialValueExpr = "0";
    script->stateDecls.push_back(stateCount);

    auto stateQuery = std::make_shared<StateDecl>();
    stateQuery->name = "query";
    stateQuery->typeName = "string";
    stateQuery->initialValueExpr = "\"\"";
    script->stateDecls.push_back(stateQuery);

    auto letTitle = std::make_shared<LetDecl>();
    letTitle->name = "title";
    letTitle->typeName = "string";
    letTitle->defaultValueExpr = "\"Dashboard\"";
    script->letDecls.push_back(letTitle);

    // 1.1 def function
    auto resetFn = std::make_shared<FunctionDecl>();
    resetFn->name = "reset";
    resetFn->returnType = "void";
    resetFn->body = "count = 0; Analytics::logReset();";
    script->functions.push_back(resetFn);

    file->script = std::move(script);

    // 1.1 Template with 1.2 Event expressions and two-way bindings
    auto tmpl = std::make_unique<TemplateBlock>();
    auto rootDiv = std::make_shared<ElementNode>("div");
    rootDiv->attributes.push_back(AttributeBinding{
        .kind = AttributeKind::StaticLiteral,
        .name = "class",
        .rawValue = "counter-card"
    });

    auto h1 = std::make_shared<ElementNode>("h1");
    h1->children.push_back(std::make_shared<InterpolationNode>("title"));
    rootDiv->children.push_back(h1);

    // Two-way binding (bind:value={query})
    auto input = std::make_shared<ElementNode>("input");
    input->isSelfClosing = true;
    input->attributes.push_back(AttributeBinding{
        .kind = AttributeKind::StaticLiteral,
        .name = "type",
        .rawValue = "text"
    });
    input->attributes.push_back(AttributeBinding{
        .kind = AttributeKind::TwoWayBinding,
        .name = "bind:value",
        .bindTargetProperty = "value",
        .bindStateVariable = "query"
    });
    rootDiv->children.push_back(input);

    // Inline event mutation (onClick={() => count += 1})
    auto btnInc = std::make_shared<ElementNode>("button");
    btnInc->attributes.push_back(AttributeBinding{
        .kind = AttributeKind::EventCallback,
        .name = "onClick",
        .rawValue = "() => count += 1",
        .eventExpr = EventExpression{
            .paramStyle = EventParamStyle::ZeroArg,
            .rawExpression = "count += 1",
            .targetState = "count",
            .mutationOp = "+=",
            .mutationValue = "1"
        }
    });
    btnInc->children.push_back(std::make_shared<TextNode>("+1"));
    rootDiv->children.push_back(btnInc);

    // Control flow (if-else)
    auto ifNode = std::make_shared<IfBlockNode>();
    IfBranch thenBranch;
    thenBranch.conditionExpr = "count > 10";
    auto highP = std::make_shared<ElementNode>("p");
    highP->children.push_back(std::make_shared<TextNode>("High count!"));
    thenBranch.children.push_back(highP);
    ifNode->branches.push_back(thenBranch);

    IfBranch elseBranch;
    elseBranch.conditionExpr = ""; // unconditional else
    auto lowP = std::make_shared<ElementNode>("p");
    lowP->children.push_back(std::make_shared<TextNode>("Keep clicking..."));
    elseBranch.children.push_back(lowP);
    ifNode->branches.push_back(elseBranch);
    rootDiv->children.push_back(ifNode);

    tmpl->roots.push_back(rootDiv);
    file->templateBlock = std::move(tmpl);

    // 1.1 Scoped Styles
    auto style = std::make_unique<StyleBlock>();
    auto rule = std::make_shared<StyleRule>();
    rule->selector = ".counter-card";
    rule->declarations.push_back(StyleDeclaration{.property = "padding", .value = "1.5rem"});
    rule->declarations.push_back(StyleDeclaration{.property = "background", .value = "#f8fafc"});
    style->rules.push_back(rule);
    file->styleBlock = std::move(style);

    // Full-stack server block
    auto server = std::make_unique<ServerBlock>();
    auto db = std::make_shared<DatabaseDecl>();
    db->name = "db";
    db->filePath = "app_data.db";
    auto usersTable = std::make_shared<TableDecl>();
    usersTable->name = "users";
    auto colId = std::make_shared<TableColumnDecl>();
    colId->name = "id";
    colId->typeName = "int";
    colId->isPrimaryKey = true;
    colId->isAutoIncrement = true;
    usersTable->columns.push_back(colId);
    auto colName = std::make_shared<TableColumnDecl>();
    colName->name = "name";
    colName->typeName = "string";
    usersTable->columns.push_back(colName);
    db->tables.push_back(usersTable);
    server->database = db;

    auto backend = std::make_shared<BackendServerDecl>();
    backend->name = "api";
    backend->port = 8000;
    backend->threads = 4;
    auto routeGet = std::make_shared<RouteDecl>();
    routeGet->method = "GET";
    routeGet->path = "/api/users";
    routeGet->paramName = "req";
    routeGet->body = "return db.table(\"users\").find();";
    backend->routes.push_back(routeGet);
    server->backend = backend;

    auto frontend = std::make_shared<FrontendServerDecl>();
    frontend->name = "web";
    frontend->port = 8080;
    frontend->apiBaseUrl = "http://localhost:8000";
    frontend->pageTitle = "Counter Dashboard App";
    server->frontend = frontend;

    file->server = std::move(server);

    // Test AST Printer
    ASTPrinter printer;
    std::string dump = printer.print(*file);
    std::cout << dump << std::endl;

    assert(dump.find("SourceFile: CounterDashboard") != std::string::npos);
    assert(dump.find("ServerBlock") != std::string::npos);
    assert(dump.find("Database: db(\"app_data.db\")") != std::string::npos);
    assert(dump.find("Table: users") != std::string::npos);
    assert(dump.find("Column: id: int primary_key auto_increment") != std::string::npos);
    assert(dump.find("BackendServer: api (port: 8000, threads: 4, 1 routes)") != std::string::npos);
    assert(dump.find("Route: GET \"/api/users\" => (req)") != std::string::npos);
    assert(dump.find("FrontendServer: web (port: 8080") != std::string::npos);
    assert(dump.find("import cpp from \"services/Analytics.h\" as Analytics") != std::string::npos || dump.find("ImportCpp: \"services/Analytics.h\" as Analytics") != std::string::npos);
    assert(dump.find("bind:value={query}") != std::string::npos);
    assert(dump.find("onClick={() => count += 1}") != std::string::npos);
    assert(dump.find("If: count > 10") != std::string::npos);
    assert(dump.find("Else:") != std::string::npos);
    assert(dump.find("padding: 1.5rem;") != std::string::npos);

    // Test Parser on a complete .keet source containing <server>
    std::string fullSource = R"(
<server>
  database db("todos.db") {
    table todos {
      id: int primary_key auto_increment,
      title: string not_null unique,
      completed: bool default false
    };
  };

  backend api(port: 8081, threads: 4) {
    get "/api/todos" => (req) {
      return db.table("todos").find();
    };
    post "/api/todos" => (req) {
      return db.table("todos").insert(req.json());
    };
  };

  frontend web(port: 8082, api: "http://localhost:8081", title: "Todos App");
</server>

<script>
  state count: int = 0;
</script>

<template>
  <div>
    <h1>Full Stack App</h1>
  </div>
</template>
)";

    Lexer lexer(fullSource, "TestFullStack.keet");
    auto tokens = lexer.tokenize();
    Parser parser(tokens, "TestFullStack");
    auto parsedFile = parser.parse();

    assert(parsedFile->server != nullptr);
    assert(parsedFile->server->database != nullptr);
    assert(parsedFile->server->database->filePath == "todos.db");
    assert(parsedFile->server->database->tables.size() == 1);
    assert(parsedFile->server->database->tables[0]->name == "todos");
    assert(parsedFile->server->database->tables[0]->columns.size() == 3);
    assert(parsedFile->server->database->tables[0]->columns[0]->isPrimaryKey);
    assert(parsedFile->server->database->tables[0]->columns[1]->isNotNull);
    assert(parsedFile->server->database->tables[0]->columns[1]->isUnique);
    assert(parsedFile->server->database->tables[0]->columns[2]->defaultValueExpr == "false");
    assert(parsedFile->server->backend != nullptr);
    assert(parsedFile->server->backend->port == 8081);
    assert(parsedFile->server->backend->routes.size() == 2);
    assert(parsedFile->server->backend->routes[0]->method == "GET");
    assert(parsedFile->server->backend->routes[1]->method == "POST");
    assert(parsedFile->server->frontend != nullptr);
    assert(parsedFile->server->frontend->port == 8082);
    assert(parsedFile->server->frontend->pageTitle == "Todos App");

    std::cout << "Parser verification of <server> block passed! 🐱" << std::endl;

    // Test Standalone KeetScript pure module lexing (without XML tags)
    std::string pureModuleSource = R"(
fn add(a: int, b: int): int {
    return a + b;
}

export class Foo {
    public let c: int;
    private let d: int = 4;

    Foo(a: int, b: int) {
        this.c = a + b;
        print(subtract());
    }

    private fn subtract(): int {
        return this.c - this.d;
    }
}

fn main(): void {
    let c: int = add(4, 5);
    print("Hello, World! 4 + 5 =", c);
}
)";

    Lexer moduleLexer(pureModuleSource, "PureModule.keet");
    auto modTokens = moduleLexer.tokenize();
    assert(!modTokens.empty());
    bool foundFn = false, foundExport = false, foundClass = false;
    bool foundPublic = false, foundPrivate = false, foundThis = false, foundPrint = false;
    for (const auto& tok : modTokens) {
        if (tok.is(TokenKind::KwFn)) foundFn = true;
        if (tok.is(TokenKind::KwExport)) foundExport = true;
        if (tok.is(TokenKind::KwClass)) foundClass = true;
        if (tok.is(TokenKind::KwPublic)) foundPublic = true;
        if (tok.is(TokenKind::KwPrivate)) foundPrivate = true;
        if (tok.is(TokenKind::KwThis)) foundThis = true;
        if (tok.is(TokenKind::KwPrint)) foundPrint = true;
    }
    assert(foundFn && foundExport && foundClass && foundPublic && foundPrivate && foundThis && foundPrint);
    std::cout << "Standalone KeetScript pure module tokenization passed! 🐱" << std::endl;

    // Test Standalone KeetScript PureModule AST and ASTPrinter
    auto pureFile = std::make_unique<SourceFile>("MathModule");
    pureFile->isPureModule = true;
    auto mod = std::make_unique<PureModule>();
    mod->moduleName = "Math";

    auto addFn = std::make_shared<StandaloneFunctionDecl>();
    addFn->name = "add";
    addFn->returnType = "int";
    addFn->isExported = true;
    addFn->parameters.push_back(ParameterDecl{.name = "a", .typeName = "int"});
    addFn->parameters.push_back(ParameterDecl{.name = "b", .typeName = "int"});
    addFn->body = "return a + b;";
    mod->functions.push_back(addFn);

    auto fooClass = std::make_shared<ClassDecl>();
    fooClass->name = "Foo";
    fooClass->isExported = true;

    auto fieldC = std::make_shared<ClassFieldDecl>();
    fieldC->name = "c";
    fieldC->typeName = "int";
    fieldC->visibility = Visibility::Public;
    fooClass->fields.push_back(fieldC);

    auto fieldD = std::make_shared<ClassFieldDecl>();
    fieldD->name = "d";
    fieldD->typeName = "int";
    fieldD->visibility = Visibility::Private;
    fieldD->defaultValueExpr = "4";
    fooClass->fields.push_back(fieldD);

    auto ctorFoo = std::make_shared<ConstructorDecl>();
    ctorFoo->name = "Foo";
    ctorFoo->parameters.push_back(ParameterDecl{.name = "a", .typeName = "int"});
    ctorFoo->parameters.push_back(ParameterDecl{.name = "b", .typeName = "int"});
    ctorFoo->body = "this->c = a + b;\nprint(subtract());";
    fooClass->constructors.push_back(ctorFoo);

    auto methodSub = std::make_shared<ClassMethodDecl>();
    methodSub->name = "subtract";
    methodSub->returnType = "int";
    methodSub->visibility = Visibility::Private;
    methodSub->body = "return this->c - this->d;";
    fooClass->methods.push_back(methodSub);

    mod->classes.push_back(fooClass);

    auto mainFn = std::make_shared<StandaloneFunctionDecl>();
    mainFn->name = "main";
    mainFn->returnType = "void";
    mainFn->isMain = true;
    mainFn->body = "let c: int = add(4, 5);\nprint(\"Hello, World! 4 + 5 =\", c);";
    mod->functions.push_back(mainFn);

    pureFile->moduleBlock = std::move(mod);

    std::string pureDump = printer.print(*pureFile);
    assert(pureDump.find("PureModule: Math") != std::string::npos);
    assert(pureDump.find("Fn: add(a: int, b: int): int") != std::string::npos);
    assert(pureDump.find("Class: Foo (2 fields, 1 ctors, 1 methods)") != std::string::npos);
    assert(pureDump.find("ClassField: public c: int") != std::string::npos);
    assert(pureDump.find("ClassField: private d: int = 4") != std::string::npos);
    assert(pureDump.find("Constructor: Foo(a: int, b: int)") != std::string::npos);
    assert(pureDump.find("ClassMethod: private fn subtract(): int") != std::string::npos);
    assert(pureDump.find("Fn: main(): void [main]") != std::string::npos);

    std::cout << "Standalone KeetScript PureModule AST & ASTPrinter passed! 🐱" << std::endl;

    // Test Parser on pure KeetScript standalone module
    std::string standaloneCode = R"(
module MathUtils;

import cpp from "iostream" as io;

fn add(a: int, b: int): int {
    return a + b;
}

export class Foo {
    public let c: int;
    private let d: int = 4;

    Foo(a: int, b: int) {
        this.c = a + b;
        print(subtract());
    }

    private fn subtract(): int {
        return this.c - this.d;
    }
}

export struct Point {
    public let x: int;
    public let y: int = 0;
}

fn main(): void {
    let c: int = add(4, 5);
    print("Hello, World! 4 + 5 =", c);
}
)";

    Lexer standLexer(standaloneCode, "MathUtils.keet");
    auto standTokens = standLexer.tokenize();
    Parser standParser(standTokens, "MathUtils");
    auto parsedStand = standParser.parse();

    assert(parsedStand->isPureModule);
    assert(parsedStand->moduleBlock != nullptr);
    assert(parsedStand->moduleBlock->moduleName == "MathUtils");

    // Verify imports
    assert(parsedStand->moduleBlock->imports.size() == 1);
    assert(parsedStand->moduleBlock->imports[0]->headerPath == "iostream");
    assert(parsedStand->moduleBlock->imports[0]->asAlias == "io");

    // Verify standalone functions
    assert(parsedStand->moduleBlock->functions.size() == 2);
    auto pAdd = parsedStand->moduleBlock->functions[0];
    assert(pAdd->name == "add");
    assert(pAdd->returnType == "int");
    assert(!pAdd->isMain);
    assert(pAdd->parameters.size() == 2);
    assert(pAdd->parameters[0].name == "a" && pAdd->parameters[0].typeName == "int");
    assert(pAdd->parameters[1].name == "b" && pAdd->parameters[1].typeName == "int");
    assert(pAdd->body.find("return a + b;") != std::string::npos);

    auto pMain = parsedStand->moduleBlock->functions[1];
    assert(pMain->name == "main");
    assert(pMain->returnType == "void");
    assert(pMain->isMain);
    assert(pMain->body.find("int c = add(4, 5);") != std::string::npos);
    assert(pMain->body.find("keet::io::print(\"Hello, World! 4 + 5 =\", c);") != std::string::npos);

    // Verify classes
    assert(parsedStand->moduleBlock->classes.size() == 1);
    auto pFoo = parsedStand->moduleBlock->classes[0];
    assert(pFoo->name == "Foo");
    assert(pFoo->isExported);
    assert(pFoo->fields.size() == 2);
    assert(pFoo->fields[0]->name == "c" && pFoo->fields[0]->visibility == Visibility::Public);
    assert(pFoo->fields[1]->name == "d" && pFoo->fields[1]->visibility == Visibility::Private && pFoo->fields[1]->defaultValueExpr == "4");

    assert(pFoo->constructors.size() == 1);
    auto pCtor = pFoo->constructors[0];
    assert(pCtor->name == "Foo");
    assert(pCtor->parameters.size() == 2);
    assert(pCtor->body.find("this->c = a + b;") != std::string::npos);
    assert(pCtor->body.find("keet::io::print(subtract());") != std::string::npos);

    assert(pFoo->methods.size() == 1);
    auto pMethod = pFoo->methods[0];
    assert(pMethod->name == "subtract");
    assert(pMethod->returnType == "int");
    assert(pMethod->visibility == Visibility::Private);
    assert(pMethod->body.find("return this->c - this->d;") != std::string::npos);

    // Verify structs
    assert(parsedStand->moduleBlock->structs.size() == 1);
    auto pPoint = parsedStand->moduleBlock->structs[0];
    assert(pPoint->name == "Point");
    assert(pPoint->isExported);
    assert(pPoint->fields.size() == 2);
    assert(pPoint->fields[0]->name == "x");
    assert(pPoint->fields[1]->name == "y" && pPoint->fields[1]->defaultValueExpr == "0");

    std::string parsedStandDump = printer.print(*parsedStand);
    assert(parsedStandDump.find("PureModule: MathUtils") != std::string::npos);
    assert(parsedStandDump.find("Class: Foo") != std::string::npos);
    assert(parsedStandDump.find("Struct: Point") != std::string::npos);
    std::cout << "Standalone KeetScript Parser verification passed! 🐱" << std::endl;

    // Test Codegen on standalone pure KeetScript module (Phase 3)
    Codegen standCodegen(*parsedStand, "MathUtils.h");
    GeneratedCode standGen = standCodegen.generate();

    std::cout << "--- Generated Header ---\n" << standGen.headerCode << std::endl;
    std::cout << "--- Generated Source ---\n" << standGen.sourceCode << std::endl;

    // Header assertions
    assert(standGen.headerCode.find("#pragma once") != std::string::npos);
    assert(standGen.headerCode.find("#include <keet/runtime/IO.h>") != std::string::npos);
    assert(standGen.headerCode.find("#include <keet/runtime/Math.h>") != std::string::npos);
    assert(standGen.headerCode.find("#include <keet/runtime/Collections.h>") != std::string::npos);
    assert(standGen.headerCode.find("namespace MathUtils {") != std::string::npos);
    assert(standGen.headerCode.find("class Foo {") != std::string::npos);
    assert(standGen.headerCode.find("public:") != std::string::npos);
    assert(standGen.headerCode.find("int c = {};") != std::string::npos);
    assert(standGen.headerCode.find("Foo(int a, int b);") != std::string::npos);
    assert(standGen.headerCode.find("private:") != std::string::npos);
    assert(standGen.headerCode.find("int d = 4;") != std::string::npos);
    assert(standGen.headerCode.find("int subtract();") != std::string::npos);
    assert(standGen.headerCode.find("struct Point {") != std::string::npos);
    assert(standGen.headerCode.find("int x = {};") != std::string::npos);
    assert(standGen.headerCode.find("int y = 0;") != std::string::npos);
    assert(standGen.headerCode.find("int add(int a, int b);") != std::string::npos);
    assert(standGen.headerCode.find("using namespace MathUtils;") != std::string::npos);

    // Source assertions
    assert(standGen.sourceCode.find("#include \"MathUtils.h\"") != std::string::npos);
    assert(standGen.sourceCode.find("Foo::Foo(int a, int b) {") != std::string::npos);
    assert(standGen.sourceCode.find("this->c = a + b;") != std::string::npos);
    assert(standGen.sourceCode.find("keet::io::print(subtract());") != std::string::npos);
    assert(standGen.sourceCode.find("int Foo::subtract() {") != std::string::npos);
    assert(standGen.sourceCode.find("return this->c - this->d;") != std::string::npos);
    assert(standGen.sourceCode.find("int add(int a, int b) {") != std::string::npos);
    assert(standGen.sourceCode.find("return a + b;") != std::string::npos);
    assert(standGen.sourceCode.find("int main(int argc, char** argv) {") != std::string::npos);
    assert(standGen.sourceCode.find("int c = add(4, 5);") != std::string::npos);
    assert(standGen.sourceCode.find("keet::io::print(\"Hello, World! 4 + 5 =\", c);") != std::string::npos);
    assert(standGen.sourceCode.find("return 0;") != std::string::npos);

    std::cout << "Standalone KeetScript C++20 Codegen verification passed! 🐱" << std::endl;

    // =========================================================================
    // Feature 9: Phase 1 - Borrowing & Ownership Grammar & Diagnostic Tests
    // =========================================================================
    std::string borrowCode = R"(
module MemorySafety;

fn readBuffer(buf: &Buffer): int {
    return buf.size();
}

fn mutateBuffer(buf: &mut Buffer, val: string): void {
    buf.append(val);
}

fn takeBuffer(move buf: Buffer): Buffer {
    return move buf;
}

fn main(): void {
    let mut b: Buffer = Buffer{};
    {
        let r: &Buffer = &b;
    }
    {
        let rmut: &mut Buffer = &mut b;
    }
    let movedB = move b;
}
)";

    Lexer borrowLexer(borrowCode, "MemorySafety.keet");
    auto borrowTokens = borrowLexer.tokenize();
    
    // Check tokens
    bool foundAmpersand = false;
    bool foundKwMut = false;
    bool foundKwMove = false;
    for (const auto& tok : borrowTokens) {
        if (tok.is(TokenKind::Ampersand)) foundAmpersand = true;
        if (tok.is(TokenKind::KwMut)) foundKwMut = true;
        if (tok.is(TokenKind::KwMove)) foundKwMove = true;
    }
    assert(foundAmpersand);
    assert(foundKwMut);
    assert(foundKwMove);

    Parser borrowParser(borrowTokens, "MemorySafety");
    auto parsedBorrow = borrowParser.parse();
    assert(parsedBorrow->isPureModule);
    assert(parsedBorrow->moduleBlock->functions.size() == 4);

    auto fRead = parsedBorrow->moduleBlock->functions[0];
    assert(fRead->name == "readBuffer");
    assert(fRead->parameters.size() == 1);
    assert(fRead->parameters[0].name == "buf");
    assert(fRead->parameters[0].refKind == ReferenceKind::ImmutableRef);

    auto fMut = parsedBorrow->moduleBlock->functions[1];
    assert(fMut->name == "mutateBuffer");
    assert(fMut->parameters.size() == 2);
    assert(fMut->parameters[0].name == "buf");
    assert(fMut->parameters[0].refKind == ReferenceKind::MutableRef);

    auto fTake = parsedBorrow->moduleBlock->functions[2];
    assert(fTake->name == "takeBuffer");
    assert(fTake->parameters.size() == 1);
    assert(fTake->parameters[0].isMove);

    // Verify Codegen emits proper C++ references & std::move
    Codegen borrowCodegen(*parsedBorrow, "MemorySafety.h");
    GeneratedCode borrowGen = borrowCodegen.generate();
    assert(borrowGen.headerCode.find("int readBuffer(const keet::io::BufferView& buf);") != std::string::npos);
    assert(borrowGen.headerCode.find("void mutateBuffer(keet::io::BufferView& buf, std::string val);") != std::string::npos);
    assert(borrowGen.sourceCode.find("std::move(b)") != std::string::npos);

    // Test TypeAnnotation
    auto immAnn = TypeAnnotation::parse("&Buffer");
    assert(immAnn.isImmutableRef());
    assert(immAnn.baseType == "Buffer");

    auto mutAnn = TypeAnnotation::parse("&mut string");
    assert(mutAnn.isMutableRef());
    assert(mutAnn.baseType == "string");

    auto lifeAnn = TypeAnnotation::parse("&'a mut Buffer");
    assert(lifeAnn.isMutableRef());
    assert(lifeAnn.lifetime == "'a");
    assert(lifeAnn.baseType == "Buffer");

    // Test Diagnostic Formatter
    SourceLocation diagLoc{"test.keet", 2, 5};
    Diagnostic diag(ErrorCode::E0501_UseOfMovedValue, "use of moved value 'buf'", diagLoc, 3, "value used here after move");
    diag.withHelp("value was moved earlier. Consider borrowing instead: '&buf'");
    std::string diagFormatted = diag.format("let a = 1;\n    buf.size();\n");
    assert(diagFormatted.find("error[E0501]: use of moved value 'buf'") != std::string::npos);
    assert(diagFormatted.find("--> test.keet:2:5") != std::string::npos);
    assert(diagFormatted.find("^^^ value used here after move") != std::string::npos);
    assert(diagFormatted.find("help: value was moved earlier. Consider borrowing instead: '&buf'") != std::string::npos);

    std::cout << "Feature 9 Phase 1 (Tokens, AST, References, Moves & Diagnostics) verification passed! 🐱" << std::endl;

    // =========================================================================
    // Feature 9: Phase 2 - Compile-Time Borrow Checker Engine Tests
    // =========================================================================
    BorrowChecker checker;
    bool checkedOk = checker.check(*parsedBorrow, borrowCode);
    assert(checkedOk);
    assert(checker.getDiagnostics().empty());

    // Test E0501: Use of moved value
    std::string errCode1 = R"(
module ErrTest1;
fn broken(): void {
    let mut b: Buffer = Buffer{};
    let movedB = move b;
    b.size();
}
)";
    Lexer errLexer1(errCode1, "ErrTest1.keet");
    auto errTokens1 = errLexer1.tokenize();
    Parser errParser1(errTokens1, "ErrTest1");
    auto errAst1 = errParser1.parse();
    BorrowChecker checker1;
    bool checked1 = checker1.check(*errAst1, errCode1);
    assert(!checked1);
    assert(checker1.getDiagnostics().size() == 1);
    assert(checker1.getDiagnostics()[0].code == ErrorCode::E0501_UseOfMovedValue);

    // Test E0502: Borrow conflict (mutable borrow while immutably borrowed)
    std::string errCode2 = R"(
module ErrTest2;
fn broken(): void {
    let mut b: Buffer = Buffer{};
    let r: &Buffer = &b;
    let rmut: &mut Buffer = &mut b;
}
)";
    Lexer errLexer2(errCode2, "ErrTest2.keet");
    auto errTokens2 = errLexer2.tokenize();
    Parser errParser2(errTokens2, "ErrTest2");
    auto errAst2 = errParser2.parse();
    BorrowChecker checker2;
    bool checked2 = checker2.check(*errAst2, errCode2);
    assert(!checked2);
    assert(checker2.getDiagnostics().size() == 1);
    assert(checker2.getDiagnostics()[0].code == ErrorCode::E0502_BorrowConflict);

    // Test E0503: Multiple mutable borrows
    std::string errCode3 = R"(
module ErrTest3;
fn broken(): void {
    let mut b: Buffer = Buffer{};
    let r1: &mut Buffer = &mut b;
    let r2: &mut Buffer = &mut b;
}
)";
    Lexer errLexer3(errCode3, "ErrTest3.keet");
    auto errTokens3 = errLexer3.tokenize();
    Parser errParser3(errTokens3, "ErrTest3");
    auto errAst3 = errParser3.parse();
    BorrowChecker checker3;
    bool checked3 = checker3.check(*errAst3, errCode3);
    assert(!checked3);
    assert(checker3.getDiagnostics().size() == 1);
    assert(checker3.getDiagnostics()[0].code == ErrorCode::E0503_MultipleMutableBorrows);

    // Test E0504: Mutation during borrow
    std::string errCode4 = R"(
module ErrTest4;
fn broken(): void {
    let mut b: Buffer = Buffer{};
    let r: &Buffer = &b;
    b = Buffer{};
}
)";
    Lexer errLexer4(errCode4, "ErrTest4.keet");
    auto errTokens4 = errLexer4.tokenize();
    Parser errParser4(errTokens4, "ErrTest4");
    auto errAst4 = errParser4.parse();
    BorrowChecker checker4;
    bool checked4 = checker4.check(*errAst4, errCode4);
    assert(!checked4);
    assert(checker4.getDiagnostics().size() == 1);
    assert(checker4.getDiagnostics()[0].code == ErrorCode::E0504_MutationDuringBorrow);

    // Test E0505: Dangling reference to local
    std::string errCode5 = R"(
module ErrTest5;
fn broken(): &Buffer {
    let b: Buffer = Buffer{};
    return &b;
}
)";
    Lexer errLexer5(errCode5, "ErrTest5.keet");
    auto errTokens5 = errLexer5.tokenize();
    Parser errParser5(errTokens5, "ErrTest5");
    auto errAst5 = errParser5.parse();
    BorrowChecker checker5;
    bool checked5 = checker5.check(*errAst5, errCode5);
    assert(!checked5);
    assert(checker5.getDiagnostics().size() == 1);
    assert(checker5.getDiagnostics()[0].code == ErrorCode::E0505_DanglingReference);

    // Test E0506: Move out of borrowed reference
    std::string errCode6 = R"(
module ErrTest6;
fn broken(b: &Buffer): void {
    let x = move b;
}
)";
    Lexer errLexer6(errCode6, "ErrTest6.keet");
    auto errTokens6 = errLexer6.tokenize();
    Parser errParser6(errTokens6, "ErrTest6");
    auto errAst6 = errParser6.parse();
    BorrowChecker checker6;
    bool checked6 = checker6.check(*errAst6, errCode6);
    assert(!checked6);
    assert(checker6.getDiagnostics().size() == 1);
    assert(checker6.getDiagnostics()[0].code == ErrorCode::E0506_CannotMoveBorrowed);

    std::cout << "Feature 9 Phase 2 (BorrowChecker Engine & All 6 Memory Safety Rules) verification passed! 🐱" << std::endl;

    // =========================================================================
    // Feature 9: Phase 3 - K-IR Intermediate Representation Verification
    // =========================================================================
    KIRBuilder kirBuilder;
    auto kirMod = kirBuilder.build(*parsedBorrow);
    assert(kirMod != nullptr);
    assert(kirMod->name == "MemorySafety");
    assert(kirMod->functions.size() == 4);

    // Verify K-IR Text Printer
    std::string kirText = kirMod->print();
    assert(kirText.find("; K-IR Module: MemorySafety") != std::string::npos);
    assert(kirText.find("fn readBuffer(") != std::string::npos);
    assert(kirText.find("fn mutateBuffer(") != std::string::npos);
    assert(kirText.find("fn takeBuffer(") != std::string::npos);
    assert(kirText.find("fn main(): void") != std::string::npos);
    assert(kirText.find("entry:") != std::string::npos);

    // Verify K-IR Binary Serialization Round-Trip
    auto kirBinary = kirMod->toBinary();
    assert(!kirBinary.empty());
    assert(kirBinary.size() > 4);
    assert(kirBinary[0] == 'K' && kirBinary[1] == 'I' && kirBinary[2] == 'R' && kirBinary[3] == 0x01);

    auto restoredKirMod = KIRModule::fromBinary(kirBinary);
    assert(restoredKirMod != nullptr);
    assert(restoredKirMod->name == kirMod->name);
    assert(restoredKirMod->functions.size() == kirMod->functions.size());
    assert(restoredKirMod->print() == kirMod->print());

    std::cout << "Feature 9 Phase 3 (K-IR Intermediate Representation & Serialization) verification passed! 🐱" << std::endl;

    // =========================================================================
    // Feature 9: Phase 4 - Incremental Dependency Graph & Disk Cache Tests
    // =========================================================================
    std::filesystem::path phase4CacheDir = ".test_ast_phase4_cache";
    IncrementalCache phase4Cache(phase4CacheDir);
    phase4Cache.clear();

    // Verify HashUtil
    std::string testContent = "module Phase4Demo;\nfn run(): void {}\n";
    std::string testHash = HashUtil::hashString(testContent);
    assert(!testHash.empty());
    assert(testHash == HashUtil::hashString(testContent));

    // Verify Dependency Graph
    std::filesystem::path fakeSourcePath = ".test_ast_phase4_demo.keet";
    {
        std::ofstream out(fakeSourcePath);
        out << testContent;
    }
    std::string fakeSource = fakeSourcePath.string();
    assert(!phase4Cache.isFresh(fakeSource));

    // Save KIR to cache
    bool p4Saved = phase4Cache.saveKIR(fakeSource, *kirMod);
    assert(p4Saved);
    assert(phase4Cache.getStats().bytesSaved > 0);

    // Should now be fresh
    assert(phase4Cache.isFresh(fakeSource));

    // Load from cache
    auto loadedP4KIR = phase4Cache.loadKIR(fakeSource);
    assert(loadedP4KIR != nullptr);
    assert(loadedP4KIR->name == kirMod->name);
    assert(loadedP4KIR->functions.size() == kirMod->functions.size());
    assert(loadedP4KIR->print() == kirMod->print());
    assert(phase4Cache.getStats().cacheHits == 1);

    // Invalidation & Clear
    phase4Cache.invalidate(fakeSource);
    assert(!phase4Cache.isFresh(fakeSource));
    phase4Cache.clear();
    std::filesystem::remove(fakeSourcePath);
    std::filesystem::remove_all(phase4CacheDir);

    std::cout << "Feature 9 Phase 4 (Incremental Dependency Graph & Disk Cache) verification passed! 🐱" << std::endl;

    // =========================================================================
    // Feature 9: Phase 5 - C++20 Code Generation from Validated AST / K-IR
    // =========================================================================
    std::string phase5Source = R"(
module Phase5Codegen;

export struct DataPacket {
    id: int,
    payload: string
}

fn processPacket(move packet: DataPacket): string {
    return move packet.payload;
}

fn inspectPacket(packet: &DataPacket): int {
    return packet.id;
}

fn mutatePacket(packet: &mut DataPacket, newId: int): void {
    packet.id = newId;
}

fn main(): void {
    let mut pkt = DataPacket { id: 1, payload: "Hello" };
    {
        let r = &pkt;
    }
    {
        let rmut = &mut pkt;
    }
    let movedPkt = move pkt;
}
)";

    Lexer p5Lexer(phase5Source, "Phase5Codegen.keet");
    auto p5Tokens = p5Lexer.tokenize();
    Parser p5Parser(p5Tokens, "Phase5Codegen");
    auto p5Ast = p5Parser.parse();

    BorrowChecker p5Checker;
    bool p5SafetyOk = p5Checker.check(*p5Ast, phase5Source);
    assert(p5SafetyOk);

    Codegen p5Codegen(*p5Ast, "Phase5Codegen.h");
    GeneratedCode p5Gen = p5Codegen.generate();

    // Verify 5.1 & 5.2 Header declarations (reference mappings & value ownership)
    assert(p5Gen.headerCode.find("std::string processPacket(DataPacket packet);") != std::string::npos);
    assert(p5Gen.headerCode.find("int inspectPacket(const DataPacket& packet);") != std::string::npos);
    assert(p5Gen.headerCode.find("void mutatePacket(DataPacket& packet, int newId);") != std::string::npos);

    // Verify 5.1 & 5.2 Source implementation (move semantics & auto reference bindings)
    assert(p5Gen.sourceCode.find("std::move(packet.payload)") != std::string::npos);
    assert(p5Gen.sourceCode.find("const auto& r = pkt;") != std::string::npos);
    assert(p5Gen.sourceCode.find("auto& rmut = pkt;") != std::string::npos);
    assert(p5Gen.sourceCode.find("auto movedPkt = std::move(pkt);") != std::string::npos);

    std::cout << "Feature 9 Phase 5 (C++20 Code Generation from Validated Pipeline) verification passed! 🐱" << std::endl;

    std::cout << "All Phase 1, Phase 2, Phase 3, Phase 4, & Phase 5 AST, Parser, Codegen & Pipeline assertions passed successfully! 🐱" << std::endl;
    return 0;
}
