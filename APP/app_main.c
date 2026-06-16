#include "APP_main.h"
#include "arm_math.h"

//配置测试定义变量
static uint8_t fast_buffer[1024] __attribute__((section(".ccmram")));
//在 CCM 中定义一个大数组，模拟“堆空间”
// 确保.sct 文件里已经有了 RW_CCMRAM 区域且包含 *(.ccmram)
static uint8_t ccm_heap[4096] __attribute__((section(".ccmram")));

//函数声明(仅内部使用)
static void config_test_init(void);
static void config_test_loop(void);
static void test_ccm_integrity(void);
static void trace_stack_in_ccm(int depth);


//函数实现  
void app_init(void) {
    config_test_init();
}
void app_loop(void) {
    config_test_loop();
}

static void config_test_init(void) {
    //CCM测试，通过map文件查看地址是否为0x10000000判断是否正确
    fast_buffer[0] = 0xAA; // 正常使用
    test_ccm_integrity();
    trace_stack_in_ccm(0);

    //DSP测试，可以通过调试看data是否接近0.5以此判断dsp库是否正确安装，测试后可删除
    static float data;
    data = arm_cos_f32(PI/3) ;
    (void)data; 
    
}
static void config_test_loop(void) {
    // 编码对齐测试：如果你看到的是正常汉字，说明设置正确；如果是乱码，请修改 Encoding 为 GB2312
    //点灯测试，我板子的调试小灯对应PC3，可根据需求自行更改，确保正确移植，测试后可删除
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_3);
    HAL_Delay(5000);
}

static void test_ccm_integrity(void) {
    //printf("Starting CCM Integrity Test...\r\n");
    //printf("CCM Heap Address: 0x%p\r\n", (void*)ccm_heap);

    // 检查地址是否在 0x10000000 范围内
    if ((uint32_t)ccm_heap < 0x10000000 || (uint32_t)ccm_heap > 0x10010000) {
        //printf("ERROR: ccm_heap is NOT in CCM area!\r\n");
        return;
    }

    // 2. 写入测试数据
    for (int i = 0; i < 4096; i++) {
        ccm_heap[i] = (uint8_t)(i % 256);
    }

    // 3. 读取并校验
    int errors = 0;
    for (int i = 0; i < 4096; i++) {
        if (ccm_heap[i] != (uint8_t)(i % 256)) {
            errors++;
        }
    }

    if (errors == 0) {
        //printf("SUCCESS: CCM Read/Write verified!\r\n");
    } else {
        //printf("FAILED: Found %d errors in CCM!\r\n", errors);
    }
}

static void trace_stack_in_ccm(int depth) {
    uint32_t local_var; // 这个局部变量一定在栈上！
    
    // 打印（或在监视窗口看）局部变量的地址
    // &local_var 就是当前栈指针的位置
    static uint32_t last_addr = 0;
    uint32_t current_addr = (uint32_t)&local_var;

    // 验证地址是否在 CCM 范围内
    if (current_addr >= 0x10000000 && current_addr <= 0x10010000) {
        // 计算向下生长的偏移
        int growth = (last_addr == 0) ? 0 : (int)(last_addr - current_addr);
        (void)growth;
        last_addr = current_addr;
        
        // 这里你可以打断点看 current_addr
        __NOP(); 
    }

    // 递归调用一下，看看栈是不是真的在减小（向下生长）
    if (depth < 3) {
        trace_stack_in_ccm(depth + 1);
    }
}

