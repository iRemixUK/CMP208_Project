#include <platform/vita/system/platform_vita.h>
#include <platform/vita/graphics/texture_vita.h>
#include <platform/vita/system/file_vita.h>
#include <platform/vita/graphics/sprite_renderer_vita.h>
#include <platform/vita/audio/audio_manager_vita.h>
//#include <platform/vita/input/touch_input_manager_vita.h>
//#include <platform/vita/input/sony_controller_input_manager_vita.h>
#include <platform/vita/graphics/texture_vita.h>
#include <graphics/mesh.h>
#include <platform/vita/graphics/renderer_3d_vita.h>
#include <platform/vita/graphics/render_target_vita.h>
#include <platform/vita/graphics/shader_interface_vita.h>
#include <platform/vita/graphics/vertex_buffer_vita.h>
#include <platform/vita/graphics/index_buffer_vita.h>
#include <platform/vita/graphics/depth_buffer_vita.h>
#include <platform/vita/input/input_manager_vita.h>
#include <graphics/sprite.h>
#include <maths/vector2.h>
#include <gxt.h>
#include <libdbg.h>
#include <display.h>
#include <string.h>
#include <stdlib.h>
#include <kernel.h>

// GPU Debugging
#ifdef _DEBUG
#include <libsysmodule.h>
#include <razor_capture.h>
#endif

/*	The build process for the sample embeds the shader programs directly into the
    executable using the symbols below.  This is purely for convenience, it is
    equivalent to simply load the binary file into memory and cast the contents
    to type SceGxmProgram.
    */
extern const SceGxmProgram _binary_clear_v_gxp_start;
extern const SceGxmProgram _binary_clear_f_gxp_start;

void (*custom_display_callback)(const void *callbackData) = NULL;

//
// CALLBACKS
//
void displayCallback(const void *callbackData)
{
    SceDisplayFrameBuf framebuf;
    int err = SCE_OK;

    // Cast the parameters back
    const gef::DisplayData *displayData = (const gef::DisplayData *)callbackData;

    if(custom_display_callback)
        custom_display_callback(callbackData);

    // Swap to the new buffer on the next VSYNC
    memset(&framebuf, 0x00, sizeof(SceDisplayFrameBuf));
    framebuf.size        = sizeof(SceDisplayFrameBuf);
    framebuf.base        = displayData->address;
    framebuf.pitch       = DISPLAY_STRIDE_IN_PIXELS;
    framebuf.pixelformat = DISPLAY_PIXEL_FORMAT;
    framebuf.width       = DISPLAY_WIDTH;
    framebuf.height      = DISPLAY_HEIGHT;
    err = sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_UPDATETIMING_NEXTVSYNC);
    SCE_DBG_ASSERT(err == SCE_OK);

    // Block this callback until the swap has occurred and the old buffer
    // is no longer displayed
    err = sceDisplayWaitVblankStart();
    SCE_DBG_ASSERT(err == SCE_OK);
}

#ifdef __cplusplus
extern "C"
{
#endif

void *graphicsAlloc(SceKernelMemBlockType type, uint32_t size, uint32_t alignment, uint32_t attribs, SceUID *uid)
{
    int err = SCE_OK;

    /*	Since we are using sceKernelAllocMemBlock directly, we cannot directly
        use the alignment parameter.  Instead, we must allocate the size to the
        minimum for this memblock type, and just assert that this will cover
        our desired alignment.

        Developers using their own heaps should be able to use the alignment
        parameter directly for more minimal padding.
        */
    if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RWDATA) {
        // CDRAM memblocks must be 256KiB aligned
        SCE_DBG_ASSERT(alignment <= 256*1024);
        size = ALIGN(size, 256*1024);
    } else {
        // LPDDR memblocks must be 4KiB aligned
        SCE_DBG_ASSERT(alignment <= 4*1024);
        size = ALIGN(size, 4*1024);
    }

    // allocate some memory
    *uid = sceKernelAllocMemBlock("basic", type, size, NULL);
    SCE_DBG_ASSERT(*uid >= SCE_OK);

    // grab the base address
    void *mem = NULL;
    err = sceKernelGetMemBlockBase(*uid, &mem);
    SCE_DBG_ASSERT(err == SCE_OK);

    // map for the GPU
    err = sceGxmMapMemory(mem, size, attribs);
    SCE_DBG_ASSERT(err == SCE_OK);

    // done
    return mem;
}

void graphicsFree(SceUID uid)
{
    int err = SCE_OK;

    // grab the base address
    void *mem = NULL;
    err = sceKernelGetMemBlockBase(uid, &mem);
    SCE_DBG_ASSERT(err == SCE_OK);

    // unmap memory
    err = sceGxmUnmapMemory(mem);
    SCE_DBG_ASSERT(err == SCE_OK);

    // free the memory block
    err = sceKernelFreeMemBlock(uid);
    SCE_DBG_ASSERT(err == SCE_OK);
}

