#define LOG_LEVEL 10

#include "history.hh"

#include <algorithm>
#include <rd_utils/utils/log.hh>

using namespace cachecache::supervisor;
using namespace rd_utils::utils;
using namespace std;

History::History():
  _size(10)
  , _cursor(0) {}

History::History(int size):
  _size(size)
  , _cursor(0) {}

void History::add(MemorySize value) {
  if (this->_history.size() < this->_size) {
    this->_history.push_back(value);
  } else {
    this->_history[this->_cursor] = value;
  }
  this->_cursor = (this->_cursor + 1) % this->_size;
}

// formula from https://www.cuemath.com/data/regression-coefficients/
HistoryTrend History::trend(float min_coeff) {
  float sum_xy = 0;
  float sum_x = 0;
  float sum_y = 0;
  float sum_x_squared = 0;

  // history can not be filled yet
  int size = min(this->_size, (int)this->_history.size());
  for (int i = 0; i < size; i++) {
    int current = (this->_cursor + i) % size;
    float x = i;
    float y = (float)this->_history[current].bytes();

    sum_xy += x * y;
    sum_x += x;
    sum_y += y;
    sum_x_squared += x * x;
  }

  float nominator = size * sum_xy - sum_x * sum_y;
  float denominator = size * sum_x_squared - (sum_x * sum_x);
  if (denominator == 0) return HistoryTrend::STEADY;

  float coef = nominator / denominator;
  
  LOG_DEBUG("COEF IS ", coef, " MIN COEF IS ", min_coeff);
  if (coef >= min_coeff) {
    return HistoryTrend::INCREASING;
  }
  if (coef <= min_coeff * -1) {
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
