#pragma once

#define LOG_LEVEL 10

#include "ram/meta/_.hh"
#include "disk/meta.hh"
#include <rd_utils/concurrency/mutex.hh>
#include <rd_utils/utils/log.hh>
#include <map>
#include <memory>

namespace kv_store {

        class HybridKVStore {
        private:

                struct Promotion {
                        common::Key k;
                        common::Value v;
                };

        private:

                // The collection
                std::unique_ptr<memory::MetaRamCollection> _ramColl;

                // // The collection on the disk
                // disk::MetaDiskCollection _diskColl;

                // The routine time incrementation
                uint64_t _currentTime;

                // Ram collection contains slabs
                bool _hasRam = false;

                // The list of k/v to promote from disk to slab
                std::map <std::string, std::string> _promotions;

                // Mutex to lock promotion set
                rd_utils::concurrency::mutex _promoteMutex;
                rd_utils::concurrency::mutex _ramMutex;
                rd_utils::concurrency::mutex _diskMutex;

        public : 
                HybridKVStore (std::unique_ptr<memory::MetaRamCollection> ramColl)
                        : _ramColl(std::move(ramColl))
                        // , _diskColl (maxDiskSlab)
                        , _currentTime(0) 
                {
                        this-> _hasRam = (this-> _ramColl->getNbSlabs () != 0);
                        LOG_INFO ("KV Store configured with ", this-> _ramColl->getNbSlabs (), " slabs in RAM");
                        // LOG_INFO ("KV Store configured with ", this-> _diskColl.getMaxSlabs (), " maximum slabs on disk");
                };

                /**
                 * @params:
                 *    - maxRamSlabs: the number of slabs in the RAM
                 *    - maxDiskSlabs: the maximum number of slabs on disk
                 */
                static std::unique_ptr<HybridKVStore> TTLBased(uint32_t maxRamSlabs, uint32_t maxDiskSlabs, uint32_t slabTTL) {
                        std::unique_ptr<memory::MetaRamCollection> meta = std::make_unique<memory::TTLMetaRamCollection>(maxRamSlabs, slabTTL);
                        return std::make_unique<HybridKVStore>(std::move(meta));
                }

                static std::unique_ptr<HybridKVStore> WSSBased(uint32_t maxRamSlabs, uint32_t maxDiskSlabs, uint32_t slabTTL) {
                        std::unique_ptr<memory::MetaRamCollection> meta = std::make_unique<memory::WSSMetaRamCollection>(maxRamSlabs, slabTTL);
                        return std::make_unique<HybridKVStore>(std::move(meta));
                }

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * =================================          CONFIGURATION          ==================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Resize the ram collection
                 * @params:
                 *   - maxRamSlabs: the number of slabs in the ram collection
                 */
                void resizeRamColl (uint32_t maxRamSlabs);

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ===================================          COLLECTION          ===================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Insert a key value in the collection
                 */
                void insert (const common::Key & k, const common::Value & v);

                /**
                 * Find a value in the collection
                 */
                std::shared_ptr <common::Value> find (const common::Key & k, bool & onDisk);

                /**
                 * Remove a key from the collection
                 */
                void remove (const common::Key & k);


                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ====================================          GETTERS          =====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                // /**
                // * @returns: the disk collection
                // */
                // memory::MetaRamCollection& getRamColl ();

                // /**
                //  * @returns: the disk collection
                //  */
                // disk::MetaDiskCollection& getDiskColl ();

                // /**
                // * @returns: the disk collection
                // */
                // const memory::MetaRamCollection& getRamColl () const;

                // /**
                //  * @returns: the disk collection
                //  */
                // const disk::MetaDiskCollection& getDiskColl () const;

                /**
                 * @returns: the current memory usage of the ram collection
                 */
                rd_utils::utils::MemorySize getRamMemoryUsage () const;

                /**
                 * @returns: the current memory size of the ram collection
                 */
                rd_utils::utils::MemorySize getRamMemorySize () const;



                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ======================================          MISC          ======================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Update the collection to make the memory usage decrease if possible
                 */
                void loop ();

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ====================================          ROUTINE          =====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                friend std::ostream & operator<< (std::ostream & s, const HybridKVStore & mp);

        };

}
