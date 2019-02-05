#include "arch.hpp"
#include "selectors.hpp"
#include "util.hpp"
#include "x86.hpp"

extern "C" char _image_start[];
extern "C" char _image_end[];

// Page directory. Kernel code is covered using 2MB entries here.
alignas(page_size) static uint32_t pdt[page_size / sizeof(uint32_t)];

// Page table for user code.
alignas(page_size) static uint32_t user_pt[page_size / sizeof(uint32_t)];

alignas(page_size) static char user_page_backing[page_size];

static tss tss;

char *get_user_page_backing()
{
  return user_page_backing;
}

uintptr_t get_user_page()
{
  // Needs to be in the reach of 16-bit code, so we don't need a different
  // mapping for 16-bit code.
  return 1UL << 20 /* MiB */;
}

exception_frame execute_user(uintptr_t rip)
{
  // XXX
  return {};
}

static bool is_aligned(uint64_t v, int order)
{
  assert(order < sizeof(v)*8, "Order out of range");
  return 0 == (v & ((uint64_t(1) << order) - 1));
}

static void setup_paging()
{
  uintptr_t istart = reinterpret_cast<uintptr_t>(_image_start);
  uintptr_t iend = reinterpret_cast<uintptr_t>(_image_end);

  assert(is_aligned(istart, 22), "Image needs to start on large page boundary");

  // Map our binary 1:1
  for (uintptr_t c = istart; c <= iend; c += (1U << 22)) {
    uintptr_t idx = c >> 22;
    pdt[idx] = c | PTE_P | PTE_W | PTE_PS;
  }

  // Map user page
  uintptr_t up = get_user_page();

  assert(up + page_size <= istart, "User page cannot be mapped into kernel area");
  pdt[bit_select(32, 22, up)] = reinterpret_cast<uintptr_t>(user_pt) | PTE_U | PTE_P;
  user_pt[bit_select(22, 12, up)] = reinterpret_cast<uintptr_t>(get_user_page_backing()) | PTE_U | PTE_P;

  set_cr4(get_cr4() | CR4_PSE);
  set_cr3((uintptr_t)pdt);
}

static void setup_gdt()
{
  static gdt_desc gdt[] {
    {},
    gdt_desc::kern_code32_desc(),
    gdt_desc::kern_data32_desc(),
    gdt_desc::tss_desc(&tss),
  };

  lgdt(gdt);
  ltr(ring0_tss_selector);

  // Reload code segment descriptor
  asm volatile ("ljmp %[selector], $1f\n"
                "1:\n"
                :: [selector] "i" (static_cast<uint32_t>(ring0_code_selector)));

  // Reload data segment descriptors
  asm volatile ("mov %0, %%ss\n"
                "mov %0, %%ds\n"
                "mov %0, %%es\n"
                "mov %0, %%fs\n"
                "mov %0, %%gs\n"
                :: "r" (ring0_data_selector));

  // XXX Setup code/stack segments for 32-bit and 16-bit protected mode.
}

static void setup_idt()
{
  // XXX Setup task gates for all interrupt vectors
}

void setup_arch()
{
  setup_paging();
  setup_gdt();
  setup_idt();
}
