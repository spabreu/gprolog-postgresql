:- initialization(go).

go :-
	argument_list([COUNT_, BINARY_, GOAL_]),
	read_term_from_atom(COUNT_, COUNT, [end_of_term(eof)]),
	read_term_from_atom(BINARY_, BINARY, [end_of_term(eof)]),
	GOAL=..[GOAL_,_],
	!,
	benchmark(COUNT, BINARY, GOAL).

benchmark(COUNT, BINARY, GOAL) :-
	statistics(user_time, _),
	statistics(system_time, _),
	statistics(real_time, _),
	x_connect,
	pq_set_binary(0, BINARY),
	for(_, 1, COUNT),
	GOAL,
	fail.
benchmark(COUNT, BINARY, GOAL) :-
	statistics(user_time, [UT,_]),
	statistics(system_time, [ST,_]),
	statistics(real_time, [RT,_]),
	functor(GOAL,P,A),
	format("~w\t%d\t%d\t%d\t%d\t%d\n", [P/A, COUNT, BINARY, UT, ST, RT]),
	halt.
