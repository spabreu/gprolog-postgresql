x_connect :- pq_open("", 0, "coolpix", CH), g_assign(ch, CH).

x_get(R) :- x_get("select * from pix order by number", R).
x_get(S,R) :-
	g_read(ch, CH),
	pq_exec(CH, S, SH),
	pq_fetch(SH),
	pq_get_data_int(SH, 1, NUMBER),
	pq_get_data_codes(SH, 2, FILE),
	pq_get_data_atom(SH, 3, CAM),
	pq_get_data_atom(SH, 4, MET),
	pq_get_data_atom(SH, 5, MODE),
	pq_get_data_float(SH, 6, SHUTTER),
	pq_get_data_float(SH, 7, AP),
	pq_get_data_float(SH, 8, EXP),
	pq_get_data_float(SH, 9, FOCAL),
	pq_get_data_float(SH, 10, MULT),
	pq_get_data_atom(SH, 11, ADJUST),
	pq_get_data_atom(SH, 12, SENS),
	pq_get_data_atom(SH, 13, WHITE),
	pq_get_data_atom(SH, 14, SHARP),
	pq_get_data_date(SH, 15, DATE),
	pq_get_data_atom(SH, 16, QUAL),
	R=[NUMBER,FILE,CAM,MET,MODE,SHUTTER,AP,EXP,FOCAL,MULT,ADJUST,SENS,WHITE,SHARP,DATE,QUAL].

x_get2(R1, R2) :- x_get(R1), !, x_get(R2).

olá :- write(olá), nl.
