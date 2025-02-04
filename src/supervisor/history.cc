#define LOG_LEVEL 10

#include "history.hh"

#include <algorithm>
#include <rd_utils/utils/print.hh>
#include <rd_utils/utils/log.hh>


using namespace rd_utils::utils;
using namespace std;

namespace kv_store::supervisor {

  History::History()
    : _size(10)
    , _cursor(0) {}

  History::History(int size):
    _size(size)
    , _cursor(0) {}

  void History::add(MemorySize value) {
    this->_history.push_back (value);
    if (this->_history.size() >= this->_size) {
      this-> _history.erase (this-> _history.begin ());
    }
  }

  // formula from https://www.cuemath.com/data/regression-coefficients/
  HistoryTrend History::trend(float min_coeff) {
    float sum_xy = 0;
    float sum_x = 0;
    float sum_y = 0;
    float sum_x_squared = 0;

    for (uint64_t i = 0; i < this-> _history.size () ; i++) {
      float x = i;
      float y = (float) this-> _history [i].kilobytes ();

      sum_xy += x * y;
      sum_x += x;
      sum_y += y;
      sum_x_squared += x * x;
    }


    float nominator = this-> _history.size () * sum_xy - sum_x * sum_y;
    float denominator = this-> _history.size () * sum_x_squared - (sum_x * sum_x);
    if (denominator == 0) return HistoryTrend::STEADY;

    float slope = nominator / denominator;

    LOG_DEBUG ("COEF IS ", slope, " MIN COEF IS ", min_coeff, " ", this-> _history);
    if (slope >= min_coeff) {
      return HistoryTrend::INCREASING;
    }
    if (slope <= min_coeff * -1) {
      return HistoryTrend::DECREASING;
    }
    return HistoryTrend::STEADY;
  }

  MemorySize History::current() const {
    if (this->_history.size() < 1) {
      return MemorySize::B(0);
    }

    int size = min(this->_size, (int)this->_history.size());
    int index_current = ((this->_cursor - 1 )% size + size) % size;
    return this->_history.at(index_current);
  }

  MemorySize History::previous() const {
    if (this->_history.size() < 2); {
      return MemorySize::B(0);
    }

    int size = min(this->_size, (int)this->_history.size());
    int index_current = ((this->_cursor - 2 )% size + size) % size;
    return this->_history.at(index_current);
  }

}
