#include "generator.hh"

#define LOG_LEVEL 4

#include <cstdlib>
#include <rd_utils/utils/log.hh>

using namespace cachecache_sim;
using namespace std;
using namespace rd_utils::utils;

Generator::Generator(): _generator(time(nullptr)), _distribution(0, this->CHARACTERS.size() - 1) {
  srand((unsigned int)time(NULL));
}

Generator::Generator(GeneratorConfiguration& cfg):
  _generator(time(nullptr)),
  _distribution(0, this->CHARACTERS.size() - 1) {
  srand((unsigned int)time(NULL));

  this->_percentage_get = cfg.percentage_get;
  this->_percentage_unused_keys = cfg.percentage_unused_keys;
  this->_theorical_hit_ratio = cfg.theorical_hit_ratio;
  this->_size_keys = cfg.size_keys;
  this->_size_values = cfg.size_values;

  LOG_INFO(
      "Generator configured with values \% gets ", this->_percentage_get,
      " \% unused keys ", this->_percentage_unused_keys,
      " hit ratio ", this->_theorical_hit_ratio,
      " size keys ", this->_size_keys,
      " size values ", this->_size_values);
}

Generator::~Generator() {
  LOG_DEBUG("Done." 
      , " Theorical Hit ", this->_theorical_hits
      , " Miss ", this->_theorical_miss
      , " Gets ", this->_nb_gets
      , " Reqs ", this->_nb_reqs);
}

Trace Generator::next_operation() {
  if (this->_nb_reqs == 0 || ((float)this->_nb_gets / (float) (this->_nb_reqs)) > this->_percentage_get) {
    this->_nb_reqs++;
    return this->next_set();
  }

  this->_nb_reqs++;
  this->_nb_gets++;
  return this->next_get();
}

Trace Generator::next_get() {
  if ((float)this->_theorical_hits / (float)(this->_theorical_hits + this->_theorical_miss) < this->_theorical_hit_ratio) {
    this->_theorical_miss++;
    return Trace {
      .operation = CACHE_OPERATION::GET, 
      .key = this->gen_key(this->_size_keys), 
      .value_size = this->_size_values
    };
  }

  this->_theorical_hits++;
  return Trace {
    .operation = CACHE_OPERATION::GET, 
      .key = this->_keys[rand() % this->_keys.size()], 
      .value_size = this->_size_values
  };
}

Trace Generator::next_set() {
  string key = this->gen_key(this->_size_keys);

  this->_nb_generated_keys++;

  if ((float)this->_keys.size() / (float)this->_nb_generated_keys <= this->_percentage_unused_keys) {
    this->_keys.push_back(key);   
  }

  return Trace {.operation = CACHE_OPERATION::SET, .key = key, .value_size = this->_size_values};
}

string Generator::gen_key(int size) {
    string res;
    res.reserve(size);
        
    for (int i = 0; i < size; ++i) {
        res.push_back(CHARACTERS[this->_distribution(this->_generator)]);
    }
    return res;
}
