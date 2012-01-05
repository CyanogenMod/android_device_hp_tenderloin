/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <limits.h>
#include <sys/cdefs.h>
#include <hardware/gralloc.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include <cutils/native_handle.h>

#include <linux/fb.h>

#if defined(__cplusplus) && defined(HDMI_DUAL_DISPLAY)
#include "overlayLib.h"
using namespace overlay;
#endif

enum {
    /* gralloc usage bits indicating the type
     * of allocation that should be used */
    GRALLOC_USAGE_PRIVATE_ADSP_HEAP = GRALLOC_USAGE_PRIVATE_0,
    GRALLOC_USAGE_PRIVATE_EBI_HEAP = GRALLOC_USAGE_PRIVATE_1,
    GRALLOC_USAGE_PRIVATE_SMI_HEAP = GRALLOC_USAGE_PRIVATE_2,
    GRALLOC_USAGE_PRIVATE_SYSTEM_HEAP = GRALLOC_USAGE_PRIVATE_3,
    /* Set this for allocating uncached memory (using O_DSYNC)
     * cannot be used with the system heap */
    GRALLOC_USAGE_PRIVATE_UNCACHED = 0x00010000,
    /* This flag needs to be set when using a system heap
     * from ION. If not set, the system heap is assumed
     * to be coming from ashmem
     */
    GRALLOC_USAGE_PRIVATE_ION = 0x00020000,
};

enum {
    /* Gralloc perform enums
    */
    GRALLOC_MODULE_PERFORM_CREATE_HANDLE_FROM_BUFFER = 0x080000001,
};


enum {
    GPU_COMPOSITION,
    C2D_COMPOSITION,
    MDP_COMPOSITION,
    CPU_COMPOSITION,
};

/* numbers of max buffers for page flipping */
#define NUM_FRAMEBUFFERS_MIN 2
#define NUM_FRAMEBUFFERS_MAX 3

/* number of default bufers for page flipping */
#define NUM_DEF_FRAME_BUFFERS 2
#define NO_SURFACEFLINGER_SWAPINTERVAL
#define INTERLACE_MASK 0x80
#define S3D_FORMAT_MASK 0xFF000
#define COLOR_FORMAT(x) (x & 0xFFF) // Max range for colorFormats is 0 - FFF
#define DEVICE_PMEM "/dev/pmem"
#define DEVICE_PMEM_ADSP "/dev/pmem_adsp"
#define DEVICE_PMEM_SMIPOOL "/dev/pmem_smipool"
/*****************************************************************************/
#ifdef __cplusplus

//XXX: Remove framebuffer specific classes and defines to a different header
template <class T>
struct Node
{
    T data;
    Node<T> *next;
};

template <class T>
class Queue
{
public:
    Queue(): front(NULL), back(NULL), len(0) {dummy = new T;}
    ~Queue()
    {
        clear();
        delete dummy;
    }
    void push(const T& item)   //add an item to the back of the queue
    {
        if(len != 0) {         //if the queue is not empty
            back->next = new Node<T>; //create a new node
            back = back->next; //set the new node as the back node
            back->data = item;
            back->next = NULL;
        } else {
            back = new Node<T>;
            back->data = item;
            back->next = NULL;
            front = back;
       }
       len++;
    }
    void pop()                 //remove the first item from the queue
    {
        if (isEmpty())
            return;            //if the queue is empty, no node to dequeue
        T item = front->data;
        Node<T> *tmp = front;
        front = front->next;
        delete tmp;
        if(front == NULL)      //if the queue is empty, update the back pointer
            back = NULL;
        len--;
        return;
    }
    T& getHeadValue() const    //return the value of the first item in the queue
    {                          //without modification to the structure
        if (isEmpty()) {
            LOGE("Error can't get head of empty queue");
            return *dummy;
        }
        return front->data;
    }

    bool isEmpty() const       //returns true if no elements are in the queue
    {
        return (front == NULL);
    }

    size_t size() const        //returns the amount of elements in the queue
    {
        return len;
    }

private:
    Node<T> *front;
    Node<T> *back;
    size_t len;
    void clear()
    {
        while (!isEmpty())
            pop();
    }
    T *dummy;
};
#endif

enum {
    /* OEM specific HAL formats */
    HAL_PIXEL_FORMAT_NV12_ENCODEABLE  = 0x102,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED     = 0x108,
    HAL_PIXEL_FORMAT_YCbCr_420_SP           = 0x109,
    HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO    = 0x10A,
    HAL_PIXEL_FORMAT_YCrCb_422_SP           = 0x10B,
    HAL_PIXEL_FORMAT_R_8                    = 0x10D,
    HAL_PIXEL_FORMAT_RG_88                  = 0x10E,
    HAL_PIXEL_FORMAT_INTERLACE              = 0x180,

};

/* possible formats for 3D content*/
enum {
    HAL_NO_3D                         = 0x0000,
    HAL_3D_IN_SIDE_BY_SIDE_L_R        = 0x10000,
    HAL_3D_IN_TOP_BOTTOM              = 0x20000,
    HAL_3D_IN_INTERLEAVE              = 0x40000,
    HAL_3D_IN_SIDE_BY_SIDE_R_L        = 0x80000,
    HAL_3D_OUT_SIDE_BY_SIDE           = 0x1000,
    HAL_3D_OUT_TOP_BOTTOM             = 0x2000,
    HAL_3D_OUT_INTERLEAVE             = 0x4000,
    HAL_3D_OUT_MONOSCOPIC             = 0x8000
};

