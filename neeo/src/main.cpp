#include "main.h"
#include "contiki_communication/communication.h"
#include "contiki_communication/commands.h"
#include "contiki_communication/uart_protocol.h"
#include "board_config.h"
#include "log_helper.h"
#include "version.h"
#include "msleep.h"
#include "cyg/crc/crc.h"
#include "romfs.h"
#include "flashfs.h"
#include "lg4573a.h"
#include "dma2d.h"
#include "ltdc.h"
#include "battery_monitor.h"
#include "common_types.h"
#include "app_config.h"
#include <cyg/io/flash.h>
#include <cyg/io/watchdog.hxx>
#ifdef CYGPKG_PROFILE_GPROF
#include <cyg/profile/profile.h>
#endif

#define FW_MAJOR_VERSION    "2"

#define W32(a, b) (*(volatile unsigned int   *)(a)) = b
#define R32(a)    (*(volatile unsigned int   *)(a))

#ifdef BOARD_EVAL
#define RESET_LEVEL 1
#define LED1 CYGHWR_HAL_STM32_PIN_OUT(G, 6, PUSHPULL, NONE, FAST)
#define LED2 CYGHWR_HAL_STM32_PIN_OUT(G, 7, PUSHPULL, NONE, FAST)
#define LED3 CYGHWR_HAL_STM32_PIN_OUT(G, 10, PUSHPULL, NONE, FAST)
#define LED4 CYGHWR_HAL_STM32_PIN_OUT(G, 12, PUSHPULL, NONE, FAST)
#define LCDBL_PWM CYGHWR_HAL_STM32_PIN_OUT(C, 6, PUSHPULL, NONE, FAST)
#define LCD_RESET CYGHWR_HAL_STM32_PIN_OUT(C, 7, PUSHPULL, PULLUP, FAST)
#define TOUCH_RESET CYGHWR_HAL_STM32_PIN_OUT(B, 11, PUSHPULL, NONE, FAST)
#define INTERRUPT_TOUCH CYGHWR_HAL_STM32_PIN_IN(F, 9, NONE)
#define INT_TOUCH_EXTI CYGNUM_HAL_INTERRUPT_EXTI9
#endif

#ifdef BOARD_TR2
#define RESET_LEVEL 0
#ifdef BOARD_TR2_V2
#define LCDBL_PWM CYGHWR_HAL_STM32_PIN_OUT(D, 13, PUSHPULL, PULLDOWN, FAST)
#define FPGA_EN CYGHWR_HAL_STM32_PIN_OUT(D, 12, PUSHPULL, PULLDOWN, FAST)
#else
#define LCDBL_PWM CYGHWR_HAL_STM32_PIN_OUT(A, 8, PUSHPULL, PULLDOWN, FAST)
#endif
#define LCD_RESET CYGHWR_HAL_STM32_PIN_OUT(C, 13, PUSHPULL, PULLDOWN, FAST)
#define TOUCH_RESET CYGHWR_HAL_STM32_PIN_OUT(A, 3, PUSHPULL, NONE, FAST)
#define INTERRUPT_TOUCH CYGHWR_HAL_STM32_PIN_IN(B, 1, NONE)
#define WIFI_SUP_EN CYGHWR_HAL_STM32_PIN_OUT(C, 7, PUSHPULL, NONE, HIGH)
#define CHG_MODE CYGHWR_HAL_STM32_PIN_OUT(D, 11, PUSHPULL, NONE, FAST)
#define CHG_CHRG CYGHWR_HAL_STM32_PIN_IN(B, 15, PULLUP)
#define INT_TOUCH_EXTI CYGNUM_HAL_INTERRUPT_EXTI1
#define LIS2DH_INT1 CYGHWR_HAL_STM32_PIN_IN(J, 8, NONE)
#define LIS2DH_INT2 CYGHWR_HAL_STM32_PIN_IN(J, 10, NONE)
#endif

//#define TOUCH_REPORTING
#define LOW_POWER

static sem_t LoadingSem, TouchProcesSem, DMA2D_Sem;
static pthread_mutex_t JNSerialMutex;

static pthread_mutex_t RequestCollectionMutex;

static bool offline_mode = true;
static cyg_interrupt ReloadInt, DMA2DInt;
static cyg_handle_t ReloadIntHandle, DMA2DIntHandle;
static pthread_mutex_t DisplayThreadSuspendMutex;
static int DisplayThreadSuspendFlag;
static pthread_cond_t DisplayThreadResumeCond;
static sem_t DisplayThreadSuspendedSem;

pthread_attr_t DisplayThreadAttr, InputThreadAttr, TouchThreadAttr, JNThreadAttr,
    FpsThreadAttr, StateDetectionThreadAttr, GUIInitializationThreadAttr, FirmwareUpdatesThreadAttr, BatteryThreadAttr, ContentCollectorAttr;
pthread_t DisplayThreadHandle, InputThreadHandle, TouchThreadHandle, JNThreadHandle,
    FpsThreadHandle, StateDetectionThreadHandle, FirmwareUpdatesThreadHandle, BatteryThreadHandle, ContentCollectorHandle;
void *DisplayThread(void*);
void *InputThread(void*);
void *TouchThread(void*);
void *FpsThread(void*);
void *StateDetectionThread(void*);
void *JNThread(void*);
void *FirmwareUpdatesThread(void*);
void *BatteryThread(void*);
void *ContentCollectorThread(void*);
void UploadFirmwareToJn(uint8_t * jn_firmware, uint32_t jn_firmware_size);
void update_callback(const char* xml, uint16_t xml_length);
void set_thread_prio(pthread_attr_t attr, pthread_t pthread, int prio);

int BuildGUI(file_t* file);
void RequestCallback(string content);
void CancelCallback(string content);

using namespace guilib;

Manager* DispMan;

struct GTP_DATA TouchData;

file_t PreguiXml;
file_t GlobalXml;
file_t list1;
file_t list2;

static int firmwareCheckVal = -1;
static bool pairingCancel = false;
void test_on_chip_flash(void);

uint32_t data;
int wifiInit = 0;
bool doNotSleep = false;
zero_config_t conf;

#ifdef LOW_POWER
#define LCD_OFF     0
#define LCD_ON      1
int lcd_state = LCD_ON;
struct timeval last_activity;
#endif

uint32_t* backupRam = (uint32_t*) 0x40024000;

int emulator = 0;

using namespace ustl;

void MarkActivity() {
    gettimeofday(&last_activity, NULL);
}

void EnableBackupRam() {
    RCC->APB1ENR = 1<<28; //PWR on
    PWR->CR = 1<<8; //Access enable
    RCC->AHB1ENR = RCC_AHB1ENR_BKPSRAMEN; //Enable clock
}

/// CALLBACKS START 
/// ****

#define CALLBACK(name) static void GUI_CALLBACK_ ##name(void* sender, const Event::ArgVector& Args)
#define CODE(x) x
#include "callbacks.inc"
#undef CALLBACK
#undef CODE
#define CALLBACK(name) (void*)&GUI_CALLBACK_## name,
#define CODE(x)
void* callback_funcs[] = {
#include "callbacks.inc"
NULL
};
#undef CALLBACK
#define CALLBACK(name) #name,
string callback_func_names[] = {
#include "callbacks.inc"
};
#undef CODE
#undef CALLBACK

void UpdateTest(void* sender, const Event::ArgVector& Args){
    doNotSleep = true;
    if(!wifiInit){
        log_msg(LOG_INFO, __cfunc__, "Wifi not connected");
        DispMan->ShowPopup("update", "WiFi not connected", 2000);
    } else { //Updating
        file_t binaryFile;
        DispMan->ShowPopup("update", "Downloading...");
        DispMan->GetPopupInstance()->GetMessagePtr()->SetForegroundColor(COLOR_ARGB8888_GRAY);
        DispMan->Draw();
        wifi_download("app.bin", (const char*)conf.host_ip, &binaryFile.size, &binaryFile.data);
        DispMan->ShowPopup("update", "Writing to FLASH...");
        DispMan->Draw();

        uint32_t crcsum_wifi = cyg_posix_crc32(binaryFile.data, binaryFile.size);

        File tFile("/flash/image");

        log_msg(LOG_INFO, __cfunc__, "Writing");
        tFile.WriteFromBuffer(binaryFile.data, binaryFile.size);

        uint8_t *testbuffer = (uint8_t*)custom_malloc(tFile.GetSize());

        DispMan->ShowPopup("update", "Reading from FLASH...");
        DispMan->Draw();

        tFile.ReadToBuffer(testbuffer);

        uint32_t crcsum_file = cyg_posix_crc32(testbuffer, tFile.GetSize());

        log_msg(LOG_INFO, __cfunc__, "CRC_wifi: 0x%08x; CRC_file: 0x%08x;", crcsum_wifi, crcsum_file);

        if(crcsum_file == crcsum_wifi) {
            DispMan->GetPopupInstance()->GetMessagePtr()->SetForegroundColor(COLOR_ARGB8888_DARKGREEN);
            DispMan->ShowPopup("update", "CRC Correct! - rebooting");
            DispMan->Draw();

            EnableBackupRam();  //Init backup ram
            backupRam[0] = 1;   //1 - update flash after reset;

            cyg_thread_delay(2000);
            Reset();
        } else {
            DispMan->ShowPopup("update", "CRC Error!", 2000);
            DispMan->GetPopupInstance()->GetMessagePtr()->SetForegroundColor(COLOR_ARGB8888_DARKRED);
        }
    }
    MarkActivity();
    doNotSleep = false;
}