#ifdef __cplusplus
} // extern "C"
#endif

static void *vertexUsseAlloc(uint32_t size, SceUID *uid, uint32_t *usseOffset)
{
    int err = SCE_OK;

    // align to memblock alignment for LPDDR
    size = ALIGN(size, 4096);

    // allocate some memory
    *uid = sceKernelAllocMemBlock("basic", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, size, NULL);
    SCE_DBG_ASSERT(*uid >= SCE_OK);

    // grab the base address
    void *mem = NULL;
    err = sceKernelGetMemBlockBase(*uid, &mem);
    SCE_DBG_ASSERT(err == SCE_OK);

    // map as vertex USSE code for the GPU
    err = sceGxmMapVertexUsseMemory(mem, size, usseOffset);
    SCE_DBG_ASSERT(err == SCE_OK);

    // done
    return mem;
}

static void vertexUsseFree(SceUID uid)
{
    int err = SCE_OK;

    // grab the base address
    void *mem = NULL;
    err = sceKernelGetMemBlockBase(uid, &mem);
    SCE_DBG_ASSERT(err == SCE_OK);

    // unmap memory
    err = sceGxmUnmapVertexUsseMemory(mem);
    SCE_DBG_ASSERT(err == SCE_OK);

    // free the memory block
    err = sceKernelFreeMemBlock(uid);
    SCE_DBG_ASSERT(err == SCE_OK);
}

static void *fragmentUsseAlloc(uint32_t size, SceUID *uid, uint32_t *usseOffset)
{
    int err = SCE_OK;

    // align to memblock alignment for LPDDR
    size = ALIGN(size, 4096);

    // allocate some memory
    *uid = sceKernelAllocMemBlock("basic", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, size, NULL);
    SCE_DBG_ASSERT(*uid >= SCE_OK);

    // grab the base address
    void *mem = NULL;
    err = sceKernelGetMemBlockBase(*uid, &mem);
    SCE_DBG_ASSERT(err == SCE_OK);

    // map as fragment USSE code for the GPU
    err = sceGxmMapFragmentUsseMemory(mem, size, usseOffset);
    SCE_DBG_ASSERT(err == SCE_OK);

    // done
    return mem;
}

static void fragmentUsseFree(SceUID uid)
{
    int err = SCE_OK;

    // grab the base address
    void *mem = NULL;
    err = sceKernelGetMemBlockBase(uid, &mem);
    SCE_DBG_ASSERT(err == SCE_OK);

    // unmap memory
    err = sceGxmUnmapFragmentUsseMemory(mem);
    SCE_DBG_ASSERT(err == SCE_OK);

    // free the memory block
    err = sceKernelFreeMemBlock(uid);
    SCE_DBG_ASSERT(err == SCE_OK);
}

static void *patcherHostAlloc(void */*userData*/, uint32_t size)
{
    return malloc(size);
}

static void patcherHostFree(void */*userData*/, void *mem)
{
    free(mem);
}

namespace gef
{
    PlatformVita::PlatformVita() :
        vertexRingBufferUid(0),
        fragmentRingBufferUid(0),
        fragmentUsseRingBufferUid(0),
        hostMem(NULL),
        context_(NULL),
        renderTarget(NULL),
        depthBufferUid(0),
        shader_patcher_(NULL),
        patcherVertexUsseUid(0),
        patcherFragmentUsseUid(0),
        patcherBufferUid(0),
        clearVertexProgramId(NULL),
        clearFragmentProgramId(NULL),
        clearVerticesUid(0),
        clearIndicesUid(0),
        clearVertexProgram(NULL),
        clearFragmentProgram(NULL),
        backBufferIndex(0),
        frontBufferIndex(0),
        frame_time_start_(0),
		fragment_shader_clear_colour_(NULL)
    {
        set_width(DISPLAY_WIDTH);
        set_height(DISPLAY_HEIGHT);

        for(Int32 buffer_index=0;buffer_index<DISPLAY_BUFFER_COUNT;++buffer_index)
        {
            displayBufferUid[buffer_index] = 0;
            displayBufferData[buffer_index] = NULL;
            displayBufferSync[buffer_index] = NULL;
        }

        Int32 return_value = Init();
        SCE_DBG_ASSERT(return_value == SCE_OK);
    }

    PlatformVita::~PlatformVita()
    {
        CleanUp();
    }

