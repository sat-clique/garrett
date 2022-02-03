#include "validation.h"
#include "cnfkit/literal.h"

#include <gatekit/gate.h>

#include <minisat/core/Solver.h>

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

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
  sat_solver(cnfkit::var max_var)
  {
    for (size_t i = 0; i <= max_var.get_raw_value(); ++i) {
      m_solver.newVar();
    }
  }

  auto new_lit() -> cnfkit::lit
  {
    return cnfkit::lit{cnfkit::var{static_cast<uint32_t>(m_solver.newVar(Minisat::l_Undef, false))},
                       true};
  }

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
    Minisat::Lit minisat_lit =
        Minisat::mkLit(static_cast<Minisat::Var>(lit.get_var().get_raw_value()));
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
};

auto max_var(gatekit::gate_structure<ClauseHandle> const& structure) -> cnfkit::var
{
  cnfkit::var max_var{1};
  for (gatekit::gate<ClauseHandle> const& gate : structure.gates) {
    for (ClauseHandle clause : gate.clauses) {
      for (cnfkit::lit const& lit : *clause) {
        if (max_var < lit.get_var()) {
          max_var = lit.get_var();
        }
      }
    }
  }

  return max_var;
}

class gate_validator {
public:
  explicit gate_validator(gatekit::gate_structure<ClauseHandle> const& structure)
    : m_solver(max_var(structure))
  {
  }

  auto is_valid_gate(gatekit::gate<ClauseHandle> const& gate) -> bool
  {
    bool const result = has_left_totality(gate) && has_right_uniqueness(gate);
    return result;
  }


private:
  auto has_left_totality(gatekit::gate<ClauseHandle> const& gate) -> bool
  {
    cnfkit::lit selector = m_solver.new_lit();
    for (ClauseHandle const& clause : gate.clauses) {
      m_solver.add_clause(*clause, selector);
    }

    for (ClauseHandle const& clause : gate.clauses) {
      for (cnfkit::lit const& lit : *clause) {
        if (lit.get_var() != gate.output.get_var()) {
          m_solver.add_assumption(-lit);
        }
      }

      m_solver.add_assumption(-selector);
      if (!m_solver.is_satisfiable()) {
        return false;
      }
    }

    m_solver.add_clause({selector});

    return true;
  }

  auto has_right_uniqueness(gatekit::gate<ClauseHandle> const& gate) -> bool
  {
    cnfkit::lit selector = m_solver.new_lit();

    std::vector<cnfkit::lit> clause_without_o;
    for (ClauseHandle const& clause : gate.clauses) {
      for (cnfkit::lit const& lit : *clause) {
        if (lit.get_var() != gate.output.get_var()) {
          clause_without_o.push_back(lit);
        }
      }
      m_solver.add_clause(clause_without_o, selector);
      clause_without_o.clear();
    }

    m_solver.add_assumption(-selector);
    bool const result = !m_solver.is_satisfiable();

    m_solver.add_clause({selector});
    return result;
  }

  sat_solver m_solver;
};


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
  gate_validator validator{structure};

  for (gatekit::gate<ClauseHandle> const& gate : structure.gates) {
    if (!validator.is_valid_gate(gate)) {
      print_invalid_gate(gate);
      return false;
    }
  }

  return true;
}
