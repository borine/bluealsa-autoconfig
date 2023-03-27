/*
 *  BlueALSA - a2dp-codecs.h
 *
 *  Copyright (C) 2006-2010  Nokia Corporation
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2018       Pali Rohár <pali.rohar@gmail.com>
 *  Copyright (C) 2016-2022  Arkadiusz Bokowy
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once
#ifndef BLUEALSA_SHARED_A2DPCODECS_H_
#define BLUEALSA_SHARED_A2DPCODECS_H_

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <endian.h>
#include <stdint.h>

#include "hci.h"

#define A2DP_CODEC_SBC      0x00
#define A2DP_CODEC_MPEG12   0x01
#define A2DP_CODEC_MPEG24   0x02
#define A2DP_CODEC_ATRAC    0x04
#define A2DP_CODEC_VENDOR   0xFF

/**
 * Customized (BlueALSA) 16-bit vendor extension. */
#define A2DP_CODEC_VENDOR_APTX          0x4FFF
#define A2DP_CODEC_VENDOR_APTX_AD       0xADFF
#define A2DP_CODEC_VENDOR_APTX_HD       0x24FF
#define A2DP_CODEC_VENDOR_APTX_LL       0xA2FF
#define A2DP_CODEC_VENDOR_APTX_TWS      0x25FF
#define A2DP_CODEC_VENDOR_FASTSTREAM    0xA1FF
#define A2DP_CODEC_VENDOR_LC3PLUS       0xC3FF
#define A2DP_CODEC_VENDOR_LDAC          0x2DFF
#define A2DP_CODEC_VENDOR_LHDC          0x4CFF
#define A2DP_CODEC_VENDOR_LHDC_LL       0x44FF
#define A2DP_CODEC_VENDOR_LHDC_V1       0x48FF
#define A2DP_CODEC_VENDOR_SAMSUNG_HD    0x52FF
#define A2DP_CODEC_VENDOR_SAMSUNG_SC    0x53FF

#define SBC_SAMPLING_FREQ_16000         (1 << 3)
#define SBC_SAMPLING_FREQ_32000         (1 << 2)
#define SBC_SAMPLING_FREQ_44100         (1 << 1)
#define SBC_SAMPLING_FREQ_48000         1

#define SBC_CHANNEL_MODE_MONO           (1 << 3)
#define SBC_CHANNEL_MODE_DUAL_CHANNEL   (1 << 2)
#define SBC_CHANNEL_MODE_STEREO         (1 << 1)
#define SBC_CHANNEL_MODE_JOINT_STEREO   1

#define SBC_BLOCK_LENGTH_4              (1 << 3)
#define SBC_BLOCK_LENGTH_8              (1 << 2)
#define SBC_BLOCK_LENGTH_12             (1 << 1)
#define SBC_BLOCK_LENGTH_16             1

#define SBC_SUBBANDS_4                  (1 << 1)
#define SBC_SUBBANDS_8                  1

#define SBC_ALLOCATION_SNR              (1 << 1)
#define SBC_ALLOCATION_LOUDNESS         1

#define SBC_MIN_BITPOOL                 2
#define SBC_MAX_BITPOOL                 250

/**
 * Predefined SBC bit-pool values.
 *
 * Other settings:
 *  - block length = 16
 *  - allocation method = Loudness
 *  - sub-bands = 8 */
#define SBC_BITPOOL_LQ_MONO_44100          15
#define SBC_BITPOOL_LQ_MONO_48000          15
#define SBC_BITPOOL_LQ_JOINT_STEREO_44100  29
#define SBC_BITPOOL_LQ_JOINT_STEREO_48000  29
#define SBC_BITPOOL_MQ_MONO_44100          19
#define SBC_BITPOOL_MQ_MONO_48000          18
#define SBC_BITPOOL_MQ_JOINT_STEREO_44100  35
#define SBC_BITPOOL_MQ_JOINT_STEREO_48000  33
#define SBC_BITPOOL_HQ_MONO_44100          31
#define SBC_BITPOOL_HQ_MONO_48000          29
#define SBC_BITPOOL_HQ_JOINT_STEREO_44100  53
#define SBC_BITPOOL_HQ_JOINT_STEREO_48000  51

