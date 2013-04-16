/*
** Copyright 2008, Google Inc.
** Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_HARDWARE_QUALCOMM_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_QUALCOMM_CAMERA_HARDWARE_H

#include "CameraHardwareInterface.h"
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <utils/threads.h>
#include <stdint.h>
#include "Overlay.h"

extern "C" {
#include <linux/android_pmem.h>
#include <media/msm_camera.h>
#include "QCamera_Intf.h"
}
// Extra propriatary stuff (mostly from CM)
#define MSM_CAMERA_CONTROL "/dev/msm_camera/control0"

#define TRUE 1
#define FALSE 0

typedef struct {
	uint32_t in1_w;
	uint32_t out1_w;
	uint32_t in1_h;
	uint32_t out1_h;
	uint32_t in2_w;
	uint32_t out2_w;
	uint32_t in2_h;
	uint32_t out2_h;
	uint8_t update_flag;
} common_crop_t;

typedef struct {
	uint32_t timestamp;  /* seconds since 1/6/1980          */
	double   latitude;   /* degrees, WGS ellipsoid */
	double   longitude;  /* degrees                */
	int16_t  altitude;   /* meters                          */
} camera_position_type;

typedef struct {
	uint32_t dis_enable;
	uint32_t video_rec_width;
	uint32_t video_rec_height;
	uint32_t output_cbcr_offset;
} video_dis_param_ctrl_t;

typedef uint32_t jpeg_event_t;

typedef enum {
	CAMERA_WB_MIN_MINUS_1,
	CAMERA_WB_AUTO = 1,  /* This list must match aeecamera.h */
	CAMERA_WB_CUSTOM,
	CAMERA_WB_INCANDESCENT,
	CAMERA_WB_FLUORESCENT,
	CAMERA_WB_DAYLIGHT,
	CAMERA_WB_CLOUDY_DAYLIGHT,
	CAMERA_WB_TWILIGHT,
	CAMERA_WB_SHADE,
	CAMERA_WB_MAX_PLUS_1
} camera_wb_type;

typedef enum {
    CAMERA_BESTSHOT_OFF,
    CAMERA_BESTSHOT_ACTION,
    CAMERA_BESTSHOT_PORTRAIT,
    CAMERA_BESTSHOT_LANDSCAPE,
    CAMERA_BESTSHOT_NIGHT,
    CAMERA_BESTSHOT_NIGHT_PORTRAIT,
    CAMERA_BESTSHOT_THEATRE,
    CAMERA_BESTSHOT_BEACH,
    CAMERA_BESTSHOT_SNOW,
    CAMERA_BESTSHOT_SUNSET,
    CAMERA_BESTSHOT_ANTISHAKE,
    CAMERA_BESTSHOT_FIREWORKS,
    CAMERA_BESTSHOT_SPORTS,
    CAMERA_BESTSHOT_PARTY,
    CAMERA_BESTSHOT_CANDLELIGHT,
    CAMERA_BESTSHOT_BACKLIGHT,
    CAMERA_BESTSHOT_FLOWERS,
    CAMERA_BESTSHOT_AR,
} camera_scene_mode_t;

enum {
	LED_MODE_OFF,
	LED_MODE_ON,
	LED_MODE_AUTO,
	LED_MODE_TORCH
};

#define CAMERA_MIN_CONTRAST 0
#define CAMERA_MAX_CONTRAST 10
#define CAMERA_MIN_SHARPNESS 0
#define CAMERA_MAX_SHARPNESS 30
#define CAMERA_MIN_SATURATION 0
#define CAMERA_MAX_SATURATION 10

#define CAMERA_DEF_SHARPNESS 10
#define CAMERA_DEF_CONTRAST 5
#define CAMERA_DEF_SATURATION 5

#define JPEG_EVENT_DONE 0 /* useless */



