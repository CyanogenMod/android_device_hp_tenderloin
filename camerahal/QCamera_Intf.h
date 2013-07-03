/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __QCAMERA_INTF_H__
#define __QCAMERA_INTF_H__

#include <stdint.h>
#include <pthread.h>
#include <inttypes.h>

#define PAD_TO_WORD(a)               (((a)+3)&~3)
#define PAD_TO_2K(a)                 (((a)+2047)&~2047)
#define PAD_TO_4K(a)                 (((a)+4095)&~4095)
#define PAD_TO_8K(a)                 (((a)+8191)&~8191)

#define CEILING32(X) (((X) + 0x0001F) & 0xFFFFFFE0)
#define CEILING16(X) (((X) + 0x000F) & 0xFFF0)
#define CEILING4(X)  (((X) + 0x0003) & 0xFFFC)
#define CEILING2(X)  (((X) + 0x0001) & 0xFFFE)

#define MAX_ROI 2
#define MAX_NUM_PARM 5
#define MAX_NUM_OPS 2
#define VIDEO_MAX_PLANES 8
#define MAX_SNAPSHOT_BUFFERS 5
#define MAX_EXP_BRACKETING_LENGTH 32

/* Exif Tag ID */
typedef uint32_t exif_tag_id_t;

/* Exif Info (opaque definition) */
struct exif_info_t;
typedef struct exif_info_t * exif_info_obj_t;

typedef enum {
  BACK_CAMERA,
  FRONT_CAMERA,
} cam_position_t;

typedef enum {
  CAM_CTRL_FAILED,        /* Failure in doing operation */
  CAM_CTRL_SUCCESS,       /* Operation Succeded */
  CAM_CTRL_INVALID_PARM,  /* Inavlid parameter provided */
  CAM_CTRL_NOT_SUPPORTED, /* Parameter/operation not supported */
  CAM_CTRL_ACCEPTED,      /* Parameter accepted */
  CAM_CTRL_MAX,
} cam_ctrl_status_t;

typedef enum {
  CAMERA_PREVIEW_MODE_SNAPSHOT,
  CAMERA_PREVIEW_MODE_MOVIE,
  CAMERA_PREVIEW_MODE_MOVIE_120FPS,
  CAMERA_MAX_PREVIEW_MODE
} cam_preview_mode_t;

typedef enum {
  CAMERA_YUV_420_NV12,
  CAMERA_YUV_420_NV21,
  CAMERA_YUV_420_NV21_ADRENO,
  CAMERA_BAYER_SBGGR10,
  CAMERA_RDI,
  CAMERA_YUV_420_YV12,
  CAMERA_YUV_422_NV16,
  CAMERA_YUV_422_NV61
} cam_format_t;

typedef enum {
  CAMERA_PAD_NONE,
  CAMERA_PAD_TO_WORD,   /*2 bytes*/
  CAMERA_PAD_TO_LONG_WORD, /*4 bytes*/
  CAMERA_PAD_TO_8, /*8 bytes*/
  CAMERA_PAD_TO_16, /*16 bytes*/

  CAMERA_PAD_TO_1K, /*1k bytes*/
  CAMERA_PAD_TO_2K, /*2k bytes*/
  CAMERA_PAD_TO_4K,
  CAMERA_PAD_TO_8K
} cam_pad_format_t;

typedef struct {
  uint16_t dispWidth;
  uint16_t dispHeight;
} cam_ctrl_disp_t;

typedef struct {
  int ext_mode;   /* preview, main, thumbnail, video, raw, etc */
  int frame_idx;  /* frame index */
  int fd;         /* origin fd */
  uint32_t size;
} mm_camera_frame_map_type;

typedef struct {
  int ext_mode;   /* preview, main, thumbnail, video, raw, etc */
  int frame_idx;  /* frame index */
} mm_camera_frame_unmap_type;

typedef enum {
  CAM_SOCK_MSG_TYPE_FD_MAPPING,
  CAM_SOCK_MSG_TYPE_FD_UNMAPPING,
  CAM_SOCK_MSG_TYPE_WDN_START,
  CAM_SOCK_MSG_TYPE_HIST_MAPPING,
  CAM_SOCK_MSG_TYPE_HIST_UNMAPPING,
  CAM_SOCK_MSG_TYPE_MAX
}mm_camera_socket_msg_type;

#define MM_MAX_WDN_NUM 2
typedef struct {
  unsigned long cookie;
  int num_frames;
  int ext_mode[MM_MAX_WDN_NUM];
  int frame_idx[MM_MAX_WDN_NUM];
} mm_camera_wdn_start_type;

typedef struct {
  mm_camera_socket_msg_type msg_type;
  union {
    mm_camera_frame_map_type frame_fd_map;
    mm_camera_frame_unmap_type frame_fd_unmap;
    mm_camera_wdn_start_type wdn_start;
  } payload;
} cam_sock_packet_t;

typedef enum {
  CAM_VIDEO_FRAME,
  CAM_SNAPSHOT_FRAME,
  CAM_PREVIEW_FRAME,
} cam_frame_type_t;

typedef enum {
  CAMERA_MODE_2D = (1<<0),
  CAMERA_MODE_3D = (1<<1),
  CAMERA_NONZSL_MODE = (1<<2),
  CAMERA_ZSL_MODE = (1<<3),
  CAMERA_MODE_MAX = CAMERA_ZSL_MODE,
} camera_mode_t;

typedef struct {
  int  modes_supported;
  int8_t camera_id;
  cam_position_t position;
  uint32_t sensor_mount_angle;
  //uint32_t sensor_Orientation;
  //struct fih_parameters_data parameters_data;
} camera_info_t;

typedef struct {
  camera_mode_t mode;
  int8_t camera_id;
  //camera_mode_t cammode;
} config_params_t;

typedef struct {
  uint32_t len;
  uint32_t y_offset;
  uint32_t cbcr_offset;
} cam_sp_len_offset_t;

typedef struct{
  uint32_t len;
  uint32_t offset;
} cam_mp_len_offset_t;

typedef struct {
  int num_planes;
  union {
    cam_sp_len_offset_t sp;
    cam_mp_len_offset_t mp[8];
  };
  uint32_t frame_len;
} cam_frame_len_offset_t;

typedef struct {
  uint32_t parm[MAX_NUM_PARM];
  uint32_t ops[MAX_NUM_OPS];
  uint8_t yuv_output;
  uint8_t jpeg_capture;
  uint32_t max_pict_width;
  uint32_t max_pict_height;
  uint32_t max_preview_width;
  uint32_t max_preview_height;
  //uint32_t max_video_width;
  //uint32_t max_video_height;
  uint32_t effect;
  camera_mode_t modes;
  //uint8_t preview_format;
  //uint32_t preview_sizes_cnt;
  //uint32_t thumb_sizes_cnt;
  //uint32_t video_sizes_cnt;
  //uint32_t hfr_sizes_cnt;
  //uint8_t vfe_output_enable;
  //uint8_t hfr_frame_skip;
  //uint32_t default_preview_width;
  //uint32_t default_preview_height;
  //uint32_t bestshot_reconfigure;
}cam_prop_t;

