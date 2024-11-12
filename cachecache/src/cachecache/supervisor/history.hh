#pragma once 

#include <rd_utils/utils/mem_size.hh>

#include <vector>

namespace cachecache::supervisor {
  enum class HistoryTrend {
    INCREASING,
    DECREASING,
    STEADY,
  };

  /**
   * A fixed-size history of cache memory usage
   */
  class History {
    public:
      /**
       * Init the history with provided size
       * @params:
       *  - size: the maximum number of values stored
       *  in the history
       */
      History(int size);

      /**
       * Init the history with a default size
       */ 
      History();

      /**
       * Add a value to the history. If history 
       * is empty, the oldest value is removed
       * (behaving in a FIFO manner)
       */
      void add(rd_utils::utils::MemorySize value);

      /**
       * Estimate if current history shows any trend
       * and which trend
       * @params: 
       * - min_coeff: minimum (absolute) value for the computed 
       *   regression coefficient to detect an increase 
       *   or decrease
       */
      HistoryTrend trend(float min_coeff);

      // return last stored value
      rd_utils::utils::MemorySize current() const;

      // return second to last value stored
      rd_utils::utils::MemorySize previous() const;
      
    private:
      // The max history size
      int _size;
      // Index of next value to insert
      int _cursor;

      std::vector<rd_utils::utils::MemorySize> _history;
  };
}
