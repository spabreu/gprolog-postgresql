% sample code

x_connect :- pq_open("", 0, "pix", CH), g_assign(ch, CH).

x_date(R) :- x_date("select * from ea_date", R).
x_date(Q, ea_date(NUMBER, TAG, VALUE)) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_int(SH, 1, NUMBER),
    pq_get_data_int(SH, 2, TAG),
    pq_get_data_date(SH, 3, VALUE).

x_number(R) :- x_number("select * from ea_num", R).
x_number(Q, ea_number(NUMBER, TAG, VALUE)) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_int(SH, 1, NUMBER),
    pq_get_data_int(SH, 2, TAG),
    pq_get_data_float(SH, 3, VALUE).

x_int(R) :- x_int("select * from ea_int", R).
x_int(Q, ea_int(NUMBER, TAG, VALUE)) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_int(SH, 1, NUMBER),
    pq_get_data_int(SH, 2, TAG),
    pq_get_data_int(SH, 3, VALUE).

x_codes(R) :- x_codes("select * from ea_text", R).
x_codes(Q, ea_codes(NUMBER, TAG, VALUE)) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_int(SH, 1, NUMBER),
    pq_get_data_int(SH, 2, TAG),
    pq_get_data_codes(SH, 3, VALUE).

x_atom(R) :- x_atom("select * from ea_text", R).
x_atom(Q, ea_atom(NUMBER, TAG, VALUE)) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_int(SH, 1, NUMBER),
    pq_get_data_int(SH, 2, TAG),
    pq_get_data_atom(SH, 3, VALUE).



y_date(R) :- y_date("select value from ea_date", R).
y_date(Q, VALUE) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_date(SH, 1, VALUE).

y_number(R) :- y_number("select value from ea_num", R).
y_number(Q, VALUE) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_float(SH, 1, VALUE).

y_int(R) :- y_int("select value from ea_int", R).
y_int(Q, VALUE) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_int(SH, 1, VALUE).

y_codes(R) :- y_codes("select value from ea_text", R).
y_codes(Q, VALUE) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_codes(SH, 1, VALUE).

y_atom(R) :- y_atom("select value from ea_text", R).
y_atom(Q, VALUE) :-
    g_read(ch, CH),
    pq_exec(CH, Q, SH),
    pq_fetch(SH),
    pq_get_data_atom(SH, 1, VALUE).
