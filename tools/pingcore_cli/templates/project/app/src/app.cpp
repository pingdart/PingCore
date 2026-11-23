#include <pingcore/pingcore.h>

using namespace pingcore;

View App() {
  return Text("Hello, PingCore");
}

const Application application{App, {.window = {.title = "@PROJECT_NAME@"}}};