    void PlatformVita::InitGxmLibrary()
    {
        Int32 err = SCE_OK;


#ifdef _DEBUG
        err = sceSysmoduleLoadModule(SCE_SYSMODULE_RAZOR_CAPTURE);
#endif

        /* ---------------------------------------------------------------------
           2. Initialize libgxm

           We specify the default parameter buffer size of 16MiB.

           --------------------------------------------------------------------- */

        // set up parameters
        SceGxmInitializeParams initializeParams;
        memset(&initializeParams, 0, sizeof(SceGxmInitializeParams));
        initializeParams.flags							= 0;
        initializeParams.displayQueueMaxPendingCount	= DISPLAY_MAX_PENDING_SWAPS;
        initializeParams.displayQueueCallback			= displayCallback;
        initializeParams.displayQueueCallbackDataSize	= sizeof(DisplayData);
        initializeParams.parameterBufferSize			= SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

        // initialize
        err = sceGxmInitialize(&initializeParams);
        SCE_DBG_ASSERT(err == SCE_OK);
    }

    void PlatformVita::CreateRendereringContext()
    {
        Int32 err = SCE_OK;

        /* ---------------------------------------------------------------------
           3. Create a libgxm context_

           Once initialized, we need to create a rendering context_ to allow to us
           to render scenes on the GPU.  We use the default initialization
           parameters here to set the sizes of the various context_ ring buffers.
           --------------------------------------------------------------------- */

        // allocate ring buffer memory using default sizes
        void *vdmRingBuffer = graphicsAlloc(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
                SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE,
                4,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &vdmRingBufferUid);

        void *vertexRingBuffer = graphicsAlloc(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
                SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE,
                4,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &vertexRingBufferUid);

        void *fragmentRingBuffer = graphicsAlloc(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
                SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE,
                4,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &fragmentRingBufferUid);

        uint32_t fragmentUsseRingBufferOffset;
        void *fragmentUsseRingBuffer = fragmentUsseAlloc(
                SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
                &fragmentUsseRingBufferUid,
                &fragmentUsseRingBufferOffset);

        SceGxmContextParams contextParams;
        memset(&contextParams, 0, sizeof(SceGxmContextParams));
        hostMem = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
        contextParams.hostMem						= hostMem;
        contextParams.hostMemSize					= SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
        contextParams.vdmRingBufferMem				= vdmRingBuffer;
        contextParams.vdmRingBufferMemSize			= SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
        contextParams.vertexRingBufferMem			= vertexRingBuffer;
        contextParams.vertexRingBufferMemSize		= SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
        contextParams.fragmentRingBufferMem			= fragmentRingBuffer;
        contextParams.fragmentRingBufferMemSize		= SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
        contextParams.fragmentUsseRingBufferMem		= fragmentUsseRingBuffer;
        contextParams.fragmentUsseRingBufferMemSize	= SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
        contextParams.fragmentUsseRingBufferOffset	= fragmentUsseRingBufferOffset;

        err = sceGxmCreateContext(&contextParams, &context_);
        SCE_DBG_ASSERT(err == SCE_OK);
    }

    void PlatformVita::CreateRenderTarget()
    {
        Int32 err = SCE_OK;

        /* ---------------------------------------------------------------------
           4. Create a render target

           Finally we create a render target to describe the geometry of the back
           buffers we will render to.  This object is used purely to schedule
           rendering jobs for the given dimensions, the color surface and
           depth/stencil surface must be allocated separately.
           --------------------------------------------------------------------- */

        // set up parameters
        SceGxmRenderTargetParams renderTargetParams;
        memset(&renderTargetParams, 0, sizeof(SceGxmRenderTargetParams));
        renderTargetParams.flags				= 0;
        renderTargetParams.width				= DISPLAY_WIDTH;
        renderTargetParams.height				= DISPLAY_HEIGHT;
        renderTargetParams.scenesPerFrame		= 1;
        renderTargetParams.multisampleMode		= MSAA_MODE;
        renderTargetParams.multisampleLocations	= 0;
        renderTargetParams.driverMemBlock		= SCE_UID_INVALID_UID;

        // create the render target
        err = sceGxmCreateRenderTarget(&renderTargetParams, &renderTarget);
        SCE_DBG_ASSERT(err == SCE_OK);
    }