void ResetDevice(void* sender, const Event::ArgVector& Args){
    Reset();
}
// END OF CALLBACKS
// ****

int32_t total_frames = 0;

void Reset(){
    SCB->AIRCR= 0x5FA0004;
    while(1);
}

cyg_uint32 ReloadIsr(cyg_vector_t vector, cyg_addrword_t data) {
    int dsr_required = 0;
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);
    W32(_LTDC_OFFSET + _LTDC_ICR, _LTDC_ICR_CLIF | _LTDC_ICR_CRRIF);
    uint32_t active_line = R32(_LTDC_OFFSET + _LTDC_LIPCR);
    uint32_t*val = (uint32_t*)data;
    if ((active_line == LAST_LINE) || (active_line == FIRST_LINE)) {
        val[0] = active_line;
        dsr_required = 1;
    }
    cyg_interrupt_unmask(vector);
    return dsr_required ? (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR) : CYG_ISR_HANDLED;
}

void ReloadDsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data) {
    uint32_t *active_line = (uint32_t*)data;
    switch (active_line[0]) {
    	case FIRST_LINE:
	        FirstLineAchieved();
                total_frames++;
		break;
	case LAST_LINE:
	        ReloadCompleted();
		break;
	default:
        	// THIS SHOULD NOT HAPPEN
		break;
    }
}

cyg_uint32 DMA2D_ISR(cyg_vector_t vector, cyg_addrword_t data) {
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);
    uint32_t val = R32(_DMA2D_OFFSET + _DMA2D_CR);
    W32(_DMA2D_OFFSET + _DMA2D_CR, val & ~_DMA2D_CR_TCIE);
    W32(_DMA2D_OFFSET + _DMA2D_ISR, _DMA2D_ISR_TCIF);
    return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;
}

void DMA2D_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data) {
        sem_post(&DMA2D_Sem);
        cyg_interrupt_unmask(vector);
}

void dma_fill(uintptr_t outputMem, uint32_t PixelsPerLine, uint32_t NumberOfLines, uint32_t outOffset, uint32_t color_index, uint32_t outPixelFormat) {
  if ((PixelsPerLine == 0) || (NumberOfLines == 0) || (PixelsPerLine > 1024) || (NumberOfLines > 1024) || (outOffset > 0x3FFF)) {
  	log_msg(LOG_ERROR, "dma", "ERROR: Width/Height is wrong");
	return;
  }
  dma2d_dma_fill(outputMem, PixelsPerLine, NumberOfLines, outOffset, color_index, outPixelFormat);
  sem_wait(&DMA2D_Sem);
}

void dma_operation(uintptr_t inputMem, uintptr_t backgroundMem, uintptr_t outputMem, uint32_t PixelsPerLine,
        uint32_t NumberOfLines, uint32_t inOffset, uint32_t backgroundOffset, uint32_t outOffset, uint32_t inPixelFormat, uint32_t backgroundPixelFormat, uint32_t outPixelFormat, uint32_t frontColor) {
    if ((PixelsPerLine == 0) || (NumberOfLines == 0) || (PixelsPerLine > 1024) || (NumberOfLines > 1024) || (outOffset > 0x3FFF)) {
        log_msg(LOG_ERROR, "dma", "ERROR: Width/Height is wrong");
        return;
    }
    dma2d_dma_operation(inputMem, backgroundMem, outputMem, PixelsPerLine, NumberOfLines, inOffset, backgroundOffset, outOffset, inPixelFormat, backgroundPixelFormat, outPixelFormat, frontColor);
    sem_wait(&DMA2D_Sem);
}

void set_thread_prio(pthread_attr_t attr, pthread_t pthread, int prio) {
  struct sched_param schedparam;
  schedparam.sched_priority = prio;
  pthread_attr_setschedparam(&attr, &schedparam);
  pthread_setschedparam(pthread, SCHED_RR, &schedparam);
}

static ListItem* CreateListItem(string artist, string songTitle, int32_t width, string imageId) {
    ListItem* TempListItem;
    TempListItem = new ListItem();
    TempListItem->SetBackgroundColor(COLOR_ARGB8888_WHITE);
    TempListItem->SetForegroundColor(COLOR_ARGB8888_DARKGRAY);
    TempListItem->SetDescriptionColor(COLOR_ARGB8888_DARKBLUE);
    TempListItem->SetType(ListItem::StdListField);
    TempListItem->SetSize(width, 110);
    TempListItem->SetImage(Image());
    DispMan->BindImageContentToImage(imageId.c_str(), TempListItem->GetImagePtr());
    TempListItem->SetTextFont(DispMan->TryGetFontPtr("large"));
    TempListItem->SetDescriptionFont(DispMan->TryGetFontPtr("small"));
    TempListItem->SetDescription(songTitle.c_str());
    TempListItem->SetText(artist.c_str());
    return TempListItem;
}

int BuildGUI(file_t* file) {
    int ParsingErrorCode = -1;
    GetFontsFromXML((char*)file->data, file->size, false);
    ParsingErrorCode = DispMan->BuildFromXML((char*)file->data, file->size);
    if(ParsingErrorCode == -1){
        return -1;
    }
    else {
        appMainConfig.load(file);
    }

    string songs [] = {"Song 1", "Song 2", "Song 3"};
    string artists [] = {"Art 1", "Art 2", "Art 3"};

    ListView* List = (ListView*)DispMan->GetScreen("StaticList");
    if(List){
        for(int k = 0; k < 3; k++) {
            for(int l = 0; l < 3; l++) {
              List->AddElement(
                      CreateListItem(artists[k], songs[l], List->GetWidth(), k==1?"art1.png":"art2.png")
                  );
            }
        }
    }


    return 0;
}

void gui_printf(const char* text, va_list argList){
    vlog_msg(LOG_INFO, "Gui Message", text, argList);
}

void suspendDisplayThread(void)
{
    pthread_mutex_lock(&DisplayThreadSuspendMutex);
    DisplayThreadSuspendFlag = 1;
    pthread_mutex_unlock(&DisplayThreadSuspendMutex);
    sem_wait(&DisplayThreadSuspendedSem);
}

void resumeDisplayThread(void)
{
    pthread_mutex_lock(&DisplayThreadSuspendMutex);
    DisplayThreadSuspendFlag = 0;
    pthread_cond_broadcast(&DisplayThreadResumeCond);
    pthread_mutex_unlock(&DisplayThreadSuspendMutex);
}

void checkSuspendDisplayThread(void)
{
    pthread_mutex_lock(&DisplayThreadSuspendMutex);
    if(DisplayThreadSuspendFlag != 0)
        sem_post(&DisplayThreadSuspendedSem);
    while(DisplayThreadSuspendFlag != 0){
        pthread_cond_wait(&DisplayThreadResumeCond, &DisplayThreadSuspendMutex);
    }
    pthread_mutex_unlock(&DisplayThreadSuspendMutex);
}

gui_callbacks_t callbacks;
char gt_version[12];

cyg_io_handle_t uart_handle;

