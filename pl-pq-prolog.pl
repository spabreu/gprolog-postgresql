% $Id$

/* ----------------------------------------------------------------------------
 * GPROLOG-POSTGRESQL is Copyright (C) 1999-2004 Salvador Abreu
 * 
 *    This program  is free software;  you can redistribute  it and/or
 *    modify  it under  the terms  of  the GNU  Lesser General  Public
 *    License  as published  by the  Free Software  Foundation; either
 *    version 2.1, or (at your option) any later version.
 * 
 *    This program is distributed in  the hope that it will be useful,
 *    but WITHOUT  ANY WARRANTY; without even the  implied warranty of
 *    MERCHANTABILITY or  FITNESS FOR  A PARTICULAR PURPOSE.   See the
 *    GNU Lesser General Public License for more details.
 * 
 *    You should have received a copy of the GNU Lesser General Public
 *    License  along with  this program;  if  not, write  to the  Free
 *    Software Foundation, Inc., 59  Temple Place - Suite 330, Boston,
 *    MA 02111-1307, USA.
 * 
 * On Debian  GNU/Linux systems, the  complete text of the  GNU Lesser
 * General Public License is found in /usr/share/common-licenses/LGPL.
 * ----------------------------------------------------------------------------
 */

% == predicate declarations for GNU Prolog PostgreSQL interface ===============

:- foreign(pq_open(+codes, +integer, +codes, -integer)).
:- foreign(pq_close(+integer)).

:- foreign(pq_begin(+integer)).
:- foreign(pq_end(+integer, +integer)).

:- foreign(pq_get_binary(+integer, -integer)).
:- foreign(pq_set_binary(+integer, +integer)).

:- foreign(pq_exec(+integer, +codes, -integer), [choice_size(1)]).
:- foreign(pq_fetch(+integer)).

:- foreign(pq_ntuples(+integer, -integer)).
:- foreign(pq_last_oid(+integer, -integer)).

% -- Value retrieval predicates -----------------------------------------------

pq_get_data(HANDLE, INDEX, TYPE, VALUE) :-
	pq_sql_type(TYPEX, TYPE), !,
	pq_get_data_aux(TYPEX, HANDLE, INDEX, VALUE).

% -- SQL data type codes defined in sql.h -------------------------------------

pq_sql_type(unknown,	0).
pq_sql_type(char,	1).
pq_sql_type(numeric,	2).
pq_sql_type(decimal,	3).
pq_sql_type(integer,	4).
pq_sql_type(smallint,	5).
pq_sql_type(float,	6).
pq_sql_type(real,	7).
pq_sql_type(double,	8).
pq_sql_type(datetime,	9).
pq_sql_type(timestamp,	11).
pq_sql_type(varchar,	12).
pq_sql_type(bit,        13).
pq_sql_type(bitvar,     14).
pq_sql_type(bool,       15).
pq_sql_type(abstract,   16).

pq_get_data_aux(integer,H,X,V) :- pq_get_data_int(H,X,V).
pq_get_data_aux(smallint,H,X,V) :- pq_get_data_int(H,X,V).

pq_get_data_aux(float,H,X,V) :- pq_get_data_float(H,X,V).
pq_get_data_aux(real,H,X,V) :- pq_get_data_float(H,X,V).
pq_get_data_aux(double,H,X,V) :- pq_get_data_float(H,X,V).

pq_get_data_aux(bool,H,X,V) :- pq_get_data_bool(H,X,V).

pq_get_data_aux(text,H,X,V) :- pq_get_data_atom(H,X,V).
pq_get_data_aux(varchar,H,X,V) :- pq_get_data_atom(H,X,V).

pq_get_data_aux(datetime,H,X,V) :- pq_get_data_date(H,X,V).
pq_get_data_aux(timestamp,H,X,V) :- pq_get_data_date(H,X,V).


:- foreign(pq_get_data_int(+integer, +integer, -integer)).
:- foreign(pq_get_data_float(+integer, +integer, -float)).
:- foreign(pq_get_data_bool(+integer, +integer, -boolean)).
:- foreign(pq_get_data_date(+integer, +integer, -term)).

% -- These all map into a single C function -----------------------------------

:- foreign(pq_get_data_codes(+integer, +integer, -codes),
	   [fct_name(pq_get_data_string)]).

:- foreign(pq_get_data_atom(+integer, +integer, -string),
	   [fct_name(pq_get_data_string)]).

% $Log$
% Revision 1.3  2004/04/27 09:22:40  spa
% - Changed from GPL to LGPL.
% - Added binary-format support predicates.
%
% Revision 1.2  2003/03/07 21:59:57  spa
% *** empty log message ***
%
% Revision 1.1.1.1  2003/01/07 19:56:02  spa
% Initial import
%
% Revision 1.4  2002/04/23 15:58:43  spa
% *** empty log message ***
%
% Revision 1.3  2001/12/05 08:30:11  spa
% pq_get_data() rewritten as a bunch of type-specific functions.
%
% Revision 1.2  2001/08/23 23:49:49  spa
% SQL types are no longer here, as this comes together with the ODBC package.
%

% Local variables:
% mode: prolog
% mode: font-lock
% End:
