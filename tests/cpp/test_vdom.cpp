//
// Created by PAWEL on 20/10/2026.
//

#include <keet/Testing.h>
#include <keet/UIEngine.h>

using namespace UIEngine;
using namespace UIEngine::Tags;

class CounterComponent : public Component {
    public:
        explicit CounterComponent(Props props = {}) : Component(std::move(props)) {}

        [[nodiscard]] std::string getComponentName() const override { return "CounterComponent"; }

        [[nodiscard]] VNode render() const override {
            auto count = useState<int>("count", 0);
            auto textInput = useState<std::string>("text", "initial");

            return div({{"id", "counter-card"}, {"class", "card active"}}, {
                h1({{"class", "title"}}, "Counter App"),
                p({{"data-testid", "count-label"}, {"class:highlight", count.get() > 0}}, "Count: " + std::to_string(count.get())),
                button({
                    {"class", "btn btn-increment"},
                    {"data-testid", "inc-btn"},
                    {"onClick", [count] {
                        count.set(count.get() + 1);
                    }}
                }, "Increment"),
                input({
                    {"type", "text"},
                    {"id", "input-field"},
                    {"value", textInput.get()},
                    {"onInput", [textInput](const Event& ev) {
                        textInput.set(ev.value);
                    }}
                }),
                div({{"class", "items-container"}}, {
                    ul({}, {
                        li({{"class", "item first"}}, { VNode::textNode("First Item") }),
                        li({{"class", "item second"}}, { VNode::textNode("Second Item") })
                    })
                })
            });
        }
};

KEET_TEST(VDOMHarness, MountAndQueryHierarchy) {
    auto mounted = keet::Testing::TestHarness::mount<CounterComponent>();

    // Root assertions
    KEET_ASSERT_TRUE(mounted.contains("#counter-card"));
    KEET_ASSERT_TRUE(mounted.contains(".card"));
    KEET_ASSERT_TRUE(mounted.contains("h1.title"));

    auto card = mounted.find("#counter-card");
    KEET_ASSERT_EXISTS(card);
    KEET_ASSERT_HAS_CLASS(card, "card");
    KEET_ASSERT_HAS_CLASS(card, "active");
    KEET_ASSERT_ATTR(card, "id", "counter-card");

    auto title = mounted.find("h1.title");
    KEET_ASSERT_EXISTS(title);
    KEET_ASSERT_TEXT(title, "Counter App");
}

KEET_TEST(VDOMHarness, CSSCombinatorsAndAttributes) {
    auto mounted = keet::Testing::TestHarness::mount<CounterComponent>();

    // Child combinator
    auto items = mounted.findAll(".items-container > ul > li");
    KEET_ASSERT_EQ(items.size(), 2);
    KEET_ASSERT_TEXT(items[0], "First Item");
    KEET_ASSERT_TEXT(items[1], "Second Item");

    // Descendant combinator
    auto secondItem = mounted.find(".items-container li.second");
    KEET_ASSERT_EXISTS(secondItem);
    KEET_ASSERT_TEXT(secondItem, "Second Item");

    // Attribute selectors
    auto incBtn = mounted.find("button[data-testid='inc-btn']");
    KEET_ASSERT_EXISTS(incBtn);
    KEET_ASSERT_HAS_CLASS(incBtn, "btn-increment");
    KEET_ASSERT_TEXT(incBtn, "Increment");

    // Comma-separated selectors
    auto headingsAndButtons = mounted.findAll("h1, button");
    KEET_ASSERT_EQ(headingsAndButtons.size(), 2);

    // Non-existent selector
    auto missing = mounted.find(".does-not-exist");
    KEET_ASSERT_NOT_EXISTS(missing);
}

KEET_TEST(VDOMHarness, SynchronousEventDispatchAndRerender) {
    auto mounted = keet::Testing::TestHarness::mount<CounterComponent>();

    auto countLabel = mounted.find("p[data-testid='count-label']");
    KEET_ASSERT_EXISTS(countLabel);
    KEET_ASSERT_TEXT(countLabel, "Count: 0");
    KEET_ASSERT_FALSE(countLabel.hasClass("highlight"));

    auto incBtn = mounted.find("button.btn-increment");
    KEET_ASSERT_EXISTS(incBtn);

    // Trigger click on button
    incBtn.triggerClick();

    // Re-check count label
    auto updatedLabel = mounted.find("p[data-testid='count-label']");
    KEET_ASSERT_TEXT(updatedLabel, "Count: 1");
    KEET_ASSERT_TRUE(updatedLabel.hasClass("highlight"));

    // Trigger click again
    incBtn.triggerClick();
    KEET_ASSERT_TEXT(mounted.find("p[data-testid='count-label']"), "Count: 2");

    // Trigger input on text field
    auto inputField = mounted.find("#input-field");
    KEET_ASSERT_EXISTS(inputField);
    KEET_ASSERT_ATTR(inputField, "value", "initial");

    inputField.triggerInput("New Value");
    KEET_ASSERT_ATTR(mounted.find("#input-field"), "value", "New Value");
}

KEET_TEST(VDOMHarness, VNodeFactoryMountAndHtmlOutput) {
    auto mounted = keet::Testing::TestHarness::mount([] {
        return div({{"id", "simple"}, {"class", "box"}}, {
            span({{"class", "badge"}}, "Notice"),
            input({{"type", "checkbox"}, {"checked", true}})
        });
    });

    KEET_ASSERT_EXISTS(mounted.find("#simple"));
    KEET_ASSERT_TEXT(mounted.find(".badge"), "Notice");

    std::string html = mounted.html();
    KEET_ASSERT_TRUE(html.find("<div id=\"simple\" class=\"box\">") != std::string::npos);
    KEET_ASSERT_TRUE(html.find("<span class=\"badge\">Notice</span>") != std::string::npos);
    KEET_ASSERT_TRUE(html.find("<input checked type=\"checkbox\" />") != std::string::npos);
}

KEET_TEST(VDOMHarness, RouteNavigationAndLocaleInSession) {
    auto mounted = keet::Testing::TestHarness::mount([] {
        return div({}, {
            p({{"id", "path-display"}}, "Path: " + currentPath()),
            p({{"id", "locale-display"}}, "Locale: " + currentLocale())
        });
    });

    KEET_ASSERT_TEXT(mounted.find("#path-display"), "Path: /");
    KEET_ASSERT_TEXT(mounted.find("#locale-display"), "Locale: en");

    mounted.navigate("/settings");
    KEET_ASSERT_TEXT(mounted.find("#path-display"), "Path: /settings");

    mounted.setLocale("pl");
    KEET_ASSERT_TEXT(mounted.find("#locale-display"), "Locale: pl");
}

KEET_TEST_MAIN()
