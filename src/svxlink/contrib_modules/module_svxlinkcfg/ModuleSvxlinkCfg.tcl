###############################################################################
#	SVXlink Config file
#	Coded by Dan Loranger (KG7PAR)
#  
#	This module enables the user to configure most of the settings in the
#   configuration file without needing to log into the system on a computer.
#   By necessity, some settings are not capable (at this time) of being handled
#   with this module.  Some of these include complex file names, locations, etc
#   that are not easily manipulated with basic DTMF options, like simple menus
#   or numeric values
#
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleTcl] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval SvxlinkCfg {
#define where the config file lives
#source /usr/share/svxlink/modules.d/CommonCommands.tcl

#
# Check if this module is loaded in the current logic core
#
if {![info exists CFG_ID]} {
  return;
}

#
# Extract the module name from the current namespace
#
set module_name [namespace tail [namespace current]]


# A convenience function for printing out information prefixed by the
# module name
#
#   msg - The message to print
#
proc printInfo {msg} {
  variable module_name
  puts "$module_name: $msg"
}

#
# A convenience function for calling an event handler
#
#   ev - The event string to execute
#
proc processEvent {ev} {
  variable module_name
  ::processEvent "$module_name" "$ev"
}

#
# Executed when this module is being activated
#
proc activateInit {} {
  #define the array of commands and codes desired to activate them
  set InitialCode 500
  global ConfigVariablesList

  list ConfigVariablesList {}
   # variableName DTMF Code Enable HelpfileName audioDirectory
   
   # Enabled options 1-enabled, 0-disabled, x-support not planned
   # All config variables should be represented, and will be status printed to the logs
   # so users can see what can be configured and what cant
  lappend ConfigVariablesList 			Restart 				999 		 1 restart_help 						svxlinkCfg
  lappend ConfigVariablesList 			CardSampleRate 			$InitialCode 1 card_sample_rate_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			LocInfo 				$InitialCode 1 loc_info_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Links 					$InitialCode X links_help 							svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Type 					$InitialCode X type_help 							svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Rx 						$InitialCode X rx_help 								svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Tx 						$InitialCode X tx_help 								svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Modules 				$InitialCode 0 modules_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			CallSign 				$InitialCode 0 callsign_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			ShortIdInt 				$InitialCode 1 short_id_int_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			LongIdInt 				$InitialCode 1 long_id_int_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			IdAfterTx 				$InitialCode 0 id_after_tx_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			ExecuteCmdOnSqlClose 	$InitialCode 0 execute_cmd_on_sql_close_help 		svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			EventHandler 			$InitialCode 0 event_handler_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DefaultLanguage 		$InitialCode X default_language_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			RogerSoundDelay 		$InitialCode 1 roger_sound_delay_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			ReportCtcss 			$InitialCode 0 report_ctcss_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			TxCtcss 				$InitialCode 1 tx_ctcss_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Macros 					$InitialCode 0 macros_help 							svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			FxGainNormal 			$InitialCode 0 fx_gain_normal_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			FxGainLow 				$InitialCode 0 fx_gain_low_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			QsoRecorder 			$InitialCode 0 qso_recorder_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Sel5MacroRange 			$InitialCode 0 sel5_macro_range_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OnlineCmd 				$InitialCode 1 online_cmd_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			MuteRxOnTx 				$InitialCode 0 mute_rx_on_tx_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			NoRepeat 				$InitialCode 0 no_repeat_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			IdleTimeout 			$InitialCode 1 idle_timeout_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpenOn1750 				$InitialCode 0 open_on_1750_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpenOnCtcss 			$InitialCode 0 open_on_ctcss_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpenOnDtmf 				$InitialCode 0 open_on_dtmf_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpenOnSel5 				$InitialCode 0 open_on_sel5_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpenOnSql 				$InitialCode 0 open_on_sql_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpenOnSqlAfterRptClose 	$InitialCode 0 open_on_sql_after_rpt_close_help 	svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpenSqlFlank 			$InitialCode 1 open_sql_flank_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			IdleSoundInterval 		$InitialCode 0 idle_sound_interval_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlFlapSupMinTime 		$InitialCode 0 sql_flap_sup_min_time_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlFlapSupMaxCount 		$InitialCode 0 sql_flap_sup_max_count_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			ActivateModuleOnLongCmd $InitialCode 0 activate_module_on_long_cmd_help 	svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			IdentNagTimeout 		$InitialCode 0 ident_nag_timeout_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			IdentNagMinTime 		$InitialCode 0 ident_nag_min_time_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			RecDir 					$InitialCode 0 rec_dir_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			MinTime 				$InitialCode 0 min_time_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			MaxTime 				$InitialCode 0 max_time_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SoftTime 				$InitialCode 0 soft_time_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			MaxDirSize 				$InitialCode 0 max_dir_size_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DefaultActive 			$InitialCode 0 default_active_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Timeout 				$InitialCode 1 timeout_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			QsoTimeout 				$InitialCode 0 qso_timeout_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			EncodeCmd 				$InitialCode 0 encode_cmd_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			ConnectLogics 			$InitialCode 0 connect_logics_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			AutoActivateOnSql 		$InitialCode 0 auto_activate_on_sql_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			AudioDev 	 			$InitialCode 0 audio_dev_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			AudioChannel 			$InitialCode 0 audio_channel_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlDet 					$InitialCode 1 sql_detect_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlStartDelay 			$InitialCode 1 sql_start_delay_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlDelay 				$InitialCode 0 sql_delay_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlHangtime 			$InitialCode 1 sql_hangtime_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlExtendedHangtime 	$InitialCode 1 sql_extended_hangtime_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 		  SqlExtendedHangtimeThresh $InitialCode 0 sql_extended_hangtime_thresh_help 	svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlTimeout 				$InitialCode 0 sql_timeout_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			VoxFilterDepth 			$InitialCode 0 vox_filter_depth_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			VoxThresh 				$InitialCode 0 vox_thresh_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			CtcssMode 				$InitialCode 0 ctcss_mode_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			CtcssFq 				$InitialCode 0 ctcss_fq_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			CtcssOpenThresh 		$InitialCode 0 ctcss_open_Thresh_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			CtcssCloseThresh 		$InitialCode 0 ctcss_close_thresh_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			CtcssSnrOffset 			$InitialCode 0 ctcss_snr_offset_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			CtcssBpfLow 			$InitialCode 0 ctcss_bpf_low_help 					svxlinkCfg  
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			CtcssBpfHigh 			$InitialCode 0 ctcss_bpf_high_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SerialPort 				$InitialCode 0 serial_port_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SerialPin 				$InitialCode 0 serial_pin_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SerialSetPins 			$InitialCode 0 serial_set_pins_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			EvdevDevname 			$InitialCode 0 evdev_dev_name_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			EvdevOpen 				$InitialCode 0 evdev_open_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			EvdevClose 				$InitialCode 0 evdev_close 							svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			GpioSqlPin 				$InitialCode 0 gpio_sql_pin 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SiglevDet 				$InitialCode 0 siglev_det_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SiglevSlope 			$InitialCode 0 siglev_slope_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SiglevOffset 			$InitialCode 0 siglev_offset_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			ToneSiglevMap 			$InitialCode 0 tone_siglev_map_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SiglevOpenThresh 		$InitialCode 0 siglev_open_thresh_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SiglevCloseThresh 		$InitialCode 0 siglev_close_thresh_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Deemphasis 				$InitialCode 0 deemphesis_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlTailElim 			$InitialCode 0 sql_tail_elim_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Preamp 					$InitialCode 0 preamp_help 							svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			PeakMeter 				$InitialCode 0 peak_meter_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DtmfDecType 			$InitialCode 0 dtmf_dec_type_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DtmfMuting 				$InitialCode 0 dtmf_muting_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DtmfHangtime 			$InitialCode 0 dtmf_hangtime_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DtmfSerial 				$InitialCode 0 dtmf_serial_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DtmfMaxFwdTwist 		$InitialCode 0 dtmf_max_fwd_twist_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Tone1750Muting 			$InitialCode 0 tone_1750_muting_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Sel5Type 				$InitialCode 0 sel5_type_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Sel5DecType 			$InitialCode 0 sel5_dec_type_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Receivers 				$InitialCode 0 receivers_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			VotingDelay 			$InitialCode 0 voting_delay_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			BufferLength 			$InitialCode 0 buffer_length_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			RevoteInterval 			$InitialCode 0 revote_interval_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Hysteresis				$InitialCode 0 hysteresis_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			RxSwitchDelay 			$InitialCode 0 rx_switch_delay_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SqlCloseRevoteDelay 	$InitialCode 0 sql_close_revote_delay_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Host 					$InitialCode 0 host_help 							svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			AuthKey 				$InitialCode 0 auth_key_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Codec 					$InitialCode 0 codec_help 							svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SpeexEncFramesPerPacket $InitialCode 0 speex_enc_frames_per_packet_help 	svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SpeexEncQuality 		$InitialCode 0 speex_enc_quality_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SpeexEncBitrate 		$InitialCode 0 speex_enc_bitrate_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SpeexEncComplexity 		$InitialCode 0 speex_enc_complexity_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SpeexEncVbr 			$InitialCode 0 speex_enc_vbr_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SpeexEncVbrQuality 		$InitialCode 0 speex_enc_vbr_quality_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SpeexEncAbr 			$InitialCode 0 speex_enc_abr_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			SpeexDecEnhancer 		$InitialCode 0 speex_dec_enhancer_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpusEncComplexity 		$InitialCode 0 opus_enc_complexity_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpusEncBitrate 			$InitialCode 0 opus_enc_bitrate_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			OpusEncVbr 				$InitialCode 0 opus_enc_vbr_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			PttPort 				$InitialCode 0 ptt_port_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			PttPin 					$InitialCode 0 ptt_pin_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			PttHangtime 			$InitialCode 0 ptt_hangtime_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			TxDelay 				$InitialCode 0 tx_delay_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			CtcssLevel 				$InitialCode 0 ctcss_level_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Preemphesis 			$InitialCode 0 preemphesis_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DtmfToneLength 			$InitialCode 0 dtmf_tone_length 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DtmfToneSpacing 		$InitialCode 0 dtmf_tone_spacing_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			DtmfToneAmp 			$InitialCode 0 dtmf_tone_amp_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			ToneSiglevLevel 		$InitialCode 0 tone_siglev_level_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Transmitters 			$InitialCode 0 transmitters_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			StatusServerList 		$InitialCode 0 status_server_list_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			AprsServerList 			$InitialCode 0 aprs_server_list_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			LonPosition 			$InitialCode 0 lon_position_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			LatPosition 			$InitialCode 0 lat_position_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Frequency 				$InitialCode 0 frequency_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			TxPower 				$InitialCode 0 tx_power_help 						svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			AntennaGain 			$InitialCode 0 antenna_gain_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			AntennaHeight 			$InitialCode 0 antenna_height_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			AntennaDir 				$InitialCode 0 antenna_dir_help 					svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Path 					$InitialCode 0 path_help 							svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			BeaconInterval 			$InitialCode 0 beacon_interval_help 				svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Tone 					$InitialCode 0 tone_help 							svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			StatisticsInterval 		$InitialCode 0 statistics_interval_help 			svxlinkCfg
  set InitialCode [expr $InitialCode+1]
  lappend ConfigVariablesList 			Comment 				$InitialCode 0 comment_help 						svxlinkCfg
    
  global svxlinkCfgMenuLayer 
  set svxlinkCfgMenuLayer 0
  printInfo "Module activated"
  # Access Variables
  variable CFG_ACCESS_PIN
  variable CFG_ACCESS_ATTEMPTS_ALLOWED
  variable ACCESS_PIN_REQ
  variable ACCESS_GRANTED
  variable ACCESS_ATTEMPTS_ATTEMPTED
  if {[info exists CFG_ACCESS_PIN]} { 
    set ACCESS_PIN_REQ 1
	if {![info exists CFG_ACCESS_ATTEMPTS_ALLOWED]} { set CFG_ACCESS_ATTEMPTS_ALLOWED 3 }
  } else {
	set ACCESS_PIN_REQ 0
  }
  set ACCESS_GRANTED 0
  set ACCESS_ATTEMPTS_ATTEMPTED 0 

  printInfo "Module Activated"
  if {$ACCESS_PIN_REQ == "1"} {
	printInfo "--- PLEASE ENTER YOUR PIN FOLLOWED BY THE POUND SIGN ---"
	playMsg "access_enter_pin";
  } else {
	# No Pin Required but this is the first time the module has been run so play prompt
	#playMsg "enter_command";
  }
  
    ## Itterate through the list, only play the ones that are available
    
	foreach {name code enable HelpFileName FolderName} $ConfigVariablesList {
	  
	  if {$enable == 1} {
	    ##  module is enabled, announce name and code assigned
	        playCodeName $code "$name"
			printInfo "$name:$code:Enabled"
	  } elseif {$enable == 0} {
	    printInfo "$name:$code:Disabled"
	  } else {
	    printInfo "$name:$code:NotPlanned"
	  }
	}
   
}

#
# A convenience function for setting a value in the config file
#
# ChannelNumber identifies the sequential position in the declaration
# note: currently only positions 1&2 are supported
#
# Value: the new value to be written to the config file
#
# configPath: The location of the svxlink.conf file, full path req'd
#
# configSetting: the setting you want to edit, case sensitive, include the "="
# example: "DTMF_MUTING="
#
# TODO: 
# --Enhance the function to be more generic for high channel numbers
# --Enhance the function to ensure it doesn't overflow into other channels
proc setConfigValue {ChannelNumber value configPath configSetting} {
	#Get the whole config file
	set configFile [exec cat $configPath]
	# Find the channel Name 
	set ChannelName [findChannelName $SvxlinkCfgChannelNumber $configPath]
	#printInfo "$SvxlinkCfgChannelName set successfully"
	# locate the channel configuration section
	set Section [string first "\[$SvxlinkCfgChannelName\]" $configFile]
	#printInfo "Section:$Section"
	set start [string first $configSetting $configFile $Section]
	printInfo "start:$start"
	set end [string first "\n" $configFile $start]
	printInfo "end:$end"
	# Replace the setting value, using start and end of line
	set configFile [string replace $configFile $start $end "$configSetting$value\n"]
	# write the settings back to disk
	set fp [open "/var/tmp/conf.conf" w]
	puts $fp "$configFile"
	close $fp
	#exec below gives a false error for unresolvable host, unclear why
	exec -ignorestderr sudo /bin/cp /var/tmp/conf.conf $configPath

}
#
# A convenience function for locating the arbitrary name assigned to
# a given channel in the svxlink.conf file.  Reads the "LOGICS=" field
# to determine the names.
#
# channel: index (1,2,3, exclude 0) of the channel to locate, only supports
# 1 & 2 currently.
#
# configPath: The location of the svxlink.conf file, full path req'd
#
proc findChannelName {channel configPath} {
	#Get the whole config file
	set Value [exec cat $configPath]
	# search for the "LOGICS=" declaration, this defines the namespace
	# of the channels used later on in the file, these values are changeable
	# so we have to search for them
	set Temp [expr [string first "LOGICS=" $Value]+7]
	# Find the first comma, this denotes the end of the first entry
	set Temp1 [string first "," $Value $Temp]
	# Capture the name of the first channel
	set Channel1Name [string range $Value $Temp [expr $Temp1-1]]
	
	#find the second comma if any
	if {$SvxlinkCfgChannel == 1} {
		printInfo "Channel1Name:$SvxlinkCfgChannel1Name"
		return $SvxlinkCfgChannel1Name
	} else {
		# find the next comma
		set Temp2 [string first "," $Value [expr $Temp1+1]]
		# find the next new line
		set Temp3 [string first "\n" $Value $Temp1]
		# Extract the second channel name
		if {$Temp2 <= $Temp3} {
			# Capture the name of the second channel by using the $Temp2 endpoint
			# this handles the case where 3+ channels are defined
			set Channel2Name [string range $Value [expr $Temp1+1] [expr $Temp2-1]]
			
		} else { 
			# Capture the name of the second channel by using the $Temp3 (\n) endpoint
			# this handles the case where only 2 channels are defined
			set Channel2Name [string range $Value [expr $Temp1+1] [expr $Temp3-1]]
		}
		printInfo "Channel2Name:$SvxlinkCfgChannel2Name"
		return $SvxlinkCfgChannel2Name
	}
}
#
# Executed when a DTMF command is received in idle mode. That is, a command is
# received when this module has not been activated first.
#
#   cmd - The received DTMF command
#
proc dtmfCmdReceivedWhenIdle {cmd} {
  printInfo "DTMF command received when idle: $cmd"
  
}

#
# Executed when this module is being deactivated.
#
proc deactivateCleanup {} {
  printInfo "Module deactivated"
 
}

#
# Executed when the squelch open or close.
#
#   is_open - Set to 1 if the squelch is open otherwise it's set to 0
#
proc squelchOpen {is_open} {
  if {$is_open} {set str "OPEN"} else { set str "CLOSED"}
  printInfo "The squelch is $str"
  
}

#
# Executed when a DTMF digit (0-9, A-F, *, #) is received
#
#   char - The received DTMF digit
#   duration - The duration of the received DTMF digit
# 
proc dtmfDigitReceived {char duration} {
  printInfo "DTMF digit $char received with duration $duration milliseconds"
  #playMsg "$char"
}

#
# Executed when all announcement messages has been played.
# Note that this function also may be called even if it wasn't this module
# that initiated the message playing.
#
proc allMsgsWritten {} {
  #printInfo "allMsgsWritten called..."
}


# A useful function for playing the DTMF code assigned to a module
proc playCodeName {DTMFcode subfunction} {
  set NameEnding Name
  set modName [concat "$subfunction$NameEnding"]
  playNumber $DTMFcode
  #playMsg "svxConfig" "$modName"
  playMsg "$modName"
}

#
# Executed when a DTMF command is received
#
#   cmd - The received DTMF command

proc dtmfCmdReceived {cmd} {
  global Channel
  global SvxlinkCfgFunctionName
  global SvxlinkCfgFunctionHelpFile
  global SvxlinkCfgFunctionHelpFileDir
  global configPath "/etc/svxlink/svxlink.conf"
  global svxlinkCfgMenuLayer
  global ConfigVariablesList
  printInfo "DTMF command received: $cmd"
  #printInfo "LIST:$ConfigVariablesList "
  ## MODULE GLOBAL COMMANDS 
  if {$cmd == "0"} {
  #play the help information, including module names
    playMsg "config_help"
  ## use 999 to reboot the service with updated configurations
  } elseif {$cmd == "999"} {
	#	restart the service
	exec -ignorestderr sudo /usr/sbin/service svxlink restart
  }
  
  if {$svxlinkCfgMenuLayer == 0} {
    set SvxlinkCfgFunctionName ""
	set SvxlinkCfgModuleFound 0
	#Get the module number from user
    foreach {name code enable helpFile helpFileDir} $ConfigVariablesList {
	  printInfo "CODE:$code NAME:$name ENABLED:$enable"
	  if {$cmd == $code} {
	  # cmd is recognized, make sure its enabled
	    if {$enable == "1"} {
	    # selection is enabled, advance the menu, request channel
		  set SvxlinkCfgFunctionName $name
		  set SvxlinkCfgFunctionHelpFile $helpFile
          set SvxlinkCfgFunctionHelpFileDir $helpFileDir
		  set svxlinkCfgMenuLayer 1
		  # set selection name
		  playMsg "enter_channel"
		  set SvxlinkCfgModuleFound 1
		  break
	    } else {
		  playMsg "invalid_selection"
		  break
		}
	  }
    }
	if {$SvxlinkCfgModuleFound == 0} {
	  playMsg "invalid_configuration_selection"
	  return
	}
  }
  
  
  ## we have received a command that should correlate with a Channel Number 
  if {$svxlinkCfgMenuLayer == 1} {
	set svxlinkCfgChannel $cmd
    set svxlinkCfgMenuLayer 2
	return
  }
  
  # we know the function,and channel, now to announce the help for the module
  if {$svxlinkCfgMenuLayer == 2} {
     # old syntax is 'playMsg "filename"'
	 playMsg "$SvxlinkCfgFunctionHelpFile"
	 # new syntax is 'playMsg "directory" "filename"' once it gets enabled
	 # playMsg "SvxlinkCfgFunctionHelpFileDir" "$SvxlinkCfgFunctionHelpFile"
	 set svxlinkCfgMenuLayer 3
  }
  
  # We have the command, channel and now expect the user to input the setting.
  #
  # intially the setting at each module will not be validated, there may be 
  # more work for this at some later point, but initially this would balloon 
  # the scope of the effort before the module has been reviewed for initial
  # acceptance.
  #
  # Also, be nice to those who follow, keep these entries alphabetized
  if {$svxlinkCfgMenuLayer == 3} {
    switch -exact -- $SvxlinkCfgFunctionName {
	  # A
	  ActivateModuleOnLongCmd {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "ACTIVATE_MODULE_ON_LONG_CMD=" 
	    deactivateModule	  
		}
	  AntennaDir {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "ANTENNA_DIR=" 
	    deactivateModule	  
		}
	  AntennaGain {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "ANTENNA_GAIN=" 
	    deactivateModule	  
		}
	  AntennaHeight{
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "ANTENNA_HEIGHT=" 
	    deactivateModule	  
		}
	  AprsServerList {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "APRS_SERVER_LIST=" 
	    deactivateModule	  
		}
	  AudioChannel {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "AUDIO_CHANNEL=" 
	    deactivateModule	  
		}
	  AudioDev {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "AUDIO_DEV=" 
	    deactivateModule	  
		}
	  AuthKey {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "AUTH_KEY=" 
	    deactivateModule	  
		}
	  AutoActivateOnSql {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "AUTO_ACTIVATE_ON_SQL=" 
	    deactivateModule	  
		}
	  # B
	  BeaconInterval {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "BEACON_INTERVAL=" 
	    deactivateModule	  
		}
	  BufferLength {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "BUFFER_LENGTH=" 
	    deactivateModule	  
		}
	  # C
	  CallSign {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CALLSIGN=" 
	    deactivateModule	  
		}
	  CardSampleRate {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CARD_SAMPLE_RATE=" 
	    deactivateModule
		}
	  Codec {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CODEC=" 
	    deactivateModule	  
		}
	  ConnectLogics {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CONNECT_LOGICS=" 
	    deactivateModule	  
		}
	  CtcssBpfLow {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CTCSS_BPF_LOW=" 
	    deactivateModule	  
		}
	  CtcssBpfHigh {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CTCSS_BPF_HIGH=" 
	    deactivateModule	  
		}
	  CtcssCloseThresh {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CTCSS_CLOSE_THRESH=" 
	    deactivateModule	  
		}
	  CtcssFq {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CTCSS_FQ=" 
	    deactivateModule	  
		}
	  CtcssLevel {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "CTCSS_LEVEL=" 
	    deactivateModule
		}
	  CTCSSmode {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "CTCSS_MODE=" 
	    deactivateModule
		}
	  CtcssOpenThresh {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CTCSS_OPEN_THRESH=" 
	    deactivateModule	  
		}
	  CtcssSnrOffset {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "CTCSS_SNR_OFFSET=" 
	    deactivateModule	  
		}
	  # D
	  Deemphasis {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "DEEMPHESIS=" 
	    deactivateModule	  
		}
	  DefaultActive {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "DEFAULT_ACTIVE=" 
	    deactivateModule	  
		}
	  DefaultLanguage {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "DEFAULT_LANG=" 
	    deactivateModule	  
		}
	  DtmfDecType {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "DTMF_DEC_TYPE=" 
	    deactivateModule
		}
	  DtmfHangtime {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "DTMF_HANGTIME=" 
	    deactivateModule
		}
	  DtmfMaxFwdTwist {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "DTMF_MAX_FWD_TWIST=" 
	    deactivateModule
		}
	  DTMFmute {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "DTMF_MUTING=" 
	    deactivateModule
		}
	  DtmfSerial {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "DTMF_SERIAL=" 
	    deactivateModule
		}
	  DtmfToneAmp {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "DTMF_TONE_AMP=" 
	    deactivateModule
		}
	  DtmfToneLength {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "DTMF_TONE_LENGTH=" 
	    deactivateModule
		}
	  DtmfToneSpacing {
		setConfigValue $SvxlinkCfgChannel $cmd $configPath "DTMF_TONE_SPACING=" 
	    deactivateModule
		}
	  # E
	  EncodeCmd {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "ENCODE_CMD=" 
	    deactivateModule	  
		}
	  EvdevClose  {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "EVDEV_CLOSE=" 
	    deactivateModule	  
		}
	  EvdevDevname {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "EVDEV_DEVNAME=" 
	    deactivateModule	  
		}
	  EvdevOpen {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "EVDEV_OPEN=" 
	    deactivateModule	  
		}
	  EventHandler {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "EVENT_HANDLER=" 
	    deactivateModule	  
		}
	  ExecuteCmdOnSqlClose {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "EXEC_CMD_ON_SQL_CLOSE=" 
	    deactivateModule	  
		}
	  # F
	  Frequency {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "FREQUENCY=" 
	    deactivateModule	  
		}
	  FxGainLow {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "FX_GAIN_LOW=" 
	    deactivateModule	  
		}
	  FxGainNormal {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "FX_GAIN_NORMAL=" 
	    deactivateModule	  
		}
	  # G
	  GpioSqlPin {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "GPIO_SQL_PIN=" 
	    deactivateModule	  
		}
	  # H
	  Host {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "HOST=" 
	    deactivateModule	  
		}
	  Hysteresis {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "HYSTERESIS=" 
	    deactivateModule	  
		}
	  # I
	  IdAfterTx {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "IDENT_ONLY_AFTER_TX=" 
	    deactivateModule	  
		}
	  IdentNagMinTime {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "IDENT_NAG_MIN_TIME=" 
	    deactivateModule	  
		}
	  IdentNagTimeout {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "IDENT_NAG_TIMEOUT=" 
	    deactivateModule	  
		}
	  IdleSoundInterval {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "IDLE_SOUND_INTERVAL=" 
	    deactivateModule	  
		}
	  IdleTimeout {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "IDLE_TIMEOUT=" 
	    deactivateModule	  
		}
	  # J
	  # K
	  # L
	  LatPosition {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "LAT_POSITION=" 
	    deactivateModule	  
		}
	  Links {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "LINKS=" 
	    deactivateModule	  
		}
	  LocInfo {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "LOC_INFO=" 
	    deactivateModule	  
		}
	  LongIdInt {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "LONG_IDENT_INTERVAL=" 
	    deactivateModule	  
		}
	  
	  LonPosition {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "LON_POSITION=" 
	    deactivateModule	  
		}
	  # M
	  Macros {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "MACROS=" 
	    deactivateModule	  
		}
	  MaxDirSize {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "MAX_DIR_SIZE=" 
	    deactivateModule	  
		}
	  MaxTime {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "MAX_TIME=" 
	    deactivateModule	  
		}
	  MinTime {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "MIN_TIME=" 
	    deactivateModule	  
		}
	  Modules {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "MODULES=" 
	    deactivateModule	  
		}
	  MuteRxOnTx {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "MUTE_RX_ON_TX=" 
	    deactivateModule	  
		}
	  # N
	  NoRepeat {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "NO_REPEAT=" 
	    deactivateModule	  
		}
	  # O
	  OnlineCmd {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "ONLINE_CMD=" 
	    deactivateModule	  
		}
	  OpenOn1750 {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPEN_ON_1750=" 
	    deactivateModule	  
		}
	  OpenOnCtcss {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPEN_ON_CTCSS=" 
	    deactivateModule	  
		}
	  OpenOnDtmf {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPEN_ON_DTMF=" 
	    deactivateModule	  
		}
	  OpenOnSel5 {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPEN_ON_SEL5=" 
	    deactivateModule	  
		}
	  OpenOnSql {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPEN_ON_SQL=" 
	    deactivateModule	  
		}
	  OpenOnSqlAfterRptClose {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPEN_ON_SQL_AFTER_RPT_CLOSE=" 
	    deactivateModule	  
		}
	  OpenSqlFlank {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPEN_SQL_FLANK=" 
	    deactivateModule	  
		}
	  OpusEncBitrate {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPUS_ENC_BITRATE="
	    deactivateModule	  
		} 
	  OpusEncComplexity {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPUS_ENC_COMPLEXITY="
	    deactivateModule	  
		} 
	  OpusEncVbr {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "OPUS_ENC_VBR="
	    deactivateModule	  
		} 
	  # P
	  Path {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "PATH="
	    deactivateModule	  
		}
	  PeakMeter {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "PEAK_METER="
	    deactivateModule	  
		}
	  Preamp {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "PREAMP=" 
	    deactivateModule	  
		}
	  Preemphesis {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "PREEMPHESIS=" 
	    deactivateModule	  
		}
	  PttHangtime {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "PTT_HANGTIME=" 
	    deactivateModule	  
		}
	  PttPin {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "PTT_PIN=" 
	    deactivateModule	  
		}
	  PttPort {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "PTT_PORT=" 
	    deactivateModule	  
		}
	  # Q
	  QsoRecorder {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "QSO_RECORDER=" 
	    deactivateModule	  
		}
	  QsoTimeout {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "QSO_TIMEOUT=" 
	    deactivateModule	  
		}
	  # R
	  RecDir {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "REC_DIR=" 
	    deactivateModule	  
		}
	  Receivers {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "RECEIVERS=" 
	    deactivateModule	  
		}
	  ReportCtcss {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "REPORT_CTCSS=" 
	    deactivateModule	  
		}
	  RevoteInterval {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "REVOTE_INTERVAL=" 
	    deactivateModule	  
		}
	  RogerSoundDelay {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "ROGER_SOUND_DELAY=" 
	    deactivateModule	  
		}
	  Rx {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "RX=" 
	    deactivateModule	  
		}
	  RxSwitchDelay {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "RX_SWITCH_DELAY=" 
	    deactivateModule	  
		}
	  # S
	  Sel5DecType {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SEL5_DEC_TYPE=" 
	    deactivateModule	  
		}
	  Sel5MacroRange {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SEL5_MACRO_RANGE=" 
	    deactivateModule	  
		}
	  Sel5Type {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SEL5_TYPE=" 
	    deactivateModule	  
		}
	  SerialPin {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SERIAL_PIN=" 
	    deactivateModule	  
		}
	  SerialPort {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SERIAL_PORT=" 
	    deactivateModule	  
		}
	  SerialSetPins {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SERIAL_SET_PINS=" 
	    deactivateModule
		}
	  ShortIdInt {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SHORT_IDENT_INTERVAL=" 
	    deactivateModule
		}
	  SiglevCloseThresh {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SIGLEV_CLOSE_THRESH=" 
	    deactivateModule	  
		}
	  SiglevDet {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SIGLEV_DET=" 
	    deactivateModule	  
		}
	  SiglevOffset {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SIGLEV_OFFSET=" 
	    deactivateModule	  
		}
	  SiglevOpenThresh {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SIGLEV_OPEN_THRESH=" 
	    deactivateModule	  
		}
	  SiglevSlope {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SIGLEV_SLOPE=" 
	    deactivateModule	  
		}
	  SoftTime {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SOFT_TIME=" 
	    deactivateModule	  
		}
	  SpeexEncAbr {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SPEEX_ENC_ABR=" 
	    deactivateModule	  
		}
	  SpeexEncBitrate {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SPEEX_ENC_BITRATE=" 
	    deactivateModule	  
		}
	  SpeexEncComplexity {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SPEEX_ENC_COMPLEXITY=" 
	    deactivateModule	  
		}
	  SpeexDecEnhancer {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SPEEX_DEC_ENHANCER=" 
	    deactivateModule	  
		}
	  SpeexEncFramesPerPacket {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SPEEX_ENC_FRAMES_PER_PACKET=" 
	    deactivateModule	  
		}
	  SpeexEncQuality {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SPEEX_ENC_QUALITY=" 
	    deactivateModule	  
		}
	  SpeexEncVbr {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SPEEX_ENC_VBR=" 
	    deactivateModule	  
		}
	  SpeexEncVbrQuality {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SPEEX_ENC_VBR_QUALITY=" 
	    deactivateModule	  
		}
	  SqlCloseRevoteDelay {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_CLOSE_REVOTE_DELAY=" 
	    deactivateModule	  
		}
	  SqlDelay {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_DELAY=" 
	    deactivateModule	  
		}
	  SqlDet {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_DET=" 
	    deactivateModule	  
		}
	  SqlExtendedHangtime {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_EXTENDED_HANGTIME=" 
	    deactivateModule	  
		}
	  SqlExtendedHangtimeThresh {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_EXTENDED_HANGTIME_THRESH=" 
	    deactivateModule	  
		}
	  SqlFlapSupMaxCount {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_FLAP_SUP_MAX_COUNT=" 
	    deactivateModule	  
		}
	  SqlFlapSupMinTime {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_FLAP_SUP_MIN_TIME=" 
	    deactivateModule	  
		}
	  SqlHangtime {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_HANGTIME=" 
	    deactivateModule	  
		}
	  SqlStartDelay {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_START_DELAY=" 
	    deactivateModule	  
		}
	  SqlTailElim {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_TAIL_ELIM=" 
	    deactivateModule	  
		}
	  SqlTimeout {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "SQL_TIMEOUT=" 
	    deactivateModule	  
		}
	  StatusServerList {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "STATUS_SERVER_LIST=" 
	    deactivateModule	  
		}
	  # T
	  Timeout {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TIMEOUT=" 
	    deactivateModule	  
		}
	  Tone {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TONE=" 
	    deactivateModule	  
		}
	  Tone1750Muting {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TONE_1750_MUTING=" 
	    deactivateModule	  
		}
	  ToneSiglevLevel {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TONE_SIGLEV_LEVEL=" 
	    deactivateModule	  
		}
	  ToneSiglevMap {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TONE_SIGLEV_MAP=" 
	    deactivateModule	  
		}
	  Transmitters {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TRANSMITTERS=" 
	    deactivateModule	  
		}
	  
	  Tx {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TX=" 
	    deactivateModule	  
		}
	  TxCtcss {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TX_CTCSS=" 
	    deactivateModule	  
		}
	  TxDelay {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TX_DELAY=" 
	    deactivateModule	  
		}
	  TxPower {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TX_POWER=" 
	    deactivateModule	  
		}
	  Type {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "TYPE=" 
	    deactivateModule	  
		}
	  # U
	  # V
	  VotingDelay {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "VOTING_DELAY=" 
	    deactivateModule	  
		}
	  VoxFilterDepth {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "VOX_FILTER_DEPTH=" 
	    deactivateModule	  
		}
	  VoxThresh {
	    setConfigValue $SvxlinkCfgChannel $cmd $configPath "VOX_THRESH=" 
	    deactivateModule	  
		}
	  # W
	  # X
	  # Y
	  # Z
	  default {
	     printInfo "ERROR: svxlinkCfgMenuLayer 3 - entry not found"
		 playMsg "invalid_configuration_selection"
	  }
    }
  }
}


# end of namespace
}


#
# This file has not been truncated
#
