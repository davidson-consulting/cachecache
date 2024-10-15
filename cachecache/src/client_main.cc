#include <cachecache/client/service.hh>

using namespace cachecache::client;

int main (int argc, char ** argv) {
  CacheClient cl (argv [1], atoi (argv[2]));

  int v;
  scanf ("%d", &v);
  if (v == 0) {
    std::cout <<  cl.get ("salutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalutsalut90078978789") << std::endl;
  } else {
    cl.set ("salut", "test");
  }
}