typedef enum {
	CAMERA_RSP_CB_SUCCESS,    /* Function is accepted         */
	CAMERA_EXIT_CB_DONE,      /* Function is executed         */
	CAMERA_EXIT_CB_FAILED,    /* Execution failed or rejected */
	CAMERA_EXIT_CB_DSP_IDLE,  /* DSP is in idle state         */
	CAMERA_EXIT_CB_DSP_ABORT, /* Abort due to DSP failure     */
	CAMERA_EXIT_CB_ABORT,     /* Function aborted             */
	CAMERA_EXIT_CB_ERROR,     /* Failed due to resource       */
	CAMERA_EVT_CB_FRAME,      /* Preview or video frame ready */
	CAMERA_EVT_CB_PICTURE,    /* Picture frame ready for multi-shot */
	CAMERA_STATUS_CB,         /* Status updated               */
	CAMERA_EXIT_CB_FILE_SIZE_EXCEEDED, /* Specified file size not achieved,
	encoded file written & returned anyway */
	CAMERA_EXIT_CB_BUFFER,    /* A buffer is returned         */
	CAMERA_EVT_CB_SNAPSHOT_DONE,/*  Snapshot updated               */
	CAMERA_CB_MAX
} camera_cb_type;

#define EXIFTAGID_GPS_LATITUDE_REF        0x10001
#define EXIFTAGID_GPS_LATITUDE            0x20002
#define EXIFTAGID_GPS_LONGITUDE_REF       0x30003
#define EXIFTAGID_GPS_LONGITUDE           0x40004
#define EXIFTAGID_GPS_ALTITUDE_REF        0x50005
#define EXIFTAGID_GPS_ALTITUDE            0x60006
#define EXIFTAGID_GPS_TIMESTAMP           0x70007
#define EXIFTAGID_GPS_DATESTAMP           0x1D001D
#define EXIFTAGID_EXIF_CAMERA_MAKER       0x21010F
#define EXIFTAGID_EXIF_CAMERA_MODEL       0x220110
#define EXIFTAGID_EXIF_DATE_TIME_ORIGINAL 0x3A9003
#define EXIFTAGID_EXIF_DATE_TIME          0x3B9004
#define EXIFTAGID_EXIF_DATE_TIME_CREATED  0x3b9004
#define EXIFTAGID_FOCAL_LENGTH            0x45920a

typedef enum {
    AUTO,
    SPOT,
    CENTER_WEIGHTED,
    AVERAGE
} select_zone_af_t;

// End of closed stuff

struct str_map {
    const char *const desc;
    int val;
};

typedef enum {
    TARGET_MSM7625,
    TARGET_MSM7627,
    TARGET_QSD8250,
    TARGET_MSM7630,
    TARGET_MSM8660,
    TARGET_MAX
}targetType;

typedef enum {
    LIVESHOT_DONE,
    LIVESHOT_IN_PROGRESS,
    LIVESHOT_STOPPED
}liveshotState;

struct target_map {
    const char *targetStr;
    targetType targetEnum;
};

struct board_property{
    targetType target;
    unsigned int previewSizeMask;
    bool hasSceneDetect;
    bool hasSelectableZoneAf;
    bool hasFaceDetect;
};

namespace android {

class QualcommCameraHardware : public CameraHardwareInterface {
public:

    virtual sp<IMemoryHeap> getPreviewHeap() const;
    virtual sp<IMemoryHeap> getRawHeap() const;

    virtual void setCallbacks(notify_callback notify_cb,
                              data_callback data_cb,
                              data_callback_timestamp data_cb_timestamp,
                              void* user);
    virtual void enableMsgType(int32_t msgType);
    virtual void disableMsgType(int32_t msgType);
    virtual bool msgTypeEnabled(int32_t msgType);

    virtual status_t dump(int fd, const Vector<String16>& args) const;
    virtual status_t startPreview();
    virtual void stopPreview();
    virtual bool previewEnabled();
    virtual status_t startRecording();
    virtual void stopRecording();
    virtual bool recordingEnabled();
    virtual void releaseRecordingFrame(const sp<IMemory>& mem);
    virtual status_t autoFocus();
    virtual status_t cancelAutoFocus();
    virtual status_t takePicture();
    virtual status_t takeLiveSnapshot();
    void set_liveshot_exifinfo();
    virtual status_t cancelPicture();
    virtual status_t setParameters(const CameraParameters& params);
    virtual CameraParameters getParameters() const;
    virtual status_t sendCommand(int32_t command, int32_t arg1, int32_t arg2);
    virtual status_t getBufferInfo(sp<IMemory>& Frame, size_t *alignedSize);
    virtual void encodeData();