#define MPEG_CHANNEL_MODE_MONO          (1 << 3)
#define MPEG_CHANNEL_MODE_DUAL_CHANNEL  (1 << 2)
#define MPEG_CHANNEL_MODE_STEREO        (1 << 1)
#define MPEG_CHANNEL_MODE_JOINT_STEREO  1

#define MPEG_LAYER_MP1                  (1 << 2)
#define MPEG_LAYER_MP2                  (1 << 1)
#define MPEG_LAYER_MP3                  1

#define MPEG_SAMPLING_FREQ_16000        (1 << 5)
#define MPEG_SAMPLING_FREQ_22050        (1 << 4)
#define MPEG_SAMPLING_FREQ_24000        (1 << 3)
#define MPEG_SAMPLING_FREQ_32000        (1 << 2)
#define MPEG_SAMPLING_FREQ_44100        (1 << 1)
#define MPEG_SAMPLING_FREQ_48000        1

#define MPEG_BIT_RATE_INDEX_0           (1 << 0)
#define MPEG_BIT_RATE_INDEX_1           (1 << 1)
#define MPEG_BIT_RATE_INDEX_2           (1 << 2)
#define MPEG_BIT_RATE_INDEX_3           (1 << 3)
#define MPEG_BIT_RATE_INDEX_4           (1 << 4)
#define MPEG_BIT_RATE_INDEX_5           (1 << 5)
#define MPEG_BIT_RATE_INDEX_6           (1 << 6)
#define MPEG_BIT_RATE_INDEX_7           (1 << 7)
#define MPEG_BIT_RATE_INDEX_8           (1 << 8)
#define MPEG_BIT_RATE_INDEX_9           (1 << 9)
#define MPEG_BIT_RATE_INDEX_10          (1 << 10)
#define MPEG_BIT_RATE_INDEX_11          (1 << 11)
#define MPEG_BIT_RATE_INDEX_12          (1 << 12)
#define MPEG_BIT_RATE_INDEX_13          (1 << 13)
#define MPEG_BIT_RATE_INDEX_14          (1 << 14)

#define MPEG_MP1_BIT_RATE_32000         MPEG_BIT_RATE_INDEX_1
#define MPEG_MP1_BIT_RATE_64000         MPEG_BIT_RATE_INDEX_2
#define MPEG_MP1_BIT_RATE_96000         MPEG_BIT_RATE_INDEX_3
#define MPEG_MP1_BIT_RATE_128000        MPEG_BIT_RATE_INDEX_4
#define MPEG_MP1_BIT_RATE_160000        MPEG_BIT_RATE_INDEX_5
#define MPEG_MP1_BIT_RATE_192000        MPEG_BIT_RATE_INDEX_6
#define MPEG_MP1_BIT_RATE_224000        MPEG_BIT_RATE_INDEX_7
#define MPEG_MP1_BIT_RATE_256000        MPEG_BIT_RATE_INDEX_8
#define MPEG_MP1_BIT_RATE_288000        MPEG_BIT_RATE_INDEX_9
#define MPEG_MP1_BIT_RATE_320000        MPEG_BIT_RATE_INDEX_10
#define MPEG_MP1_BIT_RATE_352000        MPEG_BIT_RATE_INDEX_11
#define MPEG_MP1_BIT_RATE_384000        MPEG_BIT_RATE_INDEX_12
#define MPEG_MP1_BIT_RATE_416000        MPEG_BIT_RATE_INDEX_13
#define MPEG_MP1_BIT_RATE_448000        MPEG_BIT_RATE_INDEX_14

#define MPEG_MP2_BIT_RATE_32000         MPEG_BIT_RATE_INDEX_1
#define MPEG_MP2_BIT_RATE_48000         MPEG_BIT_RATE_INDEX_2
#define MPEG_MP2_BIT_RATE_56000         MPEG_BIT_RATE_INDEX_3
#define MPEG_MP2_BIT_RATE_64000         MPEG_BIT_RATE_INDEX_4
#define MPEG_MP2_BIT_RATE_80000         MPEG_BIT_RATE_INDEX_5
#define MPEG_MP2_BIT_RATE_96000         MPEG_BIT_RATE_INDEX_6
#define MPEG_MP2_BIT_RATE_112000        MPEG_BIT_RATE_INDEX_7
#define MPEG_MP2_BIT_RATE_128000        MPEG_BIT_RATE_INDEX_8
#define MPEG_MP2_BIT_RATE_160000        MPEG_BIT_RATE_INDEX_9
#define MPEG_MP2_BIT_RATE_192000        MPEG_BIT_RATE_INDEX_10
#define MPEG_MP2_BIT_RATE_224000        MPEG_BIT_RATE_INDEX_11
#define MPEG_MP2_BIT_RATE_256000        MPEG_BIT_RATE_INDEX_12
#define MPEG_MP2_BIT_RATE_320000        MPEG_BIT_RATE_INDEX_13
#define MPEG_MP2_BIT_RATE_384000        MPEG_BIT_RATE_INDEX_14

