#pragma once

#include <cnfkit/literal.h>
#include <gatekit/clause.h>

#include <chrono>
#include <cstdint>
#include <vector>

using Clause = std::vector<cnfkit::lit>;
using ClauseHandle = Clause const*;


namespace gatekit {
template <>
inline auto lit_to_dimacs<cnfkit::lit>(cnfkit::lit literal) -> int
{
  return cnfkit::lit_to_dimacs(literal);
}

template <>
inline auto dimacs_to_lit<cnfkit::lit>(int literal) -> cnfkit::lit
{
  return cnfkit::dimacs_to_lit(literal);
}
}


template <typename T>
class ptr_iterator {
public:
  explicit ptr_iterator(T* p) noexcept : m_ptr(p) {}

  auto operator*() const noexcept -> T* { return m_ptr; }

  auto operator++() noexcept -> ptr_iterator&
  {
    ++m_ptr;
    return *this;
  }

  auto operator+(size_t offset) noexcept -> ptr_iterator { return ptr_iterator{m_ptr + offset}; }

  auto operator!=(const ptr_iterator& rhs) const noexcept -> bool { return m_ptr != rhs.m_ptr; }

private:
  T* m_ptr;
};


template <typename Clock = std::chrono::steady_clock>
class stopwatch {
public:
  stopwatch() : m_start{Clock::now()} {}

  auto duration_since_start() const -> typename Clock::duration { return Clock::now() - m_start; }


private:
  typename Clock::time_point m_start;
};