typedef struct {
  uint16_t video_width;         /* Video width seen by VFE could be different than orig. Ex. DIS */
  uint16_t video_height;        /* Video height seen by VFE */
  uint16_t picture_width;       /* Picture width seen by VFE */
  uint16_t picture_height;      /* Picture height seen by VFE */
  uint16_t display_width;       /* width of display */
  uint16_t display_height;      /* height of display */
  uint16_t orig_video_width;    /* original video width received */
  uint16_t orig_video_height;   /* original video height received */
  uint16_t orig_picture_dx;     /* original picture width received */
  uint16_t orig_picture_dy;     /* original picture height received */
  uint16_t ui_thumbnail_height; /* Just like orig_picture_dx */
  uint16_t ui_thumbnail_width;  /* Just like orig_picture_dy */
  uint16_t thumbnail_height;
  uint16_t thumbnail_width;
#if 0
  uint16_t orig_picture_width;
  uint16_t orig_picture_height;
  uint16_t orig_thumb_width;
  uint16_t orig_thumb_height;
#endif
  uint16_t raw_picture_height;
  uint16_t raw_picture_width;
#ifdef RDI_SUPPORT /* Introduced by commit 57495c560b62b460285225f2e8ac30b0e61199ee to CAF, not supported by us */
  uint16_t rdi0_height;
  uint16_t rdi0_width;
  uint16_t rdi1_height;
  uint16_t rdi1_width;
#endif
  uint32_t hjr_xtra_buff_for_bayer_filtering;
  cam_format_t    prev_format;
  cam_format_t    enc_format;
  cam_format_t    thumb_format;
  cam_format_t    main_img_format;
#ifdef RDI_SUPPORT
  cam_format_t    rdi0_format;
  cam_format_t    rdi1_format;
#endif
  cam_pad_format_t prev_padding_format;
#if 0
  cam_pad_format_t enc_padding_format;
  cam_pad_format_t thumb_padding_format;
  cam_pad_format_t main_padding_format;
#endif
  uint16_t display_luma_width;
  uint16_t display_luma_height;
  uint16_t display_chroma_width;
  uint16_t display_chroma_height;
  uint16_t video_luma_width;
  uint16_t video_luma_height;
  uint16_t video_chroma_width;
  uint16_t video_chroma_height;
  uint16_t thumbnail_luma_width;
  uint16_t thumbnail_luma_height;
  uint16_t thumbnail_chroma_width;
  uint16_t thumbnail_chroma_height;
  uint16_t main_img_luma_width;
  uint16_t main_img_luma_height;
  uint16_t main_img_chroma_width;
  uint16_t main_img_chroma_height;
#if 0
  int rotation;
  cam_frame_len_offset_t display_frame_offset;
  cam_frame_len_offset_t video_frame_offset;
  cam_frame_len_offset_t picture_frame_offset;
  cam_frame_len_offset_t thumb_frame_offset;
#endif
#ifdef RDI_SUPPORT
  uint32_t channel_interface_mask;
#endif
} cam_ctrl_dimension_t;

/* Add enumenrations at the bottom but before MM_CAMERA_PARM_MAX */
typedef enum {
    CAMERA_PARM_PICT_SIZE,
    CAMERA_PARM_ZOOM_RATIO,
    CAMERA_PARM_HISTOGRAM,
    CAMERA_PARM_DIMENSION,
    CAMERA_PARM_FPS,
    CAMERA_PARM_FPS_MODE, /*5*/
    CAMERA_PARM_EFFECT,
    CAMERA_PARM_EXPOSURE_COMPENSATION,
    CAMERA_PARM_EXPOSURE,
    CAMERA_PARM_SHARPNESS,
    CAMERA_PARM_CONTRAST, /*10*/
    CAMERA_PARM_SATURATION,
    CAMERA_PARM_BRIGHTNESS,
    CAMERA_PARM_WHITE_BALANCE,
    CAMERA_PARM_LED_MODE,
    CAMERA_PARM_ANTIBANDING, /*15*/
    CAMERA_PARM_ROLLOFF,
    CAMERA_PARM_CONTINUOUS_AF,
    CAMERA_PARM_FOCUS_RECT,
    CAMERA_PARM_AEC_ROI,
    CAMERA_PARM_AF_ROI, /*20*/
    CAMERA_PARM_HJR,
    CAMERA_PARM_ISO,
    CAMERA_PARM_BL_DETECTION,
    CAMERA_PARM_SNOW_DETECTION,
    CAMERA_PARM_BESTSHOT_MODE, /*25*/
    CAMERA_PARM_ZOOM,
    CAMERA_PARM_VIDEO_DIS,
    CAMERA_PARM_VIDEO_ROT,
    CAMERA_PARM_SCE_FACTOR,
    CAMERA_PARM_FD, /*30*/
    CAMERA_PARM_MODE,
    /* 2nd 32 bits */
    CAMERA_PARM_3D_FRAME_FORMAT,
    CAMERA_PARM_CAMERA_ID,
    CAMERA_PARM_CAMERA_INFO,
    CAMERA_PARM_PREVIEW_SIZE, /*35*/
    CAMERA_PARM_QUERY_FALSH4SNAP,
    CAMERA_PARM_FOCUS_DISTANCES,
    CAMERA_PARM_BUFFER_INFO,
    CAMERA_PARM_JPEG_ROTATION,
    CAMERA_PARM_JPEG_MAINIMG_QUALITY, /* 40 */
    CAMERA_PARM_JPEG_THUMB_QUALITY,
    CAMERA_PARM_ZSL_ENABLE,
    CAMERA_PARM_FOCAL_LENGTH,
    CAMERA_PARM_HORIZONTAL_VIEW_ANGLE,
    CAMERA_PARM_VERTICAL_VIEW_ANGLE, /* 45 */
    CAMERA_PARM_MCE,
    CAMERA_PARM_RESET_LENS_TO_INFINITY,
    CAMERA_PARM_SNAPSHOTDATA,
    CAMERA_PARM_HFR,
    CAMERA_PARM_REDEYE_REDUCTION, /* 50 */
    CAMERA_PARM_WAVELET_DENOISE,
    CAMERA_PARM_3D_DISPLAY_DISTANCE,
    CAMERA_PARM_3D_VIEW_ANGLE,
    CAMERA_PARM_PREVIEW_FORMAT,
    CAMERA_PARM_RDI_FORMAT,
    CAMERA_PARM_HFR_SIZE, /* 55 */
    CAMERA_PARM_3D_EFFECT,
    CAMERA_PARM_3D_MANUAL_CONV_RANGE,
    CAMERA_PARM_3D_MANUAL_CONV_VALUE,
    CAMERA_PARM_ENABLE_3D_MANUAL_CONVERGENCE,
    /* These are new parameters defined here */
    CAMERA_PARM_CH_IMAGE_FMT, /* 60 */       // mm_camera_ch_image_fmt_parm_t
    CAMERA_PARM_OP_MODE,             // camera state, sub state also
    CAMERA_PARM_SHARPNESS_CAP,       //
    CAMERA_PARM_SNAPSHOT_BURST_NUM,  // num shots per snapshot action
    CAMERA_PARM_LIVESHOT_MAIN,       // enable/disable full size live shot
    CAMERA_PARM_MAXZOOM, /* 65 */
    CAMERA_PARM_LUMA_ADAPTATION,     // enable/disable
    CAMERA_PARM_HDR,
    CAMERA_PARM_CROP,
    CAMERA_PARM_MAX_PICTURE_SIZE,
    CAMERA_PARM_MAX_PREVIEW_SIZE, /* 70 */
    CAMERA_PARM_ASD_ENABLE,
    CAMERA_PARM_RECORDING_HINT,
    CAMERA_PARM_CAF_ENABLE,
    CAMERA_PARM_FULL_LIVESHOT,
    CAMERA_PARM_DIS_ENABLE, /* 75 */
    CAMERA_PARM_AEC_LOCK,
    CAMERA_PARM_AWB_LOCK,
    CAMERA_PARM_AF_MTR_AREA,
    CAMERA_PARM_AEC_MTR_AREA,
    CAMERA_PARM_LOW_POWER_MODE,
    CAMERA_PARM_MAX_HFR_MODE, /* 80 */
    CAMERA_PARM_MAX_VIDEO_SIZE,
    CAMERA_PARM_DEF_PREVIEW_SIZES,
    CAMERA_PARM_DEF_VIDEO_SIZES,
    CAMERA_PARM_DEF_THUMB_SIZES,
    CAMERA_PARM_DEF_HFR_SIZES,
    CAMERA_PARM_PREVIEW_SIZES_CNT,
    CAMERA_PARM_VIDEO_SIZES_CNT,
    CAMERA_PARM_THUMB_SIZES_CNT,
    CAMERA_PARM_HFR_SIZES_CNT,
    CAMERA_PARM_GRALLOC_USAGE,
    CAMERA_PARM_VFE_OUTPUT_ENABLE, //to check whether both oputputs are
    CAMERA_PARM_DEFAULT_PREVIEW_WIDTH,
    CAMERA_PARM_DEFAULT_PREVIEW_HEIGHT,
    CAMERA_PARM_FOCUS_MODE,
    CAMERA_PARM_HFR_FRAME_SKIP,
    CAMERA_PARM_CH_INTERFACE,
    //or single output enabled to differentiate 7x27a with others
    CAMERA_PARM_BESTSHOT_RECONFIGURE,
    CAMERA_MAX_NUM_FACES_DECT,
    CAMERA_PARM_FPS_RANGE,
    CAMERA_PARM_MAX
} mm_camera_parm_type_t;