int main(void)
{
    BSP_SDRAM_Init();

    // initialize varpool
    custom_mempool_init();

#ifdef CYGPKG_PROFILE_GPROF
    {
        extern char _stext[], _etext[];
        profile_on(_stext, _etext, 8, 3500); 
    }    
#endif

    callbacks.dma_operation = dma_operation;
    callbacks.dma_fill = dma_fill;
    callbacks.malloc = custom_malloc;
    callbacks.free = custom_free;
//    callbacks.wait_for_reload = vsync;
//    callbacks.wait_for_reload = WaitForReload;
//    callbacks.wait_for_first_line = WaitForFirstLine;
    callbacks.set_layer_pointer = SetLayerPointer;
    callbacks.set_background_colour = SetBackgroundColour;
    callbacks.set_layer_transparency = SetLayerTransparency;
    callbacks.gui_printf = gui_printf;

    GUILib::Init(&callbacks);
    
    if (romfs_mount())
    {
        log_msg(LOG_ERROR, __cfunc__, "Error while mounting romfs. Closing app");
        return -1;
    }
    
    long length;
    uint8_t* motd_buffer = romfs_allocate_and_get_file_content("motd", true, &length);
    if (motd_buffer != NULL)
    {
        uint8_t* tmp = motd_buffer;
        while (*tmp)
        {
            printf("%c", *tmp);
            if (*tmp == '\n')
            {
                printf("\r");
            }
            tmp++;
        }

        custom_free(motd_buffer);
    }

    log_msg(LOG_INFO, __cfunc__, "Starting NEEO firmware (commit %s compiled @ %s %s)", GIT_COMMIT, __DATE__, __TIME__);
    log_msg(LOG_INFO, __cfunc__, "Board: %s", BOARD_NAME);

    volatile uint32_t *unique_id = (uint32_t *)(CYGHWR_HAL_STM32_DEV_U_ID);
    // unique_id[0] = 0; // should not be possible on real cpu
    if (unique_id[0] == 0) {
        log_msg(LOG_TRACE, __cfunc__, "Running on an emulator");
	emulator = 1;
    } else {
        log_msg(LOG_TRACE, __cfunc__, "CPU unique-id: 0x%08X 0x%08X 0x%08X", unique_id[0], unique_id[1], unique_id[2]);
        volatile uint16_t *flash_size = (uint16_t *)(CYGHWR_HAL_STM32_DEV_F_ID);
        log_msg(LOG_TRACE, __cfunc__, "Flash size: %d kB", flash_size[0]);
    }
    log_msg(LOG_TRACE, __cfunc__, "CPU is configured to run @ %d MHz", CYGHWR_HAL_CORTEXM_STM32_CLOCK_PLL_MUL / 2);

if (!emulator) {
    log_msg(LOG_INFO, __cfunc__, "Mounting flash");

#if defined(BOARD_TR2) || defined(BOARD_TR2_V2)
    if (flashfs_mount())
    {
        log_msg(LOG_ERROR, __cfunc__, "Error while mounting flashfs.");
    }
#endif

	log_msg(LOG_INFO, __cfunc__, "Will check for CC3100 version...");
    int ret_cc3100 = cc3100_check_version();
    if(ret_cc3100 < 0) {
        log_msg(LOG_ERROR, __cfunc__, "Failed to get the CC3100 info. Chip not found?");
    }
    else {
        CC3100_Version cc3100_version;
        if(ret_cc3100 == 0) {
            log_msg(LOG_INFO, __cfunc__, "CC3100 firmware is up-to-date. Nothing to do.");
        }
        else {
            log_msg(LOG_INFO, __cfunc__, "CC3100's firmware is outdated...");
            cc3100_get_version(&cc3100_version);
            log_msg(LOG_INFO, __cfunc__, "Current CC3100 version: CHIP 0x%x, MAC: %d.%d.%d.%d, PHY: %d.%d.%d.%d, NWP: %d.%d.%d.%d, ROM: %d, HOST: %d.%d.%d.%d",
                    cc3100_version.chip_id,
                    cc3100_version.fw_version[0], cc3100_version.fw_version[1], cc3100_version.fw_version[2], cc3100_version.fw_version[3],
                    cc3100_version.phy_version[0], cc3100_version.phy_version[1], cc3100_version.phy_version[2], cc3100_version.phy_version[3],
                    cc3100_version.nwp_version[0], cc3100_version.nwp_version[1], cc3100_version.nwp_version[2], cc3100_version.nwp_version[3],
                    cc3100_version.rom_version,
                    cc3100_version.host_version[0], cc3100_version.host_version[1], cc3100_version.host_version[2], cc3100_version.host_version[3]
                    );
            log_msg(LOG_INFO, __cfunc__, "Updating CC3100 firmware...");
            ret_cc3100 = cc3100_update();
            if(ret_cc3100 < 0)
                log_msg(LOG_ERROR, __cfunc__, "Failed to update CC3100 firmware. Error code: %d", ret_cc3100);
            else
                log_msg(LOG_INFO, __cfunc__, "CC3100 firmware update was successful.");
        }
        cc3100_get_version(&cc3100_version);
        log_msg(LOG_INFO, __cfunc__, "Detected CC3100 version: CHIP 0x%x, MAC: %d.%d.%d.%d, PHY: %d.%d.%d.%d, NWP: %d.%d.%d.%d, ROM: %d, HOST: %d.%d.%d.%d",
                cc3100_version.chip_id,
                cc3100_version.fw_version[0], cc3100_version.fw_version[1], cc3100_version.fw_version[2], cc3100_version.fw_version[3],
                cc3100_version.phy_version[0], cc3100_version.phy_version[1], cc3100_version.phy_version[2], cc3100_version.phy_version[3],
                cc3100_version.nwp_version[0], cc3100_version.nwp_version[1], cc3100_version.nwp_version[2], cc3100_version.nwp_version[3],
                cc3100_version.rom_version,
                cc3100_version.host_version[0], cc3100_version.host_version[1], cc3100_version.host_version[2], cc3100_version.host_version[3]
                );
    }
    }
    sem_init(&LoadingSem, 0, 0);
    sem_init(&DMA2D_Sem, 0, 0);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&JNSerialMutex, &attr);

    pthread_mutexattr_t attr2;
    pthread_mutexattr_init(&attr2);
    pthread_mutex_init(&RequestCollectionMutex, &attr2);

#ifdef BOARD_TR2
    hal_stm32_gpio_set(WIFI_SUP_EN);
    hal_stm32_gpio_out(WIFI_SUP_EN, 1);
#ifdef BOARD_TR2_V2
    hal_stm32_gpio_set(FPGA_EN);
    hal_stm32_gpio_out(FPGA_EN, 0);
#endif
    PWM_BUZZER_Init();
#endif

    SynchroInit();

#ifdef BOARD_EVAL
    hal_stm32_gpio_set(LED1);
    hal_stm32_gpio_set(LED2);
    hal_stm32_gpio_set(LED3);
    hal_stm32_gpio_set(LED4);
    //hal_stm32_gpio_set(TEST_PIN);
#endif

    hal_stm32_gpio_set(LCDBL_PWM);

    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_LTDC, 0, (cyg_addrword_t)&data, ReloadIsr, ReloadDsr, &ReloadIntHandle, &ReloadInt);
    cyg_interrupt_configure(CYGNUM_HAL_INTERRUPT_LTDC, 1, true);
    cyg_interrupt_attach(ReloadIntHandle);

    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_DMA2D, 0, 0, DMA2D_ISR, DMA2D_DSR, &DMA2DIntHandle, &DMA2DInt);
    cyg_interrupt_configure(CYGNUM_HAL_INTERRUPT_DMA2D, 1, true);
    cyg_interrupt_attach(DMA2DIntHandle);

    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_DMA2D);

#ifdef LOW_POWER
    pthread_mutex_init(&DisplayThreadSuspendMutex, NULL);
    DisplayThreadSuspendFlag = 1;
    pthread_cond_init(&DisplayThreadResumeCond, NULL);
    sem_init(&DisplayThreadSuspendedSem, 0, 0);
    resumeDisplayThread();
#endif
    // PTHREAD_STACK_MIN = ~3kB
    pthread_attr_init(&DisplayThreadAttr);
    pthread_attr_setstacksize(&DisplayThreadAttr, 12*1024);

    pthread_attr_init(&FpsThreadAttr);
    pthread_attr_setstacksize(&FpsThreadAttr, PTHREAD_STACK_MIN);

    pthread_attr_init(&ContentCollectorAttr);
    pthread_attr_setstacksize(&ContentCollectorAttr, 8*1024);

    pthread_attr_init(&JNThreadAttr);
    pthread_attr_setstacksize(&JNThreadAttr, 8*1024);

    pthread_attr_init(&StateDetectionThreadAttr);
    pthread_attr_setstacksize(&StateDetectionThreadAttr, PTHREAD_STACK_MIN);

    pthread_attr_init(&InputThreadAttr);
    pthread_attr_setstacksize(&InputThreadAttr, PTHREAD_STACK_MIN);

#ifdef BOARD_TR2
    pthread_attr_init(&BatteryThreadAttr);
    pthread_attr_setstacksize(&BatteryThreadAttr, PTHREAD_STACK_MIN);
