#include "main.h"

volatile uint32_t psp;
volatile uint32_t msp;
CYG_INTERRUPT_STATE saved_state;
CYG_ADDRESS old_svc_handler;
uint32_t pc, sp;

ResetSource resetSource = Unknown;
GUIFont* font = NULL;
uint16_t kp_input, kp_touch;
bool keypadConnected = false;

void jumpToApp();
ResetSource detectResetSource();
int GetFileSize(const char* path);
bool ReadToBuffer(const char* path, uint8_t* &buffer);

uint32_t* backupRam = (uint32_t*)0x40024000;

void Reset() {
    SCB->AIRCR = 0x5FA0004;
    while (1)
        ;
}

void EnableBackupRam() {
    RCC->APB1ENR = RCC->APB1ENR | 1 << 28; //PWR on
    PWR->CR = PWR->CR | 1 << 8; //Access enable
    RCC->AHB1ENR = RCC->AHB1ENR | RCC_AHB1ENR_BKPSRAMEN; //Enable clock
}

void DrawProgressBar(int percentage) {
    FillRect(10, 575, 460, 50, LCD_COLOR_NEEOBLUE);
    DrawRect(10, 575, 460, 50, LCD_COLOR_WHITE);
    if(percentage > 0) {
        FillRect(10, 575, percentage * 4.6, 50, LCD_COLOR_WHITE);
    }
}

void Callback(int percentage) {
    DrawProgressBar(percentage);
}

void ClearScreen() {
    FillRect(0, 100, 480, 700, LCD_COLOR_NEEOBLUE);
}

void ShowMessage(const char* message, int line) {
    if(font == NULL) return;

    int messageLength = font->GetWidth(message);
    FillRect(0, 380 + 20*line, 480, 20, LCD_COLOR_NEEOBLUE);
    DisplayAntialiasedStringAt(font, (GetXSize() / 2) - (messageLength / 2), 380 + 20*line, message, LCD_COLOR_WHITE);
}

void WaitForKey(uint16_t keys){
    if(!keypadConnected) return;
    do { //Wait for key
       tca6424_read_input(&kp_input, &kp_touch);
       cyg_thread_delay(20);
   } while (kp_input != keys);

   do { //Wait for release
       tca6424_read_input(&kp_input, &kp_touch);
       cyg_thread_delay(20);
   } while (kp_input);
}

void WaitForRelease(){
    if(!keypadConnected) return;
   do { //Wait for release
       tca6424_read_input(&kp_input, &kp_touch);
       cyg_thread_delay(20);
   } while (kp_input);
}

uint16_t GetKey(){
    if(!keypadConnected) return 0;
    do { //Wait for key
       tca6424_read_input(&kp_input, &kp_touch);
       cyg_thread_delay(20);
    } while (!kp_input);

    uint16_t key = kp_input;

    do { //Wait for release
       tca6424_read_input(&kp_input, &kp_touch);
       cyg_thread_delay(20);
    } while (kp_input);

    return key;
}

