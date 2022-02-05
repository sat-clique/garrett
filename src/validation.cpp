#include "validation.h"
#include "cnfkit/literal.h"

#include <gatekit/gate.h>

#include <minisat/core/Solver.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

namespace {

auto operator<<(std::ostream& stream, cnfkit::lit const& lit) -> std::ostream&
{
  if (!lit.is_positive()) {
    stream << "-";
  }
  stream << lit.get_var().get_raw_value();
  return stream;
}

auto clause_to_string(Clause const& clause) -> std::string
{
  std::string result = "(";
  for (auto const& lit : clause) {
    result += lit.is_positive() ? " " : " -";
    result += std::to_string(lit.get_var().get_raw_value());
  }

  result += " )";
  return result;
}


class sat_solver {
public:
  void add_clause(std::vector<cnfkit::lit> const& clause,
                  std::optional<cnfkit::lit> extra_lit = std::nullopt)
  {
    to_buf(clause);
    if (extra_lit) {
      m_buffer.push(to_minisat_lit(*extra_lit));
    }

    m_solver.addClause(m_buffer);
  }

  void add_assumption(cnfkit::lit const& assumption)
  {
    m_assumptions.push(to_minisat_lit(assumption));
  }

  auto is_satisfiable() -> bool
  {
    bool const result = m_solver.solve(m_assumptions);
    m_assumptions.clear();
    return result;
  }

private:
  auto to_minisat_lit(cnfkit::lit const& lit) -> Minisat::Lit
  {
    auto cached_var = m_vars.find(lit.get_var().get_raw_value());

    Minisat::Var minisat_var = 0;
    if (cached_var == m_vars.end()) {
      minisat_var = m_solver.newVar();
      m_vars[lit.get_var().get_raw_value()] = minisat_var;
    }
    else {
      minisat_var = cached_var->second;
    }

    Minisat::Lit minisat_lit = Minisat::mkLit(minisat_var);
    return lit.is_positive() ? minisat_lit : ~minisat_lit;
  }

  void to_buf(std::vector<cnfkit::lit> const& clause)
  {
    m_buffer.clear();
    for (auto const& lit : clause) {
      m_buffer.push(to_minisat_lit(lit));
    }
  }

  Minisat::vec<Minisat::Lit> m_buffer;
  Minisat::vec<Minisat::Lit> m_assumptions;
  Minisat::Solver m_solver;

  std::unordered_map<uint32_t, Minisat::Var> m_vars;
};


auto has_left_totality(gatekit::gate<ClauseHandle> const& gate, sat_solver& solver) -> bool
{
  for (ClauseHandle const& clause : gate.clauses) {
    solver.add_clause(*clause);
  }

  for (ClauseHandle const& clause : gate.clauses) {
    for (cnfkit::lit const& lit : *clause) {
      if (lit.get_var() != gate.output.get_var()) {
        solver.add_assumption(-lit);
      }
    }

    if (!solver.is_satisfiable()) {
      return false;
    }
  }

  return true;
}

auto has_right_uniqueness(gatekit::gate<ClauseHandle> const& gate, sat_solver& solver) -> bool
{
  std::vector<cnfkit::lit> clause_without_o;
  for (ClauseHandle const& clause : gate.clauses) {
    for (cnfkit::lit const& lit : *clause) {
      if (lit.get_var() != gate.output.get_var()) {
        clause_without_o.push_back(lit);
      }
    }
    solver.add_clause(clause_without_o);
    clause_without_o.clear();
  }

  return !solver.is_satisfiable();
}


auto is_valid_gate(gatekit::gate<ClauseHandle> const& gate) -> bool
{
  // TODO: also check that if a gate with output o is nested monotonically,
  // o does not occur in any gate input
  sat_solver solver;
  bool const result = has_left_totality(gate, solver) &&
                      (gate.is_nested_monotonically || has_right_uniqueness(gate, solver));
  return result;
}


void print_invalid_gate(gatekit::gate<ClauseHandle> const& gate)
{
  std::cout << "Validation failed for gate with output variable " << gate.output << "\nClauses:\n";
  for (ClauseHandle const& clause : gate.clauses) {
    std::cout << clause_to_string(*clause) << "\n";
  }
}

template <typename GateIter>
auto verify_gates(GateIter start, GateIter stop, std::atomic<std::size_t>* num_verified_gates)
    -> bool
{
  GateIter current = start;
  while (current != stop) {
    if (!is_valid_gate(*current)) {
      print_invalid_gate(*current);
      return false;
    }

    ++current;
    ++(*num_verified_gates);
  }

  return true;
}
}

auto is_valid_gate_structure(gatekit::gate_structure<ClauseHandle> const& structure,
                             std::size_t num_threads,
                             std::function<void(std::size_t)> const& progress_callback) -> bool
{
  size_t const chunk_size = (structure.gates.size() / num_threads) + 1;

  std::vector<std::future<bool>> results;
  std::vector<std::unique_ptr<std::atomic<std::size_t>>> progress;

  auto chunk_start = structure.gates.begin();
  for (size_t n = 0; n < num_threads; ++n) {
    auto chunk_end = chunk_start;
    if (std::distance(chunk_end, structure.gates.end()) <= chunk_size) {
      chunk_end = structure.gates.end();
    }
    else {
      chunk_end = chunk_start + chunk_size;
    }

    progress.push_back(std::make_unique<std::atomic<std::size_t>>(0));
    results.push_back(std::async(std::launch::async,
                                 verify_gates<decltype(chunk_start)>,
                                 chunk_start,
                                 chunk_end,
                                 progress.back().get()));
  }

  for (auto& future : results) {
    std::future_status status = std::future_status::timeout;
    while (status == std::future_status::timeout) {
      status = future.wait_for(std::chrono::milliseconds{100});
      std::size_t total_verified_gates = 0;
      for (auto const& progress_counter : progress) {
        total_verified_gates += *progress_counter;
      }
      progress_callback(total_verified_gates);
    }
  }

  return true;
}
