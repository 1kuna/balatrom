#include <gb/gb.h>
__asm
.module raster_irq
.area _CODE
.globl _raster_stat
.globl _raster_vblank
_raster_stat::
push af
_raster_mode_gate::
ldh a, (0x41)
and #3
cp #2
push bc
push de
push hl
jp nz, raster_bad
ldh a, (0xd0)
ldh (0x70), a
ldh a, (0x44)
ldh (0xb6), a
sub #32
cp #16
jp nc, raster_bad
add a, a
ld e, a
ld d, #0
ld hl, #raster_table
add hl, de
ld a, (hl+)
ld h, (hl)
ld l, a
jp (hl)
raster_table:
.dw job32, job33, job34, job35, job36, job37, job38, job39, job40, raster_bad, job42, job43, job44, job45, job46, job47
job32:
ld a, #33
ldh (0x45), a
ld hl, #0xd298
ld c, #0x69
ld a, #0x98
ldh (0x68), a
wait32:
ldh a, (0x41)
and #3
jr nz, wait32
ldh a, (0x44)
cp #32
jp nz, raster_bad
_raster_job32_begin::
ld a, (hl+)
_raster_store_0::
ldh (c), a
_raster_store_0_done::
ld a, (hl+)
_raster_store_1::
ldh (c), a
_raster_store_1_done::
ld a, (hl+)
_raster_store_2::
ldh (c), a
_raster_store_2_done::
ld a, (hl+)
_raster_store_3::
ldh (c), a
_raster_store_3_done::
ld a, (hl+)
_raster_store_4::
ldh (c), a
_raster_store_4_done::
ld a, (hl+)
_raster_store_5::
ldh (c), a
_raster_store_5_done::
ld a, (hl+)
_raster_store_6::
ldh (c), a
_raster_store_6_done::
ld a, (hl+)
_raster_store_7::
ldh (c), a
_raster_store_7_done::
_raster_job32_end::
jp raster_return
job33:
ld a, #34
ldh (0x45), a
ld hl, #0xd2a0
ld c, #0x69
ld a, #0xa0
ldh (0x68), a
wait33:
ldh a, (0x41)
and #3
jr nz, wait33
ldh a, (0x44)
cp #33
jp nz, raster_bad
_raster_job33_begin::
ld a, (hl+)
_raster_store_8::
ldh (c), a
_raster_store_8_done::
ld a, (hl+)
_raster_store_9::
ldh (c), a
_raster_store_9_done::
ld a, (hl+)
_raster_store_10::
ldh (c), a
_raster_store_10_done::
ld a, (hl+)
_raster_store_11::
ldh (c), a
_raster_store_11_done::
ld a, (hl+)
_raster_store_12::
ldh (c), a
_raster_store_12_done::
ld a, (hl+)
_raster_store_13::
ldh (c), a
_raster_store_13_done::
ld a, (hl+)
_raster_store_14::
ldh (c), a
_raster_store_14_done::
ld a, (hl+)
_raster_store_15::
ldh (c), a
_raster_store_15_done::
_raster_job33_end::
jp raster_return
job34:
ld a, #35
ldh (0x45), a
ld hl, #0xd2a8
ld c, #0x69
ld a, #0xa8
ldh (0x68), a
wait34:
ldh a, (0x41)
and #3
jr nz, wait34
ldh a, (0x44)
cp #34
jp nz, raster_bad
_raster_job34_begin::
ld a, (hl+)
_raster_store_16::
ldh (c), a
_raster_store_16_done::
ld a, (hl+)
_raster_store_17::
ldh (c), a
_raster_store_17_done::
ld a, (hl+)
_raster_store_18::
ldh (c), a
_raster_store_18_done::
ld a, (hl+)
_raster_store_19::
ldh (c), a
_raster_store_19_done::
ld a, (hl+)
_raster_store_20::
ldh (c), a
_raster_store_20_done::
ld a, (hl+)
_raster_store_21::
ldh (c), a
_raster_store_21_done::
ld a, (hl+)
_raster_store_22::
ldh (c), a
_raster_store_22_done::
ld a, (hl+)
_raster_store_23::
ldh (c), a
_raster_store_23_done::
_raster_job34_end::
jp raster_return
job35:
ld a, #36
ldh (0x45), a
ld hl, #0xd2b0
ld c, #0x69
ld a, #0xb0
ldh (0x68), a
wait35:
ldh a, (0x41)
and #3
jr nz, wait35
ldh a, (0x44)
cp #35
jp nz, raster_bad
_raster_job35_begin::
ld a, (hl+)
_raster_store_24::
ldh (c), a
_raster_store_24_done::
ld a, (hl+)
_raster_store_25::
ldh (c), a
_raster_store_25_done::
ld a, (hl+)
_raster_store_26::
ldh (c), a
_raster_store_26_done::
ld a, (hl+)
_raster_store_27::
ldh (c), a
_raster_store_27_done::
ld a, (hl+)
_raster_store_28::
ldh (c), a
_raster_store_28_done::
ld a, (hl+)
_raster_store_29::
ldh (c), a
_raster_store_29_done::
ld a, (hl+)
_raster_store_30::
ldh (c), a
_raster_store_30_done::
ld a, (hl+)
_raster_store_31::
ldh (c), a
_raster_store_31_done::
_raster_job35_end::
jp raster_return
job36:
ld a, #37
ldh (0x45), a
ld hl, #0xd2b8
ld c, #0x69
ld a, #0xb8
ldh (0x68), a
wait36:
ldh a, (0x41)
and #3
jr nz, wait36
ldh a, (0x44)
cp #36
jp nz, raster_bad
_raster_job36_begin::
ld a, (hl+)
_raster_store_32::
ldh (c), a
_raster_store_32_done::
ld a, (hl+)
_raster_store_33::
ldh (c), a
_raster_store_33_done::
ld a, (hl+)
_raster_store_34::
ldh (c), a
_raster_store_34_done::
ld a, (hl+)
_raster_store_35::
ldh (c), a
_raster_store_35_done::
ld a, (hl+)
_raster_store_36::
ldh (c), a
_raster_store_36_done::
ld a, (hl+)
_raster_store_37::
ldh (c), a
_raster_store_37_done::
ld a, (hl+)
_raster_store_38::
ldh (c), a
_raster_store_38_done::
ld a, (hl+)
_raster_store_39::
ldh (c), a
_raster_store_39_done::
_raster_job36_end::
jp raster_return
job37:
ld a, #38
ldh (0x45), a
ld hl, #0xd2c2
ld c, #0x6b
ld a, #0x82
ldh (0x6a), a
wait37:
ldh a, (0x41)
and #3
jr nz, wait37
ldh a, (0x44)
cp #37
jp nz, raster_bad
_raster_job37_begin::
ld a, (hl+)
_raster_store_40::
ldh (c), a
_raster_store_40_done::
ld a, (hl+)
_raster_store_41::
ldh (c), a
_raster_store_41_done::
ld a, (hl+)
_raster_store_42::
ldh (c), a
_raster_store_42_done::
ld a, (hl+)
_raster_store_43::
ldh (c), a
_raster_store_43_done::
ld a, (hl+)
_raster_store_44::
ldh (c), a
_raster_store_44_done::
ld a, (hl+)
_raster_store_45::
ldh (c), a
_raster_store_45_done::
ld hl, #0xd286
ld c, #0x69
ld a, #0x86
ldh (0x68), a
ld a, (hl+)
_raster_store_46::
ldh (c), a
_raster_store_46_done::
ld a, (hl+)
_raster_store_47::
ldh (c), a
_raster_store_47_done::
_raster_job37_end::
jp raster_return
job38:
ld a, #39
ldh (0x45), a
ld hl, #0xd2ca
ld c, #0x6b
ld a, #0x8a
ldh (0x6a), a
wait38:
ldh a, (0x41)
and #3
jr nz, wait38
ldh a, (0x44)
cp #38
jp nz, raster_bad
_raster_job38_begin::
ld a, (hl+)
_raster_store_48::
ldh (c), a
_raster_store_48_done::
ld a, (hl+)
_raster_store_49::
ldh (c), a
_raster_store_49_done::
ld a, (hl+)
_raster_store_50::
ldh (c), a
_raster_store_50_done::
ld a, (hl+)
_raster_store_51::
ldh (c), a
_raster_store_51_done::
ld a, (hl+)
_raster_store_52::
ldh (c), a
_raster_store_52_done::
ld a, (hl+)
_raster_store_53::
ldh (c), a
_raster_store_53_done::
_raster_job38_end::
jp raster_return
job39:
ld a, #40
ldh (0x45), a
ld hl, #0xd2d2
ld c, #0x6b
ld a, #0x92
ldh (0x6a), a
wait39:
ldh a, (0x41)
and #3
jr nz, wait39
ldh a, (0x44)
cp #39
jp nz, raster_bad
_raster_job39_begin::
ld a, (hl+)
_raster_store_54::
ldh (c), a
_raster_store_54_done::
ld a, (hl+)
_raster_store_55::
ldh (c), a
_raster_store_55_done::
ld a, (hl+)
_raster_store_56::
ldh (c), a
_raster_store_56_done::
ld a, (hl+)
_raster_store_57::
ldh (c), a
_raster_store_57_done::
ld a, (hl+)
_raster_store_58::
ldh (c), a
_raster_store_58_done::
ld a, (hl+)
_raster_store_59::
ldh (c), a
_raster_store_59_done::
_raster_job39_end::
jp raster_return
job40:
ld a, #42
ldh (0x45), a
wait40:
ldh a, (0x41)
and #3
jr nz, wait40
ldh a, (0x44)
cp #40
jp nz, raster_bad
_raster_job40_begin::
ld a, #0xd1
_raster_dma40_call::
call #0xffc0
_raster_dma40_return::
ld a, #2
ldh (0xb7), a
_raster_job40_end::
jp raster_return
job42:
ld a, #43
ldh (0x45), a
ld hl, #0xd2da
ld c, #0x6b
ld a, #0x9a
ldh (0x6a), a
wait42:
ldh a, (0x41)
and #3
jr nz, wait42
ldh a, (0x44)
cp #42
jp nz, raster_bad
_raster_job42_begin::
ld a, (hl+)
_raster_store_60::
ldh (c), a
_raster_store_60_done::
ld a, (hl+)
_raster_store_61::
ldh (c), a
_raster_store_61_done::
ld a, (hl+)
_raster_store_62::
ldh (c), a
_raster_store_62_done::
ld a, (hl+)
_raster_store_63::
ldh (c), a
_raster_store_63_done::
ld a, (hl+)
_raster_store_64::
ldh (c), a
_raster_store_64_done::
ld a, (hl+)
_raster_store_65::
ldh (c), a
_raster_store_65_done::
_raster_job42_end::
jp raster_return
job43:
ld a, #44
ldh (0x45), a
ld hl, #0xd2e2
ld c, #0x6b
ld a, #0xa2
ldh (0x6a), a
wait43:
ldh a, (0x41)
and #3
jr nz, wait43
ldh a, (0x44)
cp #43
jp nz, raster_bad
_raster_job43_begin::
ld a, (hl+)
_raster_store_66::
ldh (c), a
_raster_store_66_done::
ld a, (hl+)
_raster_store_67::
ldh (c), a
_raster_store_67_done::
ld a, (hl+)
_raster_store_68::
ldh (c), a
_raster_store_68_done::
ld a, (hl+)
_raster_store_69::
ldh (c), a
_raster_store_69_done::
ld a, (hl+)
_raster_store_70::
ldh (c), a
_raster_store_70_done::
ld a, (hl+)
_raster_store_71::
ldh (c), a
_raster_store_71_done::
_raster_job43_end::
jp raster_return
job44:
ld a, #45
ldh (0x45), a
ld hl, #0xd2ea
ld c, #0x6b
ld a, #0xaa
ldh (0x6a), a
wait44:
ldh a, (0x41)
and #3
jr nz, wait44
ldh a, (0x44)
cp #44
jp nz, raster_bad
_raster_job44_begin::
ld a, (hl+)
_raster_store_72::
ldh (c), a
_raster_store_72_done::
ld a, (hl+)
_raster_store_73::
ldh (c), a
_raster_store_73_done::
ld a, (hl+)
_raster_store_74::
ldh (c), a
_raster_store_74_done::
ld a, (hl+)
_raster_store_75::
ldh (c), a
_raster_store_75_done::
ld a, (hl+)
_raster_store_76::
ldh (c), a
_raster_store_76_done::
ld a, (hl+)
_raster_store_77::
ldh (c), a
_raster_store_77_done::
_raster_job44_end::
jp raster_return
job45:
ld a, #46
ldh (0x45), a
ld hl, #0xd2f2
ld c, #0x6b
ld a, #0xb2
ldh (0x6a), a
wait45:
ldh a, (0x41)
and #3
jr nz, wait45
ldh a, (0x44)
cp #45
jp nz, raster_bad
_raster_job45_begin::
ld a, (hl+)
_raster_store_78::
ldh (c), a
_raster_store_78_done::
ld a, (hl+)
_raster_store_79::
ldh (c), a
_raster_store_79_done::
ld a, (hl+)
_raster_store_80::
ldh (c), a
_raster_store_80_done::
ld a, (hl+)
_raster_store_81::
ldh (c), a
_raster_store_81_done::
ld a, (hl+)
_raster_store_82::
ldh (c), a
_raster_store_82_done::
ld a, (hl+)
_raster_store_83::
ldh (c), a
_raster_store_83_done::
_raster_job45_end::
jp raster_return
job46:
ld a, #47
ldh (0x45), a
ld hl, #0xd2fa
ld c, #0x6b
ld a, #0xba
ldh (0x6a), a
wait46:
ldh a, (0x41)
and #3
jr nz, wait46
ldh a, (0x44)
cp #46
jp nz, raster_bad
_raster_job46_begin::
ld a, (hl+)
_raster_store_84::
ldh (c), a
_raster_store_84_done::
ld a, (hl+)
_raster_store_85::
ldh (c), a
_raster_store_85_done::
ld a, (hl+)
_raster_store_86::
ldh (c), a
_raster_store_86_done::
ld a, (hl+)
_raster_store_87::
ldh (c), a
_raster_store_87_done::
ld a, (hl+)
_raster_store_88::
ldh (c), a
_raster_store_88_done::
ld a, (hl+)
_raster_store_89::
ldh (c), a
_raster_store_89_done::
_raster_job46_end::
jp raster_return
job47:
ld a, #32
ldh (0x45), a
ldh a, (0xba)
or a
jr z, no_delay47
ld b, #0
delay47:
dec b
jr nz, delay47
no_delay47:
ld hl, #0xd288
ld c, #0x69
ld a, #0x88
ldh (0x68), a
wait47:
ldh a, (0x41)
and #3
jr nz, wait47
ldh a, (0x44)
cp #47
jp nz, raster_bad
_raster_job47_begin::
ld a, (hl+)
_raster_store_90::
ldh (c), a
_raster_store_90_done::
ld a, (hl+)
_raster_store_91::
ldh (c), a
_raster_store_91_done::
ld a, (hl+)
_raster_store_92::
ldh (c), a
_raster_store_92_done::
ld a, (hl+)
_raster_store_93::
ldh (c), a
_raster_store_93_done::
ld a, (hl+)
_raster_store_94::
ldh (c), a
_raster_store_94_done::
ld a, (hl+)
_raster_store_95::
ldh (c), a
_raster_store_95_done::
ld a, (hl+)
_raster_store_96::
ldh (c), a
_raster_store_96_done::
ld a, (hl+)
_raster_store_97::
ldh (c), a
_raster_store_97_done::
ld a, (hl+)
_raster_store_98::
ldh (c), a
_raster_store_98_done::
ld a, (hl+)
_raster_store_99::
ldh (c), a
_raster_store_99_done::
ld a, (hl+)
_raster_store_100::
ldh (c), a
_raster_store_100_done::
ld a, (hl+)
_raster_store_101::
ldh (c), a
_raster_store_101_done::
ld a, (hl+)
_raster_store_102::
ldh (c), a
_raster_store_102_done::
ld a, (hl+)
_raster_store_103::
ldh (c), a
_raster_store_103_done::
ld a, (hl+)
_raster_store_104::
ldh (c), a
_raster_store_104_done::
ld a, (hl+)
_raster_store_105::
ldh (c), a
_raster_store_105_done::
_raster_job47_end::
jp raster_return
raster_bad:
ld a, #1
ldh (0xb5), a
ld a, #32
ldh (0x45), a
raster_return:
ld a, #1
ldh (0x70), a
pop hl
pop de
pop bc
pop af
_raster_stat_reti::
reti
_raster_vblank::
push af
push bc
push de
push hl
ldh a, (0xd1)
or a
jp z, vblank_done
ldh a, (0xd0)
ldh (0x70), a
ldh a, (0xb5)
or a
jr z, upper_restore
ldh a, (0x40)
and #0x7f
ldh (0x40), a
xor a
ldh (0x41), a
jp vblank_done
upper_restore:
ld hl, #0xd200
ld c, #0x69
ld a, #0x80
ldh (0x68), a
ld a, (hl+)
_raster_store_106::
ldh (c), a
_raster_store_106_done::
ld a, (hl+)
_raster_store_107::
ldh (c), a
_raster_store_107_done::
ld a, (hl+)
_raster_store_108::
ldh (c), a
_raster_store_108_done::
ld a, (hl+)
_raster_store_109::
ldh (c), a
_raster_store_109_done::
ld a, (hl+)
_raster_store_110::
ldh (c), a
_raster_store_110_done::
ld a, (hl+)
_raster_store_111::
ldh (c), a
_raster_store_111_done::
ld a, (hl+)
_raster_store_112::
ldh (c), a
_raster_store_112_done::
ld a, (hl+)
_raster_store_113::
ldh (c), a
_raster_store_113_done::
ld a, (hl+)
_raster_store_114::
ldh (c), a
_raster_store_114_done::
ld a, (hl+)
_raster_store_115::
ldh (c), a
_raster_store_115_done::
ld a, (hl+)
_raster_store_116::
ldh (c), a
_raster_store_116_done::
ld a, (hl+)
_raster_store_117::
ldh (c), a
_raster_store_117_done::
ld a, (hl+)
_raster_store_118::
ldh (c), a
_raster_store_118_done::
ld a, (hl+)
_raster_store_119::
ldh (c), a
_raster_store_119_done::
ld a, (hl+)
_raster_store_120::
ldh (c), a
_raster_store_120_done::
ld a, (hl+)
_raster_store_121::
ldh (c), a
_raster_store_121_done::
ld a, (hl+)
_raster_store_122::
ldh (c), a
_raster_store_122_done::
ld a, (hl+)
_raster_store_123::
ldh (c), a
_raster_store_123_done::
ld a, (hl+)
_raster_store_124::
ldh (c), a
_raster_store_124_done::
ld a, (hl+)
_raster_store_125::
ldh (c), a
_raster_store_125_done::
ld a, (hl+)
_raster_store_126::
ldh (c), a
_raster_store_126_done::
ld a, (hl+)
_raster_store_127::
ldh (c), a
_raster_store_127_done::
ld a, (hl+)
_raster_store_128::
ldh (c), a
_raster_store_128_done::
ld a, (hl+)
_raster_store_129::
ldh (c), a
_raster_store_129_done::
ld a, (hl+)
_raster_store_130::
ldh (c), a
_raster_store_130_done::
ld a, (hl+)
_raster_store_131::
ldh (c), a
_raster_store_131_done::
ld a, (hl+)
_raster_store_132::
ldh (c), a
_raster_store_132_done::
ld a, (hl+)
_raster_store_133::
ldh (c), a
_raster_store_133_done::
ld a, (hl+)
_raster_store_134::
ldh (c), a
_raster_store_134_done::
ld a, (hl+)
_raster_store_135::
ldh (c), a
_raster_store_135_done::
ld a, (hl+)
_raster_store_136::
ldh (c), a
_raster_store_136_done::
ld a, (hl+)
_raster_store_137::
ldh (c), a
_raster_store_137_done::
ld a, (hl+)
_raster_store_138::
ldh (c), a
_raster_store_138_done::
ld a, (hl+)
_raster_store_139::
ldh (c), a
_raster_store_139_done::
ld a, (hl+)
_raster_store_140::
ldh (c), a
_raster_store_140_done::
ld a, (hl+)
_raster_store_141::
ldh (c), a
_raster_store_141_done::
ld a, (hl+)
_raster_store_142::
ldh (c), a
_raster_store_142_done::
ld a, (hl+)
_raster_store_143::
ldh (c), a
_raster_store_143_done::
ld a, (hl+)
_raster_store_144::
ldh (c), a
_raster_store_144_done::
ld a, (hl+)
_raster_store_145::
ldh (c), a
_raster_store_145_done::
ld a, (hl+)
_raster_store_146::
ldh (c), a
_raster_store_146_done::
ld a, (hl+)
_raster_store_147::
ldh (c), a
_raster_store_147_done::
ld a, (hl+)
_raster_store_148::
ldh (c), a
_raster_store_148_done::
ld a, (hl+)
_raster_store_149::
ldh (c), a
_raster_store_149_done::
ld a, (hl+)
_raster_store_150::
ldh (c), a
_raster_store_150_done::
ld a, (hl+)
_raster_store_151::
ldh (c), a
_raster_store_151_done::
ld a, (hl+)
_raster_store_152::
ldh (c), a
_raster_store_152_done::
ld a, (hl+)
_raster_store_153::
ldh (c), a
_raster_store_153_done::
ld a, (hl+)
_raster_store_154::
ldh (c), a
_raster_store_154_done::
ld a, (hl+)
_raster_store_155::
ldh (c), a
_raster_store_155_done::
ld a, (hl+)
_raster_store_156::
ldh (c), a
_raster_store_156_done::
ld a, (hl+)
_raster_store_157::
ldh (c), a
_raster_store_157_done::
ld a, (hl+)
_raster_store_158::
ldh (c), a
_raster_store_158_done::
ld a, (hl+)
_raster_store_159::
ldh (c), a
_raster_store_159_done::
ld a, (hl+)
_raster_store_160::
ldh (c), a
_raster_store_160_done::
ld a, (hl+)
_raster_store_161::
ldh (c), a
_raster_store_161_done::
ld a, (hl+)
_raster_store_162::
ldh (c), a
_raster_store_162_done::
ld a, (hl+)
_raster_store_163::
ldh (c), a
_raster_store_163_done::
ld a, (hl+)
_raster_store_164::
ldh (c), a
_raster_store_164_done::
ld a, (hl+)
_raster_store_165::
ldh (c), a
_raster_store_165_done::
ld a, (hl+)
_raster_store_166::
ldh (c), a
_raster_store_166_done::
ld a, (hl+)
_raster_store_167::
ldh (c), a
_raster_store_167_done::
ld a, (hl+)
_raster_store_168::
ldh (c), a
_raster_store_168_done::
ld a, (hl+)
_raster_store_169::
ldh (c), a
_raster_store_169_done::
ld hl, #0xd240
ld c, #0x6b
ld a, #0x80
ldh (0x6a), a
ld a, (hl+)
_raster_store_170::
ldh (c), a
_raster_store_170_done::
ld a, (hl+)
_raster_store_171::
ldh (c), a
_raster_store_171_done::
ld a, (hl+)
_raster_store_172::
ldh (c), a
_raster_store_172_done::
ld a, (hl+)
_raster_store_173::
ldh (c), a
_raster_store_173_done::
ld a, (hl+)
_raster_store_174::
ldh (c), a
_raster_store_174_done::
ld a, (hl+)
_raster_store_175::
ldh (c), a
_raster_store_175_done::
ld a, (hl+)
_raster_store_176::
ldh (c), a
_raster_store_176_done::
ld a, (hl+)
_raster_store_177::
ldh (c), a
_raster_store_177_done::
ld a, (hl+)
_raster_store_178::
ldh (c), a
_raster_store_178_done::
ld a, (hl+)
_raster_store_179::
ldh (c), a
_raster_store_179_done::
ld a, (hl+)
_raster_store_180::
ldh (c), a
_raster_store_180_done::
ld a, (hl+)
_raster_store_181::
ldh (c), a
_raster_store_181_done::
ld a, (hl+)
_raster_store_182::
ldh (c), a
_raster_store_182_done::
ld a, (hl+)
_raster_store_183::
ldh (c), a
_raster_store_183_done::
ld a, (hl+)
_raster_store_184::
ldh (c), a
_raster_store_184_done::
ld a, (hl+)
_raster_store_185::
ldh (c), a
_raster_store_185_done::
ld a, (hl+)
_raster_store_186::
ldh (c), a
_raster_store_186_done::
ld a, (hl+)
_raster_store_187::
ldh (c), a
_raster_store_187_done::
ld a, (hl+)
_raster_store_188::
ldh (c), a
_raster_store_188_done::
ld a, (hl+)
_raster_store_189::
ldh (c), a
_raster_store_189_done::
ld a, (hl+)
_raster_store_190::
ldh (c), a
_raster_store_190_done::
ld a, (hl+)
_raster_store_191::
ldh (c), a
_raster_store_191_done::
ld a, (hl+)
_raster_store_192::
ldh (c), a
_raster_store_192_done::
ld a, (hl+)
_raster_store_193::
ldh (c), a
_raster_store_193_done::
ld a, (hl+)
_raster_store_194::
ldh (c), a
_raster_store_194_done::
ld a, (hl+)
_raster_store_195::
ldh (c), a
_raster_store_195_done::
ld a, (hl+)
_raster_store_196::
ldh (c), a
_raster_store_196_done::
ld a, (hl+)
_raster_store_197::
ldh (c), a
_raster_store_197_done::
ld a, (hl+)
_raster_store_198::
ldh (c), a
_raster_store_198_done::
ld a, (hl+)
_raster_store_199::
ldh (c), a
_raster_store_199_done::
ld a, (hl+)
_raster_store_200::
ldh (c), a
_raster_store_200_done::
ld a, (hl+)
_raster_store_201::
ldh (c), a
_raster_store_201_done::
ld a, (hl+)
_raster_store_202::
ldh (c), a
_raster_store_202_done::
ld a, (hl+)
_raster_store_203::
ldh (c), a
_raster_store_203_done::
ld a, (hl+)
_raster_store_204::
ldh (c), a
_raster_store_204_done::
ld a, (hl+)
_raster_store_205::
ldh (c), a
_raster_store_205_done::
ld a, (hl+)
_raster_store_206::
ldh (c), a
_raster_store_206_done::
ld a, (hl+)
_raster_store_207::
ldh (c), a
_raster_store_207_done::
ld a, (hl+)
_raster_store_208::
ldh (c), a
_raster_store_208_done::
ld a, (hl+)
_raster_store_209::
ldh (c), a
_raster_store_209_done::
ld a, (hl+)
_raster_store_210::
ldh (c), a
_raster_store_210_done::
ld a, (hl+)
_raster_store_211::
ldh (c), a
_raster_store_211_done::
ld a, (hl+)
_raster_store_212::
ldh (c), a
_raster_store_212_done::
ld a, (hl+)
_raster_store_213::
ldh (c), a
_raster_store_213_done::
ld a, (hl+)
_raster_store_214::
ldh (c), a
_raster_store_214_done::
ld a, (hl+)
_raster_store_215::
ldh (c), a
_raster_store_215_done::
ld a, (hl+)
_raster_store_216::
ldh (c), a
_raster_store_216_done::
ld a, (hl+)
_raster_store_217::
ldh (c), a
_raster_store_217_done::
ld a, (hl+)
_raster_store_218::
ldh (c), a
_raster_store_218_done::
ld a, (hl+)
_raster_store_219::
ldh (c), a
_raster_store_219_done::
ld a, (hl+)
_raster_store_220::
ldh (c), a
_raster_store_220_done::
ld a, (hl+)
_raster_store_221::
ldh (c), a
_raster_store_221_done::
ld a, (hl+)
_raster_store_222::
ldh (c), a
_raster_store_222_done::
ld a, (hl+)
_raster_store_223::
ldh (c), a
_raster_store_223_done::
ld a, (hl+)
_raster_store_224::
ldh (c), a
_raster_store_224_done::
ld a, (hl+)
_raster_store_225::
ldh (c), a
_raster_store_225_done::
ld a, (hl+)
_raster_store_226::
ldh (c), a
_raster_store_226_done::
ld a, (hl+)
_raster_store_227::
ldh (c), a
_raster_store_227_done::
ld a, (hl+)
_raster_store_228::
ldh (c), a
_raster_store_228_done::
ld a, (hl+)
_raster_store_229::
ldh (c), a
_raster_store_229_done::
ld a, (hl+)
_raster_store_230::
ldh (c), a
_raster_store_230_done::
ld a, (hl+)
_raster_store_231::
ldh (c), a
_raster_store_231_done::
ld a, (hl+)
_raster_store_232::
ldh (c), a
_raster_store_232_done::
ld a, (hl+)
_raster_store_233::
ldh (c), a
_raster_store_233_done::
ld a, #0xd0
_raster_dma_upper_call::
call #0xffc0
_raster_dma_upper_return::
ld a, #1
ldh (0xb7), a
ld a, #32
ldh (0x45), a
ldh a, (0xb8)
inc a
ldh (0xb8), a
jr nz, vblank_done
ldh a, (0xb9)
inc a
ldh (0xb9), a
vblank_done:
ld a, #1
ldh (0x70), a
pop hl
pop de
pop bc
pop af
_raster_vblank_ret::
ret
__endasm;