typedef struct {
  int pic_width;
  int pic_hight;
  int prev_width;
  int prev_hight;
  int rotation;
} cam_ctrl_set_dimension_data_t;

typedef enum {
  STREAM_IMAGE,
  STREAM_RAW,
  STREAM_IMAGE_AND_RAW,
  STREAM_RAW_AND_RAW,
  STREAM_MAX,
} mm_camera_channel_stream_info_t;

typedef enum {
  CAMERA_SET_PARM_DISPLAY_INFO,
  CAMERA_SET_PARM_DIMENSION,

  CAMERA_SET_PARM_ZOOM,
  CAMERA_SET_PARM_SENSOR_POSITION,
  CAMERA_SET_PARM_FOCUS_RECT,
  CAMERA_SET_PARM_LUMA_ADAPTATION,
  CAMERA_SET_PARM_CONTRAST,
  CAMERA_SET_PARM_BRIGHTNESS,
  CAMERA_SET_PARM_EXPOSURE_COMPENSATION,
  CAMERA_SET_PARM_SHARPNESS,
  CAMERA_SET_PARM_HUE,  /* 10 */
  CAMERA_SET_PARM_SATURATION,
  CAMERA_SET_PARM_EXPOSURE,
  CAMERA_SET_PARM_AUTO_FOCUS,
  CAMERA_SET_PARM_WB,
  CAMERA_SET_PARM_EFFECT,
  CAMERA_SET_PARM_FPS,
  CAMERA_SET_PARM_FLASH,
  CAMERA_SET_PARM_NIGHTSHOT_MODE,
  CAMERA_SET_PARM_REFLECT,
  CAMERA_SET_PARM_PREVIEW_MODE,  /* 20 */
  CAMERA_SET_PARM_ANTIBANDING,
  CAMERA_SET_PARM_RED_EYE_REDUCTION,
  CAMERA_SET_PARM_FOCUS_STEP,
  CAMERA_SET_PARM_EXPOSURE_METERING,
  CAMERA_SET_PARM_AUTO_EXPOSURE_MODE,
  CAMERA_SET_PARM_ISO,
  CAMERA_SET_PARM_BESTSHOT_MODE,
  CAMERA_SET_PARM_ENCODE_ROTATION,

  CAMERA_SET_PARM_PREVIEW_FPS,
  CAMERA_SET_PARM_AF_MODE,  /* 30 */
  CAMERA_SET_PARM_HISTOGRAM,
  CAMERA_SET_PARM_FLASH_STATE,
  CAMERA_SET_PARM_FRAME_TIMESTAMP,
  CAMERA_SET_PARM_STROBE_FLASH,
  CAMERA_SET_PARM_FPS_LIST,
  CAMERA_SET_PARM_HJR,
  CAMERA_SET_PARM_ROLLOFF,

  CAMERA_STOP_PREVIEW,
  CAMERA_START_PREVIEW,
  CAMERA_START_SNAPSHOT, /* 40 */
  CAMERA_START_RAW_SNAPSHOT,
  CAMERA_STOP_SNAPSHOT,
  CAMERA_EXIT,
  CAMERA_ENABLE_BSM,
  CAMERA_DISABLE_BSM,
  CAMERA_GET_PARM_ZOOM,
  CAMERA_GET_PARM_MAXZOOM,
  CAMERA_GET_PARM_ZOOMRATIOS,
  CAMERA_GET_PARM_AF_SHARPNESS,
  CAMERA_SET_PARM_LED_MODE, /* 50 */
  CAMERA_SET_MOTION_ISO,
  CAMERA_AUTO_FOCUS_CANCEL,
  CAMERA_GET_PARM_FOCUS_STEP,
  CAMERA_ENABLE_AFD,
  CAMERA_PREPARE_SNAPSHOT,
  CAMERA_SET_FPS_MODE,
  CAMERA_START_VIDEO,
  CAMERA_STOP_VIDEO,
  CAMERA_START_RECORDING,
  CAMERA_STOP_RECORDING, /* 60 */
  CAMERA_SET_VIDEO_DIS_PARAMS,
  CAMERA_SET_VIDEO_ROT_PARAMS,
  CAMERA_SET_PARM_AEC_ROI,
  CAMERA_SET_CAF,
  CAMERA_SET_PARM_BL_DETECTION_ENABLE,
  CAMERA_SET_PARM_SNOW_DETECTION_ENABLE,
  CAMERA_SET_PARM_STROBE_FLASH_MODE,
  CAMERA_SET_PARM_AF_ROI,
  CAMERA_START_LIVESHOT,
  CAMERA_SET_SCE_FACTOR, /* 70 */
  CAMERA_GET_CAPABILITIES,
  CAMERA_GET_PARM_DIMENSION,
  CAMERA_GET_PARM_LED_MODE,
  CAMERA_SET_PARM_FD,
  CAMERA_GET_PARM_3D_FRAME_FORMAT,
  CAMERA_QUERY_FLASH_FOR_SNAPSHOT,
  CAMERA_GET_PARM_FOCUS_DISTANCES,
  CAMERA_START_ZSL,
  CAMERA_STOP_ZSL,
  CAMERA_ENABLE_ZSL, /* 80 */
  CAMERA_GET_PARM_FOCAL_LENGTH,
  CAMERA_GET_PARM_HORIZONTAL_VIEW_ANGLE,
  CAMERA_GET_PARM_VERTICAL_VIEW_ANGLE,
  CAMERA_SET_PARM_WAVELET_DENOISE,
  CAMERA_SET_PARM_MCE,
  CAMERA_ENABLE_STEREO_CAM,
  CAMERA_SET_PARM_RESET_LENS_TO_INFINITY,
  CAMERA_GET_PARM_SNAPSHOTDATA,
  CAMERA_SET_PARM_HFR,
  CAMERA_SET_REDEYE_REDUCTION, /* 90 */
  CAMERA_SET_PARM_3D_DISPLAY_DISTANCE,
  CAMERA_SET_PARM_3D_VIEW_ANGLE,
  CAMERA_SET_PARM_3D_EFFECT,
  CAMERA_SET_PARM_PREVIEW_FORMAT,
  CAMERA_GET_PARM_3D_DISPLAY_DISTANCE, /* 95 */
  CAMERA_GET_PARM_3D_VIEW_ANGLE,
  CAMERA_GET_PARM_3D_EFFECT,
  CAMERA_GET_PARM_3D_MANUAL_CONV_RANGE,
  CAMERA_SET_PARM_3D_MANUAL_CONV_VALUE,
  CAMERA_ENABLE_3D_MANUAL_CONVERGENCE, /* 100 */
  CAMERA_SET_PARM_HDR,
  CAMERA_SET_ASD_ENABLE,
  CAMERA_POSTPROC_ABORT,
  CAMERA_SET_AEC_MTR_AREA,
  CAMERA_SET_AEC_LOCK,       /*105*/
  CAMERA_SET_AWB_LOCK,
  CAMERA_SET_RECORDING_HINT,
  CAMERA_SET_PARM_CAF,
  CAMERA_SET_FULL_LIVESHOT,
  CAMERA_SET_DIS_ENABLE,  /*110*/
  CAMERA_GET_PARM_MAX_HFR_MODE,
  CAMERA_SET_LOW_POWER_MODE,
  CAMERA_GET_PARM_DEF_PREVIEW_SIZES,
  CAMERA_GET_PARM_DEF_VIDEO_SIZES,
  CAMERA_GET_PARM_DEF_THUMB_SIZES, /*115*/
  CAMERA_GET_PARM_DEF_HFR_SIZES,
  CAMERA_GET_PARM_MAX_LIVESHOT_SIZE,
  CAMERA_GET_PARM_FPS_RANGE,
  CAMERA_SET_3A_CONVERGENCE,
  CAMERA_SET_PREVIEW_HFR, /*120*/
  CAMERA_GET_MAX_DIMENSION,
  CAMERA_GET_MAX_NUM_FACES_DECT,
  CAMERA_SET_CHANNEL_STREAM,
  CAMERA_GET_CHANNEL_STREAM,
  CAMERA_CTRL_PARM_MAX
} cam_ctrl_type;

