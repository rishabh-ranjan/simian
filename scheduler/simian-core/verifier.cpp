/*
 * verifier.cpp: monolithic implementation of the core logic of Simian
 * author: Rishabh Ranjan
 */

#include "logger.h"
extern Logger logger;

#include "verifier.h"

#include "graph.hh"
#include "z3++.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

void Verifier::push_call(int comm, const std::string& type_str,
		int data, int src, int dest, int rtag, int stag,
		const std::vector< int >& preds) {
	calls_.emplace_back();
	Call& c = calls_.back();
	c.idx = calls_.size() - 1;
	c.pid = pid_offset_.size() - 1;
	c.loc = calls_.size() - 1 - pid_offset_.back();
	c.comm = comm;
	c.type = conv_type_(type_str);

	if (c.type == CallType::send) {
		c.data = data;
		c.nbr = dest;
		c.tag = stag;
	} else if (c.type == CallType::recv) {
		c.nbr = src;
		c.tag = rtag;
	}

	for (int x: preds) {
		c.preds.push_back(index_(c.pid, x));
	}
}

void Verifier::merge_barrs_() {
	// simplifying assumption: pid 0 sees all barriers
	std::vector< int > comm_sz; // number of barriers in each communicator
	for (int i = 0; i < pid_offset_[1]; ++i) { // for all calls with pid 0
		Call& c = calls_[i];
		if (c.type != CallType::raw_barr) continue;
		if (c.comm >= (int)comm_sz.size()) comm_sz.resize(c.comm + 1);
		++comm_sz[c.comm];
	}

	std::vector< int > comm_offset; // offset for barrs of a comm in calls_
	comm_offset.reserve(comm_sz.size() + 1);
	comm_offset.push_back(pid_offset_.back());
	for (int x: comm_sz) {
		comm_offset.push_back(comm_offset.back() + x);
	}
	calls_.reserve(comm_offset.back());
	for (int i = 0; i < (int)comm_sz.size(); ++i) { // for each comm
		for (int j = comm_offset[i]; j < comm_offset[i+1]; ++j) { // all barrs in it
			calls_.emplace_back();
			Call& c = calls_.back();
			c.idx = calls_.size() - 1;
			c.comm = i;
			c.bid = j - comm_offset[i];
			c.type = CallType::merged_barr;
			c.pid = c.comm;
			c.loc = c.bid;
		}
	}

	std::vector< int > merged_index(pid_offset_.back());
	for (int i = 0; i < (int)pid_offset_.size() - 1; ++i) {
		std::vector< int > comm_bid(comm_sz.size());
		for (int j = pid_offset_[i]; j < pid_offset_[i+1]; ++j) {
			Call& c = calls_[j];
			if (c.type == CallType::raw_barr) {
				c.bid = comm_bid[c.comm]++;
				merged_index[j] = comm_offset[c.comm] + c.bid;
			} else {
				merged_index[j] = j;
			}
		}
	}

	for (int i = 0; i < pid_offset_.back(); ++i) {
		Call& c = calls_[i];
		if (c.type == CallType::raw_barr) {
			Call& b = calls_[merged_index[i]];
			for (int x: c.preds) {
				b.preds.push_back(merged_index[x]);
			}
		} else {
			for (int &x: c.preds) {
				x = merged_index[x];
			}
		}
	}
}

void Verifier::ancestor_dfs_(std::vector< bool >& seen, int idx) {
	seen[idx] = true;
	for (int pred: calls_[idx].preds) {
		if (seen[pred]) continue;
		ancestor_dfs_(seen, pred);
	}
	for (int pred: calls_[idx].preds) {
		for (int i = 0; i < (int)calls_.size()/64 + 1; ++i) {
			ancestors_[idx][i] |= ancestors_[pred][i];
		}
		ancestors_[idx][pred/64] |= (1ULL<<(pred%64));
	}
}

