" Vim syntax file
" Language:	Oaklisp
" Last Change:	2016-04-04
" Maintainer:	<bernard.tatin@outlook.fr>
" Original author:	Dirk van Deun <dirk@igwe.vub.ac.be>


" Suggestions and bug reports are solicited by the author.

" Initializing:

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn case ignore

" Fascist highlighting: everything that doesn't fit the rules is an error...

syn match	oakError	![^ \t()\[\]";]*!
syn match	oakError	")"

" Quoted and backquoted stuff

syn region oakQuoted matchgroup=Delimiter start="['`]" end=![ \t()\[\]";]!me=e-1 contains=ALLBUT,oakStruc,oakSyntax,oakFunc

syn region oakQuoted matchgroup=Delimiter start="['`](" matchgroup=Delimiter end=")" contains=ALLBUT,oakStruc,oakSyntax,oakFunc
syn region oakQuoted matchgroup=Delimiter start="['`]#(" matchgroup=Delimiter end=")" contains=ALLBUT,oakStruc,oakSyntax,oakFunc

syn region oakStrucRestricted matchgroup=Delimiter start="(" matchgroup=Delimiter end=")" contains=ALLBUT,oakStruc,oakSyntax,oakFunc
syn region oakStrucRestricted matchgroup=Delimiter start="#(" matchgroup=Delimiter end=")" contains=ALLBUT,oakStruc,oakSyntax,oakFunc

" Popular Scheme extension:
" using [] as well as ()
syn region oakStrucRestricted matchgroup=Delimiter start="\[" matchgroup=Delimiter end="\]" contains=ALLBUT,oakStruc,oakSyntax,oakFunc
syn region oakStrucRestricted matchgroup=Delimiter start="#\[" matchgroup=Delimiter end="\]" contains=ALLBUT,oakStruc,oakSyntax,oakFunc

syn region oakUnquote matchgroup=Delimiter start="," end=![ \t\[\]()";]!me=e-1 contains=ALLBUT,oakStruc,oakSyntax,oakFunc
syn region oakUnquote matchgroup=Delimiter start=",@" end=![ \t\[\]()";]!me=e-1 contains=ALLBUT,oakStruc,oakSyntax,oakFunc

syn region oakUnquote matchgroup=Delimiter start=",(" end=")" contains=ALL
syn region oakUnquote matchgroup=Delimiter start=",@(" end=")" contains=ALL

syn region oakUnquote matchgroup=Delimiter start=",#(" end=")" contains=ALLBUT,oakStruc,oakSyntax,oakFunc
syn region oakUnquote matchgroup=Delimiter start=",@#(" end=")" contains=ALLBUT,oakStruc,oakSyntax,oakFunc

syn region oakUnquote matchgroup=Delimiter start=",\[" end="\]" contains=ALL
syn region oakUnquote matchgroup=Delimiter start=",@\[" end="\]" contains=ALL

syn region oakUnquote matchgroup=Delimiter start=",#\[" end="\]" contains=ALLBUT,oakStruc,oakSyntax,oakFunc
syn region oakUnquote matchgroup=Delimiter start=",@#\[" end="\]" contains=ALLBUT,oakStruc,oakSyntax,oakFunc


if version < 600
  set iskeyword=33,35-39,42-58,60-90,94,95,97-122,126,_
else
  setlocal iskeyword=33,35-39,42-58,60-90,94,95,97-122,126,_
endif

syn keyword oakSyntax get-type make initialize type is-a? subtype?
syn keyword oakSyntax nil coercer coercable-type mix-types mixin-manager
syn keyword oakSyntax define-constant define-instance define-constant-instance add-method 
syn keyword oakSyntax local-syntax define-local-syntax locale
syn keyword oakSyntax variable? variable-here? macro? macro-here?
syn keyword oakSyntax frozen? frozen-here? bind fluid 
syn keyword oakSyntax catch native-catch throw 
syn keyword oakSyntax wind-protect dynamic-wind funny-wind-protect
syn keyword oakSyntax warning error cerror error-return error-restart
syn keyword oakSyntax bind-error-handler 
syn keyword oakSyntax report invoke-debugger proceed remember-context
syn keyword oakSyntax invoke-in-error-context
syn keyword oakSyntax iterate block block0 dotimes dolist dolist-count
syn keyword oakSyntax mapcdr for-each for-each-cdr

syn keyword oakSyntax rest-length consume-args listify-args 
syn keyword oakSyntax setter make-locative locater contents
syn keyword oakSyntax operation settable-operation locatable-operation
syn keyword oakSyntax swap modify modify-location
syn keyword oakSyntax compile-file 


syn keyword oakSyntax lambda and or if cond case define let let* letrec
syn keyword oakSyntax begin do delay set! else =>
syn keyword oakSyntax quote quasiquote unquote unquote-splicing
syn keyword oakSyntax define-syntax let-syntax letrec-syntax syntax-rules
" R6RS
syn keyword oakSyntax define-record-type fields protocol

syn keyword oakFunc not boolean? eq? eqv? equal? pair? cons car cdr set-car!
syn keyword oakFunc set-cdr! caar cadr cdar cddr caaar caadr cadar caddr
syn keyword oakFunc cdaar cdadr cddar cdddr caaaar caaadr caadar caaddr
syn keyword oakFunc cadaar cadadr caddar cadddr cdaaar cdaadr cdadar cdaddr
syn keyword oakFunc cddaar cddadr cdddar cddddr null? list? list length
syn keyword oakFunc append reverse list-ref memq memv member assq assv assoc
syn keyword oakFunc symbol? symbol->string string->symbol number? complex?
syn keyword oakFunc real? rational? integer? exact? inexact? = < > <= >=
syn keyword oakFunc zero? positive? negative? odd? even? max min + * - / abs
syn keyword oakFunc quotient remainder modulo gcd lcm numerator denominator
syn keyword oakFunc floor ceiling truncate round rationalize exp log sin cos
syn keyword oakFunc tan asin acos atan sqrt expt make-rectangular make-polar
syn keyword oakFunc real-part imag-part magnitude angle exact->inexact
syn keyword oakFunc inexact->exact number->string string->number char=?
syn keyword oakFunc char-ci=? char<? char-ci<? char>? char-ci>? char<=?
syn keyword oakFunc char-ci<=? char>=? char-ci>=? char-alphabetic? char?
syn keyword oakFunc char-numeric? char-whitespace? char-upper-case?
syn keyword oakFunc char-lower-case?
syn keyword oakFunc char->integer integer->char char-upcase char-downcase
syn keyword oakFunc string? make-string string string-length string-ref
syn keyword oakFunc string-set! string=? string-ci=? string<? string-ci<?
syn keyword oakFunc string>? string-ci>? string<=? string-ci<=? string>=?
syn keyword oakFunc string-ci>=? substring string-append vector? make-vector
syn keyword oakFunc vector vector-length vector-ref vector-set! procedure?
syn keyword oakFunc apply map for-each call-with-current-continuation
syn keyword oakFunc call-with-input-file call-with-output-file input-port?
syn keyword oakFunc output-port? current-input-port current-output-port
syn keyword oakFunc open-input-file open-output-file close-input-port
syn keyword oakFunc close-output-port eof-object? read read-char peek-char
syn keyword oakFunc write display newline write-char call/cc
syn keyword oakFunc list-tail string->list list->string string-copy
syn keyword oakFunc string-fill! vector->list list->vector vector-fill!
syn keyword oakFunc force with-input-from-file with-output-to-file
syn keyword oakFunc char-ready? load transcript-on transcript-off eval
syn keyword oakFunc dynamic-wind port? values call-with-values
syn keyword oakFunc oak-report-environment null-environment
syn keyword oakFunc interaction-environment
" R6RS
syn keyword oakFunc make-eq-hashtable make-eqv-hashtable make-hashtable
syn keyword oakFunc hashtable? hashtable-size hashtable-ref hashtable-set!
syn keyword oakFunc hashtable-delete! hashtable-contains? hashtable-update!
syn keyword oakFunc hashtable-copy hashtable-clear! hashtable-keys
syn keyword oakFunc hashtable-entries hashtable-equivalence-function hashtable-hash-function
syn keyword oakFunc hashtable-mutable? equal-hash string-hash string-ci-hash symbol-hash
syn keyword oakFunc find for-all exists filter partition fold-left fold-right
syn keyword oakFunc remp remove remv remq memp assp cons*

" ... so that a single + or -, inside a quoted context, would not be
" interpreted as a number (outside such contexts, it's a oakFunc)

syn match	oakDelimiter	!\.[ \t\[\]()";]!me=e-1
syn match	oakDelimiter	!\.$!
" ... and a single dot is not a number but a delimiter

" This keeps all other stuff unhighlighted, except *stuff* and <stuff>:

syn match	oakOther	,[a-z!$%&*/:<=>?^_~+@#%-][-a-z!$%&*/:<=>?^_~0-9+.@#%]*,
syn match	oakError	,[a-z!$%&*/:<=>?^_~+@#%-][-a-z!$%&*/:<=>?^_~0-9+.@#%]*[^-a-z!$%&*/:<=>?^_~0-9+.@ \t\[\]()";]\+[^ \t\[\]()";]*,

syn match	oakOther	"\.\.\."
syn match	oakError	!\.\.\.[^ \t\[\]()";]\+!
" ... a special identifier

syn match	oakConstant	,\*[-a-z!$%&*/:<=>?^_~0-9+.@]\+\*[ \t\[\]()";],me=e-1
syn match	oakConstant	,\*[-a-z!$%&*/:<=>?^_~0-9+.@]\+\*$,
syn match	oakError	,\*[-a-z!$%&*/:<=>?^_~0-9+.@]*\*[^-a-z!$%&*/:<=>?^_~0-9+.@ \t\[\]()";]\+[^ \t\[\]()";]*,

syn match	oakConstant	,<[-a-z!$%&*/:<=>?^_~0-9+.@]*>[ \t\[\]()";],me=e-1
syn match	oakConstant	,<[-a-z!$%&*/:<=>?^_~0-9+.@]*>$,
syn match	oakError	,<[-a-z!$%&*/:<=>?^_~0-9+.@]*>[^-a-z!$%&*/:<=>?^_~0-9+.@ \t\[\]()";]\+[^ \t\[\]()";]*,

" Non-quoted lists, and strings:

syn region oakStruc matchgroup=Delimiter start="(" matchgroup=Delimiter end=")" contains=ALL
syn region oakStruc matchgroup=Delimiter start="#(" matchgroup=Delimiter end=")" contains=ALL

syn region oakStruc matchgroup=Delimiter start="\[" matchgroup=Delimiter end="\]" contains=ALL
syn region oakStruc matchgroup=Delimiter start="#\[" matchgroup=Delimiter end="\]" contains=ALL

" Simple literals:
syn region oakString start=+\%(\\\)\@<!"+ skip=+\\[\\"]+ end=+"+ contains=@Spell

" Comments:

syn match	oakComment	";.*$" contains=@Spell


" Writing out the complete description of Scheme numerals without
" using variables is a day's work for a trained secretary...

syn match	oakOther	![+-][ \t\[\]()";]!me=e-1
syn match	oakOther	![+-]$!
"
" This is a useful lax approximation:
syn match	oakNumber	"[-#+.]\=[0-9][-#+/0-9a-f@i.boxesfdl]*"
syn match	oakError	![-#+0-9.][-#+/0-9a-f@i.boxesfdl]*[^-#+/0-9a-f@i.boxesfdl \t\[\]()";][^ \t\[\]()";]*!

syn match	oakBoolean	"#[tf]"
syn match	oakError	!#[tf][^ \t\[\]()";]\+!

syn match	oakCharacter	"#\\"
syn match	oakCharacter	"#\\."
syn match       oakError	!#\\.[^ \t\[\]()";]\+!
syn match	oakCharacter	"#\\space"
syn match	oakError	!#\\space[^ \t\[\]()";]\+!
syn match	oakCharacter	"#\\newline"
syn match	oakError	!#\\newline[^ \t\[\]()";]\+!

" R6RS
syn match oakCharacter "#\\x[0-9a-fA-F]\+"

" R7RS
syn region	oakComment start="#|" end="|#" contains=@Spell



" Synchronization and the wrapping up...

syn sync match matchPlace grouphere NONE "^[^ \t]"
" ... i.e. synchronize on a line that starts at the left margin

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_oak_syntax_inits")
  if version < 508
    let did_oak_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink oakSyntax		Statement
  HiLink oakFunc		Function

  HiLink oakString		String
  HiLink oakCharacter	Character
  HiLink oakNumber		Number
  HiLink oakBoolean		Boolean

  HiLink oakDelimiter	Delimiter
  HiLink oakConstant		Constant

  HiLink oakComment		Comment
  HiLink oakMultilineComment	Comment
  HiLink oakError		Error

  HiLink oakExtSyntax	Type
  HiLink oakExtFunc		PreProc
  delcommand HiLink
endif

let b:current_syntax = "oak"

let &cpo = s:cpo_save
unlet s:cpo_save
