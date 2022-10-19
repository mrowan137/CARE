
#ifndef CHAI_Accessor__HPP
#define CHAI_Accessor__HPP

#include "care/config.h"
#include "care/RAJAPlugin.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <set>
#include <unordered_map>


namespace care {

template <typename T>
class DefaultAccessor {
   public:
   DefaultAccessor<T>() = default;
   DefaultAccessor<T>(size_t elems) {}
   
   template<typename Idx> inline CARE_HOST_DEVICE void operator[](const Idx i) const {}
   void set_size(size_t elems) {}
};

template <typename T>
void detectRaceCondition(T* data, T* prev_data, std::unordered_map<int, std::set<int>> * accesses, size_t len,
                         chai::ExecutionSpace space, const char * fileName, int lineNumber) {
   printf("Checking For Race Condition at %s: %i\n", fileName, lineNumber);
   for (size_t i = 0; i < len; ++i) {
      if (!(data[i] == prev_data[i])) {
         // then a write occurred
         if ((*accesses)[i].size() > 1) {
            printf("RACE CONDITION DETECTED, loop in execution space %i, fileName: %s, lineNumber %i\n", (int) space, fileName, lineNumber);
            printf("DATA %p at index %zu changed and accessed by following threads\n\t", data, i);
            for (auto const & x : (*accesses)[i]) {
               printf("%i,",x);
            }
            printf("\n");
         }
      }
   }
   delete [] prev_data;
   delete accesses;
}

template <typename T>
class RaceConditionAccessor {
   public:

   RaceConditionAccessor<T>() = default;
   RaceConditionAccessor<T>(size_t elems) : m_shallow_copy_of_cpu_data(nullptr), m_deep_copy_of_previous_state_of_cpu_data(nullptr), m_accesses(nullptr), m_size_in_bytes(elems) {} 


   CARE_HOST_DEVICE RaceConditionAccessor<T>(RaceConditionAccessor<T> const & other ) {
      printf("in copy constructor\n");
      if (RAJAPlugin::isParallelContext()) {
         printf("in parallel context\n");
         auto data = m_shallow_copy_of_cpu_data;
         if (!RAJAPlugin::post_parallel_forall_action_registered((void *)data)) {
            printf("registering action\n");
            auto len = m_size_in_bytes / sizeof(T);
            m_deep_copy_of_previous_state_of_cpu_data = new std::remove_const_t<T>[len];
            std::copy_n(data, len, m_deep_copy_of_previous_state_of_cpu_data);
            auto prev_data = m_deep_copy_of_previous_state_of_cpu_data;
            m_accesses = new std::unordered_map<int, std::set<int>> {};
            auto accesses = m_accesses;
            RAJAPlugin::register_post_parallel_forall_action((void *)data, std::bind(detectRaceCondition<T>, data, prev_data, accesses, len, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
         }
      }
   }


   template<typename Idx>
   inline CARE_HOST_DEVICE void operator[](const Idx i)
   const
   {
      if (m_accesses && RAJAPlugin::isParallelContext()) {
         (*m_accesses)[i].insert(RAJAPlugin::threadID);
      }
   }

   void set_size(size_t elems) {
      m_size_in_bytes = elems*sizeof(T);
   }
protected:
   T* m_shallow_copy_of_cpu_data;
private:
   std::remove_const_t<T> * m_deep_copy_of_previous_state_of_cpu_data;
   std::unordered_map<int, std::set<int>> * m_accesses;
   size_t m_size_in_bytes;
};


} // namespace care

#endif // CHAI_Accessor__HPP
