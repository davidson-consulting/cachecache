#include <gtest/gtest.h>
#include <cache/ram/cuckoofilter/wss_estimator.hh>
#include <cache/common/_.hh>

using namespace kv_store::memory::wss;
using namespace kv_store::memory::cuckoofilter;
using namespace kv_store::common;


TEST(WSSEstimator, distribution) {
  WSSEstimator estimator(1000);

  Key k;
  k.set("Coucou");

  estimator.insert(k, 100);
  EXPECT_EQ(estimator.size(), 100);
  EXPECT_EQ(estimator.wss(), 0);
 
  estimator.find(k);
  EXPECT_EQ(estimator.size(), 100);
  EXPECT_EQ(estimator.wss(), 100);

  /*std::array<int, 3> expected = {1, 0, 0};
  EXPECT_EQ(filter.distribution(), expected);

  Item item;
  filter.find(k, item);
  expected = {0, 1, 0};
  EXPECT_EQ(filter.distribution(), expected);

  for (int i = 0; i < 10; i++) {
    filter.find(k, item);
  }

  expected = {0, 0, 1};
  EXPECT_EQ(filter.distribution(), expected);*/
}
