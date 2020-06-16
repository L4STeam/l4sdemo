#!/bin/bash

color_a=""
color_b=""
color_dropsecn=""
color_util=""
color_tb=""
cc_a=""
cc_b=""
aqmname=""
aqmcomb=""
link=""
folder=""
linenr=''
cur_testcase=""
testcase_id=""
tbps=""
declare -i a_flows b_flows testcase_count totalflows fairrate x_count aqm_count rtt_count


function contains() {
    local n=$#
    local value=${!n}
    for ((i=1;i < $#;i++)) {
        if [ "${!i}" == "${value}" ]; then
            echo "y"
            return 0
        fi
    }
    echo "n"
    return 1
}

function get_cc_captions_colors() {
	
	if [ "$MIXRTT2" == "1" ]; then
		cc1="${aqmname:4:1}"
		cc2="${aqmname:5:1}"
	else
		cc1="${aqmname:2:1}"
		cc2="${aqmname:3:1}"
	fi

	if [[ $cc1 == "c" ]]; then
		echo "Cubic"
		cc_a="Cubic"
		color_a="#FC6C6C" # pink/salmon
	elif [[ $cc1 == "d" ]]; then
		cc_a="DCTCP"
		color_a="#0970c4" # water-green
		color_dropsecn="purple"
		tbps="7"
	elif [[ $cc1 == "e" ]]; then
		cc_a="ECN-Cubic"
		#color_b="#800080"
		color_a="black"
		color_dropsecn="dark-gray"
		tbps="3"
	elif [[ $cc1 == "r" ]]; then
		cc_a="Reno"
		color_a="brown"
	elif [[ $cc1 == "n" ]]; then
		cc_a="ECN-Reno"
		color_a="orange"
	elif [[ $cc1 == "p" ]]; then
		cc_a="Prague"
		color_a="#003300" # dark-green
		color_dropsecn="magenta"
	elif [[ $cc1 == "s" ]]; then
		cc_a="Prague-scalable"
		color_a="#003300" # dark-green
		color_dropsecn="magenta"
	elif [[ $cc1 == "a" ]]; then
		cc_a="Prague-additive"
		color_a="#003300" # dark-green
		color_dropsecn="magenta"
	elif [[ $cc1 == "B" ]]; then
		cc_a="BBRv2-ECN"
		color_a="brown"
		color_dropsecn="orange-red"
	elif [[ $cc1 == "b" ]]; then
		cc_a="BBRv2"
		color_a="blue"
	fi
	if [[ $cc2 == "c" ]]; then
		cc_b="Cubic"
		color_b="#FC6C6C"
	elif [[ $cc2 == "d" ]]; then
		cc_b="DCTCP"
		color_b="#0970c4"
	elif [[ $cc2 == "e" ]]; then
		cc_b="ECN-Cubic"
		#color_b="#800080"
		color_b="black"
	elif [[ $cc2 == "r" ]]; then
		cc_b="Reno"
		color_b="brown"
	elif [[ $cc1 == "n" ]]; then
		cc_a="ECN-Reno"
		color_a="orange"
	elif [[ $cc2 == "p" ]]; then
		cc_b="Prague"
		color_b="#003300"
	elif [[ $cc2 == "P" ]]; then
		cc_b="Prague-RTT-indep"
		color_b="#003300"
	elif [[ $cc2 == "B" ]]; then
		cc_a="BBRv2-ECN"
		color_a="brown"
		color_dropsecn="brown"
	elif [[ $cc2 == "b" ]]; then
		cc_a="BBRv2"
		color_a="blue"
	fi
	color_marks="purple"
}

function get_tb_color() {	
	cc1="${aqmname:4:1}"
	cc2="${aqmname:5:1}"
	
	if [ "$cc1" == "c" ] && [ "$cc2" == "c" ]; then
		cc_a="Cubic"
		cc_b="Cubic"
		color_tb="#FC6C6C"
	elif [ "$cc1" == "d" ] && [ "$cc2" == "d" ]; then
		cc_a="DCTCP"
		cc_b="DCTCP"
		color_tb="#0970c4"
	elif [ "$cc1" == "e" ] && [ "$cc2" == "e" ]; then
		cc_a="ECN-Cubic"
		cc_b="ECN-Cubic"
		#color_tb="#800080"
		color_tb="black"
	elif [ "$cc1" == "d" ] && [ "$cc2" == "c" ]; then
		cc_a="DCTCP"
		cc_b="Cubic"
		color_tb="brown"
	elif [ "$cc1" == "e" ] && [ "$cc2" == "c" ]; then
		cc_a="ECN-Cubic"
		cc_b="Cubic"
		color_tb="pink"
	elif [ "$cc1" == "p" ] && [ "$cc2" == "c" ]; then
		cc_a="Prague"
		cc_b="Cubic"
		color_tb="green"
	elif [ "$cc1" == "a" ] && [ "$cc2" == "c" ]; then
		cc_a="Prague-additive"
		cc_b="Cubic"
		color_tb="green"
	elif [ "$cc1" == "s" ] && [ "$cc2" == "c" ]; then
		cc_a="Prague-scalable"
		cc_b="Cubic"
		color_tb="green"
	elif [ "$cc1" == "p" ] && [ "$cc2" == "d" ]; then
		cc_a="FDCTCP"
		cc_b="DCTCP"
		color_tb="purple"
	elif [ "$cc1" == "p" ] && [ "$cc2" == "p" ]; then
		cc_a="FDCTCP"
		cc_b="FDCTCP"
		color_tb="orange"
	elif [ "$cc1" == "B" ] && [ "$cc2" == "p" ]; then
		cc_a="BBRv2-ECN"
		cc_b="FDCTCP"
		color_tb="brown"
	elif [ "$cc1" == "b" ] && [ "$cc2" == "p" ]; then
		cc_a="BBRv2"
		cc_b="FDCTCP"
		color_tb="blue"
	elif [ "$cc1" == "B" ] && [ "$cc2" == "c" ]; then
		cc_a="BBRv2"
		cc_b="Cubic"
		color_tb="orange"
	elif [ "$cc1" == "n" ] && [ "$cc2" == "r" ]; then
		cc_a="ECN-Reno"
		cc_b="Reno"
		color_tb="salmon"
	elif [ "$cc1" == "P" ] && [ "$cc2" == "c" ]; then
		cc_a="Prague-RTT-indep"
		cc_b="Cubic"
		color_tb="green"
	elif [ "$cc1" == "P" ] && [ "$cc2" == "e" ]; then
		cc_a="Prague-RTT-indep"
		cc_b="Cubic"
		color_tb="salmon"
	elif [ "$cc1" == "P" ] && [ "$cc2" == "P" ]; then
		cc_a="Prague-RTT-indep"
		cc_b="Prague-RTT-indep"
		color_tb="orange"
	fi
}

function get_util_color() {
	if [ "$MIXRTT2" == "1" ]; then
		cc1="${aqmname:4:1}"
		cc2="${aqmname:5:1}"
	else
		cc1="${aqmname:2:1}"
		cc2="${aqmname:3:1}"
	fi



	if [[ $cc1 == $cc2 ]]; then
		color_util=$color_a
	elif [[ $cc_a == "DCTCP" ]]; then
		color_util="#0970c4"
	elif [[ $cc_a == "ECN-Cubic" ]]; then	
		color_util="black"
	elif [[ $cc_a == "Prague" ]]; then
		color_util="green"
	elif [[ $cc_a == "Prague-additive" ]]; then
		color_util="green"
	elif [[ $cc_a == "Prague-scalable" ]]; then
		color_util="green"
	elif [[ $cc_a == "BBRv2-ECN" ]]; then
		color_util="brown"
	elif [[ $cc_a == "BBRv2" ]]; then
		color_util="orange"
	elif [[ $cc_a == "ECN-Reno" ]]; then
		color_util="salmon"
	fi
}

function gen_foldername_id() {
	declare -i lcount
	lcount=1
	a_static="${testcase:$lcount:1}"
	lcount=$lcount+1
	if [ "${testcase:$lcount:1}" != "d" ]; then
		a_static=$a_static"${testcase:$lcount:1}"
		lcount=$lcount+1
	fi
	a_flows=$a_static

	lcount=$lcount+1
	a_dyn="${testcase:$lcount:1}"

	lcount=$lcount+2
	b_static="${testcase:$lcount:1}"
	lcount=$lcount+1
	if [ "${testcase:$lcount:1}" != "d" ]; then
		b_static=$b_static"${testcase:$lcount:1}"
		lcount=$lcount+1
	fi
	b_flows=$b_static

	lcount=$lcount+1
	b_dyn="${testcase:$lcount:1}"  

	testcase_id=${a_static}
	if [ "$a_dyn" != "0" ]; then
		testcase_id=${testcase_id}${a_dyn}
	fi
	testcase_id=${testcase_id}"-"${b_static}
	if [ "$b_dyn" != "0" ]; then
		testcase_id=${testcase_id}${b_dyn}
	fi


	set_dyn_captions_mix
	set_mit_mix
	foldername="/d_"d${a_dyn}s${a_static}"_r_"d${b_dyn}s${b_static}

}

function copy_data_line_m() {
	declare -i casenr=0 link
	for t in "${testcases_to_plot[@]}"; do
		testcase=$t
		gen_foldername_id
		for link in "${link_array[@]}"; do
			#TB
			filename=${targetfolder}"/data/"${aqmname}"_"${link}"_tb"
			filecontent=""
			for rtt in "${rtt_array[@]}"; do
				folder="${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/"${foldername}_static
				sudo ./calc_qpd $folder 0
				sudo ./calc_mix $folder $link $rtt $rtt $a_flows $b_flows
				line=$(cat ${folder}/rr)
				filecontent=${filecontent}${rtt}" "${line}$'\n'
			done
			echo "$filecontent" > ${filename}_${testcase_id}

			#WB
			filename=${targetfolder}"/data/"${aqmname}"_"${link}"_wb"
			filecontent=""
			for rtt in "${rtt_array[@]}"; do
				folder="${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/"${foldername}_static
				line=$(cat ${folder}/wr)
				filecontent=${filecontent}${rtt}" "${line}$'\n'
			done
			echo "$filecontent" > ${filename}_${testcase_id}
			
			#UT
			filename=${targetfolder}"/data/"${aqmname}"_"${link}"_ut"
			filecontent=""
			for rtt in "${rtt_array[@]}"; do
				folder="${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/"${foldername}
				line=$(cat ${folder}/stat_util)
				filecontent=${filecontent}${rtt}" "${line}$'\n'
			done
			echo "$filecontent" > ${filename}_${testcase_id}



			#ecn and nonecn
			#TS
			filename=${targetfolder}"/data/"${aqmname}"_"${link}"_ts"
			filecontent_a=""
			filecontent_b=""

			for rtt in "${rtt_array[@]}"; do
				folder="${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/"${foldername}_static
				line_a=$(cat ${folder}/stat_rpf_a)
				line_b=$(cat ${folder}/stat_rpf_b)

				filecontent_a=${filecontent_a}${rtt}" "${line_a}$'\n'
				filecontent_b=${filecontent_b}${rtt}" "${line_b}$'\n'

			done
			echo "$filecontent_a" > ${filename}"_a"_${testcase_id}
			echo "$filecontent_b" > ${filename}"_b"_${testcase_id}

			#WS
			filename=${targetfolder}"/data/"${aqmname}"_"${link}"_ws"
			filecontent_a=""
			filecontent_b=""

			for rtt in "${rtt_array[@]}"; do
				folder="${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/"${foldername}_static
				line_a=$(cat ${folder}/stat_win_a)
				line_b=$(cat ${folder}/stat_win_b)

				declare -i avgqs_a_rtt=$(cat ${folder}/avgqs_a)+${rtt}
				declare -i avgqs_b_rtt=$(cat ${folder}/avgqs_b)+${rtt}
				declare -i totalflows=$a_flows+$b_flows
				declare -i fairrate=$link*125000/$totalflows
				#tp->demo_data->fair_window = remaining_bw / (nr_al_flows_a/(tp->demo_data->avg_qsize_ll+tp->demo_data->rtt_a) +
                               # nr_al_flows_b/(tp->demo_data->avg_qsize_c+tp->demo_data->rtt_b));
				declare -i flows_over_qsrtt_a=${a_flows}*1000/${avgqs_a_rtt}
				declare -i flows_over_qsrtt_b=${b_flows}*1000/${avgqs_b_rtt}
				declare -i sum=$flows_over_qsrtt_a+$flows_over_qsrtt_b
				#declare -i fairwin=${link}*125000/${sum}/1000000
				fairwin=$(cat ${folder}/fairwin)
				#declare -i fairwin_b=${fairrate}*${avgqs_b_rtt}/1000

				filecontent_a=${filecontent_a}${rtt}" "${line_a}" ${fairwin}"$'\n'
				filecontent_b=${filecontent_b}${rtt}" "${line_b}" ${fairwin}"$'\n'

			done
			echo "$filecontent_a" > ${filename}"_a"_${testcase_id}
			echo "$filecontent_b" > ${filename}"_b"_${testcase_id}
			
			#QD
			filename=${targetfolder}"/data/"${aqmname}"_"${link}"_qd"
			filecontent_a=""
			filecontent_b=""

			for rtt in "${rtt_array[@]}"; do
				folder="${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/"${foldername}
						
				line_a=$(cat ${folder}/stat_qs_a)
				line_b=$(cat ${folder}/stat_qs_b)

				filecontent_a=${filecontent_a}${rtt}" "${line_a}$'\n'
				filecontent_b=${filecontent_b}${rtt}" "${line_b}$'\n'

			done
			echo "$filecontent_a" > ${filename}"_a"_${testcase_id}
			echo "$filecontent_b" > ${filename}"_b"_${testcase_id}

			#LS
			filename=${targetfolder}"/data/"${aqmname}"_"${link}"_ls"

			filecontent_marks_a=""
			filecontent_marks_b=""
			filecontent_a=""
			filecontent_b=""

			for rtt in "${rtt_array[@]}"; do
				folder="${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/"${foldername}
							
				line_marks_a=$(cat ${folder}/stat_marks_a)
				line_marks_b=$(cat ${folder}/stat_marks_b)
				line_a=$(cat ${folder}/stat_drops_a)
				line_b=$(cat ${folder}/stat_drops_b)

				filecontent_marks_a=${filecontent_marks_a}${rtt}" "${line_marks_a}$'\n'
				filecontent_marks_b=${filecontent_marks_b}${rtt}" "${line_marks_b}$'\n'
				filecontent_a=${filecontent_a}${rtt}" "${line_a}$'\n'
				filecontent_b=${filecontent_b}${rtt}" "${line_b}$'\n'

			done

			echo "$filecontent_marks_a" > ${filename}"_marks_a"_${testcase_id}
			echo "$filecontent_marks_b" > ${filename}"_marks_b"_${testcase_id}
			echo "$filecontent_a" > ${filename}"_a"_${testcase_id}
			echo "$filecontent_b" > ${filename}"_b"_${testcase_id}

			#CT
			filecontent_a=""
			filecontent_b=""
			filename_a=""
			filename_b=""
			if [ "$a_dyn" != "0" ]; then	
				for rtt in "${rtt_array[@]}"; do

					filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ct_a_"${rtt}_${testcase_id}
					filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ct_b_"${rtt}_${testcase_id}

					folder="${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_completion/${foldername}"
					avgrate_a=$(cat ${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/${foldername}/avgrate_a)
					avgrate_b=$(cat ${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/${foldername}/avgrate_a)
					sudo ./compl $folder $avgrate_a $avgrate_b $rtt $link
				
					filecontent_a=$(cat ${folder}/compl_bins_a_hs)
					echo "$filecontent_a" > ${filename_a}
					filecontent_b=$(cat ${folder}/compl_bins_b_hs)
					echo "$filecontent_b" > ${filename_b}


				done
			fi
			
		done
		casenr=$casenr+1
	done
}

function copy_data_line_mr2() {
	
	declare -i aport bport rtt_a rtt_b a_flows b_flows totalflows fairrate
	
	

	portarray=("0" "5005" "5010" "5020" "5050" "5100")
	a_flows=1

	#for link in "${link_array[@]}"; do
	for link in "40"; do
		filename_tb=${targetfolder}"/data/"${aqmname}"_"${link}"_tb_mrtt2"
		filename_wb=${targetfolder}"/data/"${aqmname}"_"${link}"_wb_mrtt2"
		filename_ut=${targetfolder}"/data/"${aqmname}"_"${link}"_ut_mrtt2"
		filename_ts=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_mrtt2"
		filename_ws=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_mrtt2"
		filename_qd=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_mrtt2"
		filename_ls=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_mrtt2"

		filecontent_tb=""
		filecontent_wb=""
		filecontent_ut=""
		filecontent_ts_a=""
		filecontent_ts_b=""
		filecontent_ws_a=""
		filecontent_ws_b=""
		filecontent_qd_a=""
		filecontent_qd_b=""
		filecontent_ls_a=""
		filecontent_ls_b=""
		filecontent_ls_marks_a=""
		filecontent_ls_marks_b=""

		for aport in "5005" "5100"; do 
			for bport in "${portarray[@]}"; do 
				rtt_a=$aport-5000
				if [ "$bport" == "0" ]; then
					b_flows=0
					a_flows=1
					rtt_b=0
					rtt_label="A${rtt_a}-"
				else
					b_flows=1
					rtt_b=$bport-5000
					rtt_label="A${rtt_a}-B${rtt_b}"
				fi
				
			
				folder="${mainfolder}/${aqmname}_*_${link}_0/mix_1000_d${aport}_r${bport}"
				
				sudo ./calc_qpd $folder 0
				sudo ./calc_mix $folder $link $rtt_a $rtt_b $a_flows $b_flows
				
				#TB
				if [ "$bport" != "0" ]; then
					line=$(cat ${folder}/rr)
					filecontent_tb=${filecontent_tb}${rtt_label}" "${line}$'\n'
				fi

				#WB
				if [ "$bport" != "0" ]; then
					line=$(cat ${folder}/wr)
					filecontent_wb=${filecontent_wb}${rtt_label}" "${line}$'\n'
				fi

				#UT
				line=$(cat ${folder}/stat_util)
				filecontent_ut=${filecontent_ut}${rtt_label}" "${line}$'\n'

				#ecn and nonecn
				#TS
				totalflows=${a_flows}+${b_flows}
				fairrate=${link}*125000/$totalflows
				line_a=$(cat ${folder}/stat_rpf_a)
				line_a=$line_a" $fairrate"
				line_b=$(cat ${folder}/stat_rpf_b)
				line_b=$line_b" $fairrate"

				filecontent_ts_a=${filecontent_ts_a}${rtt_label}" "${line_a}$'\n'
				filecontent_ts_b=${filecontent_ts_b}${rtt_label}" "${line_b}$'\n'
				#WS
				
				fairwin=$(cat ${folder}/fairwin)

				line_a=$(cat ${folder}/stat_win_a)
				line_b=$(cat ${folder}/stat_win_b)
				#echo $line_a



				filecontent_ws_a=${filecontent_ws_a}${rtt_label}" "${line_a}" ${fairwin}"$'\n'
				filecontent_ws_b=${filecontent_ws_b}${rtt_label}" "${line_b}" ${fairwin}"$'\n'
			
				#QD
				line_a=$(cat ${folder}/stat_qs_a)
				line_b=$(cat ${folder}/stat_qs_b)

				filecontent_qd_a=${filecontent_qd_a}${rtt_label}" "${line_a}$'\n'
				filecontent_qd_b=${filecontent_qd_b}${rtt_label}" "${line_b}$'\n'
		
				#LS

				line_marks_a=$(cat ${folder}/stat_marks_a)
				line_marks_b=$(cat ${folder}/stat_marks_b)
				line_a=$(cat ${folder}/stat_drops_a)
				line_b=$(cat ${folder}/stat_drops_b)

				filecontent_ls_marks_a=${filecontent_ls_marks_a}${rtt_label}" "${line_marks_a}$'\n'
				filecontent_ls_marks_b=${filecontent_ls_marks_b}${rtt_label}" "${line_marks_b}$'\n'
				filecontent_ls_a=${filecontent_ls_a}${rtt_label}" "${line_a}$'\n'
				filecontent_ls_b=${filecontent_ls_b}${rtt_label}" "${line_b}$'\n'

			done
		done
		echo "$filecontent_tb" > ${filename_tb}
		echo "$filecontent_wb" > ${filename_wb}
		echo "$filecontent_ut" > ${filename_ut}
		echo "$filecontent_ts_a" > ${filename_ts}"_a"
		echo "$filecontent_ts_b" > ${filename_ts}"_b"
		echo "$filecontent_ws_a" > ${filename_ws}"_a"
		echo "$filecontent_ws_b" > ${filename_ws}"_b"
		echo "$filecontent_qd_a" > ${filename_qd}"_a"
		echo "$filecontent_qd_b" > ${filename_qd}"_b"
		echo "$filecontent_ls_marks_a" > ${filename_ls}"_marks_a"
		echo "$filecontent_ls_marks_b" > ${filename_ls}"_marks_b"
		echo "$filecontent_ls_a" > ${filename_ls}"_a"
		echo "$filecontent_ls_b" > ${filename_ls}"_b"
	done

}

function copy_data_link40_rtt10_extra() {
	link=40
	rtt=10
	filename_tb=${targetfolder}"/data/"${aqmname}"_"${link}"_tb_extra"
	filename_ut=${targetfolder}"/data/"${aqmname}"_"${link}"_ut_extra"
	filename_ts=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_extra"
	filename_ws=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_extra"
	filename_wb=${targetfolder}"/data/"${aqmname}"_"${link}"_wb_extra"
	filename_qd=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_extra"
	filename_ls=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_extra"

	filecontent_tb=""
	filecontent_wb=""
	filecontent_ut=""
	filecontent_ts=""
	filecontent_ts_a=""
	filecontent_ts_b=""
	filecontent_ws_a=""
	filecontent_ws_b=""
	filecontent_qd_a=""
	filecontent_qd_b=""
	filecontent_ls_a=""
	filecontent_ls_b=""


	for t in "${testcases_to_plot_extra[@]}"; do
		testcase=$t
		gen_foldername_id
		#TB
		folder="${mainfolder}/${aqmname}_*_${link}_${rtt}/mix_1000/"${foldername}
		sudo ./calc_qpd $folder 0
		sudo ./calc_mix $folder $link $rtt $rtt $a_flows $b_flows
		line=$(cat ${folder}/rr)
		filecontent_tb=${filecontent_tb}A${a_flows}-B${b_flows}" "${line}$'\n'
		#WB
		if [ "$testcase" != "s0d0s10d0" ] && [ "$testcase" != "s10d0s0d0" ] ; then
			line=$(cat ${folder}/wr)
			filecontent_wb=${filecontent_wb}A${a_flows}-B${b_flows}" "${line}$'\n'
		fi
		#UT
		line=$(cat ${folder}/stat_util)
		filecontent_ut=${filecontent_ut}A${a_flows}-B${b_flows}" "${line}$'\n'
			
		#ecn and nonecn
		#TS
		totalflows=$a_flows+$b_flows
        fairrate=$link*125000/$totalflows
		line_a=$(cat ${folder}/stat_rpf_a)
		line_b=$(cat ${folder}/stat_rpf_b)
		if [ "$a_flows" != "0" ]; then
			filecontent_ts_a=${filecontent_ts_a}A${a_flows}-B${b_flows}" "${line_a}" "${fairrate}$'\n'
		else
			filecontent_ts_a=${filecontent_ts_a}A${a_flows}-B${b_flows}" s0 0 0 0 0 1"$'\n'
		fi		
		if [ "$b_flows" != "0" ]; then
			filecontent_ts_b=${filecontent_ts_b}A${a_flows}-B${b_flows}" "${line_b}" "${fairrate}$'\n'	
		else
            filecontent_ts_b=${filecontent_ts_b}A${a_flows}-B${b_flows}" s0 0 0 0 0 1"$'\n'              
        fi 
		#WS
		line_a=""
		line_b=""
		line_a=$(cat ${folder}/stat_win_a)
		line_b=$(cat ${folder}/stat_win_b)

		fairwin=$(cat ${folder}/fairwin)
		if [ "$a_flows" != "0" ]; then
			filecontent_ws_a=${filecontent_ws_a}A${a_flows}-B${b_flows}" "${line_a}" $fairwin"$'\n'
		else
			filecontent_ws_a=${filecontent_ws_a}A${a_flows}-B${b_flows}" s0 0 0 0 0 1"$'\n'
		fi
		if [ "$b_flows" != "0" ]; then
			filecontent_ws_b=${filecontent_ws_b}A${a_flows}-B${b_flows}" "${line_b}" $fairwin"$'\n'
		else
			filecontent_ws_b=${filecontent_ws_b}A${a_flows}-B${b_flows}" "${line_b}" s0 0 0 0 0 1"$'\n'
		fi

		#QD
		line_a=$(cat ${folder}/stat_qs_a)
		line_b=$(cat ${folder}/stat_qs_b)
		filecontent_qd_a=${filecontent_qd_a}${testcase_id}" "${line_a}$'\n'
		filecontent_qd_b=${filecontent_qd_b}${testcase_id}" "${line_b}$'\n'

		#LS
		line_a=$(cat ${folder}/stat_marks_a)
		line_b=$(cat ${folder}/stat_drops_b)
		filecontent_ls_a=${filecontent_ls_a}${testcase_id}" "${line_a}$'\n'
		filecontent_ls_b=${filecontent_ls_b}${testcase_id}" "${line_b}$'\n'
	
	done
	echo "$filecontent_tb" > ${filename_tb}
	echo "$filecontent_wb" > ${filename_wb}
	echo "$filecontent_ut" > ${filename_ut}

	echo "$filecontent_ts_a" > ${filename_ts}"_a"
	echo "$filecontent_ts_b" > ${filename_ts}"_b"

	echo "$filecontent_ws_a" > ${filename_ws}"_a"
	echo "$filecontent_ws_b" > ${filename_ws}"_b"

	echo "$filecontent_qd_a" > ${filename_qd}"_a"
	echo "$filecontent_qd_b" > ${filename_qd}"_b"

	echo "$filecontent_ls_a" > ${filename_ls}"_a"
	echo "$filecontent_ls_b" > ${filename_ls}"_b"

}

function copy_data_line_o() {
	declare -i casenr=0 l=0
	link=100
	rtt=10
	a_flows=5
	b_flows=5


	for m in "ws" "wb" "ts" "qd" "ls" "marks"; do

		declare -i avgqs_a_rtt_ud
		declare -i avgqs_a_rtt_ur
		declare -i avgqs_b_rtt_ud
		declare -i avgqs_b_rtt_ur

		declare -i fairrate_ud
		declare -i fairrate_ur

		declare -i fairwin_ud
		declare -i fairwin_ur
		declare -i fairwin_b_ud
		declare -i fairwin_b_ur

		declare -i link_u

		filecontent_a_ud="#udprate #nrflows #avg #p1 #p25 #p75 #p99 #stddev"$'\n'
		filecontent_b_ud="#udprate #nrflows #avg #p1 #p25 #p75 #p99 #stddev"$'\n'
		filecontent_ud="#udprate #nrflows #wr"$'\n'

		filecontent_a_ur="#udprate #nrflows #avg #p1 #p25 #p75 #p99 #stddev"$'\n'
		filecontent_b_ur="#udprate #nrflows #avg #p1 #p25 #p75 #p99 #stddev"$'\n'
		filecontent_ur="#udprate #nrflows #wr"$'\n'

		filecontent_udp_d="#udprate #nrflows #avg #p1 #p25 #p75 #p99 #stddev"$'\n'
		filecontent_udp_r="#udprate #nrflows #avg #p1 #p25 #p75 #p99 #stddev"$'\n'

		filecontent_rateshare_ud="#udprate #udpshare #ecnshare #nonecnshare #link"$'\n'
		filecontent_rateshare_ur="#udprate #udpshare #ecnshare #nonecnshare #link"$'\n'
		
		for udprate in "50" "70" "100" "140" "200"; do

			foldername_ud="mix_1000d_u${udprate}s5_r_u0s5"
			foldername_ur="mix_1000d_u0s5_r_u${udprate}s5"

			filename=${targetfolder}"/data/"${aqmname}"_"${link}"_"${m}

			folder_ud="${mainfolder}/${aqmname}_*_${link}_${rtt}/"${foldername_ud}
			sudo ./calc_qpd ${folder_ud}_tcp 0
			sudo ./calc_mix ${folder_ud}_tcp $link $rtt $rtt $a_flows $b_flows
			sudo ./calc_qpd ${folder_ud}_udp 0
			sudo ./calc_mix ${folder_ud}_udp $link $rtt $rtt $a_flows $b_flows


			folder_ur="${mainfolder}/${aqmname}_*_${link}_${rtt}/"${foldername_ur}
			sudo ./calc_qpd ${folder_ur}_tcp 0
			sudo ./calc_mix ${folder_ur}_tcp $link $rtt $rtt $a_flows $b_flows
			sudo ./calc_qpd ${folder_ur}_udp 0
			sudo ./calc_mix ${folder_ur}_udp $link $rtt $rtt $a_flows $b_flows

			fn=""
			if [ "$m" == "ws" ]; then
				fn="stat_win"
				avgqs_a_rtt_ud=$(cat ${folder_ud}_tcp/avgqs_a)+${rtt}
				avgqs_b_rtt_ud=$(cat ${folder_ud}_tcp/avgqs_b)+${rtt}
				
				avgqs_a_rtt_ur=$(cat ${folder_ur}_tcp/avgqs_a)+${rtt}
				avgqs_b_rtt_ur=$(cat ${folder_ur}_tcp/avgqs_b)+${rtt}

				declare -i avgrate_a_ud=$(cat ${folder_ud}_tcp/avgrate_a)
				declare -i avgrate_b_ud=$(cat ${folder_ud}_tcp/avgrate_b)
				declare -i avgrate_a_ur=$(cat ${folder_ur}_tcp/avgrate_a)
				declare -i avgrate_b_ur=$(cat ${folder_ur}_tcp/avgrate_b)
				
				
				totrate_ud=$avgrate_a_ud*5+$avgrate_b_ud*5
				totrate_ur=$avgrate_a_ur*5+$avgrate_b_ur*5


				declare -i avgqsrtt_ud=500000/${avgqs_a_rtt_ud}+500000/${avgqs_b_rtt_ud}
				declare -i avgqsrtt_ur=500000/${avgqs_a_rtt_ur}+500000/${avgqs_b_rtt_ur}


				fairwin_ud=${totrate_ud}/${avgqsrtt_ud}/1000000
				fairwin_ur=${totrate_ur}/${avgqsrtt_ur}/1000000

			elif [ "$m" == "wb" ]; then
				fn="wr"
				line_ud=$(cat ${folder_ud}_tcp/${fn})
				line_ur=$(cat ${folder_ur}_tcp/${fn})
				filecontent_ud=${filecontent_ud}${udprate}" "${line_ud}$'\n'
				filecontent_ur=${filecontent_ur}${udprate}" "${line_ur}$'\n'
			elif [ "$m" == "ts" ]; then
				fn="stat_rpf"
				fn1="rateshare"
				avgqs_a_rtt_ud=$(cat ${folder_ud}_tcp/avgqs_a)+${rtt}
				avgqs_b_rtt_ud=$(cat ${folder_ud}_tcp/avgqs_b)+${rtt}
				
				avgqs_a_rtt_ur=$(cat ${folder_ur}_tcp/avgqs_a)+${rtt}
				avgqs_b_rtt_ur=$(cat ${folder_ur}_tcp/avgqs_b)+${rtt}

				declare -i avgrate_a_ud=$(cat ${folder_ud}_tcp/avgrate_a)
				declare -i avgrate_b_ud=$(cat ${folder_ud}_tcp/avgrate_b)
				declare -i avgrate_udp_ud=$(cat ${folder_ud}_udp/avgrate_a)
				
				declare -i avgrate_a_ur=$(cat ${folder_ur}_tcp/avgrate_a)
				declare -i avgrate_b_ur=$(cat ${folder_ur}_tcp/avgrate_b)
				declare -i avgrate_udp_ur=$(cat ${folder_ur}_udp/avgrate_b)

				totlinkratebytes="12500000"
				line_ud="${udprate} ${avgrate_udp_ud} ${avgrate_b_ud} ${avgrate_a_ud} ${totlinkratebytes}"
				line_ur="${udprate} ${avgrate_udp_ur} ${avgrate_b_ur} ${avgrate_a_ur} ${totlinkratebytes}"
			
				fairrate_ud=$avgrate_a_ud+$avgrate_b_ud
				fairrate_ud=$fairrate_ud/2

				fairrate_ur=$avgrate_a_ur+$avgrate_b_ur
				fairrate_ur=$fairrate_ur/2

			elif [ "$m" == "qd" ]; then
				fn="stat_qs"
			elif [ "$m" == "ls" ]; then
				fn="stat_drops"
			elif [ "$m" == "marks" ]; then
				fn="stat_marks"
			fi

			if [ "$m" != "wb" ]; then
				line_a_ud=$(cat ${folder_ud}_tcp/${fn}_a)
				line_a_ur=$(cat ${folder_ur}_tcp/${fn}_a)
			fi
			if [ "$m" == "ws" ]; then
				line_a_ud=${line_a_ud}" "$fairwin_ud
				line_a_ur=${line_a_ur}" "$fairwin_ur
			elif [ "$m" == "ts" ]; then
				line_a_ud=${line_a_ud}" "$fairrate_ud
				line_a_ur=${line_a_ur}" "$fairrate_ur
				filecontent_rateshare_ud=${filecontent_rateshare_ud}${line_ud}$'\n'
				filecontent_rateshare_ur=${filecontent_rateshare_ur}${line_ur}$'\n'
			fi
			
			if [ "$m" != "wb" ]; then
				line_udp_d=$(cat ${folder_ud}_udp/${fn}_a)

				filecontent_a_ud=${filecontent_a_ud}${udprate}" "${line_a_ud}$'\n'
				filecontent_a_ur=${filecontent_a_ur}${udprate}" "${line_a_ur}$'\n'

				filecontent_udp_d=${filecontent_udp_d}${udprate}" "${line_udp_d}$'\n'
			fi
			if [ "$m" != "wb" ]; then

				line_b_ud=$(cat ${folder_ud}_tcp/${fn}_b)
				line_b_ur=$(cat ${folder_ur}_tcp/${fn}_b)
				if [ "$m" == "ws" ]; then
					line_b_ud=${line_b_ud}" $fairwin_ud"
					line_b_ur=${line_b_ur}" $fairwin_ur"
				elif [ "$m" == "ts" ]; then
					line_b_ud=${line_a_ud}" $fairrate_ud"
					line_b_ur=${line_a_ur}" $fairrate_ur"
				fi
				line_udp_r=$(cat ${folder_ur}_udp/${fn}_b)

				filecontent_b_ud=${filecontent_b_ud}${udprate}" "${line_b_ud}$'\n'
				filecontent_b_ur=${filecontent_b_ur}${udprate}" "${line_b_ur}$'\n'
				filecontent_udp_r=${filecontent_udp_r}${udprate}" "${line_udp_r}$'\n'

			fi

		done
		if [ "$m" == "ts" ]; then
			echo "$filecontent_rateshare_ud" > ${filename}"rateshare_ud"
			echo "$filecontent_rateshare_ur" > ${filename}"rateshare_ur"
		fi
		echo "$filecontent_a_ud" > ${filename}"_a_ud"
		echo "$filecontent_a_ur" > ${filename}"_a_ur"
		echo "$filecontent_ud" > ${filename}"_ud"
		echo "$filecontent_ur" > ${filename}"_ur"

		echo "$filecontent_b_ud" > ${filename}"_b_ud"
		echo "$filecontent_b_ur" > ${filename}"_b_ur"
		

		echo "$filecontent_udp_d" > ${filename}"_udp_ud"
		echo "$filecontent_udp_r" > ${filename}"_udp_ur"
	done
			
	
}

function do_header() {
	x_count=0
	aqm_count=0
	rtt_count=0
	for aqc in "${aqm_labels[@]}"; do
		aqm_count=$aqm_count+1
		for linkc in "${link_array[@]}"; do
			for rttc in "${rtt_array[@]}"; do
				x_count=$x_count+1
			done
			x_count=$x_count+1
		done
		x_count=$x_count+1
	done
	x_count=$x_count-2

	for rttc in "${rtt_array[@]}"; do
		rtt_count=$rtt_count+1
	done
	x_count=$x_count+1

	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
'
if [ "$aqm_count" == "2" ]; then
	gpi=$gpi'set xlabel "Link[Mbps]:  4            |           12           |           40           |          120          |          200                        4           |          12          |          40          |          120          |          200                     " 
'
elif [ "$aqm_count" == "3" ]; then
	gpi=$gpi'set xlabel "Link[Mbps]:  4       |      12      |      40      |     120     |     200                 4       |      12      |      40      |     120      |      200                4       |      12      |      40      |      120      |      200                  " 
'
else
	gpi=$gpi'set xlabel "Link[Mbps]:  4            |           12           |           40           |          120          |          200                        4           |          12          |          40          |          120          |          200                     " 
'
fi
gpi=$gpi'set label 2 "RTT[ms]:" at screen 0.1,0.6 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'

gpi=$gpi"set xrange [-1:$x_count]"$'\n'

	aqm_step="$((12 / $aqm_count))-1"
	aqm_start=2
	declare -i acq
	for (( aqc=0; aqc<$aqm_count; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start},3.4 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done

}

function genplot_tb_aqm() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/tb_${testcase_id}_aqm.eps\""$'\n'
	gpi=$gpi'set ylabel "Rate balance [ratio] "
set yrange [0.01:50]
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 index=0
	gpi=$gpi"plot "
	title_tb=""
	for aqm in "${aqm_array[@]}"; do 
		aqmname="m_${aqm}"
		get_cc_captions_colors
		title_tb="${cc_a}/${cc_b} ratio"
		if [ $(contains "${used_titles[@]}" "${title_tb}") == "y" ]; then
			title_tb=""
		else
			used_titles[used_titles_index]=$title_tb
			used_titles_index=$used_titles_index+1
		fi
		
		for link in "${link_array[@]}"; do
			filename_tb=${targetfolder}"/data/"${aqmname}"_"${link}"_tb"_${testcase_id}
			gpi=$gpi"'${filename_tb}' using (\$0+$gap):2:xtic(1) with points ls 3 lw 80 lc rgb '${color_a}' title \"${title_tb}\", " 
			gap=$gap+1+$rtt_count
			title_tb=""
		done

		index=$index+1
		gap=$gap+1
	done
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/tb_${testcase_id}_aqm.gpi
	gnuplot ${targetfolder}/tb_${testcase_id}_aqm.gpi
}

function genplot_wb_aqm() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/wb_${testcase_id}_aqm.eps\""$'\n'
	gpi=$gpi'set ylabel "Window balance [ratio]"
set yrange [0.01:20]
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0
	gpi=$gpi"plot "
	title_wb=""
	for aqm in "${aqm_array[@]}"; do 
		aqmname="m_${aqm}"
		get_cc_captions_colors
		title_wb="${cc_a}/${cc_b} ratio"
		if [ $(contains "${used_titles[@]}" "${title_wb}") == "y" ]; then
			title_wb=""
		else
			used_titles[used_titles_index]=$title_wb
			used_titles_index=$used_titles_index+1
		fi
		for link in "${link_array[@]}"; do
			filename_wb=${targetfolder}"/data/"${aqmname}"_"${link}"_wb"_${testcase_id}
			gpi=$gpi"'${filename_wb}' using (\$0+$gap):2:xtic(1) with points ls 3 lw 80 lc rgb '${color_a}' title \"${title_wb}\", " 
			gap=$gap+1+$rtt_count
			title_wb=""
		done
		gap=$gap+1
	done
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/wb_${testcase_id}_aqm.gpi
	gnuplot ${targetfolder}/wb_${testcase_id}_aqm.gpi
}

function count_extra_testcases() {
	testcase_count=0
	for tc in "${testcases_to_plot_extra[@]}"; do
		testcase_count=$testcase_count+1
	done
}

function genplot_tb_aqm_extra() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/tb_aqm_extra.eps\""$'\n'
	gpi=$gpi'set ylabel "Rate balance [ratio] "
set yrange [0.01:10]
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 linestyle
	gpi=$gpi"plot "
	title_tb=""
	for aqm in "${aqm_array[@]}"; do 
		aqmname="m_${aqm}"
		get_cc_captions_colors
		
		for link in "${link_array[@]}"; do
			linestyle=1
			for t in "${testcases_to_plot_tb[@]}"; do
				testcase=$t
				gen_foldername_id			
				filename_tb=${targetfolder}"/data/"${aqmname}"_"${link}"_tb"_${testcase_id}

				title_tb="${cc_a}/${cc_b} ratio ${testcase_id}"
				if [ $(contains "${used_titles[@]}" "${title_tb}") == "y" ]; then
					title_tb=""
				else
	 				used_titles[used_titles_index]=$title_tb
					used_titles_index=$used_titles_index+1
				fi
				gpi=$gpi"'${filename_tb}' using (\$0+$gap):3:xtic(1) with points ls ${linestyle} lw 35 lc rgb '${color_a}' title \"${title_tb}\", " 
				title_tb=""
				linestyle=$linestyle+1
			done
			gap=$gap+1+$rtt_count

		done
		gap=$gap+1
	done
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/tb_aqm_extra.gpi
	gnuplot ${targetfolder}/tb_aqm_extra.gpi
}


function genplot_ws_link40_rtt10() {
	count_extra_testcases
	declare -i xmax=$testcase_count*2
	#xmax=$xmax+1
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute
set xlabel "Nr of flows"
'
gpi=$gpi"set xrange [-1:62]"$'\n'
gpi=$gpi'
#set label 2 "RTT[ms]:" at screen 0.1,0.6 front font "Times-Roman,100" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
gpi=$gpi"set label 3 \"${aqm_labels[0]}\" at screen  1.8,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
gpi=$gpi"set label 4 \"${aqm_labels[1]}\" at screen  5.1,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
gpi=$gpi"set label 5 \"${aqm_labels[2]}\" at screen  8.7,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
	gpi=$gpi"set output \"${targetfolder}/ws_link40_rtt10.eps\""$'\n'
	gpi=$gpi'set ylabel "Window per flow normalised"
#set yrange [0.01:10]
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 
	gpi=$gpi"plot "
	title_a_mean=""
	title_b_mean=""

	link=40
	
	for aqm in "${aqm_array[@]}"; do
		
		aqmname="m_${aqm}"
		get_cc_captions_colors
		echo "color_a" $color_a
		echo "color_b" $color_b

		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_extra_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_extra_b"

		title_a_mean="${cc_a}(A) P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}(B)  P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi

		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_extra_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_extra_b"
		gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9):xtic(1) with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_a_mean}\", "

		smallgap="$gap".4
		gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9) with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_b_mean}\", "
		gap=$gap+1+$testcase_count
	
		
	done
	
	

	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ws_link40_rtt10.gpi
	gnuplot ${targetfolder}/ws_link40_rtt10.gpi
}