/* ancestor computation in n^2/64 using bitmasking and dfs */
void Verifier::compute_ancestors_() {
	ancestors_.resize(calls_.size(),
			std::vector< uint64_t > (calls_.size()/64 + 1));
	std::vector< bool > seen(calls_.size());
	for (int i = 0; i < (int)calls_.size(); ++i) {
		if (seen[i]) continue;
		if (calls_[i].type != CallType::raw_barr) ancestor_dfs_(seen, i);
	}
	for (Call& c: calls_) {
		for (int i = 0; i < (int)calls_.size(); ++i) {
			if (has_ancestor_(c.idx, i)) c.ancs.push_back(i);
		}
	}
}

void Verifier::basic_mplus_() {
	for (int i = 0; i < pid_offset_.back(); ++i) {
		Call& s = calls_[i];
		if (s.type != CallType::send) continue;
		for (int j = pid_offset_[s.nbr]; j < pid_offset_[s.nbr+1]; ++j) {
			Call& r = calls_[j];
			if (r.type != CallType::recv) continue;
			if ((r.comm == s.comm)
					&& (r.nbr == s.pid || r.nbr == -1)
					&& (r.tag == s.tag || r.tag == -1)) {
				s.mplus.push_back(j);
				r.mplus.push_back(i);
			}
		}
	}
}

void Verifier::prune_mplus_by_ancs_() {
	for (int i = 0; i < pid_offset_.back(); ++i) {
		Call& c = calls_[i];
		for (auto it = c.mplus.begin(); it != c.mplus.end(); ) {
			if (has_ancestor_(i, *it) || has_ancestor_(*it, i)) {
				std::swap(*it, c.mplus.back());
				c.mplus.pop_back();
			} else {
				++it;
			}
		}
	}
}

/*
void Verifier::prune_mplus_by_mo_dfs_(std::vector< bool >& seen, int idx) {
	seen[idx] = true;
	Call& c = calls_[idx];
	for (int p: c.preds) {
		if (seen[p]) continue;
		prune_mplus_by_mo_dfs_(seen, p);
	}
	for (auto it = c.mplus.begin(); it != c.mplus.end(); ) {
		Call& b = calls_[*it];
		if (has_ancestor_(b.idx, c.idx) || has_ancestor_(c.idx, b.idx)) {
			// TODO: does this really need to be said separately
			// or there is some flaw in the code?
			// I think this should automatically be handled by the mo heuristic
			// because mo heuristic is supposed to be somewhat powerful
			// Also, see how effective is this alone
			std::swap(*it, c.mplus.back());
			c.mplus.pop_back();
			continue;
		}
		bool del = false;
		for (int p: c.ancs) {
			if (calls_[p].type != CallType::send && calls_[p].type != CallType::recv) continue;
			bool flag = false;
			for (int q: calls_[p].mplus) {
				if (!has_ancestor_(q, b.idx)) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				del = true;
				break;
			}
		}
		if (del) {
			std::swap(*it, c.mplus.back());
			c.mplus.pop_back();
			continue;
		}
		for (int p: b.preds) {
			if (seen[p]) continue;
			prune_mplus_by_mo_dfs_(seen, p);
		}
		for (int p: b.ancs) {
			if (calls_[p].type != CallType::send && calls_[p].type != CallType::recv) continue;
			bool flag = false;
			for (int q: calls_[p].mplus) {
				if (!has_ancestor_(q, c.idx)) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				del = true;
				break;
			}
		}
		if (del) {
			std::swap(*it, c.mplus.back());
			c.mplus.pop_back();
			continue;
		}

		++it;
	}
}

void Verifier::prune_mplus_by_mo_() {
	std::vector< bool > seen(calls_.size());
	for (int i = 0; i < (int)calls_.size(); ++i) {
		if (seen[i]) continue;
		if (calls_[i].type != CallType::raw_barr) prune_mplus_by_mo_dfs_(seen, i);
	}
}
*/

