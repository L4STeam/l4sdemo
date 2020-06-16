#!/bin/bash
. sh/testcase_functions.sh
. sh/hlplot_functions.sh


declare -a aqm_array=( $( cat aqm_array ) )
declare -a aqm_labels

index=0
while read line ; do
    aqm_labels[$index]="$line"
    index=$(($index+1))
done < aqm_labels

echo "aqm_array" ${aqm_array[@]}
echo "aqm_labels" ${aqm_labels[1]}

mainfolder=$1
targetfolder=$2
mkdir $targetfolder
mkdir ${targetfolder}/data
PLOTONLY=0
SAMERTT=$3
MIXRTT2=$4
EXTRA=$5
OVERLOAD=$6
CCDF=$7

link_array=("4" "12" "40" "120" "200")
rtt_array=("5" "10" "20" "50" "100")

testcases_to_plot=("s1d0s1d0" "s1dhs1dh")
testcases_to_plot_tb=("s1d0s1d0" "s1dhs1dh" "s5d0s5d0")
testcases_to_plot_extra=("s1d0s1d0" "s2d0s2d0" "s3d0s3d0" "s4d0s4d0" "s5d0s5d0" "s6d0s6d0" "s7d0s7d0" "s8d0s8d0" "s9d0s9d0" "s10d0s10d0"
                         "s0d0s10d0" "s1d0s9d0" "s2d0s8d0" "s3d0s7d0" "s4d0s6d0" "s6d0s4d0" "s7d0s3d0" "s8d0s2d0" "s9d0s1d0" "s10d0s0d0")
testcases_to_plot_single=("s1d0s0d0")

testcase_id=""

function copy_data_m() {
	for aqm in "${aqm_array[@]}"; do
		aqmname="m_${aqm}"
		copy_data_line_m
	done
}

function copy_data_mrtt2() {
	for aqm in "${aqm_array[@]}"; do
		aqmname=mr2_${aqm}
		copy_data_line_mr2
	done
}
function copy_data_m_extra() {
	declare -i index=0
	for aqm in "${aqm_array[@]}"; do
		
		aqmname="m_${aqm}"
		copy_data_link40_rtt10_extra
		index=$index+1
	done
}

function copy_data_overload() {
	declare -i index=0
	for aqm in "${aqm_array[@]}"; do
		aqmname="o_${aqm}"
		copy_data_line_o
	done
}

function copy_and_plot() {
	if [ "$CCDF" == "1" ]; then
		genplot_qd_ccdf
	elif [ "$MIXRTT2" == "1" ]; then
		
		#copy the data for mixed rtt experiments, running the calculations first
		if [ "$PLOTONLY" == "0" ]; then
			copy_data_mrtt2
		fi

		genplot_qd_mrtt2_aqm_40
		genplot_qd_mrtt2_aqm_40
		genplot_ls_mrtt2_aqm_40
		genplot_ts_mrtt2_aqm_40
		genplot_ws_mrtt2_aqm_40
		genplot_ut_mrtt2_aqm_40			
		genplot_tb_mrtt2_aqm_40
		genplot_wb_mrtt2_aqm_40
	elif  [ "$EXTRA" == "1" ]; then	
		if [ "$PLOTONLY" == "0" ]; then
			copy_data_m_extra
		fi
		genplot_ws_link40_rtt10
		genplot_tb_link40_rtt10
		genplot_wb_link40_rtt10
		genplot_ts_link40_rtt10
	elif [ "$OVERLOAD" == "1" ]; then
		if [ "$PLOTONLY" == "0" ]; then
			copy_data_overload
		fi
		genplot_qd_o
		genplot_ws_o
		genplot_ts_o
		genplot_ls_o

	else
		#copy the data for mixed experiments, running the calculations first
		if [ "$PLOTONLY" == "0" ]; then
			copy_data_m
		fi
		make_plots_mix	
	fi
}

function make_plots_mix() {
	for t in "${testcases_to_plot[@]}"; do
		testcase=$t
		gen_foldername_id
		genplot_tb_aqm
		genplot_ut_aqm
		genplot_qd_aqm
		genplot_qd_aqm_logscale
		genplot_ls_aqm
		genplot_ts_aqm
		genplot_ws_aqm
		genplot_wb_aqm		
	

		if [ "$a_dyn" != "0" ]; then
			for linkcap in "${link_array[@]}"; do
				link=$linkcap
				genplot_ct_link
			done
		fi
	done
}

export -f copy_and_plot
copy_and_plot