function genplot_wb_link40_rtt10() {
	count_extra_testcases
	declare -i xmax=$testcase_count*2
	#xmax=$xmax+1
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute
set xlabel "Nr of flows"
'
gpi=$gpi"set xrange [-1:62]"$'\n'
gpi=$gpi'
#set label 2 "RTT[ms]:" at screen 0.1,0.6 front font "Times-Roman,100" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
gpi=$gpi"set label 3 \"${aqm_labels[0]}\" at screen  1.8,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
gpi=$gpi"set label 4 \"${aqm_labels[1]}\" at screen  5.1,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
gpi=$gpi"set label 5 \"${aqm_labels[2]}\" at screen  8.7,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
	gpi=$gpi"set output \"${targetfolder}/wb_link40_rtt10.eps\""$'\n'
	gpi=$gpi'set ylabel "Window balance per flow"
#set yrange [0.01:10]
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 gap_start
	gpi=$gpi"plot "
	title_wb=""
	link=40
	declare -i index=0
	for aqm in "${aqm_array[@]}"; do
		aqmname="m_${aqm}"
		get_cc_captions_colors

		filename_wb=${targetfolder}"/data/"${aqmname}"_"${link}"_wb_extra"
		title_wb="${cc_a}(A)/${cc_b}(B) ratio"
		if [ $(contains "${used_titles[@]}" "${title_wb}") == "y" ]; then
			title_wb=""
		else
			used_titles[used_titles_index]=$title_wb
			used_titles_index=$used_titles_index+1
		fi
		
		gpi=$gpi"'${filename_wb}' using (\$0+$gap):2:xtic(1) with points ls 3 lw 80 lc rgb '${color_a}' title \"${title_wb}\", " 
		title_tb=""
		
		gap=$gap+1+$testcase_count
		index=$index+1

	done
	
	

	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/wb_link40_rtt10.gpi
	gnuplot ${targetfolder}/wb_link40_rtt10.gpi
}

