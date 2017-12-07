ZZCOVTST;OSEHRA/JPS -- Test routine for Coverage Parsing;4/28/2014
	; (tab) This is series of comments
	; (tab) it should all be not executable
        ; (spaces) one of these sets might be a problem
        ; (spaces) we will have to see.
EN	; This entry point shouldn't be found without fixing
 N D
 S D=1 ;An executable line
 D T1^ZZCOVTST
 I '$$T5 W "RETURNED FROM t5",!
 Q
 ; This line not executable
 D T6^ZZCOVTST
 ;
T1 ; This line should always be found
 N D
 S D=2
 W !,D,!,"This is the second entry point",!
 D T2^ZZCOVTST(D)
 Q
 ;
T2(EQ) ; This is debatable and only called with ENT^ROU notation
 N D
 S D=3
 W !,D,!,EQ,"This is the third entry point",!
 D T3^ZZCOVTST
 Q
 ;
T3  N D S D=4 W D,!,"Fourth Entry point",! Q
 ;
T4  N D S D=5 W "Shouldn't be executed"
 W "Lots to not do"
 Q
T5(EQ) ;this entry point is called with a $$ notation
 W "THIS IS THE $$ NOTATION!",!
 Q 0
T6 ; An entry point to show comments inside of "DO" blocks
 D
 . W "This is executable code",!
 . ;This is a comment inside the do block, not executable
 . S ZZBLAH="blah"
 W "Ending T6",!
 ;