typedef enum {
  CAMERA_ERROR_NO_MEMORY,
  CAMERA_ERROR_EFS_FAIL,                /* Low-level operation failed */
  CAMERA_ERROR_EFS_FILE_OPEN,           /* File already opened */
  CAMERA_ERROR_EFS_FILE_NOT_OPEN,       /* File not opened */
  CAMERA_ERROR_EFS_FILE_ALREADY_EXISTS, /* File already exists */
  CAMERA_ERROR_EFS_NONEXISTENT_DIR,     /* User directory doesn't exist */
  CAMERA_ERROR_EFS_NONEXISTENT_FILE,    /* User directory doesn't exist */
  CAMERA_ERROR_EFS_BAD_FILE_NAME,       /* Client specified invalid file/directory name*/
  CAMERA_ERROR_EFS_BAD_FILE_HANDLE,     /* Client specified invalid file/directory name*/
  CAMERA_ERROR_EFS_SPACE_EXHAUSTED,     /* Out of file system space */
  CAMERA_ERROR_EFS_OPEN_TABLE_FULL,     /* Out of open-file table slots                */
  CAMERA_ERROR_EFS_OTHER_ERROR,         /* Other error                                 */
  CAMERA_ERROR_CONFIG,
  CAMERA_ERROR_EXIF_ENCODE,
  CAMERA_ERROR_VIDEO_ENGINE,
  CAMERA_ERROR_IPL,
  CAMERA_ERROR_INVALID_FORMAT,
  CAMERA_ERROR_TIMEOUT,
  CAMERA_ERROR_ESD,
  CAMERA_ERROR_MAX
} camera_error_type;

typedef enum {
  CAMERA_SUCCESS = 0,
  CAMERA_INVALID_STATE,
  CAMERA_INVALID_PARM,
  CAMERA_INVALID_FORMAT,
  CAMERA_NO_SENSOR,
  CAMERA_NO_MEMORY,
  CAMERA_NOT_SUPPORTED,
  CAMERA_FAILED,
  CAMERA_INVALID_STAND_ALONE_FORMAT,
  CAMERA_MALLOC_FAILED_STAND_ALONE,
  CAMERA_RET_CODE_MAX
} camera_ret_code_type;

typedef enum {
  CAMERA_RAW,
  CAMERA_JPEG,
  CAMERA_PNG,
  CAMERA_YCBCR_ENCODE,
  CAMERA_ENCODE_TYPE_MAX
} camera_encode_type;

#if !defined FEATURE_CAMERA_ENCODE_PROPERTIES && defined FEATURE_CAMERA_V7
typedef enum {
  CAMERA_SNAPSHOT,
  CAMERA_RAW_SNAPSHOT
} camera_snapshot_type;
#endif /* nFEATURE_CAMERA_ENCODE_PROPERTIES && FEATURE_CAMERA_V7 */

typedef enum {
  /* YCbCr, each pixel is two bytes. Two pixels form a unit.
   * MSB is Y, LSB is CB for the first pixel and CR for the second pixel. */
  CAMERA_YCBCR,
#ifdef FEATURE_CAMERA_V7
  CAMERA_YCBCR_4_2_0,
  CAMERA_YCBCR_4_2_2,
  CAMERA_H1V1,
  CAMERA_H2V1,
  CAMERA_H1V2,
  CAMERA_H2V2,
  CAMERA_BAYER_8BIT,
  CAMERA_BAYER_10BIT,
#endif /* FEATURE_CAMERA_V7 */
  /* RGB565, each pixel is two bytes.
   * MS 5-bit is red, the next 6-bit is green. LS 5-bit is blue. */
  CAMERA_RGB565,
  /* RGB666, each pixel is four bytes.
   * MS 14 bits are zeros, the next 6-bit is red, then 6-bit of green.
   * LS 5-bit is blue. */
  CAMERA_RGB666,
  /* RGB444, each pixel is 2 bytes. The MS 4 bits are zeros, the next
   * 4 bits are red, the next 4 bits are green. The LS 4 bits are blue. */
  CAMERA_RGB444,
  /* Bayer, each pixel is 1 bytes. 2x2 pixels form a unit.
   * First line: first byte is blue, second byte is green.
   * Second line: first byte is green, second byte is red. */
  CAMERA_BAYER_BGGR,
  /* Bayer, each pixel is 1 bytes. 2x2 pixels form a unit.
   * First line: first byte is green, second byte is blue.
   * Second line: first byte is red, second byte is green. */
  CAMERA_BAYER_GBRG,
  /* Bayer, each pixel is 1 bytes. 2x2 pixels form a unit.
   * First line: first byte is green, second byte is red.
   * Second line: first byte is blue, second byte is green. */
  CAMERA_BAYER_GRBG,
  /* Bayer, each pixel is 1 bytes. 2x2 pixels form a unit.
   * First line: first byte is red, second byte is green.
   * Second line: first byte is green, second byte is blue. */
  CAMERA_BAYER_RGGB,
  /* RGB888, each pixel is 3 bytes. R is 8 bits, G is 8 bits,
   * B is 8 bits*/
  CAMERA_RGB888
} camera_format_type;

typedef enum {
  CAMERA_ORIENTATION_LANDSCAPE,
  CAMERA_ORIENTATION_PORTRAIT
} camera_orientation_type;

typedef enum {
  CAMERA_DESCRIPTION_STRING,
  CAMERA_USER_COMMENT_STRING,
  CAMERA_GPS_AREA_INFORMATION_STRING
} camera_string_type;

typedef enum {
  CAM_STATS_TYPE_HIST,
  CAM_STATS_TYPE_MAX
} camstats_type;

typedef struct {
  int32_t  buffer[256];       /* buffer to hold data */
  int32_t  max_value;
} camera_preview_histogram_info;

typedef struct {
  /* Format of the frame */
  camera_format_type format;

  /* For pre-V7, Width and height of the picture.
   * For V7:
   *   Snapshot:     thumbnail dimension
   *   Raw Snapshot: not applicable
   *   Preview:      not applicable
   */
  uint16_t dx;
  uint16_t dy;
  /* For pre_V7: For BAYER format, RAW data before scaling.
   * For V7:
   *   Snapshot:     Main image dimension
   *   Raw snapshot: raw image dimension
   *   Preview:      preview image dimension
   */
  uint16_t captured_dx;
  uint16_t captured_dy;
  /* it indicates the degree of clockwise rotation that should be
   * applied to obtain the exact view of the captured image. */
  uint16_t rotation;

#ifdef FEATURE_CAMERA_V7
  /* Preview:      not applicable
   * Raw shapshot: not applicable
   * Snapshot:     thumbnail image buffer
   */
  uint8_t *thumbnail_image;
#endif /* FEATURE_CAMERA_V7 */

  /* For pre-V7:
   *   Image buffer ptr
   * For V7:
   *   Preview: preview image buffer ptr
   *   Raw snapshot: Raw image buffer ptr
   *   Shapshot:     Main image buffer ptr
   */
  uint8_t  *buffer;
} camera_frame_type;

typedef struct {
  uint16_t  uMode;        // Input / Output AAC mode
  uint32_t  dwFrequency;  // Sampling Frequency
  uint16_t  uQuality;     // Audio Quality
  uint32_t  dwReserved;   // Reserved for future use
} camera_aac_encoding_info_type;

typedef enum {
  TIFF_DATA_BYTE = 1,
  TIFF_DATA_ASCII = 2,
  TIFF_DATA_SHORT = 3,
  TIFF_DATA_LONG = 4,
  TIFF_DATA_RATIONAL = 5,
  TIFF_DATA_UNDEFINED = 7,
  TIFF_DATA_SLONG = 9,
  TIFF_DATA_SRATIONAL = 10
} tiff_data_type;