bool Verifier::prune_mplus_by_mo_pass_() {
	bool del_flag = false;
	for (int i = 0; i < pid_offset_.back(); ++i) {
		Call& c = calls_[i];
		if (c.type != CallType::send && c.type != CallType::recv) continue;
		std::unordered_set<int> del;
		for (int j: c.mplus) {
			Call& b = calls_[j];
			bool forall_flag = false;
			for (int k: c.ancs) {
				bool exists_flag = false;
				Call& cc = calls_[k];
				if (cc.type != CallType::send && cc.type != CallType::recv) continue;
				for (int l: cc.mplus) {
					Call& bb = calls_[l];
					if (!has_ancestor_(bb.idx, b.idx) && bb.idx != b.idx) {
						exists_flag = true;
						break;
					}
				}
				if (!exists_flag) {
					forall_flag = true;
					break;
				}
			}
			if (forall_flag) {
				del.insert(j);
				continue;
			}
			for (int k: b.ancs) {
				bool exists_flag = false;
				Call& bb = calls_[k];
				if (bb.type != CallType::send && bb.type != CallType::recv) continue;
				for (int l: bb.mplus) {
					Call& cc = calls_[l];
					if (!has_ancestor_(cc.idx, c.idx) && cc.idx != c.idx) {
						exists_flag = true;
						break;
					}
				}
				if (!exists_flag) {
					forall_flag = true;
					break;
				}
			}
			if (forall_flag) {
				del.insert(j);
			}
		}
		for (int j = 0; j < (int)c.mplus.size(); ) {
			if (del.find(c.mplus[j]) != del.end()) {
				std::swap(c.mplus[j], c.mplus.back());
				c.mplus.pop_back();
				del_flag = true;
			} else {
				++j;
			}
		}
	}
	return del_flag;
}

void Verifier::mplus_clean_() {
	for (int i = 0; i < pid_offset_.back(); ++i) {
		Call& c = calls_[i];
		if (c.type != CallType::send && c.type != CallType::recv) continue;
		std::unordered_set<int> del;
		for (int j: c.mplus) {
			Call& b = calls_[j];
			if (std::find(b.mplus.begin(), b.mplus.end(), i) == b.mplus.end()) {
				del.insert(j);
			}
		}
		for (int j = 0; j < (int)c.mplus.size(); ) {
			if (del.find(c.mplus[j]) != del.end()) {
				std::swap(c.mplus[j], c.mplus.back());
				c.mplus.pop_back();
			} else {
				++j;
			}
		}
	}
}

void Verifier::prune_mplus_by_mo_() {
	while (prune_mplus_by_mo_pass_()) {
		mplus_clean_();
	}
}

void Verifier::prune_mplus_by_count_() {
	std::vector< bool > frozen(pid_offset_.back());
	for (int i = 0; i < (int)pid_offset_.size() - 1; ++i) {
		std::map< std::array< int, 3 >, std::pair< int, std::set< int >>> table;
		for (int j = pid_offset_[i]; j < pid_offset_[i+1]; ++j) {
			Call& c = calls_[j];
			if (c.type != CallType::recv) continue;
			auto& record = table[{c.nbr, c.tag, c.comm}];
			auto& count = record.first;
			auto& space = record.second;
			for (int k = 0; k < (int)c.mplus.size(); ++k) {
				if (!frozen[c.mplus[k]]) continue;
				std::swap(c.mplus[k], c.mplus.back());
				c.mplus.pop_back();
				--k;
			}
			for (int x: c.mplus) space.insert(x);
			++count;
			if (count == (int)space.size()) {
				for (int x: space) frozen[x] = true;
				space.clear();
				count = 0;
			}
		}
	}
	for (int i = 0; i < pid_offset_.back(); ++i) {
		Call& c = calls_[i];
		if (c.type != CallType::send) continue;
		c.mplus.clear();
	}
	for (int i = 0; i < pid_offset_.back(); ++i) {
		Call& c = calls_[i];
		if (c.type != CallType::recv) continue;
		for (int x: c.mplus) calls_[x].mplus.push_back(i);
	}
}

void Verifier::compute_cnd_mplus_() {
	for (int i = 0; i < pid_offset_.back(); ++i) {
		Call& c = calls_[i];
		if (c.type == CallType::send) {
			for (int m: c.mplus) {
				Call& b = calls_[m];
				if (b.data_cnd.check(c.data)
						&& b.src_cnd.check(c.pid)
						&& b.tag_cnd.check(c.tag)) {
					c.cnd_mplus.push_back(m);
				}
			}
		} else if (c.type == CallType::recv) {
			for (int m: c.mplus) {
				Call& b = calls_[m];
				if (c.data_cnd.check(b.data)
						&& c.src_cnd.check(b.pid)
						&& c.tag_cnd.check(b.tag)) {
					c.cnd_mplus.push_back(m);
				}
			}
		}
	}
}

