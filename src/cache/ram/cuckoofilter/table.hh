#pragma once
#include "bitsutil.hh"
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <cstring>

#include <iostream>

namespace kv_store::memory::cuckoofilter {
    struct Item {
        uint32_t tag;
        uint32_t size;
        uint16_t nb_gets;
    };

    template <size_t bits_per_tag>
        class Table {
            static const size_t kTagsPerBucket = 4;

            // Size of a bucket in bytes
            // We add seven to ensure a minimum of 1 byte per bucket
            // and right shift by 3 to convert bits into bytes
            //
            // Each bucket contains a set of tags and their 
            // metadata (value size and number of access since 
            // insertion)
            static const size_t kBytesPerBuckets = 
                ((bits_per_tag + sizeof(uint32_t) + sizeof(uint16_t)) * kTagsPerBucket + 7) >> 3;

            // Create a mask of size bits_per_tag, full of 1 (because of the '- 1' ) 
            static const uint32_t kTagMask = (1ULL << bits_per_tag) - 1;

            static const size_t kPaddingBuckets = 
                ((((kBytesPerBuckets + 7) / 8) * 8) - 1) / kBytesPerBuckets;

            // Each bucket is a table of chars
            struct Bucket {
                char content[kBytesPerBuckets];
                uint32_t sizes[kTagsPerBucket];
                uint16_t nb_gets[kTagsPerBucket];
            } __attribute__((__packed__)); 

            public:
            explicit Table(const size_t nb_buckets): _nb_buckets(nb_buckets) {
                this->_buckets = new Bucket[nb_buckets + kPaddingBuckets];
                memset(this->_buckets, 0, kBytesPerBuckets * (nb_buckets + kPaddingBuckets));
            }
            ~Table() { 
                delete[] this->_buckets;
            }

            Table(Table& other) = delete;
            void operator=(Table& other) = delete;

            /// Return the number of buckets contained in
            /// the table 
            size_t nbBuckets() const{
                return this->_nb_buckets;
            }

            /// Return the amount of memory occupied by the table
            size_t sizeInBytes() const {
                return kBytesPerBuckets * this->_nb_buckets;
            }

            size_t sizeInTags() const {
                return kTagsPerBucket * this->_nb_buckets;
            }

            /// Return the number of tags the table can contains
            size_t capacity() const {
                return kTagsPerBucket * this->_nb_buckets;
            }

            // /!\ Item is an out parameter
            void readAt(const size_t i, const size_t j, Item& item, bool update = false) const {
                const char *p = this->_buckets[i].content;
                uint32_t tag; 

                /* following code only works for little-endian */
                if (bits_per_tag == 2) {
                    tag = *((uint8_t *)p) >> (j * 2);
                } else if (bits_per_tag == 4) {
                    p += (j >> 1);
                    tag = *((uint8_t *)p) >> ((j & 1) << 2);
                } else if (bits_per_tag == 8) {
                    p += j;
                    tag = *((uint8_t *)p);
                } else if (bits_per_tag == 12) {
                    p += j + (j >> 1);
                    tag = *((uint16_t *)p) >> ((j & 1) << 2);
                } else if (bits_per_tag == 16) {
                    p += (j << 1);
                    tag = *((uint16_t *)p);
                } else if (bits_per_tag == 32) {
                    tag = ((uint32_t *)p)[j];
                }
                tag &= kTagMask;

                item.tag = tag;
                item.size = this->_buckets[i].sizes[j];
                if (update) this->_buckets[i].nb_gets[j]++;
                item.nb_gets = this->_buckets[i].nb_gets[j];
            }

            void writeAt(const size_t i, const size_t j, const Item item) {
                char *p = this->_buckets[i].content;
                uint32_t tag = item.tag & kTagMask;

                /* following code only works for little-endian */
                if (bits_per_tag == 2) {
                    *((uint8_t *)p) |= tag << (2 * j);
                } else if (bits_per_tag == 4) {
                    p += (j >> 1);
                    if ((j & 1) == 0) {
                        *((uint8_t *)p) &= 0xf0;
                        *((uint8_t *)p) |= tag;
                    } else {
                        *((uint8_t *)p) &= 0x0f;
                        *((uint8_t *)p) |= (tag << 4);
                    }
                } else if (bits_per_tag == 8) {
                    ((uint8_t *)p)[j] = tag;
                } else if (bits_per_tag == 12) {
                    p += (j + (j >> 1));
                    if ((j & 1) == 0) {
                        ((uint16_t *)p)[0] &= 0xf000;
                        ((uint16_t *)p)[0] |= tag;
                    } else {
                        ((uint16_t *)p)[0] &= 0x000f;
                        ((uint16_t *)p)[0] |= (tag << 4);
                    }
                } else if (bits_per_tag == 16) {
                    ((uint16_t *)p)[j] = tag;
                } else if (bits_per_tag == 32) {
                    ((uint32_t *)p)[j] = tag;
                }

                this->_buckets[i].sizes[j] = item.size;
                this->_buckets[i].nb_gets[j] = item.nb_gets;
            }

