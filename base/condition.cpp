#include "../std/target_os.hpp"

#if defined(OMIM_OS_BADA)
  #include "condition_bada.cpp"
#elif defined(OMIM_OS_WINDOWS_NATIVE)
  #include "condition_windows_native.cpp"
#else
  #include "condition_posix.cpp"
#endif

namespace threads
{
  ConditionGuard::ConditionGuard(Condition & condition)
    : m_Condition(condition)
  {
    m_Condition.Lock();
  }

  ConditionGuard::~ConditionGuard()
  {
    m_Condition.Unlock();
  }

  void ConditionGuard::Wait()
  {
    m_Condition.Wait();
  }

  void ConditionGuard::Signal(bool broadcast)
  {
    m_Condition.Signal(broadcast);
  }
}