#define MPEG_MP3_BIT_RATE_32000         MPEG_BIT_RATE_INDEX_1
#define MPEG_MP3_BIT_RATE_40000         MPEG_BIT_RATE_INDEX_2
#define MPEG_MP3_BIT_RATE_48000         MPEG_BIT_RATE_INDEX_3
#define MPEG_MP3_BIT_RATE_56000         MPEG_BIT_RATE_INDEX_4
#define MPEG_MP3_BIT_RATE_64000         MPEG_BIT_RATE_INDEX_5
#define MPEG_MP3_BIT_RATE_80000         MPEG_BIT_RATE_INDEX_6
#define MPEG_MP3_BIT_RATE_96000         MPEG_BIT_RATE_INDEX_7
#define MPEG_MP3_BIT_RATE_112000        MPEG_BIT_RATE_INDEX_8
#define MPEG_MP3_BIT_RATE_128000        MPEG_BIT_RATE_INDEX_9
#define MPEG_MP3_BIT_RATE_160000        MPEG_BIT_RATE_INDEX_10
#define MPEG_MP3_BIT_RATE_192000        MPEG_BIT_RATE_INDEX_11
#define MPEG_MP3_BIT_RATE_224000        MPEG_BIT_RATE_INDEX_12
#define MPEG_MP3_BIT_RATE_256000        MPEG_BIT_RATE_INDEX_13
#define MPEG_MP3_BIT_RATE_320000        MPEG_BIT_RATE_INDEX_14

#define MPEG_BIT_RATE_FREE              MPEG_BIT_RATE_INDEX_0

#define MPEG_GET_BITRATE(a) \
	((uint16_t)(a).bitrate1 << 8 | (a).bitrate2)
#define MPEG_SET_BITRATE(a, b) \
	do { \
		(a).bitrate1 = ((b) >> 8) & 0x7f; \
		(a).bitrate2 = (b) & 0xff; \
	} while (0)
#define MPEG_INIT_BITRATE(b) \
	.bitrate1 = ((b) >> 8) & 0x7f, \
	.bitrate2 = (b) & 0xff,

#define AAC_OBJECT_TYPE_MPEG2_AAC_LC    0x80
#define AAC_OBJECT_TYPE_MPEG4_AAC_LC    0x40
#define AAC_OBJECT_TYPE_MPEG4_AAC_LTP   0x20
#define AAC_OBJECT_TYPE_MPEG4_AAC_SCA   0x10

#define AAC_SAMPLING_FREQ_8000          0x0800
#define AAC_SAMPLING_FREQ_11025         0x0400
#define AAC_SAMPLING_FREQ_12000         0x0200
#define AAC_SAMPLING_FREQ_16000         0x0100
#define AAC_SAMPLING_FREQ_22050         0x0080
#define AAC_SAMPLING_FREQ_24000         0x0040
#define AAC_SAMPLING_FREQ_32000         0x0020
#define AAC_SAMPLING_FREQ_44100         0x0010
#define AAC_SAMPLING_FREQ_48000         0x0008
#define AAC_SAMPLING_FREQ_64000         0x0004
#define AAC_SAMPLING_FREQ_88200         0x0002
#define AAC_SAMPLING_FREQ_96000         0x0001

#define AAC_CHANNELS_1                  0x02
#define AAC_CHANNELS_2                  0x01

#define AAC_GET_BITRATE(a) \
	((a).bitrate1 << 16 | (a).bitrate2 << 8 | (a).bitrate3)
#define AAC_GET_FREQUENCY(a) \
	((a).frequency1 << 4 | (a).frequency2)