    virtual void release();
    virtual bool useOverlay();
    virtual status_t setOverlay(const sp<Overlay> &overlay);

    /* For compatibility with TouchPad binary libcamera */
    virtual void stub1() {};
    virtual void stub2() {};
    virtual void stopSnapshot() {};

    static sp<CameraHardwareInterface> createInstance();
    static sp<QualcommCameraHardware> getInstance();

    void receivePreviewFrame(struct msm_frame *frame);
    void receiveLiveSnapshot(uint32_t jpeg_size);
    void receiveCameraStats(camstats_type stype, camera_preview_histogram_info* histinfo);
    void receiveRecordingFrame(struct msm_frame *frame);
    void receiveJpegPicture(void);
    void jpeg_set_location();
    void receiveJpegPictureFragment(uint8_t *buf, uint32_t size);
    void notifyShutter(common_crop_t *crop, bool mPlayShutterSoundOnly);
    void receive_camframe_error_timeout();
    static void getCameraInfo();

private:
    QualcommCameraHardware();
    virtual ~QualcommCameraHardware();
    status_t startPreviewInternal();
    status_t setHistogramOn();
    status_t setHistogramOff();
    status_t runFaceDetection();
    status_t setFaceDetection(const char *str);
    void stopPreviewInternal();
    friend void *auto_focus_thread(void *user);
    void runAutoFocus();
    status_t cancelAutoFocusInternal();
    bool native_set_dimension (int camfd);
    bool native_jpeg_encode (void);
    bool native_set_parms(mm_camera_parm_type_t type, uint16_t length, void *value);
    bool native_set_parms( mm_camera_parm_type_t type, uint16_t length, void *value, int *result);
    bool native_zoom_image(int fd, int srcOffset, int dstOffset, common_crop_t *crop);

    static wp<QualcommCameraHardware> singleton;

    /* These constants reflect the number of buffers that libmmcamera requires
       for preview and raw, and need to be updated when libmmcamera
       changes.
    */
    static const int kPreviewBufferCount = NUM_PREVIEW_BUFFERS;
    static const int kRawBufferCount = 1;
    static const int kJpegBufferCount = 1;

    int jpegPadding;

    CameraParameters mParameters;
    unsigned int frame_size;
    bool mCameraRunning;
    Mutex mCameraRunningLock;
    bool mPreviewInitialized;


    class MMCameraDL : public RefBase{
    private:
        static wp<MMCameraDL> instance;
        MMCameraDL();
        virtual ~MMCameraDL();
        void *libmmcamera;
        static Mutex singletonLock;
    public:
        static sp<MMCameraDL> getInstance();
        void * pointer();
    };

    // This class represents a heap which maintains several contiguous
    // buffers.  The heap may be backed by pmem (when pmem_pool contains
    // the name of a /dev/pmem* file), or by ashmem (when pmem_pool == NULL).

    struct MemPool : public RefBase {
        MemPool(int buffer_size, int num_buffers,
                int frame_size,
                const char *name);

        virtual ~MemPool() = 0;

        void completeInitialization();
        bool initialized() const {
            return mHeap != NULL && mHeap->base() != MAP_FAILED;
        }

        virtual status_t dump(int fd, const Vector<String16>& args) const;

        int mBufferSize;
        int mAlignedBufferSize;
        int mNumBuffers;
        int mFrameSize;
        sp<MemoryHeapBase> mHeap;
        sp<MemoryBase> *mBuffers;

        const char *mName;
    };

    struct AshmemPool : public MemPool {
        AshmemPool(int buffer_size, int num_buffers,
                   int frame_size,
                   const char *name);
    };

