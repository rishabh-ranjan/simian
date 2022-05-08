## Future dev work:

1. Improve Hermes frontend (clangTool):
	1.1. can handle single source file programs only
	1.2. variables names used in interumentation code are too common -- clashes with the identifiers in the input program
	1.3. can handle only very simple if conditions
	1.4. static analysis used is very fragile

2. Generate deadlocking trace from SMT solution

3. Map detected deadlock to relevant code sections (epochs can help to localize the bug)

4. User-friendly interface (GUI) to explore the deadlocks/epochs/symmetries present in the program in a graphical manner

5. Implement support for conditional matches-before order of MPI semantics

6. Remove unnecessary code, especially from SMTEncoding.cpp since most of it is now obsolete (make will show compiler warnings for the unused variables in SMTEncoding.cpp).

7. Optimize processing (linear instead of quadratic)

8. Remove SMTEncoding altogether and refactor to merge into Simian