#define AAC_SET_BITRATE(a, b) \
	do { \
		(a).bitrate1 = ((b) >> 16) & 0x7f; \
		(a).bitrate2 = ((b) >> 8) & 0xff; \
		(a).bitrate3 = (b) & 0xff; \
	} while (0)
#define AAC_SET_FREQUENCY(a, f) \
	do { \
		(a).frequency1 = ((f) >> 4) & 0xff; \
		(a).frequency2 = (f) & 0x0f; \
	} while (0)

#define AAC_INIT_BITRATE(b) \
	.bitrate1 = ((b) >> 16) & 0x7f, \
	.bitrate2 = ((b) >> 8) & 0xff, \
	.bitrate3 = (b) & 0xff,
#define AAC_INIT_FREQUENCY(f) \
	.frequency1 = ((f) >> 4) & 0xff, \
	.frequency2 = (f) & 0x0f,

#define ATRAC_CHANNEL_MODE_MONO         0x04
#define ATRAC_CHANNEL_MODE_DUAL_CHANNEL 0x02
#define ATRAC_CHANNEL_MODE_JOINT_STEREO 0x01

#define ATRAC_SAMPLING_FREQ_44100       0x02
#define ATRAC_SAMPLING_FREQ_48000       0x01

#define ATRAC_GET_BITRATE(a) \
	((a).bitrate1 << 16 | (a).bitrate2 << 8 | (a).bitrate3)
#define ATRAC_GET_MAX_SUL(a) \
	((a).max_sul1 << 8 | (a).max_sul2)

#define APTX_VENDOR_ID                  BT_COMPID_APT
#define APTX_CODEC_ID                   0x0001

#define APTX_CHANNEL_MODE_MONO          0x01
#define APTX_CHANNEL_MODE_STEREO        0x02
#define APTX_CHANNEL_MODE_TWS           0x08

#define APTX_SAMPLING_FREQ_16000        0x08
#define APTX_SAMPLING_FREQ_32000        0x04
#define APTX_SAMPLING_FREQ_44100        0x02
#define APTX_SAMPLING_FREQ_48000        0x01

#define FASTSTREAM_VENDOR_ID            BT_COMPID_QUALCOMM_TECH_INTL
#define FASTSTREAM_CODEC_ID             0x0001

#define FASTSTREAM_DIRECTION_VOICE      0x2
#define FASTSTREAM_DIRECTION_MUSIC      0x1

#define FASTSTREAM_SAMPLING_FREQ_MUSIC_44100  0x2
#define FASTSTREAM_SAMPLING_FREQ_MUSIC_48000  0x1

#define FASTSTREAM_SAMPLING_FREQ_VOICE_16000  0x2

#define APTX_LL_VENDOR_ID               BT_COMPID_QUALCOMM_TECH_INTL
#define APTX_LL_CODEC_ID                0x0002

/**
 * Default parameters for aptX LL (Sprint) encoder */
#define APTX_LL_TARGET_CODEC_LEVEL      180  /* target codec buffer level */
#define APTX_LL_INITIAL_CODEC_LEVEL     360  /* initial codec buffer level */
#define APTX_LL_SRA_MAX_RATE            50   /* x/10000 = 0.005 SRA rate */
#define APTX_LL_SRA_AVG_TIME            1    /* SRA averaging time = 1s */
#define APTX_LL_GOOD_WORKING_LEVEL      180  /* good working buffer level */

#define APTX_LL_GET_TARGET_CODEC_LEVEL(a) \
	((a).target_codec_level1 << 8 | (a).target_codec_level2)
#define APTX_LL_GET_INITIAL_CODEC_LEVEL(a) \
	((a).initial_codec_level1 << 8 | (a).initial_codec_level2)
#define APTX_LL_GET_GOOD_WORKING_LEVEL(a) \
	((a).good_working_level1 << 8 | (a).good_working_level2)

#define APTX_HD_VENDOR_ID               BT_COMPID_QUALCOMM_TECH
#define APTX_HD_CODEC_ID                0x0024

#define APTX_TWS_VENDOR_ID              BT_COMPID_QUALCOMM_TECH
#define APTX_TWS_CODEC_ID               0x0025