function genplot_tb_link40_rtt10() {
	count_extra_testcases
	declare -i xmax=$testcase_count*2
	#xmax=$xmax+1
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute
set xlabel "Nr of flows"
'
gpi=$gpi"set xrange [-1:62]"$'\n'
gpi=$gpi'
#set label 2 "RTT[ms]:" at screen 0.1,0.6 front font "Times-Roman,100" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
gpi=$gpi"set label 3 \"${aqm_labels[0]}\" at screen  1.8,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
gpi=$gpi"set label 4 \"${aqm_labels[1]}\" at screen  5.1,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
gpi=$gpi"set label 5 \"${aqm_labels[2]}\" at screen  8.7,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
	gpi=$gpi"set output \"${targetfolder}/tb_link40_rtt10.eps\""$'\n'
	gpi=$gpi'set ylabel "Rate balance per flow"
#set yrange [0.01:10]
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 gap_start
	gpi=$gpi"plot "
	title_wb=""
	link=40
	declare -i index=0
	for aqm in "${aqm_array[@]}"; do
		aqmname="m_${aqm}"
		get_cc_captions_colors

		filename_wb=${targetfolder}"/data/"${aqmname}"_"${link}"_tb_extra"
		title_wb="${cc_a}(A)/${cc_b}(B) ratio"
		if [ $(contains "${used_titles[@]}" "${title_wb}") == "y" ]; then
			title_wb=""
		else
			used_titles[used_titles_index]=$title_wb
			used_titles_index=$used_titles_index+1
		fi
		
		gpi=$gpi"'${filename_wb}' using (\$0+$gap):2:xtic(1) with points ls 3 lw 80 lc rgb '${color_a}' title \"${title_wb}\", " 
		title_tb=""
		
		gap=$gap+1+$testcase_count
		index=$index+1

	done
	
	

	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/tb_link40_rtt10.gpi
	gnuplot ${targetfolder}/tb_link40_rtt10.gpi
}

