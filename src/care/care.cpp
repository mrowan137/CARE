//////////////////////////////////////////////////////////////////////////////////////
// Copyright 2020 Lawrence Livermore National Security, LLC and other CARE developers.
// See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: BSD-3-Clause
//////////////////////////////////////////////////////////////////////////////////////

// CARE config header
#include "care/config.h"

// Other CARE headers
#include "care/Setup.h"

// Other library headers
#include "chai/ArrayManager.hpp"
#include "umpire/Allocator.hpp"
#include "umpire/ResourceManager.hpp"
#include "umpire/strategy/QuickPool.hpp"


namespace care {

  ///
  /// @brief Initializes a pool using umpire's default strategy
  ///
   void initialize_pool(
      const std::string& resource, ///< The name of the umpire resource this pool will be built on
      const std::string& poolname, ///< The (application specific) name of the pool to be created
      chai::ExecutionSpace space,  ///< The CHAI Execution space associated with this pool
      std::size_t initial_size,    ///< The initial size in bytes
      std::size_t min_block_size,  ///< The minimum block size in bytes
      bool /* grows */)
   {
#ifndef CHAI_DISABLE_RM
      auto& rm = umpire::ResourceManager::getInstance();

      auto allocator = rm.getAllocator(resource);

      auto pooled_allocator =
         rm.makeAllocator<umpire::strategy::QuickPool>(poolname,
                                                       allocator,
                                                       initial_size, /* default = 512Mb*/
                                                       min_block_size); /* default = 1Mb */

      chai::ArrayManager * am = chai::ArrayManager::getInstance();
      am->setAllocator(space, pooled_allocator);
#endif
   }
  ///
  /// @brief Initializes a pool using a block heuristic
  ///
   void initialize_pool_block_heuristic(
      const std::string& resource, ///< The name of the umpire resource this pool will be built on
      const std::string& poolname, ///< The (application specific) name of the pool to be created
      chai::ExecutionSpace space,  ///< The CHAI Execution space associated with this pool
      std::size_t initial_size,    ///< The initial size in bytes
      std::size_t min_block_size,  ///< The minimum block size in bytes
      std::size_t block_coalesce_heuristic, ///< The number of blocks that should be releasable to trigger coalescing
      bool /* grows */)
   {
#ifndef CHAI_DISABLE_RM
      auto& rm = umpire::ResourceManager::getInstance();

      auto allocator = rm.getAllocator(resource);

      auto pooled_allocator =
         rm.makeAllocator<umpire::strategy::QuickPool>(poolname,
                                                       allocator,
                                                       initial_size, /* default = 512Mb*/
                                                       min_block_size, /* default = 1Mb */
                                                       16, /* default alignment */
                                                       umpire::strategy::QuickPool::blocks_releasable(block_coalesce_heuristic));

      chai::ArrayManager * am = chai::ArrayManager::getInstance();
      am->setAllocator(space, pooled_allocator);
#endif
   }

  ///
  /// @brief Initializes a pool using a percent heuristic
  ///
   void initialize_pool_percent_heuristic(
      const std::string& resource, ///< The name of the umpire resource this pool will be built on
      const std::string& poolname, ///< The (application specific) name of the pool to be created
      chai::ExecutionSpace space,  ///< The CHAI Execution space associated with this pool
      std::size_t initial_size,    ///< The initial size in bytes
      std::size_t min_block_size,  ///< The minimum block size in bytes
      std::size_t percent_coalesce_heuristic, ///< The percentage of blocks that should be releasable to trigger coalescing
      bool /* grows */)
   {
#ifndef CHAI_DISABLE_RM
      auto& rm = umpire::ResourceManager::getInstance();

      auto allocator = rm.getAllocator(resource);

      auto pooled_allocator =
         rm.makeAllocator<umpire::strategy::QuickPool>(poolname,
                                                       allocator,
                                                       initial_size, /* default = 512Mb*/
                                                       min_block_size, /* default = 1Mb */
                                                       16, /* default alignment */
                                                       umpire::strategy::QuickPool::percent_releasable(percent_coalesce_heuristic));

      chai::ArrayManager * am = chai::ArrayManager::getInstance();
      am->setAllocator(space, pooled_allocator);
#endif
   }

   void dump_memory_statistics() {
      auto& resourceManager = umpire::ResourceManager::getInstance();
      chai::ArrayManager* arrayManager = chai::ArrayManager::getInstance();

      for (int space = chai::ExecutionSpace::CPU; space < chai::ExecutionSpace::NUM_EXECUTION_SPACES; ++space) {
         chai::ExecutionSpace executionSpace = (chai::ExecutionSpace) space;
         auto allocatorId = arrayManager->getAllocatorId(executionSpace);
         auto allocator = resourceManager.getAllocator(allocatorId);

         printf("\n");
         printf("Execution space: %s\n", allocator.getName().c_str());
         printf("Currently used:      %lu bytes\n", allocator.getCurrentSize());
         printf("Currently allocated: %lu bytes\n", allocator.getActualSize());
         printf("High watermark:      %lu bytes\n", allocator.getHighWatermark());
      }
   }

   bool syncIfNeeded() {
#ifndef CHAI_DISABLE_RM
      return chai::ArrayManager::getInstance()->syncIfNeeded();
#else
      return false;
#endif
   }
}

#ifndef CARE_DISABLE_RAJAPLUGIN
#if defined(_WIN32) && !defined(CARESTATICLIB)
#ifdef CARE_EXPORTS

#include "RAJA/util/PluginStrategy.hpp"
RAJA_INSTANTIATE_REGISTRY(RAJA::util::PluginRegistry);

namespace RAJA
{
	namespace util
	{
		PluginStrategy::PluginStrategy() = default;
	}
}  // namespace RAJA

#endif
#endif
#endif