#endif

    pthread_attr_init(&TouchThreadAttr);
    pthread_attr_setstacksize(&TouchThreadAttr, 8*1024);

    pthread_attr_init(&GUIInitializationThreadAttr);
    pthread_attr_setstacksize(&GUIInitializationThreadAttr, 8*1024);

    pthread_attr_init(&FirmwareUpdatesThreadAttr);
    pthread_attr_setstacksize(&FirmwareUpdatesThreadAttr, 8*1024);

    // set main thread priority
    struct sched_param schedparam;
    schedparam.sched_priority = MAIN_THREAD_PRIO;
    pthread_setschedparam(pthread_self(), SCHED_RR, &schedparam);

    log_msg(LOG_INFO, __cfunc__, "Spawning FpsThread");
    pthread_create(&FpsThreadHandle, &FpsThreadAttr, &FpsThread, NULL);
    set_thread_prio(FpsThreadAttr, FpsThreadHandle, FPS_THREAD_PRIO);

    log_msg(LOG_INFO, __cfunc__, "Spawning StateDetectionThread");
    pthread_create(&StateDetectionThreadHandle, &StateDetectionThreadAttr, &StateDetectionThread, NULL);
    set_thread_prio(StateDetectionThreadAttr, StateDetectionThreadHandle, STATEDETECTION_THREAD_PRIO);

    init_uart(&JNSerialMutex, update_callback);
    if (!emulator) {
	    log_msg(LOG_INFO, __cfunc__, "Will check for JN firmware version...");
	    if(!JN_is_firmware_up_to_date("/rom/jn.bin"))
	    {
            long jn_firmware_size;
            uint8_t * jn_firmware = romfs_allocate_and_get_file_content("jn.bin", false, &jn_firmware_size);
            UploadFirmwareToJn(jn_firmware, jn_firmware_size);
            custom_free(jn_firmware);
	    }
	    log_msg(LOG_INFO, __cfunc__, "JN ready");
    }

    PreguiXml.data = (unsigned char*) romfs_allocate_and_get_file_content("pregui.xml", true, (long int*)&PreguiXml.size);
    if (PreguiXml.data == NULL)
    {
       log_msg(LOG_ERROR, __cfunc__, "error loading gui.xml, ignoring it");
    }
    log_msg(LOG_TRACE, __cfunc__, "Finished loading XML, size %u", PreguiXml.size);

    GlobalXml.data = (unsigned char*) romfs_allocate_and_get_file_content("gui.xml", true, (long int*)&GlobalXml.size);
    if (GlobalXml.data == NULL)
    {
            log_msg(LOG_ERROR, __cfunc__, "error loading gui.xml, ignoring it");
    }
    log_msg(LOG_TRACE, __cfunc__, "Finished loading XML, size %u", GlobalXml.size);

    list1.data = (unsigned char*) romfs_allocate_and_get_file_content("listContent.xml", true, (long int*)&list1.size);
    if (list1.data == NULL)
    {
            log_msg(LOG_ERROR, __cfunc__, "error loading listContent.xml, ignoring it");
    }
    log_msg(LOG_TRACE, __cfunc__, "Finished loading XML, size %u", list1.size);

    list2.data = (unsigned char*) romfs_allocate_and_get_file_content("listContent2.xml", true, (long int*)&list2.size);
    if (list2.data == NULL)
    {
            log_msg(LOG_ERROR, __cfunc__, "error loading listContent2.xml, ignoring it");
    }
    log_msg(LOG_TRACE, __cfunc__, "Finished loading XML, size %u", list2.size);

    log_msg(LOG_INFO, __cfunc__, "Spawning DisplayThread");
    pthread_create(&DisplayThreadHandle, &DisplayThreadAttr, &DisplayThread, NULL);
    set_thread_prio(DisplayThreadAttr, DisplayThreadHandle, DISPLAY_THREAD_PRIO);

#ifdef BOARD_EVAL
    hal_stm32_gpio_out(LED4, 1);
#endif

#ifdef BOARD_TR2
    MarkActivity();
    log_msg(LOG_INFO, __cfunc__, "Spawning BatteryThread");
    pthread_create(&BatteryThreadHandle, &BatteryThreadAttr, &BatteryThread, NULL);
    set_thread_prio(BatteryThreadAttr, BatteryThreadHandle, BATTERY_THREAD_PRIO);
#endif

    Cyg_Watchdog::watchdog.start();
    log_msg(LOG_INFO, __cfunc__, "Joining threads");
    pthread_join(DisplayThreadHandle, NULL);
    pthread_join(InputThreadHandle, NULL);
    pthread_join(TouchThreadHandle, NULL);
    pthread_join(FpsThreadHandle, NULL);
    pthread_join(StateDetectionThreadHandle, NULL);
    
    log_msg(LOG_INFO, __cfunc__, "All is finished, system halted...");
    while(1)
    {
    	msleep(1000);
        log_msg(LOG_INFO, __cfunc__, "System halted, spinning...");
    }
}

void test_on_chip_flash(void)
{
    int ret;
    cyg_flashaddr_t block_start=0; //, flash_end=0;
    cyg_flash_info_t info;
    cyg_uint32 i=0;
    cyg_uint32 j;
    int block_size=0; //, blocks=0;
    unsigned char * ptr;

    cyg_flash_init(NULL);

    do {
      ret = cyg_flash_get_info(i, &info);
      if (ret == CYG_FLASH_ERR_OK) {
        log_msg(LOG_WARNING, "FLASH", "Nth=%d, start=%p, end=%p", i, (void *) info.start, (void *) info.end);
        if (i == 0) {
          block_start = info.start;
          //flash_end = info.end;
          block_size = info.block_info[0].block_size;
          //blocks = info.block_info[0].blocks;
        }
        
        for (j=0;j < info.num_block_infos; j++) {
            log_msg(LOG_WARNING, "FLASH", "\t block_size %zd, blocks %u", info.block_info[j].block_size, info.block_info[j].blocks);
        }
      }
      i++;
    } while (ret != CYG_FLASH_ERR_INVALID);

    block_start = 0x08100000;
    block_size = 0x00004000;
    //flash_end = 0x081fffff;

    /* .. and check nothing else changed */
    for (ptr=(unsigned char *)block_start,ret=0, i=0; ptr < (unsigned char *)block_start+256; ptr++, i++) {
        printf("%02x ", *ptr);
        if (*ptr != i) {
            ret++;
        }
    }
    printf("\n");
    log_msg(LOG_INFO, "FLASH", "Verify. Bad bytes: %u", ret);

    ret=cyg_flash_erase(block_start, block_size, NULL);
    if(ret != CYG_FLASH_ERR_OK)
        log_msg(LOG_ERROR, "FLASH", "Erase failed! (%d) %s", ret, cyg_flash_errmsg(ret));

    /* check that its actually been erased, and test the mmap area */
    for (ptr=(unsigned char *)block_start,ret=0, i=0; ptr < (unsigned char *)block_start+256; ptr++, i++) {
        printf("%02x ", *ptr);
        if (*ptr != 0xff) {
            ret++;
        }
    }
    printf("\n");
    log_msg(LOG_INFO, "FLASH", "Erase check. Unerased bytes: %u", ret);

    uint8_t data[256];

    for(i=0; i<256; i++)
        data[i] = i;

    ret = cyg_flash_program(block_start, &data, 256, NULL);

    /* .. and check nothing else changed */
    for (ptr=(unsigned char *)block_start,ret=0, i=0; ptr < (unsigned char *)block_start+256; ptr++, i++) {
        printf("%02x ", *ptr);
        if (*ptr != i) {
            ret++;
        }
    }
    printf("\n");
    log_msg(LOG_INFO, "FLASH", "Verify. Bad bytes: %u", ret);
}

void UploadFirmwareToJn(uint8_t * jn_firmware, uint32_t jn_firmware_size)
{
    uint8_t mac[8];
    if(pthread_mutex_lock(&JNSerialMutex) == 0)
    {
        if(jn_init_bootloader() == 0)
        {
            log_msg(LOG_INFO, __cfunc__, "Uploading firmware");
            jn_read_mac(mac);
            jn_upload(jn_firmware, jn_firmware_size);
            log_msg(LOG_INFO, __cfunc__, "JN5168 firmware uploaded!");
            log_msg(LOG_INFO, __cfunc__, "JN5168 MAC: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n\r", 
                                          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]);
        }
        else
        {
            log_msg(LOG_ERROR, __cfunc__, "JN5168 init failed!");
        }
        pthread_mutex_unlock(&JNSerialMutex);
    }
}