function genplot_ts_link40_rtt10() {
	count_extra_testcases
	declare -i xmax=$testcase_count*2
	#xmax=$xmax+1
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute
set xlabel "Nr of flows"
'
gpi=$gpi"set xrange [-1:62]"$'\n'
gpi=$gpi'
#set label 2 "RTT[ms]:" at screen 0.1,0.6 front font "Times-Roman,100" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
gpi=$gpi"set label 3 \"${aqm_labels[0]}\" at screen  1.8,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
gpi=$gpi"set label 4 \"${aqm_labels[1]}\" at screen  5.1,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
gpi=$gpi"set label 5 \"${aqm_labels[2]}\" at screen  8.7,3.4 font \"Times-Roman,120\" tc rgb \"black\" left"$'\n'
	gpi=$gpi"set output \"${targetfolder}/ts_link40_rtt10.eps\""$'\n'
	gpi=$gpi'set ylabel "Normalised rate per flow"
#set yrange [0.001:100]
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 
	gpi=$gpi"plot "
	title_a_mean=""
	title_b_mean=""

	link=40
	
	for aqm in "${aqm_array[@]}"; do
		
		aqmname="m_${aqm}"
		get_cc_captions_colors
		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_extra_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_extra_b"

		title_a_mean="${cc_a}(A) P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}(B)  P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi

		gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9):xtic(1) with candlesticks ls 1 lw 35 lc rgb '${color_a}' title \"${title_a_mean}\", "

			smallgap="$gap".4
		gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9) with candlesticks ls 1 lw 35 lc rgb '${color_b}' title \"${title_b_mean}\", "
		gap=$gap+1+$testcase_count


	done
	
	

	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ts_link40_rtt10.gpi
	gnuplot ${targetfolder}/ts_link40_rtt10.gpi
}

function genplot_ts_aqm() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/ts_${testcase_id}_aqm.eps\""$'\n'
	gpi=$gpi'set ylabel "Normalised rate per flow "
set yrange [0.01:10]
set logscale y
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 fairrate totalflows
	gpi=$gpi"plot "
	title_a_mean=""
	title_b_mean=""
	for aqm in "${aqm_array[@]}"; do 
		aqmname="m_${aqm}"
		color_a=""
		get_cc_captions_colors
		title_a_mean="${cc_a} P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}  P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		for link in "${link_array[@]}"; do
			totalflows=$a_flows+$b_flows
			fairrate=$link*125000/$totalflows
			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_a"_${testcase_id}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_b"_${testcase_id}
			#echo "color_a: "$color_a
			gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/${fairrate}):(\$7/${fairrate}):(\$4/${fairrate}):(\$3/${fairrate}):xtic(1) with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_a_mean}\", "

			smallgap="$gap".4
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/${fairrate}):(\$7/${fairrate}):(\$4/${fairrate}):(\$3/${fairrate}) with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_b_mean}\", "
			gap=$gap+1+$rtt_count
			title_a_mean=""
			title_b_mean=""
		done
		gap=$gap+1
	done
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ts_${testcase_id}_aqm.gpi
	gnuplot ${targetfolder}/ts_${testcase_id}_aqm.gpi
}


function genplot_ts_aqm_dualpi2() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 70
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
'
	gpi=$gpi'set xlabel "Link[Mbps]:  4            |           12           |           40           |          120          |          200              " 
'

gpi=$gpi'set label 2 "RTT[ms]:" at screen 0.1,0.4 front font "Times-Roman,70" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'