            // /!\ Item is an out parameter
            bool findInBuckets(const size_t i1, const size_t i2, const uint32_t tag, Item& item) {
                item = {0, 0, 0};

                const char *p1 = this->_buckets[i1].content;
                const char *p2 = this->_buckets[i2].content;

                uint64_t v1 = *((uint64_t *)p1);
                uint64_t v2 = *((uint64_t *)p2);

                // caution: unaligned access & assuming little endian
                /*if (bits_per_tag == 4 && kTagsPerBucket == 4) {
                    return hasvalue4(v1, tag) || hasvalue4(v2, tag);
                } else if (bits_per_tag == 8 && kTagsPerBucket == 4) {
                    return hasvalue8(v1, tag) || hasvalue8(v2, tag);
                } else if (bits_per_tag == 12 && kTagsPerBucket == 4) {
                    return hasvalue12(v1, tag) || hasvalue12(v2, tag);
                } else if (bits_per_tag == 16 && kTagsPerBucket == 4) {
                    return hasvalue16(v1, tag) || hasvalue16(v2, tag);
                } else {*/
                    for (size_t j = 0; j < kTagsPerBucket; j++) {

                        this->readAt(i1, j, item, true);
                        if (item.tag == tag) return true;

                        this->readAt(i2, j, item, true);
                        if (item.tag == tag) return true;
                    }
                    return false;
                //}
            }

            // /!\ Item is an out parameter
            bool findInBucket(const size_t i, const uint32_t tag, Item& item) {
                // caution: unaligned access & assuming little endian
                /*if (bits_per_tag == 4 && kTagsPerBucket == 4) {
                    const char *p = this->_buckets[i].content;
                    uint64_t v = *(uint64_t *)p;  // uint16_t may suffice
                    return hasvalue4(v, tag);
                } else if (bits_per_tag == 8 && kTagsPerBucket == 4) {
                    const char *p = this->_buckets[i].content;
                    uint64_t v = *(uint64_t *)p;  // uint32_t may suffice
                    return hasvalue8(v, tag);
                } else if (bits_per_tag == 12 && kTagsPerBucket == 4) {
                    const char *p = this->_buckets[i].content;
                    uint64_t v = *(uint64_t *)p;
                    return hasvalue12(v, tag);
                } else if (bits_per_tag == 16 && kTagsPerBucket == 4) {
                    const char *p = this->_buckets[i].content;
                    uint64_t v = *(uint64_t *)p;
                    return hasvalue16(v, tag);
                } else {*/
                    for (size_t j = 0; j < kTagsPerBucket; j++) {
                        this->readAt(i, j, item);
                        if (item.tag == tag) return true;
                    }
                    return false;
               // }
            }


            bool deleteFromBucket(const size_t i, const uint32_t tag, Item& deletedItem) {
                for (size_t j = 0; j < kTagsPerBucket; j++) {
                    this->readAt(i, j, deletedItem);
                    if (deletedItem.tag == tag) {
                        this->writeAt(i, j, Item {0, 0, 0});
                        return true;
                    }
                }

                return false;
            }

            bool insertToBucket(const size_t i, const Item &item,
                    const bool kickout, Item &old) {
                Item tmp_item = {0, 0, 0};
                for (size_t j = 0; j < kTagsPerBucket; j++) {
                    // If space is available, use it
                    this->readAt(i, j, tmp_item);
                    if (tmp_item.tag == 0) {
                        this->writeAt(i, j, item);
                        return true;
                    }
                }

                if (kickout) {
                    size_t r = rand() % kTagsPerBucket;
                    this->readAt(i, r, old);
                    this->writeAt(i, r, item);
                }

                return false;
            }

            size_t numTagsInBucket(const size_t i) const {
                Item item;
                size_t num = 0;
                for (size_t j = 0; j < kTagsPerBucket; j++) {
                    item = {0, 0, 0};
                    this->readAt(i, j, item);
                    if (item.tag != 0) {
                        num++;
                    }
                }

                return num;
            }

            private:
            Bucket * _buckets;
            size_t _nb_buckets;
        };
}