void update_callback(const char* xml, uint16_t xml_length)
{
    XMLDocument doc(true, COLLAPSE_WHITESPACE);

    log_msg(LOG_TRACE, __cfunc__, "Received push update!");

    if(doc.Parse(xml, xml_length) != XML_SUCCESS) {
        doc.Clear();
        return;
    }

    XMLElement * root = doc.FirstChildElement("update");
    if(root != 0) {
        log_msg(LOG_TRACE, __cfunc__, "Root found...");
        string screen_id;
        string element_id;

        for (XMLElement* screen = root->FirstChildElement("screen");
             screen != NULL;
             screen = screen->NextSiblingElement()) {
            screen_id = XMLSupport::GetAttributeOrDefault(screen, "id", "");
            log_msg(LOG_TRACE, __cfunc__, "Screen %s found", screen_id.c_str());
            if(screen_id == "")
                break;
            for (XMLElement* element = screen->FirstChildElement("element");
                 element != NULL;
                 element = element->NextSiblingElement()) {
                element_id = XMLSupport::GetAttributeOrDefault(element, "id", "");
                log_msg(LOG_TRACE, __cfunc__, "Element %s found", element_id.c_str());
                if(element_id == "")
                    break;
                Component* c = DispMan->GetScreen(screen_id.c_str())->GetElement(element_id.c_str());
                log_msg(LOG_TRACE, __cfunc__, "Setting %s to %s", element_id.c_str(), element->GetText());
                if((element_id == "sw1") || (element_id == "sw2")) {
                    if(strcmp(element->GetText(), "true") == 0) {
                        ((SwitchButton*)c)->SetSwitchState(true);
                    }
                    else
                        ((SwitchButton*)c)->SetSwitchState(false);
                }
                else if(element_id == "sb1") {
                    ((ScrollBar*)c)->SetValue(strtol(element->GetText(), NULL, 10));
                }
                else if(element_id == "pb1") {
                    ((ProgressBar*)c)->SetProgressValue(strtol(element->GetText(), NULL, 10));
                }
                else if(element_id == "TestImg") {
                    file_t pngImage;

                    if(wifi_download(element->GetText(), (const char*)conf.host_ip,  &pngImage.size,  &pngImage.data) == 0){
                        log_msg(LOG_TRACE, __cfunc__, "Replacing...");
                        ((Image*)c)->UpdateImageContent(new ImageContent(ImageContent::FromPNG(pngImage.data,pngImage.size, 1, false)));
                    } else {
                        log_msg(LOG_TRACE, __cfunc__, "Image not available...");
                        ((Image*)c)->DeleteImageContent();
                    }
                }
            }
        }    
        doc.Clear();
    }
    else {
        log_msg(LOG_ERROR, __cfunc__, "Error in update document.");
    }
}

void *StateDetectionThread(void*)
{
    while(1)
    {
        int state;
        if(JN_check_status(&state) != 0)
        {
            log_msg(LOG_ERROR, __cfunc__, "Error while checking state. I will try again later");
        }
        else
        {
            if(DispMan && lcd_state == LCD_ON && DispMan->GetTopPanel()) {
                Image* state_image = (Image*) DispMan->GetTopPanel()->GetElement("iSignal");
                if(state_image != NULL) {
                    state_image->SetActiveFrame(state);
                    // log_msg(LOG_TRACE, __cfunc__, "State image active frame set to: %d", state);
                }
            }
        }

        msleep(500);
    }

    pthread_exit(NULL);
}

void * FirmwareUpdatesThread(void *)
{
    firmware_info_t fw_info = {0};

    log_msg(LOG_INFO, __cfunc__, "Checking for updates!");

    if(offline_mode)
    {
        log_msg(LOG_WARNING, __cfunc__, "Offline mode active! Can not check for updates!");
        pthread_exit(NULL);
    }

    if(JN_get_firmware_info("neeo", &fw_info) == 0)
    {
        log_msg(LOG_INFO, __cfunc__, "Firmware name: %s", fw_info.name);
        log_msg(LOG_INFO, __cfunc__, "Firmware file: %s", fw_info.file_name);
        log_msg(LOG_INFO, __cfunc__, "Firmware version: %u", fw_info.version);
        log_msg(LOG_INFO, __cfunc__, "Firmware CRC32: 0x%x", fw_info.crc32);
    }
    if(fw_info.version > (uint32_t)atoi(FW_MAJOR_VERSION))
    {
        log_msg(LOG_WARNING, __cfunc__, "Main firmware is too old, trying to update.");

        DispMan->ShowPopup("YesNo", "New version available, upgrade?");
        while(firmwareCheckVal == -1)
            msleep(100);
        DispMan->ClosePopup();

        if(firmwareCheckVal == 2)
        {
            log_msg(LOG_INFO, __cfunc__, "Will not upgrade");
            firmwareCheckVal = -1;
            pthread_exit(NULL);
        }
        firmwareCheckVal = -1;
        log_msg(LOG_INFO, __cfunc__, "Will upgrade");

        file_t fw_binary;
        uint32_t crc;
        if(wifi_download(fw_info.file_name, (const char*)conf.host_ip, &fw_binary.size, &fw_binary.data))
        {
            log_msg(LOG_ERROR, __cfunc__, "Can not download firmware file at: %s", fw_info.file_name);
        }
        else
        {
            log_msg(LOG_INFO, __cfunc__, "Firmware file opened correctly, len = %u", fw_binary.size);
            // TODO: choose proper crc32 algorithm
            // crc = cyg_crc32(fw_binary.data, fw_binary.size);
            // log_msg(LOG_INFO, __cfunc__, "cyg_crc32 = 0x%x", crc);
            // crc = cyg_posix_crc32(fw_binary.data, fw_binary.size);
            // log_msg(LOG_INFO, __cfunc__, "cyg_posix_crc32 = 0x%x", crc);

            // cyg_ether_crc32 generates same crc as Python zlib.crc32
            crc = cyg_ether_crc32(fw_binary.data, fw_binary.size);
            log_msg(LOG_INFO, __cfunc__, "cyg_ether_crc32 = 0x%x", crc);
            if(crc != fw_info.crc32)
                log_msg(LOG_ERROR, __cfunc__, "CRC32 mismatch! expected is 0x%x, calculated is 0x%x.", fw_info.crc32, crc);
            else
                log_msg(LOG_INFO, __cfunc__, "CRC is ok, but I don't know how to upgrade firmware", fw_binary.size);
        }
    }
    pthread_exit(NULL);
}

void * GUIInitializationThread(void *)
{
    //Background
    DispMan->SetBackgroundImage(Image(new ImageContent(ImageContent::FromPNG("/rom/images/background.png", 1, false, DITHER_INDEXED))));

    //Fake downloading images, that in fact are in rom memory.
    //TODO: as this is going to be dynamic, we have to somehow store this (5, true), (1, false), (number of frames, alpha) etc. info inside the png file!
    #define IMAGE(nm, a, b) nm , new ImageContent(ImageContent::FromPNG("/rom/images/" nm, a, b))
    DispMan->AddImageContentToContainer(IMAGE("signal.png", 5, true));
    DispMan->AddImageContentToContainer(IMAGE("battery_anim.png", 6, true));
    DispMan->AddImageContentToContainer(IMAGE("art1.png", 1, false));
    DispMan->AddImageContentToContainer(IMAGE("art2.png", 1, false));
    DispMan->AddImageContentToContainer(IMAGE("slider.png", 1, true));
    DispMan->AddImageContentToContainer(IMAGE("switch.png", 2, true));
    
    //Default font
    DispMan->AddFontToFontContainer("", new GUIFont("/rom/fonts/normal.font.gz"));

    int i = 0;
    while (callback_funcs[i] != NULL) {
        DispMan->AddCallbackToContainer(callback_func_names[i], (guilib::Event::CallbackPtr) callback_funcs[i]);
        i++;
    }

    DispMan->AddCallbackToContainer("UpdateTest", UpdateTest);
    DispMan->AddCallbackToContainer("ResetDevice", ResetDevice);

    DispMan->SetExternalContentRequestCallback(RequestCallback);
    DispMan->SetCancelExternalContentRequestCallback(CancelCallback);

    int ParsingErrorCode = -1;
    GetFontsFromXML((char*)PreguiXml.data, PreguiXml.size, false);
    ParsingErrorCode = DispMan->BuildFromXML((char*)PreguiXml.data, PreguiXml.size);
    if(ParsingErrorCode == -1){
        //TODO Handle this error
    } else {
        appMainConfig.load(&PreguiXml);
        DispMan->SetActiveScreen("start", 0);
    }

#ifdef BOARD_TR2
    PWM_BUZZER_Beep(500,100);
    PWM_BUZZER_Beep(800,100);
    PWM_BUZZER_Beep(1000,100);
#endif
    sem_post(&LoadingSem);
    pthread_exit(NULL);
}

vector<string> RequestVector;

void RequestCallback(string content) {
    if(!content.empty()){
        pthread_mutex_lock(&RequestCollectionMutex);
        RequestVector.push_back(content);
        pthread_mutex_unlock(&RequestCollectionMutex);
    }
}

void CancelCallback(string content) {
    if(!content.empty()){
        pthread_mutex_lock(&RequestCollectionMutex);
        for(vector<string>::iterator it = RequestVector.begin(); it < RequestVector.end(); ++it) {
            if (it->length() == content.length() && strcmp(it->c_str(), content.c_str()) == 0) {
                RequestVector.erase(it);
                break;
            }
        }
        pthread_mutex_unlock(&RequestCollectionMutex);
    }
}