    struct PmemPool : public MemPool {
        PmemPool(const char *pmem_pool,
                 int flags, int pmem_type,
                 int buffer_size, int num_buffers,
                 int frame_size, int cbcr_offset,
                 int yoffset, const char *name);
        virtual ~PmemPool();
        int mFd;
        int mPmemType;
        int mCbCrOffset;
        int myOffset;
        int mCameraControlFd;
        uint32_t mAlignedSize;
        struct pmem_region mSize;
        sp<QualcommCameraHardware::MMCameraDL> mMMCameraDLRef;
    };

    sp<PmemPool> mPreviewHeap;
    sp<PmemPool> mRecordHeap;
    sp<PmemPool> mThumbnailHeap;
    sp<PmemPool> mRawHeap;
    sp<PmemPool> mDisplayHeap;
    sp<AshmemPool> mJpegHeap;
    sp<AshmemPool> mStatHeap;
    sp<AshmemPool> mMetaDataHeap;
    sp<PmemPool> mRawSnapShotPmemHeap;
    sp<PmemPool> mPostViewHeap;


    sp<MMCameraDL> mMMCameraDLRef;

    bool startCamera();
    bool initPreview();
    bool initRecord();
    void deinitPreview();
    bool initRaw(bool initJpegHeap);
    bool initLiveSnapshot(int videowidth, int videoheight);
    bool initRawSnapshot();
    void deinitRaw();
    void deinitRawSnapshot();

    bool mFrameThreadRunning;
    Mutex mFrameThreadWaitLock;
    Condition mFrameThreadWait;
    friend void *frame_thread(void *user);
    void runFrameThread(void *data);

    //720p recording video thread
    bool mVideoThreadExit;
    bool mVideoThreadRunning;
    Mutex mVideoThreadWaitLock;
    Condition mVideoThreadWait;
    friend void *video_thread(void *user);
    void runVideoThread(void *data);

    friend void *openCamera(void *data);

    // For Histogram
    int mStatsOn;
    int mCurrent;
    bool mSendData;
    Mutex mStatsWaitLock;
    Condition mStatsWait;

    //For Face Detection
    int mFaceDetectOn;
    bool mSendMetaData;
    Mutex mMetaDataWaitLock;

    bool mShutterPending;
    Mutex mShutterLock;

    bool mSnapshotThreadRunning;
    Mutex mSnapshotThreadWaitLock;
    Condition mSnapshotThreadWait;
    friend void *snapshot_thread(void *user);
    void runSnapshotThread(void *data);
    Mutex mRawPictureHeapLock;
    bool mJpegThreadRunning;
    Mutex mJpegThreadWaitLock;
    Condition mJpegThreadWait;
    bool mInSnapshotMode;
    Mutex mInSnapshotModeWaitLock;
    Condition mInSnapshotModeWait;
    bool mEncodePending;
    Mutex mEncodePendingWaitLock;
    Condition mEncodePendingWait;

    void debugShowPreviewFPS() const;
    void debugShowVideoFPS() const;

    int mSnapshotFormat;
    bool mFirstFrame;
    void hasAutoFocusSupport();
    void filterPictureSizes();
    void filterPreviewSizes();
    void storeTargetType();
    bool supportsSceneDetection();
    bool supportsSelectableZoneAf();
    bool supportsFaceDetection();

    void initDefaultParameters();

