:- initialization(go).

go :-
	argument_list([COUNT_, BINARY_]),
	read_term_from_atom(COUNT_, COUNT, [end_of_term(eof)]),
	read_term_from_atom(BINARY_, BINARY, [end_of_term(eof)]),
	benchmark(COUNT, BINARY).

benchmark(COUNT, BINARY) :-
	statistics(user_time, _),
	statistics(system_time, _),
	statistics(real_time, _),
	x_connect,
	pq_set_binary(0, BINARY),
	for(_, 1, COUNT),
	x_get(_),
	fail.
benchmark(COUNT, BINARY) :-
	statistics(user_time, [UT,_]),
	statistics(system_time, [ST,_]),
	statistics(real_time, [RT,_]),
	format("%d\t%d\t%d\t%d\t%d\n", [COUNT, BINARY, UT, ST, RT]),
	halt.