#set xlabel "Link[Mbps]:     4           |         40        |         200                  4           |         40        |         200                 4           |         40        |         200                  4           |         40        |         200                 4           |         40        |         200                  4           |         40        |         200                       "


	
		gpi=$gpi"set label 3$aqc \"DUALPI2 (DCTCP+Cubic)\" at screen ${aqm_start}.9,3.4 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
	


	gpi=$gpi"set output \"${targetfolder}/ts_${testcase_id}_aqm.eps\""$'\n'
	gpi=$gpi'set ylabel "Normalised rate per flow " offset 2
	set size 4,4
set size ratio 0.75
set yrange [0.1:2]
set logscale y
set xrange [-1:29]
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 fairrate totalflows
	gpi=$gpi"plot "
	title_a_mean=""
	title_b_mean=""

		aqmname="m_dcdualpi2"
		aqm="m_dcdualpi2"

		color_a=""
		get_cc_captions_colors
		title_a_mean="${cc_a} P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}  P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		for link in "${link_array[@]}"; do
			totalflows=$a_flows+$b_flows
			fairrate=$link*125000/$totalflows
			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_a"_${testcase_id}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_b"_${testcase_id}
			gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/${fairrate}):(\$7/${fairrate}):(\$4/${fairrate}):(\$3/${fairrate}):xtic(1) with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_a_mean}\", "

			smallgap="$gap".4
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/${fairrate}):(\$7/${fairrate}):(\$4/${fairrate}):(\$3/${fairrate}) with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_b_mean}\", "
			gap=$gap+1+$rtt_count
			title_a_mean=""
			title_b_mean=""
		done
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ts_${testcase_id}_aqm.gpi
	gnuplot ${targetfolder}/ts_${testcase_id}_aqm.gpi
}

function genplot_ws_aqm() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/ws_${testcase_id}_aqm.eps\""$'\n'
	gpi=$gpi'set ylabel "Window per flow normalised"
	set yrange [0.01:12]
set logscale y
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 fairwin 
	gpi=$gpi"plot "
	title_a_mean=""
	title_b_mean=""
	for aqm in "${aqm_array[@]}"; do 
		aqmname="m_${aqm}"
		color_a=""
		get_cc_captions_colors
		title_a_mean="${cc_a} P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}  P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		for link in "${link_array[@]}"; do
			totalflows=$a_flows+$b_flows
			#fairrate=$link*125000/$totalflows
			fairwin=1
			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_a"_${testcase_id}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_b"_${testcase_id}

			gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9):xtic(1) with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_a_mean}\", "

			smallgap="$gap".4
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9) with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_b_mean}\", "
			gap=$gap+1+$rtt_count
			title_a_mean=""
			title_b_mean=""
		done
		gap=$gap+1
	done
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ws_${testcase_id}_aqm.gpi
	gnuplot ${targetfolder}/ws_${testcase_id}_aqm.gpi
}

function genplot_qd_aqm() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/qd_${testcase_id}_aqm.eps\""$'\n'
	gpi=$gpi'set ylabel "Queue delay [ms]"
set boxwidth 0.2
'
if [ "$testcase" == "s1d0s1d0" ]; then
	gpi=$gpi"set yrange [0:70]"$'\n'
else
	gpi=$gpi"set yrange [0:150]"$'\n'
fi
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean="${cc_a} mean, P_{99}"
	title_b_mean="${cc_b} mean, P_{99}"
	for aqm in "${aqm_array[@]}"; do 
		for link in "${link_array[@]}"; do
			aqmname="m_${aqm}"
			get_cc_captions_colors
			title_a_mean="${cc_a} mean, P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
    			title_a_mean=""
			else
				used_titles[used_titles_index]=$title_a_mean
				used_titles_index=$used_titles_index+1
			fi
			title_b_mean="${cc_b} mean, P_{99}"
			title_b_p99="P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
    			title_b_mean=""
			else
				used_titles[used_titles_index]=$title_b_mean
				used_titles_index=$used_titles_index+1
			fi
			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_a"_${testcase_id}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_b"_${testcase_id}
			gpi=$gpi"'${filename_a}' using (\$0+${gap}):3:(\$7-\$3):xtic(1) with boxerrorbars ls 1 lw 10 lc rgb '${color_a}' title \"${title_a_mean}\", '' using (\$0+$gap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_a}' ps 3 title '', "
			smallgap="$gap".3
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):3:(\$7-\$3) with boxerrorbars ls 1 lw 10 lc rgb '${color_b}' title \"${title_b_mean}\", '' using (\$0+$smallgap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_b}' ps 3 title '', " 
			gap=$gap+1+$rtt_count
			title_a_mean=""
			title_a_p99=""
			title_b_mean=""
			title_b_p99=""
		done
		gap=$gap+1
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/qd_${testcase_id}_aqm.gpi
	gnuplot ${targetfolder}/qd_${testcase_id}_aqm.gpi
}

function genplot_qd_aqm_logscale() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/qd_${testcase_id}_aqm_logscale.eps\""$'\n'
	gpi=$gpi'set ylabel "Queue delay [ms]"
set boxwidth 0.2
set logscale y
'
if [ "$testcase" == "s1d0s1d0" ]; then
	gpi=$gpi"set yrange [0.001:70]"$'\n'
else
	gpi=$gpi"set yrange [0.001:200]"$'\n'
fi
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean="${cc_a} mean, P_{99}"
	title_b_mean="${cc_b} mean, P_{99}"
	for aqm in "${aqm_array[@]}"; do 
		for link in "${link_array[@]}"; do
			aqmname="m_${aqm}"
			get_cc_captions_colors
			title_a_mean="${cc_a} mean, P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
    			title_a_mean=""
			else
				used_titles[used_titles_index]=$title_a_mean
				used_titles_index=$used_titles_index+1
			fi
			title_b_mean="${cc_b} mean, P_{99}"
			title_b_p99="P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
    			title_b_mean=""
			else
				used_titles[used_titles_index]=$title_b_mean
				used_titles_index=$used_titles_index+1
			fi
			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_a"_${testcase_id}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_b"_${testcase_id}
			gpi=$gpi"'${filename_a}' using (\$0+${gap}):3:(\$7-\$3):xtic(1) with boxerrorbars ls 1 lw 10 lc rgb '${color_a}' title \"${title_a_mean}\", '' using (\$0+$gap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_a}' ps 3 title '', "
			smallgap="$gap".3
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):3:(\$7-\$3) with boxerrorbars ls 1 lw 10 lc rgb '${color_b}' title \"${title_b_mean}\", '' using (\$0+$smallgap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_b}' ps 3 title '', " 
			gap=$gap+1+$rtt_count
			title_a_mean=""
			title_a_p99=""
			title_b_mean=""
			title_b_p99=""
		done
		gap=$gap+1
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/qd_${testcase_id}_aqm_logscale.gpi
	gnuplot ${targetfolder}/qd_${testcase_id}_aqm_logscale.gpi
}

function genplot_qd_mrtt2() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
set xlabel "Link[Mbps]:                  4                           |                         12                         |                        40                        |                       120                     |                        200                                    " 
set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.1,0.8 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.2
'
gpi=$gpi"set label 3 \"${aqm_label}\" at screen 6,3.6 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'

	gpi=$gpi"set output \"${targetfolder}/qd_${testcase_id}_${cong}${cur}_mrtt2.eps\""$'\n'
	gpi=$gpi'set ylabel "Queue delay [ms]"
	set yrange [0:]
	set xrange [-1:65]
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean="${cc_a} mean"
	title_a_p99="P_{99}"
	title_b_mean="${cc_b} mean"
	title_b_p99="P_{99}"

	for link in "${link_array[@]}"; do
		get_cc_captions_colors
		title_a_mean="${cc_a} mean"
		title_a_p99="P_{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
			title_a_p99=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b} mean"
		title_b_p99="P_{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
			title_b_p99=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_mrtt2_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_mrtt2_b"
		gpi=$gpi"'${filename_a}' using (\$0+${gap}):3:(\$7-\$3):xtic(1) with boxerrorbars ls 1 lw 10 lc rgb '${color_a}' title \"${title_a_mean}\", '' using (\$0+$gap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_a}' ps 3 title \"${title_a_p99}\", "
		smallgap="$gap".3
		gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):3:(\$7-\$3) with boxerrorbars ls 1 lw 10 lc rgb '${color_b}' title \"${title_b_mean}\", '' using (\$0+$smallgap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_b}' ps 3 title \"${title_b_p99}\", " 
		title_a_mean=""
		title_a_p99=""
		title_b_mean=""
		title_b_p99=""
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/qd_${testcase_id}_${cong}${cur}_mrtt2.gpi
	gnuplot ${targetfolder}/qd_${testcase_id}_${cong}${cur}_mrtt2.gpi
}

function genplot_ls_mrtt2() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
set xlabel "Link[Mbps]:                  4                           |                         12                         |                        40                        |                       120                     |                        200                                    " 
set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.3,0.8 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
gpi=$gpi"set label 3 \"${aqm_label}\" at screen 6,3.5 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'

	gpi=$gpi"set output \"${targetfolder}/ls_${testcase_id}_${cong}${cur}_mrtt2.eps\""$'\n'
	gpi=$gpi'set ylabel "Drop/Mark prob. [log(%)]"
set yrange [0.001:100]
set logscale y
	set xrange [-1:65]
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean=""
	title_b_mean=""

	for link in "${link_array[@]}"; do
		get_cc_captions_colors
		title_a_mean="${cc_a} P{25}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b} P{25}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_mrtt2_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_mrtt2_b"
		gpi=$gpi"'${filename_a}' using (\$0+${gap}):3:7:5:3:xtic(1) with candlesticks ls 1 lw 35 lc rgb '${color_a}' title \"${title_a_mean}\", "

			smallgap="$gap".4
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):3:7:5:3 with candlesticks ls 1 lw 35 lc rgb '${color_b}' title \"${title_b_mean}\", "
		title_a_mean=""

		title_b_mean=""
		gap=$gap+13
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ls_${testcase_id}_${cong}${cur}_mrtt2.gpi
	gnuplot ${targetfolder}/ls_${testcase_id}_${cong}${cur}_mrtt2.gpi
}

function genplot_ut_mrtt2() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
set xlabel "Link[Mbps]:                  4                           |                         12                         |                        40                        |                       120                     |                        200                                    " 
set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.3,0.8 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
gpi=$gpi"set label 3 \"${aqm_label}\" at screen 6,3.5 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'

	gpi=$gpi"set output \"${targetfolder}/ut_${testcase_id}_${cong}${cur}_mrtt2.eps\""$'\n'
	gpi=$gpi'set ylabel "Link utilisation"
set yrange [60:100]
	set xrange [-1:65]
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_tb="${cc_a}+${cc_b} P{1}, mean, P{99}"

	for link in "${link_array[@]}"; do
		get_util_color
		if [ $(contains "${used_titles[@]}" "${title_tb}") == "y" ]; then
			title_tb=""
		else
			used_titles[used_titles_index]=$title_tb
			used_titles_index=$used_titles_index+1
		fi
		filename_tb=${targetfolder}"/data/"${aqmname}"_"${link}"_ut_mrtt2"
		gpi=$gpi"'${filename_tb}' using (\$0+${gap}.25):3:7:4:3:xtic(1) with candlesticks ls 1 lw 35  lc rgb '${color_gen}' title \"${title_tb}\", " 

		
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ut_${testcase_id}_${cong}${cur}_mrtt2.gpi
	gnuplot ${targetfolder}/ut_${testcase_id}_${cong}${cur}_mrtt2.gpi
}

function genplot_tb_mrtt2() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
set xlabel "Link[Mbps]:                  4                           |                         12                         |                        40                        |                       120                     |                        200                                    " 
set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.3,0.8 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
gpi=$gpi"set label 3 \"${aqm_label}\" at screen 6,3.5 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'

	gpi=$gpi"set output \"${targetfolder}/tb_${testcase_id}_${cong}${cur}_mrtt2.eps\""$'\n'
	gpi=$gpi'set ylabel "Rate balance [ratio] 1"
	set yrange [0.01:10]
set logscale y
	set xrange [-1:65]
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_tb="${cc_a}+${cc_b}"

	for link in "${link_array[@]}"; do
		get_cc_captions_colors
		title_tb="flow A (${cc_b}) / flow B (${cc_a}) ratio"
		if [ $(contains "${used_titles[@]}" "${title_tb}") == "y" ]; then
			title_tb=""
		else
			used_titles[used_titles_index]=$title_tb
			used_titles_index=$used_titles_index+1
		fi
		filename_tb=${targetfolder}"/data/"${aqmname}"_"${link}"_tb_mrtt2"
		gpi=$gpi"'${filename_tb}' using (\$0+$gap):2:xtic(1) with points ls 3 lw 80 lc rgb '${color_a}' title \"${title_tb}\", " 
		title_tb=""
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/tb_${testcase_id}_${cong}${cur}_mrtt2.gpi
	gnuplot ${targetfolder}/tb_${testcase_id}_${cong}${cur}_mrtt2.gpi
}

