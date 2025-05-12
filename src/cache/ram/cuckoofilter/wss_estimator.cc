#include <cassert>
#include <iostream>
#include "wss_estimator.hh"

using namespace kv_store::memory::wss;
using namespace kv_store::memory::cuckoofilter;

WSSEstimator::WSSEstimator(size_t max_nb_keys):
    _filter(max_nb_keys) {}

void WSSEstimator::insert(const common::Key &key, uint32_t value_size) {
    Item inserted;
    Status status = this->_filter.insertKey(key, value_size, inserted);
    if (status == Status::Ok) {
        this->_distribution.insert(inserted);
    }
}

void WSSEstimator::find(const common::Key &key) {
    Item item;
    Status status = this->_filter.find(key, item);
    if (status == Status::Ok) {
        this->_distribution.update(item);
    }
}

void WSSEstimator::del(const common::Key &key) {
    Item item;
    Status status = this->_filter.del(key, item);
    if (status == Status::Ok) {
        this->_distribution.remove(item);
    }
}

uint32_t WSSEstimator::wss() const {
    uint32_t wss = 0;
    for (int i = 1; i < this->_distribution.nb_buckets(); i++) {
        wss += this->_distribution.size_of(i);
    }
    
    return wss;
}

uint32_t WSSEstimator::size() const {
    uint32_t size = 0;
    for (int i = 0; i < this->_distribution.nb_buckets(); i++) {
        size += this->_distribution.size_of(i);
    }

    return size;
}

//
//  DISTRIBUTION
//

constexpr int WSSEstimator::Distribution::nb_buckets() const {
    return this->_get_stages.size();
}


int WSSEstimator::Distribution::nb_in(int i) const {
    assert(i < this->nb_buckets());
    return this->_distribution[i];
}

int WSSEstimator::Distribution::size_of(int i) const {
    assert(i < this->nb_buckets());
    return this->_sizes[i];
}

int WSSEstimator::Distribution::threshold_of(int i) const {
    assert(i < this->nb_buckets());
    return this->_get_stages[i];
}

void WSSEstimator::Distribution::insert(const Item& item) {
    this->add_to_bucket(0, item); 
}

void WSSEstimator::Distribution::update(const Item& item) {
   for (int i = this->_get_stages.size() - 1; i > 0; i--) {
        // if nbGets strictly superior than threshold for current
        // bucket, it means it has not changed bucket
        if (item.nb_gets > this->_get_stages[i]) break;

        // item just has changed bucket
        if (item.nb_gets == this->_get_stages[i]) {
            this->move(i - 1, i, item);
            return;
        }
    } 
}

void WSSEstimator::Distribution::remove(const Item& item) {
    for (int i = this->_get_stages.size() - 1; i > 0; i--) {
        if (this->_get_stages[i] >= item.nb_gets) {
            this->remove_from_bucket(i, item); 
        }
    }
}

void WSSEstimator::Distribution::move(int from, int to, const Item& item) {
    this->remove_from_bucket(from, item);
    this->add_to_bucket(to, item);
}

void WSSEstimator::Distribution::add_to_bucket(int i, const Item& item) {
    this->_distribution[i] += 1;
    this->_sizes[i] += item.size;
}

void WSSEstimator::Distribution::remove_from_bucket(int i, const Item& item) {
    assert(this->_distribution[i] >= 1);
    assert(this->_sizes[i] >= item.size);

    this->_distribution[i] -= 1;
    this->_sizes[i] -= item.size;
}