int main(int argc, char *argv[]) {
    printf(">>>>>>>>>BOOTLOADER<<<<<<<<<<\r\n");
    bool updatePending = false;
    bool developerMode = false;
    BSP_SDRAM_Init();
    custom_mempool_init();

    //Initialize keypad
    if(!tca6424_init()) {
        printf("Keypad not ready\r\n");
    } else {
        printf("Keypad ready\r\n");
        keypadConnected = true;
    }

    if(keypadConnected && tca6424_read_input(&kp_input, &kp_touch)) {
        printf("Key state: 0x%04x 0x%04x\r\n", kp_input, kp_touch);
    }

    resetSource = detectResetSource();

    const uintptr_t flashAddr = 0x08000000;
    const uintptr_t imageOffset = 0x00040000;
    const uintptr_t headerSize = sizeof(int) * 3;
    const uintptr_t loadAddr = flashAddr + imageOffset;
    const uintptr_t appAddr = loadAddr + headerSize;

    EnableBackupRam();

    if(backupRam[0] == 1) {
        printf("Firmware update\r\n");
        backupRam[0] = 0; //Disable auto update
        updatePending = true;
    }

    uint32_t ROM_CRC = 0;
    uint32_t IMAGE_CRC = ((uint32_t *)loadAddr)[0];
    uint32_t LEN = ((uint32_t *)loadAddr)[1];
    uint32_t stackPointer = ((uint32_t *)appAddr)[0];

    if(LEN == 0xFFFFFFFF && IMAGE_CRC == 0xFFFFFFFF && stackPointer != 0xFFFFFFFF) {
        printf("Developer mode\r\n");
        developerMode = true;
    } else {
        if(LEN > 2048 * 1024 - imageOffset - headerSize) {
            LEN = 2048 * 1024 - imageOffset - headerSize;
        }
        printf("Checking CRC \r\n");
        ROM_CRC = cyg_posix_crc32((uint8_t*)appAddr, LEN);
        if(IMAGE_CRC == ROM_CRC) {
            printf("\tCRC Correct\r\n");
        } else {
            printf("\tCRC Incorrect\r\n");
        }
    }

    if(kp_input || updatePending || (!developerMode && IMAGE_CRC != ROM_CRC)) { //Update

        printf("Initializing ROMFS\r\n");
        if(romfs_mount()) {
            printf("Error while mounting romfs.");
        }

        printf("Loading font\r\n");
        font = new GUIFont("small.font"); //Taken from ROMFS

        printf("Initializing LCD \r\n");
        hal_stm32_gpio_set(LCDBL_PWM);
        hal_stm32_gpio_set(LCD_RESET);
        hal_stm32_gpio_out(LCD_RESET, RESET_LEVEL);
        cyg_thread_delay(100);
        hal_stm32_gpio_out(LCD_RESET, !RESET_LEVEL);
        LTDC_MUX_Init();
        LTDC_Init();

        LG4573A_Init();
        hal_stm32_gpio_out(LCDBL_PWM, 1);
        uint8_t* framebuffer = (uint8_t*)custom_malloc(800 * 480 * 2);
        LTDC_SetConfig(LCD_FOREGROUND_LAYER, 480, 800, PIXEL_FORMAT_ARGB4444, (uint32_t)framebuffer);
        DrawInit(480, 800, (uint32_t)framebuffer);
        FillRect(0, 0, 480, 800, LCD_COLOR_NEEOBLUE);
        InternalFlash_RegisterBlockCallback(Callback);

        DisplayAntialiasedStringAt(font, 10, 10, "NEEO Bootloader V0.0.3 ALPHA", LCD_COLOR_WHITE);
        uint16_t key = 0;
        if(!developerMode && IMAGE_CRC != ROM_CRC){ //Flashing error
            ShowMessage("Flashing error detected.", 0);
            ShowMessage("Press OK to retry.", 1);
            ShowMessage("Press MENU to recovery.", 2);
            WaitForRelease();
            while((key & (KEY_MENU | KEY_OK)) == 0) {key = GetKey();};
            if(key == KEY_MENU){
                ClearScreen();
                ShowMessage("Recovery not available yet! - retrying.", 0);
                cyg_thread_delay(2000);
            }
        } else if (kp_input) { //key triggered
            ShowMessage("What to do?", 0);
            ShowMessage("Press OK to flash the device.", 1);
            ShowMessage("Press BACK to reset.", 2);
            ShowMessage("Press MENU to recovery.", 3);
            WaitForRelease();
            while((key & (KEY_MENU | KEY_BACK | KEY_OK)) == 0) {key = GetKey();};
            if(key == KEY_MENU){
                ClearScreen();
                ShowMessage("Recovery not available yet! - retrying.", 0);
                cyg_thread_delay(2000);
            } else if (key == KEY_BACK) {
                ClearScreen();
                ShowMessage("Resetting...", 0);
                cyg_thread_delay(2000);
                Reset();
            }
        }

        ClearScreen();
        ShowMessage("Preparing image...", 0);

        char* path = "/flash/image"; //Could be /flash/recovery for the recovery or provided by backup ram.

        printf("Mounting flash... \r\n");
        if(flashfs_mount()) {
            printf("Error while mounting romfs\r\n");
        } else {
            InternalFlash_Unlock();
            printf("Programming... \r\n");

            printf("Loading image to the RAM\r\n");
            int fileSize = GetFileSize(path);

            if(fileSize == -1) { //Firmware doesn't exist.
                //TODO error handling - not tested
                ClearScreen();
                ShowMessage("Firmware image doesn't exist.", 0);
                ShowMessage("Flashing recovery firmware.", 1);
                cyg_thread_delay(2000);
                char* recoveryPath = "/flash/recovery";
                int recoveryFileSize = GetFileSize(recoveryPath);
                if(recoveryFileSize == -1){
                    ShowMessage("Recovery image doesn't exist.", 3);
                    ShowMessage("Press OK to reset.", 4);
                    WaitForKey(KEY_OK);
                    Reset();
                }
            }

            uint8_t* data = (uint8_t*)custom_malloc(fileSize);
            ReadToBuffer(path, data);

            printf("Checking image CRC \r\n");
            uint32_t RAM_CRC = cyg_posix_crc32((uint8_t*)data + headerSize, fileSize - headerSize);
            uint32_t RAM_IMAGE_CRC = ((uint32_t *)data)[0];

            if(RAM_CRC == RAM_IMAGE_CRC) {
                printf("\tCRC Correct\r\n");
                ClearScreen();
                ShowMessage("Writing image...", 5);
                InternalFlash_ProgramBinary(loadAddr, data, fileSize);
                ShowMessage("Programming finished.", 0);
                ShowMessage("Press OK.", 1);
                WaitForKey(KEY_OK);
            } else {
                printf("\tCRC Incorrect - image corrupted\r\n");
                ShowMessage("Image corrupted. Press OK to reset.", 0);
                WaitForKey(KEY_OK);
            }

            printf("Programming finished \r\n");

            InternalFlash_Lock();
            Reset();
        }
    }

    if(keypadConnected){
        tca6424_deinit();
    }
    printf(">>>STARTING THE APPLICATION<<<\r\n\r\n");

    sp = ((uint32_t *)appAddr)[0]; //stack pointer at the end of the internal memory
    pc = ((uint32_t *)appAddr)[1]; //first instruction

    old_svc_handler = hal_vsr_table[CYGNUM_HAL_VECTOR_SERVICE];
    hal_vsr_table[CYGNUM_HAL_VECTOR_SERVICE] = (CYG_ADDRESS)jumpToApp;
    asm ( "swi 0;" );
    hal_vsr_table[CYGNUM_HAL_VECTOR_SERVICE] = old_svc_handler;
    HAL_DISABLE_INTERRUPTS(saved_state);
    int i;
    for (i = 0; i < 16; ++i) {
        hal_interrupt_mask(i);
    }
    asm (
    "msr msp, %0;"
    "msr psp, %0;" //what should be here?
    "mov r0, %1;"
    "blx r0;"
    :: "r" (sp), "r" (pc)

    : "r0"
    );

    printf("Something went wrong...\r\n");

    while (1) {
        cyg_thread_delay(1000);
    }
}