#define APTX_AD_VENDOR_ID               BT_COMPID_QUALCOMM_TECH
#define APTX_AD_CODEC_ID                0x00ad

#define LC3PLUS_VENDOR_ID               BT_COMPID_FRAUNHOFER_IIS
#define LC3PLUS_CODEC_ID                0x0001

#define LC3PLUS_FRAME_DURATION_100      0x04
#define LC3PLUS_FRAME_DURATION_050      0x02
#define LC3PLUS_FRAME_DURATION_025      0x01

#define LC3PLUS_CHANNELS_1              0x80
#define LC3PLUS_CHANNELS_2              0x40

#define LC3PLUS_SAMPLING_FREQ_48000     0x0100
#define LC3PLUS_SAMPLING_FREQ_96000     0x0080

#define LC3PLUS_GET_FREQUENCY(a) \
	((a).frequency1 << 8 | (a).frequency2)
#define LC3PLUS_SET_FREQUENCY(a, f) \
	do { \
		(a).frequency1 = ((f) >> 8) & 0xff; \
		(a).frequency2 = (f) & 0xff; \
	} while (0)
#define LC3PLUS_INIT_FREQUENCY(f) \
	.frequency1 = ((f) >> 8) & 0xff, \
	.frequency2 = (f) & 0xff,

#define LDAC_VENDOR_ID                  BT_COMPID_SONY
#define LDAC_CODEC_ID                   0x00aa

#define LDAC_SAMPLING_FREQ_44100        0x20
#define LDAC_SAMPLING_FREQ_48000        0x10
#define LDAC_SAMPLING_FREQ_88200        0x08
#define LDAC_SAMPLING_FREQ_96000        0x04
#define LDAC_SAMPLING_FREQ_176400       0x02
#define LDAC_SAMPLING_FREQ_192000       0x01

#define LDAC_CHANNEL_MODE_MONO          0x04
#define LDAC_CHANNEL_MODE_DUAL          0x02
#define LDAC_CHANNEL_MODE_STEREO        0x01

#define LHDC_VENDOR_ID                  BT_COMPID_SAVITECH
#define LHDC_CODEC_ID                   0x4C32

#define LHDC_LL_VENDOR_ID               BT_COMPID_SAVITECH
#define LHDC_LL_CODEC_ID                0x4C4C

#define LHDC_V1_VENDOR_ID               BT_COMPID_SAVITECH
#define LHDC_V1_CODEC_ID                0x484C

#define LHDC_BIT_DEPTH_16               0x02
#define LHDC_BIT_DEPTH_24               0x01

#define LHDC_SAMPLING_FREQ_44100        0x08
#define LHDC_SAMPLING_FREQ_48000        0x04
#define LHDC_SAMPLING_FREQ_88200        0x02
#define LHDC_SAMPLING_FREQ_96000        0x01

#define LHDC_MAX_BIT_RATE_400K          0x02
#define LHDC_MAX_BIT_RATE_500K          0x01
#define LHDC_MAX_BIT_RATE_900K          0x00

#define LHDC_CH_SPLIT_MODE_NONE         0x01
#define LHDC_CH_SPLIT_MODE_TWS          0x02
#define LHDC_CH_SPLIT_MODE_TWS_PLUS     0x04

#define SAMSUNG_HD_VENDOR_ID            BT_COMPID_SAMSUNG_ELEC
#define SAMSUNG_HD_CODEC_ID             0x0102

#define SAMSUNG_SC_VENDOR_ID            BT_COMPID_SAMSUNG_ELEC
#define SAMSUNG_SC_CODEC_ID             0x0103

typedef struct {
	uint8_t vendor_id4;
	uint8_t vendor_id3;
	uint8_t vendor_id2;
	uint8_t vendor_id1;
	uint8_t codec_id2;
	uint8_t codec_id1;
} __attribute__ ((packed)) a2dp_vendor_codec_t;

#define A2DP_GET_VENDOR_ID(a) ( \
		(((uint32_t)(a).vendor_id4) <<  0) | \
		(((uint32_t)(a).vendor_id3) <<  8) | \
		(((uint32_t)(a).vendor_id2) << 16) | \
		(((uint32_t)(a).vendor_id1) << 24) \
	)
