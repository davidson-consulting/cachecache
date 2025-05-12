#include <gtest/gtest.h>
#include <cache/ram/cuckoofilter/_.hh>
#include <cache/common/_.hh>

using namespace kv_store::memory::cuckoofilter;
using namespace kv_store::common;

// A key that has been inserted should be found
TEST(Cuckoofilter, InsertAndFind) {
  CuckooFilter<16> filter(1000);

  Key k;
  Item inserted;
  k.set("Coucou");
  filter.insertKey(k, 100, inserted);
  EXPECT_EQ(inserted.size, 100);

  Item i;

  EXPECT_EQ(filter.find(k, i), Status::Ok);
  EXPECT_EQ(filter.size(), 1);
  EXPECT_EQ(i.size, 100);
}

// A key that has not been inserted in a non-empty
// cache should not be found
TEST(Cuckoofilter, InsertAndFind2) {
  CuckooFilter<16> filter(1000);
  Key k;
  Item inserted;
  k.set("Coucou");
  filter.insertKey(k, 100, inserted);

  Key k2;
  k2.set("foo");

  Item i;
  EXPECT_EQ(filter.find(k2, i), Status::NotFound);
}

// A key inserted and read twice should have nb_gets 
// at 2
TEST(Cuckoofilter, ProperGetUpdate) {
  CuckooFilter<16> filter(1000);
  Key k;
  Item inserted;
  k.set("Coucou");
  filter.insertKey(k, 100, inserted);

  Item i;

  EXPECT_EQ(filter.find(k, i), Status::Ok);
  EXPECT_EQ(i.nb_gets, 1);
  EXPECT_EQ(filter.find(k, i), Status::Ok);
  EXPECT_EQ(i.nb_gets, 2);
}

// It should be possible to delete a key that 
// has been inserted. The key shouldn't be found
// after deletion.
TEST(Cuckoofilter, DelAndFind) {
  CuckooFilter<16> filter(1000);
  Key k;
  k.set("Coucou");

  Item inserted;
  filter.insertKey(k, 100, inserted);

  Item deleted;
  EXPECT_EQ(filter.del(k, deleted), Status::Ok);

  Item i;
  EXPECT_EQ(filter.find(k, i), Status::NotFound);
  EXPECT_EQ(filter.size(), 0);
}

// A key that has not been inserted should not be found.
TEST(CuckooFilter, NotFound) {
  CuckooFilter<16> filter(1000);
  Key k;
  k.set("Coucou");

  Item i;
  EXPECT_EQ(filter.find(k, i), Status::NotFound);
  EXPECT_EQ(filter.size(), 0);
}