typedef enum {
  ZERO_IFD = 0, /* This must match AEECamera.h */
  FIRST_IFD,
  EXIF_IFD,
  GPS_IFD,
  INTEROP_IFD,
  DEFAULT_IFD
} camera_ifd_type;

typedef struct {
  uint16_t tag_id;
  camera_ifd_type ifd;
  tiff_data_type type;
  uint16_t count;
  void     *value;
} camera_exif_tag_type;

typedef struct {
  /* What is the ID for this sensor */
  uint16_t sensor_id;
  /* Sensor model number, null terminated, trancate to 31 characters */
  char sensor_model[32];
  /* Width and height of the sensor */
  uint16_t sensor_width;
  uint16_t sensor_height;
  /* Frames per second */
  uint16_t fps;
  /* Whether the device driver can sense when sensor is rotated */
  int8_t  sensor_rotation_sensing;
  /* How the sensor are installed */
  uint16_t default_rotation;
  camera_orientation_type default_orientation;
  /*To check antibanding support */
  int8_t  support_auto_antibanding;
} camera_info_type;

typedef struct {
  int32_t                quality;
  camera_encode_type     format;
  int32_t                file_size;
} camera_encode_properties_type;

typedef enum {
  CAMERA_DEVICE_MEM,
  CAMERA_DEVICE_EFS,
  CAMERA_DEVICE_MAX
} camera_device_type;

#define MAX_JPEG_ENCODE_BUF_NUM 4
#define MAX_JPEG_ENCODE_BUF_LEN (1024*8)

typedef struct {
  uint32_t buf_len;/* Length of each buffer */
  uint32_t used_len;
  int8_t   valid;
  uint8_t  *buffer;
} camera_encode_mem_type;

typedef struct {
  camera_device_type     device;
#ifndef FEATURE_CAMERA_ENCODE_PROPERTIES
  int32_t                quality;
  camera_encode_type     format;
#endif /* nFEATURE_CAMERA_ENCODE_PROPERTIES */
  int32_t                encBuf_num;
  camera_encode_mem_type encBuf[MAX_JPEG_ENCODE_BUF_NUM];
} camera_handle_mem_type;

#ifdef FEATURE_EFS
typedef struct {
  camera_device_type     device;
  #ifndef FEATURE_CAMERA_ENCODE_PROPERTIES
  int32_t                quality;
  camera_encode_type     format;
  #endif /* nFEATURE_CAMERA_ENCODE_PROPERTIES */
  char                   filename[FS_FILENAME_MAX_LENGTH_P];
} camera_handle_efs_type;
#endif /* FEATURE_EFS */

typedef enum {
  CAMERA_PARM_FADE_OFF,
  CAMERA_PARM_FADE_IN,
  CAMERA_PARM_FADE_OUT,
  CAMERA_PARM_FADE_IN_OUT,
  CAMERA_PARM_FADE_MAX
} camera_fading_type;

typedef union {
  camera_device_type      device;
  camera_handle_mem_type  mem;
} camera_handle_type;

typedef enum {
  CAMERA_AUTO_FOCUS,
  CAMERA_MANUAL_FOCUS
} camera_focus_e_type;

/* AEC: Frame average weights the whole preview window equally
   AEC: Center Weighted weights the middle X percent of the window
   X percent compared to the rest of the frame
   AEC: Spot metering weights the very center regions 100% and
   discounts other areas                                        */
typedef enum {
  CAMERA_AEC_FRAME_AVERAGE,
  CAMERA_AEC_CENTER_WEIGHTED,
  CAMERA_AEC_SPOT_METERING,
  CAMERA_AEC_MAX_MODES
} camera_auto_exposure_mode_type;

/* Auto focus mode, used for CAMERA_PARM_AF_MODE */
typedef enum {
  AF_MODE_UNCHANGED = -1,
  AF_MODE_NORMAL    = 0,
  AF_MODE_MACRO,
  AF_MODE_AUTO,
  AF_MODE_MAX
} isp3a_af_mode_t;

/* VFE Focus Region:
 *  VFE input image dimension
 * VFE Focus Window:
 *  A rectangle in the VFE Focus Region. VFE Focus Window
 *  area must not exceed one quarter of the area of VFE Focus
 *  Region.
 * Display Focus Region:
 *  Diplay dimensions (not LCD dimensions)
 * Display Focus Window:
 *  A rectangle in Display Focus Region
 *  Focus Window Freedom: Movement of Focus Window in x and y direction.
 */

typedef  struct {
  /* Focus Window dimensions, could be negative. */
  int16_t x;
  int16_t y;
  int16_t dx;
  int16_t dy;

  /* Focus Window Freedom granularity in x-direction */
  int16_t dx_step_size;

  /* Focus Window Freedom granularity in y-direction */
  int16_t dy_step_size;

  /*  Focus Window can only move within this Focus Region and
   *  the maximum Focus Window area must not exceed one quarter of the
   *  Focus Region.
   */
  int16_t min_x;
  int16_t min_y;
  int16_t max_x;
  int16_t max_y;
} camera_focus_window_type;

typedef unsigned int Offline_Input_PixelSizeType;
typedef uint16_t Offline_Snapshot_PixelSizeType;
typedef uint16_t Offline_Thumbnail_PixelSizeType;
typedef uint16_t Offline_NumFragments_For_Input;
typedef uint16_t Offline_NumFragments_For_Output;

typedef  struct {
  /* Focus Window dimensions */
  int16_t x_upper_left;
  int16_t y_upper_left;
  int16_t width;
  int16_t height;
} camera_focus_rectangle_dimensions_type;

typedef  struct {
  int16_t focus_window_count;
  camera_focus_rectangle_dimensions_type *windows_list;
} camera_focus_window_rectangles_type;

typedef enum Offline_InputFormatType {
  CAMERA_BAYER_G_B,
  CAMERA_BAYER_B_G,
  CAMERA_BAYER_G_R,
  CAMERA_BAYER_R_G,
  CAMERA_YCbCr_Y_Cb_Y_Cr,
  CAMERA_YCbCr_Y_Cr_Y_Cb,
  CAMERA_YCbCr_Cb_Y_Cr_Y,
  CAMERA_YCbCr_Cr_Y_Cb_Y,
  CAMERA_YCbCr_4_2_2_linepacked,
  CAMERA_YCbCr_4_2_0_linepacked,
  CAMERA_NumberOf_InputFormatType   /* Used for count purposes only */
} Offline_InputFormatType;

typedef enum Offline_YCbCr_InputCositingType {
  CAMERA_CHROMA_NOT_COSITED,
  CAMERA_CHROMA_COSITED,
  CAMERA_NumberOf_YCbCr_InputCositingType   /* Used for count purposes only */
} Offline_YCbCr_InputCositingType;

typedef enum Offline_Input_PixelDataWidthType {
  CAMERA_8Bit,
  CAMERA_10Bit,
  CAMERA_NumberOf_PixelDataWidthType /* Used for count purposes only */
} Offline_Input_PixelDataWidthType;

typedef struct OfflineInputConfigurationType {
  Offline_YCbCr_InputCositingType  YCbCrCositing    ;
  Offline_InputFormatType          format           ;
  Offline_Input_PixelDataWidthType dataWidth        ;
  Offline_Input_PixelSizeType      height           ;
  Offline_Input_PixelSizeType      width            ;
  Offline_Thumbnail_PixelSizeType  thumbnail_width  ;
  Offline_Thumbnail_PixelSizeType  thumbnail_height ;
  Offline_Snapshot_PixelSizeType   snapshot_width   ;
  Offline_Snapshot_PixelSizeType   snapshot_height  ;
  char*                            file_name        ;
  Offline_NumFragments_For_Input   input_fragments  ;
  Offline_NumFragments_For_Output  output_fragments ;
} OfflineInputConfigurationType;

/* Enum Type for bracketing support */
typedef enum {
  CAMERA_BRACKETING_OFF,
  CAMERA_BRACKETING_EXPOSURE,
  CAMERA_BRACKETING_MAX
} camera_bracketing_mode_type;