function genplot_ts_mrtt2() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
set xlabel "Link[Mbps]:                  4                           |                         12                         |                        40                        |                       120                     |                        200                                    " 
set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.3,0.8 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
gpi=$gpi"set label 3 \"${aqm_label}\" at screen 6,3.5 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'

	gpi=$gpi"set output \"${targetfolder}/ts_${testcase_id}_${cong}${cur}_mrtt2.eps\""$'\n'
	gpi=$gpi'set ylabel "Normalised rate per flow"
set yrange [0.1:10]
set logscale y
	set xrange [-1:65]
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0
	declare -i gap=0
	gpi=$gpi"plot "
title_a_mean="${cc_a} mean"
	title_a_p99="P_{99}"
	title_b_mean="${cc_b} mean"
	title_b_p99="P_{99}"
	declare -i totalflows
	for link in "${link_array[@]}"; do
		get_cc_captions_colors
		title_a_mean="${cc_a} P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}  P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		
		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_mrtt2_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_mrtt2_b"

		gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9):xtic(1) with candlesticks ls 1 lw 35 lc rgb '${color_a}' title \"${title_a_mean}\", "

		smallgap="$gap".4
		gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9) with candlesticks ls 1 lw 35 lc rgb '${color_b}' title \"${title_b_mean}\", "

		title_a_mean=""
		title_b_mean=""
		
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ts_${testcase_id}_${cong}${cur}_mrtt2.gpi
	gnuplot ${targetfolder}/ts_${testcase_id}_${cong}${cur}_mrtt2.gpi
}

function genplot_ws_mrtt2() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
set xlabel "Link[Mbps]:                  4                           |                         12                         |                        40                        |                       120                     |                        200                                    " 
set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.3,0.8 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
gpi=$gpi"set label 3 \"${aqm_label}\" at screen 6,3.5 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'

	gpi=$gpi"set output \"${targetfolder}/ws_${testcase_id}_${cong}${cur}_mrtt2.eps\""$'\n'
	gpi=$gpi'set ylabel "Window size per flow"
#set yrange [0.01:10]
set logscale y
	set xrange [-1:65]
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
title_a_mean="${cc_a} mean"
	title_a_p99="P_{99}"
	title_b_mean="${cc_b} mean"
	title_b_p99="P_{99}"
	declare -i totalflows
	for link in "${link_array[@]}"; do
		get_cc_captions_colors
		title_a_mean="${cc_a} P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}  P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		
		
		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_mrtt2_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_mrtt2_b"
		gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/\$8):(\$7/\$8):(\$4/\$8):(\$3/\$8):xtic(1) with candlesticks ls 1 lw 35 lc rgb '${color_a}' title \"${title_a_mean}\", "

		smallgap="$gap".4
		gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$8):(\$7/\$8):(\$4/\$8):(\$3/\$8) with candlesticks ls 1 lw 35 lc rgb '${color_b}' title \"${title_b_mean}\", "

		title_a_mean=""
		title_b_mean=""
		
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ws_${testcase_id}_${cong}${cur}_mrtt2.gpi
	gnuplot ${targetfolder}/ws_${testcase_id}_${cong}${cur}_mrtt2.gpi
}

function genplot_qd_mrtt2_aqm_40() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
'

gpi=$gpi'set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.1,0.4 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.2
'
	gpi=$gpi"set output \"${targetfolder}/qd_mrtt2_link40.eps\""$'\n'
	gpi=$gpi'set ylabel "Queue delay [ms]"
	set yrange [0:]
	set xrange [-1:38]
'
declare -i acq
declare -i aqm_start=1
declare -i 	aqm_step=3

	for (( aqc=0; aqc<3; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start}.9,3.1 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done
	used_titles=("" "" "" "" "" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean=""
	title_a_p99=""
	title_b_mean=""
	title_b_p99=""

	link=40
	for aqm in "${aqm_array[@]}"; do
		aqmname="mr2_${aqm}"
		get_cc_captions_colors
		title_a_mean="${cc_a}(A) mean"
		title_a_p99="P_{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
			title_a_p99=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}(B) mean"
		title_b_p99="P_{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
			title_b_p99=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_mrtt2_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_mrtt2_b"
		gpi=$gpi"'${filename_a}' using (\$0+${gap}):3:(\$7-\$3):xtic(1) with boxerrorbars ls 1 lw 10 lc rgb '${color_a}' title \"${title_a_mean}\", '' using (\$0+$gap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_a}' ps 3 title \"${title_a_p99}\", "
		smallgap="$gap".3
		gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):3:(\$7-\$3) with boxerrorbars ls 1 lw 10 lc rgb '${color_b}' title \"${title_b_mean}\", '' using (\$0+$smallgap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_b}' ps 3 title \"${title_b_p99}\", " 
		title_a_mean=""
		title_a_p99=""
		title_b_mean=""
		title_b_p99=""
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/qd_mrtt2_link40_${aqmfn}.gpi
	gnuplot ${targetfolder}/qd_mrtt2_link40_${aqmfn}.gpi
}

function genplot_qd_mrtt2_aqm_40_logscale() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
'

gpi=$gpi'set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.1,0.4 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.2
'
	gpi=$gpi"set output \"${targetfolder}/qd_mrtt2_link40_logscale.eps\""$'\n'
	gpi=$gpi'set ylabel "Queue delay [ms]"
	set yrange [0.001:]
	set xrange [-1:38]
	set logscale y
'
declare -i acq
declare -i aqm_start=1
declare -i 	aqm_step=3

	for (( aqc=0; aqc<3; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start}.9,3.1 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done
	used_titles=("" "" "" "" "" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean=""
	title_a_p99=""
	title_b_mean=""
	title_b_p99=""

	link=40
	for aqm in "${aqm_array[@]}"; do
		aqmname="mr2_${aqm}"
		get_cc_captions_colors
		title_a_mean="${cc_a}(A) mean"
		title_a_p99="P_{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
			title_a_p99=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}(B) mean"
		title_b_p99="P_{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
			title_b_p99=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_mrtt2_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_mrtt2_b"
		gpi=$gpi"'${filename_a}' using (\$0+${gap}):3:(\$7-\$3):xtic(1) with boxerrorbars ls 1 lw 10 lc rgb '${color_a}' title \"${title_a_mean}\", '' using (\$0+$gap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_a}' ps 3 title \"${title_a_p99}\", "
		smallgap="$gap".3
		gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):3:(\$7-\$3) with boxerrorbars ls 1 lw 10 lc rgb '${color_b}' title \"${title_b_mean}\", '' using (\$0+$smallgap):7 with points ls 1 lw 10 pt 5 lc rgb '${color_b}' ps 3 title \"${title_b_p99}\", " 
		title_a_mean=""
		title_a_p99=""
		title_b_mean=""
		title_b_p99=""
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/qd_mrtt2_link40_${aqmfn}_logscale.gpi
	gnuplot ${targetfolder}/qd_mrtt2_link40_${aqmfn}_logscale.gpi
}


function genplot_ls_mrtt2_aqm_40() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
'

gpi=$gpi'set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.1,0.4 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
	gpi=$gpi"set output \"${targetfolder}/ls_mrtt2_link40.eps\""$'\n'
	gpi=$gpi'set ylabel "Mark/Drop probability [%]"
	set yrange [0.001:100]
set logscale y
	set xrange [-1:38]
'
declare -i acq
declare -i aqm_start=1
declare -i 	aqm_step=3

	for (( aqc=0; aqc<3; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start}.9,3.1 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done
	used_titles=("" "" "" "" "" "" "" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean=""
	title_b_mean=""
	title_marks_a=""
	title_marks_b=""
	link=40

	for aqm in "${aqm_array[@]}"; do 
		aqmname="mr2_${aqm}"
		get_cc_captions_colors
		title_a_mean="Drops L4S P{25}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="Drops Classic P{25}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		title_marks_a="Marks ${cc_a} P{25}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_marks_a}") == "y" ]; then
			title_marks_a=""
		else
			used_titles[used_titles_index]=$title_marks_a
			used_titles_index=$used_titles_index+1
		fi
		title_marks_b="Marks ${cc_b} P{25}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_marks_b}") == "y" ]; then
			title_marks_b=""
		else
			used_titles[used_titles_index]=$title_marks_b
			used_titles_index=$used_titles_index+1
		fi

		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_mrtt2_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_mrtt2_b"
		filename_marks_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_mrtt2_marks_a"
		filename_marks_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_mrtt2_marks_b"

		gpi=$gpi"'${filename_b}' using (\$0+${gap}):3:7:5:3:xtic(1) with candlesticks ls 1 lw 30 lc rgb 'red' title \"${title_b_mean}\", "
		smallgap="$gap".3
		gpi=$gpi"'${filename_a}' using (\$0+${gap}):3:7:5:3:xtic(1) with candlesticks ls 1 lw 30 lc rgb 'purple' title \"${title_a_mean}\", "
			smallgap="$gap".6

			gpi=$gpi"'${filename_marks_a}' using (\$0+${smallgap}):3:7:5:3 with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_marks_a}\", "
			smallgap="$gap".9
			gpi=$gpi"'${filename_marks_b}' using (\$0+${smallgap}):3:7:5:3 with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_marks_b}\", "

		title_a_mean=""
		title_b_mean=""
		title_marks_a=""
		title_marks_b=""
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ls_mrtt2_link40.gpi
	gnuplot ${targetfolder}/ls_mrtt2_link40.gpi
}

function genplot_ts_mrtt2_aqm_40() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
'

gpi=$gpi'set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.1,0.4 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
	gpi=$gpi"set output \"${targetfolder}/ts_mrtt2_link40.eps\""$'\n'
	gpi=$gpi'set ylabel "Normalised rate per flow"
	set yrange [0.01:10]
	set logscale y
	set xrange [-1:38]
'
declare -i acq
declare -i aqm_start=1
declare -i 	aqm_step=3

	for (( aqc=0; aqc<3; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start}.9,3.3 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 totalflows

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean=""
	title_a_p99=""
	title_b_mean=""
	title_b_p99=""

	link=40
	for aqm in "${aqm_array[@]}"; do 
		aqmname="mr2_${aqm}"
		get_cc_captions_colors
		title_a_mean="${cc_a}(A) P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}(B) P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		
		filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_mrtt2_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_mrtt2_b"
		
		gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9):xtic(1) with candlesticks ls 1 lw 35 lc rgb '${color_a}' title \"${title_a_mean}\", "

		smallgap="$gap".4
		gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9) with candlesticks ls 1 lw 35 lc rgb '${color_b}' title \"${title_b_mean}\", "
		title_a_mean=""
		title_b_mean=""
		
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ts_mrtt2_link40_${aqmfn}.gpi
	gnuplot ${targetfolder}/ts_mrtt2_link40_${aqmfn}.gpi
}

function genplot_ws_mrtt2_aqm_40() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
'

gpi=$gpi'set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.1,0.4 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
	gpi=$gpi"set output \"${targetfolder}/ws_mrtt2_link40.eps\""$'\n'
	gpi=$gpi'set ylabel "Window per flow normalised"
	set logscale y
	#set yrange [0.01:10]
	set xrange [-1:38]
'
declare -i acq
declare -i aqm_start=1
declare -i 	aqm_step=3

	for (( aqc=0; aqc<3; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start}.9,3.3 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean=""
	title_a_p99=""
	title_b_mean=""
	title_b_p99=""

	link=40
	MIXRTT2=1
	for aqm in "${aqm_array[@]}"; do 
		aqmname="mr2_${aqm}"
		get_cc_captions_colors
		title_a_mean="${cc_a}(A) P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
			title_a_mean=""
		else
			used_titles[used_titles_index]=$title_a_mean
			used_titles_index=$used_titles_index+1
		fi
		title_b_mean="${cc_b}(B) P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
			title_b_mean=""
		else
			used_titles[used_titles_index]=$title_b_mean
			used_titles_index=$used_titles_index+1
		fi
		
		
			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_mrtt2_a"
		filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_mrtt2_b"

			gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9):xtic(1) with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_a_mean}\", "

			smallgap="$gap".4
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9) with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_b_mean}\", "
		title_a_mean=""
		title_b_mean=""
		
		gap=$gap+13

	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ws_mrtt2_link40_${aqmfn}.gpi
	gnuplot ${targetfolder}/ws_mrtt2_link40_${aqmfn}.gpi
}

function genplot_ut_mrtt2_aqm_40() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
'

gpi=$gpi'set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.1,0.4 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
	gpi=$gpi"set output \"${targetfolder}/ut_mrtt2_link40.eps\""$'\n'
	gpi=$gpi'set ylabel "Link utilisation [%]"
	set yrange [60:100]
	set xrange [-1:38]
