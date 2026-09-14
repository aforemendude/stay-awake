#include "stay_awake/platform_factory.hpp"

#include "application/fake_platform_binding.hpp"

#include <gtest/gtest.h>

// tests/CMakeLists.txt renames only the test compilation of src/main.cpp.
int StayAwakeMain();

namespace stay_awake
{
namespace
{
std::function<std::unique_ptr<PlatformBinding>()> platform_factory;

class MainTest : public testing::Test
{
  protected:
    std::vector<std::string> calls;
    std::vector<std::string> errors;
    std::weak_ptr<int> binding_lifetime;
    OperationResult service_result;
    std::vector<ApplicationEvent> service_events{{EventKind::initialized}, {EventKind::quit}};

    void SetUp() override
    {
        platform_factory = [this] {
            calls.emplace_back("create");
            auto binding = std::make_unique<FakePlatformBinding>();
            binding->service_result = service_result;
            binding->service_events = service_events;
            auto lifetime = std::make_shared<int>(0);
            binding_lifetime = lifetime;
            binding->on_service = [this, lifetime] { calls.emplace_back("run"); };
            binding->on_error = [this, observed = binding.get()] { errors = observed->errors; };
            return binding;
        };
    }

    void TearDown() override
    {
        platform_factory = {};
    }
};

TEST_F(MainTest, CreatesRunsAndDestroysTheBindingOnSuccess)
{
    EXPECT_EQ(StayAwakeMain(), 0);
    EXPECT_EQ(calls, (std::vector<std::string>{"create", "run"}));
    EXPECT_TRUE(errors.empty());
    EXPECT_TRUE(binding_lifetime.expired());
}

TEST_F(MainTest, ReturnsFailureAndDestroysTheBindingAfterReportingTheServiceError)
{
    service_result = {false, "service startup failed"};
    service_events.clear();
    EXPECT_EQ(StayAwakeMain(), 1);
    EXPECT_EQ(calls, (std::vector<std::string>{"create", "run"}));
    EXPECT_EQ(errors, (std::vector<std::string>{"service startup failed"}));
    EXPECT_TRUE(binding_lifetime.expired());
}

TEST_F(MainTest, ReturnsSuccessForASecondaryLaunchWithoutInitialization)
{
    service_events.clear();
    EXPECT_EQ(StayAwakeMain(), 0);
    EXPECT_EQ(calls, (std::vector<std::string>{"create", "run"}));
    EXPECT_TRUE(errors.empty());
    EXPECT_TRUE(binding_lifetime.expired());
}
} // namespace

std::unique_ptr<PlatformBinding> CreatePlatformBinding()
{
    return platform_factory();
}
} // namespace stay_awake