typedef struct la_config {
  /* Pointer in input image - input */
  uint8_t *data_in;
  /* Luma re-mapping curve - output */
  uint16_t *la_curve;
  /* Detect - if 1 image needs LA , if 0 does not - output */
  int16_t detect;
  /* input image size - input */
  uint32_t size;
  /* Old gamma correction LUT - input */
  uint8_t *gamma;
  /* New gamma correction LUT - output */
  uint8_t *gamma_new;
  /* Chroma scale value - output */
  uint32_t chroma_scale_Q20;

  /* Detection related parameters - inputs*/
  /****************************************/
  uint8_t low_range;
  uint16_t low_perc_Q11;
  uint8_t high_range;
  uint16_t high_perc_Q11;

  /* Capping of the histogram - input*/
  uint16_t cap_Q10;

  /* The range to test if diffusion shift is needed - input*/
  uint8_t diff_range;
  /* The percentile to test if diffusion shift is needed - input*/
  uint16_t diff_tail_threshold_Q8;

  /* Scale how much to include contrast tranform
     0 - no contrast transform, 32 - all is contrast transform   - input*/
  uint16_t scale;

  /* Cap high final re-mapping function - input */
  uint16_t cap_high_Q2;
  /* Cap low final re-mapping function - input */
  uint16_t cap_low_Q2;
  /* Number of itterations for the diffusion */
  uint16_t numIt;

} la_config;

#if defined CAMERA_ANTIBANDING_OFF
#undef CAMERA_ANTIBANDING_OFF
#endif

#if defined CAMERA_ANTIBANDING_60HZ
#undef CAMERA_ANTIBANDING_60HZ
#endif

#if defined CAMERA_ANTIBANDING_50HZ
#undef CAMERA_ANTIBANDING_50HZ
#endif

#if defined CAMERA_ANTIBANDING_AUTO
#undef CAMERA_ANTIBANDING_AUTO
#endif

typedef enum {
  CAMERA_ANTIBANDING_OFF,
  CAMERA_ANTIBANDING_60HZ,
  CAMERA_ANTIBANDING_50HZ,
  CAMERA_ANTIBANDING_AUTO,
  CAMERA_ANTIBANDING_AUTO_50HZ,
  CAMERA_ANTIBANDING_AUTO_60HZ,
  CAMERA_MAX_ANTIBANDING,
} camera_antibanding_type;

/* Enum Type for different ISO Mode supported */
typedef enum {
  CAMERA_ISO_AUTO = 0,
  CAMERA_ISO_DEBLUR,
  CAMERA_ISO_100,
  CAMERA_ISO_200,
  CAMERA_ISO_400,
  CAMERA_ISO_800,
  CAMERA_ISO_1600,
  CAMERA_ISO_MAX
} camera_iso_mode_type;

typedef enum {
  STROBE_FLASH_MODE_OFF,
  STROBE_FLASH_MODE_AUTO,
  STROBE_FLASH_MODE_ON,
  STROBE_FLASH_MODE_MAX,
} strobe_flash_mode_t;

/* Clockwise */
typedef enum {
  CAMERA_ENCODING_ROTATE_0,
  CAMERA_ENCODING_ROTATE_90,
  CAMERA_ENCODING_ROTATE_180,
  CAMERA_ENCODING_ROTATE_270
} camera_encoding_rotate_t;

typedef enum {
  MOTION_ISO_OFF,
  MOTION_ISO_ON
} motion_iso_t;

typedef enum {
  FPS_MODE_AUTO,
  FPS_MODE_FIXED,
} fps_mode_t;

typedef struct {
  int32_t minimum_value; /* Minimum allowed value */
  int32_t maximum_value; /* Maximum allowed value */
  int32_t step_value;    /* step value */
  int32_t default_value; /* Default value */
  int32_t current_value; /* Current value */
} cam_parm_info_t;

typedef struct {
  struct msm_ctrl_cmd ctrlCmd;
  int fd;
  void (*af_cb)(int8_t );
  int8_t is_camafctrl_thread_join;
  isp3a_af_mode_t af_mode;
} cam_af_ctrl_t;

typedef enum {
  CAF_OFF,
  CAF_ON
} caf_ctrl_t;

typedef enum {
  AEC_ROI_OFF,
  AEC_ROI_ON
} aec_roi_ctrl_t;

typedef enum {
  AEC_ROI_BY_INDEX,
  AEC_ROI_BY_COORDINATE,
} aec_roi_type_t;

typedef struct {
  uint32_t x;
  uint32_t y;
} cam_coordinate_type_t;

/*
 * Define DRAW_RECTANGLES to draw rectangles on screen. Just for test purpose.
 */
//#define DRAW_RECTANGLES

typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t dx;
  uint16_t dy;
} roi_t;

typedef struct {
  aec_roi_ctrl_t aec_roi_enable;
  aec_roi_type_t aec_roi_type;
  union {
    cam_coordinate_type_t coordinate;
    uint32_t aec_roi_idx;
  } aec_roi_position;
} cam_set_aec_roi_t;

typedef struct {
  uint32_t frm_id;
  uint8_t num_roi;
  roi_t roi[MAX_ROI];
  uint8_t is_multiwindow;
} roi_info_t;

/* Exif Tag Data Type */
typedef enum
{
    EXIF_BYTE      = 1,
    EXIF_ASCII     = 2,
    EXIF_SHORT     = 3,
    EXIF_LONG      = 4,
    EXIF_RATIONAL  = 5,
    EXIF_UNDEFINED = 7,
    EXIF_SLONG     = 9,
    EXIF_SRATIONAL = 10
} exif_tag_type_t;


/* Exif Rational Data Type */
typedef struct
{
    uint32_t  num;    // Numerator
    uint32_t  denom;  // Denominator

} rat_t;

/* Exif Signed Rational Data Type */
typedef struct
{
    int32_t  num;    // Numerator
    int32_t  denom;  // Denominator

} srat_t;

typedef struct
{
  exif_tag_type_t type;
  uint8_t copy;
  uint32_t count;
  union
  {
    char      *_ascii;
    uint8_t   *_bytes;
    uint8_t    _byte;
    uint16_t  *_shorts;
    uint16_t   _short;
    uint32_t  *_longs;
    uint32_t   _long;
    rat_t     *_rats;
    rat_t      _rat;
    uint8_t   *_undefined;
    int32_t   *_slongs;
    int32_t    _slong;
    srat_t    *_srats;
    srat_t     _srat;
  } data;
} exif_tag_entry_t;

typedef struct {
    uint32_t      tag_id;
    exif_tag_entry_t  tag_entry;
} exif_tags_info_t;


typedef enum {
 HDR_BRACKETING_OFF,
 HDR_MODE,
 EXP_BRACKETING_MODE
 } hdr_mode;

typedef struct {
  hdr_mode mode;
  uint32_t hdr_enable;
  uint32_t total_frames;
  uint32_t total_hal_frames;
  char values[MAX_EXP_BRACKETING_LENGTH];  /* user defined values */
} exp_bracketing_t;

typedef struct {
  roi_t      mtr_area[MAX_ROI];
  uint32_t   num_area;
  int        weight[MAX_ROI];
} aec_mtr_area_t;

typedef struct {
  int denoise_enable;
  int process_plates;
} denoise_param_t;

/* to select no vignette correction, luma vignette correction */
/* or bayer vignette correction */
typedef enum {
  CAMERA_NO_VIGNETTE_CORRECTION,
  CAMERA_LUMA_VIGNETTE_CORRECTION,
  CAMERA_BAYER_VIGNETTE_CORRECTION
} camera_vc_mode_type;

typedef enum {
  CAMERA_BVCM_DISABLE,
  CAMERA_BVCM_RAW_CAPTURE,
  CAMERA_BVCM_OFFLINE_CAPTURE
} camera_bvcm_capture_type;

#ifndef HAVE_CAMERA_SIZE_TYPE
  #define HAVE_CAMERA_SIZE_TYPE
struct camera_size_type {
  int width;
  int height;
};
#endif