    void PlatformVita::AllocateDisplayBuffers()
    {
        Int32 err = SCE_OK;

        /* ---------------------------------------------------------------------
           5. Allocate display buffers and sync objects

           We will allocate our back buffers in CDRAM, and create a color
           surface for each of them.

           To allow display operations done by the CPU to be synchronized with
           rendering done by the GPU, we also create a SceGxmSyncObject for each
           display buffer.  This sync object will be used with each scene that
           renders to that buffer and when queueing display flips that involve
           that buffer (either flipping from or to).
           --------------------------------------------------------------------- */

        // allocate memory and sync objects for display buffers
        for (uint32_t i = 0; i < DISPLAY_BUFFER_COUNT; ++i) {
            // allocate memory for display
            displayBufferData[i] = graphicsAlloc(
                    SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RWDATA,
                    4*DISPLAY_STRIDE_IN_PIXELS*DISPLAY_HEIGHT,
                    SCE_GXM_COLOR_SURFACE_ALIGNMENT,
                    SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
                    &displayBufferUid[i]);

            // memset the buffer to black
            for (uint32_t y = 0; y < DISPLAY_HEIGHT; ++y) {
                uint32_t *row = (uint32_t *)displayBufferData[i] + y*DISPLAY_STRIDE_IN_PIXELS;
                for (uint32_t x = 0; x < DISPLAY_WIDTH; ++x) {
                    row[x] = 0xff000000;
                }
            }

            // initialize a color surface for this display buffer
            err = sceGxmColorSurfaceInit(
                    &displaySurface[i],
                    DISPLAY_COLOR_FORMAT,
                    SCE_GXM_COLOR_SURFACE_LINEAR,
                    (MSAA_MODE == SCE_GXM_MULTISAMPLE_NONE) ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
                    SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
                    DISPLAY_WIDTH,
                    DISPLAY_HEIGHT,
                    DISPLAY_STRIDE_IN_PIXELS,
                    displayBufferData[i]);
            SCE_DBG_ASSERT(err == SCE_OK);

            // create a sync object that we will associate with this buffer
            err = sceGxmSyncObjectCreate(&displayBufferSync[i]);
            SCE_DBG_ASSERT(err == SCE_OK);
        }
    }

    void PlatformVita::AllocateDepthBuffer()
    {
        Int32 err = SCE_OK;

        /* ---------------------------------------------------------------------
           6. Allocate a depth buffer

           Note that since this sample renders in a strictly back-to-front order,
           a depth buffer is not strictly required.  However, since it is usually
           necessary to create one to handle partial renders, we will create one
           now.  Note that we do not enable force load or store, so this depth
           buffer will not actually be read or written by the GPU when this
           sample is executed, so will have zero performance impact.
           --------------------------------------------------------------------- */

        // compute the memory footprint of the depth buffer
        const uint32_t alignedWidth = ALIGN(DISPLAY_WIDTH, SCE_GXM_TILE_SIZEX);
        const uint32_t alignedHeight = ALIGN(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);
        uint32_t sampleCount = alignedWidth*alignedHeight;
        uint32_t depthStrideInSamples = alignedWidth;
        if (MSAA_MODE == SCE_GXM_MULTISAMPLE_4X) {
            // samples increase in X and Y
            sampleCount *= 4;
            depthStrideInSamples *= 2;
        } else if (MSAA_MODE == SCE_GXM_MULTISAMPLE_2X) {
            // samples increase in Y only
            sampleCount *= 2;
        }

        // allocate it
        void *depthBufferData = graphicsAlloc(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
                4*sampleCount,
                SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT,
                SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
                &depthBufferUid);

        // create the SceGxmDepthStencilSurface structure
        err = sceGxmDepthStencilSurfaceInit(
                &depthSurface,
                SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
                SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
                depthStrideInSamples,
                depthBufferData,
                NULL);
        SCE_DBG_ASSERT(err == SCE_OK);
    }