enum {
	BUFFER_TYPE_UI = 0,
	BUFFER_TYPE_VIDEO
};
/*****************************************************************************/

struct private_module_t;
struct private_handle_t;
struct PmemAllocator;

struct qbuf_t {
    buffer_handle_t buf;
    int  idx;
};

enum buf_state {
    SUB,
    REF,
    AVL
};

struct avail_t {
    pthread_mutex_t lock;
    pthread_cond_t cond;
#ifdef __cplusplus
    bool is_avail;
    buf_state state;
#endif
};

struct private_module_t {
    gralloc_module_t base;

    struct private_handle_t* framebuffer;
    uint32_t fbFormat;
    uint32_t flags;
    uint32_t numBuffers;
    uint32_t bufferMask;
    pthread_mutex_t lock;
    buffer_handle_t currentBuffer;

    struct fb_var_screeninfo info;
    struct fb_fix_screeninfo finfo;
    float xdpi;
    float ydpi;
    float fps;
    int swapInterval;
#ifdef __cplusplus
    Queue<struct qbuf_t> disp; // non-empty when buffer is ready for display
#endif
    int currentIdx;
    struct avail_t avail[NUM_FRAMEBUFFERS_MAX];
    pthread_mutex_t qlock;
    pthread_cond_t qpost;

    enum {
        // flag to indicate we'll post this buffer
        PRIV_USAGE_LOCKED_FOR_POST = 0x80000000,
        PRIV_MIN_SWAP_INTERVAL = 0,
        PRIV_MAX_SWAP_INTERVAL = 1,
    };
#if defined(__cplusplus) && defined(HDMI_DUAL_DISPLAY)
    Overlay* pobjOverlay;
    int orientation;
    bool videoOverlay;
    uint32_t currentOffset;
    bool enableHDMIOutput;
    bool exitHDMIUILoop;
    float actionsafeWidthRatio;
    float actionsafeHeightRatio;
    bool hdmiStateChanged;
    pthread_mutex_t overlayLock;
    pthread_cond_t overlayPost;
#endif
#ifdef COMPOSITION_BYPASS
    pthread_mutex_t bufferPostLock;
    pthread_cond_t bufferPostCond;
    bool bufferPostDone;
#endif
};

/*****************************************************************************/

#ifdef __cplusplus
struct private_handle_t : public native_handle {
#else
struct private_handle_t {
    native_handle_t nativeHandle;
#endif
    enum {
        PRIV_FLAGS_FRAMEBUFFER    = 0x00000001,
        PRIV_FLAGS_USES_PMEM      = 0x00000002,
        PRIV_FLAGS_USES_PMEM_ADSP = 0x00000004,
        PRIV_FLAGS_USES_ION       = 0x00000008,
        PRIV_FLAGS_USES_ASHMEM    = 0x00000010,
        PRIV_FLAGS_NEEDS_FLUSH    = 0x00000020,
        PRIV_FLAGS_DO_NOT_FLUSH   = 0x00000040,
        PRIV_FLAGS_SW_LOCK        = 0x00000080,
        PRIV_FLAGS_NONCONTIGUOUS_MEM = 0x00000100,
    };

    // file-descriptors
    int     fd;
    int     genlockHandle; // genlock handle to be dup'd by the binder
    // ints
    int     magic;
    int     flags;
    int     size;
    int     offset;
    int     bufferType;

    // FIXME: the attributes below should be out-of-line
    int     base;
    int     gpuaddr; // The gpu address mapped into the mmu. If using ashmem, set to 0 They don't care
    int     pid;
    int     format;
    int     width;
    int     height;
    int     genlockPrivFd; // local fd of the genlock device.

#ifdef __cplusplus
    static const int sNumInts = 12;
    static const int sNumFds = 2;
    static const int sMagic = 'gmsm';

    private_handle_t(int fd, int size, int flags, int bufferType, int format, int width, int height) :
        fd(fd), genlockHandle(-1), magic(sMagic), flags(flags), size(size), offset(0),
        bufferType(bufferType), base(0), gpuaddr(0), pid(getpid()), format(format),
        width(width), height(height), genlockPrivFd(-1)
    {
        version = sizeof(native_handle);
        numInts = sNumInts;
        numFds = sNumFds;
    }
    ~private_handle_t() {
        magic = 0;
    }

    bool usesPhysicallyContiguousMemory() {
        return (flags & PRIV_FLAGS_USES_PMEM) != 0;
    }

    static int validate(const native_handle* h) {
        const private_handle_t* hnd = (const private_handle_t*)h;
        if (!h || h->version != sizeof(native_handle) ||
                h->numInts != sNumInts || h->numFds != sNumFds ||
                hnd->magic != sMagic)
        {
            LOGE("invalid gralloc handle (at %p)", h);
            return -EINVAL;
        }
        return 0;
    }

    static private_handle_t* dynamicCast(const native_handle* in) {
        if (validate(in) == 0) {
            return (private_handle_t*) in;
        }
        return NULL;
    }
#endif
};

#endif /* GRALLOC_PRIV_H_ */