void *ContentCollectorThread(void*) {
    string currentContent = "";
    file_t imageContent;

    while (1) {
        if(wifiInit) {
            pthread_mutex_lock(&RequestCollectionMutex);
            if(!RequestVector.empty()) {
                currentContent = RequestVector.begin()->data();
                RequestVector.erase(RequestVector.begin());
            }
            pthread_mutex_unlock(&RequestCollectionMutex);

            if(!currentContent.empty()) {
                if(wifi_download((char *)string("images/" + currentContent), (const char*)conf.host_ip,
                &imageContent.size, &imageContent.data)) {
                    log_msg(LOG_ERROR, __cfunc__, "Can not download imageContent: %s", currentContent.c_str());
                    currentContent.clear();
                } else {
                    //Unpack and put to the Display Manager
                    DispMan->AddImageContentToContainer(currentContent,
                    new ImageContent(ImageContent::FromPNG(imageContent.data, imageContent.size, 1, false)));
                    currentContent.clear();
                }
            }
            if(RequestVector.empty()) msleep(200); //Check if download request available every 200ms
        } else {
            msleep(1000); //Wifi not even initialized. Check if ready every 1s
        }

    }
    pthread_exit(NULL);
}

void *FpsThread(void*)
{
    struct timeval tv, lasttv;
    lasttv.tv_sec = 0;
    lasttv.tv_usec = 0;
    uint32_t last_flips = 0;
    int last_frames = 0;
    total_frames = 0;
    while (1) {
        cyg_thread_delay(2000);
        gettimeofday(&tv, NULL);
        if (last_flips != 0) {
                uint32_t delta_msec = (tv.tv_sec - lasttv.tv_sec) * 1000;
                delta_msec += (tv.tv_usec - lasttv.tv_usec) / 1000;
                double delta_sec = (double)(delta_msec) / 1000.0;
                double fps = (double)(DispMan->flips-last_flips) / delta_sec;
                log_msg(LOG_TRACE, __cfunc__, "avarage fps: %.2f, total frames: %d, delta: %d, refresh rate: %.0f Hz", fps, total_frames, (total_frames-last_frames), ((total_frames-last_frames)/2.0));
        }
        if (emulator) {
                static int minfo_delay = 5;
                minfo_delay--;
                if (minfo_delay <= 0) {
                        uint32_t fr = custom_freemem();
                        uint32_t tm = custom_totalmem();
                        struct mallinfo minfo = mallinfo();
                        log_msg(LOG_INFO, __cfunc__, "malloc stats | free internal: %.2f / %.2f kB (%d%%) | free external: %.2f / %.2f kB (%d%%)", minfo.maxfree/1024.0, minfo.arena/1024.0, (100 * minfo.maxfree) / minfo.arena,fr/1024.0, tm/1024.0, (100*fr)/tm);
                        minfo_delay = 5;
                }
        }
        last_frames = total_frames;
        last_flips = DispMan->flips;
        lasttv = tv;
#ifdef LOW_POWER
        /* required to keep system running during DeisplayThread suspend */
        Cyg_Watchdog::watchdog.reset();
#endif
    }
    pthread_exit(NULL);
}

void *JNThread(void* arg)
{
    string *cp6address = (string*)arg;
    log_msg(LOG_INFO, __cfunc__, "Pairing with: %s", cp6address->c_str());
    DispMan->ShowPopup("start", "Pairing...");
    log_msg(LOG_TRACE, __cfunc__, "Init pairing");
    while(JN_pair_with_cp6(*cp6address))
    {
        if(pairingCancel) {
            pairingCancel = false;
            DispMan->ClosePopup();
            log_msg(LOG_ERROR, __cfunc__, "Pairing aborted!");
            pthread_exit(NULL);
        }
        log_msg(LOG_TRACE, __cfunc__, "Init pairing will try again.");
        msleep(50);
    }

    char hostname[128];
    uint16_t hostname_length;
    log_msg(LOG_TRACE, __cfunc__, "Getting brain info.");
    while(JN_get_brain_info(hostname, 128, &hostname_length))
    {
        if(pairingCancel == true) {
            pairingCancel = false;
            DispMan->ClosePopup();
            log_msg(LOG_ERROR, __cfunc__, "Pairing aborted!");
            pthread_exit(NULL);
        }
        log_msg(LOG_TRACE, __cfunc__, "Getting brain info will try again.");
        msleep(50);
    }

    DispMan->ShowPopup("start", "Getting zero configuration...");
    log_msg(LOG_TRACE, __cfunc__, "Receiving zero configuration.");
    while(JN_get_zero_config(hostname, &conf))
    {
        if(pairingCancel) {
            pairingCancel = false;
            DispMan->ClosePopup();
            log_msg(LOG_ERROR, __cfunc__, "Pairing aborted!");
            pthread_exit(NULL);
        }
        log_msg(LOG_TRACE, __cfunc__, "Receiving zero configuration will try again.");
        msleep(50);
    }

    DispMan->ShowPopup("start", "Connecting WiFi...");
    log_msg(LOG_TRACE, __cfunc__, "Received WiFi credentials: %s/%s. GUI file at %s", conf.ssid, conf.pass, conf.gui_loc);

    if(pairingCancel) {
        pairingCancel = false;
        DispMan->ClosePopup();
        log_msg(LOG_ERROR, __cfunc__, "Pairing aborted!");
        pthread_exit(NULL);
    }

    if(wifi_init(conf.ssid, conf.ssid_length, conf.pass, conf.pass_length))
    {
        log_msg(LOG_ERROR, __cfunc__, "Failed to init WIFI.");
        DispMan->ClosePopup();
        pthread_exit(NULL);
        return NULL;
    }
    else
    {
        wifiInit = 1;
    }

    if(pairingCancel) {
        pairingCancel = false;
        DispMan->ClosePopup();
        log_msg(LOG_ERROR, __cfunc__, "Pairing aborted!");
        pthread_exit(NULL);
    }

    file_t xml;
    log_msg(LOG_TRACE, __cfunc__, "Fetching gui.xml");
    if(wifi_download((char *)conf.gui_loc, (const char*)conf.host_ip, &xml.size, &xml.data))
    {
        log_msg(LOG_ERROR, __cfunc__, "Can not download gui.xml, falling back to defaults.");
    } else {
        log_msg(LOG_INFO, __cfunc__, "File opened correctly, len = %u", xml.size);
       
        int ret = BuildGUI(&xml);
        if(ret == -1){
            log_msg(LOG_INFO, __cfunc__, "BuildGUI failed!");
            DispMan->ClosePopup();
        } else {
            log_msg(LOG_INFO, __cfunc__, "BuildGUI ok ret = %d", ret);
            DispMan->ClosePopup();
            DispMan->SetActiveScreen("0", 0);
            offline_mode = false;
        }
    }
    DispMan->ClosePopup();

    pthread_exit(NULL);
}

