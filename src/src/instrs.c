#ifndef FAST

char *ArglessInstrs[] = {
	"NOOP",		/* 0 */
	"PLUS",
	"NEGATE",
	"EQ?",
	"NOT",
	"TIMES",
	"LOAD-GLO",
	"DIV",
	"=0?",
	"GET-TAG",
	"GET-DATA",		/* 10 */
	"CRUNCH",
	"GETC",
	"PUTC",
	"CONTENTS",
	"SET-CONTENTS",
	"LOAD-TYPE",
	"CONS",
	"<0?",
	"MODULO",
	"ASH",		/* 20 */
	"ROT",
	"STORE-BP-I",
	"LOAD-BP-I",
	"RETURN",
	"ALLOCATE",
	"ASSQ",
	"LOAD-LENGTH",
	"PEEK",
	"POKE",
	"MAKE-CELL",		/* 30 */
	"SUBTRACT",
	"=",
	"<",
	"BIT-NOT",
	"LONG-BRANCH",
	"LONG-BRANCH-NIL",
	"LONG-BRANCH-T",
	"LOCATE-BP-I",
	"LOAD-GLO-CON",
	"CAR",		/* 40 */
	"CDR",
	"SET-CAR",
	"SET-CDR",
	"LOCATE-CAR",
	"LOCATE-CDR",
	"PUSH-CXT-LONG",
	"CALL-PRIMITIVE",
	"THROW",
	"OBJECT-HASH",
	"OBJECT-UNHASH",		/* 50 */
	"GC",
	"BIG-ENDIAN?",
	"VLEN-ALLOCATE",
	"INC-LOC",
	"FILL-CONTINUATION",
	"CONTINUE",
	"REVERSE-CONS",
	"MOST-NEGATIVE-FIXNUM?",
	"FX-PLUS",
	"FX-TIMES",		/* 60 */
	"GET-TIME",
	"REMAINDER",
	"QUOTIENTM",
	"FULL-GC",
	"MAKE-LAMBDA",
};

char *Instrs[] = {
	"#<Undefined IVAR 1039>",		/* 0 */
	"HALT",
	"LOG-OP",
	"BLT-STK",
	"BRANCH-NIL",
	"BRANCH-T",
	"BRANCH",
	"POP",
	"SWAP",
	"BLAST",
	"LOAD-IMM-FIX",		/* 10 */
	"STORE-STK",
	"LOAD-BP",
	"STORE-BP",
	"LOAD-ENV",
	"STORE-ENV",
	"LOAD-STK",
	"MAKE-BP-LOC",
	"MAKE-ENV-LOC",
	"STORE-REG",
	"LOAD-REG",		/* 20 */
	"FUNCALL-CXT",
	"FUNCALL-TAIL",
	"STORE-NARGS",
	"CHECK-NARGS",
	"CHECK-NARGS-GTE",
	"STORE-SLOT",
	"LOAD-SLOT",
	"MAKE-CLOSED-ENVIRONMENT",
	"PUSH-CXT",
	"LOCATE-SLOT",		/* 30 */
	"STREAM-PRIMITIVE",
	"FILLTAG",
	"^SUPER-CXT",
	"^SUPER-TAIL",
};

#endif FAST

