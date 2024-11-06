#include <cachecache/client/service.hh>

using namespace cachecache::client;

void get(CacheClient& cl, const std::string key) {
  std::string val;
  if (cl.get (key, val)) {
    std::cout << val << std::endl;
  } else {
    std::cout << "Not found" << std::endl;
  }
}

void set(CacheClient& cl, const std::string key, const std::string value) {
  if (cl.set (key, value)) {
    std::cout << "success" << std::endl;
  } else {
    std::cout << "failure" << std::endl;
  }
}

void interactive(CacheClient& cl) {
  int v;
  scanf ("%d", &v);
  if (v == 0) {
    get(cl, "foo");   
  } else {
    set(cl, "foo", "bar"); 
  }
}

int main (int argc, char ** argv) {
  CacheClient cl (argv [1], atoi (argv[2]));

  // interactive mode
  // ./client $addr $port
  if (argc == 3) {
    interactive(cl);
  }

  // execute a set directly
  // ./client $addr $port $key $value
  if (argc == 5) {
    set(cl, argv[3], argv[4]);
  }

  // execute a get directly
  // ./client $addr $port $key
  if (argc == 4) {
    get(cl, argv[3]);
  }
}
