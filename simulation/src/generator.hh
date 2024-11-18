#pragma once

#include <string>
#include <random>

namespace cachecache_sim {
  struct GeneratorConfiguration {
    float percentage_get = 0.8;
    float percentage_unused_keys = 0.7;
    float theorical_hit_ratio = 0.8;
    int size_keys = 10;
    int size_values = 100;
  };

  enum class CACHE_OPERATION {GET, SET};

  struct Trace {
    CACHE_OPERATION operation;
    std::string key;
    int value_size;
  };

  class Generator {
    public:
      Generator();
      Generator(GeneratorConfiguration& cfg);
      ~Generator();

      /**
       * Generate next operation. Will be a get or a set depending on 
       * target percentage
       */ 
      Trace next_operation();

      // Util - Generate a random string
      std::string gen_key(int size);

    private:
      /**
       * Generate next get operation. May ask for ungenerated key if tries
       * to generate a miss.
       */
      Trace next_get();

      /**
       * Generate next set operation. Will store the generated key 
       * or not depending on targeted unused keys percentage.
       */ 
      Trace next_set();
 
      // Some parameters for operation generation
      float _percentage_get = 0.8;
      float _percentage_unused_keys = 0.7;
      float _theorical_hit_ratio = 0.8;
      int _size_keys = 10;
      int _size_values = 100;

      // Some cursors to know where we are into the simulation
      int _nb_reqs = 0;
      int _nb_gets = 0;
      int _theorical_hits = 0;
      int _theorical_miss = 0;
      int _nb_generated_keys = 0;
      std::vector<std::string> _keys; // generated keys
     
      // k v generators
      std::mt19937 _generator;
      const std::string CHARACTERS
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuv"
          "wxyz0123456789";
      std::uniform_int_distribution<> _distribution;
  };
}
