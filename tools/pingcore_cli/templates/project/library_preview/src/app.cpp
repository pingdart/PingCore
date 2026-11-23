#include <pingcore/pingcore.h>
#include <@LIBRARY_PRIMARY_HEADER@>

using namespace pingcore;

View App() {
  return Text("@LIBRARY_PROJECT_NAME@ preview");
}

const Application application{
    App,
    {
        .window = {
            .title = "@PROJECT_NAME@",
        },
        .application_hooks = {@LIBRARY_NAMESPACE@::Install},
    }
};
