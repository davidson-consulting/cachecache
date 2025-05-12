#pragma once
#include <cstdint>
#include "table.hh"
#include "../../common/key.hh"
#include "hashutil.h"
#include "bitsutil.hh"
#include <iostream>
#include <array>

namespace kv_store::memory::cuckoofilter {
    enum Status {
        Ok = 0,
        NotFound = 1,
        NotEnoughSpace = 2,
        NotSupported = 3,
    };

    const size_t kMaxCuckooCount = 500;

    template <size_t bits_per_item>
        class CuckooFilter {
            public:
                CuckooFilter(size_t max_nb_keys) {
                    size_t assoc = 4;
                    size_t nb_buckets = upperpower2(std::max<uint64_t>(1, max_nb_keys / assoc));
                    double frac = (double)max_nb_keys / nb_buckets / assoc;
                    this->_nb_items = 0;

                    if (frac > 0.96) {
                        nb_buckets <<= 1;
                    }

                    this->_victim.used = false;
                    this->_table = new Table<bits_per_item>(nb_buckets);
                }

            ~CuckooFilter() {
                delete this->_table;
            }

            Status insertKey(const common::Key &key, uint32_t value_size, Item& inserted){
                size_t bucketIndex;
                uint32_t tag;

                if (this->_victim.used) {
                    return Status::NotEnoughSpace;
                }

                this->generateIndexTagHash(key, &bucketIndex, &tag);
                inserted.tag = tag;
                inserted.size = value_size;
                inserted.nb_gets = 0;
                return this->addToBucket(bucketIndex, inserted);
            }


            Status find(const common::Key &key, Item& item){
                bool found = false;
                size_t i1, i2;
                uint32_t tag;

                this->generateIndexTagHash(key, &i1, &tag);
                i2 = this->altIndex(i1, tag);

                assert(i1 == this->altIndex(i2, tag));

                found = this->_victim.used && (tag == this->_victim.item.tag) &&
                    (i1 == this->_victim.index || i2 == this->_victim.index);

                if (found) {
                    item.tag = this->_victim.item.tag;
                    item.size = this->_victim.item.size;
                    item.nb_gets = this->_victim.item.nb_gets;
                    return Status::Ok;
                } else if (this->_table->findInBuckets(i1, i2, tag, item)) {
                    return Status::Ok;
                }
                
                return Status::NotFound;
            }


            Status del(const common::Key &key, Item& deletedItem) {
                size_t i1, i2;
                uint32_t tag;
                this->generateIndexTagHash(key, &i1, &tag);
                i2 = this->altIndex(i1, tag);

                if (this->_table->deleteFromBucket(i1, tag, deletedItem) || this->_table->deleteFromBucket(i2, tag, deletedItem)) {
                    this->_nb_items--;
                    this->tryEliminateVictim();
                    return Status::Ok;
                } else if (this->_victim.used && tag == this->_victim.item.tag &&
                        (i1 == this->_victim.index || i2 == this->_victim.index)) {
                    this->_victim.used = false;
                    deletedItem.tag = this->_victim.item.tag;
                    deletedItem.size = this->_victim.item.size;
                    deletedItem.nb_gets = this->_victim.item.nb_gets;
                    return Status::Ok;
                }

                return Status::Ok;
            }

            size_t size() const {
                return this->_nb_items;
            };

        private:
            typedef struct {
                size_t index;
                Item item;
                bool used;
            } VictimCache;

            VictimCache _victim;

            Table<bits_per_item> *_table;
            size_t _nb_items;

            Status addToBucket(const size_t bucketIndex, 
                    const Item &item) {
                size_t curindex = bucketIndex;
                Item curItem = item;
                Item oldItem;

                for (uint32_t count = 0; count < kMaxCuckooCount; count++) {
                    bool kickout = count > 0;
                    oldItem = Item {0,0,0};
                    if (this->_table->insertToBucket(curindex, curItem, kickout, oldItem)) {
                        this->_nb_items++;
                        return Status::Ok;
                    }

                    if (kickout) {
                        curItem = oldItem;
                    }

                    curindex = this->altIndex(curindex, curItem.tag);
                }

                this->_victim.index = curindex;
                this->_victim.item = curItem;
                this->_victim.used = true;

                return Status::Ok;
            }


            Status tryEliminateVictim() {
                if (this->_victim.used) {
                    this->_victim.used = false;
                    size_t i = this->_victim.index;
                    this->addToBucket(i, this->_victim.item);
                }
                return Status::Ok;
            }

            /*
             * UTILS
             */ 

            size_t indexHash(uint32_t hv) const {
                return hv & (this->_table->nbBuckets() - 1);
            }

            uint32_t tagHash(uint32_t hv) const {
                uint32_t tag = hv & ((1ULL << bits_per_item) - 1);
                tag += (tag == 0);
                return tag;
            }
            
            void generateIndexTagHash(const common::Key &key, size_t* index,
                    uint32_t* tag) const {

                const uint64_t hash = key.hash();
                *index = this->indexHash(hash);
                *tag = this->tagHash(hash);
            }

            size_t altIndex(const size_t index, const uint32_t tag) const {
                return this->indexHash((uint32_t)(index ^ (tag * 0x5bd1e995)));
            }

            double loadFactor() const {
                return 1.0 * this->size() / this->_table->sizeInTags();
            } 

    };
}