#define A2DP_GET_CODEC_ID(a) \
	((a).codec_id2 | (((uint16_t)(a).codec_id1) << 8))
#define A2DP_SET_VENDOR_ID_CODEC_ID(v, c) { \
		.vendor_id4 = (((v) >>  0) & 0xff), \
		.vendor_id3 = (((v) >>  8) & 0xff), \
		.vendor_id2 = (((v) >> 16) & 0xff), \
		.vendor_id1 = (((v) >> 24) & 0xff), \
		.codec_id2 = (((c) >> 0) & 0xff), \
		.codec_id1 = (((c) >> 8) & 0xff), \
	}

#if defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
	__BYTE_ORDER == __LITTLE_ENDIAN

typedef struct {
	uint8_t channel_mode:4;
	uint8_t frequency:4;
	uint8_t allocation_method:2;
	uint8_t subbands:2;
	uint8_t block_length:4;
	uint8_t min_bitpool;
	uint8_t max_bitpool;
} __attribute__ ((packed)) a2dp_sbc_t;

typedef struct {
	uint8_t channel_mode:4;
	uint8_t crc:1;
	uint8_t layer:3;
	uint8_t frequency:6;
	uint8_t mpf:1;
	uint8_t rfa:1;
	uint8_t bitrate1:7;
	uint8_t vbr:1;
	uint8_t bitrate2;
} __attribute__ ((packed)) a2dp_mpeg_t;

typedef struct {
	uint8_t object_type;
	uint8_t frequency1;
	uint8_t rfa:2;
	uint8_t channels:2;
	uint8_t frequency2:4;
	uint8_t bitrate1:7;
	uint8_t vbr:1;
	uint8_t bitrate2;
	uint8_t bitrate3;
} __attribute__ ((packed)) a2dp_aac_t;

typedef struct {
	uint8_t rfa1:2;
	uint8_t channel_mode:3;
	uint8_t version:3;
	uint8_t bitrate1:3;
	uint8_t vbr:1;
	uint8_t frequency:2;
	uint8_t rfa2:2;
	uint8_t bitrate2;
	uint8_t bitrate3;
	uint8_t max_sul1;
	uint8_t max_sul2;
	uint8_t rfa3;
} __attribute__ ((packed)) a2dp_atrac_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t channel_mode:4;
	uint8_t frequency:4;
} __attribute__ ((packed)) a2dp_aptx_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t direction;
	uint8_t frequency_music:4;
	uint8_t frequency_voice:4;
} __attribute__ ((packed)) a2dp_faststream_t;

typedef struct {
	a2dp_aptx_t aptx;
	uint8_t bidirect_link:1;
	uint8_t has_new_caps:1;
	uint8_t reserved:6;
} __attribute__ ((packed)) a2dp_aptx_ll_t;

typedef struct {
	a2dp_aptx_t aptx;
	uint32_t rfa;
} __attribute__ ((packed)) a2dp_aptx_hd_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t rfa:4;
	uint8_t frame_duration:4;
	uint8_t channels;
	uint8_t frequency1;
	uint8_t frequency2;
} __attribute__ ((packed)) a2dp_lc3plus_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t frequency:6;
	uint8_t rfa1:2;
	uint8_t channel_mode:3;
	uint8_t rfa2:5;
} __attribute__ ((packed)) a2dp_ldac_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t frequency:4;
	uint8_t bit_depth:2;
	uint8_t rfa1:2;
	uint8_t version:4;
	uint8_t max_bit_rate:3;
	uint8_t low_latency:1;
	uint8_t ch_split_mode:4;
	uint8_t rfa2:4;
} __attribute__ ((packed)) a2dp_lhdc_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t frequency:4;
	uint8_t bit_depth:2;
	uint8_t ch_separation:1;
	uint8_t rfa:1;
} __attribute__ ((packed)) a2dp_lhdc_v1_t;

#elif defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
	__BYTE_ORDER == __BIG_ENDIAN

typedef struct {
	uint8_t frequency:4;
	uint8_t channel_mode:4;
	uint8_t block_length:4;
	uint8_t subbands:2;
	uint8_t allocation_method:2;
	uint8_t min_bitpool;
	uint8_t max_bitpool;
} __attribute__ ((packed)) a2dp_sbc_t;