    void PlatformVita::CreateShaderPatcher()
    {
        Int32 err = SCE_OK;

        /* ---------------------------------------------------------------------
           7. Create a shader patcher and register programs

           A shader patcher object is required to produce vertex and fragment
           programs from the shader compiler output.  First we create a shader
           patcher instance, using callback functions to allow it to allocate
           and free host memory for internal state.

           We will use the shader patcher's internal heap to handle buffer
           memory and USSE memory for the final programs.  To do this, we
           leave the callback functions as NULL, but provide static memory
           blocks.

           In order to create vertex and fragment programs for a particular
           shader, the compiler output must first be registered to obtain an ID
           for that shader.  Within a single ID, vertex and fragment programs
           are reference counted and could be shared if created with identical
           parameters.  To maximise this sharing, programs should only be
           registered with the shader patcher once if possible, so we will do
           this now.
           --------------------------------------------------------------------- */

        // set buffer sizes for the application
        const uint32_t patcherBufferSize		= 64*1024;
        const uint32_t patcherVertexUsseSize	= 64*1024;
        const uint32_t patcherFragmentUsseSize	= 64*1024;

        // allocate memory for buffers and USSE code
        void *patcherBuffer = graphicsAlloc(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
                patcherBufferSize,
                4,
                SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
                &patcherBufferUid);

        uint32_t patcherVertexUsseOffset;
        void *patcherVertexUsse = vertexUsseAlloc(
                patcherVertexUsseSize,
                &patcherVertexUsseUid,
                &patcherVertexUsseOffset);

        uint32_t patcherFragmentUsseOffset;
        void *patcherFragmentUsse = fragmentUsseAlloc(
                patcherFragmentUsseSize,
                &patcherFragmentUsseUid,
                &patcherFragmentUsseOffset);

        // create a shader patcher
        SceGxmShaderPatcherParams patcherParams;
        memset(&patcherParams, 0, sizeof(SceGxmShaderPatcherParams));
        patcherParams.userData					= NULL;
        patcherParams.hostAllocCallback			= &patcherHostAlloc;
        patcherParams.hostFreeCallback			= &patcherHostFree;
        patcherParams.bufferAllocCallback		= NULL;
        patcherParams.bufferFreeCallback		= NULL;
        patcherParams.bufferMem					= patcherBuffer;
        patcherParams.bufferMemSize				= patcherBufferSize;
        patcherParams.vertexUsseAllocCallback	= NULL;
        patcherParams.vertexUsseFreeCallback	= NULL;
        patcherParams.vertexUsseMem				= patcherVertexUsse;
        patcherParams.vertexUsseMemSize			= patcherVertexUsseSize;
        patcherParams.vertexUsseOffset			= patcherVertexUsseOffset;
        patcherParams.fragmentUsseAllocCallback	= NULL;
        patcherParams.fragmentUsseFreeCallback	= NULL;
        patcherParams.fragmentUsseMem			= patcherFragmentUsse;
        patcherParams.fragmentUsseMemSize		= patcherFragmentUsseSize;
        patcherParams.fragmentUsseOffset		= patcherFragmentUsseOffset;

        err = sceGxmShaderPatcherCreate(&patcherParams, &shader_patcher_);
        SCE_DBG_ASSERT(err == SCE_OK);

    }

    void PlatformVita::InitClearScreen()
    {
        Int32 err = SCE_OK;

        // clear screen shader programs
        // use embedded GXP files
        const SceGxmProgram *const clearVertexProgramGxp	= &_binary_clear_v_gxp_start;
        const SceGxmProgram *const clearFragmentProgramGxp	= &_binary_clear_f_gxp_start;

        // register programs with the patcher
        err = sceGxmShaderPatcherRegisterProgram(shader_patcher_, clearVertexProgramGxp, &clearVertexProgramId);
        SCE_DBG_ASSERT(err == SCE_OK);
        err = sceGxmShaderPatcherRegisterProgram(shader_patcher_, clearFragmentProgramGxp, &clearFragmentProgramId);
        SCE_DBG_ASSERT(err == SCE_OK);

        /* ---------------------------------------------------------------------
           8. Create the programs and data for the clear

           On SGX hardware, vertex programs must perform the unpack operations
           on vertex data, so we must define our vertex formats in order to
           create the vertex program.  Similarly, fragment programs must be
           specialized based on how they output their pixels and MSAA mode.

           We define the clear geometry vertex format here and create the vertex
           and fragment program.

           The clear vertex and index data is static, we allocate and write the
           data here.
           --------------------------------------------------------------------- */

        // get attributes by name to create vertex format bindings
        const SceGxmProgramParameter *paramClearPositionAttribute = sceGxmProgramFindParameterByName(clearVertexProgramGxp, "aPosition");
        SCE_DBG_ASSERT(paramClearPositionAttribute && (sceGxmProgramParameterGetCategory(paramClearPositionAttribute) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE));

        fragment_shader_clear_colour_ = sceGxmProgramFindParameterByName(clearFragmentProgramGxp, "clear_colour");
        SCE_DBG_ASSERT(fragment_shader_clear_colour_ && (sceGxmProgramParameterGetCategory(fragment_shader_clear_colour_) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM));


        // create clear vertex format
        SceGxmVertexAttribute clearVertexAttributes[1];
        SceGxmVertexStream clearVertexStreams[1];
        clearVertexAttributes[0].streamIndex = 0;
        clearVertexAttributes[0].offset = 0;
        clearVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
        clearVertexAttributes[0].componentCount = 2;
        clearVertexAttributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(paramClearPositionAttribute);
        clearVertexStreams[0].stride = sizeof(ClearVertex);
        clearVertexStreams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

        // create sclear programs
        err = sceGxmShaderPatcherCreateVertexProgram(
                shader_patcher_,
                clearVertexProgramId,
                clearVertexAttributes,
                1,
                clearVertexStreams,
                1,
                &clearVertexProgram);
        SCE_DBG_ASSERT(err == SCE_OK);
        err = sceGxmShaderPatcherCreateFragmentProgram(
                shader_patcher_,
                clearFragmentProgramId,
                SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
                MSAA_MODE,
                NULL,
                clearVertexProgramGxp,
                &clearFragmentProgram);
        SCE_DBG_ASSERT(err == SCE_OK);

        // create the clear triangle vertex/index data
        clearVertices = (ClearVertex *)graphicsAlloc(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
                3*sizeof(ClearVertex),
                4,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &clearVerticesUid);
        clearIndices = (uint16_t *)graphicsAlloc(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
                3*sizeof(uint16_t),
                2,
                SCE_GXM_MEMORY_ATTRIB_READ,
                &clearIndicesUid);

        clearVertices[0].x = -1.0f;
        clearVertices[0].y = -1.0f;
        clearVertices[1].x =  3.0f;
        clearVertices[1].y = -1.0f;
        clearVertices[2].x = -1.0f;
        clearVertices[2].y =  3.0f;

        clearIndices[0] = 0;
        clearIndices[1] = 1;
        clearIndices[2] = 2;
    }


