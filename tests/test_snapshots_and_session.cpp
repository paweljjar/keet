//
// Created by PAWEL on 20/10/2026.
//

#include <keet/Testing.h>
#include <keet/UIEngine.h>

#include <filesystem>

using namespace UIEngine;
using namespace UIEngine::Tags;

class UserDashboardComponent : public Component {
    public:
        [[nodiscard]] std::string getComponentName() const override { return "UserDashboardComponent"; }

        [[nodiscard]] VNode render() const override {
            auto user = useState<std::string>("user", "guest");
            auto role = useState<std::string>("role", "viewer");

            return div({{"id", "dashboard"}, {"class", "container"}}, {
                header({{"class", "dash-header"}}, {
                    h1({{"class", "greeting"}}, "Welcome, " + user.get()),
                    span({{"class", "role-badge"}}, role.get())
                }),
                main({{"class", "dash-content"}}, {
                    p({{"id", "dash-path"}}, "Route: " + currentPath()),
                    p({{"id", "dash-locale"}}, "Locale: " + currentLocale())
                })
            });
        }
};

KEET_TEST(MockSession, InitialStateAndMutation) {
    keet::Testing::MountOptions options{
        .path = "/dashboard",
        .locale = "fr",
        .initialState = {
            {"user", std::string("alice")},
            {"role", std::string("admin")},
            {"csrf_token", std::string("secret_token_abc")}
        }
    };

    auto mounted = keet::Testing::TestHarness::mount<UserDashboardComponent>(options);

    KEET_ASSERT_EQ(mounted.sessionId().empty(), false);
    KEET_ASSERT_TRUE(mounted.hasState("user"));
    KEET_ASSERT_TRUE(mounted.hasState("csrf_token"));
    KEET_ASSERT_FALSE(mounted.hasState("unknown_key"));

    KEET_ASSERT_EQ(mounted.getState<std::string>("user"), "alice");
    KEET_ASSERT_EQ(mounted.getState<std::string>("role"), "admin");
    KEET_ASSERT_EQ(mounted.getState<std::string>("csrf_token"), "secret_token_abc");

    KEET_ASSERT_TEXT(mounted.find("h1.greeting"), "Welcome, alice");
    KEET_ASSERT_TEXT(mounted.find(".role-badge"), "admin");
    KEET_ASSERT_TEXT(mounted.find("#dash-path"), "Route: /dashboard");
    KEET_ASSERT_TEXT(mounted.find("#dash-locale"), "Locale: fr");

    // Mutate state in session
    mounted.setState<std::string>("user", "charlie");
    KEET_ASSERT_EQ(mounted.getState<std::string>("user"), "charlie");
    KEET_ASSERT_TEXT(mounted.find("h1.greeting"), "Welcome, charlie");
}

KEET_TEST(SnapshotTesting, MatchesAndUpdatesSnapshot) {
    auto mounted = keet::Testing::TestHarness::mount([] {
        return div({{"id", "card-123"}, {"class", "card active"}}, {
            h2({{"class", "card-title"}}, "System Status"),
            p({{"class", "status-ok"}}, "All systems operational"),
            button({{"class", "btn btn-primary"}}, "Refresh")
        });
    });

    // Test snapshot assertion on mounted component
    KEET_ASSERT_MATCHES_SNAPSHOT(mounted, "system_status_card");

    // Test snapshot assertion on single element
    auto title = mounted.find("h2.card-title");
    KEET_ASSERT_MATCHES_SNAPSHOT(title, "system_status_title");

    // Verify snapshot file was created
    std::filesystem::path snapFile = std::filesystem::path(__FILE__).parent_path() / "__snapshots__" / "system_status_card.snap";
    KEET_ASSERT_TRUE(std::filesystem::exists(snapFile));

    // Verify mismatch detection throws AssertionFailure
    KEET_ASSERT_THROWS(
        keet::Testing::Snapshot::assertMatchesSnapshot(
            "<div class=\"corrupted\">Mismatched Content</div>",
            "system_status_card",
            snapFile.parent_path().string(),
            __FILE__,
            __LINE__
        ),
        keet::Testing::AssertionFailure
    );
}

KEET_TEST_MAIN()