void Verifier::order_dfs_(std::vector< bool >& seen, int idx) {
	Call& c = calls_[idx];
	for (int x: c.succs) {
		if (seen[x]) continue;
		seen[x] = true;
		order_dfs_(seen, x);
	}
	for (int x: c.mplus) {
		if (seen[x]) continue;
		seen[x] = true;
		order_dfs_(seen, x);
	}
	order_.push_back(idx);
}

void Verifier::pre_epoch_() {
	epochs_.clear();
	for (Call& c: calls_) {
		if (c.type == CallType::raw_barr) continue;
		c.epoch = -1;
		for (int x: c.preds) calls_[x].succs.push_back(c.idx);
	}
	if (use_epoch_) {
		order_.clear();
		std::vector< bool > seen(calls_.size());
		for (Call& c: calls_) {
			if (c.type == CallType::raw_barr) continue;
			if (seen[c.idx]) continue;
			seen[c.idx] = true;
			order_dfs_(seen, c.idx);
		}
	}
}

bool Verifier::next_epoch_() {
	epochs_.emplace_back();
	//TODO: move this to the end
	while (!order_.empty() && calls_[order_.back()].epoch != -1) order_.pop_back();
	if (order_.empty()) return false;

	Call& c = calls_[order_.back()];
	c.epoch = epochs_.size() - 1;
	epochs_.back().push_back(c.idx);
	std::queue< int > q;
	q.push(c.idx);
	while (!q.empty()) {
		Call& f = calls_[q.front()];
		q.pop();
		for (int x: f.preds) {
			Call& p = calls_[x];
			if (p.epoch != -1) continue;
			p.epoch = c.epoch;
			epochs_.back().push_back(x);
			q.push(x);
		}
		for (int x: f.mplus) {
			Call& m = calls_[x];
			if (m.epoch != -1) continue;
			m.epoch = c.epoch;
			epochs_.back().push_back(x);
			q.push(x);
		}
	}

	return true;
}

void SolverHook(void* user_param, unsigned n, const unsigned* aut) {
	auto obj = static_cast< std::pair<int, Verifier* >* >(user_param);
	obj->second->encode_aut_(obj->first, n, aut);
}

bool Verifier::verify_epoch_() {
	logger << "epoch: " << ((int)epochs_.size()-1);
	logger << "epoch size: " << epochs_.back().size();
	inv_.resize(calls_.size());
	for (int i = 0; i < (int)epochs_.back().size(); ++i) {
		inv_[epochs_.back()[i]] = i;
	}

	bliss::Digraph dg(epochs_.back().size());
	if (use_epoch_ || use_symmetry_) {
		logger << "construct program graph for epoch...";
		for (int x: epochs_.back()) {
			Call& c = calls_[x];
			for (int y: c.preds) {
				Call& p = calls_[y];
				if (p.epoch != c.epoch) continue;
				dg.add_edge(inv_[p.idx], inv_[c.idx]);
			}
			for (int y: c.cnd_mplus) {
				dg.add_edge(inv_[x], inv_[y]);
				dg.add_edge(inv_[y], inv_[x]);
			}
		}
	}

	int hash;
	EpochInfo* epoch_info_p;
	if (use_epoch_) {
		
		logger << "check epoch cache...";
		hash = dg.get_hash();
		auto it = verified_hashes_.find(hash);
		if (it != verified_hashes_.end()) {
			for (auto& epoch_info: it->second) {
				if (dg.cmp(epoch_info.dg) == 0) {
					logger << "found in cache";
					epoch_info.reps += 1;
					return true;
				}
			}
		}

		logger << "fresh epoch";
		auto& v = verified_hashes_[hash];
		v.emplace_back(dg);
		epoch_info_p = &v.back();
		epoch_info_p->size = epochs_.back().size();
		epoch_info_p->reps = 1;
	}

	logger << "create SMT formula...";
	encode_gen_();

	if (use_symmetry_) {
		bliss::Stats stats;
		auto obj = std::make_pair((int)epochs_.size()-1, this);
		logger << "detect and encode symmetry...";
		aut_ctr_ = 0;
		dg.find_automorphisms(stats, SolverHook, &obj);
		logger << "symmetry generators: " << aut_ctr_;
	}

	if (use_epoch_) {
		epoch_info_p->syms = aut_ctr_;
	}

	logger << "invoke SMT solver...";
	ull tic = unix_time_ms();
	bool sat;
	sat = check_sat_();
	logger << "solver stats:\n" << slv_.statistics();

	if (use_epoch_) {
		epoch_info_p->solver_time = unix_time_ms() - tic;
	}

	if (sat) {
		logger << "formula is sat";
		return false;
	} else {
		logger << "formula is unsat";
		return true;
	}
}

