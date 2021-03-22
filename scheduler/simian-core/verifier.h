/*
 * verifier.h: classes and declarations for the core logic of Simian
 * author: Rishabh Ranjan
 */

#pragma once

// graph.hh from BLISS
#include "graph.hh"
// z3 theorem prover API for C++
#include "z3++.h"

#include <array>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/* operators supported in conditions */
enum class Operator {
	equal,
	not_equal,
	less,
	greater,
	less_or_equal,
	greater_or_equal
};

/* a condition is a conjunction of binary operators with an int as rhs */
class Condition {
	std::vector< std::pair< Operator, int >> and_;

public:
	bool operator==(const Condition& c) {
		return and_ == c.and_;
	}

	void add(Operator op, int val) {
		and_.emplace_back(op, val);
	}

	void clear() {
		and_.clear();
	}

	bool check(int x) {
		for (const auto& e: and_) {
			int v = e.second;
			switch (e.first) {
				case Operator::equal:
					if (x != v) return false;
					break;
				case Operator::not_equal:
					if (x == v) return false;
					break;
				case Operator::less:
					if (x >= v) return false;
					break;
				case Operator::greater:
					if (x <= v) return false;
					break;
				case Operator::less_or_equal:
					if (x > v) return false;
					break;
				case Operator::greater_or_equal:
					if (x < v) return false;
					break;
			}
		}
		return true;
	}

	friend std::ostream& operator<<(std::ostream&, const Condition&);
};

enum class CallType {
	send,
	recv,
	raw_barr,
	merged_barr,
	misc
};

struct Call {
	int idx;					// index in internal representation
	int pid;					// rank of process to which it belongs
	int loc;					// index of the call among calls in the same process
	int comm;					// communicator id
	int bid;					// barrier id among same-communicator barriers
	int epoch;				// epoch id
	CallType type;
	int data;					// data for send type calls
	int nbr;					// communication partner pid (or -1) for send/recv calls
	int tag;					// tag (or -1) for send/recv calls
	Condition data_cnd;						// data condition for recv calls
	Condition src_cnd;						// source pid condition for recv calls
	Condition tag_cnd;						// tag condition for recv calls
	std::vector< int > preds;			// predecessors in the matches-before order
	std::vector< int > succs;			// successors in the matches-before order
	std::vector< int > ancs;			// all ancestors in the matches-before order
	std::vector< int > mplus;			// potential matches ignoring conditions
	std::vector< int > cnd_mplus;	// potential matches considering conditions
};

std::ostream& operator<<(std::ostream&, const Call&);

// MPI calls belonging to each type
// can be extended
static const std::vector< std::string > send_vec = {
	"Send", "Isend", "Ssend"
};

static const std::vector< std::string > recv_vec = {
	"Recv", "Irecv"
};

static const std::vector< std::string > barr_vec = {
	"Barrier"
};

static const std::vector< std::string > misc_vec = {
	"Wait", "Waitall",
	"Bcast", "Gather", "Scatter", "Reduce", "Allgather", "Allreduce",
	"Comm_create", "Comm_split", "Comm_dup", "Comm_free", "Cart_create"
};

/* collect info about epochs for debugging/diagnostic purposes */
struct EpochInfo {
	bliss::Digraph dg;		// digraph characterizing epoch
	int size;							// number of calls in the epoch
	int syms;							// number of symmetry generators returned by BLISS
	unsigned long long solver_time;		// time to solve epoch by z3 (in ms) 
	int reps;							// repetitions of this epoch seen
	EpochInfo(bliss::Digraph& dg): dg(dg), size(0), syms(0), reps(0) {}
};

/* verification using symmetry and epoch-decomposition + feasibility check */
class Verifier {

	/* convert call name to call type */
	static CallType conv_type_(const std::string& str) {
		for (const std::string& name: send_vec) {
			if (str == name) return CallType::send;
		}
		for (const std::string& name: recv_vec) {
			if (str == name) return CallType::recv;
		}
		for (const std::string& name: barr_vec) {
			if (str == name) return CallType::raw_barr;
		}
		for (const std::string& name: misc_vec) {
			if (str == name) return CallType::misc;
		}

		std::cerr << str << ": unknown call.\nAdd to verifier.h and retry.\n"; 
		exit(1);
	}

	/* reference to hash-table of already verified epochs */
	std::unordered_map< int, std::vector< EpochInfo >>& verified_hashes_;

	/* all calls laid out linearly */
	std::vector< Call > calls_;
	/* pid_offset_[i] = index of first call with pid = i in calls_ */
	std::vector< int > pid_offset_;
	/* (pid, loc) to index of calls_ */
	int index_(int pid, int loc) {
		return pid_offset_[pid] + loc;
	}

	/* merges raw_barr into merged_barr calls at the end of calls_ */
	void merge_barrs_();

	/* bitmask representation of ancestors for calls (in matches-before order) */
	//NOTE: a call is NOT an ancestor of itself
	std::vector< std::vector< uint64_t >> ancestors_;
	/* dfs to compute ancestors */
	void ancestor_dfs_(std::vector< bool >&, int);
	/* compute ancestors for all calls, fills the ancs fields */
	void compute_ancestors_();
	/* is y an ancestor of x */
	bool has_ancestor_(int x, int y) {
		return (ancestors_[x][y/64]>>(y%64))&1;
	}

