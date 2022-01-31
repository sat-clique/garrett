#include "evaluation.h"

#include "types.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <vector>


namespace {
auto operator<<(std::ostream& s, cnfkit::lit const& lit) -> std::ostream&
{
  if (!lit.is_positive()) {
    s << "-";
  }

  s << lit.get_var().get_raw_value();
  return s;
}

auto vars_in_problem(std::vector<Clause> const& problem) -> size_t
{
  std::unordered_set<uint32_t> vars;
  for (Clause const& clause : problem) {
    for (cnfkit::lit const& literal : clause) {
      vars.insert(literal.get_var().get_raw_value());
    }
  }
  return vars.size();
}

auto clauses_in_problem(std::vector<Clause> const& problem) -> size_t
{
  return problem.size();
}

auto unaries_in_problem(std::vector<Clause> const& problem) -> size_t
{
  return std::count_if(problem.begin(), problem.end(), [](auto const& x) { return x.size() == 1; });
}

auto clauses_in_gates(gatekit::gate_structure<ClauseHandle> const& structure) -> size_t
{
  size_t result = 0;
  for (gatekit::gate<ClauseHandle> const& gate : structure.gates) {
    result += gate.clauses.size();
  }
  return result;
}

template <typename T>
void print_stat(std::string name, T&& value)
{
  std::cout << name << ": " << value << "\n";
}
}

void print_stats(std::vector<Clause> const& problem,
                 gatekit::gate_structure<ClauseHandle> const& structure,
                 std::filesystem::path const& path,
                 std::chrono::milliseconds duration)
{
  print_stat("name", path.filename().string());
  print_stat("dur_gate_scan_seconds", static_cast<double>(duration.count()) / 1000.0);
  print_stat("num_vars_in_problem", vars_in_problem(problem));
  print_stat("num_clauses_in_problem", clauses_in_problem(problem));
  print_stat("num_unaries_in_problem", unaries_in_problem(problem));
  print_stat("num_clauses_in_gates", clauses_in_gates(structure));
  print_stat("num_gates", structure.gates.size());
  print_stat("num_roots", structure.roots.size());
  print_stat("num_gates/num_vars_in_problem",
             static_cast<double>(structure.gates.size()) /
                 static_cast<double>(vars_in_problem(problem)));
  print_stat("num_clauses_in_gates/num_clauses_in_problem",
             static_cast<double>(clauses_in_gates(structure)) /
                 static_cast<double>(clauses_in_problem(problem)));
}