static inline std::string var_name(const std::string& t, const Call& c) {
	return t + "_" + std::to_string(c.idx);
}

static inline std::string var_name(const std::string& t, const Call& a,
	 const Call& b) {
	return t + "_" + std::to_string(a.idx) + "_" + std::to_string(b.idx);
}

void Verifier::init_encoding_vars_() {
	m_.reserve(calls_.size());
	r_.reserve(calls_.size());
	clk_.reserve(calls_.size());
	s_.reserve(calls_.size());
	for (Call& c: calls_) {
		m_.push_back(ctx_.bool_const(var_name("m", c).c_str()));
		r_.push_back(ctx_.bool_const(var_name("r", c).c_str()));
		clk_.push_back(ctx_.int_const(var_name("clk", c).c_str()));
		s_.emplace_back();
	}
	for (Call& c: calls_) {
		if (c.type != CallType::send) continue;
		for (int x: c.mplus) {
			Call& m = calls_[x];
			z3::expr tmp = ctx_.bool_const(var_name("s", c, m).c_str());
			s_[c.idx].insert({x, tmp});
			s_[x].insert({c.idx, tmp});
		}
	}
}

void Verifier::encode_gen_() {
	slv_.reset();
	z3::expr_vector m_all(ctx_);

	for (int x: epochs_.back()) {
		Call& a = calls_[x];
		m_all.push_back(m_[x]);
		slv_.add(z3::implies(m_[x], r_[x]));

		z3::expr_vector m_imm(ctx_);
		for (int y: a.preds) {
			Call& b = calls_[y];
			if (b.epoch != a.epoch) continue;
			m_imm.push_back(m_[y]);
			slv_.add(clk_[y] < clk_[x]);
		}
		slv_.add(r_[x] == z3::mk_and(m_imm));

		if (a.type == CallType::send || a.type == CallType::recv) {
			z3::expr_vector s_row(ctx_);
			for (int y: a.cnd_mplus) {
				s_row.push_back(s_[x].at(y));
			}
			if (a.cnd_mplus.empty()) {
			}
			assert(!a.cnd_mplus.empty());
			slv_.add(z3::atmost(s_row, 1));
			slv_.add(z3::implies(m_[x], z3::mk_or(s_row)));
		} else {
			slv_.add(m_[x] || !r_[x]);
		}

		if (a.type == CallType::send) {
			for (int y: a.cnd_mplus) {
				slv_.add(z3::implies(s_[x].at(y), clk_[x] == clk_[y] && m_[x] && m_[y]));
				slv_.add(m_[x] || !r_[x] || m_[y] || !r_[y]);
			}
		}
	}
	slv_.add(!z3::mk_and(m_all));
}