void *DisplayThread(void* arg)
{

#ifdef LCD_F_WVGA_3262DG
    hal_stm32_gpio_set(LCD_RESET);
    hal_stm32_gpio_out(LCD_RESET, RESET_LEVEL);
    msleep(100);
    hal_stm32_gpio_out(LCD_RESET, !RESET_LEVEL);
#endif

    LTDC_MUX_Init();
    LTDC_Init();

#ifdef LCD_F_WVGA_3262DG
    LG4573A_Init();
#endif

    bool rotate90 = false;
#ifdef ROTATE90
    rotate90 = true;
#endif

    log_msg(LOG_INFO, __cfunc__, "Front layer: %d bits, back layer: %d bits", FRONT_DEPTH*8, BACK_DEPTH*8);
    
    LTDC_SetConfig(LCD_FOREGROUND_LAYER, 480, 800, FRONT_DEPTH == 4 ? PIXEL_FORMAT_ARGB8888 : PIXEL_FORMAT_ARGB4444, 0);
    LTDC_SetConfig(LCD_BACKGROUND_LAYER, 480, 800, FRONT_DEPTH == 4 ? PIXEL_FORMAT_ARGB8888 : PIXEL_FORMAT_RGB565, 0);

    SetLayerEnable(LCD_BACKGROUND_LAYER, false);

    Manager::Initialize(480, 800, FRONT_DEPTH, BACK_DEPTH, rotate90);
    DispMan = &Manager::GetInstance();
    DispMan->SetBackgroundColour(0xFF000030);

    //Loading image
    //TODO: Also, I would like this loading screen to also be defined in romfs, as init.xml. I would put a couple of "default"
    //      screens there - "loading" screen and error handling types of screens. Of course we can start with just a "loading" screen.
    ImageContent* imageContent = new ImageContent(ImageContent::FromPNG("/rom/images/spinner-white-plain.png", 19, true));
    DispMan->SetLoadingImage(Image(imageContent, (DispMan->GetWidth() - imageContent->GetWidth())/2, (DispMan->GetHeight() - imageContent->GetHeight())/2+100, 0));

    uint32_t val = R32(_LTDC_OFFSET + _LTDC_IER);
    W32(_LTDC_OFFSET + _LTDC_IER, val | _LTDC_IT_LI);
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_LTDC);

    //Logo image
    imageContent = new ImageContent(ImageContent::FromPNG("/rom/images/bootlogo.png", 1, true));
    DispMan->SetLogoImage(Image(imageContent, (DispMan->GetWidth() - imageContent->GetWidth())/2, 200, 0));

    DispMan->SetScrollingDuration(300);

    hal_stm32_gpio_out(LCDBL_PWM, 1);

    pthread_t guiInitializationThread;
    pthread_create(&guiInitializationThread, &GUIInitializationThreadAttr, &GUIInitializationThread, NULL);
    set_thread_prio(GUIInitializationThreadAttr, guiInitializationThread, GUIINIT_THREAD_PRIO);

    // wait for things to load
    int SemValue=0;
    while (SemValue == 0)
    {
        DispMan->Draw();
        msleep(20);
        sem_getvalue(&LoadingSem, &SemValue);
    }

    pthread_join(guiInitializationThread, NULL);

    log_msg(LOG_INFO, __cfunc__, "Spawning InputThread");
    pthread_create(&InputThreadHandle, &InputThreadAttr, &InputThread, NULL);
    set_thread_prio(InputThreadAttr, InputThreadHandle, KEYBOARD_THREAD_PRIO);

    log_msg(LOG_INFO, __cfunc__, "Spawning TouchThread");
    pthread_create(&TouchThreadHandle, &TouchThreadAttr, &TouchThread, NULL);
    set_thread_prio(TouchThreadAttr, TouchThreadHandle, TOUCH_THREAD_PRIO);

    // TODO:
    //  we should have:
    //  DispMan->SetFadeInDuration(500);
    //  DispMan->SetFadeOutDuration(500);

    // fade out load screen.
    // TODO: this takes ~20*20ms = 400ms (+ drawing time).
    //       I've tested this to be 735ms including drawing time.
    
    int k;
    for(k = 20; k>=0; --k) //Fading out
    {
        DispMan->Draw();
        msleep(20);
        DispMan->SetLayerTransparency(LCD_FOREGROUND_LAYER, (uint32_t)(12.75*k));
    }

    DispMan->InitializationFinished();

    SetLayerEnable(LCD_BACKGROUND_LAYER, true);

    log_msg(LOG_INFO, __cfunc__, "Entering GUI redraw loop");

    k = 0;
	while(1)
    {
        // inital fade-in
	// TODO: this fade-in is a hack. We could theoretically move it to gui lib and have it in draw, making "k" dependant on time instead of doing "k++".
	//       but we can fix that later.
        if (k <= 20) {
            DispMan->SetLayerTransparency(LCD_BACKGROUND_LAYER, (uint32_t)(12.75*k));
            DispMan->SetLayerTransparency(LCD_FOREGROUND_LAYER, (uint32_t)(12.75*k));
            k++;
        }

        msleep(1); // TODO: here we could theoretically wait for LTDC irq...

        DispMan->MainLoop();

#ifdef LOW_POWER
        checkSuspendDisplayThread();
#endif
        Cyg_Watchdog::watchdog.reset();

#ifdef BOARD_EVAL
        hal_stm32_gpio_toggle(LED3);
#endif
    }
    pthread_exit(NULL);
}


cyg_handle_t touch_isr_handle;
cyg_interrupt touch_isr_object;

cyg_uint32 touchISR(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);

#ifdef BOARD_EVAL
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_PR, CYGHWR_HAL_STM32_EXTI_BIT(9));
#endif
#ifdef BOARD_TR2
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_PR, CYGHWR_HAL_STM32_EXTI_BIT(1));
#endif

    return CYG_ISR_CALL_DSR | CYG_ISR_HANDLED;
}

void touchDSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    sem_post(&TouchProcesSem);
#ifdef BOARD_EVAL
    hal_stm32_gpio_toggle(LED1);
#endif
    cyg_interrupt_unmask(vector);
}

void *TouchThread(void*){
    hal_stm32_gpio_set(TOUCH_RESET);

    sem_init(&TouchProcesSem, 0, 0);

    int tries = 3;
    while(tries)
    {
        hal_stm32_gpio_out(TOUCH_RESET, 1);
        msleep(150);
        hal_stm32_gpio_out(TOUCH_RESET, 0);
        msleep(150);

        if(gtp_read_id(gt_version))
        {
           log_msg(LOG_INFO, __cfunc__, "Touch Panel version: %s", gt_version);
           break;
        }
        else
        {
            log_msg(LOG_INFO, __cfunc__, "Touch panel detect failed, retrying...");
            tries--;
        }
    }

    if(!tries)
    {
        log_msg(LOG_ERROR, __cfunc__, "Touch Panel detection failed!");
	    pthread_exit(NULL);
    }
    
    //Setting ISR
    hal_stm32_gpio_set(INTERRUPT_TOUCH);
    cyg_interrupt_create(INT_TOUCH_EXTI, 0, 0, touchISR, touchDSR, &touch_isr_handle, &touch_isr_object);
    cyg_interrupt_configure(INT_TOUCH_EXTI, false, true);
    cyg_interrupt_attach(touch_isr_handle);
    cyg_interrupt_unmask(INT_TOUCH_EXTI);
    uint32_t regval;
#ifdef BOARD_EVAL
    HAL_READ_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(9), regval);
    regval |= CYGHWR_HAL_STM32_SYSCFG_EXTICRX_PORTF(9);
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(9), regval);
#endif
#ifdef BOARD_TR2
    HAL_READ_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(1), regval);
    regval |= CYGHWR_HAL_STM32_SYSCFG_EXTICRX_PORTB(1);
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(1), regval);
#endif

    LIS2DH_init();

    while (true)
    {
        sem_wait(&TouchProcesSem);
        gtp_get_points(&TouchData);
        if(TouchData.touch_flag && lcd_state == LCD_ON) {
            MarkActivity();
        }

        //Put X and Y in in GUI coordinate system

#ifdef GT_SCALE_DOWN
	const float touchScale = 0.975;
#else
	const float touchScale = 1.0;
#endif

#ifdef TOUCH_REPORTING
        log_msg(LOG_INFO, __cfunc__, "Processing touch point: X:Y %d:%d", 480-TouchData.x, (int32_t)((800-TouchData.y)*touchScale));
#endif
        DispMan->ProcessTouchPoint(TouchData.touch_flag, 480-TouchData.x, (int32_t)((800-TouchData.y)*touchScale)); //Y axis scaled
#ifdef BOARD_EVAL
        hal_stm32_gpio_toggle(LED2);
#endif
    }

    pthread_exit(NULL);
}

void wakeUpSystem() {
    uint32_t reg;
    /* set SDRAM in normal mode */
    // TODO: add proper bas address (FMC), register offsets and bit definitions
    /*reg = R32(0xa0000000 + 0x150);
    reg &= ~(0x00000007);
    reg |= 0x00;
    reg &= ~(0x00000018);
    reg |= 0x10;
    W32(0xa0000000 + 0x150, reg);
    msleep(5);*/

    /* enable clock for FMC */
//    hal_stm32_clock_enable(CYGHWR_HAL_STM32_CLOCK(AHB3, FSMC));

    /* enable clock for SPI2 (LCD) */
    hal_stm32_clock_enable(CYGHWR_HAL_STM32_CLOCK(APB1, SPI2));

    /* enable clock for SPI1 (WiFi) */
    hal_stm32_clock_enable(CYGHWR_HAL_STM32_CLOCK(APB2, SPI1));

    /* release LCD reset */
    hal_stm32_gpio_out(LCD_RESET, !RESET_LEVEL);

    /* reinitialize LCD */
    LG4573A_Init();

    /* turn on Flash prefetching */
    reg = R32(CYGHWR_HAL_STM32_FLASH + CYGHWR_HAL_STM32_FLASH_ACR);
    reg |= CYGHWR_HAL_STM32_FLASH_ACR_PRFTEN;
    W32(CYGHWR_HAL_STM32_FLASH + CYGHWR_HAL_STM32_FLASH_ACR, reg);

    /* enable clock for DMA2D */
    hal_stm32_clock_enable(CYGHWR_HAL_STM32_CLOCK(AHB1, DMA2D));

    /* enable clock for LTDC */
    hal_stm32_clock_enable(CYGHWR_HAL_STM32_CLOCK(APB2, LTDC));
    SetLTDCEnable(true);

    resumeDisplayThread();

    msleep(60); //Wait for LCD to start

    /* turn on LCD backlight */
    hal_stm32_gpio_out(LCDBL_PWM, 1);
}