	/* mplus taking only compatibility of send/recv into account */
	void basic_mplus_();
	/* mplus with pairs with ancestor relation removed
	 * handles barrier-related pruning as it uses merged barriers */
	void prune_mplus_by_ancs_();
	/* removes b from a.mplus if a is not in b.mplus, for all calls */
	void mplus_clean_();
	/* a single pass of prune_mplus_by_mo_ computation
	 * return true if mplus changes, false otherwise */
	bool prune_mplus_by_mo_pass_();
	/* prune mplus using the mo-based criteria in FM14/FM18 */
	void prune_mplus_by_mo_();
	/* prune mplus using a counting argument on sets of potential matches */
	void prune_mplus_by_count_();

	/* compute the cnd_mplus for all calls */
	void compute_cnd_mplus_();
	/* recompute the cnd_mplus for all calls in epoch #eid */
	void recompute_cnd_mplus_(int eid);

	/* vector of all epochs in order of discovery */
	std::vector< std::vector< int >> epochs_;
	/* inverse of the current epoch vector (epochs_.back()) */
	std::vector< int > inv_;
	/* order of calls for SCC computation */
	std::vector< int > order_;
	/* dfs to compute order */
	void order_dfs_(std::vector< bool >&, int);
	/* pre-processing before start of epoch extraction */
	void pre_epoch_();
	/* extract next epoch; returns false if no more epochs, else true */
	bool next_epoch_();
	/* returns true iff the epochs is verified to not contain deadlocks */
	bool verify_epoch_();
	/* returns true iff epoch eid with changed conditions is feasible */
	bool check_feasibility_epoch_(int, const std::vector< int >&);

	/* z3 context */
	z3::context ctx_;
	/* m, r, clk smt-variables for each call */
	std::vector< z3::expr > m_;
	std::vector< z3::expr > r_;
	std::vector< z3::expr > clk_;
	/* s smt-variables s_[send-call-idx].at(recv-call-idx) */
	std::vector< std::unordered_map< int, z3::expr >> s_;
	/* create the smt-variables m, r, clk, s */
	void init_encoding_vars_();
	/* z3 solver */
	z3::solver slv_;
	/* count of number of automorphisms encoded */
	int aut_ctr_;

	/* encode the general smt constraints for deadlock detection */
	//TODO: add constraints for conditional matches-before
	void encode_gen_();
	/* encode the automorphism aut interpreted for epoch #eid */
	void encode_aut_(int eid, unsigned n, const unsigned* aut);
	/* run the z3 solver */
	bool check_sat_() {
		switch (slv_.check()) {
			case z3::sat: return true;
			case z3::unsat: return false;
			default:
				std::cerr << "z3 returned 'unknown': possibly interrupted\n";
				exit(1);
		}
	}

	/* vector of (pid, (data_cnd, src_cnd, tag_cnd)) recording original cnds */
	std::vector< std::pair< int, std::array< Condition, 3 >>> orig_cnds_;
	/* encode the feasibility of epoch #eid under changed conditions
	 * anchors are calls whose conditions have changed */
	void encode_feasibility_(int eid, const std::vector< int >& anchors);

public:
	explicit Verifier(
			std::unordered_map< int, std::vector< EpochInfo >>& verified_hashes):
		verified_hashes_(verified_hashes),
		ctx_(), slv_(ctx_), aut_ctr_(0) {}

	/* push a new process
	 * push_proc() should also be called after the last call */
	void push_proc() {
		pid_offset_.push_back(calls_.size());
	}

	/* push a new call */
	void push_call(int comm, const std::string& type_str,
			int data, int src, int dest, int rtag, int stag,
			const std::vector< int >& preds); 

	void add_data_condition(int pid, int loc, Operator op, int val) {
		calls_[index_(pid, loc)].data_cnd.add(op, val);
	}

	void add_src_condition(int pid, int loc, Operator op, int val) {
		calls_[index_(pid, loc)].src_cnd.add(op, val);
	}

	void add_tag_condition(int pid, int loc, Operator op, int val) {
		calls_[index_(pid, loc)].tag_cnd.add(op, val);
	}

	/* verify that current set of calls and conditions don't deadlock */
	bool verify();

	/* record current conditions as original conditions (in orig_cnds_) */
	void preserve_conditions();
	/* clear all conditions */
	void clear_conditions();
	/* check if the changed conditions result in a feasible path */
	bool check_feasibility();

	/* hook for find_automorphisms of BLISS to call the encode_aut of verifier */
	friend void SolverHook(void* user_param, unsigned n, const unsigned* aut);

	/* print info about all distinct epochs */
	void print_epoch_infos(std::ostream&);
	void print_epoch_bc(std::ostream&);
	/* write out the program graph in DOT format */
	void write_dot_prog(std::ostream&);
	/* write out the epoch graph in DOT format */
	void write_dot_epoch(std::ostream&);
	/* write all calls for debugging purposes */
	void write_calls(std::ostream&);
};