/* Sensor position type, used for CAMERA_PARM_SENSOR_POSITION */
typedef enum {
  CAMERA_SP_NORMAL = 0,
  CAMERA_SP_REVERSE,
} camera_sp_type;

/* Exposure type, used for CAMERA_PARM_EXPOSURE */
typedef enum {
  CAMERA_EXPOSURE_MIN_MINUS_1,
  CAMERA_EXPOSURE_AUTO = 1,  /* This list must match aeecamera.h */
  CAMERA_EXPOSURE_DAY,
  CAMERA_EXPOSURE_NIGHT,
  CAMERA_EXPOSURE_LANDSCAPE,
  CAMERA_EXPOSURE_STRONG_LIGHT,
  CAMERA_EXPOSURE_SPOTLIGHT,
  CAMERA_EXPOSURE_PORTRAIT,
  CAMERA_EXPOSURE_MOVING,
  CAMERA_EXPOSURE_MAX_PLUS_1
} camera_exposure_type;

typedef enum {
  CAMERA_NIGHTSHOT_MODE_OFF,
  CAMERA_NIGHTSHOT_MODE_ON,
  CAMERA_MAX_NIGHTSHOT_MODE
} camera_nightshot_mode_type;

typedef struct {
  uint32_t yoffset;
  uint32_t cbcr_offset;
  uint32_t size;
  struct camera_size_type resolution;
} cam_buf_info_t;

typedef struct {
  int x;
  int y;
} cam_point_t;

typedef struct {
  /* AF parameters */
  uint8_t focus_position;
  /* AEC parameters */
  uint32_t line_count;
  uint8_t luma_target;
  /* AWB parameters */
  int32_t r_gain;
  int32_t b_gain;
  int32_t g_gain;
  uint8_t exposure_mode;
  uint8_t exposure_program;
  float exposure_time;
  uint32_t iso_speed;
} snapshotData_info_t;


typedef enum {
  CAMERA_HFR_MODE_OFF = 1,
  CAMERA_HFR_MODE_60FPS,
  CAMERA_HFR_MODE_90FPS,
  CAMERA_HFR_MODE_120FPS,
  CAMERA_HFR_MODE_150FPS,
} camera_hfr_mode_t;

typedef struct video_frame_info {
  /* NOTE !!!! Important:  It's recommended to make sure both
     w/h of the buffer & image are 32x. When rotation is needed,
     the values in this data structure is POST-rotation. */

  uint32_t               y_buffer_width;     /* y plane */
  uint32_t               cbcr_buffer_width;  /* cbcr plane */
  uint32_t               image_width;        /**< original image width.   */
  uint32_t               image_height;       /**< original image height. */
  uint32_t               color_format;
} video_frame_info;

typedef enum VIDEO_ROT_ENUM {
  ROT_NONE               = 0,
  ROT_CLOCKWISE_90       = 1,
  ROT_CLOCKWISE_180      = 6,
  ROT_CLOCKWISE_270      = 7,
} VIDEO_ROT_ENUM;

typedef struct video_rotation_param_ctrl_t {
  VIDEO_ROT_ENUM rotation; /* 0 degree = rot disable. */
} video_rotation_param_ctrl_t;

typedef struct {
  uint32_t               dis_enable;  /* DIS feature: 1 = enable, 0 = disable.
                                           when enable, caller makes sure w/h are 10% more. */
  VIDEO_ROT_ENUM         rotation;        /* when rotation = 0, that also means it is disabled.*/
  video_frame_info       input_frame;
  video_frame_info       output_frame;
} video_param_ctrl_t;

void *cam_conf (void *data);
int launch_cam_conf_thread(void);
int wait_cam_conf_ready(void);
int release_cam_conf_thread(void);
void set_config_start_params(config_params_t*);
int launch_camafctrl_thread(cam_af_ctrl_t *pAfctrl);
/* Stats */
typedef enum {
  CAM_STATS_MSG_HIST,
  CAM_STATS_MSG_EXIT
}camstats_msg_type;

/* Stats messages */
typedef struct {
  camstats_msg_type msg_type;
  union {
    camera_preview_histogram_info hist_data;
  } msg_data;
} camstats_msg;
/*cam stats thread*/
int launch_camstats_thread(void);
void release_camstats_thread(void);
int8_t send_camstats(camstats_type msg_type, void* data, int size);
int8_t send_camstats_msg(camstats_type stats_type, camstats_msg* p_msg);
int is_camstats_thread_running(void);

struct cam_frame_start_parms {
	unsigned int unknown;
	struct msm_frame frame;
	struct msm_frame video_frame;
};

int launch_camframe_thread(cam_frame_start_parms* parms);
void release_camframe_thread(void);
void camframe_terminate(void);
void *cam_frame(void *data);
/*sends the free frame of given type to mm-camera*/
int8_t camframe_add_frame(cam_frame_type_t, struct msm_frame*);
/*release all frames of given type*/
int8_t camframe_release_all_frames(cam_frame_type_t);

/* frame Q*/
struct fifo_node
{
  struct fifo_node *next;
  void *f;
};

struct fifo_queue
{
  int num_of_frames;
  struct fifo_node *front;
  struct fifo_node *back;
  pthread_mutex_t mut;
  pthread_cond_t wait;
  char* name;
};

enum focus_distance_index{
  FOCUS_DISTANCE_NEAR_INDEX,  /* 0 */
  FOCUS_DISTANCE_OPTIMAL_INDEX,
  FOCUS_DISTANCE_FAR_INDEX,
  FOCUS_DISTANCE_MAX_INDEX
};

typedef struct {
  float focus_distance[FOCUS_DISTANCE_MAX_INDEX];
} focus_distances_info_t;

struct msm_frame* get_frame(struct fifo_queue* queue);
int8_t add_frame(struct fifo_queue* queue, struct msm_frame *p);
void flush_queue (struct fifo_queue* queue);
void wait_queue (struct fifo_queue* queue);
void signal_queue (struct fifo_queue* queue);

/* Display */
typedef struct {
    uint16_t user_input_display_width;
    uint16_t user_input_display_height;
} USER_INPUT_DISPLAY_T;
int launch_camframe_fb_thread(void);
void release_camframe_fb_thread(void);
void use_overlay_fb_display_driver(void);

/* v4L2 */
void *v4l2_cam_frame(void *data);
void release_v4l2frame_thread(pthread_t frame_thread);

typedef struct {
  uint32_t buf_len;
  uint8_t num;
  uint8_t pmem_type;
  uint32_t vaddr[8];
} mm_camera_histo_mem_info_t;

typedef enum {
  MM_CAMERA_CTRL_EVT_ZOOM_DONE,
  MM_CAMERA_CTRL_EVT_AUTO_FOCUS_DONE,
  MM_CAMERA_CTRL_EVT_PREP_SNAPSHOT,
  MM_CAMERA_CTRL_EVT_SNAPSHOT_CONFIG_DONE,
  MM_CAMERA_CTRL_EVT_WDN_DONE, // wavelet denoise done
  MM_CAMERA_CTRL_EVT_ERROR,
  MM_CAMERA_CTRL_EVT_MAX
} mm_camera_ctrl_event_type_t;

typedef struct {
  mm_camera_ctrl_event_type_t evt;
  cam_ctrl_status_t status;
  unsigned long cookie;
} mm_camera_ctrl_event_t;

typedef enum {
  MM_CAMERA_CH_EVT_STREAMING_ON,
  MM_CAMERA_CH_EVT_STREAMING_OFF,
  MM_CAMERA_CH_EVT_STREAMING_ERR,
  MM_CAMERA_CH_EVT_DATA_DELIVERY_DONE,
  MM_CAMERA_CH_EVT_DATA_REQUEST_MORE,
  MM_CAMERA_CH_EVT_MAX
} mm_camera_ch_event_type_t;

typedef struct {
  uint32_t ch;
  mm_camera_ch_event_type_t evt;
} mm_camera_ch_event_t;

typedef struct {
  uint32_t index;
  /* TBD: need more fields for histo stats? */
} mm_camera_stats_histo_t;

typedef struct  {
  uint32_t event_id;
  union {
    mm_camera_stats_histo_t    stats_histo;
  } e;
} mm_camera_stats_event_t;