    status_t setPreviewSize(const CameraParameters& params);
    status_t setJpegThumbnailSize(const CameraParameters& params);
    status_t setPreviewFpsRange(const CameraParameters& params);
    status_t setPreviewFrameRate(const CameraParameters& params);
    status_t setPreviewFrameRateMode(const CameraParameters& params);
    status_t setRecordSize(const CameraParameters& params);
    status_t setPictureSize(const CameraParameters& params);
    status_t setJpegQuality(const CameraParameters& params);
    status_t setAntibanding(const CameraParameters& params);
    status_t setEffect(const CameraParameters& params);
    status_t setExposureCompensation(const CameraParameters &params);
    status_t setAutoExposure(const CameraParameters& params);
    status_t setWhiteBalance(const CameraParameters& params);
    status_t setFlash(const CameraParameters& params);
    status_t setGpsLocation(const CameraParameters& params);
    status_t setRotation(const CameraParameters& params);
    status_t setZoom(const CameraParameters& params);
    status_t setFocusMode(const CameraParameters& params);
    status_t setBrightness(const CameraParameters& params);
    status_t setSkinToneEnhancement(const CameraParameters& params);
    status_t setOrientation(const CameraParameters& params);
    status_t setLensshadeValue(const CameraParameters& params);
    status_t setISOValue(const CameraParameters& params);
    status_t setPictureFormat(const CameraParameters& params);
    status_t setSharpness(const CameraParameters& params);
    status_t setContrast(const CameraParameters& params);
    status_t setSaturation(const CameraParameters& params);
    status_t setSceneMode(const CameraParameters& params);
    status_t setContinuousAf(const CameraParameters& params);
    status_t setTouchAfAec(const CameraParameters& params);
    status_t setSceneDetect(const CameraParameters& params);
    status_t setStrTextures(const CameraParameters& params);
    status_t setPreviewFormat(const CameraParameters& params);
    status_t setSelectableZoneAf(const CameraParameters& params);
    void setGpsParameters();
    bool storePreviewFrameForPostview();
    bool isValidDimension(int w, int h);

    Mutex mLock;
    Mutex mCamframeTimeoutLock;
    bool camframe_timeout_flag;
    bool mReleasedRecordingFrame;

    bool receiveRawPicture(void);
    bool receiveRawSnapshot(void);

    Mutex mCallbackLock;
    Mutex mOverlayLock;
	Mutex mRecordLock;
	Mutex mRecordFrameLock;
	Condition mRecordWait;
    Condition mStateWait;

    /* mJpegSize keeps track of the size of the accumulated JPEG.  We clear it
       when we are about to take a picture, so at any time it contains either
       zero, or the size of the last JPEG picture taken.
    */
    uint32_t mJpegSize;
    unsigned int        mPreviewFrameSize;
    unsigned int        mRecordFrameSize;
    int                 mRawSize;
    int                 mCbCrOffsetRaw;
    int                 mJpegMaxSize;
    int32_t             mStatSize;


    cam_ctrl_dimension_t mDimension;
    bool mAutoFocusThreadRunning;
    Mutex mAutoFocusThreadLock;

    Mutex mAfLock;

    pthread_t mFrameThread;
    pthread_t mVideoThread;
    pthread_t mSnapshotThread;
    pthread_t mDeviceOpenThread;

    common_crop_t mCrop;

    bool mInitialized;

    int mBrightness;
    int mSkinToneEnhancement;
    int mHJR;
    struct msm_frame frames[kPreviewBufferCount];
    struct msm_frame *recordframes;
    bool *record_buffers_tracking_flag;
    bool mInPreviewCallback;
    bool mUseOverlay;
    sp<Overlay>  mOverlay;

    int32_t mMsgEnabled;    // camera msg to be handled
    notify_callback mNotifyCallback;
    data_callback mDataCallback;
    data_callback_timestamp mDataCallbackTimestamp;
    void *mCallbackCookie;  // same for all callbacks
    int mDebugFps;
    int kPreviewBufferCountActual;
    int previewWidth, previewHeight;
    bool mSnapshotDone;
    bool mSnapshotPrepare;
    int maxSnapshotWidth;
    int maxSnapshotHeight;
    bool mHasAutoFocusSupport;
    int videoWidth, videoHeight;

    bool mDisEnabled;
    int mRotation;
    bool mResetOverlayCrop;
    int mThumbnailWidth, mThumbnailHeight;
    status_t setVpeParameters();
    status_t setDIS();
    bool strTexturesOn;
    Mutex mPmemWaitLock;
    Condition mPmemWait;
    bool mPrevHeapDeallocRunning;
    bool mSnapshotCancel;
    Mutex mSnapshotCancelLock;
};

}; // namespace android

#endif