typedef struct {
	uint8_t layer:3;
	uint8_t crc:1;
	uint8_t channel_mode:4;
	uint8_t rfa:1;
	uint8_t mpf:1;
	uint8_t frequency:6;
	uint8_t vbr:1;
	uint8_t bitrate1:7;
	uint8_t bitrate2;
} __attribute__ ((packed)) a2dp_mpeg_t;

typedef struct {
	uint8_t object_type;
	uint8_t frequency1;
	uint8_t frequency2:4;
	uint8_t channels:2;
	uint8_t rfa:2;
	uint8_t vbr:1;
	uint8_t bitrate1:7;
	uint8_t bitrate2;
	uint8_t bitrate3;
} __attribute__ ((packed)) a2dp_aac_t;

typedef struct {
	uint8_t version:3;
	uint8_t channel_mode:3;
	uint8_t rfa1:2;
	uint8_t rfa2:2;
	uint8_t frequency:2;
	uint8_t vbr:1;
	uint8_t bitrate1:3;
	uint8_t bitrate2;
	uint8_t bitrate3;
	uint8_t max_sul1;
	uint8_t max_sul2;
	uint8_t rfa3;
} __attribute__ ((packed)) a2dp_atrac_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t frequency:4;
	uint8_t channel_mode:4;
} __attribute__ ((packed)) a2dp_aptx_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t direction;
	uint8_t frequency_voice:4;
	uint8_t frequency_music:4;
} __attribute__ ((packed)) a2dp_faststream_t;

typedef struct {
	a2dp_aptx_t aptx;
	uint8_t reserved:6;
	uint8_t has_new_caps:1;
	uint8_t bidirect_link:1;
} __attribute__ ((packed)) a2dp_aptx_ll_t;

typedef struct {
	a2dp_aptx_t aptx;
	uint32_t rfa;
} __attribute__ ((packed)) a2dp_aptx_hd_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t frame_duration:4;
	uint8_t rfa:4;
	uint8_t channels;
	uint8_t frequency1;
	uint8_t frequency2;
} __attribute__ ((packed)) a2dp_lc3plus_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t rfa1:2;
	uint8_t frequency:6;
	uint8_t rfa2:5;
	uint8_t channel_mode:3;
} __attribute__ ((packed)) a2dp_ldac_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t rfa1:2;
	uint8_t bit_depth:2;
	uint8_t frequency:4;
	uint8_t low_latency:1;
	uint8_t max_bit_rate:3;
	uint8_t version:4;
	uint8_t rfa2:4;
	uint8_t ch_split_mode:4;
} __attribute__ ((packed)) a2dp_lhdc_t;

typedef struct {
	a2dp_vendor_codec_t info;
	uint8_t rfa:1;
	uint8_t ch_separation:1;
	uint8_t bit_depth:2;
	uint8_t frequency:4;
} __attribute__ ((packed)) a2dp_lhdc_v1_t;

#else
# error "Unknown byte order"
#endif

typedef struct {
	a2dp_aptx_ll_t aptx_ll;
	uint8_t reserved;
	uint8_t target_codec_level2;
	uint8_t target_codec_level1;
	uint8_t initial_codec_level2;
	uint8_t initial_codec_level1;
	uint8_t sra_max_rate;
	uint8_t sra_avg_time;
	uint8_t good_working_level2;
	uint8_t good_working_level1;
} __attribute__ ((packed)) a2dp_aptx_ll_new_t;

typedef union {
	a2dp_sbc_t sbc;
	a2dp_mpeg_t mpeg;
	a2dp_aac_t aac;
	a2dp_atrac_t atrac;
	a2dp_aptx_t aptx;
	a2dp_faststream_t faststream;
	a2dp_aptx_ll_t aptx_ll;
	a2dp_aptx_ll_new_t aptx_ll_new;
	a2dp_aptx_hd_t aptx_hd;
	a2dp_lc3plus_t lc3plus;
	a2dp_ldac_t ldac;
	a2dp_lhdc_t lhdc;
	a2dp_lhdc_v1_t lhdc_v1;
} a2dp_t;

uint16_t a2dp_codecs_codec_id_from_string(const char *alias);
const char *a2dp_codecs_codec_id_to_string(uint16_t codec_id);
const char *a2dp_codecs_get_canonical_name(const char *alias);

#endif