typedef enum {
  FD_ROI_TYPE_HEADER,
  FD_ROI_TYPE_DATA
} fd_roi_type_t;

typedef struct {
  uint32_t frame_id;
  int16_t num_face_detected;
} fd_roi_header_type;

struct fd_rect_t {
  uint16_t x;
  uint16_t y;
  uint16_t dx;
  uint16_t dy;
};

typedef struct {
  struct fd_rect_t face_boundary;
  uint16_t left_eye_center[2];
  uint16_t right_eye_center[2];
  uint16_t mouth_center[2];
  uint8_t smile_degree;  //0 -100
  uint8_t smile_confidence;  //
  uint8_t blink_detected;  // 0 or 1
  uint8_t is_face_recognised;  // 0 or 1
  int8_t gaze_angle;  // -90 -45 0 45 90 for head left to rigth tilt
  int8_t updown_dir;  // -90 to 90
  int8_t leftright_dir;  //-90 to 90
  int8_t roll_dir;  // -90 to 90
  int8_t left_right_gaze;  // -50 to 50
  int8_t top_bottom_gaze;  // -50 to 50
  uint8_t left_blink;  // 0 - 100
  uint8_t right_blink;  // 0 - 100
  int8_t id;  // unique id for face tracking within view unless view changes
  int8_t score;  // score of confidence( 0 -100)
} fd_face_type;

typedef struct {
  uint32_t frame_id;
  uint8_t idx;
  fd_face_type face;
} fd_roi_data_type;
/*
struct fd_roi_t {
  fd_roi_type_t type;
  union {
    fd_roi_header_type hdr;
    fd_roi_data_type data;
  } d;
};
*/
struct fd_roi_t {
    uint32_t frame_id;
    int16_t rect_num;
    struct fd_rect_t faces[MAX_ROI];
};

typedef struct  {
  uint32_t event_id;
  union {
    mm_camera_histo_mem_info_t histo_mem_info;
    struct fd_roi_t roi;
  } e;
} mm_camera_info_event_t;


typedef enum {
  MM_CAMERA_EVT_TYPE_CH,
  MM_CAMERA_EVT_TYPE_CTRL,
  MM_CAMERA_EVT_TYPE_STATS,
  MM_CAMERA_EVT_TYPE_INFO,
  MM_CAMERA_EVT_TYPE_MAX
} mm_camera_event_type_t;

/******************************************************************************
 * Function: exif_set_tag
 * Description: Inserts or modifies an Exif tag to the Exif Info object. Typical
 *              use is to call this function multiple times - to insert all the
 *              desired Exif Tags individually to the Exif Info object and
 *              then pass the info object to the Jpeg Encoder object so
 *              the inserted tags would be emitted as tags in the Exif header.
 * Input parameters:
 *   obj       - The Exif Info object where the tag would be inserted to or
 *               modified from.
 *   tag_id    - The Exif Tag ID of the tag to be inserted/modified.
 *   p_entry   - The pointer to the tag entry structure which contains the
 *               details of tag. The pointer can be set to NULL to un-do
 *               previous insertion for a certain tag.
 * Return values:
 *     JPEGERR_SUCCESS
 *     JPEGERR_ENULLPTR
 *     JPEGERR_EFAILED
 * (See jpegerr.h for description of error values.)
 * Notes: none
 *****************************************************************************/
int exif_set_tag(exif_info_obj_t    obj,
                 exif_tag_id_t      tag_id,
                 exif_tag_entry_t  *p_entry);


typedef enum {
  MM_CAMERA_SUCCESS,
  MM_CAMERA_ERR_GENERAL,
  MM_CAMERA_ERR_NO_MEMORY,
  MM_CAMERA_ERR_NOT_SUPPORTED,
  MM_CAMERA_ERR_INVALID_INPUT,
  MM_CAMERA_ERR_INVALID_OPERATION,
  MM_CAMERA_ERR_ENCODE,
  MM_CAMERA_ERR_BUFFER_REG,
  MM_CAMERA_ERR_PMEM_ALLOC,
  MM_CAMERA_ERR_CAPTURE_FAILED,
  MM_CAMERA_ERR_CAPTURE_TIMEOUT,
} mm_camera_status_t;

typedef struct {
  uint8_t* ptr;
  uint32_t actual_size;
  uint32_t size;
  int32_t fd;
  uint32_t offset;
} mm_camera_buffer_t;

typedef enum {
  CAMERA_OPS_STREAMING_PREVIEW,
  CAMERA_OPS_STREAMING_ZSL,
  CAMERA_OPS_STREAMING_VIDEO,
  CAMERA_OPS_CAPTURE, /*not supported*/
  CAMERA_OPS_FOCUS,
  CAMERA_OPS_GET_PICTURE, /*5*/
  CAMERA_OPS_PREPARE_SNAPSHOT,
  CAMERA_OPS_SNAPSHOT,
  CAMERA_OPS_LIVESHOT,
  CAMERA_OPS_RAW_SNAPSHOT,
  CAMERA_OPS_VIDEO_RECORDING, /*10*/
  CAMERA_OPS_REGISTER_BUFFER,
  CAMERA_OPS_UNREGISTER_BUFFER,
  CAMERA_OPS_SENSOR_RESET,
} mm_camera_ops_type_t;

typedef struct {
  mm_camera_status_t (*mm_camera_start) (mm_camera_ops_type_t ops_type,
    void* parm1, void* parm2);
  mm_camera_status_t(*mm_camera_stop) (mm_camera_ops_type_t ops_type,
    void* parm1, void* parm2);
  int8_t (*mm_camera_is_supported) (mm_camera_ops_type_t ops_type);
} mm_camera_ops;

typedef enum {
  FRAME_READY,
  SNAPSHOT_DONE,
  SNAPSHOT_FAILED,
  JPEG_ENC_DONE,
  JPEG_ENC_FAILED,
} mm_camera_event_type;

typedef struct {
  mm_camera_event_type event_type;
  union {
   mm_camera_buffer_t* encoded_frame;
   struct msm_frame* preview_frame;
  }event_data;
} mm_camera_event;

typedef struct {
  mm_camera_status_t (*mm_camera_query_parms) (mm_camera_parm_type_t parm_type,
    void** pp_values, uint32_t* p_count);
  mm_camera_status_t (*mm_camera_set_parm) (mm_camera_parm_type_t parm_type,
    void* p_value);
  mm_camera_status_t(*mm_camera_get_parm) (mm_camera_parm_type_t parm_type,
    void* p_value);
  int8_t (*mm_camera_is_supported) (mm_camera_parm_type_t parm_type);
  int8_t (*mm_camera_is_parm_supported) (mm_camera_parm_type_t parm_type,
   void* sub_parm);
} mm_camera_config;

typedef enum {
  LIVESHOT_SUCCESS,
  LIVESHOT_ENCODE_ERROR,
  LIVESHOT_UNKNOWN_ERROR,
} liveshot_status;

typedef struct {
  int8_t (*on_event)(mm_camera_event* evt);
  void (*video_frame_cb) (struct msm_frame *);
  void (*preview_frame_cb) (struct msm_frame *);
  void (*on_error_event) (camera_error_type err);
  void (*camstats_cb) (camstats_type, camera_preview_histogram_info*);
  void (*jpegfragment_cb)(uint8_t *, uint32_t);
  void (*on_jpeg_event)(uint32_t status);
  void (*on_liveshot_event)(liveshot_status status, uint32_t jpeg_size);
} mm_camera_notify;

static struct fifo_node* dequeue(struct fifo_queue* q) {
  struct fifo_node *tmp = q->front;

  if (!tmp)
      return 0;
  if (q->front == q->back)
      q->front = q->back = 0;
  else if (q->front)
          q->front = q->front->next;
  tmp->next = 0;
  q->num_of_frames -=1;
  return tmp;
}

static void enqueue(struct fifo_queue* q, struct fifo_node*p) {
  if (q->back) {
      q->back->next = p;
      q->back = p;
  }
  else {
      q->front = p;
      q->back = p;
  }
  q->num_of_frames +=1;
  return;
}

#endif /* __QCAMERA_INTF_H__ */
