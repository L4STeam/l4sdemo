#!/bin/bash
set -x
pushd ../
source environment.sh
popd
. sh/runtest_functions.sh
. sh/hlplot_functions.sh
# if "1" then no commands are executed, just printed (partly TODO)
PRINT_COMMANDS_ONLY=0

# run the experiments
DO_RUN=1
# generate plots
DO_PLOT=1
# generate plots summaryas PDF
DO_PDF=1

#output folder for the results
FOLDER_PREFIX="test"

declare -a aqm_labels=("PIE (ECN-Cubic+Cubic)" "DualPI2 (FDCTCP+Cubic)" "FQCoDel (FDCTCP+Cubic)")

declare -a aqm_array=( "ecpie" "pcdualpi2" "ecfqcodel")

#text describing experiments, LaTeX-friendly, will be added to the final PDF with plots
# read -d -r '' EXPINFO << EOF
# Reference test results for ECN-Reno and Reno
# 
# \date{}
# 
# This uses the git tag \href{https://github.com/L4STeam/linux/releases/tag/testing%2F5-11-2019}{testing/5-11-2019}, which is based on top Linux 5.4-rc3.
# EOF



# Experiment scenarios
DO_MIX=1
DO_EXTRA=1
DO_MIXRTT2=1
DO_OVERLOAD=1
DO_CCDF=1

function run_tests() {
	#example
	run_test $i__link $i__rtt "pr" "dualpi2" "100msecn20ms" "any_ect" 
}

