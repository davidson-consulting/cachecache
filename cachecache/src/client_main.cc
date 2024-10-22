#include <cachecache/client/service.hh>

using namespace cachecache::client;

int main (int argc, char ** argv) {
  CacheClient cl (argv [1], atoi (argv[2]));

  int v;
  scanf ("%d", &v);
  if (v == 0) {
    std::string val;
    if (cl.get ("salut", val)) {
      std::cout << val << std::endl;
    } else {
      std::cout << "Not found" << std::endl;
    }
  } else {
    if (cl.set ("salut", "test")) {
      std::cout << "success" << std::endl;
    } else {
      std::cout << "failure" << std::endl;
    }
  }
}
