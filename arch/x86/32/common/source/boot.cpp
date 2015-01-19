#include "types.h"
#include "debug.h"
#include "debug_bochs.h"
#include "multiboot.h"
#include "offsets.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "paging-definitions.h"

#define PRINT(X) do { if (A_BOOT & OUTPUT_ENABLED) { writeLine2Bochs(VIRTUAL_TO_PHYSICAL_BOOT(X)); } } while (0)

extern "C" void _entry();
extern "C" void parseMultibootHeader();
extern "C" void initialiseBootTimePaging();
extern "C" void startup();

extern "C" void entry()
{
  extern multiboot_info_t* multi_boot_structure_pointer;
  asm("mov %%ebx,%0": "=m"(*((multiboot_info_t**)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&multi_boot_structure_pointer))));
  PRINT("Booting...\n");
  PRINT("Clearing Framebuffer...\n");
  ArchCommon::bzero(ArchCommon::getFBPtr(0),80 * 25 * 2);
  PRINT("Clearing BSS...\n");
  extern uint32 bss_start_address;
  extern uint32 bss_end_address;
  ArchCommon::bzero(VIRTUAL_TO_PHYSICAL_BOOT((pointer)&bss_start_address),(uint32)&bss_end_address - (uint32)&bss_start_address);
  asm("push $2\n"
      "popf\n");
  PRINT("Parsing Multiboot Header...\n");
  parseMultibootHeader();
  PRINT("Initializing Kernel Paging Structures...\n");
  initialiseBootTimePaging();
  PRINT("Setting CR3 Register...\n");
  asm("mov %[pd],%%cr3" : : [pd]"r"(VIRTUAL_TO_PHYSICAL_BOOT((pointer)ArchMemory::getRootOfKernelPagingStructure())));
  PRINT("Enable Page Size Extensions...\n");
  uint32 cr4;
  asm("mov %%cr4,%[v]\n" : [v]"=r"(cr4));
  cr4 |= 0x10;
  if (PAGE_DIRECTORY_ENTRIES == 512)
    cr4 |= 0x20;
  asm("mov %[v],%%cr4\n" : : [v]"r"(cr4));
  PRINT("Enable Paging...\n");
  asm("mov %cr0,%eax\n"
      "or $0x80010001,%eax\n"
      "mov %eax,%cr0\n");
  PRINT("Switch to our own stack...\n");
  extern uint8 boot_stack[];
  asm("mov %[v],%%esp\n"
      "mov %%esp,%%ebp\n" : : [v]"i"(&boot_stack));
  PRINT("Calling startup()...\n");
  asm("call *%%eax" : : "a"(startup));
  PRINT("Returned from startup()? This should never happen.\n");
  while(1);
}