'
declare -i acq
declare -i aqm_start=1
declare -i 	aqm_step=3

	for (( aqc=0; aqc<3; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start}.9,3.3 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done
	used_titles=("" "" "" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	link=40
	title_tb=""
	for aqm in "${aqm_array[@]}"; do 
		aqmname="mr2_${aqm}"
		get_cc_captions_colors
		title_tb="${cc_a}(A)+${cc_b}(B) P_{1}, mean, P_{99}"
		if [ $(contains "${used_titles[@]}" "${title_tb}") == "y" ]; then
			title_tb=""
		else
			used_titles[used_titles_index]=$title_tb
			used_titles_index=$used_titles_index+1
		fi
		filename_tb=${targetfolder}"/data/"${aqmname}"_"${link}"_ut_mrtt2"
		gpi=$gpi"'${filename_tb}' using (\$0+${gap}.25):3:7:4:3:xtic(1) with candlesticks ls 1 lw 35  lc rgb '${color_a}' title \"${title_tb}\", " 

		
		gap=$gap+13
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ut_mrtt2_link40_${aqmfn}.gpi
	gnuplot ${targetfolder}/ut_mrtt2_link40_${aqmfn}.gpi
}

function genplot_tb_mrtt2_aqm_40() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
'

gpi=$gpi'set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.1,0.4 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
	gpi=$gpi"set output \"${targetfolder}/tb_mrtt2_link40.eps\""$'\n'
	gpi=$gpi'set ylabel "Rate ratio"
	set yrange [0.01:100]
set logscale y
	set xrange [-1:38]
'
declare -i acq
declare -i aqm_start=1
declare -i 	aqm_step=3

	for (( aqc=0; aqc<3; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start}.9,3.3 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done
	used_titles=("" "" "" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean="${cc_a} mean"
	title_a_p99="P_{99}"
	title_b_mean="${cc_b} mean"
	title_b_p99="P_{99}"

	link=40
	title_tb="${cc_a}+${cc_b}"
	for aqm in "${aqm_array[@]}"; do 
		aqmname="mr2_${aqm}"
		get_tb_color
	
		title_tb="${cc_a}(A)/${cc_b}(B) ratio"
		if [ $(contains "${used_titles[@]}" "${title_tb}") == "y" ]; then
			title_tb=""
		else
			used_titles[used_titles_index]=$title_tb
			used_titles_index=$used_titles_index+1
		fi
		filename_tb=${targetfolder}"/data/"${aqmname}"_"${link}"_tb_mrtt2"
		gpi=$gpi"'${filename_tb}' using (\$0+$gap):2:xtic(1) with points ls 3 lw 80 lc rgb '${color_tb}' title \"${title_tb}\", " 
		title_tb=""
		gap=$gap+13
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/tb_mrtt2_link40_${aqmfn}.gpi
	gnuplot ${targetfolder}/tb_mrtt2_link40_${aqmfn}.gpi
}

function genplot_wb_mrtt2_aqm_40() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 
'

gpi=$gpi'set label 2 "RTT (flow A - flow B)[ms]:" at screen 0.1,0.4 front font "Times-Roman,120" tc rgb "black" left
set xtic rotate by -65
set style fill solid 1.0 border 
set boxwidth 0.4
'
	gpi=$gpi"set output \"${targetfolder}/wb_mrtt2_link40.eps\""$'\n'
	gpi=$gpi'set ylabel "Window balance [ratio]"
	set yrange [0.01:20]
set logscale y
	set xrange [-1:38]
'
declare -i acq
declare -i aqm_start=1
declare -i 	aqm_step=3

	for (( aqc=0; aqc<3; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start}.9,3.3 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done
	used_titles=("" "" "" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean="${cc_a} mean"
	title_a_p99="P_{99}"
	title_b_mean="${cc_b} mean"
	title_b_p99="P_{99}"

	link=40
	title_tb=""
	for aqm in "${aqm_array[@]}"; do 
		aqmname="mr2_${aqm}"
		get_cc_captions_colors
	
		title_tb="${cc_a}(A)/${cc_b}(B) ratio"
		if [ $(contains "${used_titles[@]}" "${title_tb}") == "y" ]; then
			title_tb=""
		else
			used_titles[used_titles_index]=$title_tb
			used_titles_index=$used_titles_index+1
		fi
		filename_tb=${targetfolder}"/data/"${aqmname}"_"${link}"_wb_mrtt2"
		gpi=$gpi"'${filename_tb}' using (\$0+$gap):2:xtic(1) with points ls 3 lw 80 lc rgb '${color_a}' title \"${title_tb}\", " 
		title_tb=""
		gap=$gap+13
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/wb_mrtt2_link40_${aqmfn}.gpi
	gnuplot ${targetfolder}/wb_mrtt2_link40_${aqmfn}.gpi
}

function genplot_ls_aqm() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/ls_${testcase_id}_aqm.eps\""$'\n'
	gpi=$gpi'set ylabel "Drop/Mark probability [%] logscale"
set yrange [0.001:100]
set logscale y
set key spacing 0.9
set label 2 "RTT[ms]:" at screen 0.3,0.6 front font "Times-Roman,120" tc rgb "black" left
'


	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0
	
	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean=""
	title_b_mean=""
	title_marks_a=""
	title_marks_b=""
	for aqm in "${aqm_array[@]}"; do 
		for link in "${link_array[@]}"; do
			aqmname="m_${aqm}"
			get_cc_captions_colors
			title_a_mean="Drops L4S P{25}, mean, P{99}"
			if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
    			title_a_mean=""
			else
				used_titles[used_titles_index]=$title_a_mean
				used_titles_index=$used_titles_index+1
			fi
			title_b_mean="Drops Classic P{25}, mean, P{99}"
			if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
    			title_b_mean=""
			else
				used_titles[used_titles_index]=$title_b_mean
				used_titles_index=$used_titles_index+1
			fi
			title_marks_a="Marks ${cc_a} P{25}, mean, P{99}"
			if [ $(contains "${used_titles[@]}" "${title_marks_a}") == "y" ]; then
    			title_marks_a=""
			else
				used_titles[used_titles_index]=$title_marks_a
				used_titles_index=$used_titles_index+1
			fi

			title_marks_b="Marks ${cc_b} P{25}, mean, P{99}"
			if [ $(contains "${used_titles[@]}" "${title_marks_b}") == "y" ]; then
    			title_marks_b=""
			else
				used_titles[used_titles_index]=$title_marks_b
				used_titles_index=$used_titles_index+1
			fi
			
			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_a"_${testcase_id}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_b"_${testcase_id}
			filename_marks_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_marks_a"_${testcase_id}
			filename_marks_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_marks_b"_${testcase_id}

			gpi=$gpi"'${filename_b}' using (\$0+${gap}):3:7:5:3:xtic(1) with candlesticks ls 1 lw 30 lc rgb 'red' title \"${title_b_mean}\", "
			smallgap="$gap".3

			gpi=$gpi"'${filename_a}' using (\$0+${smallgap}):3:7:5:3 with candlesticks ls 1 lw 30 lc rgb 'purple' title \"${title_a_mean}\", "
			smallgap="$gap".6

			gpi=$gpi"'${filename_marks_a}' using (\$0+${smallgap}):3:7:5:3 with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_marks_a}\", "
			smallgap="$gap".9

			gpi=$gpi"'${filename_marks_b}' using (\$0+${smallgap}):3:7:5:3 with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_marks_b}\", "
			gap=$gap+1+$rtt_count
			title_a_mean=""
			title_b_mean=""
			title_marks_a=""
			title_marks_b=""
		done
		gap=$gap+1
	done
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ls_${testcase_id}_aqm.gpi
	gnuplot ${targetfolder}/ls_${testcase_id}_aqm.gpi
}

function genplot_ut_aqm() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/ut_${testcase_id}_aqm.eps\""$'\n'
	gpi=$gpi'set ylabel "Link utilisation [%]"
set yrange [50:100]
set xrange [-1:105]
'

if [ "$testcase_id" == "1-1" ]; then
	gpi=$gpi'set yrange [50:100]
	'
fi

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 count=0 index=0
	gpi=$gpi"plot "
	title_tb=""
	for aqm in "${aqm_array[@]}"; do 
		aqmname="m_${aqm}"
		get_cc_captions_colors
		get_util_color
		title_ut="${cc_a}+${cc_b} P{1}, mean, P{99}"
		if [ $(contains "${used_titles[@]}" "${title_ut}") == "y" ]; then
			title_ut=""
		else
			used_titles[used_titles_index]=$title_ut
			used_titles_index=$used_titles_index+1
		fi
		
		for link in "${link_array[@]}"; do
			filename_ut=${targetfolder}"/data/"${aqmname}"_"${link}"_ut"_${testcase_id}

				gpi=$gpi"'${filename_ut}' using (\$0+${gap}):3:7:5:3:xtic(1) with candlesticks ls 1 lw 35  lc rgb '${color_util}' title \"${title_ut}\", " 
			
			gap=$gap+7
			title_ut=""
		done
		index=$index+1

		gap=$gap+1
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ut_${testcase_id}_aqm.gpi
	gnuplot ${targetfolder}/ut_${testcase_id}_aqm.gpi
}

function genplot_ct_link() {
	do_header
	gpi=$gpi"set output \"${targetfolder}/ct_${testcase_id}_link${link}.eps\""$'\n'
	gpi=$gpi'set ylabel "Completion efficiency [log(sec/sec)]"
set xrange [-1:131]
set logscale y
set label 2 "Bin start[KB]:" at screen 0.33,0.9 front font "Times-Roman,80" tc rgb "black" left
set xlabel "RTT[ms]:   5             10             20             50            100                  5             10             20             50             100                  5             10             20             50             100                  "
set arrow 1 from -1,1 to 131,1 nohead lw 25 lc rgb "green"
set xtics font ",80"
set xtic rotate by -90
set yrange [0.01:2]
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0 gap=0 
	gpi=$gpi"plot "
	title_a=""
	title_b=""
	for aqm in "${aqm_array[@]}"; do 
		aqmname="m_${aqm}"
		get_cc_captions_colors
		title_a="${cc_a}"
		title_b="${cc_b}"
		if [ $(contains "${used_titles[@]}" "${title_a}") == "y" ]; then
    		title_a=""
		else
			used_titles[used_titles_index]=$title_a
			used_titles_index=$used_titles_index+1
		fi
		if [ $(contains "${used_titles[@]}" "${title_b}") == "y" ]; then
    		title_b=""
		else
			used_titles[used_titles_index]=$title_b
			used_titles_index=$used_titles_index+1
		fi
		for rtt in "${rtt_array[@]}"; do
			filename_a=${targetfolder}/data/${aqmname}_${link}_ct_a_${rtt}_${testcase_id}
			filename_b=${targetfolder}/data/${aqmname}_${link}_ct_b_${rtt}_${testcase_id}
			gpi=$gpi"'${filename_a}' using (\$0+${gap}):2:4:3:2:xtic(1) with candlesticks ls 1 lw 35 lc rgb '${color_a}' title \"${title_a}\", " 
			smallgap="$gap".4

			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):2:4:3:2 with candlesticks ls 1 lw 35  lc rgb '${color_b}' title \"${title_b}\", " 
			
			gap=$gap+8
    		title_a=""
    		title_b=""
		done
		gap=$gap+6
	done
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/all_ct_${testcase_id}_link${link}.gpi
	gnuplot ${targetfolder}/all_ct_${testcase_id}_link${link}.gpi
}

function do_header_overload() {
	gpi='reset
set terminal postscript eps enhanced color "Times-Roman" 120
set notitle
set key above
set key box linestyle 99
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size 12,4
set size ratio 0.15
set boxwidth 0.2 absolute 

set xtic rotate by -65
set xtics font "Times-Roman" 120
set style fill solid 1.0 border 
set boxwidth 0.4
set xrange [-1:37]
set xlabel "UDP class:                   Classic                                ECN                                   Classic                               ECN                                       Classic                              ECN               " 
set label 2 "UDP rate:" at screen 0.2,0.85 front font "Times-Roman,120" tc rgb "black" left
set label 3 "[Mbps]:" at screen 0.35,0.65 front font "Times-Roman,120" tc rgb "black" left
'


	aqm_step=3
	aqm_start=2
	declare -i acq
	for (( aqc=0; aqc<3; aqc++ ))
	do
		if [ "$aqc" == "2" ]; then
			aqm_start=$aqm_start+1
		fi
		gpi=$gpi"set label 3$aqc \"${aqm_labels[aqc]}\" at screen ${aqm_start},3.4 font \"Times-Roman,140\" tc rgb \"black\" left"$'\n'
		aqm_start="$(($aqm_start + $aqm_step))"
	done


}