void jumpToApp() {
    HAL_DISABLE_INTERRUPTS(saved_state);

    asm (
    "mrs     %0, msp;"
    "mrs     %1, psp;"
    "mov     r0,#0;"
    "msr     control,r0;"
    : "=r" (msp), "=r" (psp):: "r0"
    );

    asm ( "isb; dsb; dmb; " );
}

ResetSource detectResetSource() {
    ResetSource source = Unknown;
    printf("Reset source:\r\n");
    if(RCC->CSR & RCC_CSR_PORRSTF) {
        printf("\tPower On\r\n");
        source = PowerOn;
    } else if(RCC->CSR & RCC_CSR_SFTRSTF) {
        printf("\tSoftware reset\r\n");
        source = Software;
    } else if(RCC->CSR & RCC_CSR_WDGRSTF) {
        printf("\tWatchdog reset\r\n");
        source = Watchdog;
    } else if(RCC->CSR & RCC_CSR_LPWRRSTF) {
        printf("\tLow-power reset\r\n");
        source = LowPower;
    } else if(RCC->CSR & RCC_CSR_WWDGRSTF) {
        printf("\tWindow watchdog reset\r\n");
        source = WindowWatchdog;
    } else if(RCC->CSR & RCC_CSR_PORRSTF) {
        printf("\tPOR-PDR reset\r\n");
        source = PORPDR;
    } else if(RCC->CSR & RCC_CSR_PADRSTF) {
        printf("\tPIN reset\r\n");
        source = PIN;
    } else if(RCC->CSR & RCC_CSR_BORRSTF) {
        printf("\tBOR reset\r\n");
        source = BOR;
    }

    RCC->CSR = RCC_CSR_RMVF; //Clear flags
    return source;
}

int GetFileSize(const char* path) {
    struct stat file_stat;
    if(stat(path, &file_stat) == -1) {
        return -1;
    }

    return file_stat.st_size;
}

bool ReadToBuffer(const char* path, uint8_t* &buffer) {
    uint32_t offset = 0;
    size_t read_bytes;

    FILE *file = fopen(path, "rb");
    if(file == NULL) return false;

    while ((read_bytes = fread(buffer + offset, 1, 128, file)) > 0) {
        offset += read_bytes;
    }

    if(!feof(file) || ferror(file)) {
        fclose(file);
        return false;
    }
    fclose(file);
    return true;
}
