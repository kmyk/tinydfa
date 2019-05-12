#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <map>
#include <queue>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#define REP(i, n) for (int i = 0; (i) < (int)(n); ++ (i))
#define ALL(x) begin(x), end(x)


inline std::array<int, 256> prepare_alphabet_inverse_table(const std::string & alphabet) {
    std::array<int, 256> lookup;
    std::fill(ALL(lookup), -1);
    REP (i, alphabet.size()) {
        char c = alphabet[i];
        assert (std::string("()|*.+?").find(c) == std::string::npos);
        assert (lookup[c] == -1);
        lookup[c] = i;
    }
    return lookup;
}

/**
 * @brief construct an epsilon-NFA from a given regex-string
 * @return a graph enfa s.t. enfa[x] contains pair<>(j, y) if an edge x -> y exists with a label c = alphabet[j]  (c = epsilon if j = -1)
 * @note acceptable nodes are only the last one, x = enfa.size() - 1
 * @note support only:
 *         grouping ( )
 *         alternation |
 *         Kleene star *
 *         some abbreviations . + ?
 */
inline std::vector<std::vector<std::pair<int, int> > > construct_epsilon_nfa(const std::string & re, const std::string & alphabet) {
    using namespace std;
    auto lookup = prepare_alphabet_inverse_table(alphabet);

    // pre-process about parens
    vector<int> paren(re.size(), -1); {
        stack<int> stk;
        REP (i, re.size()) {
            if (re[i] == '(') {
                stk.push(i);
            } else if (re[i] == ')') {
                assert (not stk.empty());
                int j = stk.top();
                stk.pop();
                paren[i] = j;
                paren[j] = i;
            } else if (re[i] == '|') {
                if (not stk.empty()) {
                    paren[i] = stk.top();
                }
            }
        }
        assert (stk.empty());
    }

    // list the edges of an epsilon-NFA; use the positions on regex-string as vertices
    constexpr int EPSILON = -1;
    vector<vector<pair<int, int> > > enfa(re.size() + 3);
    auto use = [&](int i, int j, int c) {
        enfa[i + 1].emplace_back(c, j + 1);
    };
    use(-1, 0, EPSILON);
    use(re.size(), re.size() + 1, EPSILON);
    REP (i, re.size()) {
        if (re[i] == '(' or re[i] == ')') {
            use(i, i + 1, EPSILON);
        } else if (re[i] == '|') {
            int l = (paren[i] == -1 ? -1 : paren[i]);
            int r = (paren[i] == -1 ? re.size() + 1 : paren[l] + 1);
            use(l, i + 1, EPSILON);
            use(i, r, EPSILON);
        } else if (re[i] == '*' or re[i] == '+' or re[i] == '?') {
            assert (i - 1 >= 0);
            assert (re[i - 1] == ')' or re[i - 1] == '.' or lookup[re[i - 1]] != -1);
            int j = (re[i - 1] == ')' ? paren[i - 1] : i - 1);
            if (re[i] == '*' or re[i] == '?') {
                use(j, i + 1, EPSILON);
            }
            if (re[i] == '*' or re[i] == '+') {
                use(i, j, EPSILON);
            }
            use(i, i + 1, EPSILON);
        } else if (re[i] == '.') {
            for (char c : alphabet) {
                use(i, i + 1, lookup[c]);
            }
        } else {
            assert (lookup[re[i]] != -1);
            use(i, i + 1, lookup[re[i]]);
        }
    }
    return enfa;
}

/**
 * @brief construct a NFA from a given epsilon-NFA
 * @return a graph nfa s.t. nfa[j][x] has y means that an edge x -> y exists with a label c = alphabet[j]
 * @assume acceptable nodes of both enfa and nfa are only the last one
 */
