#include "arch.hpp"
#include "util.hpp"
#include "x86.hpp"

extern "C" char _image_start[];
extern "C" char _image_end[];

static const uint32_t PTE_P = 1 << 0;
static const uint32_t PTE_W = 1 << 1;
static const uint32_t PTE_U = 1 << 2;
static const uint32_t PTE_PS = 1 << 7;

// Page directory. Kernel code is covered using 2MB entries here.
alignas(page_size) static uint32_t pdt[page_size / sizeof(uint32_t)];

// Page table for user code.
alignas(page_size) static uint32_t user_pt[page_size / sizeof(uint32_t)];

alignas(page_size) static char user_page_backing[page_size];

char *get_user_page_backing()
{
  return user_page_backing;
}

uintptr_t get_user_page()
{
  // XXX
  return 0;
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

void setup_arch()
{
  uintptr_t istart = reinterpret_cast<uintptr_t>(_image_start);
  uintptr_t iend = reinterpret_cast<uintptr_t>(_image_end);

  assert(is_aligned(istart, 22), "Image needs to start on large page boundary");

  // Map our binary 1:1
  for (uintptr_t c = istart; c <= iend; c += (1U << 22)) {
    uintptr_t idx = c >> 22;
    pdt[idx] = c | PTE_P | PTE_W | PTE_PS;
  }

  // TODO Map user page

  set_cr3((uintptr_t)pdt);
}