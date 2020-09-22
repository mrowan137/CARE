//////////////////////////////////////////////////////////////////////////////////////
// Copyright 2020 Lawrence Livermore National Security, LLC and other CARE developers.
// See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: BSD-3-Clause
//////////////////////////////////////////////////////////////////////////////////////

#ifndef CARE_MANAGED_PTR_H
#define CARE_MANAGED_PTR_H

// CARE config header
#include "care/config.h"

#if defined(CARE_ENABLE_MANAGED_PTR)

// Other library headers
#include "chai/managed_ptr.hpp"

namespace care{
   template <typename T>
   using managed_ptr = chai::managed_ptr<T>;

   template <typename T,
             typename... Args>
   inline managed_ptr<T> make_managed(Args&&... args) {
      return chai::make_managed<T>(std::forward<Args>(args)...);
   }

   template <typename T,
             typename F,
             typename... Args>
   inline managed_ptr<T> make_managed_from_factory(F&& f, Args&&... args) {
      return chai::make_managed_from_factory<T>(std::forward<F>(f), std::forward<Args>(args)...);
   }
}

#else // defined(CARE_ENABLE_MANAGED_PTR)

#error "managed_ptr is disabled! Check build configuration options."

#endif // defined(CARE_ENABLE_MANAGED_PTR)

#endif // CARE_MANAGED_PTR_H