inline std::vector<std::vector<std::vector<int> > > remove_epsilon_moves(const std::vector<std::vector<std::pair<int, int> > > & enfa, int alphabet_size) {
    using namespace std;
    constexpr int EPSILON = -1;
    int accepted = enfa.size() - 1;

    // merge epsilon-moves with Warshall-Floyd algorithm
    vector<vector<bool> > epsilon(enfa.size(), vector<bool>(enfa.size()));
    REP (x, enfa.size()) {
        epsilon[x][x] = true;
        for (auto edge : enfa[x]) {
            int c, y; tie(c, y) = edge;
            if (c == EPSILON) {
                epsilon[x][y] = true;
            }
        }
    }
    REP (z, enfa.size()) {
        REP (x, enfa.size()) {
            REP (y, enfa.size()) {
                epsilon[x][y] = (epsilon[x][y] or (epsilon[x][z] and epsilon[z][y]));
            }
        }
    }

    // construct the NFA
    vector<vector<vector<int> > > nfa(alphabet_size, vector<vector<int> >(enfa.size()));
    REP (x, enfa.size()) {
        REP (y, enfa.size()) if (epsilon[x][y]) {
            for (auto edge : enfa[y]) {
                int c, z; tie(c, z) = edge;
                if (c != EPSILON) {
                    nfa[c][x].push_back(z);
                    if (epsilon[z][accepted]) {
                        nfa[c][x].push_back(accepted);
                    }
                }
            }
        }
        REP (c, alphabet_size) {
            auto & edges = nfa[c][x];
            sort(ALL(edges));
            edges.erase(unique(ALL(edges)), edges.end());
        }
    }
    return nfa;
}

/**
 * @brief construct a DFA from a given NFA
 * @return a graph dfa s.t. dfa[j][x] = y means that an edge x -> y exists with a label c = alphabet[j]
 * @note assume acceptable nodes of the NFA are only the last one
 */
inline std::pair<std::vector<std::vector<int> >, std::vector<bool> > apply_subset_construction(const std::vector<std::vector<std::vector<int> > > & nfa, int alphabet_size) {
    using namespace std;
    assert (not nfa.empty());
    int accepted = nfa.front().size() - 1;

    // construct a DFA with the subset-construction
    vector<vector<int> > dfa(alphabet_size);
    vector<bool> is_acceptable;
    map<vector<int>, int> subsets; {
        queue<vector<int> > que;
        vector<int> initial({ 0 });
        subsets[initial] = 0;
        is_acceptable.push_back(initial.back() == accepted);
        que.push(initial);
        while (not que.empty()) {
            vector<int> cur = que.front();
            que.pop();
            REP (c, alphabet_size) {
                vector<int> nxt;
                for (int x : cur) {
                    nxt.insert(nxt.end(), ALL(nfa[c][x]));
                }
                sort(ALL(nxt));
                nxt.erase(unique(ALL(nxt)), nxt.end());
                if (not subsets.count(nxt)) {
                    subsets.emplace(nxt, subsets.size());
                    is_acceptable.push_back(not nxt.empty() and nxt.back() == accepted);
                    que.push(nxt);
                }
                dfa[c].push_back(subsets[nxt]);
            }
        }
    }
    return make_pair(dfa, is_acceptable);
}

inline std::pair<std::vector<std::vector<int> >, std::vector<bool> > construct_dfa_from_regex(const std::string & re, const std::string & alphabet) {
    auto enfa = construct_epsilon_nfa(re, alphabet);
    auto nfa = remove_epsilon_moves(enfa, alphabet.size());
    auto dfa = apply_subset_construction(nfa, alphabet.size());
    return dfa;
}

/**
 * @brief check if the DFA accepts the entire text
 * @note O(length of text)
 */
inline bool regex_match_with_dfa(const std::string & text, const std::vector<std::vector<int> > & dfa, const std::vector<bool> & is_acceptable, const std::string & alphabet) {
    auto lookup = prepare_alphabet_inverse_table(alphabet);
    int x = 0;
    for (char c : text) {
        assert (lookup[c] != -1);
        x = dfa[lookup[c]][x];
    }
    return is_acceptable[x];
}
