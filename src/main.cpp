#include "stay_awake/application.hpp"
#include "stay_awake/platform_factory.hpp"

int main()
{
    auto platform = stay_awake::CreatePlatformBinding();
    stay_awake::Application application(*platform);
    return application.Run();
}
