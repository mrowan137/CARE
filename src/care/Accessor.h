
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
class NoOpAccessor {
   public:
   NoOpAccessor<T>() = default;
   NoOpAccessor<T>(size_t , const char * ) {}
   
   template<typename Idx> inline CARE_HOST_DEVICE void operator[](const Idx) const {}
   void set_size(size_t) {}
   void set_data(T *) {}
   void set_name(char const *) {}
};

template <typename T>
void detectRaceCondition(T* data, T* prev_data, std::unordered_map<int, std::set<int>> * accesses, size_t len, const char * fieldName,
                         chai::ExecutionSpace space, const char * fileName, int lineNumber) {
   for (size_t i = 0; i < len; ++i) {
      if (!(data[i] == prev_data[i])) {
         if ((*accesses)[i].size() > 1) {
            printf("RACE CONDITION DETECTED, loop in execution space %i, fileName: %s, lineNumber %i\n", (int) space, fileName, lineNumber);
            printf("DATA %p NAMED %s at index %zu changed and accessed by following threads\n\t", data, fieldName, i);
            for (auto const & x : (*accesses)[i]) {
               printf("%i,",x);
            }
            printf("\n");
            break;
         }
      }
   }
   delete [] prev_data;
   delete accesses;
}

template <typename T>
class RaceConditionAccessor : public NoOpAccessor<T> {
   public:

   RaceConditionAccessor<T>() = default;
   RaceConditionAccessor<T>(size_t elems, const char * name) : m_shallow_copy_of_cpu_data(nullptr), m_deep_copy_of_previous_state_of_cpu_data(nullptr), m_accesses(nullptr), m_size_in_bytes(elems*sizeof(T)), m_name(name) {}


   CARE_HOST_DEVICE RaceConditionAccessor<T>(RaceConditionAccessor<T> const & other ) : m_shallow_copy_of_cpu_data(other.m_shallow_copy_of_cpu_data), m_deep_copy_of_previous_state_of_cpu_data(other.m_deep_copy_of_previous_state_of_cpu_data), m_accesses(other.m_accesses), m_size_in_bytes(other.m_size_in_bytes), m_name(other.m_name) {
      if (RAJAPlugin::isParallelContext()) {
         auto data = m_shallow_copy_of_cpu_data;
         if (!RAJAPlugin::post_parallel_forall_action_registered((void *)data)) {
            auto len = m_size_in_bytes / sizeof(T);
            m_deep_copy_of_previous_state_of_cpu_data = new typename std::remove_const<T>::type[len];
            std::copy_n(data, len, m_deep_copy_of_previous_state_of_cpu_data);
            auto prev_data = m_deep_copy_of_previous_state_of_cpu_data;
            m_accesses = new std::unordered_map<int, std::set<int>> {};
            auto accesses = m_accesses;
            const char * name = m_name;
            RAJAPlugin::register_post_parallel_forall_action((void *)data, std::bind(detectRaceCondition<T>, data, prev_data, accesses, len, name, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
         }
      }
   }


   template<typename Idx>
   inline CARE_HOST_DEVICE void operator[](const Idx i)
   const
   {
      if (m_accesses && RAJAPlugin::isParallelContext()) {
         (*m_accesses)[i].insert(RAJAPlugin::s_threadID);
      }
   }

   void set_size(size_t elems) {
      m_size_in_bytes = elems*sizeof(T);
   }

   void set_data(T * ptr) {
      m_shallow_copy_of_cpu_data = ptr;
   }
   void set_name(char const * name) { m_name = name;}
protected:
   T* m_shallow_copy_of_cpu_data;
   typename std::remove_const<T>::type * m_deep_copy_of_previous_state_of_cpu_data;
   std::unordered_map<int, std::set<int>> * m_accesses;
   size_t m_size_in_bytes;
   char const* m_name;
};

template <typename T>
class RaceConditionAccessorWithCallback : public RaceConditionAccessor<T> {
   public:
   RaceConditionAccessorWithCallback<T>() = default;
   RaceConditionAccessorWithCallback<T>(size_t elems, const char * name) : RaceConditionAccessor<T>(elems,name) {}
   RaceConditionAccessorWithCallback(RaceConditionAccessor<T> const & other) : RaceConditionAccessor<T>(other) {}
   void set_callback(std::function<void(int)> callback) {m_callback = callback;}


   CARE_HOST_DEVICE RaceConditionAccessorWithCallback<T>(RaceConditionAccessorWithCallback<T> const & other ) : RaceConditionAccessor<T>(other) , m_callback(other.m_callback) {}

   template<typename Idx>
   inline CARE_HOST_DEVICE void operator[](const Idx i)
   const
   {
      m_callback(i);
      if (RaceConditionAccessor<T>::m_accesses && RAJAPlugin::isParallelContext()) {
         printf("inserting %i into (%p) accesses[%i] for pointer %p\n", RAJAPlugin::s_threadID, (void *) RaceConditionAccessor<T>::m_accesses, i, RaceConditionAccessor<T>::m_shallow_copy_of_cpu_data);
         printf("size of accesses before insert %lu\n", RaceConditionAccessor<T>::m_accesses->size());
      }
      RaceConditionAccessor<T>::operator[](i);
      if (RaceConditionAccessor<T>::m_accesses && RAJAPlugin::isParallelContext()) {
         printf("size of accesses after insert %lu\n", RaceConditionAccessor<T>::m_accesses->size());
         printf("size of accesses[%i] after insert %lu\n", i, (*RaceConditionAccessor<T>::m_accesses)[i].size());
      }
   }

protected:
   std::function<void(int)> m_callback; 
};

#ifdef CARE_ENABLE_RACE_DETECTION
#define CARE_DEFAULT_ACCESSOR RaceConditionAccessor
#else
#define CARE_DEFAULT_ACCESSOR NoOpAccessor
#endif



} // namespace care

#endif // CHAI_Accessor__HPP