function run_overload() {
	DO_MIX=0
	DO_EXTRA=0
	DO_MIXRTT2=0
	DO_OVERLOAD=1
	DO_CCDF=0
	mainfolder=$FOLDER_PREFIX"/res_overload"
	if [ "$DO_RUN" == "1" ]; then
		for i__link in 100; do for i__rtt in 10; do
    		echo $i__link $i__rtt
			run_tests	
		done; done
		if [[ -d "$mainfolder" ]]; then
			mv res/* $mainfolder/.
		else
			mv res $mainfolder
		fi
	fi
	if [ "$DO_PLOT" == "1" ]; then
		SAMERTT=0
		MIXRTT2=0
		EXTRA=0
		OVERLOAD=1
		CCDF=0
		targetfolder=$FOLDER_PREFIX"/plots_overload"
		./gen_copy_data.sh $mainfolder $targetfolder $SAMERTT $MIXRTT2 $EXTRA $OVERLOAD $CCDF
	fi
}

function run_mixrtt2() {
	DO_MIX=0
	DO_EXTRA=0
	DO_MIXRTT2=1
	DO_OVERLOAD=0
	DO_CCDF=0
	i__rtt=0
	mainfolder=$FOLDER_PREFIX"/res_mr"
	if [ "$DO_RUN" == "1" ]; then
		for i__link in 40; do
	 		echo $i__link $i__rtt
			run_tests
		done
		if [[ -d "$mainfolder" ]]; then
			mv res/* $mainfolder/.
		else
			mv res $mainfolder
		fi
	fi
	if [ "$DO_PLOT" == "1" ]; then
		SAMERTT=0
		MIXRTT2=1
		EXTRA=0
		OVERLOAD=0
		CCDF=0
		targetfolder=$FOLDER_PREFIX"/plots_mr"
		./gen_copy_data.sh $mainfolder $targetfolder $SAMERTT $MIXRTT2 $EXTRA $OVERLOAD $CCDF
	fi
}

function run_mix() {
	DO_MIX=1
	DO_EXTRA=0
	DO_MIXRTT2=0
	DO_OVERLOAD=0
	DO_CCDF=0
	mainfolder=$FOLDER_PREFIX"/res_er"
	if [ "$DO_RUN" == "1" ]; then
		for i__link in 200 120 40 12 4; do for i__rtt in 100 50 20 10 5; do
		    	echo $i__link $i__rtt
			run_tests
		done; done
		mv res $mainfolder

	fi
	if [ "$DO_PLOT" == "1" ]; then
		SAMERTT=1
		MIXRTT2=0
		EXTRA=0
		OVERLOAD=0
		CCDF=0
		targetfolder=$FOLDER_PREFIX"/plots_er"
		./gen_copy_data.sh $mainfolder $targetfolder $SAMERTT $MIXRTT2 $EXTRA $OVERLOAD $CCDF
	fi
}

function run_extra() {
	echo "extra"
	DO_MIX=1
	DO_EXTRA=1
	DO_MIXRTT2=0
	DO_OVERLOAD=0
	DO_CCDF=0
	mainfolder=$FOLDER_PREFIX"/res_tb"
	if [ "$DO_RUN" == "1" ]; then
		for i__link in 40; do for i__rtt in 10; do
	    	echo $i__link $i__rtt
			run_tests
		done; done
		mv res $mainfolder
	fi
	if [ "$DO_PLOT" == "1" ]; then
		echo "do plot"
		
		SAMERTT=0
		MIXRTT2=0
		EXTRA=1
		OVERLOAD=0
		CCDF=0
		targetfolder=$FOLDER_PREFIX"/plots_extra"
		./gen_copy_data.sh $mainfolder $targetfolder $SAMERTT $MIXRTT2 $EXTRA $OVERLOAD $CCDF
	fi
}

function run_ccdf() {
	echo "ccdf"
	DO_MIX=1
	DO_EXTRA=0
	DO_MIXRTT2=0
	DO_OVERLOAD=0
	DO_CCDF=1
	mainfolder=$FOLDER_PREFIX"/res_ccdf"
	if [ "$DO_RUN" == "1" ]; then
		for i__link in 120; do for i__rtt in 10; do
	    	echo $i__link $i__rtt
			run_tests
		done; done
		mv res $mainfolder
	fi
	if [ "$DO_PLOT" == "1" ]; then
		echo "do plot"
		
		SAMERTT=0
		MIXRTT2=0
		EXTRA=0
		OVERLOAD=0
		CCDF=1
		targetfolder=$FOLDER_PREFIX"/plots_ccdf"
		./gen_copy_data.sh $mainfolder $targetfolder $SAMERTT $MIXRTT2 $EXTRA $OVERLOAD $CCDF
	fi
}


if [[ -d "$FOLDER_PREFIX" ]]; then
	echo "$FOLDER_PREFIX exists"
else
	mkdir $FOLDER_PREFIX
fi
touch "${FOLDER_PREFIX}/COMMIT_$(git describe --always --dirty)"

printf "%s\n" "${aqm_array[@]}" > aqm_array
printf "%s\n" "${aqm_labels[@]}" > aqm_labels

#if [ "$DO_RUN" == "1" ]; then
#	interface_options
#fi

function create_plotssummary_pdf()
{
	
	commit=$(git describe --always --dirty)
	cat << EOF > tex/expinfo.tex
	\def \plotsfolder {${FOLDER_PREFIX}}

Evaluating the latest version of AccECN and TCP Prague w/ RTT independence

	COMMIT: ${commit}
EOF
	pdflatex tex/plotssummary.tex
	mv plotssummary.pdf ${FOLDER_PREFIX}/${FOLDER_PREFIX}.pdf
	rm plotssummary.*
}

# experiments flow each, 120Mbps link, 10ms RTT, to produce queue delay CCDF plots
run_ccdf

# experiments with equal RTT, [1-1,1h-1h]
##run_mix

#extra experiments with different flow combinations, 40Mbps link speed, 10ms RTT
##run_extra

#experiments with mixed RTT, one flow each
##run_mixrtt2

#overload experiments 5 flow each, 100Mbps link, 10ms RTT, UDP rate [50, 70, 100, 140, 200]Mbps, once with ECN enabled, once as classic
##run_overload

if [ "$DO_PDF" == "1" ]; then
	create_plotssummary_pdf
fi





