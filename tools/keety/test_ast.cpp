//
// Created by PAWEL on 19/10/2026.
// Phase 1 AST and Grammar Verification Test
//

#include <iostream>
#include <cassert>
#include "Token.h"
#include "AST.h"
#include "ASTPrinter.h"
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
    assert(standGen.headerCode.find("namespace keet::io") != std::string::npos);
    assert(standGen.headerCode.find("inline void print") != std::string::npos);
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

    std::cout << "All Phase 1, Phase 2, & Phase 3 AST, Parser & Codegen assertions passed successfully! 🐱" << std::endl;
    return 0;
}
