How to use Image Generate Tool
=========================GNU=================================
1.modify image_gen_config_gnu.ini
	[PROJECT_BASE]
  chip_name = [LQFP128/QFN72/WLCSP38]
  config_mode = [Debug/Release]
  part_number = [HX6539-A04TLDG-1111K/HX6537-A08TDHG-1111K/...]
  
  [MEMORY_DESCIRPTOR]
  sec_format = BLp
  
  [BOOTLOADER]
  [2ND_BOOTLOADER]
  [APPLICATION]
  sec_format = [BLp/BLw]
  
  
  # part_type
  e_bootloader       = 0
  e_2nd_bootloader   = 1
  e_boot_patcher     = 2
  e_layout           = 3
  e_app              = 4
  e_app_config       = 5
  e_emza_config      = 6
  e_cnn_lut          = 7
  e_loger            = 8
  
  e_standalone       = 12   #need to signature(int)
  e_standalone_3rd   = 13   #3rd signature by themself  

2.make flash

=========================WINDOWS=================================
1.modify image_gen_config_BLp.ini/image_gen_config_BLw.ini
# part_type
e_bootloader       = 0
e_2nd_bootloader   = 1
e_boot_patcher     = 2
e_layout           = 3
e_app              = 4
e_app_config       = 5
e_emza_config      = 6
e_cnn_lut          = 7
e_loger            = 8

e_standalone       = 12   #need to signature(int)
e_standalone_3rd   = 13   #3rd signature by themself


ini setting example:

[STANDALONE]
pat_type = c
input_file = input\standalone_algo
sec_format = BLp
version = 2
fw_type = 3
flash_addr = 0x60000
devision_size = FB00


or 

[STANDALONE]
pat_type = d
input_file = input\standalone_output.img
standalone_dsp = input\layout_standalone.bin
sec_format = RAW
version = 2
fw_type = 3
flash_addr = 0x60000
devision_size = FB00

Notice :
STANDALONE flash_addr can't be too large

##Autorun list##
Modify setting.txt directly
follow below rule
[IC] [version] [PART NUMBER] [CONFIG MODE] [SEC MODE]
WLCSP38 r16 HX6535-A01TWA-0000K Debug BLp


[CONFIG MODE]: Debug Release
[SEC MODE]: BLw BLp
2.check the setting in setting.txt 
3.run Autorun.bat