function genplot_qd_o() {
	do_header_overload

	gpi=$gpi"set output \"${targetfolder}/qd_o.eps\""$'\n'
	gpi=$gpi'set ylabel "Queue delay log[ms]"
set logscale y
set boxwidth 0.2
'
	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean="${cc_a} mean"
	title_b_mean="${cc_b} mean"
	title_udp="UDP mean"
	link=100

	for aqm in "${aqm_array[@]}"; do 
		for t in "ur" "ud"; do
			aqmname="o_${aqm}"
			get_cc_captions_colors
			title_a_mean="${cc_a} mean, P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
    			title_a_mean=""
			else
				used_titles[used_titles_index]=$title_a_mean
				used_titles_index=$used_titles_index+1
			fi
			title_b_mean="${cc_b} mean, P_{99}"
			title_b_p99="P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
    			title_b_mean=""
			else
				used_titles[used_titles_index]=$title_b_mean
				used_titles_index=$used_titles_index+1
			fi
			title_udp="UDP mean, P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_udp}") == "y" ]; then
    			title_udp=""
			else
				used_titles[used_titles_index]=$title_udp
				used_titles_index=$used_titles_index+1
			fi

			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_a"_${t}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_b"_${t}
			filename_udp=${targetfolder}"/data/"${aqmname}"_"${link}"_qd_udp"_${t}

			gpi=$gpi"'${filename_a}' using (\$0+${gap}):3:(\$7-\$3):xtic(1) with boxerrorbars ls 1 lw 10 lc rgb '${color_a}' title \"${title_a_mean}\", "
			smallgap="$gap".3
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):3:(\$7-\$3) with boxerrorbars ls 1 lw 10 lc rgb '${color_b}' title \"${title_b_mean}\", " 
			smallgap="$gap".6
			gpi=$gpi"'${filename_udp}' using (\$0+${smallgap}):3:(\$7-\$3) with boxerrorbars ls 1 lw 10 lc rgb 'brown' title \"${title_udp}\"," 


			gap=$gap+6
			title_a_mean=""
			title_a_p99=""
			title_b_mean=""
			title_b_p99=""
		done
		gap=$gap+1
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/qd_o.gpi
	gnuplot ${targetfolder}/qd_o.gpi
}

function genplot_ws_o() {
	do_header_overload

	gpi=$gpi"set output \"${targetfolder}/ws_o.eps\""$'\n'
	gpi=$gpi'set ylabel "Window size per flow"
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean="${cc_a} mean, P_{99}"
	title_b_mean="${cc_b} mean, P_{99}"
	link=100
	for aqm in "${aqm_array[@]}"; do 
		for t in "ur" "ud"; do
			aqmname="o_${aqm}"
			get_cc_captions_colors
			title_a_mean="${cc_a} mean, P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
    			title_a_mean=""
			else
				used_titles[used_titles_index]=$title_a_mean
				used_titles_index=$used_titles_index+1
			fi
			title_b_mean="${cc_b} mean, P_{99}"
			title_b_p99="P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
    			title_b_mean=""
			else
				used_titles[used_titles_index]=$title_b_mean
				used_titles_index=$used_titles_index+1
			fi
			
			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_a"_${t}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ws_b"_${t}
			#echo "color_a: "$color_a
			gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9):xtic(1) with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_a_mean}\", "

			smallgap="$gap".4
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9) with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_b_mean}\", "
		
			gap=$gap+6
			title_a_mean=""
			title_b_mean=""
		

		done
		gap=$gap+1
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ws_o.gpi
	gnuplot ${targetfolder}/ws_o.gpi
}


function genplot_ts_o() {
	do_header_overload

	gpi=$gpi"set output \"${targetfolder}/ts_o.eps\""$'\n'
	gpi=$gpi'set ylabel "Normalised rate per flow"
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean="${cc_a} mean, P_{99}"
	title_b_mean="${cc_b} mean, P_{99}"
	link=100
	for aqm in "${aqm_array[@]}"; do 
		for t in "ur" "ud"; do
			aqmname="o_${aqm}"
			get_cc_captions_colors
			title_a_mean="${cc_a} mean, P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
    			title_a_mean=""
			else
				used_titles[used_titles_index]=$title_a_mean
				used_titles_index=$used_titles_index+1
			fi
			title_b_mean="${cc_b} mean, P_{99}"
			title_b_p99="P_{99}"
			if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
    			title_b_mean=""
			else
				used_titles[used_titles_index]=$title_b_mean
				used_titles_index=$used_titles_index+1
			fi
			
			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_a"_${t}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ts_b"_${t}
			gpi=$gpi"'${filename_a}' using (\$0+${gap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9):xtic(1) with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_a_mean}\", "

			smallgap="$gap".4
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9) with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_b_mean}\", "
		
			gap=$gap+6
			title_a_mean=""
			title_b_mean=""
		

		done
		gap=$gap+1
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ws_o.gpi
	gnuplot ${targetfolder}/ws_o.gpi
}

function genplot_ls_o() {
	do_header_overload

	gpi=$gpi"set output \"${targetfolder}/ls_o.eps\""$'\n'
	gpi=$gpi'set ylabel "Drop/Mark prob. [log(%)]"
	set boxwidth 0.4

set logscale y
set label 5 "mean, P_{25}, P_{99}:" at screen 0.05,3.65 front font "Times-Roman,120" tc rgb "black" left


'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0
	declare -i gap=0
	gpi=$gpi"plot "
	title_a_mean="Drops L4S"
	title_b_mean="Drops Classic"
	title_marks_a="Marks ${cc_a}"
	title_marks_b="Marks ${cc_b}"
	title_drops_udp="Drops UDP"
	link=100
	for aqm in "${aqm_array[@]}"; do 
		for t in "ur" "ud"; do
			aqmname="o_${aqm}"
			get_cc_captions_colors
			title_a_mean="Drops L4S"
			if [ $(contains "${used_titles[@]}" "${title_a_mean}") == "y" ]; then
    			title_a_mean=""
			else
				used_titles[used_titles_index]=$title_a_mean
				used_titles_index=$used_titles_index+1
			fi
			title_b_mean="Drops Classic"
			if [ $(contains "${used_titles[@]}" "${title_b_mean}") == "y" ]; then
    			title_b_mean=""
			else
				used_titles[used_titles_index]=$title_b_mean
				used_titles_index=$used_titles_index+1
			fi
			title_marks_a="Marks ${cc_a}"
			if [ $(contains "${used_titles[@]}" "${title_marks_a}") == "y" ]; then
    			title_marks_a=""
			else
				used_titles[used_titles_index]=$title_marks_a
				used_titles_index=$used_titles_index+1
			fi
			
			title_marks_b="Marks ${cc_b}"
			if [ $(contains "${used_titles[@]}" "${title_marks_b}") == "y" ]; then
    			title_marks_b=""
			else
				used_titles[used_titles_index]=$title_marks_b
				used_titles_index=$used_titles_index+1
			fi

			filename_a=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_a"_${t}
			filename_b=${targetfolder}"/data/"${aqmname}"_"${link}"_ls_b"_${t}
			filename_marks_a=${targetfolder}"/data/"${aqmname}"_"${link}"_marks_a"_${t}
			filename_marks_b=${targetfolder}"/data/"${aqmname}"_"${link}"_marks_b"_${t}
			gpi=$gpi"'${filename_b}' using (\$0+${gap}):3:7:5:3 with candlesticks ls 1 lw 30 lc rgb 'red' title \"${title_b_mean}\", "
			smallgap="$gap".3

			gpi=$gpi"'${filename_a}' using (\$0+${smallgap}):3:7:5:3:xtic(1) with candlesticks ls 1 lw 30 lc rgb 'purple' title \"${title_a_mean}\", "
			smallgap="$gap".6

			gpi=$gpi"'${filename_marks_a}' using (\$0+${smallgap}):3:7:5:3 with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_marks_a}\", "
			smallgap="$gap".9
			gpi=$gpi"'${filename_marks_b}' using (\$0+${smallgap}):3:7:5:3 with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_marks_b}\", "

			gap=$gap+6
			title_a_mean=""
			title_b_mean=""
			title_marks_a=""
			title_marks_b=""
		done
		gap=$gap+1
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/ls_o.gpi
	gnuplot ${targetfolder}/ls_o.gpi
}

function genplot_wb_o() {
	do_header_overload

	gpi=$gpi"set output \"${targetfolder}/wb_o.eps\""$'\n'
	gpi=$gpi'set ylabel "Window balance [ratio]"
set logscale y
'

	used_titles=("" "" "" "" "")
	declare -i used_titles_index=0

	declare -i gap=0
	gpi=$gpi"plot "
	title_wb="${cc_a}/${cc_b}"
	link=100
	for aqm in "${aqm_array[@]}"; do 
		for t in "ur" "ud"; do
			aqmname="o_${aqm}"
			get_cc_captions_colors
			title_wb="${cc_a}/${cc_b}"
			if [ $(contains "${used_titles[@]}" "${title_wb}") == "y" ]; then
    			title_wb=""
			else
				used_titles[used_titles_index]=$title_wb
				used_titles_index=$used_titles_index+1
			fi
			
			filename=${targetfolder}"/data/"${aqmname}"_"${link}"_wb"_${t}

			gpi=$gpi"'${filename}' using (\$0+${gap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9):xtic(1) with candlesticks ls 1 lw 30 lc rgb '${color_a}' title \"${title_a_mean}\", "

			smallgap="$gap".4
			gpi=$gpi"'${filename_b}' using (\$0+${smallgap}):(\$3/\$9):(\$7/\$9):(\$4/\$9):(\$3/\$9) with candlesticks ls 1 lw 30 lc rgb '${color_b}' title \"${title_b_mean}\", "
		
			gap=$gap+6
			title_a_mean=""
			title_b_mean=""
		done
		gap=$gap+1
	done
	
	gpi=$gpi$'\n'
	echo "$gpi" > ${targetfolder}/wb_o.gpi
	gnuplot ${targetfolder}/wb_o.gpi
}

function genplot_qd_ccdf()
{
	gpi='reset
set terminal postscript eps size 32,20 enhanced color "Times-Roman" 75
set size 1,1'
	gpi=$gpi$'\n'
	gpi=$gpi"set title \"${aqm_labels[0]}\" font \"Times-Roman-Bold, 65\""$'\n'
	gpi=$gpi"set output \"${targetfolder}/ccdf.eps\""$'\n'
	gpi=$gpi'set multiplot layout 2,3 scale 1,1
set key above
set key spacing 1.2
set grid xtics ytics ztics lw 0.2 lc rgb "gray"
set size ratio 1
set yrange [0.001:100]
set xrange [0:80] 
set ytics ("50" 50, "90" 10, "99" 1, "99.9" 0.1, "99.99" 0.01, "99.999" 0.001)
set xtics 10
set xtics add ("" 1, "" 2, "" 3, "" 4, "5" 5, "" 6, "" 7, "" 8, "" 9)
set logscale y
'
	gpi=$gpi$'\n'

	declare -i index=0
	for test in "d_d10s1_r_d10s1" "d_d100s1_r_d100s1"; do
		for aqm in "${aqm_array[@]}"; do
			aqmname="m_${aqm}"
			get_cc_captions_colors
			folder=${mainfolder}/m_${aqm}_*/mix_1000/${test}
			sudo ./calc_qpd $folder 0
			filename_a=$(find ${folder}/queue_delay_a_ccdf -print)
			filename_b=$(find ${folder}/queue_delay_b_ccdf -print)

			gpi=$gpi"set title \"${aqm_labels[${index}]}\" font \"Times-Roman-Bold, 75\""$'\n'
			subtitle=""
			key_title=""
			if [ "$test" == "d_d10s1_r_d10s1" ]; then
				subtitle="High load (1h:1h): "
				key_title_a="notitle"
				key_title_b="notitle"
			else
				subtitle="Low load (1l:1l): "
				key_title_a="title \"${cc_a}\""
				key_title_b="title \"${cc_b}\""
			fi
			if [ "$index" == "0" ]; then
				gpi=$gpi"set ylabel \"${subtitle}Percentile [%]\""$'\n'
				gpi=$gpi"set xlabel \"   \""$'\n'
			elif [ "$index" == "1" ]; then
				gpi=$gpi'set ylabel "   "
set xlabel "Queue delay [ms]"
'
				gpi=$gpi$'\n'
			else
				gpi=$gpi"set xlabel \"   \""$'\n'
			fi

			gpi=$gpi"plot '${filename_a}' using (\$1/1000):2 with lines ls 1 lw 25  lc rgb '${color_a}' ${key_title_a}, '${filename_b}' using (\$1/1000):2 with lines ls 1 lw 25  lc rgb '${color_b}' ${key_title_b}"$'\n'
			index=$index+1
		done
		index=0
	done

	echo "$gpi" > ${targetfolder}/ccdf.gpi
	gnuplot ${targetfolder}/ccdf.gpi
}

