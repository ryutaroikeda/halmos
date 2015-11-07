$( Propositional logic $)

$c ( $.
$c ) $.
$c -> $.
$c -. $.
$c wff $.
$c |- $.

$v ph ps ch $.
wph $f wff ph $.
wps $f wff ps $.
wch $f wff ch $.

$( well-formedness $)
wn $a wff -. ph $.	
wi $a wff ( ph -> ps ) $.


$( axiom of simplification $)
${
	ax-1 $a |- ( ph -> ( ps -> ph ) ) $.
$}

$( Frege's axiom $)
${
	ax-2 $a 
	|- ( ( ph -> ( ps -> ch ) ) -> ( ( ph -> ps ) -> ( ph -> ch ) ) ) 
	$.
$}

$( axiom of transposition $)
${
	ax-3 $a ( -. ph -> -. ps ) -> ( ps -> ph ) $.
$}

$( modus ponens $) 
${
	min $e |- ph $.
	maj $e |- ( ph -> ps ) $.
	ax-mp $a |- ps $.
$}

$( logical implication $)

$( double modus ponens $) 
${ 
	mp2b.1 $e |- ph $.
	mp2b.2 $e |- ( ph -> ps ) $.
	mp2b.3 $e |- ( ps -> ch ) $.
	mp2b $p |- ch $=
	wps wch wph wps mp2b.1 mp2b.2 ax-mp mp2b.3 ax-mp $.
$}

${
	a1i.1 $e |- ph $.
	a1i $p |- ( ps -> ph ) $=
	wph
	wps wph wi
	a1i.1
	wph wps ax-1
	ax-mp
	$.
$}

${
	mp1i.1 $e |- ph $.
	mp1i.2 $e |- ( ph -> ps ) $.
	mp1i $p |- ( ch -> ps ) $=
	wps
	wch
	$( |- ps $)
	wph wps mp1i.1 mp1i.2 ax-mp
	a1i
	$.
$}

${
	a2i.1 $e |- ( ph -> ( ps -> ch ) ) $.
	a2i $p |- ( ( ph -> ps ) -> ( ph -> ch ) ) $=
	wph wps wch wi wi
	wph wps wi wph wch wi wi
	a2i.1
	wph wps wch ax-2
	ax-mp
	$.
$}

${
	imim2i.1 $e |- ( ph -> ps ) $.
	imim2i $p |- ( ( ch -> ph ) -> ( ch -> ps ) ) $=
	wch wph wps
	wph wps wi wch imim2i.1 a1i
	a2i
	$.
$}

${ $( modus ponens deduction $)
	mdp.1 $e |- ( ph -> ps ) $.
	mdp.2 $e |- ( ph -> ( ps -> ch ) ) $.
	mdp $p |- ( ph -> ch ) $=
	wph wps wi
	wph wch wi
	mdp.1
	wph wps wch mdp.2 a2i
	ax-mp
	$.
$}

${ $( The principle of syllogism $)
	syl.1 $e |- ( ph -> ps ) $.
	syl.2 $e |- ( ps -> ch ) $.
	syl $p |- ( ph -> ch ) $=
	wph wps wch
	syl.1

	wps wch wi
	wph
	syl.2
	a1i

	mdp
	$.
$}

${ $( nested modus ponens $)
	mpi.1 $e |- ps $.
	mpi.2 $e |- ( ph -> ( ps -> ch ) ) $.
	mpi $p |- ( ph -> ch ) $=
	wph wps wi
	wph wch wi

	wps wph
	mpi.1
	a1i $( |- ( ph -> ps ) $)

	wph wps wch
	mpi.2
	a2i $( |- ( ( ph -> ps ) -> ( ps -> ch ) ) $)

	ax-mp
	$.
$}


