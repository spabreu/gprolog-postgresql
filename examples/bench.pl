:- initialization(benchmark).

benchmark :-
	statistics(user_time, _),
	statistics(system_time, _),
	statistics(real_time, _),
	x_connect,
	argument_list([COUNT_, BINARY_]),
	read_term_from_atom(COUNT_, COUNT, [end_of_term(eof)]),
	read_term_from_atom(BINARY_, BINARY, [end_of_term(eof)]),
	pq_set_binary(0, BINARY),
	for(_, 1, COUNT),
	x_get(_),
	fail.
benchmark :-
	statistics(user_time, [UT,_]),
	statistics(system_time, [ST,_]),
	statistics(real_time, [RT,_]),
	format("user %d, system %d, elapsed %d\n", [UT, ST, RT]),
	halt.
