#include <string>
#include <utility>
#include <vector>

std::vector<std::vector<std::pair<int, int> > > construct_epsilon_nfa(const std::string & re, const std::string & alphabet);
std::vector<std::vector<std::vector<int> > > remove_epsilon_moves(const std::vector<std::vector<std::pair<int, int> > > & enfa, int alphabet_size);
std::pair<std::vector<std::vector<int> >, std::vector<bool> > apply_subset_construction(const std::vector<std::vector<std::vector<int> > > & nfa, int alphabet_size);
std::pair<std::vector<std::vector<int> >, std::vector<bool> > construct_dfa_from_regex(const std::string & re, const std::string & alphabet);