void Verifier::encode_aut_(int eid, unsigned n, const unsigned* aut) {
	std::vector< z3::expr > lhs, rhs;
	for (int i = 0; i < (int)epochs_[eid].size(); ++i) {
		if (calls_[epochs_[eid][i]].type != CallType::send) continue;
		Call& c = calls_[epochs_[eid][i]];
		for (int x: c.cnd_mplus) {
			int ix = inv_[x];
			if (i == (int)aut[i] && ix == (int)aut[ix]) continue;
			lhs.emplace_back(s_[c.idx].at(x));
			rhs.emplace_back(s_[epochs_[eid][aut[i]]].at(epochs_[eid][aut[ix]]));
		}
	}
	int sz = lhs.size();
	std::vector< z3::expr > aux;
	aux.reserve(sz);
	for (int i = 0; i < sz; ++i) {
		aux.emplace_back(ctx_.bool_const(
				("aux_" + std::to_string(aut_ctr_) + "_" + std::to_string(i)).c_str()
				));
	}
	if (0 < sz) {
		slv_.add(!lhs[0] || rhs[0]);
		slv_.add(aux[0] == (lhs[0] == rhs[0]));
	}
	for (int i = 1; i < sz; ++i) {
		slv_.add(z3::implies(aux[i-1], !lhs[i] || rhs[i]));
		if (i != sz-1) {
			slv_.add(aux[i] == (aux[i-1] && (lhs[i] == rhs[i])));
		}
	}
	++aut_ctr_;
}

void Verifier::preserve_conditions() {
	for (Call& c: calls_) {
		if (c.type != CallType::recv) continue;
		orig_cnds_.push_back({c.idx, {c.data_cnd, c.src_cnd, c.tag_cnd}});
	}
}

void Verifier::clear_conditions() {
	for (Call& c: calls_) {
		if (c.type != CallType::recv) continue;
		c.data_cnd.clear();
		c.src_cnd.clear();
		c.tag_cnd.clear();
	}
}

void Verifier::encode_feasibility_(int eid, const std::vector< int >& anchors) {
	slv_.reset();

	for (int x: epochs_[eid]) {
		Call& a = calls_[x];
		slv_.add(z3::implies(m_[x], r_[x]));

		z3::expr_vector m_imm(ctx_);
		for (int y: a.preds) {
			Call& b = calls_[y];
			if (b.epoch != a.epoch) continue;
			m_imm.push_back(m_[y]);
			slv_.add(clk_[y] < clk_[x]);
		}
		slv_.add(r_[x] == z3::mk_and(m_imm));

		if (a.type == CallType::send || a.type == CallType::recv) {
			z3::expr_vector s_row(ctx_);
			for (int y: a.cnd_mplus) {
				s_row.push_back(s_[x].at(y));
			}
			if (!s_row.empty()) slv_.add(z3::atmost(s_row, 1));
			slv_.add(z3::implies(m_[x], z3::mk_or(s_row)));
		}

		if (a.type == CallType::send) {
			for (int y: a.cnd_mplus) {
				slv_.add(z3::implies(s_[x].at(y), clk_[x] == clk_[y] && m_[x] && m_[y]));
			}
		}
	}

	for (int x: anchors) {
		slv_.add(m_[x]);
	}
}

void Verifier::recompute_cnd_mplus_(int eid) {
	for (int x: epochs_[eid]) {
		Call& c = calls_[x];
		if (c.type == CallType::send) {
			c.cnd_mplus.clear();
			for (int m: c.mplus) {
				Call& b = calls_[m];
				if (b.data_cnd.check(c.data)
						&& b.src_cnd.check(c.pid)
						&& b.tag_cnd.check(c.tag)) {
					c.cnd_mplus.push_back(m);
				}
			}
		} else if (c.type == CallType::recv) {
			c.cnd_mplus.clear();
			for (int m: c.mplus) {
				Call& b = calls_[m];
				if (c.data_cnd.check(b.data)
						&& c.src_cnd.check(b.pid)
						&& c.tag_cnd.check(b.tag)) {
					c.cnd_mplus.push_back(m);
				}
			}
		}
	}
}

