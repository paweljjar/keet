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
      title: string,
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
    assert(parsedFile->server->backend != nullptr);
    assert(parsedFile->server->backend->port == 8081);
    assert(parsedFile->server->backend->routes.size() == 2);
    assert(parsedFile->server->backend->routes[0]->method == "GET");
    assert(parsedFile->server->backend->routes[1]->method == "POST");
    assert(parsedFile->server->frontend != nullptr);
    assert(parsedFile->server->frontend->port == 8082);
    assert(parsedFile->server->frontend->pageTitle == "Todos App");

    std::cout << "Parser verification of <server> block passed! 🐱" << std::endl;
    std::cout << "All Phase 1 AST assertions passed successfully! 🐱" << std::endl;
    return 0;
}
