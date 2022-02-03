#include "validation.h"
#include "cnfkit/literal.h"

#include <gatekit/gate.h>

#include <minisat/core/Solver.h>

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
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
  sat_solver solver;
  bool const result = has_left_totality(gate, solver) && has_right_uniqueness(gate, solver);
  return result;
}


void print_invalid_gate(gatekit::gate<ClauseHandle> const& gate)
{
  std::cout << "Validation failed for gate with output variable " << gate.output << "\nClauses:\n";
  for (ClauseHandle const& clause : gate.clauses) {
    std::cout << clause_to_string(*clause) << "\n";
  }
}
}

auto is_valid_gate_structure(gatekit::gate_structure<ClauseHandle> const& structure) -> bool
{
  for (gatekit::gate<ClauseHandle> const& gate : structure.gates) {
    if (!is_valid_gate(gate)) {
      print_invalid_gate(gate);
      return false;
    }
  }

  return true;
}
