#include <cassert>
#include <string>
#include <utility>
#include <vector>
#include "tinydfa.hpp"
#define REP(i, n) for (int i = 0; (i) < (int)(n); ++ (i))
#define REP3(i, m, n) for (int i = (m); (i) < (int)(n); ++ (i))
#define REP_R(i, n) for (int i = (int)(n) - 1; (i) >= 0; -- (i))
#define REP3R(i, m, n) for (int i = (int)(n) - 1; (i) >= (int)(m); -- (i))
#define ALL(x) begin(x), end(x)
using namespace std;

/**
 * @brief count strings which the regex matchs
 * @note O((the size of DFA) * (the size of alphabet) * limit)
 * @note you need to use appropriate integer class to avoid overflow
 * @note we can compute this with O((the size of DFA) * log limit) if you use matrix-exponentation
 */
template <class Integer>
vector<Integer> count_regex_match(const vector<vector<int> > & dfa, const vector<bool> & is_acceptable, int alphabet_size, int limit) {
    int dfa_size = dfa.front().size();
    vector<Integer> result(limit);
    vector<Integer> cur(dfa_size), prv;
    cur[0] = 1;
    if (is_acceptable[0]) {
        result[0] += cur[0];
    }
    REP3 (l, 1, limit) {
        cur.swap(prv);
        cur.assign(dfa_size, 0);
        REP (c, alphabet_size) {
            REP (x, dfa_size) {
                cur[dfa[c][x]] += prv[x];
            }
        }
        REP (x, dfa_size) if (is_acceptable[x]) {
            result[l] += cur[x];
        }
    }
    return result;
}


template <int32_t MOD>
struct mint {
    int32_t value;
    mint() = default;
    mint(int32_t value_) : value(value_) {}
    inline mint<MOD> operator + (mint<MOD> other) const { int32_t c = this->value + other.value; return mint<MOD>(c >= MOD ? c - MOD : c); }
    inline mint<MOD> operator * (mint<MOD> other) const { int32_t c = (int64_t)this->value * other.value % MOD; return mint<MOD>(c < 0 ? c + MOD : c); }
    inline mint<MOD> & operator += (mint<MOD> other) { this->value += other.value; if (this->value >= MOD) this->value -= MOD; return *this; }
    inline mint<MOD> & operator *= (mint<MOD> other) { this->value = (int64_t)this->value * other.value % MOD; if (this->value < 0) this->value += MOD; return *this; }
};

constexpr int MOD = 1e9 + 7;
int main() {
    // test regex_match_with_dfa()
    auto match = [&](const string & text, const string & re, const string & alphabet) {
        auto dfa = construct_dfa_from_regex(re, alphabet);
        return regex_match_with_dfa(text, dfa.first, dfa.second, alphabet);
    };
    assert (match("AABAAB", ".*A.*", "ABC"));
    assert (match("AABAAB", "(.A*.)*", "ABC"));
    assert (not match("AABAAB", "AAB", "ABC"));
    assert (not match("AABAAB", ".*AAA.*", "ABC"));

    // test count_regex_match()
    auto count = [&](const string & re, const string & alphabet, int length) {
        auto dfa = construct_dfa_from_regex(re, alphabet);
        return count_regex_match<mint<MOD> >(dfa.first, dfa.second, alphabet.size(), length + 1)[length].value;
    };
    assert (count(".*A.*", "AB", 4) == 15);  // all but BBBB
    assert (count("(AA?B)*", "AB", 6) == 2);  // ABABAB and AABAAB
    assert (count("A*B*|CC", "ABC", 2) == 4);  // AA AB BB CC
    assert (count("A+B*C+|D*", "ABCDE", 5) == 11);  // AAAAC AAABC AABBC ABBBC AAACC AABCC ABBCC AACCC ABCCC ACCCC DDDDD
    assert (count("(BB?)?(AA?BB?)*AAA+(BB?A+)*(BB?)?", "AB", 10) == 326);  // >>> len(list(filter(lambda s: 'AAA' in s and 'BBB' not in s, map(''.join, itertools.product('AB', repeat=10)))))
    assert (count("(BB?)?(AA?BB?)*AAA+(BB?A+)*(BB?)?", "AB", 10000000) == 302889810);  // the number of strings which contain AAA and don't contain BBB
    assert (count(".*A................", "AB", 1000) == 344211605);  // the size of DFA eplodes (~ 13000)  (minimization doesn't work)
    return 0;
}