bool Verifier::check_feasibility_epoch_(int eid, const std::vector< int >& anchors) {
	recompute_cnd_mplus_(eid);

	logger << "construct new program graph for epoch...";
	bliss::Digraph dg(epochs_[eid].size());
	for (int x: epochs_[eid]) {
		Call& c = calls_[x];
		for (int y: c.preds) {
			Call& p = calls_[y];
			if (p.epoch != c.epoch) continue;
			dg.add_edge(inv_[p.idx], inv_[c.idx]);
		}
		for (int y: c.cnd_mplus) {
			dg.add_edge(inv_[x], inv_[y]);
			dg.add_edge(inv_[y], inv_[x]);
		}
	}

	int w = dg.add_vertex(1);
	for (int x: anchors) {
		dg.add_edge(inv_[x], w);
	}

	logger << "create smt formula for feasibility...";
	encode_feasibility_(eid, anchors);
	bliss::Stats stats;
	auto obj = std::make_pair(eid, this);
	logger << "detect and encode symmetry...";
	aut_ctr_ = 0;
	dg.find_automorphisms(stats, SolverHook, &obj);
	logger << "symmetry generators: " << aut_ctr_;
	path_epoch_info.emplace_back(epochs_[eid].size(), aut_ctr_);
	//statf << "path epoch symmetry generators: " << aut_ctr_ << std::endl;

	ull tic = unix_time_ms();
	bool sat = check_sat_();
	//statf << "path epoch solver time: " << unix_time_ms() - tic << std::endl;
	path_solver_time += unix_time_ms() - tic;

	if (!sat) {
		logger << "formula is unsat";
		return false;
	} else {
		logger << "formula is sat";
		return true;
	}
}

bool Verifier::check_feasibility() {
	std::map< int, std::vector< int >> emap;
	logger << "identify changed epochs...";
	for (auto& p: orig_cnds_) {
		Call& c = calls_[p.first];
		Condition& orig_data_cnd = p.second[0];
		Condition& orig_src_cnd = p.second[1];
		Condition& orig_tag_cnd = p.second[2];
		if (c.data_cnd == orig_data_cnd
				&& c.src_cnd == orig_src_cnd
				&& c.tag_cnd == orig_tag_cnd) continue;
		emap[c.epoch].push_back(c.idx);
	}
	//statf << "path changed epochs: " << emap.size() << std::endl;

	bool res;
	for (auto& p: emap) {
		int eid = p.first;
		logger << "changed epoch: " << eid;
		//statf << "path epoch size: " << epochs_[eid].size() << std::endl;
		std::vector< int >& anchors = p.second;
		res = check_feasibility_epoch_(eid, anchors);
		if (!res) break;
	}
	return res;
}

bool Verifier::verify() {
	statf << "trace size: " << calls_.size() << std::endl;
	logger << "compute m+...";
	merge_barrs_();
	basic_mplus_();

	if (use_epoch_) {
		compute_ancestors_();
		prune_mplus_by_ancs_();
		prune_mplus_by_mo_();
		prune_mplus_by_count_();
	}

	compute_cnd_mplus_();
	init_encoding_vars_();
	pre_epoch_();

	bool res;
	if (use_epoch_) {
		logger << "epoch decomposition...";

		while (true) {
			bool nxt;
			logger << "";
			logger << "extract epoch...";
			nxt = next_epoch_();
			if (!nxt) {
				logger << "epochs exhausted";
				break;
			}

			logger << "verify epoch...";
			res = verify_epoch_();
			logger << (res ? "deadlock free epoch" : "deadlock present in epoch");
			if (!res) break;
		}
	} else {
		logger << "epoch decomposition disabled.";

		epochs_.emplace_back();
		for (Call& c: calls_) {
			epochs_.back().push_back(c.idx);
			c.epoch = 0;
		}

		res = verify_epoch_();
		logger << (res ? "deadlock free epoch" : "deadlock present in epoch");
	}

	//statf << "trace total epochs: " << (int)epochs_.size()-1 << std::endl;

	std::string pathp = "runlogs/" + dirname + "/procs.dot";
	std::ofstream ofsp(pathp);
	write_dot_prog(ofsp);
	logger << "process graph written in: " << pathp;

	std::string pathe = "runlogs/" + dirname + "/epochs.dot";
	std::ofstream ofse(pathe);
	write_dot_epoch(ofse);
	logger << "epoch graph written in: " << pathe;

	return res;
}