void sleepSystem() {
    /* turn off LCD backlight */
    hal_stm32_gpio_out(LCDBL_PWM, 0);

    /* assert LCD reset */
    hal_stm32_gpio_out(LCD_RESET, RESET_LEVEL);

    uint32_t reg;

    /* turn off Flash prefetching */
    reg = R32(CYGHWR_HAL_STM32_FLASH + CYGHWR_HAL_STM32_FLASH_ACR);
    reg &= ~CYGHWR_HAL_STM32_FLASH_ACR_PRFTEN;
    W32(CYGHWR_HAL_STM32_FLASH + CYGHWR_HAL_STM32_FLASH_ACR, reg);

    suspendDisplayThread();

    /* disable clock for LTDC */
    SetLTDCEnable(false);
    msleep(10); //Wait for ltdc to stop
    hal_stm32_clock_disable(CYGHWR_HAL_STM32_CLOCK(APB2, LTDC));

    /* disable clock for SPI2 (LCD) */
    hal_stm32_clock_disable(CYGHWR_HAL_STM32_CLOCK(APB1, SPI2));

    /* disable clock for SPI1 (WiFi) */
    hal_stm32_clock_disable(CYGHWR_HAL_STM32_CLOCK(APB2, SPI1));

    /* disable clock for DMA2D */
    do{
        reg = R32(0x4002b000);
    }while(reg & 0x00000001);
    hal_stm32_clock_disable(CYGHWR_HAL_STM32_CLOCK(AHB1, DMA2D));

    /* set SDRAM in self-refresh mode */
    //TODO: add definitions for FMC base address, registers names and fields names
    /*reg = R32(0xa0000000 + 0x150);
    reg &= ~(0x00000007);
    reg |= 0x05;
    reg &= ~(0x00000018);
    reg |= 0x10;
    W32(0xa0000000 + 0x150, reg);
    msleep(5);*/

    /* disable clock for FMC */
//    hal_stm32_clock_disable(CYGHWR_HAL_STM32_CLOCK(AHB3, FSMC));
}

static cyg_interrupt LIS2DH_int1_object;
static cyg_handle_t LIS2DH_int1_handle;

cyg_uint32 LIS2DH_INT1_intISR(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);

    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_PR, CYGHWR_HAL_STM32_EXTI_BIT(8));

    return CYG_ISR_CALL_DSR | CYG_ISR_HANDLED;
}

void LIS2DH_INT1_intDSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    MarkActivity();
    cyg_interrupt_unmask(vector);
}

void *InputThread(void*)
{
#if defined(BOARD_TR2) || defined(BOARD_TR2_V2)
    //Initializing accelerometer
    hal_stm32_gpio_set(LIS2DH_INT1);

    if (!LIS2DH_init()) {
        log_msg(LOG_ERROR, __cfunc__, "LIS2DH not detected!");
        pthread_exit(NULL);
    } else {
        log_msg(LOG_INFO, __cfunc__, "LIS2DH detected");
    }

    //Exti configuration
    uint32_t regval;
    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_EXTI8, 0, 0, LIS2DH_INT1_intISR, LIS2DH_INT1_intDSR, &LIS2DH_int1_handle, &LIS2DH_int1_object);
    cyg_interrupt_configure(CYGNUM_HAL_INTERRUPT_EXTI8, false, true);
    cyg_interrupt_attach(LIS2DH_int1_handle);
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EXTI8);
    HAL_READ_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(8), regval);
    regval |= CYGHWR_HAL_STM32_SYSCFG_EXTICRX_PORTJ(8);
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(8), regval);

#endif


#if defined(BOARD_TR2) && !defined(BOARD_TR2_V2)
    uint16_t kp_input;
    // Initializing keypad
    if (!tca9535_init()) {
        log_msg(LOG_ERROR, __cfunc__, "TCA9535 not detected!");
        pthread_exit(NULL);
    } else {
        log_msg(LOG_INFO, __cfunc__, "TCA9535 detected");
    }

    while (true)
    {
        if (tca9535_read_input(&kp_input)) {
#ifdef LOW_POWER
            MarkActivity();
            if(lcd_state == LCD_OFF) {
                continue;
            }
#endif
            kp_input &= 0x3FFF;
            log_msg(LOG_INFO, __cfunc__, "Keypad input: = 0x%04X", kp_input);
            DispMan->ProcessKeyInput(kp_input>0, kp_input);
        }

    /*
    - Add button functionality, so that the pages of the demo application are switched (as working on the eval board) using the left/right button
    - Add button functionality, so that on the list view (only there), the list can be scrolled up/down using the UP/DOWN buttons (as we do it on the evaluation board)
    - Add button functionality (middle button, toggling ON/OFF) , so that the display backlight and LCD controller is switched off and the CPU frequency is lowered to allow for longer battery lifetime, as it is currently too short, since you need to walk around and cannot put the device into the charger all the time. Be aware that this is not yet the final power save process, it's only for the demo.
    */
    }
#endif

#ifdef BOARD_TR2_V2
    uint16_t kp_input, kp_touch;

    if(!tca6424_init()) {
        log_msg(LOG_ERROR, __cfunc__, "TCA6424 not detected!");
        pthread_exit(NULL);
    } else
        log_msg(LOG_INFO, __cfunc__, "TCA6424 detected");
     
    while(1) {     
        if (tca6424_read_input(&kp_input, &kp_touch)) {
#ifdef LOW_POWER
            gettimeofday(&last_activity, NULL);
            if(lcd_state == LCD_OFF) {
                continue;
            }
#endif
            kp_input &= 0x3FFF;
            kp_touch &= 0x0007;
            log_msg(LOG_INFO, __cfunc__, "Keypad input: = 0x%04X", kp_input);
            log_msg(LOG_INFO, __cfunc__, "Keypad touch: = 0x%04X", kp_touch);
            DispMan->ProcessKeyInput(kp_input>0, kp_input);
        }     
    }
#endif
    pthread_exit(NULL);
}

void *BatteryThread(void*)
{
#ifdef BOARD_TR2
    double battery_voltage;
    int notCharging = 1;
    int prevState = 1;
#ifdef LOW_POWER
    struct timeval lcd_timeout;
#endif
    hal_stm32_gpio_set(CHG_CHRG);
    hal_stm32_gpio_set(CHG_MODE);

    hal_stm32_gpio_out(CHG_MODE, 0);

    if(bm_init() != ENOERR)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error initializing battery monitor!");
        pthread_exit(NULL);
    }

    int battery_state = 0;

    while (true) {
        prevState = notCharging;

        hal_stm32_gpio_in(CHG_CHRG, &notCharging);

        if (notCharging) {
	    battery_voltage = bm_read();

            if(battery_voltage <= 3.24) {
                battery_state = 0;
            } else if(battery_voltage <= 3.48) {
                battery_state = 1;
            } else if(battery_voltage <= 3.72) {
                battery_state = 2;
            } else if(battery_voltage <= 3.96) {
                battery_state = 3;
            } else if(battery_voltage > 3.96) {
                battery_state = 4;
            }
        } else {
            battery_state = 5;
        }

        if (DispMan && DispMan->GetTopPanel()) {
            Image* state_image = (Image*) DispMan->GetTopPanel()->GetElement("Battery");
            if (state_image != NULL) {
                state_image->SetActiveFrame(battery_state);
            }
        }

        if (prevState > notCharging) {
            PWM_BUZZER_Beep(1000,50);
            msleep(50);
            PWM_BUZZER_Beep(1000,50);
            msleep(50);
            PWM_BUZZER_Beep(1000,50);
        } else if(prevState < notCharging) {
            MarkActivity();
        }

#ifdef LOW_POWER
        gettimeofday(&lcd_timeout, NULL);
        int32_t timeout_sec = lcd_timeout.tv_sec - last_activity.tv_sec;
        if((notCharging == 0 || (timeout_sec > appMainConfig.powerSaveTimeout)) && (lcd_state == LCD_ON) && !doNotSleep)
        {
            log_msg(LOG_INFO, __cfunc__, "LCD timout, turning off...");

            if(appMainConfig.cp6Ipv6 != "") {
                log_msg(LOG_INFO, __cfunc__, "Trying to notify the CP-6 that I'm sleepy!");
                if(JN_rest_request(REST_GET, "/power_save_enabled", "", NULL, 0) < 0) {
                    log_msg(LOG_ERROR, __cfunc__, "Unable to notify the CP-6 that I'm sleepy!");
                }
                else {
                    log_msg(LOG_INFO, __cfunc__, "CP-6 notified that I'm sleepy!");
                }
            }
            else {
                log_msg(LOG_ERROR, __cfunc__, "Not paired with CP-6! Nobody knows that I'm sleepy!");
            }

            sleepSystem();
            lcd_state = LCD_OFF;
        } else if((notCharging && (timeout_sec < appMainConfig.powerSaveTimeout)) && (lcd_state == LCD_OFF)) {
            log_msg(LOG_INFO, __cfunc__, "Activity! Waking-up LCD!");
            wakeUpSystem();
            lcd_state = LCD_ON;
        }

        Cyg_Watchdog::watchdog.reset();
#endif
        msleep(100); // TODO: why 1000 ?
    }
#endif
    pthread_exit(NULL);
}