    Int32 PlatformVita::Init()
    {
        Int32 err = SCE_OK;

        InitGxmLibrary();
        CreateRendereringContext();
        CreateRenderTarget();
        AllocateDisplayBuffers();
        AllocateDepthBuffer();
        CreateShaderPatcher();
        InitClearScreen();

		// create default texture
		default_texture_ = Texture::CreateCheckerTexture(16, 1, *this);
		AddTexture(default_texture_);

        return err;
    }

    void PlatformVita::CleanUp()
    {
        Int32 err = SCE_OK;

        /* ---------------------------------------------------------------------
           14. Wait for rendering to complete and shut down

           Since there could be many queued operations not yet completed it is
           important to wait until the GPU has finished before destroying
           resources.  We do this by calling sceGxmFinish for our rendering
           context_.

           Once the GPU is finished, we release all our programs, deallocate
           all our memory, destroy all object and finally terminate libgxm.
           --------------------------------------------------------------------- */

        // wait until rendering is done
        sceGxmFinish(context_);


        // clean up allocations
        sceGxmShaderPatcherReleaseFragmentProgram(shader_patcher_, clearFragmentProgram);
        sceGxmShaderPatcherReleaseVertexProgram(shader_patcher_, clearVertexProgram);
        graphicsFree(clearIndicesUid);
        graphicsFree(clearVerticesUid);

        // wait until display queue is finished before deallocating display buffers
        err = sceGxmDisplayQueueFinish();
        SCE_DBG_ASSERT(err == SCE_OK);

        // clean up display queue
        graphicsFree(depthBufferUid);
        for (uint32_t i = 0; i < DISPLAY_BUFFER_COUNT; ++i) {
            // clear the buffer then deallocate
            memset(displayBufferData[i], 0, DISPLAY_HEIGHT*DISPLAY_STRIDE_IN_PIXELS*4);
            graphicsFree(displayBufferUid[i]);

            // destroy the sync object
            sceGxmSyncObjectDestroy(displayBufferSync[i]);
        }

        // unregister programs and destroy shader patcher
        sceGxmShaderPatcherUnregisterProgram(shader_patcher_, clearFragmentProgramId);
        sceGxmShaderPatcherUnregisterProgram(shader_patcher_, clearVertexProgramId);

        sceGxmShaderPatcherDestroy(shader_patcher_);
        fragmentUsseFree(patcherFragmentUsseUid);
        vertexUsseFree(patcherVertexUsseUid);
        graphicsFree(patcherBufferUid);

        // destroy the render target
        sceGxmDestroyRenderTarget(renderTarget);

        // destroy the context_
        sceGxmDestroyContext(context_);
        fragmentUsseFree(fragmentUsseRingBufferUid);
        graphicsFree(fragmentRingBufferUid);
        graphicsFree(vertexRingBufferUid);
        graphicsFree(vdmRingBufferUid);
        free(hostMem);
        hostMem = NULL;

        // terminate libgxm
        sceGxmTerminate();
#ifdef _DEBUG
        sceSysmoduleUnloadModule(SCE_SYSMODULE_RAZOR_CAPTURE);
#endif
    }

    void PlatformVita::PreRender()
    {
        /* -----------------------------------------------------------------
           12. Rendering step

           This sample renders a single scene containing the two triangles,
           the clear triangle followed by the spinning triangle.  Before
           any drawing can take place, a scene must be started.  We render
           to the back buffer, so it is also important to use a sync object
           to ensure that these rendering operations are synchronized with
           display operations.

           The clear triangle shaders do not declare any uniform variables,
           so this may be rendered immediately after setting the vertex and
           fragment program.

           The spinning triangle vertex program declares a matrix parameter,
           so this forms part of the vertex default uniform buffer and must
           be written before the triangle can be drawn.

           Once both triangles have been drawn the scene can be ended, which
           submits it for rendering on the GPU.
           ----------------------------------------------------------------- */

        // start rendering to the main render target
        /*
           sceGxmBeginScene(
           context_,
           0,
           renderTarget,
           NULL,
           NULL,
           displayBufferSync[backBufferIndex],
           &displaySurface[backBufferIndex],
           &depthSurface);

           DrawClearScreen();
           */
    }

    void PlatformVita::PostRender()
    {
        // end the scene on the main render target, submitting rendering work to the GPU
        //			SceGxmErrorCode error_code = sceGxmEndScene(context_, NULL, NULL);

#ifdef _DEBUG
        // PA heartbeat to notify end of frame
        sceGxmPadHeartbeat(&displaySurface[backBufferIndex], displayBufferSync[backBufferIndex]);
#endif

        /* -----------------------------------------------------------------
           13. Flip operation

           Now we have finished submitting rendering work for this frame it
           is time to submit a flip operation.  As part of specifying this
           flip operation we must provide the sync objects for both the old
           buffer and the new buffer.  This is to allow synchronization both
ways: to not flip until rendering is complete, but also to ensure
that future rendering to these buffers does not start until the
flip operation is complete.

The callback function will be called from an internal thread once
queued GPU operations involving the sync objects is complete.
Assuming we have not reached our maximum number of queued frames,
this function returns immediately.

Once we have queued our flip, we manually cycle through our back
buffers before starting the next frame.
----------------------------------------------------------------- */

        // queue the display swap for this frame
        DisplayData displayData;
        displayData.address = displayBufferData[backBufferIndex];
        sceGxmDisplayQueueAddEntry(
                displayBufferSync[frontBufferIndex],	// front buffer is OLD buffer
                displayBufferSync[backBufferIndex],		// back buffer is NEW buffer
                &displayData);

        // update buffer indices
        frontBufferIndex = backBufferIndex;
        backBufferIndex = (backBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
    }

    void PlatformVita::DrawClearScreen() const
    {
        Int32 err = SCE_OK;

        // set clear shaders
        sceGxmSetVertexProgram(context_, clearVertexProgram);
        sceGxmSetFragmentProgram(context_, clearFragmentProgram);

        void *fragment_shader_data_buffer;
        sceGxmReserveFragmentDefaultUniformBuffer(context_, &fragment_shader_data_buffer);
        sceGxmSetUniformDataF(fragment_shader_data_buffer, fragment_shader_clear_colour_, 0, 4, &render_target_clear_colour_.r);

        // draw the clear triangle
        err = sceGxmSetVertexStream(context_, 0, clearVertices);
        SCE_DBG_ASSERT(err == SCE_OK);
        err = sceGxmDraw(context_, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, clearIndices, 3);
        SCE_DBG_ASSERT(err == SCE_OK);
    }

    void PlatformVita::Clear() const
    {
        DrawClearScreen();
    }


    void PlatformVita::BeginScene() const
    {
		const SceGxmDepthStencilSurface* gxm_depth_stencil_surface = NULL;
		const SceGxmColorSurface* gxm_color_surface = NULL;
		const SceGxmRenderTarget* gxm_render_target = NULL;
		SceGxmSyncObject* gxm_sync = NULL;

		// check to see if user created depth buffer has been set
		if (depth_buffer())
		{
			// get depth/stencil surface from set depth buffer
			gef::DepthBufferVita* depth_buffer_vita = static_cast<gef::DepthBufferVita*>(depth_buffer());
			gxm_depth_stencil_surface = depth_buffer_vita->depth_stencil_surface();
		}
		else
		{
			// use default depth/stencil surface
			gxm_depth_stencil_surface = &depthSurface;
		}


		// check to see if user created render target has been set
        if(render_target())
        {
			// get render target surfaces from set render target
			gef::RenderTargetVita* render_target_vita = static_cast<gef::RenderTargetVita*>(render_target());
			gxm_render_target = render_target_vita->render_target();
			gxm_color_surface = render_target_vita->colour_surface();
			gxm_sync = NULL;
        }
		else
		{
			// use back buffer as the render target
			gxm_render_target = renderTarget;
			gxm_color_surface = &displaySurface[backBufferIndex];
			gxm_sync = displayBufferSync[backBufferIndex];
		}

        Int32 err = SCE_OK;

        // start rendering to the main render target
        err = sceGxmBeginScene(
                context_,
                0,
				gxm_render_target,
                NULL,
                NULL,
				gxm_sync,
				gxm_color_surface,
				gxm_depth_stencil_surface);
        SCE_DBG_ASSERT(err == SCE_OK);
    }

    void PlatformVita::EndScene() const
    {
#if 0
        if(render_target())
        {
            render_target()->End(*this);
        }
        else
#endif
        {
            // end the scene on the main render target, submitting rendering work to the GPU
            SceGxmErrorCode err = sceGxmEndScene(context_, NULL, NULL);
            SCE_DBG_ASSERT(err == SCE_OK);
        }

		// wait until rendering has finished in case 
		sceGxmFinish(context_);
    }

    SceGxmShaderPatcherId PlatformVita::RegisterShaderProgram(const SceGxmProgram* program)
    {
        SceGxmShaderPatcherId result;
        Int32 err = SCE_OK;
        err = sceGxmShaderPatcherRegisterProgram(shader_patcher_, program, &result);
        SCE_DBG_ASSERT(err == SCE_OK);

        return result;
    }

    bool PlatformVita::Update()
    {
        return true;
    }

    float PlatformVita::GetFrameTime()
    {
        if(frame_time_start_ == 0)
            frame_time_start_ = sceKernelGetProcessTimeWide();
        SceUInt64 frame_time_end = sceKernelGetProcessTimeWide(); // returns time in milliseconds
        SceUInt32 frame_cycles = static_cast<SceUInt32>(frame_time_end - frame_time_start_);
        float frame_time = (float)frame_cycles / 1000000.f;
        frame_time_start_ = frame_time_end;

        return frame_time;
    }

    std::string PlatformVita::FormatFilename(const std::string& filename) const
    {
        return FormatFilename(filename.c_str());
    }

    std::string PlatformVita::FormatFilename(const char* filename) const
    {
        std::string platform_filename(filename);
        platform_filename = "app0:/"+platform_filename;
        return platform_filename;
    }


    SpriteRenderer* PlatformVita::CreateSpriteRenderer()
    {
        return new SpriteRendererVita(*this);
    }

    File* PlatformVita::CreateFile() const
    {
        return new FileVita();
    }

    AudioManager* PlatformVita::CreateAudioManager() const
    {
        return new AudioManagerVita();
    }

    //TouchInputManager* PlatformVita::CreateTouchInputManager() const
    //{
    //    //		return new TouchInputManagerVita();
    //    return NULL;
    //}

    Texture* PlatformVita::CreateTexture(const ImageData& image_data) const
    {
        return new TextureVita(*this, image_data);
    }

    Mesh* PlatformVita::CreateMesh()
    {
        return new Mesh(*this);
    }

    //SonyControllerInputManager* PlatformVita::CreateSonyControllerInputManager() const
    //{
    //    return new SonyControllerInputManagerVita(*this);
    //}

    Renderer3D* PlatformVita::CreateRenderer3D()
    {
        return new Renderer3DVita(*this);
    }

    Matrix44 PlatformVita::PerspectiveProjectionFov(const float fov, const float aspect_ratio, const float near_distance, const float far_distance) const
    {
        Matrix44 projection;
        projection.PerspectiveFovGL(fov, aspect_ratio, near_distance, far_distance);
        return projection;
    }

    Matrix44 PlatformVita::PerspectiveProjectionFrustum(const float left, const float right, const float top, const float bottom, const float near_distance, const float far_distance) const
    {
        Matrix44 projection_matrix;
        projection_matrix.PerspectiveFrustumGL(left, right, top, bottom, near_distance, far_distance);
        return projection_matrix;
    }

    Matrix44 PlatformVita::OrthographicFrustum(const float left, const float right, const float top, const float bottom, const float near_distance, const float far_distance) const
    {
        Matrix44 projection_matrix;
        projection_matrix.OrthographicFrustumGL(left, right, top, bottom, near_distance, far_distance);
        return projection_matrix;
    }

    RenderTarget* PlatformVita::CreateRenderTarget(const Int32 width, const Int32 height) const
    {
        //		return new RenderTargetVita(*this, width, height);
        return NULL;
    }


    void PlatformVita::InitTouchInputManager()
    {

    }

    void PlatformVita::ReleaseTouchInputManager()
    {

    }

    VertexBuffer* PlatformVita::CreateVertexBuffer() const
    {
        return new VertexBufferVita();
    }

    IndexBuffer* PlatformVita::CreateIndexBuffer() const
    {
        return new IndexBufferVita();
    }

    ShaderInterface* PlatformVita::CreateShaderInterface() const
    {
        return new ShaderInterfaceVita(context_, shader_patcher_);
    }

    const char* PlatformVita::GetShaderDirectory() const
    {
        return "vita";
    }

    const char* PlatformVita::GetShaderFileExtension() const
    {
        return "gxp";
    }

    DepthBuffer* PlatformVita::CreateDepthBuffer(UInt32 width, UInt32 height) const
    {
        return NULL;
    }

	InputManager* PlatformVita::CreateInputManager()
	{
		return new InputManagerVita(*this);
	}

}
